#pragma once
#include "raylib.h"

// Sinh & phát âm thanh hoàn toàn bằng procedural synthesis (waveform tự tạo trong RAM).
// Lý do không dùng file .wav rời: tránh crash "file not found" khi build trên máy khác,
// và giữ project gọn nhẹ, không phải quản lý thư mục assets/.
class AudioManager {
private:
    Sound sfxShoot{};
    Sound sfxHit{};
    Sound sfxExplosion{};
    Sound sfxGameOver{};
    Sound sfxWin{};
    Sound bassNotes[4]{};

    int bassIndex = 0;
    float bassTimer = 0.0f;
    float masterVolume = 0.6f;
    bool initialized = false;

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
    void PlayWin();

    // Gọi mỗi frame khi đang PLAYING. Tempo bassline tăng dần theo enemySpeed hiện tại,
    // tái hiện đúng cơ chế "càng gần thua càng dồn dập" của bản gốc.
    void UpdateBassline(float dt, float enemySpeed, float enemySpeedMax);

    void SetVolume(float v); // 0..1
    float GetVolume() const { return masterVolume; }
};
