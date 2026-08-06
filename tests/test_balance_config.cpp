#include "thirdparty/catch.hpp"
#include "config.h"
#include "enemy_types.h"
#include <fstream>
#include <cstdio>

namespace {
    const char* TestJsonPath() { return "test_balance_tmp.json"; }

    void WriteFile(const char* content) {
        std::ofstream out(TestJsonPath(), std::ios::trunc);
        out << content;
    }

    struct CleanupGuard {
        ~CleanupGuard() { std::remove(TestJsonPath()); }
    };
}

TEST_CASE("Config::LoadBalance: file khong ton tai -> giu nguyen gia tri mac dinh, khong crash", "[balance]") {
    std::remove("khong_ton_tai.json");
    float before = Config::PLAYER_SPEED;
    Config::LoadBalance("khong_ton_tai.json");
    REQUIRE(Config::PLAYER_SPEED == Approx(before));
}

TEST_CASE("Config::LoadBalance: JSON hop le ghi de dung field, KHONG dung toi field khac", "[balance]") {
    CleanupGuard guard;
    float originalFireRate = Config::PLAYER_FIRE_RATE;

    WriteFile(R"({
        "player": { "speed": 999.0 }
    })");
    Config::LoadBalance(TestJsonPath());

    REQUIRE(Config::PLAYER_SPEED == Approx(999.0f));       // Field co trong JSON -> bi ghi de
    REQUIRE(Config::PLAYER_FIRE_RATE == Approx(originalFireRate)); // Field KHONG nhac toi -> giu nguyen
}

TEST_CASE("Config::LoadBalance: sai cu phap JSON -> tu choi toan bo, khong crash, khong sua gi", "[balance]") {
    CleanupGuard guard;
    Config::PLAYER_SPEED = 400.0f; // Dat lai ve mac dinh biet truoc de so sanh

    WriteFile("{ nay khong phai JSON hop le !!! ");
    Config::LoadBalance(TestJsonPath());

    REQUIRE(Config::PLAYER_SPEED == Approx(400.0f)); // Khong bi thay doi boi file hong
}

TEST_CASE("Config::LoadBalance: 1 muc sai kieu du lieu khong lam sap cac muc khac", "[balance]") {
    CleanupGuard guard;
    Config::PLAYER_SPEED = 400.0f;

    // "speed" cua player la CHUOI thay vi so - muc "player" phai bi bo qua (giu mac
    // dinh), nhung muc "ufo" hop le phia sau van phai duoc ap dung binh thuong.
    WriteFile(R"({
        "player": { "speed": "khong-phai-so" },
        "ufo": { "speed": 12345.0 }
    })");
    Config::LoadBalance(TestJsonPath());

    REQUIRE(Config::PLAYER_SPEED == Approx(400.0f));   // Muc loi -> giu mac dinh
    REQUIRE(Config::UFO_SPEED == Approx(12345.0f));    // Muc hop le khac van duoc ap dung
}

TEST_CASE("Config::LoadBalance: ghi de duoc chi so HP/diem cua tung loai dich (enemy_stats)", "[balance]") {
    CleanupGuard guard;
    int originalTankyHp = TankyEnemy::HP;

    WriteFile(R"({
        "enemy_stats": { "tanky_hp": 7, "basic_score": 777 }
    })");
    Config::LoadBalance(TestJsonPath());

    REQUIRE(TankyEnemy::HP == 7);
    REQUIRE(BasicEnemy::SCORE_VALUE == 777);

    TankyEnemy::HP = originalTankyHp; // Khoi phuc de khong ro ri sang cac test khac chay cung binary
    BasicEnemy::SCORE_VALUE = 10;
}

TEST_CASE("Config::LoadBalance: ghi de dung bang do kho (difficulty.hard)", "[balance]") {
    CleanupGuard guard;
    float originalHardSpeed = Config::g_difficultyTable[2].enemyBaseSpeed;

    WriteFile(R"({
        "difficulty": { "hard": { "base_speed": 999.0 } }
    })");
    Config::LoadBalance(TestJsonPath());

    REQUIRE(Config::g_difficultyTable[2].enemyBaseSpeed == Approx(999.0f)); // HARD bi doi
    REQUIRE(Config::g_difficultyTable[0].enemyBaseSpeed == Approx(40.0f));  // EASY khong bi anh huong

    Config::g_difficultyTable[2].enemyBaseSpeed = originalHardSpeed; // Khoi phuc
}

TEST_CASE("Config::LoadBalance: ghi de dung muc 'dda' (B2), khong dung toi g_difficultyTable", "[balance][dda]") {
    CleanupGuard guard;
    float originalStepUp = Config::DDA_STEP_UP;
    int originalThreshold = Config::DDA_STRUGGLE_THRESHOLD;
    float originalEasySpeed = Config::g_difficultyTable[0].enemyBaseSpeed;

    WriteFile(R"({
        "dda": { "step_up": 0.2, "struggle_threshold": 5 }
    })");
    Config::LoadBalance(TestJsonPath());

    REQUIRE(Config::DDA_STEP_UP == Approx(0.2f));
    REQUIRE(Config::DDA_STRUGGLE_THRESHOLD == 5);
    // Field khac trong CUNG muc "dda" nhung khong nhac toi -> giu mac dinh.
    REQUIRE(Config::DDA_MAX_MUL == Approx(1.3f));
    // DDA la khoi RIENG - khong duoc dung vao bang do kho goc.
    REQUIRE(Config::g_difficultyTable[0].enemyBaseSpeed == Approx(originalEasySpeed));

    Config::DDA_STEP_UP = originalStepUp;
    Config::DDA_STRUGGLE_THRESHOLD = originalThreshold;
}
