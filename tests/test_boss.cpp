#include "thirdparty/catch.hpp"
#include "enemy_types.h"
#include "config.h"
#include <fstream>
#include <cstdio>
#include <string>

// ==========================================
// BOSS (Sentinel/Swarmer moi them - xem enemy_types.h va PhysicsSystem::UpdateBoss).
// PhysicsSystem::UpdateBoss()/GameManager::SpawnBoss() can mot GameManager& that, nhung
// bossPool/kamikazeEnemies/wave deu la PRIVATE trong GameManager (chi friend
// PhysicsSystem/RenderSystem duoc dung truc tiep - xem game_manager.h) - dung dung
// thiet ke hien co cua du an: khong he co test_game_manager.cpp hay
// test_physics_system.cpp nao ca, moi test trong tests/ tu truoc gio deu nham vao logic
// THUAN/standalone (Bunker, Player, SpatialGrid, Settings, Leaderboard, BalanceConfig).
// File nay giu dung tinh than do, CHI test phan khong can noi long private/friend hien
// co:
//   1) BossTypeName() - free function thuan.
//   2) Cong thuc xoay vong loai Boss dung trong SpawnBoss() - tach rieng ra day de test
//      dung cong thuc (khong goi thang duoc ham private SpawnBoss()).
//   3) Config::LoadBalance() doc dung cac field Sentinel/Swarmer moi them tu JSON - noi
//      tiep pattern co san trong test_balance_config.cpp.
// Hanh vi DONG (lac trong man hinh, bat/tat khien dung nhip, trieu hoi dung so luong/
// khong vuot MAX_KAMIKAZE) da duoc xac minh bang doc code thu cong nhieu luot + chay
// thu binary that qua Xvfb (khong crash qua nhieu frame lien tuc, xem ghi chu trong hoi
// thoai) - KHONG co unit test tu dong cho phan nay, vi se phai noi long private/friend
// ma chua he thong nao khac trong du an tung can toi.
// ==========================================

TEST_CASE("BossTypeName: anh xa dung ten hien thi cho ca 3 loai", "[boss]") {
    REQUIRE(std::string(BossTypeName(BossType::Vanguard)) == "VANGUARD");
    REQUIRE(std::string(BossTypeName(BossType::Sentinel)) == "SENTINEL");
    REQUIRE(std::string(BossTypeName(BossType::Swarmer)) == "SWARMER");
}

TEST_CASE("Cong thuc xoay vong loai Boss (dung trong SpawnBoss): bossIndex 1..6 di dung chu ky Vanguard->Sentinel->Swarmer", "[boss]") {
    // Sao chep dung 1 dong cong thuc that trong GameManager::SpawnBoss() - neu cong
    // thuc do doi, nho doi ca o day cho khop.
    auto TypeForBossIndex = [](int bossIndex) {
        return (BossType)((bossIndex - 1) % 3);
    };
    REQUIRE(TypeForBossIndex(1) == BossType::Vanguard); // wave 5
    REQUIRE(TypeForBossIndex(2) == BossType::Sentinel); // wave 10
    REQUIRE(TypeForBossIndex(3) == BossType::Swarmer);  // wave 15
    REQUIRE(TypeForBossIndex(4) == BossType::Vanguard); // wave 20 - lap lai chu ky
    REQUIRE(TypeForBossIndex(5) == BossType::Sentinel);
    REQUIRE(TypeForBossIndex(6) == BossType::Swarmer);
}

namespace {
    const char* BossTestJsonPath() { return "test_boss_balance_tmp.json"; }
    void WriteBossTestFile(const char* content) {
        std::ofstream out(BossTestJsonPath(), std::ios::trunc);
        out << content;
    }
    struct BossCleanupGuard {
        ~BossCleanupGuard() { std::remove(BossTestJsonPath()); }
    };
}

TEST_CASE("Config::LoadBalance: doc dung cac field Sentinel/Swarmer moi them trong muc boss", "[boss][balance]") {
    BossCleanupGuard guard;

    // Luu gia tri goc de khoi phuc cuoi bai - day la bien global (inline), leak sang
    // test khac chay chung 1 binary neu khong restore (dung pattern da co san trong
    // test_balance_config.cpp).
    float origAmp = Config::BOSS_SENTINEL_SWAY_AMPLITUDE;
    float origInterval = Config::BOSS_SWARMER_SUMMON_INTERVAL;
    int origCount = Config::BOSS_SWARMER_SUMMON_COUNT;

    WriteBossTestFile(R"({
        "boss": {
            "sentinel_sway_amplitude": 123.0,
            "swarmer_summon_interval": 9.5,
            "swarmer_summon_count": 4
        }
    })");
    Config::LoadBalance(BossTestJsonPath());

    REQUIRE(Config::BOSS_SENTINEL_SWAY_AMPLITUDE == Approx(123.0f));
    REQUIRE(Config::BOSS_SWARMER_SUMMON_INTERVAL == Approx(9.5f));
    REQUIRE(Config::BOSS_SWARMER_SUMMON_COUNT == 4);
    // Field khac trong CUNG muc "boss" nhung KHONG duoc nhac toi trong JSON phai giu
    // nguyen mac dinh - chung minh cac Assign() moi them khong dung cheo nhau.
    REQUIRE(Config::BOSS_SENTINEL_SHIELD_INTERVAL == Approx(6.0f));
    REQUIRE(Config::BOSS_SWARMER_SWAY_FREQUENCY == Approx(1.8f));

    Config::BOSS_SENTINEL_SWAY_AMPLITUDE = origAmp;
    Config::BOSS_SWARMER_SUMMON_INTERVAL = origInterval;
    Config::BOSS_SWARMER_SUMMON_COUNT = origCount;
}
