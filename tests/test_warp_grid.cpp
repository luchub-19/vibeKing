#include "thirdparty/catch.hpp"
#include "warp_grid.h"
#include <cmath>

// ==========================================
// WARP GRID (GD 1) - mo phong lo xo thuan CPU, khong can raylib. Hinh dang luoi la viec cua
// anh chup; o day khoa cac TINH CHAT vat ly ma loi dau/cong thuc se pha vo: luoi dung yen tu
// dung yen, vu no day RA (khong hut vao), vien khong bao gio dich, va luon dan hoi ve.
// ==========================================

namespace {
    float Dist(Vector2 a, Vector2 b) { return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

    float MaxDeviation(const WarpGrid& g) {
        float worst = 0.0f;
        for (int r = 0; r < g.Rows(); r++)
            for (int c = 0; c < g.Cols(); c++) worst = fmaxf(worst, Dist(g.PointAt(c, r), g.RestAt(c, r)));
        return worst;
    }

    void Simulate(WarpGrid& g, float seconds) {
        int steps = (int)(seconds / WarpGrid::STEP);
        for (int i = 0; i < steps; i++) g.Update(WarpGrid::STEP, true);
    }
}

TEST_CASE("WarpGrid: luoi phu kin man hinh ke ca khi kich thuoc khong chia het cho o", "[warp_grid][vfx]") {
    WarpGrid g;
    g.Init(800.0f, 600.0f, 32.0f); // 800/32 = 25, 600/32 = 18.75
    REQUIRE(g.Cols() == 26);
    REQUIRE(g.Rows() == 20);
    REQUIRE(g.RestAt(g.Cols() - 1, 0).x >= 800.0f);
    REQUIRE(g.RestAt(0, g.Rows() - 1).y >= 600.0f);
}

TEST_CASE("WarpGrid: khong co luc nao thi dung yen (luc lo xo can bang - khong troi)", "[warp_grid][vfx]") {
    WarpGrid g;
    g.Init(800.0f, 600.0f, 32.0f);
    Simulate(g, 2.0f);
    REQUIRE(MaxDeviation(g) < 1e-3f);
}

TEST_CASE("WarpGrid: vu no day diem gan RA XA tam, diem xa khong bi anh huong", "[warp_grid][vfx]") {
    WarpGrid g;
    g.Init(800.0f, 600.0f, 32.0f);
    const Vector2 center{ 400.0f, 300.0f };
    const int nc = 13, nr = 9;               // (416, 288): cach tam ~20px, trong ban kinh
    const int fc = 2, fr = 2;                // (64, 64): rat xa
    const float before = Dist(g.PointAt(nc, nr), center);

    g.ApplyExplosiveForce(center, 4.0f, 90.0f);
    // Lay khoang cach LON NHAT trong 0.2s, khong doc 1 thoi diem co dinh: luoi dao dong chu ky
    // ~8 buoc (~0.14s), doc nham luc no di qua vi tri can bang thi thay "khong nhuc nhich"
    // (ban dau test nay do dung vi the - 20.08 vs 20.0).
    float farthest = 0.0f;
    for (int i = 0; i < 12; i++) {
        g.Update(WarpGrid::STEP, true);
        farthest = fmaxf(farthest, Dist(g.PointAt(nc, nr), center));
    }
    REQUIRE(farthest > before + 1.0f); // dau sai (hut vao) se khong bao gio ra xa hon `before`
    REQUIRE(Dist(g.PointAt(fc, fr), g.RestAt(fc, fr)) < 1e-3f);
}

TEST_CASE("WarpGrid: vien la diem neo - khong bao gio dich du no sat vien", "[warp_grid][vfx]") {
    WarpGrid g;
    g.Init(800.0f, 600.0f, 32.0f);
    g.ApplyExplosiveForce({ 10.0f, 10.0f }, 10.0f, 240.0f);
    Simulate(g, 0.5f);
    for (int c = 0; c < g.Cols(); c++) {
        REQUIRE(Dist(g.PointAt(c, 0), g.RestAt(c, 0)) == Approx(0.0f));
        REQUIRE(Dist(g.PointAt(c, g.Rows() - 1), g.RestAt(c, g.Rows() - 1)) == Approx(0.0f));
    }
}

TEST_CASE("WarpGrid: sau vu no lon luoi dan hoi ve hinh goc (khong mat on dinh)", "[warp_grid][vfx]") {
    WarpGrid g;
    g.Init(800.0f, 600.0f, 32.0f);
    g.ApplyExplosiveForce({ 400.0f, 300.0f }, 10.0f, 240.0f);
    Simulate(g, 0.2f);
    REQUIRE(MaxDeviation(g) > 5.0f);   // that su bien dang (khong phai test rong)
    Simulate(g, 10.0f);
    REQUIRE(MaxDeviation(g) < 0.5f);   // va da ve lai, khong no tung hay dao dong mai
}

TEST_CASE("WarpGrid: Update(simulate=false) - preset Low, khong diem nao di chuyen", "[warp_grid][vfx][graphics]") {
    WarpGrid g;
    g.Init(800.0f, 600.0f, 32.0f);
    g.ApplyExplosiveForce({ 400.0f, 300.0f }, 10.0f, 240.0f);
    for (int i = 0; i < 30; i++) g.Update(WarpGrid::STEP, false);
    REQUIRE(MaxDeviation(g) < 1e-3f);
}

TEST_CASE("WarpGrid::CalmFactor: nen diu dan khi dan dich day man (luat R2)", "[warp_grid][vfx]") {
    REQUIRE(WarpGrid::CalmFactor(0) == Approx(1.0f));
    REQUIRE(WarpGrid::CalmFactor(10) == Approx(1.0f));
    REQUIRE(WarpGrid::CalmFactor(25) == Approx(0.7f));
    REQUIRE(WarpGrid::CalmFactor(40) == Approx(0.4f));
    REQUIRE(WarpGrid::CalmFactor(400) == Approx(0.4f));
}
