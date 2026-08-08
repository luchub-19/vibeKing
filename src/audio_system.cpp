#include "audio_system.h"
#include <cmath>
#include <random>

namespace {
    constexpr float PI_F = 3.14159265358979323846f;

    // std::mt19937 seed 1 lần bằng random_device (entropy thật từ OS) rồi tái dùng cho
    // toàn bộ noise waveform. rand() cũ dùng chung 1 seed toàn cục kém chất lượng và
    // dễ trùng chu kỳ giữa các lần chạy - mt19937 cho phân bố đều và chất lượng cao hơn.
    std::mt19937& NoiseRng() {
        static std::mt19937 rng(std::random_device{}());
        return rng;
    }

    // ===== NHAC NEN PROCEDURAL (AudioSystem::FillMusicBuffer) =====

    // Cung goc voi 4 not bassline (220/196/174.6/164.8 Hz = A3/G3/F3/E3 - xem Init()),
    // nhung khai bao RIENG o day (khong doc mang `notes` cuc bo trong Init(), chi song
    // trong scope ham do) de FillMusicBuffer() khong phu thuoc chi tiet trien khai cua
    // Init(). Trung gia tri la CHU DICH: nhac nen la "ho hang" cung 1 motif voi bassline,
    // chi khac octave/thu tu, khong phai giai dieu ngoai lai.
    constexpr float kMusicMotif[4] = { 220.0f, 196.0f, 174.6f, 164.8f };

    inline float SemitoneRatio(float semitones) { return powf(2.0f, semitones / 12.0f); }

    // sin(2*pi*f*t)/kiem tra dau truc tiep se MAT DAN chinh xac khi t lon (phien choi cang
    // dai cang ro) do gioi han chu so co nghia cua double khi nhan voi 1 gia tri lon - wrap
    // PHA ve [0,1) truoc bang fmod() thay vi de f*t tu do phinh to vo han. Dung double cho
    // ca freq/t (khong phai float) vi day la noi DUY NHAT quyet dinh cao do co dung hay
    // khong qua thoi gian dai; ket qua tra ve luon nam trong [-1,1] nen ep ve float sau
    // cung khong mat gi.
    inline float OscSquare(double freq, double t) {
        double phase = fmod(freq * t, 1.0);
        return (phase < 0.5) ? 1.0f : -1.0f;
    }
    inline float OscSine(double freq, double t) {
        double phase = fmod(freq * t, 1.0);
        return (float)sin(2.0 * (double)PI_F * phase);
    }

    // Envelope hinh thang: attack/release NGAN (giay) o 2 dau moi not, phang o giua -
    // not lien tuc noi duoi nhau (khac 1 Sound doc lap chi can fade-out nhu GenerateTone()
    // o tren) nen can CA fade-in, khong chi fade-out, moi tranh duoc tieng "click" o CA 2
    // dau not.
    inline float NoteEnvelope(float tInNote, float noteLen, float attack, float release) {
        if (tInNote < attack) return tInNote / attack;
        if (tInNote > noteLen - release) return fmaxf(0.0f, (noteLen - tInNote) / release);
        return 1.0f;
    }
}

Sound AudioSystem::GenerateTone(float frequency, float durationSec, int waveType) {
    const int sampleRate = 44100;
    const int sampleCount = (int)(sampleRate * durationSec);

    // Cap phat buffer qua RL_MALLOC (macro cong khai trong raylib.h, mac dinh = malloc)
    // - Wave SO HUU that su buffer nay, UnloadWave() ben duoi giai phong dung chuan qua
    // RL_FREE, khop voi idiom chinh thuc cua chinh raylib (LoadSound() trong raudio.c).
    short* data = (short*)RL_MALLOC(sizeof(short) * (size_t)sampleCount);

    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / (float)sampleRate;
        float envelope = 1.0f - (float)i / (float)sampleCount; // Fade out, tránh tiếng "click" khi cắt đột ngột
        float value;

        if (waveType == 0) {
            value = sinf(2.0f * PI_F * frequency * t);
        } else if (waveType == 1) {
            value = (sinf(2.0f * PI_F * frequency * t) >= 0.0f) ? 1.0f : -1.0f;
        } else {
            static std::uniform_real_distribution<float> noiseDist(-1.0f, 1.0f);
            value = noiseDist(NoiseRng());
        }

        data[i] = (short)(value * envelope * 12000.0f);
    }

    Wave wave{};
    wave.frameCount = (unsigned int)sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound sound = LoadSoundFromWave(wave); // Copy sang AudioBuffer rieng cua raylib
    UnloadWave(wave);                      // Giai phong buffer RL_MALLOC ngay - dung idiom chuan
    return sound;
}

void AudioSystem::Init() {
    if (initialized) return;
    InitAudioDevice();

    sfxShoot.Init(GenerateTone(880.0f, 0.08f, 1));
    sfxHit.Init(GenerateTone(200.0f, 0.15f, 2));
    sfxExplosion.Init(GenerateTone(120.0f, 0.30f, 2));
    sfxPickup.Init(GenerateTone(990.0f, 0.12f, 1));    // Cao & ngan, tach biet ro voi sfxShoot (880Hz)
    sfxUfoAppear.Init(GenerateTone(500.0f, 0.35f, 0)); // Tieng "vo vo" trung binh khi UFO xuat hien
    sfxUfoHit.Init(GenerateTone(1200.0f, 0.45f, 1));   // Cao & dai hon pickup - cam giac "trung thuong lon"
    sfxCleanser.Init(GenerateTone(80.0f, 0.5f, 2));    // Am tram + noise - cam giac "no bom don gian"
    sfxBossPhase.Init(GenerateTone(350.0f, 0.25f, 1)); // Vuong, trung binh - tach biet ro voi moi sfx khac (880/990/500/1200/200/120/80Hz)

    sfxGameOver   = GenerateTone(110.0f, 0.80f, 0);
    sfxWaveClear  = GenerateTone(660.0f, 0.60f, 0);
    sfxBossDefeat = GenerateTone(150.0f, 1.2f, 2);     // Dai va tram nhat - cam giac "no lon"

    // 4 nốt giảm dần - đặc trưng bassline "duh-duh-duh-duh" của Space Invaders gốc.
    // Bassline luon phat TUAN TU (1 not/lan, cach nhau it nhat 0.2s - xem UpdateBassline)
    // nen khong bao gio bi trung lap/can voice pool nhu cac sfx phan ung theo hanh dong.
    const float notes[4] = { 220.0f, 196.0f, 174.6f, 164.8f };
    for (int i = 0; i < 4; i++) bassNotes[i] = GenerateTone(notes[i], 0.15f, 0);

    // A8: hi-hat - noise TRANG (uniform, xem GenerateTone() o tren: waveType != 0/1 bo
    // qua tham so frequency, chi random moi sample) RAT ngan (0.04s) de nghe nhu 1 tieng
    // "tsk" gon. Khac bass (sinf() thuan, cao do RO RANG ~165-220Hz - xem mang `notes`
    // o tren) ca ve BAN CHAT (dai pho vs 1 tan so don) lan DO DAI (0.04s vs 0.15s) nen
    // tach biet ro, khong lan vao nhau du 2 lop vang GAN NHAU ve mat thoi gian.
    hiHat = GenerateTone(2400.0f, 0.04f, 2);

    // NHAC NEN PROCEDURAL: AudioStream mono 16-bit 44100Hz - GIONG het tham so sampleRate/
    // sampleSize/channels ma moi Sound o tren dung (xem GenerateTone()), nhung day la 1
    // STREAM lien tuc (khong phai buffer co dinh) nen phai LoadAudioStream()+
    // PlayAudioStream() thay vi LoadSoundFromWave(). PlayAudioStream() chi "mo van" phat -
    // stream con RONG cho toi khi UpdateMusic() lan dau day PCM vao qua UpdateAudioStream()
    // (raylib tu phat im lang trong luc cho, khong crash).
    musicStream = LoadAudioStream(44100, 16, 1);
    PlayAudioStream(musicStream);
    musicSampleCursor = 0.0;
    musicTensionLerp = 0.0f;

    SetVolume(masterVolume);
    initialized = true;
}

void AudioSystem::Shutdown() {
    if (!initialized) return;
    sfxShoot.Unload();
    sfxHit.Unload();
    sfxExplosion.Unload();
    sfxPickup.Unload();
    sfxUfoAppear.Unload();
    sfxUfoHit.Unload();
    sfxCleanser.Unload();
    sfxBossPhase.Unload();

    UnloadSound(sfxGameOver);
    UnloadSound(sfxWaveClear);
    UnloadSound(sfxBossDefeat);
    for (int i = 0; i < 4; i++) UnloadSound(bassNotes[i]);
    UnloadSound(hiHat);
    UnloadAudioStream(musicStream);
    CloseAudioDevice();
    initialized = false;
}

void AudioSystem::PlayShoot()     { sfxShoot.Play(); }
void AudioSystem::PlayExplosion() { sfxExplosion.Play(); }
void AudioSystem::PlayHit()       { sfxHit.Play(); }
void AudioSystem::PlayGameOver()  { PlaySound(sfxGameOver); }
void AudioSystem::PlayWaveClear() { PlaySound(sfxWaveClear); }
void AudioSystem::PlayPickup()    { sfxPickup.Play(); }
void AudioSystem::PlayUfoAppear() { sfxUfoAppear.Play(); }
void AudioSystem::PlayUfoHit()     { sfxUfoHit.Play(); }
void AudioSystem::PlayCleanser()   { sfxCleanser.Play(); }
void AudioSystem::PlayBossDefeat() { PlaySound(sfxBossDefeat); }
void AudioSystem::PlayBossPhase()  { sfxBossPhase.Play(); }

void AudioSystem::UpdateBassline(float dt, float enemySpeed, float enemySpeedMax) {
    float speedRatio = enemySpeedMax > 0.0f ? (enemySpeed / enemySpeedMax) : 0.0f;
    if (speedRatio > 1.0f) speedRatio = 1.0f;
    float interval = 0.55f - 0.35f * speedRatio; // 0.55s (chậm) -> 0.20s (gấp gáp)

    bassTimer += dt;
    if (bassTimer >= interval) {
        bassTimer = 0.0f;
        PlaySound(bassNotes[bassIndex]);
        bassIndex = (bassIndex + 1) % 4;
    }

    // A8: hi-hat tick DEU dan voi nhip do GAP DOI bass (hiHatInterval = interval/2) qua
    // 1 timer RIENG (hiHatTimer, khong dung chung/doc bassTimer) - nghia la hi-hat va
    // bass CHIA se 1 nua so lan tick voi nhau (rieng: 0.5*interval, 1.5*interval...;
    // trung voi bass: 1.0*interval, 2.0*interval...) va NUA con lai hi-hat vang MOT
    // MINH xen giua 2 not bass - dung kieu "hi-hat 8 nghich pham deu tren bass 4 nghich
    // pham" pho bien, khong phai kieu offbeat tuyet doi (khong bao gio trung). Cung
    // scale theo speedRatio nhu bass (qua `interval`) de hi-hat gap gap dan dung nhip
    // voi bass khi dich tien gan, khong bi "tut lai" phia sau.
    hiHatTimer += dt;
    float hiHatInterval = interval * 0.5f;
    if (hiHatTimer >= hiHatInterval) {
        hiHatTimer = 0.0f;
        PlaySound(hiHat);
    }
}

void AudioSystem::SetVolume(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    masterVolume = v;
    sfxShoot.SetVolume(v);
    sfxHit.SetVolume(v);
    sfxExplosion.SetVolume(v);
    sfxPickup.SetVolume(v);
    sfxUfoAppear.SetVolume(v);
    sfxUfoHit.SetVolume(v);
    sfxCleanser.SetVolume(v);
    sfxBossPhase.SetVolume(v);
    SetSoundVolume(sfxGameOver, v);
    SetSoundVolume(sfxWaveClear, v);
    SetSoundVolume(sfxBossDefeat, v);
    for (int i = 0; i < 4; i++) SetSoundVolume(bassNotes[i], v * 0.7f); // Bass nhỏ hơn SFX chính 1 chút
    SetSoundVolume(hiHat, v * 0.35f); // Hi-hat chi la lop nen tinh te, nho hon ca bass
    SetAudioStreamVolume(musicStream, v * Config::MUSIC_MASTER_GAIN); // Nhac nen: cung 1 nut volume duy nhat voi moi SFX khac, chi nho hon theo MUSIC_MASTER_GAIN
}

void AudioSystem::UpdateMusic(float dt, const MusicContext& ctx) {
    if (!initialized) return; // Chua Init() (vd trong unit test headless) -> AudioStream chua ton tai, no-op an toan giong moi ham Update/Play khac

    // Tron MUOT toi trang thai "nguy hiem" (mang <= MUSIC_LOW_LIVES_THRESHOLD) qua dt,
    // KHONG bat/tat dot ngot ngay khung hinh vua mat mang - xem `tension` trong
    // FillMusicBuffer() (dieu khien tremolo + not nghich).
    float tensionTarget = (ctx.livesRemaining <= Config::MUSIC_LOW_LIVES_THRESHOLD) ? 1.0f : 0.0f;
    constexpr float kTensionLerpRate = 1.5f; // 1/giay - ~0.67s de tron xong 100%, du cham de khong nghe "giat"
    if (musicTensionLerp < tensionTarget) {
        musicTensionLerp = fminf(musicTensionLerp + kTensionLerpRate * dt, tensionTarget);
    } else if (musicTensionLerp > tensionTarget) {
        musicTensionLerp = fmaxf(musicTensionLerp - kTensionLerpRate * dt, tensionTarget);
    }

    // Chi sinh chunk PCM MOI khi raylib da phat HET chunk truoc - khong phai moi frame
    // game deu can sinh am thanh moi (chunk ~2048/44100s ~ 46ms, dai hon 1 frame 60fps).
    if (!IsAudioStreamProcessed(musicStream)) return;

    FillMusicBuffer(musicBuffer, Config::MUSIC_STREAM_BUFFER_FRAMES, ctx);
    UpdateAudioStream(musicStream, musicBuffer, Config::MUSIC_STREAM_BUFFER_FRAMES);
    musicSampleCursor += Config::MUSIC_STREAM_BUFFER_FRAMES;
}

// ==========================================
// FillMusicBuffer(): sinh `frameCount` sample PCM mono 16-bit tiep theo, bat dau tu thoi
// diem TUYET DOI musicSampleCursor/44100 giay (KHONG phai 0 moi lan goi - xem comment
// musicSampleCursor trong audio_system.h) - moi tham so am nhac (tempo/octave/hop am/
// tension) doc TRUC TIEP tu `ctx` do GameManager truyen vao MOI FRAME (UpdatePlaying()),
// nen phan ung ngay khi wave/Boss/mang/combo doi, khong tre hon 1 chunk (~46ms).
//
// 2 lop am tron voi nhau:
//   - LEAD : arpeggio vuong (OscSquare), dao qua kMusicMotif - tempo theo enemySpeedRatio,
//     transpose theo wave, doi thu tu/octave khi Boss active, "sparkle" quang 8 khi combo cao.
//   - PAD  : 1 not sine giu dai = goc cua chu ky 4-not hien tai, doi giua 2 hop am moi
//     chu ky (chuyen dong hoa am cham, tron mem hon lead).
// Cang gan het mang (tension) -> tremolo bien do + not thu 2 cua lead ha 1/2 cung (quang
// nghich thoang qua).
// ==========================================
void AudioSystem::FillMusicBuffer(short* buffer, int frameCount, const MusicContext& ctx) {
    constexpr double sampleRate = 44100.0;

    // TEMPO: cung cong thuc "cang nhanh dich cang don" nhu UpdateBassline(), nhung mien
    // gia tri RIENG (MUSIC_ARPEGGIO_*) - lead luon gap hon lop bass nen phia duoi ~2x
    // thay vi trung khop tuyet doi tung nhip.
    float interval = Config::MUSIC_ARPEGGIO_INTERVAL_MIN +
        (Config::MUSIC_ARPEGGIO_INTERVAL_MAX - Config::MUSIC_ARPEGGIO_INTERVAL_MIN) * (1.0f - ctx.enemySpeedRatio);
    if (ctx.bossActive) interval *= Config::MUSIC_BOSS_INTERVAL_MUL; // Boss active -> don dap hon han

    // TRANSPOSE: cu moi MUSIC_WAVE_SECTION_LEN wave, len 1 bac (toi da
    // MUSIC_MAX_TRANSPOSE_SECTIONS bac) - cang danh xa cang cang thang. Boss dung octave/
    // thu tu not RIENG ben duoi nen KHONG transpose theo wave, giu on dinh du dang wave nao.
    int section = 0;
    if (!ctx.bossActive) {
        section = ctx.wave / Config::MUSIC_WAVE_SECTION_LEN;
        if (section > Config::MUSIC_MAX_TRANSPOSE_SECTIONS) section = Config::MUSIC_MAX_TRANSPOSE_SECTIONS;
    }
    float transposeRatio = SemitoneRatio((float)section * Config::MUSIC_TRANSPOSE_SEMITONES_PER_SECTION);

    float tension = musicTensionLerp; // Da tron muot san trong UpdateMusic(), dung thang o day

    for (int i = 0; i < frameCount; i++) {
        double t = (musicSampleCursor + (double)i) / sampleRate; // Thoi gian TUYET DOI (giay) tu luc AudioStream bat dau - khong reset ve 0 moi chunk

        double noteCount = floor(t / (double)interval);
        int rawStep = (int)fmod(noteCount, 4.0);
        float tInNote = (float)(t - noteCount * (double)interval);
        int leadNoteIndex = ctx.bossActive ? (3 - rawStep) : rawStep; // Boss: dao nguoc thu tu not - motif nghe "gian" hon

        float octaveMul = ctx.bossActive ? 1.0f : 2.0f; // Thuong: len 1 quang 8 tren bass (220Hz ho) tranh chua am; Boss: bassline da IM LANG luc nay (UpdatePlaying() chi goi UpdateBassline() khi !isBossWave) nen dung chung octave voi bass van an toan
        float leadFreq = kMusicMotif[leadNoteIndex] * octaveMul * transposeRatio;
        if (leadNoteIndex == 1) leadFreq *= SemitoneRatio(-1.0f * tension); // Not thu 2 ha dan 1/2 cung khi cang thang -> quang nghich (minor 2nd) thoang qua, bao hieu nguy hiem

        float leadEnv = NoteEnvelope(tInNote, interval, 0.012f, 0.05f);
        float lead = OscSquare(leadFreq, t) * leadEnv * Config::MUSIC_LEAD_VOLUME_MUL;
        if (ctx.comboCount >= 5) {
            // Combo cao -> them 1 lop "sparkle" nho, cao hon 1 quang 8 nua - thuong cam
            // giac cho loi choi tot, khong doi tempo/hop am (chi la lop diem xuyet).
            lead += OscSquare(leadFreq * 2.0f, t) * leadEnv * 0.25f;
        }

        // PAD: giu 1 not = goc cua chu ky 4-not hien tai, doi giua 2 hop am (I / vi-ish,
        // motif[0]=A3 va motif[2]=F3) moi chu ky cho co chuyen dong hoa am cham ben duoi,
        // tron mem (sine) chu khong sac (vuong) nhu lead.
        int cycleIndex = (int)floor(t / ((double)interval * 4.0));
        int padNoteIndex = ((cycleIndex % 2) == 0) ? 0 : 2;
        float padFreq = kMusicMotif[padNoteIndex] * 0.5f * transposeRatio; // 1 quang 8 duoi bass
        float pad = OscSine(padFreq, t) * Config::MUSIC_PAD_VOLUME_MUL;

        // TREMOLO: chi ro rang khi tension cao (LFO bien do quanh 1.0)
        float tremolo = 1.0f - tension * 0.35f *
            (0.5f + 0.5f * (float)sin(2.0 * (double)PI_F * (double)Config::MUSIC_TENSION_TREMOLO_HZ * t));

        float mixed = (lead + pad) * tremolo * Config::MUSIC_MASTER_GAIN;
        if (mixed > 1.0f) mixed = 1.0f;
        else if (mixed < -1.0f) mixed = -1.0f;
        buffer[i] = (short)(mixed * 12000.0f); // Cung bien do voi GenerateTone() (12000/32767 - con du du chieu truoc khi clipping)
    }
}
