#include "thirdparty/catch.hpp"
#include "game_manager_test_access.h"
#include "achievements.h"
#include "save_checksum.h"
#include <cstdio>
#include <fstream>
#include <sstream>

// ==========================================
// THANH TUU - 2 nua:
//   1. AchievementProgress thuan (dieu kien, idempotent, file save) - khung TestPath() +
//      CleanupGuard cua test_leaderboard.cpp.
//   2. Tich hop GameManager qua GameManagerTestAccess: kill dem dung, UNTOUCHABLE xet theo
//      WAVE, va luat "CR thuong chi vao vi luc ket thuc van" (khong pha test khoa "WAVE_CLEAR
//      khong cong currency" trong test_game_manager.cpp).
// ==========================================
namespace {
    const char* TestPath() { return "test_achievements_tmp.dat"; }
    const char* MetaTestPath() { return "test_achievements_meta_tmp.dat"; }
    const char* LeaderboardTestPath() { return "test_achievements_leaderboard_tmp.dat"; }

    struct CleanupGuard {
        ~CleanupGuard() {
            std::remove(TestPath());
            std::remove(MetaTestPath());
            std::remove(LeaderboardTestPath());
        }
    };

    // Giong QuarantinePersistence() cua test_game_manager.cpp - KHONG Load() achievements:
    // de nguyen filePath rong cung la cach kiem tra Save() no-op (khong rot file ra cwd).
    void Quarantine(GameManager& gm) {
        GameManagerTestAccess::MetaProgressRef(gm).Load(MetaTestPath());
        GameManagerTestAccess::LeaderboardRef(gm).Load(LeaderboardTestPath());
    }

    bool PathOpens(const char* path) { return std::ifstream(path).is_open(); }

    GameEvent KillEvent(int score) {
        GameEvent ev;
        ev.position = { 400.0f, 300.0f };
        ev.scoreValue = score;
        return ev;
    }
}

using GTA = GameManagerTestAccess;

// ---------- 1. AchievementProgress thuan ----------

TEST_CASE("Achievements: file khong ton tai -> chua mo gi, 0 kill, khong crash", "[achievements]") {
    CleanupGuard guard;
    std::remove(TestPath());
    AchievementProgress a;
    a.Load(TestPath());
    REQUIRE(a.UnlockedCount() == 0);
    REQUIRE(a.GetLifetimeKills() == 0);
}

TEST_CASE("Achievements: moi dieu kien DUOI nguong 1 don vi thi chua dat, DUNG nguong thi dat", "[achievements]") {
    // Do tu descriptor (khong hardcode so) - doi nguong trong bang thi test van dung y.
    auto t = [](AchievementId id) { return GetAchievementDescriptor(id).threshold; };

    AchievementSnapshot s;
    s.lifetimeKills = t(AchievementId::Exterminator) - 1;
    REQUIRE_FALSE(IsAchievementMet(AchievementId::Exterminator, s));
    s.lifetimeKills++;
    REQUIRE(IsAchievementMet(AchievementId::Exterminator, s));

    s = {};
    s.runBestCombo = t(AchievementId::ChainReaction) - 1;
    REQUIRE_FALSE(IsAchievementMet(AchievementId::ChainReaction, s));
    s.runBestCombo++;
    REQUIRE(IsAchievementMet(AchievementId::ChainReaction, s));

    s = {};
    s.waveReached = t(AchievementId::Veteran) - 1;
    REQUIRE(IsAchievementMet(AchievementId::HoldingTheLine, s)); // Nguong thap hon da qua
    REQUIRE_FALSE(IsAchievementMet(AchievementId::Veteran, s));
    s.waveReached++;
    REQUIRE(IsAchievementMet(AchievementId::Veteran, s));

    s = {};
    s.score = t(AchievementId::HighRoller) - 1;
    REQUIRE_FALSE(IsAchievementMet(AchievementId::HighRoller, s));
    s.score++;
    REQUIRE(IsAchievementMet(AchievementId::HighRoller, s));

    s = {};
    REQUIRE_FALSE(IsAchievementMet(AchievementId::GiantSlayer, s));
    REQUIRE_FALSE(IsAchievementMet(AchievementId::Untouchable, s));
    s.bossDefeated = true;
    s.flawlessClear = true;
    REQUIRE(IsAchievementMet(AchievementId::GiantSlayer, s));
    REQUIRE(IsAchievementMet(AchievementId::Untouchable, s));
}

TEST_CASE("Achievements: Evaluate chi tra ve cai VUA mo - goi lai cung snapshot tra ve rong", "[achievements]") {
    AchievementProgress a; // Khong Load -> Save no-op, khong can file tam
    AchievementSnapshot s;
    s.lifetimeKills = 1;
    s.bossDefeated = true;

    std::vector<AchievementId> first = a.Evaluate(s);
    REQUIRE(first.size() == 2);
    REQUIRE(a.IsUnlocked(AchievementId::FirstContact));
    REQUIRE(a.IsUnlocked(AchievementId::GiantSlayer));

    REQUIRE(a.Evaluate(s).empty()); // Neu mo lai lan 2, thuong CR se bi cong 2 lan

    s.waveReached = 5; // Them 1 dieu kien moi -> chi DUNG cai do duoc tra ve
    std::vector<AchievementId> second = a.Evaluate(s);
    REQUIRE(second.size() == 1);
    REQUIRE(second[0] == AchievementId::HoldingTheLine);
}

TEST_CASE("Achievements: chua Load() thi Evaluate/Flush KHONG ghi file nao ra cwd", "[achievements]") {
    AchievementProgress a;
    AchievementSnapshot s;
    s.lifetimeKills = 1;
    REQUIRE(a.Evaluate(s).size() == 1); // Co thay doi -> Evaluate co goi Save()
    a.Flush();
    REQUIRE_FALSE(PathOpens(".tmp"));
    REQUIRE_FALSE(PathOpens(""));
}

TEST_CASE("Achievements: Save/Load giu nguyen kill tron doi + co mo khoa", "[achievements]") {
    CleanupGuard guard;
    std::remove(TestPath());
    {
        AchievementProgress a;
        a.Load(TestPath());
        a.AddKills(37);
        AchievementSnapshot s;
        s.lifetimeKills = a.GetLifetimeKills();
        s.flawlessClear = true;
        a.Evaluate(s); // FirstContact + Untouchable -> tu luu
        a.AddKills(3); // Sau lan luu cuoi - chi con nho nho Flush()
        a.Flush();
    }
    AchievementProgress b;
    b.Load(TestPath());
    REQUIRE(b.GetLifetimeKills() == 40);
    REQUIRE(b.IsUnlocked(AchievementId::FirstContact));
    REQUIRE(b.IsUnlocked(AchievementId::Untouchable));
    REQUIRE_FALSE(b.IsUnlocked(AchievementId::GiantSlayer));
    REQUIRE(b.UnlockedCount() == 2);
}

TEST_CASE("Achievements: file bi sua tay (checksum lech) -> tu choi nap toan bo", "[achievements]") {
    CleanupGuard guard;
    {
        AchievementProgress a;
        a.Load(TestPath());
        a.AddKills(5);
        a.Flush();
    }
    // Sua so kill 5 -> 9999 nhung giu nguyen dong SIG cu.
    std::ifstream in(TestPath());
    std::string sig, body;
    std::getline(in, sig);
    std::getline(in, body);
    in.close();
    REQUIRE(body.rfind("5 ", 0) == 0);
    {
        std::ofstream out(TestPath(), std::ios::trunc);
        out << sig << "\n" << "9999" << body.substr(1) << "\n";
    }

    AchievementProgress b;
    b.Load(TestPath());
    REQUIRE(b.GetLifetimeKills() == 0);
}

TEST_CASE("Achievements: file tu ban CU (it co hon so thanh tuu hien tai) van nap duoc, co thieu = chua mo", "[achievements]") {
    CleanupGuard guard;
    std::string body = "12 1 0 1\n"; // Chi 3 co dau
    {
        std::ofstream out(TestPath(), std::ios::trunc);
        out << "SIG " << SaveChecksum::ToHex(SaveChecksum::Fnv1a64(body)) << "\n" << body;
    }
    AchievementProgress a;
    a.Load(TestPath());
    REQUIRE(a.GetLifetimeKills() == 12);
    REQUIRE(a.IsUnlocked(AchievementId::FirstContact));
    REQUIRE_FALSE(a.IsUnlocked(AchievementId::ChainReaction));
    REQUIRE(a.IsUnlocked(AchievementId::HoldingTheLine));
    REQUIRE(a.UnlockedCount() == 2);
}

// ---------- 2. Tich hop GameManager ----------

TEST_CASE("GameManager: ha dich dem vao kill tron doi, mo FIRST CONTACT, xep toast - CR CHUA vao vi giua van", "[achievements][game_manager][currency]") {
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    GTA::CallInitLevel(gm, true);
    GTA::SetState(gm, GameState::PLAYING);

    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();
    GTA::PendingEvents(gm).push_back(KillEvent(10));
    GTA::CallUpdatePlaying(gm, 0.016f); // ProcessEvents() xu ly event, roi CheckAchievements()

    REQUIRE(GTA::AchievementsRef(gm).GetLifetimeKills() == 1);
    REQUIRE(GTA::AchievementsRef(gm).IsUnlocked(AchievementId::FirstContact));
    REQUIRE(GTA::ToastQueue(gm).size() == 1);
    REQUIRE(GTA::ToastQueue(gm).front() == AchievementId::FirstContact);
    int reward = GetAchievementDescriptor(AchievementId::FirstContact).rewardCurrency;
    REQUIRE(reward > 0); // Neu thuong = 0 thi 2 khang dinh ben duoi khong phan biet duoc gi
    REQUIRE(GTA::RunAchievementBonus(gm) == reward);
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore);
}

TEST_CASE("GameManager: GAME_OVER tra thuong thanh tuu cung luc voi CR quy doi tu diem, va bang tong ket dem ca 2", "[achievements][game_manager][currency][summary]") {
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    GTA::CallInitLevel(gm, true);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::PendingEvents(gm).push_back(KillEvent(10));
    GTA::CallUpdatePlaying(gm, 0.016f);
    REQUIRE(GTA::RunAchievementBonus(gm) > 0);

    int bonus = GTA::RunAchievementBonus(gm);
    int fromScore = GTA::PlayerRef(gm).GetScore() / Config::META_SCORE_TO_CURRENCY_RATE;
    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();
    GTA::CallTriggerGameOver(gm);

    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore + fromScore + bonus);
    REQUIRE(GTA::RunCurrencyEarned(gm) == fromScore + bonus);
    REQUIRE(GTA::RunAchievementBonus(gm) == 0);

    GTA::CallTriggerGameOver(gm); // Idempotent - khong tra thuong lan 2
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore + fromScore + bonus);
}

TEST_CASE("GameManager: bo van giua chung (InitLevel(true)) van tra thuong thanh tuu da mo", "[achievements][game_manager][currency]") {
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    GTA::CallInitLevel(gm, true);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::PendingEvents(gm).push_back(KillEvent(10));
    GTA::CallUpdatePlaying(gm, 0.016f);
    int bonus = GTA::RunAchievementBonus(gm);
    REQUIRE(bonus > 0);

    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();
    GTA::CallInitLevel(gm, true); // Nhu bam R giua van
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore + bonus);
    REQUIRE(GTA::RunAchievementBonus(gm) == 0);
}

namespace {
    // Dung san 1 wave dang choi (sau InitLevel(true) de ddaLastKnownLives khop mang that),
    // tuy chon mat 1 mang GIUA wave, roi don sach doi hinh -> duong WAVE_CLEAR cua
    // PhysicsSystem::UpdateEnemies().
    void PlayWaveThenClear(GameManager& gm, bool loseLife) {
        GTA::CallInitLevel(gm, true);
        GTA::SetState(gm, GameState::PLAYING);
        REQUIRE_FALSE(GTA::IsBossWave(gm));
        if (loseLife) REQUIRE(GTA::PlayerRef(gm).TakeDamage());
        GTA::CallUpdatePlaying(gm, 0.016f); // Doi hinh con nguyen -> chua clear, chi ghi nhan mang mat

        GTA::BasicEnemies(gm).Clear();
        GTA::TankyEnemies(gm).Clear();
        GTA::ZigzagEnemies(gm).Clear();
        GTA::WardenEnemies(gm).Clear();
        GTA::MedicEnemies(gm).Clear();
        PhysicsSystem::UpdateEnemies(gm, 0.016f);
        REQUIRE(GTA::PendingState(gm) == GameState::WAVE_CLEAR);
    }
}

TEST_CASE("GameManager: don sach wave KHONG mat mang -> UNTOUCHABLE", "[achievements][game_manager]") {
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    PlayWaveThenClear(gm, false);
    REQUIRE(GTA::WaveLivesLost(gm) == 0);
    REQUIRE(GTA::AchievementsRef(gm).IsUnlocked(AchievementId::Untouchable));
}

TEST_CASE("GameManager: don sach wave nhung DA mat 1 mang trong wave -> KHONG co UNTOUCHABLE", "[achievements][game_manager]") {
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    PlayWaveThenClear(gm, true);
    REQUIRE(GTA::WaveLivesLost(gm) == 1);
    REQUIRE_FALSE(GTA::AchievementsRef(gm).IsUnlocked(AchievementId::Untouchable));
}

TEST_CASE("GameManager: waveLivesLost reset o wave MOI (InitLevel(false)), khong mang theo wave truoc", "[achievements][game_manager]") {
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    PlayWaveThenClear(gm, true);
    REQUIRE(GTA::WaveLivesLost(gm) == 1);
    GTA::CallInitLevel(gm, false);
    REQUIRE(GTA::WaveLivesLost(gm) == 0);
}

TEST_CASE("GameManager: ha boss mo GIANT SLAYER, thuong don lai nhung currency KHONG doi tai WAVE_CLEAR", "[achievements][game_manager][currency][boss]") {
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    GTA::CallInitLevel(gm, true);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, true);
    GTA::SetWave(gm, 5);
    Boss b{};
    b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
    b.hp = 0;
    b.maxHp = 40;
    b.type = BossType::Vanguard;
    GTA::BossPool(gm).Clear();
    GTA::BossPool(gm).Spawn(b);

    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();
    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::PendingState(gm) == GameState::WAVE_CLEAR);
    REQUIRE(GTA::AchievementsRef(gm).IsUnlocked(AchievementId::GiantSlayer));
    REQUIRE(GTA::RunAchievementBonus(gm) >= GetAchievementDescriptor(AchievementId::GiantSlayer).rewardCurrency);
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore);
}

TEST_CASE("Toast: hien lan luot tung thanh tuu, moi cai dung ACHIEVEMENT_TOAST_DURATION giay", "[achievements][game_manager][toast]") {
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    GTA::CallInitLevel(gm, true);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetWave(gm, 5); // HOLDING THE LINE cung frame voi FIRST CONTACT -> 2 toast
    GTA::PendingEvents(gm).push_back(KillEvent(10));
    GTA::CallUpdatePlaying(gm, 0.016f);
    REQUIRE(GTA::ToastQueue(gm).size() == 2);
    AchievementId first = GTA::ToastQueue(gm).front();

    GTA::CallUpdateToasts(gm, Config::ACHIEVEMENT_TOAST_DURATION * 0.5f);
    REQUIRE(GTA::ToastQueue(gm).size() == 2); // Chua het gio - van la cai dau
    REQUIRE(GTA::ToastQueue(gm).front() == first);

    GTA::CallUpdateToasts(gm, Config::ACHIEVEMENT_TOAST_DURATION * 0.6f);
    REQUIRE(GTA::ToastQueue(gm).size() == 1);
    REQUIRE(GTA::ToastQueue(gm).front() != first);

    GTA::CallUpdateToasts(gm, Config::ACHIEVEMENT_TOAST_DURATION + 0.01f);
    REQUIRE(GTA::ToastQueue(gm).empty());
}

TEST_CASE("GameManager: mang mat NGAY truoc khi doi hinh sach (chua qua SyncLivesLost cua frame) van tinh -> KHONG co UNTOUCHABLE", "[achievements][game_manager]") {
    // Duong WAVE_CLEAR cua UpdateEnemies() chay TRUOC diem ghi nhan mang mat thuong le trong
    // UpdatePlaying(). Truoc khi OnWaveCleared() tu dong bo, kich ban nay mo UNTOUCHABLE du
    // nguoi choi vua mat mang (lo ra qua test GAME_OVER cua test_game_manager.cpp: Kamikaze
    // boc con dich cuoi cung khoi doi hinh -> wave "sach" dung frame player het mang).
    CleanupGuard guard;
    GameManager gm;
    Quarantine(gm);
    GTA::CallInitLevel(gm, true);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::BasicEnemies(gm).Clear();
    GTA::TankyEnemies(gm).Clear();
    GTA::ZigzagEnemies(gm).Clear();
    GTA::WardenEnemies(gm).Clear();
    GTA::MedicEnemies(gm).Clear();

    REQUIRE(GTA::PlayerRef(gm).TakeDamage());
    PhysicsSystem::UpdateEnemies(gm, 0.016f); // KHONG qua UpdatePlaying() -> chua ai dong bo mang
    REQUIRE(GTA::PendingState(gm) == GameState::WAVE_CLEAR);
    REQUIRE(GTA::WaveLivesLost(gm) == 1);
    REQUIRE_FALSE(GTA::AchievementsRef(gm).IsUnlocked(AchievementId::Untouchable));
}
