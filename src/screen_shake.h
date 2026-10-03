#pragma once
#include "raylib.h"
#include <cmath>
#include <cstdint>

// ==========================================
// SCREEN SHAKE - mo hinh TRAUMA (Squirrel Eiserloh, "Math for Game Programmers: Juicing Your
// Cameras With Math", GDC 2016) - GD 2 cua docs/GRAPHICS_UPGRADE_PLAN.md.
//
// LICH SU: ban dau GetOffset() tu random moi lan goi trong ham ve (phu thuoc FPS, nhay vo
// huong). Ban thu 2 chuyen random vao Update(): cu 50ms GetRandomValue() 1 diem dich roi lerp
// toi - het phu thuoc FPS nhung (1) bien do giam TUYEN TINH nen duoi rung lê thê, (2) ket qua
// khac nhau moi lan chay (khong chup anh truoc/sau so duoc, khong test duoc), (3) chi tinh tien.
//
// GIO:
//   - trauma 1 -> 0 tuyen tinh trong `duration` giay; bien do = peak * trauma^2 -> cu rung
//     manh luc dau va tat GON (binh phuong lam duoi nho rat nhanh) thay vi lê thê.
//   - Nhieu MUOT tat dinh (value noise 1D noi suy cosine, hash so nguyen) thay GetRandomValue:
//     cung chuoi Trigger/Update -> cung offset, test duoc va anh trinh dien lap lai duoc.
//   - Them XOAY nho (toi da MAX_ROTATION_DEG cho cu rung peak = REFERENCE_PEAK_PX) - cu no
//     lon "lac" ca khung hinh, cu trung nho gan nhu chi tinh tien.
// Ngu nghia Trigger() GIU NGUYEN ban cu: rung yeu hon KHONG de len rung manh dang chay.
// ==========================================
class ScreenShake {
private:
    float trauma = 0.0f;     // 1 = vua trigger, 0 = yen
    float decayRate = 0.0f;  // trauma/giay, = 1/duration cua lan trigger dang thang the
    float peak = 0.0f;       // px, bien do cua lan trigger dang thang the
    float clock = 0.0f;      // Thoi gian noi bo cho nhieu (doc lap GetTime() -> test duoc)

    Vector2 currentOffset{ 0.0f, 0.0f };
    float currentRotation = 0.0f;

    static constexpr float NOISE_FREQ = 22.0f;        // Hz - so "cu lac" moi giay
    static constexpr float MAX_ROTATION_DEG = 1.5f;
    static constexpr float REFERENCE_PEAK_PX = 12.0f; // Cu rung lon nhat hien co (boss guc)

    // Value noise 1D: hash so nguyen -> [-1,1], noi suy cosine giua 2 diem nguyen lien ke.
    // `seed` tach 3 kenh (x, y, xoay) de chung khong dong bo voi nhau.
    static float Hash(int32_t i, uint32_t seed) {
        uint32_t h = (uint32_t)i * 374761393u + seed * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (float)(h & 0xFFFFu) / 32767.5f - 1.0f;
    }

public:
    static float SmoothNoise(float x, uint32_t seed) {
        float fl = floorf(x);
        int32_t i = (int32_t)fl;
        float f = x - fl;
        float t = (1.0f - cosf(f * 3.14159265f)) * 0.5f;
        return Hash(i, seed) * (1.0f - t) + Hash(i + 1, seed) * t;
    }

    // Rung manh hon se ghi de rung yeu dang chay, tranh 1 va cham nho lam mat hieu ung lon.
    // So sanh theo BIEN DO HIEN TAI (peak * trauma^2) chu khong phai peak goc - cu no lon da
    // tat 90% thi 1 cu trung vua phai van duoc phep thay the.
    void Trigger(float durationSec, float mag) {
        if (durationSec <= 0.0f || mag <= 0.0f) return;
        if (mag >= peak * trauma * trauma) {
            trauma = 1.0f;
            peak = mag;
            decayRate = 1.0f / durationSec;
        }
    }

    void Update(float dt) {
        clock += dt;
        if (trauma <= 0.0f) {
            currentOffset = { 0.0f, 0.0f };
            currentRotation = 0.0f;
            return;
        }
        trauma = fmaxf(0.0f, trauma - decayRate * dt);
        float shake = trauma * trauma;
        float n = clock * NOISE_FREQ;
        currentOffset = { peak * shake * SmoothNoise(n, 1u), peak * shake * SmoothNoise(n, 2u) };
        currentRotation = MAX_ROTATION_DEG * (peak / REFERENCE_PEAK_PX) * shake * SmoothNoise(n, 3u);
    }

    Vector2 GetOffset() const { return currentOffset; }
    float GetRotation() const { return currentRotation; } // Do - xem DrawPlaying() (xoay quanh TAM man hinh)
    bool IsActive() const { return trauma > 0.0f; }
};
