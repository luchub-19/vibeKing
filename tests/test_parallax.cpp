#include "thirdparty/catch.hpp"
#include "parallax.h"
#include "config.h"

// ==========================================
// PARALLAX (Nguoi 2 - Lighting & World) - chi test Parallax::WrappedY(), ham THUAN duy
// nhat khong dung GetTime()/trang thai raylib nao (xem parallax.h). Init()/Draw() can GPU
// context that (GetRandomValue can InitWindow() de co RNG seed thuc su co y nghia,
// DrawCircle can canvas dang mo) nen KHONG test truc tiep o day - cung tinh than voi cac
// he thong GPU-only khac trong du an (khong co test_post_process.cpp vi ly do tuong tu:
// toan bo la shader/render texture, khong co logic thuan nao de tach ra test headless).
// ==========================================

TEST_CASE("Parallax::WrappedY: time=0 tra ve dung baseY (chua di chuyen)", "[parallax]") {
    REQUIRE(Parallax::WrappedY(100.0f, 50.0f, 0.0f, 600.0f) == Approx(100.0f));
    REQUIRE(Parallax::WrappedY(0.0f, 20.0f, 0.0f, 600.0f) == Approx(0.0f));
}

TEST_CASE("Parallax::WrappedY: chua vuot screenH thi di chuyen dung speed*time", "[parallax]") {
    // baseY=50, speed=100 px/s, time=2s -> 50+200=250, con < 600 nen chua wrap.
    REQUIRE(Parallax::WrappedY(50.0f, 100.0f, 2.0f, 600.0f) == Approx(250.0f));
}

TEST_CASE("Parallax::WrappedY: vuot screenH thi quay vong ve dung phan du", "[parallax]") {
    // baseY=500, speed=100, time=2 -> 500+200=700, 700 mod 600 = 100.
    REQUIRE(Parallax::WrappedY(500.0f, 100.0f, 2.0f, 600.0f) == Approx(100.0f));
    // Wrap nhieu vong: 500 + 100*20 = 2500 = 4*600 + 100 -> van ve dung 100.
    REQUIRE(Parallax::WrappedY(500.0f, 100.0f, 20.0f, 600.0f) == Approx(100.0f));
}

TEST_CASE("Parallax::WrappedY: ket qua luon nam trong [0, screenH) voi nhieu input khac nhau", "[parallax]") {
    float screenH = 600.0f;
    for (float baseY : { 0.0f, 150.0f, 599.0f }) {
        for (float speed : { 0.0f, 12.0f, 45.0f, 200.0f }) {
            for (float t : { 0.0f, 0.5f, 3.7f, 100.0f }) {
                float y = Parallax::WrappedY(baseY, speed, t, screenH);
                REQUIRE(y >= 0.0f);
                REQUIRE(y < screenH);
            }
        }
    }
}

TEST_CASE("Parallax::WrappedY: lop toc do nhanh hon di duoc quang duong xa hon tai cung 1 thoi diem (chua wrap)", "[parallax]") {
    // Cung baseY=0, cung time, nhung con trong mien de so sanh truc tiep (khong bi phep
    // mod lam gap) - PARALLAX_SPEED_FAR/NEAR (config.h) trong 1 giay deu con << screenH.
    float screenH = 600.0f;
    float yFar = Parallax::WrappedY(0.0f, Config::PARALLAX_SPEED_FAR, 1.0f, screenH);
    float yNear = Parallax::WrappedY(0.0f, Config::PARALLAX_SPEED_NEAR, 1.0f, screenH);
    REQUIRE(yNear > yFar); // lop gan (nhanh hon) di xa hon lop xa (cham hon) trong cung 1 giay
}

TEST_CASE("Parallax::WarpSpeedMul: khong warp = 1, len dinh roi ve lai 1, khong bao gio duoi 1", "[parallax][vfx]") {
    REQUIRE(Parallax::WarpSpeedMul(0.0f) == Approx(1.0f));
    REQUIRE(Parallax::WarpSpeedMul(-1.0f) == Approx(1.0f));
    // Vua bat dau (con nguyen thoi gian) -> chua tang: tranh giat cuc tu 1 len 8 trong 1 frame
    REQUIRE(Parallax::WarpSpeedMul(Parallax::WARP_DURATION) == Approx(1.0f));
    // Dinh o 15% thoi gian
    float peak = Parallax::WarpSpeedMul(Parallax::WARP_DURATION * 0.85f);
    REQUIRE(peak == Approx(Parallax::WARP_PEAK_MUL));
    float prev = peak;
    for (int i = 1; i <= 20; i++) {
        float remaining = Parallax::WARP_DURATION * 0.85f * (1.0f - (float)i / 20.0f);
        float m = Parallax::WarpSpeedMul(remaining);
        REQUIRE(m <= prev + 1e-4f); // pha phanh: giam don dieu
        REQUIRE(m >= 1.0f);
        prev = m;
    }
}
