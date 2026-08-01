#include "thirdparty/catch.hpp"
#include "leaderboard.h"
#include <fstream>
#include <sstream>
#include <cstdio>

namespace {
    // Duong dan file tam RIENG cho bo test - khong dung chung voi save that cua game.
    const char* TestPath() { return "test_leaderboard_tmp.dat"; }

    struct CleanupGuard {
        ~CleanupGuard() { std::remove(TestPath()); }
    };
}

TEST_CASE("Leaderboard: file khong ton tai -> danh sach rong, khong crash", "[leaderboard]") {
    CleanupGuard guard;
    std::remove(TestPath());

    Leaderboard lb;
    lb.Load(TestPath());
    REQUIRE(lb.GetEntries().empty());
    REQUIRE(lb.GetTopScore() == 0);
}

TEST_CASE("Leaderboard: TrySubmit sap xep giam dan theo diem va bao dung ket qua", "[leaderboard]") {
    CleanupGuard guard;
    std::remove(TestPath());

    Leaderboard lb;
    lb.Load(TestPath());

    REQUIRE(lb.TrySubmit(100, 1) == SubmitResult::NewRecord); // Danh sach rong -> luon la ky luc moi
    REQUIRE(lb.TrySubmit(500, 3) == SubmitResult::NewRecord); // Cao hon #1 hien tai
    REQUIRE(lb.TrySubmit(300, 2) == SubmitResult::MadeTop10); // Lot top nhung khong phai #1

    const auto& entries = lb.GetEntries();
    REQUIRE(entries.size() == 3);
    REQUIRE(entries[0].score == 500); // Sap xep giam dan
    REQUIRE(entries[1].score == 300);
    REQUIRE(entries[2].score == 100);
    REQUIRE(lb.GetTopScore() == 500);
}

TEST_CASE("Leaderboard: qua LEADERBOARD_MAX_ENTRIES thi diem thap nhat bi loai, diem qua thap bi tu choi", "[leaderboard]") {
    CleanupGuard guard;
    std::remove(TestPath());

    Leaderboard lb;
    lb.Load(TestPath());
    for (int i = 0; i < Config::LEADERBOARD_MAX_ENTRIES; i++) {
        lb.TrySubmit((i + 1) * 100, 1); // 100, 200, ..., 1000 (danh sach vua day)
    }
    REQUIRE((int)lb.GetEntries().size() == Config::LEADERBOARD_MAX_ENTRIES);

    // Diem thap hon MOI entry hien co -> khong du dieu kien lot top, danh sach khong doi.
    REQUIRE(lb.TrySubmit(50, 1) == SubmitResult::NotQualified);
    REQUIRE((int)lb.GetEntries().size() == Config::LEADERBOARD_MAX_ENTRIES);

    // Diem cao hon entry yeu nhat (100) -> phai lot top, day entry yeu nhat cu ra ngoai,
    // danh sach van giu dung LEADERBOARD_MAX_ENTRIES phan tu (khong phinh to).
    REQUIRE(lb.TrySubmit(150, 1) == SubmitResult::MadeTop10);
    REQUIRE((int)lb.GetEntries().size() == Config::LEADERBOARD_MAX_ENTRIES);
    bool stillHas100 = false;
    for (const auto& e : lb.GetEntries()) if (e.score == 100) stillHas100 = true;
    REQUIRE_FALSE(stillHas100); // Diem yeu nhat cu (100) da bi day ra ngoai top
}

TEST_CASE("Leaderboard: Save roi Load lai tu file cho ra dung du lieu (round-trip)", "[leaderboard][checksum]") {
    CleanupGuard guard;
    std::remove(TestPath());

    {
        Leaderboard lb;
        lb.Load(TestPath());
        lb.TrySubmit(777, 5);
        lb.TrySubmit(333, 2);
    }

    Leaderboard lb2;
    lb2.Load(TestPath());
    REQUIRE(lb2.GetEntries().size() == 2);
    REQUIRE(lb2.GetTopScore() == 777);
}

TEST_CASE("Leaderboard: BAO MAT - file bi sua tay (checksum khong khop) bi TU CHOI nap", "[leaderboard][checksum]") {
    CleanupGuard guard;
    std::remove(TestPath());

    {
        Leaderboard lb;
        lb.Load(TestPath());
        lb.TrySubmit(500, 3);
    }

    // Mo file vua ghi, GIU nguyen dong checksum nhung sua thang con so diem ben duoi -
    // mo phong dung kieu gian lan "sua file .dat bang text editor".
    {
        std::ifstream in(TestPath());
        std::string sigLine;
        std::getline(in, sigLine);
        in.close();

        std::ofstream out(TestPath(), std::ios::trunc);
        out << sigLine << "\n";
        out << "999999 3\n"; // Diem gia mao - checksum cu khong con khop voi noi dung nay
    }

    Leaderboard lb2;
    lb2.Load(TestPath());
    REQUIRE(lb2.GetEntries().empty()); // Phai bi tu choi toan bo, khong nap "nua tin nua ngo"
}

TEST_CASE("Leaderboard: BAO MAT - file dinh dang cu (khong co dong SIG) bi tu choi nap", "[leaderboard][checksum]") {
    CleanupGuard guard;
    std::remove(TestPath());

    {
        std::ofstream out(TestPath(), std::ios::trunc);
        out << "500 3\n300 2\n"; // Dinh dang truoc khi co checksum
    }

    Leaderboard lb;
    lb.Load(TestPath());
    REQUIRE(lb.GetEntries().empty());
}
