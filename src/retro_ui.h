#pragma once
#include "raylib.h"
#include "palette.h"
#include "draw_helpers.h"
#include "ui_nav.h"
#include <cmath>
#include <cstring>
#include <string>

// ==========================================
// BO WIDGET "RETRO ARCADE" CHO GUI (nang cap GUI). Ham VE thuan - khong test duoc, kiem bang anh
// chup (scripts/capture_showcase.sh --scene=menu|settings|...; xem CLAUDE.md "Ham VE thuan").
//
// Nguon cam hung (tong hop tu nhieu ngon ngu/cong dong):
//   - Tu may arcade Nhat 70-80 (Space Invaders/Galaga cua Taito/Namco): "SCORE ADVANCE TABLE",
//     INSERT COIN nhap nhay, CREDIT o goc, diem 6 chu so co so 0 dau.
//   - Famicom/NES JRPG (Dragon Quest - "コマンドウィンドウ"): khung vien pixel goc bac thang,
//     con tro tam giac nhay nhot ben trai muc dang chon, chu hien tung ky tu kem tieng bip.
//   - Balatro / Hotline Miami: chu muc dang chon NHUN theo song sin tung ky tu, glitch tach RGB
//     khi vao man - chuyen dong lam menu "song" ma khong can them hinh ve.
//   - Cong dong demoscene Han/Viet (bang LED, thanh am luong chia vach): thanh truot dang dai
//     vach LED roi rac thay vi thanh tron muot - doc duoc so buoc bang mat.
// Tat ca hieu ung chuyen dong deu tat/giam theo reduceFlashing (WCAG 2.3.1).
// ==========================================
namespace RetroUi {
    // KHUNG PIXEL goc bac thang (khong bo tron, khong goc vuong): vien day `t` px, goc cat `n` px
    // theo bac thang t x t - net "cua so lenh" cua game 8-bit. fill.a = 0 -> chi ve vien.
    inline void PixelFrame(Rectangle r, Color fill, Color border, float n = 6.0f, float t = 2.0f) {
        if (fill.a > 0) {
            DrawRectangleRec({ r.x + n, r.y, r.width - 2.0f * n, r.height }, fill);
            DrawRectangleRec({ r.x, r.y + n, n, r.height - 2.0f * n }, fill);
            DrawRectangleRec({ r.x + r.width - n, r.y + n, n, r.height - 2.0f * n }, fill);
            // Lap phan tam giac o 4 goc giua bac thang (de nen khong hong 1 khe den)
            for (float k = t; k < n; k += t) {
                DrawRectangleRec({ r.x + k, r.y + n - k, n - k, k }, fill);
                DrawRectangleRec({ r.x + r.width - n, r.y + n - k, n - k, k }, fill);
                DrawRectangleRec({ r.x + k, r.y + r.height - n, n - k, k }, fill);
                DrawRectangleRec({ r.x + r.width - n, r.y + r.height - n, n - k, k }, fill);
            }
        }
        if (border.a == 0) return;
        DrawRectangleRec({ r.x + n, r.y, r.width - 2.0f * n, t }, border);
        DrawRectangleRec({ r.x + n, r.y + r.height - t, r.width - 2.0f * n, t }, border);
        DrawRectangleRec({ r.x, r.y + n, t, r.height - 2.0f * n }, border);
        DrawRectangleRec({ r.x + r.width - t, r.y + n, t, r.height - 2.0f * n }, border);
        for (float k = t; k < n; k += t) { // Bac thang: (x, y+n) -> (x+n, y)
            DrawRectangleRec({ r.x + k, r.y + n - k, t, t }, border);
            DrawRectangleRec({ r.x + r.width - k - t, r.y + n - k, t, t }, border);
            DrawRectangleRec({ r.x + k, r.y + r.height - n + k - t, t, t }, border);
            DrawRectangleRec({ r.x + r.width - k - t, r.y + r.height - n + k - t, t, t }, border);
        }
    }

    // CON TRO TAM GIAC PIXEL (khong dung glyph - VT323 khong co ky hieu tam giac, xem localization.h
    // luat 3). Ve bang cac cot chu nhat cao giam dan -> dung chat pixel. `dir` +1 chi sang phai.
    inline void PixelArrow(Vector2 tip, float h, Color c, int dir = 1) {
        const float px = 2.0f;
        int cols = (int)(h / 2.0f / px);
        for (int i = 0; i < cols; i++) {
            float colH = h - (float)i * 2.0f * px;
            float x = tip.x - (float)dir * (float)(cols - i) * px - (dir > 0 ? 0.0f : px);
            DrawRectangleRec({ x, tip.y - colH / 2.0f, px, colH }, c);
        }
    }

    inline float TextWidth(const Font& font, const char* text, float size, float spacing = 1.0f) {
        return MeasureTextEx(font, text, size, spacing).x;
    }

    inline void TextCentered(const Font& font, const char* text, float cx, float y, float size, Color c, float spacing = 1.0f) {
        DrawTextEx(font, text, { cx - TextWidth(font, text, size, spacing) / 2.0f, y }, size, spacing, c);
    }

    inline void TextRight(const Font& font, const char* text, float right, float y, float size, Color c, float spacing = 1.0f) {
        DrawTextEx(font, text, { right - TextWidth(font, text, size, spacing), y }, size, spacing, c);
    }

    // CHU NHUN (Balatro): moi ky tu lech Y theo sin(thoi gian + chi so ky tu). Duyet theo
    // CODEPOINT (GetCodepointNext) - chu tieng Viet nhieu byte van la 1 ky tu.
    inline void WavyText(const Font& font, const char* text, Vector2 pos, float size, float spacing, Color c,
                         float time, float amp) {
        if (amp <= 0.0f) { DrawTextEx(font, text, pos, size, spacing, c); return; }
        const float scale = size / (float)font.baseSize;
        float x = pos.x;
        int i = 0, idx = 0;
        const int len = (int)strlen(text);
        while (i < len) {
            int bytes = 0;
            int cp = GetCodepointNext(&text[i], &bytes);
            int g = GetGlyphIndex(font, cp);
            float dy = sinf(time * 7.0f + (float)idx * 0.55f) * amp;
            if (cp != ' ') DrawTextCodepoint(font, cp, { x, pos.y + dy }, size, c);
            float adv = (font.glyphs[g].advanceX == 0) ? font.recs[g].width : (float)font.glyphs[g].advanceX;
            x += adv * scale + spacing;
            i += bytes > 0 ? bytes : 1;
            idx++;
        }
    }

    inline void WavyTextCentered(const Font& font, const char* text, float cx, float y, float size, Color c, float time, float amp) {
        WavyText(font, text, { cx - TextWidth(font, text, size) / 2.0f, y }, size, 1.0f, c, time, amp);
    }

    // Do glitch 0..1 cua tieu de: manh luc VUA vao man (0.35s dau), sau do cu ~4.5s chop 1 nhip
    // ngan 0.12s cho "man hinh CRT cu" con song. Ham thuan theo thoi gian. reduceFlashing -> 0.
    inline float GlitchAmount(float sinceEnter, float time, bool reduceFlashing) {
        if (reduceFlashing) return 0.0f;
        if (sinceEnter < 0.35f) return 1.0f - sinceEnter / 0.35f;
        float ph = fmodf(time, 4.5f);
        return (ph < 0.12f) ? 0.6f : 0.0f;
    }

    // TIEU DE NEON + GLITCH: chu Bungee co quang (DrawNeonText) + khi glitch > 0: 2 ban tach kenh
    // do/lam lech ngang + 3 dai cat ngang bi xo lech (scissor). Do lech gieo theo floor(time*24)
    // -> moi khung hinh 1 kieu xo nhung khong can bo sinh so ngau nhien co trang thai.
    inline void GlitchTitle(const Font& font, const char* text, Vector2 center, float size, Color c, float glitch, float time) {
        if (glitch <= 0.0f) { DrawNeonText(font, text, center, size, c, 1.0f); return; }
        Vector2 dim = MeasureTextEx(font, text, size, 2.0f);
        Vector2 pos{ center.x - dim.x / 2.0f, center.y - dim.y / 2.0f };
        float seed = floorf(time * 24.0f);
        auto rnd = [&](float k) { float v = sinf(seed * 12.9898f + k * 78.233f) * 43758.5453f; return v - floorf(v); };
        float split = 4.0f * glitch;
        BeginBlendMode(BLEND_ADDITIVE);
        DrawTextEx(font, text, { pos.x - split, pos.y }, size, 2.0f, Fade(Color{ 255, 40, 80, 255 }, 0.55f * glitch));
        DrawTextEx(font, text, { pos.x + split, pos.y }, size, 2.0f, Fade(Color{ 40, 220, 255, 255 }, 0.55f * glitch));
        EndBlendMode();
        DrawNeonText(font, text, center, size, c, 1.0f - 0.35f * glitch);
        for (int b = 0; b < 3; b++) {
            float by = pos.y + rnd((float)b) * dim.y;
            float bh = 2.0f + rnd((float)b + 5.0f) * dim.y * 0.25f;
            float dx = (rnd((float)b + 9.0f) - 0.5f) * 28.0f * glitch;
            BeginScissorMode((int)(pos.x - 40.0f), (int)by, (int)(dim.x + 80.0f), (int)bh);
            DrawRectangle((int)(pos.x - 40.0f), (int)by, (int)(dim.x + 80.0f), (int)bh, Palette::Background);
            DrawTextEx(font, text, { pos.x + dx, pos.y }, size, 2.0f, c);
            EndScissorMode();
        }
    }

    // CHU GO TUNG KY TU + con tro khoi nhap nhay o cuoi (kieu terminal). Tra ve true khi da go xong.
    inline bool Typewriter(const Font& font, const char* text, Vector2 pos, float size, Color c,
                           float elapsed, float cps, float time, bool reduceFlashing, bool centered = false) {
        size_t n = UiNav::TypewriterBytes(text, elapsed, cps);
        std::string shown(text, n);
        float fullW = TextWidth(font, text, size);
        if (centered) pos.x -= fullW / 2.0f; // Canh theo do dai CUOI -> chu khong troi ngang khi dang go
        DrawTextEx(font, shown.c_str(), pos, size, 1.0f, c);
        bool done = n >= strlen(text);
        if (!done || UiNav::BlinkOn(time, 1.6f, reduceFlashing)) {
            float w = TextWidth(font, shown.c_str(), size);
            if (!done || !reduceFlashing) {
                DrawRectangleRec({ pos.x + w + 2.0f, pos.y + size * 0.15f, size * 0.42f, size * 0.7f }, Fade(c, done ? 0.6f : 0.9f));
            }
        }
        return done;
    }

    // THANH LED chia vach (am luong): `segments` o roi rac, o sang = mau `on`, o tat = mo. O dau
    // tien sang khi value > 0 -> 5% van thay 1 vach, 0% tat het (khong nham "tat tieng" voi "nho").
    inline void LedBar(Rectangle r, float value, int segments, Color on, Color off) {
        if (segments <= 0) return;
        float gap = 2.0f;
        float w = (r.width - gap * (float)(segments - 1)) / (float)segments;
        int lit = (int)ceilf(value * (float)segments - 0.001f);
        for (int i = 0; i < segments; i++) {
            Color col = i < lit ? on : off;
            // Vach cuoi cao dan - doc nhu "cot song am" thay vi 1 hang gach phang
            float h = r.height * (0.55f + 0.45f * (float)i / (float)(segments - 1 > 0 ? segments - 1 : 1));
            DrawRectangleRec({ r.x + (float)i * (w + gap), r.y + r.height - h, w, h }, col);
        }
    }

    // NUT PHIM (keycap) - o vuong vien pixel co chu ten phim, dung cho bang dieu khien/doi phim.
    inline void KeyCap(const Font& font, const char* label, Rectangle r, Color border, Color text, float size) {
        PixelFrame(r, Fade(Palette::UiPanelFill, 0.9f), border, 4.0f, 2.0f);
        DrawRectangleRec({ r.x + 4.0f, r.y + r.height - 4.0f, r.width - 8.0f, 2.0f }, Fade(border, 0.4f)); // Mep day "noi" cua phim
        TextCentered(font, label, r.x + r.width / 2.0f, r.y + (r.height - size) / 2.0f - 1.0f, size, text);
    }

    // DAI QUET CRT: 1 vach sang mo troi tu tren xuong lien tuc - nen "dong" cho man menu. Rat nhat
    // (alpha <= 0.05) nen khong phai nhap nhay; reduceFlashing van tat cho chac.
    inline void ScanSweep(float time, float screenW, float screenH, bool reduceFlashing) {
        if (reduceFlashing) return;
        float y = fmodf(time * 90.0f, screenH + 120.0f) - 60.0f;
        for (int i = 0; i < 6; i++) {
            DrawRectangleRec({ 0.0f, y + (float)i * 6.0f, screenW, 6.0f }, Fade(Palette::Weaver, 0.008f * (float)(6 - i)));
        }
    }
}
