#pragma once
#include "raylib.h"
#include <array>
#include <cmath>

// ==========================================
// DU LIEU HAU KY MOI FRAME (GD 3 - docs/GRAPHICS_UPGRADE_PLAN.md) - phan THUAN (khong goi
// raylib ve/GPU) cua pass cuoi trong assets/shaders/final.fs. GameManager giu ShockwaveField +
// cac dong ho, dien PostFxFrame moi frame, PostProcess chi viec day thanh uniform. Tach rieng
// de test headless (tests/test_post_fx.cpp) - shader thi phai kiem bang anh chup.
// ==========================================

// SONG XUNG KICH: vong mo rong lam meo anh phia sau (nguon: Habr "displacement shader", gameidea
// "Shockwave Distortion"). Moi vu no them 1 song; day thi thay song CU nhat (song moi luon quan
// trong hon - no vua xay ra).
class ShockwaveField {
public:
    static constexpr int CAPACITY = 8;

    struct Wave {
        Vector2 pos{};
        float age = 0.0f;
        float duration = 0.0f;   // 0 = o trong
        float maxRadius = 0.0f;  // px (toa do game 800x600)
        float strength = 0.0f;   // px dich chuyen toi da o mep song
    };

    void Clear() { for (Wave& w : waves) w.duration = 0.0f; }

    void Add(Vector2 pos, float maxRadius, float duration, float strength) {
        if (duration <= 0.0f) return;
        int slot = 0;
        float oldest = -1.0f;
        for (int i = 0; i < CAPACITY; i++) {
            if (waves[(size_t)i].duration <= 0.0f) { slot = i; oldest = -2.0f; break; }
            float progress = waves[(size_t)i].age / waves[(size_t)i].duration;
            if (progress > oldest) { oldest = progress; slot = i; }
        }
        waves[(size_t)slot] = Wave{ pos, 0.0f, duration, maxRadius, strength };
    }

    void Update(float dt) {
        for (Wave& w : waves) {
            if (w.duration <= 0.0f) continue;
            w.age += dt;
            if (w.age >= w.duration) w.duration = 0.0f;
        }
    }

    int ActiveCount() const {
        int n = 0;
        for (const Wave& w : waves) n += (w.duration > 0.0f) ? 1 : 0;
        return n;
    }

    // Ghi toi da maxCount song (x, y, banKinhHienTai, doManhHienTai) vao out[4*i..]. Ban kinh
    // no ease-out (nhanh roi cham), do manh tat theo binh phuong - song lan xa thi yeu han,
    // khong "cat" dot ngot khi het duration. Tra ve so song da ghi.
    int Fill(float* out, int maxCount) const {
        int n = 0;
        for (const Wave& w : waves) {
            if (n >= maxCount) break;
            if (w.duration <= 0.0f) continue;
            float t = w.age / w.duration;
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            float fade = (1.0f - t) * (1.0f - t);
            out[n * 4 + 0] = w.pos.x;
            out[n * 4 + 1] = w.pos.y;
            out[n * 4 + 2] = w.maxRadius * ease;
            out[n * 4 + 3] = w.strength * fade;
            n++;
        }
        return n;
    }

private:
    std::array<Wave, CAPACITY> waves{};
};

// Moi thu pass cuoi can biet ve TRANG THAI GAME trong 1 frame (khong phai cai dat nguoi choi -
// cai do o GraphicsSettings).
struct PostFxFrame {
    float waves[ShockwaveField::CAPACITY * 4] = {};
    int waveCount = 0;
    float enrage = 0.0f; // 0..1 - boss stage 2 = 0.5, stage 3 = 1 (vien man hinh ngả do)
    float hurt = 0.0f;   // 0..1 - vua mat mang: khu bao hoa thoang qua
};

// Ham thuan: dong ho "vua trung don" (giay con lai) -> muc khu bao hoa 0..1. Len ngay 1 roi tat
// dan theo binh phuong - cu "choang" ngan, khong phai man xam keo dai.
inline float HurtDesaturation(float remaining, float duration) {
    if (remaining <= 0.0f || duration <= 0.0f) return 0.0f;
    float k = remaining / duration;
    if (k > 1.0f) k = 1.0f;
    return k * k;
}
