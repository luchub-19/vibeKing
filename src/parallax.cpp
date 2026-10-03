#include "parallax.h"
#include <cmath>

namespace {
    // 1 lop do sau: toc do cuon (px/giay), khoang ban kinh, do sang kenh R/G (kenh B se
    // duoc nga xanh nhe hon - xem vong lap trong Init()), va SO LUONG sao thuoc lop nay.
    // KHOP VOI Config::PARALLAX_LAYER_COUNT = 3 (config.h) - doi so lop can sua ca mang
    // `layers` trong Parallax::Init() ben duoi, khong tu dong theo hang so.
    struct LayerDef {
        float speed;
        float radiusMin, radiusMax;
        unsigned char brightness;
        int count;
    };
}

void Parallax::Init() {
    const int total = (int)stars.size(); // == Config::PARALLAX_STAR_COUNT
    // 2 lop dau lay ti le co dinh, lop GAN NHAT lay PHAN CON LAI - tong luon dung "total"
    // du PARALLAX_STAR_COUNT co chia het cho 9 hay khong (tranh sao "chet" mac dinh o
    // (0,0) neu chia le).
    const int farCount = total * 4 / 9;
    const int midCount = total * 3 / 9;
    const int nearCount = total - farCount - midCount;

    const float speedMid = (Config::PARALLAX_SPEED_FAR + Config::PARALLAX_SPEED_NEAR) / 2.0f;
    const LayerDef layers[3] = {
        { Config::PARALLAX_SPEED_FAR,  0.6f, 1.0f, 110, farCount  }, // xa: nho/mo/cham, nhieu sao nhat
        { speedMid,                    1.0f, 1.6f, 175, midCount  }, // giua
        { Config::PARALLAX_SPEED_NEAR, 1.6f, 2.4f, 255, nearCount }, // gan: to/sang/nhanh nhat, it sao nhat
    };

    int idx = 0;
    for (const LayerDef& layer : layers) {
        for (int i = 0; i < layer.count && idx < total; ++i, ++idx) {
            Star& s = stars[idx];
            s.x = (float)GetRandomValue(0, Config::SCREEN_W);
            s.baseY = (float)GetRandomValue(0, Config::SCREEN_H);
            s.speed = layer.speed;

            int radiusRoll = GetRandomValue(0, 100);
            s.radius = layer.radiusMin + (layer.radiusMax - layer.radiusMin) * (radiusRoll / 100.0f);

            unsigned char b = layer.brightness;
            unsigned char blueBoost = (b > 235) ? 255 : (unsigned char)(b + 20); // Hoi nga xanh nhe - tranh trang/xam thuan tuy
            s.color = { b, b, blueBoost, 255 };
            s.layer = (unsigned char)(&layer - layers);
            s.phase = (float)GetRandomValue(0, 628) / 100.0f;
        }
    }
}

float Parallax::WrappedY(float baseY, float speed, float time, float screenH) {
    if (screenH <= 0.0f) return baseY; // Phong ve - SCREEN_H co dinh > 0 trong thuc te, khong nen roi vao day
    float y = fmodf(baseY + speed * time, screenH);
    return (y < 0.0f) ? y + screenH : y; // (baseY, speed, time deu >= 0 nen thuc te luon >= 0 - giu phong ve cho ro nghia)
}

float Parallax::WarpSpeedMul(float remaining) {
    if (remaining <= 0.0f) return 1.0f;
    if (remaining > WARP_DURATION) remaining = WARP_DURATION;
    float elapsed = 1.0f - remaining / WARP_DURATION; // 0 -> 1
    float env = (elapsed < 0.15f) ? elapsed / 0.15f : 1.0f - (elapsed - 0.15f) / 0.85f;
    env = env * env * (3.0f - 2.0f * env); // smoothstep - khong co goc gay o 2 dau
    return 1.0f + (WARP_PEAK_MUL - 1.0f) * env;
}

void Parallax::Update(float dt, float speedMul) {
    scrollTime += dt * speedMul;
    twinklePhase += dt;
    currentMul = speedMul;
}

void Parallax::Draw(bool twinkle) const {
    const float screenH = (float)Config::SCREEN_H;
    for (const Star& s : stars) {
        float y = WrappedY(s.baseY, s.speed, scrollTime, screenH);
        if (s.layer == 2) {
            // Vet: dai = ban kinh + quang duong ~60ms o toc do hien tai. Binh thuong ~6px,
            // luc warp x8 thanh vach ~25px - mat tu doc ra "dang lao nhanh".
            float len = s.radius * 1.5f + s.speed * currentMul * 0.06f;
            Color c = s.color;
            DrawLineEx({ s.x, y }, { s.x, y - len }, s.radius, Fade(c, 0.55f));
            DrawCircleV({ s.x, y }, s.radius * 0.8f, c);
            continue;
        }
        Color c = s.color;
        if (twinkle && s.layer == 0) {
            // 0.8 Hz, bien do 0.35-1.0 - cham va nho (cham 1px) nen khong tinh la "nhap nhay"
            // theo WCAG, nhung van tat theo reduceFlashing cho chac.
            float k = 0.675f + 0.325f * sinf(twinklePhase * 5.0f + s.phase);
            c = Fade(c, k);
        }
        DrawCircle((int)s.x, (int)y, s.radius, c);
    }
}
