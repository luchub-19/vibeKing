#include "thirdparty/catch.hpp"
#include "settings.h"
#include <cstdio>
#include <fstream>

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
