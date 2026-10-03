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

