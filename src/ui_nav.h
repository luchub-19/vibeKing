#pragma once
#include "raylib.h"
#include <string_view>
#include <cmath>

// ==========================================
// DIEU HUONG GUI - HAM THUAN (khong goi ham raylib nao can cua so) -> test tai
// tests/test_ui.cpp ([ui]). Moi man hinh menu deu dung chung cac khoi nay thay vi tu viet lai
// phep "vong quanh dau/cuoi danh sach" hay "chuot dang o o nao" moi noi 1 kieu.
//
// Y TUONG CHUNG (tham khao Hades/Celeste/Balatro): 3 nguon nhap - ban phim, tay cam, chuot -
// cung dieu khien 1 con tro DUY NHAT tren moi man. Chuot chi "gianh" con tro khi no THAT SU di
// chuyen (khong phai moi frame nam yen tren 1 nut) - neu khong, nguoi dung ban phim se thay con
// tro bi giat ve cho chuot dang nam ngay sau moi lan bam Len/Xuong.
// ==========================================

namespace UiNav {
    // Di chuyen con tro trong danh sach `count` phan tu, VONG QUANH 2 dau (kieu menu arcade:
    // bam Xuong o muc cuoi quay ve muc dau). count <= 0 -> 0.
    inline int Wrap(int index, int dir, int count) {
        if (count <= 0) return 0;
        int v = (index + dir) % count;
        return v < 0 ? v + count : v;
    }

    // Kep (khong vong) - dung cho thanh truot va cac lua chon co thu tu tang dan (am luong).
    inline int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

    struct VirtualPoint {
        Vector2 pos{ 0.0f, 0.0f };
        bool inside = false; // false = chuot dang nam tren vien den letterbox/pillarbox
    };

    // Doi toa do chuot TREN CUA SO THAT ve toa do canvas noi bo (vd 800x600). Phai khop DUNG
    // phep upscale giu ty le trong GameManager::Run() (scale = min theo 2 truc, canh giua) -
    // neu lech, nut tren man hinh va vung bam chuot se truot khoi nhau khi Fullscreen.
    inline VirtualPoint ScreenToVirtual(Vector2 mouse, float screenW, float screenH, float virtW, float virtH) {
        VirtualPoint vp;
        if (screenW <= 0.0f || screenH <= 0.0f || virtW <= 0.0f || virtH <= 0.0f) return vp;
        float scale = fminf(screenW / virtW, screenH / virtH);
        float offX = (screenW - virtW * scale) / 2.0f;
        float offY = (screenH - virtH * scale) / 2.0f;
        vp.pos = { (mouse.x - offX) / scale, (mouse.y - offY) / scale };
        vp.inside = vp.pos.x >= 0.0f && vp.pos.y >= 0.0f && vp.pos.x < virtW && vp.pos.y < virtH;
        return vp;
    }

    inline bool PointIn(Vector2 p, Rectangle r) {
        return p.x >= r.x && p.x < r.x + r.width && p.y >= r.y && p.y < r.y + r.height;
    }

    // HIEU UNG GO CHU (typewriter): so BYTE dau tien cua `utf8` da "go xong" sau `elapsed` giay
    // voi toc do `charsPerSec` ky tu/giay. Dem theo KY TU (codepoint) chu khong theo byte - chu
    // tieng Viet co dau dai 2-3 byte, cat giua chung se ve ra rac. Luon dung tren ranh gioi ky tu.
    inline size_t TypewriterBytes(std::string_view utf8, float elapsed, float charsPerSec) {
        if (elapsed <= 0.0f || charsPerSec <= 0.0f) return 0;
        double want = (double)elapsed * (double)charsPerSec;
        size_t chars = 0, i = 0;
        while (i < utf8.size()) {
            if ((double)chars >= want) return i;
            unsigned char c = (unsigned char)utf8[i];
            size_t len = (c < 0x80) ? 1 : ((c >> 5) == 0x6) ? 2 : ((c >> 4) == 0xE) ? 3 : ((c >> 3) == 0x1E) ? 4 : 1;
            i += len;
            chars++;
        }
        return utf8.size();
    }

    // So KY TU da go - dung de phat tieng "tach" go phim moi khi tang (xem GameManager).
    inline int TypewriterChars(std::string_view utf8, float elapsed, float charsPerSec) {
        size_t bytes = TypewriterBytes(utf8, elapsed, charsPerSec);
        int chars = 0;
        for (size_t i = 0; i < bytes; i++) {
            if (((unsigned char)utf8[i] & 0xC0) != 0x80) chars++; // Byte dau cua 1 ky tu
        }
        return chars;
    }

    // NHAP NHAY kieu "INSERT COIN": bat/tat voi tan so `hz` (chu ky bat+tat). Giam nhap nhay ->
    // luon bat (WCAG 2.3.1 - xem GraphicsSettings::reduceFlashing). hz bi kep <= 2 (4 lan doi
    // trang thai/giay van an toan duoi nguong 3 chop SANG/giay vi moi chu ky chi 1 lan sang).
    inline bool BlinkOn(float t, float hz, bool reduceFlashing) {
        if (reduceFlashing || hz <= 0.0f) return true;
        if (hz > 2.0f) hz = 2.0f;
        float phase = t * hz - floorf(t * hz);
        return phase < 0.6f; // Sang lau hon tat 1 chut - doc de hon
    }

    // Gia tri thanh truot (0..1) tu vi tri X cua chuot tren thanh - keo/bam chuot vao thanh am luong.
    // Lam tron ve buoc `step` (vd 0.05) de gia tri luu file gon va khop buoc ban phim.
    inline float SliderValueAt(float mouseX, Rectangle bar, float step) {
        if (bar.width <= 0.0f) return 0.0f;
        float v = (mouseX - bar.x) / bar.width;
        v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        if (step > 0.0f) v = roundf(v / step) * step;
        return v > 1.0f ? 1.0f : v;
    }
}
