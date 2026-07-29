#include "audio_manager.h"
#include <cmath>
#include <random>
#include <vector>

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

Sound AudioManager::GenerateTone(float frequency, float durationSec, int waveType) {
    const int sampleRate = 44100;
    const int sampleCount = (int)(sampleRate * durationSec);

    std::vector<short> samples(sampleCount);
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

        samples[i] = (short)(value * envelope * 12000.0f);
    }

    Wave wave{};
    wave.frameCount = (unsigned int)sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();

    // LoadSoundFromWave copy dữ liệu vào buffer audio riêng của raylib ngay khi gọi,
    // nên `samples` (vector cục bộ) an toàn khi ra khỏi scope - không cần UnloadWave() ở đây
    // vì wave.data không được cấp phát qua RL_MALLOC (tự UnloadWave sẽ crash).
    return LoadSoundFromWave(wave);
}

void AudioManager::Init() {
    if (initialized) return;
    InitAudioDevice();

    sfxShoot     = GenerateTone(880.0f, 0.08f, 1);
    sfxHit       = GenerateTone(200.0f, 0.15f, 2);
    sfxExplosion = GenerateTone(120.0f, 0.30f, 2);
    sfxGameOver  = GenerateTone(110.0f, 0.80f, 0);
    sfxWin       = GenerateTone(660.0f, 0.60f, 0);

    // 4 nốt giảm dần - đặc trưng bassline "duh-duh-duh-duh" của Space Invaders gốc
    const float notes[4] = { 220.0f, 196.0f, 174.6f, 164.8f };
    for (int i = 0; i < 4; i++) bassNotes[i] = GenerateTone(notes[i], 0.15f, 0);

    SetVolume(masterVolume);
    initialized = true;
}

void AudioManager::Shutdown() {
    if (!initialized) return;
    UnloadSound(sfxShoot);
    UnloadSound(sfxHit);
    UnloadSound(sfxExplosion);
    UnloadSound(sfxGameOver);
    UnloadSound(sfxWin);
    for (int i = 0; i < 4; i++) UnloadSound(bassNotes[i]);
    CloseAudioDevice();
    initialized = false;
}

void AudioManager::PlayShoot()     { PlaySound(sfxShoot); }
void AudioManager::PlayExplosion() { PlaySound(sfxExplosion); }
void AudioManager::PlayHit()       { PlaySound(sfxHit); }
void AudioManager::PlayGameOver()  { PlaySound(sfxGameOver); }
void AudioManager::PlayWin()       { PlaySound(sfxWin); }

void AudioManager::UpdateBassline(float dt, float enemySpeed, float enemySpeedMax) {
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

void AudioManager::SetVolume(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    masterVolume = v;
    SetSoundVolume(sfxShoot, v);
    SetSoundVolume(sfxHit, v);
    SetSoundVolume(sfxExplosion, v);
    SetSoundVolume(sfxGameOver, v);
    SetSoundVolume(sfxWin, v);
    for (int i = 0; i < 4; i++) SetSoundVolume(bassNotes[i], v * 0.7f); // Bass nhỏ hơn SFX chính 1 chút
}
