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

    sfxGameOver   = GenerateTone(110.0f, 0.80f, 0);
    sfxWaveClear  = GenerateTone(660.0f, 0.60f, 0);
    sfxBossDefeat = GenerateTone(150.0f, 1.2f, 2);     // Dai va tram nhat - cam giac "no lon"

    // 4 nốt giảm dần - đặc trưng bassline "duh-duh-duh-duh" của Space Invaders gốc.
    // Bassline luon phat TUAN TU (1 not/lan, cach nhau it nhat 0.2s - xem UpdateBassline)
    // nen khong bao gio bi trung lap/can voice pool nhu cac sfx phan ung theo hanh dong.
    const float notes[4] = { 220.0f, 196.0f, 174.6f, 164.8f };
    for (int i = 0; i < 4; i++) bassNotes[i] = GenerateTone(notes[i], 0.15f, 0);

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

    UnloadSound(sfxGameOver);
    UnloadSound(sfxWaveClear);
    UnloadSound(sfxBossDefeat);
    for (int i = 0; i < 4; i++) UnloadSound(bassNotes[i]);
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
    SetSoundVolume(sfxGameOver, v);
    SetSoundVolume(sfxWaveClear, v);
    SetSoundVolume(sfxBossDefeat, v);
    for (int i = 0; i < 4; i++) SetSoundVolume(bassNotes[i], v * 0.7f); // Bass nhỏ hơn SFX chính 1 chút
}
