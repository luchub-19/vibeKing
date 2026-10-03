#include "thirdparty/catch.hpp"
#include "screen_shake.h"
#include <cmath>

// ==========================================
// SCREEN SHAKE - mo hinh trauma (GD 2). Ban cu dung GetRandomValue nen khong test duoc; ban
// nay tat dinh (value noise hash), nen khoa duoc ca ngu nghia "ai de len ai" lan do muot.
// ==========================================

namespace {
    float Mag(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
}

TEST_CASE("ScreenShake: yen thi offset/xoay = 0; het duration thi ve 0 han", "[shake][vfx]") {
    ScreenShake s;
    s.Update(0.016f);
    REQUIRE(Mag(s.GetOffset()) == Approx(0.0f));
    REQUIRE(s.GetRotation() == Approx(0.0f));

    s.Trigger(0.2f, 8.0f);
    s.Update(0.01f);
    REQUIRE(s.IsActive());
    REQUIRE(Mag(s.GetOffset()) <= 8.0f * 1.4143f); // moi truc <= peak

    for (int i = 0; i < 30; i++) s.Update(0.01f); // 0.31s > 0.2s
    REQUIRE_FALSE(s.IsActive());
    REQUIRE(Mag(s.GetOffset()) == Approx(0.0f));
}

TEST_CASE("ScreenShake: rung yeu KHONG de len rung manh con dang manh", "[shake][vfx]") {
    // Neu cu yeu (0.1s) de len, sau 0.15s se het rung; cu manh (0.4s) thi van con.
    ScreenShake s;
    s.Trigger(0.4f, 12.0f);
    s.Update(0.01f);
    s.Trigger(0.1f, 4.0f);
    for (int i = 0; i < 15; i++) s.Update(0.01f);
    REQUIRE(s.IsActive());
}

TEST_CASE("ScreenShake: rung manh da tat gan het thi rung moi vua phai duoc thay the", "[shake][vfx]") {
    // Sau 0.36/0.4s cu manh con 12 * 0.1^2 = 0.12px < 4px -> cu moi (0.3s) thang. Neu so theo
    // peak goc (12 > 4) nhu ban cu thi cu moi bi bo, va o moc 0.46s da het rung.
    ScreenShake s;
    s.Trigger(0.4f, 12.0f);
    for (int i = 0; i < 36; i++) s.Update(0.01f);
    s.Trigger(0.3f, 4.0f);
    for (int i = 0; i < 10; i++) s.Update(0.01f);
    REQUIRE(s.IsActive());
}

TEST_CASE("ScreenShake: tat dinh va MUOT giua cac frame lien tiep", "[shake][vfx]") {
    ScreenShake a, b;
    a.Trigger(0.5f, 10.0f);
    b.Trigger(0.5f, 10.0f);
    Vector2 prev{};
    float worstJump = 0.0f;
    for (int i = 0; i < 60; i++) {
        a.Update(1.0f / 240.0f);
        b.Update(1.0f / 240.0f);
        REQUIRE(a.GetOffset().x == b.GetOffset().x); // cung chuoi -> cung ket qua
        Vector2 o = a.GetOffset();
        if (i > 0) worstJump = fmaxf(worstJump, Mag({ o.x - prev.x, o.y - prev.y }));
        prev = o;
    }
    // 240fps: 1 frame = ~0.09 chu ky nhieu -> buoc nhay nho hon han bien do. Random rời rạc
    // (moi frame 1 diem moi) se cho buoc nhay ~ ca bien do.
    REQUIRE(worstJump < 10.0f * 0.5f);
}

TEST_CASE("ScreenShake::SmoothNoise: nam trong [-1,1] va lien tuc qua diem nguyen", "[shake][vfx]") {
    for (int i = 0; i < 200; i++) {
        float x = (float)i * 0.173f;
        float v = ScreenShake::SmoothNoise(x, 7u);
        REQUIRE(v >= -1.0f);
        REQUIRE(v <= 1.0f);
    }
    REQUIRE(ScreenShake::SmoothNoise(3.0f - 1e-4f, 1u) == Approx(ScreenShake::SmoothNoise(3.0f + 1e-4f, 1u)).margin(1e-2));
}
