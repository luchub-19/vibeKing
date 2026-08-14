#include "thirdparty/catch.hpp"
#include "upgrade_types.h"
#include "player.h"
#include "config.h"

// ==========================================
// TEST_UPGRADES (Track C - Nguoi 2, Phase 3: he thong chon nang cap sau wave) - 2 nhom:
//   1. Bang descriptor g_upgradeTypeDescriptors[]/GetUpgradeTypeDescriptor() - du lieu
//      THUAN, khong dung Player/GameManager (dung khuon tests/test_boss.cpp cho
//      BossTypeDescriptor).
//   2. Player::ApplyRunUpgrade() - logic cong don THAT SU tren tung field (speed/lives/
//      score), khong can InitWindow/GPU nen chay duoc trong unit_tests binh thuong (dung
//      khuon tests/test_player.cpp).
// Man hinh chon that su (cycle Trai/Phai + Confirm trong GameManager::UpdateEndScreen())
// duoc test rieng trong tests/test_game_manager.cpp - khong the "gia bam phim" o day (xem
// comment dau file do ve gioi han cua PollMenu() luc headless).
// ==========================================

TEST_CASE("GetUpgradeTypeDescriptor: ca 3 loai co ten/mo ta hop le, coefficient dung nullptr dung cho", "[upgrade][descriptor]") {
    const UpgradeTypeDescriptor& speed = GetUpgradeTypeDescriptor(UpgradeType::MoveSpeed);
    REQUIRE(speed.name != nullptr);
    REQUIRE(speed.description != nullptr);
    REQUIRE(speed.coefficient != nullptr); // MoveSpeed dung he so nhan - PHAI co con tro

    const UpgradeTypeDescriptor& life = GetUpgradeTypeDescriptor(UpgradeType::ExtraLife);
    REQUIRE(life.name != nullptr);
    REQUIRE(life.coefficient == nullptr); // ExtraLife luon +1 mang/lan, khong dung he so rieng (xem upgrade_types.h)

    const UpgradeTypeDescriptor& score = GetUpgradeTypeDescriptor(UpgradeType::BonusScore);
    REQUIRE(score.name != nullptr);
    REQUIRE(score.coefficient != nullptr); // BonusScore dung he so la SO DIEM cong thang
}

TEST_CASE("GetUpgradeTypeDescriptor: coefficient tro THANG Config, doi gia tri goc thi doc lai phai thay ngay", "[upgrade][descriptor][balance]") {
    float original = Config::UPGRADE_MOVE_SPEED_MUL;
    const UpgradeTypeDescriptor& desc = GetUpgradeTypeDescriptor(UpgradeType::MoveSpeed);

    Config::UPGRADE_MOVE_SPEED_MUL = 2.5f;
    REQUIRE(*desc.coefficient == Approx(2.5f)); // Doc lai QUA CUNG 1 con tro, khong goi lai GetUpgradeTypeDescriptor()

    Config::UPGRADE_MOVE_SPEED_MUL = original; // Khoi phuc, tranh ro ri sang test khac chay cung binary
}

TEST_CASE("GetUpgradeTypeDescriptor: index ngoai pham vi fallback ve index 0, khong crash", "[upgrade][descriptor]") {
    const UpgradeTypeDescriptor& fallback = GetUpgradeTypeDescriptor((UpgradeType)99);
    const UpgradeTypeDescriptor& first = GetUpgradeTypeDescriptor((UpgradeType)0);
    REQUIRE(fallback.name == first.name);
}

TEST_CASE("Player::ApplyRunUpgrade(MoveSpeed): mot lan chon nhan speed len dung Config::UPGRADE_MOVE_SPEED_MUL", "[upgrade][player]") {
    Player p;
    float speedBefore = p.GetSpeed();
    p.ApplyRunUpgrade(UpgradeType::MoveSpeed);
    REQUIRE(p.GetSpeed() == Approx(speedBefore * Config::UPGRADE_MOVE_SPEED_MUL));
    REQUIRE(p.GetUpgradeStacks(UpgradeType::MoveSpeed) == 1);
}

TEST_CASE("Player::ApplyRunUpgrade(MoveSpeed): 3 lan cong don kieu NHAN LUY THUA (x^3)", "[upgrade][player]") {
    Player p;
    float speedBefore = p.GetSpeed();
    p.ApplyRunUpgrade(UpgradeType::MoveSpeed);
    p.ApplyRunUpgrade(UpgradeType::MoveSpeed);
    p.ApplyRunUpgrade(UpgradeType::MoveSpeed);

    float expected = speedBefore * Config::UPGRADE_MOVE_SPEED_MUL * Config::UPGRADE_MOVE_SPEED_MUL * Config::UPGRADE_MOVE_SPEED_MUL;
    REQUIRE(p.GetSpeed() == Approx(expected));
    REQUIRE(p.GetUpgradeStacks(UpgradeType::MoveSpeed) == 3);
}

TEST_CASE("Player::ApplyRunUpgrade(ExtraLife): +1 mang moi lan chon", "[upgrade][player]") {
    Player p;
    int livesBefore = p.GetLives();
    p.ApplyRunUpgrade(UpgradeType::ExtraLife);
    REQUIRE(p.GetLives() == livesBefore + 1);
    REQUIRE(p.GetUpgradeStacks(UpgradeType::ExtraLife) == 1);
}

TEST_CASE("Player::ApplyRunUpgrade(ExtraLife): cap o Config::MAX_LIVES du chon lap lai nhieu lan, khong tran so", "[upgrade][player]") {
    Player p;
    for (int i = 0; i < Config::MAX_LIVES + 5; i++) p.ApplyRunUpgrade(UpgradeType::ExtraLife);
    REQUIRE(p.GetLives() == Config::MAX_LIVES);
    // Stack DEM SO LAN DA CHON that (de UI hien dung so lan nguoi choi bam) - khong phai
    // so mang thuc te tang len, 2 con so nay co the LECH nhau khi cham tran, dung y muon.
    REQUIRE(p.GetUpgradeStacks(UpgradeType::ExtraLife) == Config::MAX_LIVES + 5);
}

TEST_CASE("Player::ApplyRunUpgrade(BonusScore): cong diem qua AddScore(), cong don tuyen tinh", "[upgrade][player]") {
    Player p;
    int scoreBefore = p.GetScore();
    p.ApplyRunUpgrade(UpgradeType::BonusScore);
    p.ApplyRunUpgrade(UpgradeType::BonusScore);
    p.ApplyRunUpgrade(UpgradeType::BonusScore);

    REQUIRE(p.GetScore() == scoreBefore + 3 * (int)Config::UPGRADE_BONUS_SCORE);
    REQUIRE(p.GetUpgradeStacks(UpgradeType::BonusScore) == 3);
}

TEST_CASE("Player::ApplyRunUpgrade: 3 loai cong don DOC LAP nhau, khong dung cheo stack", "[upgrade][player]") {
    Player p;
    p.ApplyRunUpgrade(UpgradeType::MoveSpeed);
    p.ApplyRunUpgrade(UpgradeType::MoveSpeed);

    REQUIRE(p.GetUpgradeStacks(UpgradeType::MoveSpeed) == 2);
    REQUIRE(p.GetUpgradeStacks(UpgradeType::ExtraLife) == 0);
    REQUIRE(p.GetUpgradeStacks(UpgradeType::BonusScore) == 0);
}

TEST_CASE("Player::Reset(): xoa sach ca 3 stack nang cap (van MOI) - khong ro ri sang van tiep theo", "[upgrade][player]") {
    Player p;
    p.ApplyRunUpgrade(UpgradeType::MoveSpeed);
    p.ApplyRunUpgrade(UpgradeType::ExtraLife);
    p.ApplyRunUpgrade(UpgradeType::BonusScore);
    REQUIRE(p.GetUpgradeStacks(UpgradeType::MoveSpeed) > 0);

    p.Reset();

    REQUIRE(p.GetUpgradeStacks(UpgradeType::MoveSpeed) == 0);
    REQUIRE(p.GetUpgradeStacks(UpgradeType::ExtraLife) == 0);
    REQUIRE(p.GetUpgradeStacks(UpgradeType::BonusScore) == 0);
    REQUIRE(p.GetSpeed() == Approx(Config::PLAYER_SPEED)); // speed cung ve mac dinh (dong Reset() co san tu truoc, khong phai dong moi cua Phase 3)
}

TEST_CASE("Player::ResetForNewWave(): KHONG xoa stack nang cap - nang cap song xuyen suot nhieu wave", "[upgrade][player]") {
    Player p;
    p.ApplyRunUpgrade(UpgradeType::MoveSpeed);
    float speedAfterUpgrade = p.GetSpeed();

    p.ResetForNewWave();

    REQUIRE(p.GetUpgradeStacks(UpgradeType::MoveSpeed) == 1); // KHONG bi xoa
    REQUIRE(p.GetSpeed() == Approx(speedAfterUpgrade)); // speed van giu nguyen hieu ung nang cap
}
