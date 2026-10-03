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

// LOC MAU cho nguoi thieu sac giac (trang Cai dat > Tro nang). Thuc thi o pass cuoi (final.fs)
// bang thuat toan daltonize (Fidaner et al.): mo phong cach mat loai do nhin anh, lay phan
// thong tin mau BI MAT roi don sang kenh mat VAN phan biet duoc. Khong doi Palette:: - luat
// lanh/nong van giu, chi lam no doc duoc voi nhieu nguoi hon.
enum class ColorFilter : uint8_t { Off, Protan, Deutan, Tritan };
constexpr int COLOR_FILTER_COUNT = 4;

struct GraphicsSettings {
    GraphicsQuality quality = GraphicsQuality::Medium; // Medium = dien mao truoc khi co trang nay
    bool crtEnabled = true;
    // Che do nhay sang (Xbox Accessibility Guideline 118 / WCAG 2.3.1): tat moi nhap nhay toan
    // man hinh. Hien chi co CRT flicker; cac flash lon them o GD 2-4 phai doc co nay.
    bool reduceFlashing = false;
    int shakePercent = 100; // Luon la 1 phan tu cua SHAKE_PERCENT_LEVELS sau Sanitize()
    ColorFilter colorFilter = ColorFilter::Off;

    // Low tat bloom: pass dat nhat (3 lan ve full-texture), la thu dau tien bo khi may yeu.
    bool BloomEnabled() const { return quality != GraphicsQuality::Low; }
    // So muc thu nho Dual Kawase sau buoc trich (1/2 man hinh). Medium 1 muc: quang ~ bang Gauss
    // cu (so anh). DINH CHINH: commit dua Kawase vao ghi "nhanh hon Gauss ~5%" - SAI, do 5 lan
    // trung binh bi vai lan chay nhieu cua ban goc keo lech. Do lai 8 lan xen ke lay trung vi:
    // tren llvmpipe Kawase 1 muc CHAM hon Gauss ~4% (20.1 vs 19.1 ms), 2 muc cham hon ~8%. Giu
    // Kawase vi ly do GPU (bang thong ~7% Gauss - ARM SIGGRAPH 2015), CHUA do duoc tren iGPU that.
    // High 3 muc: quang rong gap 4 - danh cho may du suc.
    int BloomLevels() const { return quality == GraphicsQuality::High ? 3 : 1; }

    // He so nhan so particle moi Burst()/manh vo Explosion(). High 1.5x (pool da nang len 800 -
    // Config::MAX_PARTICLES); Low 0.5x.
    float ParticleScale() const { return quality == GraphicsQuality::Low ? 0.5f : (quality == GraphicsQuality::High ? 1.5f : 1.0f); }

    // Luoi lo xo (WarpGrid): Low TAT han (ban de xuat la "luoi dung yen", nhung do bang bench:
    // ve ~1000-1600 doan thang van ton ngang 1 pass post-process tren renderer CPU - may yeu
    // can bo het, khong phai giu 1 luoi khong chuyen dong). Medium o 32px, High o 25px.
    bool GridEnabled() const { return quality != GraphicsQuality::Low; }
    float GridCellSize() const { return quality == GraphicsQuality::High ? 25.0f : 32.0f; }

    // Tinh van (Nebula): Low 0 lop (bo ca shader lan texture), Medium 1, High 2 lop parallax.
    int NebulaLayers() const { return quality == GraphicsQuality::Low ? 0 : (quality == GraphicsQuality::High ? 2 : 1); }

    // Pass cuoi (final.fs, GD 3): so song xung kich toi da gui len shader, tach RGB o mep song, va
    // do cong man hinh. CRT cong la thanh phan cua CRT: tat CRT thi khong cong du o High.
    int ShockwaveMax() const { return quality == GraphicsQuality::Low ? 0 : (quality == GraphicsQuality::High ? 8 : 4); }
    bool ChromaticShockwave() const { return quality == GraphicsQuality::High; }
    float BarrelAmount() const { return (quality == GraphicsQuality::High && crtEnabled) ? 0.06f : 0.0f; }

    float ShakeScale() const { return (float)shakePercent / 100.0f; }

    float CrtFlickerScale() const { return reduceFlashing ? 0.0f : 1.0f; }

    void CycleQuality(int dir) {
        int q = ((int)quality + dir % GRAPHICS_QUALITY_COUNT + GRAPHICS_QUALITY_COUNT) % GRAPHICS_QUALITY_COUNT;
        quality = (GraphicsQuality)q;
    }

    void CycleColorFilter(int dir) {
        int f = ((int)colorFilter + dir % COLOR_FILTER_COUNT + COLOR_FILTER_COUNT) % COLOR_FILTER_COUNT;
        colorFilter = (ColorFilter)f;
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

inline const char* GraphicsQualityLabel(GraphicsQuality q) {
    switch (q) {
        case GraphicsQuality::Low:    return "LOW";
        case GraphicsQuality::Medium: return "MEDIUM";
        case GraphicsQuality::High:   return "HIGH";
    }
    return "MEDIUM";
}

// Ma luu file (settings.cfg) - KHONG dich, giong GraphicsQualityLabel.
inline const char* ColorFilterCode(ColorFilter f) {
    switch (f) {
        case ColorFilter::Protan: return "PROTAN";
        case ColorFilter::Deutan: return "DEUTAN";
        case ColorFilter::Tritan: return "TRITAN";
        case ColorFilter::Off:    break;
    }
    return "OFF";
}
