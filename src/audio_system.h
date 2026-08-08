#pragma once
#include "raylib.h"
#include "voice_pool.h"
#include "config.h"

// NGU CANH GAMEPLAY hien tai de UpdateMusic() quyet dinh tempo/hop am/lop am cua nhac
// nen - GameManager nap lai gia tri MOI FRAME tu UpdatePlaying() (xem game_manager.cpp)
// va truyen vao, AudioSystem KHONG tu giu 1 ban sao rieng cua cac gia tri nay (tranh 2
// noi cung giu 1 nguon su that, dung tinh than canh bao da co san trong events.h).
struct MusicContext {
    float enemySpeedRatio = 0.0f; // enemySpeed/enemySpeedMax, giong het tham so UpdateBassline dang dung
    int wave = 1;
    bool bossActive = false;
    int livesRemaining = 3;
    int comboCount = 0;
};

// Sinh & phát âm thanh hoàn toàn bằng procedural synthesis (waveform tự tạo trong RAM).
// Lý do không dùng file .wav rời: tránh crash "file not found" khi build trên máy khác,
// và giữ project gọn nhẹ, không phải quản lý thư mục assets/.
class AudioSystem {
private:
    // VoicePool cho SFX co the bi goi CHONG LAP trong thuc te choi (nhieu dich no cung
    // luc, ban lien tuc, nhat lien tiep...) - xem voice_pool.h. So voice chon theo tan
    // suat trigger thuc te: shoot/explosion de bi spam nhat nen duoc nhieu voice nhat.
    VoicePool<6> sfxShoot;
    VoicePool<4> sfxHit;
    VoicePool<6> sfxExplosion;
    VoicePool<3> sfxPickup;
    VoicePool<2> sfxUfoAppear;
    VoicePool<3> sfxUfoHit;
    VoicePool<2> sfxCleanser;
    VoicePool<2> sfxBossPhase; // Sentinel bat/tat khien + Swarmer trieu hoi - it khi chong lap nhung van dung pool cho nhat quan

    // GameOver/WaveClear/BossDefeat chi phat DUNG 1 LAN moi su kien tuong ung (state
    // chuyen ngay sau do, khong bao gio bi goi de trong luc dang phat) - khong can pool.
    Sound sfxGameOver{};
    Sound sfxWaveClear{};
    Sound sfxBossDefeat{};

    Sound bassNotes[4]{};

    // A8 - LAYER NHIP PHU (hi-hat): noise trang, rat ngan, tick DEU voi nhip GAP DOI
    // bass (kieu "8 nghich pham" tren "4 nghich pham" cua bass - xem UpdateBassline() de
    // biet chi tiet nhip) - hoan toan doc lap voi bassline (khong dung chung voice/
    // timer), chi la 1 lop "texture" nen them vao BEN TREN nhip bass da co, KHONG doi
    // tempo/logic bass hien tai. Tu no khong can VoicePool (nhu sfxShoot/sfxHit...) vi
    // UpdateBassline() da tu gian cach cac lan Play() du xa (>= nua interval) de
    // khong bao gio chong lap.
    Sound hiHat{};
    float hiHatTimer = 0.0f;

    int bassIndex = 0;
    float bassTimer = 0.0f;
    float masterVolume = 0.6f;
    bool initialized = false;

    // NHAC NEN PROCEDURAL (Config::MUSIC_*) - AudioStream RIENG BIET HOAN TOAN voi
    // Sound/VoicePool o tren (SFX + bassline giu nguyen KHONG doi): sinh PCM REALTIME
    // moi frame qua UpdateAudioStream() thay vi phat 1 Sound co san, vi can PHAN UNG
    // LIEN TUC theo wave/Boss/mang con lai/combo (xem FillMusicBuffer() trong
    // audio_system.cpp) chu khong chi 1 nhip co dinh nhu bassline.
    AudioStream musicStream{};
    double musicSampleCursor = 0.0; // Tong so sample DA sinh tu luc Init() - dung lam "dong ho" rieng cho song synth (KHONG phai wall-clock/dt cong don) de pha song lien tuc qua nhieu lan goi FillMusicBuffer() khac nhau, du frame co bi giat.
    float musicTensionLerp = 0.0f;  // 0..1, tron MUOT toi trang thai "nguy hiem" (xem UpdateMusic()) - tranh dissonance/tremolo bat dot ngot khi vua mat mang
    short musicBuffer[Config::MUSIC_STREAM_BUFFER_FRAMES]{}; // Tai dung 1 buffer duy nhat, khong cap phat lai moi frame - cung triet ly voi pendingEvents (xem events.h)

    void FillMusicBuffer(short* buffer, int frameCount, const MusicContext& ctx);

    // waveType: 0 = sine (êm, dùng cho bassline/win/lose), 1 = square (sắc, dùng cho bắn),
    // 2 = noise (dùng cho nổ/trúng đạn)
    static Sound GenerateTone(float frequency, float durationSec, int waveType);

public:
    void Init();
    void Shutdown();

    void PlayShoot();
    void PlayExplosion();
    void PlayHit();
    void PlayGameOver();
    void PlayWaveClear(); // Phat khi don sach 1 wave (khong con la "thang" cuoi cung - xem GameState::WAVE_CLEAR)
    void PlayPickup();    // Phat khi nhat power-up
    void PlayUfoAppear(); // Phat khi Mystery Ship xuat hien
    void PlayUfoHit();    // Phat khi ban trung Mystery Ship
    void PlayCleanser();  // Phat khi kich hoat bom xoa dan (power-up Cleanser)
    void PlayBossDefeat(); // Phat khi ha guc Boss
    void PlayBossPhase();  // Phat khi Sentinel bat/tat khien HOAC Swarmer trieu hoi tiep vien - goi TRUC TIEP tu PhysicsSystem::UpdateBoss(), khong qua GameEvent (xem comment tai noi goi)

    // Gọi mỗi frame khi đang PLAYING. Tempo bassline tăng dần theo enemySpeed hiện tại,
    // tái hiện đúng cơ chế "càng gần thua càng dồn dập" của bản gốc.
    void UpdateBassline(float dt, float enemySpeed, float enemySpeedMax);

    // Goi MOI frame khi dang PLAYING (canh UpdateBassline, xem UpdatePlaying()) - day
    // tiep PCM vao AudioStream khi buffer truoc da phat xong (IsAudioStreamProcessed()),
    // KHONG phai moi lan goi deu sinh am thanh moi. An toan goi ngay ca khi Init() chua
    // chay (initialized=false -> no-op, giong triet ly cac ham Update/Play khac).
    void UpdateMusic(float dt, const MusicContext& ctx);

    void SetVolume(float v); // 0..1
    float GetVolume() const { return masterVolume; }
};
