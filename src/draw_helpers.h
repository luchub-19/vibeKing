#pragma once
#include "raylib.h"
#include "palette.h"
#include <cmath>

// ==========================================
// HAM VE DUNG CHUNG giua nhieu file ve (render_system.cpp, player.cpp...) - chi nhung hinh
// mang CUNG 1 NGHIA o nhieu noi (1 nguon duy nhat, giong tinh than Palette::). Ham ve thuan:
// khong test duoc, kiem bang anh chup (xem CLAUDE.md).
// ==========================================

// KHIEN LUC GIAC - kieu chung cho khien player va Sentinel (Palette::ShieldBarrier). Luc giac
// xoay cham + vien kep + nen rat mo nhip tho: "ky thuat, co cau truc" hop phong cach vector
// hon vong tron/khung vuong tron cu. rotationDeg truyen vao de 2 khien khong xoay dong bo.
inline void DrawHexShield(Vector2 center, float radius, float time, float rotationDeg) {
    float pulse = 0.5f + 0.5f * sinf(time * 4.0f);
    Color fill = Fade(Palette::ShieldBarrier, 0.06f + 0.05f * pulse);
    DrawPoly(center, 6, radius, rotationDeg, fill);
    DrawPolyLinesEx(center, 6, radius, rotationDeg, 2.0f, Fade(Palette::ShieldBarrier, 0.85f));
    DrawPolyLinesEx(center, 6, radius - 4.0f, -rotationDeg, 1.0f, Fade(Palette::ShieldBarrier, 0.35f + 0.25f * pulse));
}


// CHU NEON (GD 5): quang = 8 ban sao lech 2px quanh tam (alpha thap) + 1 lop loi sang hon. Bloom
// lam phan con lai. `brightness` 0..1 - dung cho hieu ung "bat den" (NeonPowerOn) va fade.
inline void DrawNeonText(const Font& font, const char* text, Vector2 center, float size, Color color, float brightness) {
    if (brightness <= 0.0f) return;
    Vector2 dim = MeasureTextEx(font, text, size, 2.0f);
    Vector2 pos{ center.x - dim.x / 2.0f, center.y - dim.y / 2.0f };
    Color halo = Fade(color, 0.16f * brightness);
    for (int i = 0; i < 8; i++) {
        float a = (float)i * 0.785398f;
        DrawTextEx(font, text, { pos.x + cosf(a) * 2.5f, pos.y + sinf(a) * 2.5f }, size, 2.0f, halo);
    }
    DrawTextEx(font, text, pos, size, 2.0f, Fade(color, brightness));
    // Loi "ong neon": sang hon mau goc - giup bloom bat dung net chu thay vi ca khoi quang
    DrawTextEx(font, text, pos, size, 2.0f, Fade(Palette::Lerp(color, WHITE, 0.5f), 0.55f * brightness));
}

// "Bat den" ong neon luc mo game: 2 lan chop roi sang han trong ~0.9s. Ham THUAN theo thoi gian
// (test duoc). reduceFlashing -> hien dan deu, KHONG chop. 2 lan chop/0.9s duoi nguong 3 lan/s
// cua WCAG, va chi 1 dong chu (khong phai flash toan man hinh) - nhung van tat theo co cho chac.
inline float NeonPowerOn(float t, bool reduceFlashing) {
    if (t <= 0.0f) return 0.0f;
    if (reduceFlashing) return t >= 0.6f ? 1.0f : t / 0.6f;
    if (t < 0.15f) return 0.0f;
    if (t < 0.22f) return 0.8f;   // chop 1
    if (t < 0.40f) return 0.08f;
    if (t < 0.47f) return 0.9f;   // chop 2
    if (t < 0.62f) return 0.15f;
    if (t < 0.9f)  return 0.6f + 0.4f * (t - 0.62f) / 0.28f;
    return 1.0f;
}
