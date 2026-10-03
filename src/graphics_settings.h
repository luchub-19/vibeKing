#pragma once
#include <cstdint>

// ==========================================
// CAI DAT DO HOA - tuy chon CUA NGUOI CHOI (luu trong settings.cfg cung Settings), khong phai
// du lieu can bang: co y KHONG nam trong balance.json/Config::LoadBalance(), va khong phai
// constexpr vi doi duoc luc dang chay (trang GRAPHICS, phim G o menu chinh).
//
// Quan he voi Config::BLOOM_ENABLED/CRT_ENABLED (config.h): 2 hang constexpr do la cong tac
// TONG luc build - false thi PostProcess khong bao gio load shader. Cai dat o day chi quyet
// dinh pass da load DUOC co chay o frame nay hay khong.
//
// 1-NGUON-DUY-NHAT cho "preset X bat cai gi": moi noi can biet (PostProcess, ParticlePool,
// trang GRAPHICS de hien mo ta) deu hoi qua cac ham ben duoi, khong tu so sanh `quality`.
// Them hieu ung moi o cac giai doan sau (luoi lo xo, shockwave, CRT cong...) -> them 1 ham
// o day, dung rai `if (quality == High)` khap noi.
//
// Ham thuan, khong goi raylib - test tai tests/test_settings.cpp ([graphics]).
// ==========================================

enum class GraphicsQuality : uint8_t { Low, Medium, High };
constexpr int GRAPHICS_QUALITY_COUNT = 3;

// 3 muc rung man hinh cho phep - roi rac thay vi thanh truot: nguoi nhay cam chuyen dong can
// "tat han" de tim, khong phai keo tung 10%.
constexpr int SHAKE_PERCENT_LEVELS[] = { 100, 50, 0 };
constexpr int SHAKE_PERCENT_LEVEL_COUNT = 3;

struct GraphicsSettings {
    GraphicsQuality quality = GraphicsQuality::Medium; // Medium = dien mao truoc khi co trang nay
    bool crtEnabled = true;
    // Che do nhay sang (Xbox Accessibility Guideline 118 / WCAG 2.3.1): tat moi nhap nhay toan
    // man hinh. Hien chi co CRT flicker; cac flash lon them o GD 2-4 phai doc co nay.
    bool reduceFlashing = false;
    int shakePercent = 100; // Luon la 1 phan tu cua SHAKE_PERCENT_LEVELS sau Sanitize()

    // Low tat bloom: pass dat nhat (3 lan ve full-texture), la thu dau tien bo khi may yeu.
    bool BloomEnabled() const { return quality != GraphicsQuality::Low; }

    // He so nhan so particle moi Burst(). High hien = Medium; GD 2 se nang len khi pool lon hon.
    float ParticleScale() const { return quality == GraphicsQuality::Low ? 0.5f : 1.0f; }

    float ShakeScale() const { return (float)shakePercent / 100.0f; }

    float CrtFlickerScale() const { return reduceFlashing ? 0.0f : 1.0f; }

    void CycleQuality(int dir) {
        int q = ((int)quality + dir % GRAPHICS_QUALITY_COUNT + GRAPHICS_QUALITY_COUNT) % GRAPHICS_QUALITY_COUNT;
        quality = (GraphicsQuality)q;
    }

    void CycleShake(int dir) {
        int idx = 0;
        for (int i = 0; i < SHAKE_PERCENT_LEVEL_COUNT; i++) {
            if (SHAKE_PERCENT_LEVELS[i] == shakePercent) idx = i;
        }
        idx = (idx + dir % SHAKE_PERCENT_LEVEL_COUNT + SHAKE_PERCENT_LEVEL_COUNT) % SHAKE_PERCENT_LEVEL_COUNT;
        shakePercent = SHAKE_PERCENT_LEVELS[idx];
    }

    // File sua tay co the ghi GFX_SHAKE=73 -> lam tron ve muc gan nhat thay vi giu 1 gia tri
    // ma trang GRAPHICS khong hien duoc (CycleShake se nhay lung tung tu do).
    void Sanitize() {
        int best = SHAKE_PERCENT_LEVELS[0];
        for (int lv : SHAKE_PERCENT_LEVELS) {
            int d = shakePercent - lv, bd = shakePercent - best;
            if ((d < 0 ? -d : d) < (bd < 0 ? -bd : bd)) best = lv;
        }
        shakePercent = best;
    }
};

// Thu tu cac dong tren trang GRAPHICS - 1 nguon cho ca GameManager::UpdateGraphicsScreen()
// (dong nao doi gi) lan RenderSystem::DrawGraphicsSettings() (dong nao ve gi).
enum class GraphicsRow : uint8_t { Quality, Crt, ReduceFlashing, Shake };
constexpr int GRAPHICS_ROW_COUNT = 4;

inline const char* GraphicsQualityLabel(GraphicsQuality q) {
    switch (q) {
        case GraphicsQuality::Low:    return "LOW";
        case GraphicsQuality::Medium: return "MEDIUM";
        case GraphicsQuality::High:   return "HIGH";
    }
    return "MEDIUM";
}
