#pragma once
#include "raylib.h"
#include "config.h"

// ==========================================
// OBJECT CULLING
// Bo qua lenh ve GPU cho bat ky rect nao nam HOAN TOAN ngoai vung nhin camera - ap dung
// cho cac thuc the CO THE ton tai ngoai man hinh 1 khoang thoi gian dang ke:
//   - Kamikaze: spawn tren dinh man hinh (y < 0) roi moi roi xuong.
//   - UFO: spawn han ngoai canh trai/phai (x < 0 hoac x > SCREEN_W) roi moi bay vao.
//   - Particle: trong luc chiu "trong luc" nhe (Config::PARTICLE_GRAVITY) co the bi day
//     ra ngoai bien duoi truoc khi het "life".
// KHONG ap dung cho Bullet/PowerUp: ca 2 da tu huy ngay khi vuot bien man hinh (xem
// Bullet::Update() va PowerUpPool::Update()) nen luon nam rat gan/trong man hinh trong
// suot vong doi con active - them 1 lop kiem tra o day chi la chi phi thua, khong loai
// duoc lenh ve nao ca.
//
// MARGIN nho de bu cho ScreenShake dich camera vai px moi frame - tranh cull nham 1
// thuc the vua o ria man hinh khi camera dang rung.
// ==========================================
namespace Culling {
    constexpr float MARGIN = 24.0f;

    inline bool IsVisible(const Rectangle& r) {
        return r.x + r.width  >= -MARGIN &&
               r.x            <= (float)Config::SCREEN_W + MARGIN &&
               r.y + r.height >= -MARGIN &&
               r.y            <= (float)Config::SCREEN_H + MARGIN;
    }
}
