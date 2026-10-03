#include "thirdparty/catch.hpp"
#include "post_fx.h"
#include "graphics_settings.h"

// ==========================================
// POST FX (GD 3) - phan thuan cua pass cuoi final.fs. Shader kiem bang anh chup; o day khoa
// vong doi song xung kich + cac quy tac preset.
// ==========================================

TEST_CASE("ShockwaveField: song no ra roi tat, het duration thi bien mat", "[post_fx][vfx]") {
    ShockwaveField f;
    f.Add({ 100.0f, 200.0f }, 80.0f, 0.4f, 6.0f);
    REQUIRE(f.ActiveCount() == 1);

    float out[ShockwaveField::CAPACITY * 4] = {};
    f.Update(0.1f);
    REQUIRE(f.Fill(out, 8) == 1);
    REQUIRE(out[0] == Approx(100.0f));
    REQUIRE(out[1] == Approx(200.0f));
    const float r1 = out[2], s1 = out[3];
    REQUIRE(r1 > 0.0f);
    REQUIRE(r1 < 80.0f);
    REQUIRE(s1 < 6.0f);

    f.Update(0.2f);
    f.Fill(out, 8);
    REQUIRE(out[2] > r1); // ban kinh tang
    REQUIRE(out[3] < s1); // do manh giam

    f.Update(0.2f);       // tong 0.5 > 0.4
    REQUIRE(f.ActiveCount() == 0);
    REQUIRE(f.Fill(out, 8) == 0);
}

TEST_CASE("ShockwaveField: day 8 song thi song MOI thay song gia nhat; Fill ton trong gioi han preset", "[post_fx][vfx][graphics]") {
    ShockwaveField f;
    for (int i = 0; i < ShockwaveField::CAPACITY; i++) {
        f.Add({ (float)i, 0.0f }, 50.0f, 1.0f, 1.0f);
        f.Update(0.05f); // song 0 gia nhat
    }
    f.Add({ 999.0f, 0.0f }, 50.0f, 1.0f, 1.0f);
    REQUIRE(f.ActiveCount() == ShockwaveField::CAPACITY);

    float out[ShockwaveField::CAPACITY * 4] = {};
    int n = f.Fill(out, ShockwaveField::CAPACITY);
    bool hasNew = false, hasOldest = false;
    for (int i = 0; i < n; i++) {
        if (out[i * 4] == Approx(999.0f)) hasNew = true;
        if (out[i * 4] == Approx(0.0f)) hasOldest = true;
    }
    REQUIRE(hasNew);
    REQUIRE_FALSE(hasOldest);

    REQUIRE(f.Fill(out, 4) == 4); // Medium chi gui 4 song
    f.Clear();
    REQUIRE(f.ActiveCount() == 0);
}

TEST_CASE("HurtDesaturation: day ngay luc trung don, tat dan, 0 khi het", "[post_fx][vfx]") {
    REQUIRE(HurtDesaturation(0.35f, 0.35f) == Approx(1.0f));
    REQUIRE(HurtDesaturation(0.175f, 0.35f) == Approx(0.25f));
    REQUIRE(HurtDesaturation(0.0f, 0.35f) == Approx(0.0f));
    REQUIRE(HurtDesaturation(0.1f, 0.0f) == Approx(0.0f)); // phong chia 0
}

TEST_CASE("GraphicsSettings: pass cuoi theo preset - CRT cong chi o High VA khi bat CRT", "[post_fx][graphics]") {
    GraphicsSettings g;
    g.quality = GraphicsQuality::Low;
    REQUIRE(g.ShockwaveMax() == 0);
    REQUIRE(g.BarrelAmount() == Approx(0.0f));
    g.quality = GraphicsQuality::Medium;
    REQUIRE(g.ShockwaveMax() == 4);
    REQUIRE_FALSE(g.ChromaticShockwave());
    REQUIRE(g.BarrelAmount() == Approx(0.0f));
    g.quality = GraphicsQuality::High;
    REQUIRE(g.ShockwaveMax() == ShockwaveField::CAPACITY);
    REQUIRE(g.ChromaticShockwave());
    REQUIRE(g.BarrelAmount() > 0.0f);
    g.crtEnabled = false;
    REQUIRE(g.BarrelAmount() == Approx(0.0f)); // tat CRT = tat ca cong
}

#include "draw_helpers.h"

TEST_CASE("NeonPowerOn: chop toi da 2 lan roi sang han; reduceFlashing thi chi sang dan, khong bao gio toi lai", "[ui][vfx][graphics]") {
    REQUIRE(NeonPowerOn(0.0f, false) == Approx(0.0f));
    REQUIRE(NeonPowerOn(5.0f, false) == Approx(1.0f));
    REQUIRE(NeonPowerOn(5.0f, true) == Approx(1.0f));

    // Dem so lan "bat" (do sang nhay len > 0.5 tu duoi 0.5) trong 1 giay dau - luat WCAG 2.3.1:
    // khong qua 3 lan/giay. Thiet ke la 2 lan chop + 1 lan sang han.
    int rises = 0;
    float prev = 0.0f;
    bool monotonicReduced = true;
    float prevReduced = 0.0f;
    for (int i = 1; i <= 100; i++) {
        float t = (float)i / 100.0f;
        float v = NeonPowerOn(t, false);
        if (prev < 0.5f && v >= 0.5f) rises++;
        prev = v;
        float r = NeonPowerOn(t, true);
        if (r + 1e-6f < prevReduced) monotonicReduced = false;
        prevReduced = r;
    }
    REQUIRE(rises <= 3);
    REQUIRE(rises >= 2); // Van co hieu ung chop (khong phai test rong)
    REQUIRE(monotonicReduced);
}
