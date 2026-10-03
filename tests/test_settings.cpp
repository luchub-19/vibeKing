#include "thirdparty/catch.hpp"
#include "settings.h"
#include "particle_pool.h"
#include <cstdio>
#include <fstream>
#include <cmath>

// ==========================================
// SETTINGS - truoc ban sua nay chua co test nao (chi HighScore/Leaderboard co). Tap
// trung vao round-trip Save->Load cho 4 field ma phim moi them (keyMoveLeft/Right/
// Shoot/Pause) - day la cach dang tin cay nhat de verify logic persist thuc su dung,
// vi mo phong nhan phim that qua X11/Xvfb trong moi truong sandbox headless (khong
// window manager that, khong bao gio chac chan GLFW nhan dung focus) cho ket qua
// khong on dinh giua cac lan chay - khong phan anh dung logic C++ thuc te.
// ==========================================

namespace {
    const char* TestPath() { return "test_settings_tmp.cfg"; }

    struct CleanupGuard {
        ~CleanupGuard() { std::remove(TestPath()); }
    };
}

TEST_CASE("Settings: file khong ton tai -> dung gia tri mac dinh, khong crash", "[settings]") {
    CleanupGuard guard;
    std::remove(TestPath());

    Settings cfg = Settings::LoadFromFile(TestPath());
    REQUIRE(cfg.keyMoveLeft == KEY_A);
    REQUIRE(cfg.keyMoveRight == KEY_D);
    REQUIRE(cfg.keyShoot == KEY_SPACE);
    REQUIRE(cfg.keyPause == KEY_P);
}

TEST_CASE("Settings: Save roi Load lai khop CHINH XAC toan bo field, ke ca 4 phim moi", "[settings]") {
    CleanupGuard guard;
    std::remove(TestPath());

    Settings original;
    original.difficulty = Difficulty::HARD;
    original.volume = 0.42f;
    original.keyMoveLeft = KEY_J;
    original.keyMoveRight = KEY_L;
    original.keyShoot = KEY_K;
    original.keyPause = KEY_ZERO;
    original.SaveToFile(TestPath());

    Settings loaded = Settings::LoadFromFile(TestPath());
    REQUIRE(loaded.difficulty == Difficulty::HARD);
    REQUIRE(loaded.volume == Approx(0.42f));
    REQUIRE(loaded.keyMoveLeft == KEY_J);
    REQUIRE(loaded.keyMoveRight == KEY_L);
    REQUIRE(loaded.keyShoot == KEY_K);
    REQUIRE(loaded.keyPause == KEY_ZERO);
}

TEST_CASE("Settings: ma phim vo ly trong file (am hoac vuot MAX_KEYBOARD_KEYS) -> tra ve mac dinh cho field do", "[settings]") {
    CleanupGuard guard;
    std::remove(TestPath());

    {
        std::ofstream file(TestPath(), std::ios::trunc);
        file << "KEY_MOVE_LEFT=-5\n";   // Am - vo ly
        file << "KEY_MOVE_RIGHT=99999\n"; // Vuot xa MAX_KEYBOARD_KEYS=512 - vo ly
        file << "KEY_SHOOT=65\n";        // Hop le (KEY_A) - phai giu nguyen, KHONG bi anh huong boi 2 dong loi ben tren
    }

    Settings cfg = Settings::LoadFromFile(TestPath());
    REQUIRE(cfg.keyMoveLeft == KEY_A);   // Bi loai -> ve mac dinh
    REQUIRE(cfg.keyMoveRight == KEY_D);  // Bi loai -> ve mac dinh
    REQUIRE(cfg.keyShoot == KEY_A);      // Gia tri hop le duoc ap dung dung nhu file ghi
}

TEST_CASE("Settings: chi 1 phim doi thi cac phim con lai khong bi anh huong", "[settings]") {
    CleanupGuard guard;
    std::remove(TestPath());

    Settings cfg;
    cfg.keyShoot = KEY_ENTER; // Doi DUY NHAT 1 phim
    cfg.SaveToFile(TestPath());

    Settings loaded = Settings::LoadFromFile(TestPath());
    REQUIRE(loaded.keyShoot == KEY_ENTER);
    REQUIRE(loaded.keyMoveLeft == KEY_A);   // Khong doi
    REQUIRE(loaded.keyMoveRight == KEY_D);  // Khong doi
    REQUIRE(loaded.keyPause == KEY_P);      // Khong doi
}

TEST_CASE("Settings::ResetKeyBindingsToDefault() dua ca 4 phim ve dung mac dinh", "[settings]") {
    Settings cfg;
    cfg.keyMoveLeft = KEY_J;
    cfg.keyMoveRight = KEY_L;
    cfg.keyShoot = KEY_K;
    cfg.keyPause = KEY_ZERO;

    cfg.ResetKeyBindingsToDefault();

    REQUIRE(cfg.keyMoveLeft == KEY_A);
    REQUIRE(cfg.keyMoveRight == KEY_D);
    REQUIRE(cfg.keyShoot == KEY_SPACE);
    REQUIRE(cfg.keyPause == KEY_P);
}

TEST_CASE("Settings: VOLUME=nan -> ve mac dinh, khong lot qua clamp", "[settings]") {
    CleanupGuard guard;
    {
        std::ofstream file(TestPath(), std::ios::trunc);
        file << "VOLUME=nan\n";
    }
    Settings loaded = Settings::LoadFromFile(TestPath());
    REQUIRE_FALSE(std::isnan(loaded.volume));
    REQUIRE(loaded.volume == Approx(Settings{}.volume));
}

// ==========================================
// GRAPHICS SETTINGS (trang GRAPHICS, graphics_settings.h)
// ==========================================

TEST_CASE("Settings: 4 field do hoa round-trip qua file", "[settings][graphics]") {
    CleanupGuard guard;
    std::remove(TestPath());

    // Moi field dat KHAC mac dinh - neu Save/Load bo sot 1 field thi gia tri mac dinh lot
    // qua va REQUIRE tuong ung do (mac dinh: Medium / CRT bat / khong giam / 100%).
    Settings original;
    original.graphics.quality = GraphicsQuality::Low;
    original.graphics.crtEnabled = false;
    original.graphics.reduceFlashing = true;
    original.graphics.shakePercent = 50;
    original.SaveToFile(TestPath());

    Settings loaded = Settings::LoadFromFile(TestPath());
    REQUIRE(loaded.graphics.quality == GraphicsQuality::Low);
    REQUIRE_FALSE(loaded.graphics.crtEnabled);
    REQUIRE(loaded.graphics.reduceFlashing);
    REQUIRE(loaded.graphics.shakePercent == 50);
}

TEST_CASE("Settings: settings.cfg cu (truoc khi co GFX_*) -> do hoa giu mac dinh = dien mao cu", "[settings][graphics]") {
    CleanupGuard guard;
    {
        std::ofstream f(TestPath());
        f << "DIFFICULTY=HARD\nVOLUME=0.5\n";
    }
    Settings loaded = Settings::LoadFromFile(TestPath());
    REQUIRE(loaded.graphics.quality == GraphicsQuality::Medium);
    REQUIRE(loaded.graphics.crtEnabled);
    REQUIRE_FALSE(loaded.graphics.reduceFlashing);
    REQUIRE(loaded.graphics.shakePercent == 100);
}

TEST_CASE("Settings: gia tri GFX_* sua tay sai -> mac dinh, GFX_SHAKE lam tron ve muc gan nhat", "[settings][graphics]") {
    CleanupGuard guard;
    {
        std::ofstream f(TestPath());
        f << "GFX_QUALITY=ULTRA\nGFX_CRT=yes\nGFX_REDUCE_FLASHING=2\nGFX_SHAKE=73\n";
    }
    Settings loaded = Settings::LoadFromFile(TestPath());
    REQUIRE(loaded.graphics.quality == GraphicsQuality::Medium);
    REQUIRE(loaded.graphics.crtEnabled);
    REQUIRE_FALSE(loaded.graphics.reduceFlashing);
    REQUIRE(loaded.graphics.shakePercent == 50); // |73-50|=23 < |73-100|=27

    {
        std::ofstream f(TestPath());
        f << "GFX_QUALITY=high\nGFX_SHAKE=-40\n"; // khong phan biet hoa/thuong; am -> OFF
    }
    loaded = Settings::LoadFromFile(TestPath());
    REQUIRE(loaded.graphics.quality == GraphicsQuality::High);
    REQUIRE(loaded.graphics.shakePercent == 0);
}

TEST_CASE("GraphicsSettings: preset quyet dinh bloom/particle, Cycle quay vong 2 chieu", "[graphics]") {
    GraphicsSettings g;
    g.quality = GraphicsQuality::Low;
    REQUIRE_FALSE(g.BloomEnabled());
    REQUIRE(g.ParticleScale() == Approx(0.5f));
    g.quality = GraphicsQuality::Medium;
    REQUIRE(g.BloomEnabled());
    REQUIRE(g.ParticleScale() == Approx(1.0f));
    g.quality = GraphicsQuality::High;
    REQUIRE(g.ParticleScale() == Approx(1.5f));
    g.quality = GraphicsQuality::Medium;

    g.quality = GraphicsQuality::High;
    g.CycleQuality(1);
    REQUIRE(g.quality == GraphicsQuality::Low);   // High -> Low
    g.CycleQuality(-1);
    REQUIRE(g.quality == GraphicsQuality::High);  // Low -> High

    g.shakePercent = 100;
    g.CycleShake(1);  REQUIRE(g.shakePercent == 50);
    g.CycleShake(1);  REQUIRE(g.shakePercent == 0);
    g.CycleShake(1);  REQUIRE(g.shakePercent == 100);
    g.CycleShake(-1); REQUIRE(g.shakePercent == 0);
    REQUIRE(g.ShakeScale() == Approx(0.0f));

    g.reduceFlashing = true;
    REQUIRE(g.CrtFlickerScale() == Approx(0.0f));
}

TEST_CASE("ParticlePool::ScaledCount: giam theo preset nhung khong bao gio xoa mat 1 cum", "[graphics]") {
    REQUIRE(ParticlePool<8>::ScaledCount(40, 0.5f) == 20);
    REQUIRE(ParticlePool<8>::ScaledCount(3, 0.5f) == 2);  // 1.5 lam tron len
    REQUIRE(ParticlePool<8>::ScaledCount(1, 0.5f) == 1);  // muzzle flash 1 hat van con
    REQUIRE(ParticlePool<8>::ScaledCount(0, 0.5f) == 0);
    REQUIRE(ParticlePool<8>::ScaledCount(7, 1.0f) == 7);
}

TEST_CASE("GraphicsSettings: lop nen theo preset - Low tat luoi + tinh van, High luoi day hon + 2 lop tinh van", "[graphics][vfx]") {
    GraphicsSettings g;
    g.quality = GraphicsQuality::Low;
    REQUIRE_FALSE(g.GridEnabled());
    REQUIRE(g.NebulaLayers() == 0);
    g.quality = GraphicsQuality::Medium;
    REQUIRE(g.GridEnabled());
    REQUIRE(g.NebulaLayers() == 1);
    const float mediumCell = g.GridCellSize();
    g.quality = GraphicsQuality::High;
    REQUIRE(g.NebulaLayers() == 2);
    REQUIRE(g.GridCellSize() < mediumCell);
}
