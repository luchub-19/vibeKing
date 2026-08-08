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
        }
    }
}

float Parallax::WrappedY(float baseY, float speed, float time, float screenH) {
    if (screenH <= 0.0f) return baseY; // Phong ve - SCREEN_H co dinh > 0 trong thuc te, khong nen roi vao day
    float y = fmodf(baseY + speed * time, screenH);
    return (y < 0.0f) ? y + screenH : y; // (baseY, speed, time deu >= 0 nen thuc te luon >= 0 - giu phong ve cho ro nghia)
}

void Parallax::Draw() const {
    const float t = (float)GetTime();
    const float screenH = (float)Config::SCREEN_H;
    for (const Star& s : stars) {
        float y = WrappedY(s.baseY, s.speed, t, screenH);
        DrawCircle((int)s.x, (int)y, s.radius, s.color);
    }
}
