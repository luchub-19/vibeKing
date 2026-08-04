#include "thirdparty/catch.hpp"
#include "meta_progress.h"
#include "player.h"
#include "config.h"
#include <fstream>
#include <sstream>
#include <cstdio>

// ==========================================
// META PROGRESS - dung y het bo khung test cua test_leaderboard.cpp (TestPath rieng +
// CleanupGuard RAII, khong dung chung file that cua game) vi MetaProgress copy dung khuon
// Leaderboard (checksum FNV-1a + ghi file atomic). Phan cuoi file kiem tra them
// Player::ApplyStartBonus() - diem tich hop giua loadout va Player.
// ==========================================
namespace {
    const char* TestPath() { return "test_meta_progress_tmp.dat"; }

    struct CleanupGuard {
        ~CleanupGuard() { std::remove(TestPath()); }
    };
}

TEST_CASE("MetaProgress: file khong ton tai -> 0 currency, chua mo khoa gi, khong crash", "[meta_progress]") {
    CleanupGuard guard;
    std::remove(TestPath());

    MetaProgress mp;
    mp.Load(TestPath());
    REQUIRE(mp.GetCurrency() == 0);
    REQUIRE_FALSE(mp.IsUnlocked(LoadoutType::Vanguard));
    REQUIRE_FALSE(mp.IsUnlocked(LoadoutType::Overcharge));
    REQUIRE(mp.IsUnlocked(LoadoutType::Standard)); // Mien phi san, luon "mo khoa"
}

TEST_CASE("AwardCurrency: quy doi dung cong thuc score/RATE va cong don qua nhieu van", "[meta_progress]") {
    CleanupGuard guard;
    std::remove(TestPath());

    MetaProgress mp;
    mp.Load(TestPath());
    mp.AwardCurrency(1000); // 1000 / 50 = 20
    REQUIRE(mp.GetCurrency() == 20);
    mp.AwardCurrency(500); // + 500/50 = 10 -> cong don
    REQUIRE(mp.GetCurrency() == 30);
}

TEST_CASE("AwardCurrency: diem duoi 1 x RATE thi khong cong currency nao (chia lay phan nguyen)", "[meta_progress]") {
    CleanupGuard guard;
    std::remove(TestPath());

    MetaProgress mp;
    mp.Load(TestPath());
    mp.AwardCurrency(Config::META_SCORE_TO_CURRENCY_RATE - 1);
    REQUIRE(mp.GetCurrency() == 0);
}

TEST_CASE("TryUnlock: du currency thi tru tien, bat co unlocked, tra ve true", "[meta_progress]") {
    CleanupGuard guard;
    std::remove(TestPath());

    MetaProgress mp;
    mp.Load(TestPath());
    mp.AwardCurrency(150 * Config::META_SCORE_TO_CURRENCY_RATE); // Vua du 150 currency

    REQUIRE(mp.TryUnlock(LoadoutType::Vanguard, 150));
    REQUIRE(mp.IsUnlocked(LoadoutType::Vanguard));
    REQUIRE(mp.GetCurrency() == 0); // Da tru het
}

TEST_CASE("TryUnlock: khong du currency thi tra ve false, khong tru tien, khong mo khoa", "[meta_progress]") {
    CleanupGuard guard;
    std::remove(TestPath());

    MetaProgress mp;
    mp.Load(TestPath());
    mp.AwardCurrency(100 * Config::META_SCORE_TO_CURRENCY_RATE); // Chi co 100, can 150

    REQUIRE_FALSE(mp.TryUnlock(LoadoutType::Vanguard, 150));
    REQUIRE_FALSE(mp.IsUnlocked(LoadoutType::Vanguard));
    REQUIRE(mp.GetCurrency() == 100); // Khong doi
}

TEST_CASE("TryUnlock: da mo khoa roi thi lan goi sau tra ve false, khong tru tien lan 2", "[meta_progress]") {
    CleanupGuard guard;
    std::remove(TestPath());

    MetaProgress mp;
    mp.Load(TestPath());
    mp.AwardCurrency(400 * Config::META_SCORE_TO_CURRENCY_RATE);
    REQUIRE(mp.TryUnlock(LoadoutType::Overcharge, 400));
    REQUIRE(mp.GetCurrency() == 0);

    mp.AwardCurrency(400 * Config::META_SCORE_TO_CURRENCY_RATE); // Co them 400 nua
    REQUIRE_FALSE(mp.TryUnlock(LoadoutType::Overcharge, 400)); // Da mo khoa - khong tru tien lan 2
    REQUIRE(mp.GetCurrency() == 400); // Van con nguyen 400 vua nhan
}

TEST_CASE("TryUnlock: goi voi Standard luon tra ve false (mien phi san, khong can mo khoa)", "[meta_progress]") {
    CleanupGuard guard;
    std::remove(TestPath());

    MetaProgress mp;
    mp.Load(TestPath());
    mp.AwardCurrency(10000);
    int before = mp.GetCurrency();

    REQUIRE_FALSE(mp.TryUnlock(LoadoutType::Standard, 0));
    REQUIRE(mp.GetCurrency() == before); // Khong tru gi ca
}

TEST_CASE("Save roi Load lai tu file cho ra dung du lieu (round-trip)", "[meta_progress][checksum]") {
    CleanupGuard guard;
    std::remove(TestPath());

    {
        MetaProgress mp;
        mp.Load(TestPath());
        mp.AwardCurrency(1000); // +20
        mp.TryUnlock(LoadoutType::Vanguard, 20); // Tru het 20, mo khoa Vanguard
    }

    MetaProgress mp2;
    mp2.Load(TestPath());
    REQUIRE(mp2.GetCurrency() == 0);
    REQUIRE(mp2.IsUnlocked(LoadoutType::Vanguard));
    REQUIRE_FALSE(mp2.IsUnlocked(LoadoutType::Overcharge));
}

TEST_CASE("MetaProgress: BAO MAT - file bi sua tay (checksum khong khop) bi TU CHOI nap", "[meta_progress][checksum]") {
    CleanupGuard guard;
    std::remove(TestPath());

    {
        MetaProgress mp;
        mp.Load(TestPath());
        mp.AwardCurrency(5000);
    }

    // Giu nguyen dong checksum, sua thang so currency ben duoi - mo phong gian lan sua
    // file .dat bang text editor (dung y het bai test tuong ung cua Leaderboard).
    {
        std::ifstream in(TestPath());
        std::string sigLine;
        std::getline(in, sigLine);
        in.close();

        std::ofstream out(TestPath(), std::ios::trunc);
        out << sigLine << "\n";
        out << "999999 1 1\n"; // Currency + ca 2 co unlocked gia mao
    }

    MetaProgress mp2;
    mp2.Load(TestPath());
    REQUIRE(mp2.GetCurrency() == 0); // Phai bi tu choi toan bo, ve lai mac dinh
    REQUIRE_FALSE(mp2.IsUnlocked(LoadoutType::Vanguard));
    REQUIRE_FALSE(mp2.IsUnlocked(LoadoutType::Overcharge));
}

TEST_CASE("MetaProgress: BAO MAT - file dinh dang cu (khong co dong SIG) bi tu choi nap", "[meta_progress][checksum]") {
    CleanupGuard guard;
    std::remove(TestPath());

    {
        std::ofstream out(TestPath(), std::ios::trunc);
        out << "9999 1 1\n"; // Dinh dang truoc khi co checksum
    }

    MetaProgress mp;
    mp.Load(TestPath());
    REQUIRE(mp.GetCurrency() == 0);
}

// ==========================================
// PLAYER INTEGRATION - Player::ApplyStartBonus(LoadoutType), diem tich hop giua loadout
// va Player. Khong dung InitWindow/GPU (giong toan bo test_player.cpp).
// ==========================================
TEST_CASE("ApplyStartBonus(Vanguard): +1 mang so voi luc Reset()", "[meta_progress][player]") {
    Player p;
    int before = p.GetLives();
    p.ApplyStartBonus(LoadoutType::Vanguard);
    REQUIRE(p.GetLives() == before + 1);
}

TEST_CASE("ApplyStartBonus(Vanguard): khong vuot qua MAX_LIVES neu da o tran", "[meta_progress][player]") {
    Player p;
    p.AddScore(Config::EXTRA_LIFE_SCORE_THRESHOLD * (Config::MAX_LIVES + 5)); // Ep dat tran MAX_LIVES
    REQUIRE(p.GetLives() == Config::MAX_LIVES);

    p.ApplyStartBonus(LoadoutType::Vanguard);
    REQUIRE(p.GetLives() == Config::MAX_LIVES); // Khong vuot tran
}

TEST_CASE("ApplyStartBonus(Overcharge): kich hoat RapidFire ngay", "[meta_progress][player]") {
    Player p;
    REQUIRE_FALSE(p.HasRapidFire());
    p.ApplyStartBonus(LoadoutType::Overcharge);
    REQUIRE(p.HasRapidFire());
}

TEST_CASE("ApplyStartBonus(Standard): khong doi gi ca", "[meta_progress][player]") {
    Player p;
    int livesBefore = p.GetLives();
    p.ApplyStartBonus(LoadoutType::Standard);
    REQUIRE(p.GetLives() == livesBefore);
    REQUIRE_FALSE(p.HasRapidFire());
    REQUIRE_FALSE(p.HasShield());
    REQUIRE_FALSE(p.HasPiercing());
}
