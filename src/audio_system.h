#pragma once
#include "raylib.h"
#include "voice_pool.h"

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

    void SetVolume(float v); // 0..1
    float GetVolume() const { return masterVolume; }
};
