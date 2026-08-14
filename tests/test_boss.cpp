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
// PhysicsSystem/RenderSystem duoc dung truc tiep - xem game_manager.h). File nay CHI test
// phan khong can noi long private/friend:
//   1) BossTypeName() - free function thuan.
//   2) Cong thuc xoay vong loai Boss dung trong SpawnBoss() - tach rieng ra day de test
//      dung cong thuc (khong goi thang duoc ham private SpawnBoss()).
//   3) Config::LoadBalance() doc dung cac field Sentinel/Swarmer moi them tu JSON - noi
//      tiep pattern co san trong test_balance_config.cpp.
// [CAP NHAT] boss.hp/BossStage()/khien Sentinel GIO co test rieng qua CheckCollisions()
// THAT (tests/test_physics_system.cpp, dung 1 friend test-only - GameManagerTestAccess,
// xem tests/game_manager_test_access.h) - khong dat lai o day de tranh trung lap. Van
// CHUA co test tu dong cho hanh vi DONG cua rieng UpdateBoss() (lac trong man hinh, nhip
// bat/tat khien, trieu hoi dung so luong/khong vuot MAX_KAMIKAZE) - phan do van chi duoc
// xac minh bang doc code thu cong + chay thu binary qua Xvfb nhu truoc, khong doi.
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

// ==========================================
// BOSS TYPE DESCRIPTOR (B4) - GetBossTypeDescriptor()/g_bossTypeDescriptors[] la free
// function/bien thuan (khong can GameManager) nen test duoc ngay tai day, dung tinh than
// "chi test phan khong can noi long private/friend" cua file nay (xem comment dau file).
// ==========================================
TEST_CASE("GetBossTypeDescriptor: dung movement pattern + co che rieng cho ca 3 loai", "[boss][descriptor]") {
    const BossTypeDescriptor& vanguard = GetBossTypeDescriptor(BossType::Vanguard);
    REQUIRE(vanguard.movement == BossMovementPattern::Pace);
    REQUIRE_FALSE(vanguard.hasShieldMechanic);
    REQUIRE_FALSE(vanguard.hasSummonMechanic);

    const BossTypeDescriptor& sentinel = GetBossTypeDescriptor(BossType::Sentinel);
    REQUIRE(sentinel.movement == BossMovementPattern::Sway);
    REQUIRE(sentinel.hasShieldMechanic);
    REQUIRE_FALSE(sentinel.hasSummonMechanic);
    REQUIRE(sentinel.swayAmplitude != nullptr);
    REQUIRE(sentinel.shieldFireInterval != nullptr);

    const BossTypeDescriptor& swarmer = GetBossTypeDescriptor(BossType::Swarmer);
    REQUIRE(swarmer.movement == BossMovementPattern::Sway);
    REQUIRE_FALSE(swarmer.hasShieldMechanic);
    REQUIRE(swarmer.hasSummonMechanic);
    REQUIRE(swarmer.summonCount != nullptr);
    // Phase 4 (Enemy & Item Revolution): summonPool phai co du 3 loai {Kamikaze,Weaver,
    // Bomber} - truoc Phase 4 Swarmer LUON trieu hoi Kamikaze, gio phai co the ra ca 3.
    REQUIRE(swarmer.summonPool != nullptr);
    REQUIRE(swarmer.summonPoolSize == 3);
    bool hasKamikaze = false, hasWeaver = false, hasBomber = false;
    for (int i = 0; i < swarmer.summonPoolSize; i++) {
        if (swarmer.summonPool[i] == SummonKind::Kamikaze) hasKamikaze = true;
        if (swarmer.summonPool[i] == SummonKind::Weaver) hasWeaver = true;
        if (swarmer.summonPool[i] == SummonKind::Bomber) hasBomber = true;
    }
    REQUIRE(hasKamikaze);
    REQUIRE(hasWeaver);
    REQUIRE(hasBomber);

    // Vanguard/Sentinel khong trieu hoi gi - summonPoolSize phai la 0 (an toan, khong ai
    // vo tinh doc mang null).
    REQUIRE(vanguard.summonPoolSize == 0);
    REQUIRE(sentinel.summonPoolSize == 0);
}

TEST_CASE("GetBossTypeDescriptor: con tro TRO THANG Config (khong sao chep) - LoadBalance ghi de van thay ngay", "[boss][descriptor][balance]") {
    BossCleanupGuard guard;
    float origAmp = Config::BOSS_SENTINEL_SWAY_AMPLITUDE;

    const BossTypeDescriptor& sentinel = GetBossTypeDescriptor(BossType::Sentinel);
    REQUIRE(*sentinel.swayAmplitude == Approx(origAmp));

    WriteBossTestFile(R"({ "boss": { "sentinel_sway_amplitude": 321.0 } })");
    Config::LoadBalance(BossTestJsonPath());

    // Doc lai QUA CUNG 1 con tro (khong goi lai GetBossTypeDescriptor) - phai thay gia
    // tri MOI ngay lap tuc vi descriptor chi giu dia chi, khong giu ban sao.
    REQUIRE(*sentinel.swayAmplitude == Approx(321.0f));

    Config::BOSS_SENTINEL_SWAY_AMPLITUDE = origAmp;
}
