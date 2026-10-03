#include "thirdparty/catch.hpp"
#include "launch_options.h"

// ==========================================
// LAUNCH OPTIONS (GD 0 ke hoach do hoa) - parse tham so dong lenh cho che do chup anh/do
// hieu nang + cach tinh p95. Ca 2 la ham thuan, khong can raylib.
// ==========================================

TEST_CASE("ParseLaunchOptions: khong tham so = choi binh thuong", "[launch]") {
    LaunchOptions o = ParseLaunchOptions({});
    REQUIRE(o.Ok());
    REQUIRE(o.scene == ShowcaseScene::None);
    REQUIRE(o.captureFile.empty());
    REQUIRE(o.benchFrames == 0);
}

TEST_CASE("ParseLaunchOptions: doc scene/capture/capture-frame", "[launch]") {
    LaunchOptions o = ParseLaunchOptions({ "--scene=boss", "--capture=out/a.png", "--capture-frame=90" });
    REQUIRE(o.Ok());
    REQUIRE(o.scene == ShowcaseScene::Boss);
    REQUIRE(o.captureFile == "out/a.png"); // Giu nguyen thu muc - ly do khong dung TakeScreenshot()
    REQUIRE(o.captureFrame == 90);

    REQUIRE(ParseLaunchOptions({ "--scene=combat" }).scene == ShowcaseScene::Combat);
}

TEST_CASE("ParseLaunchOptions: tu choi gia tri sai thay vi lang le bo qua", "[launch]") {
    REQUIRE_FALSE(ParseLaunchOptions({ "--scene=menu" }).Ok());
    REQUIRE_FALSE(ParseLaunchOptions({ "--bench=0" }).Ok());
    REQUIRE_FALSE(ParseLaunchOptions({ "--bench=-5" }).Ok());
    REQUIRE_FALSE(ParseLaunchOptions({ "--bench=12abc" }).Ok()); // atoi() se doc thanh 12
    REQUIRE_FALSE(ParseLaunchOptions({ "--capture=" }).Ok());
    REQUIRE_FALSE(ParseLaunchOptions({ "--fullscreen" }).Ok());
    REQUIRE_FALSE(ParseLaunchOptions({ "--capture=a.png", "--bench=100" }).Ok());
}

TEST_CASE("SummarizeFrameTimes: rong tra ve 0, khong chia cho 0", "[launch]") {
    FrameStats s = SummarizeFrameTimes({});
    REQUIRE(s.count == 0);
    REQUIRE(s.avgMs == Approx(0.0));
}

TEST_CASE("SummarizeFrameTimes: avg/max/p95 nearest-rank, khong phu thuoc thu tu dau vao", "[launch]") {
    // 1..20 xao tron. p95 nearest-rank = phan tu thu ceil(19) = 19, KHONG phai max 20 - neu
    // cai dat lay nham index n*0.95 (0-based) thi ra 20 va test nay do.
    std::vector<double> v{ 20, 3, 1, 19, 2, 18, 4, 17, 5, 16, 6, 15, 7, 14, 8, 13, 9, 12, 10, 11 };
    FrameStats s = SummarizeFrameTimes(v);
    REQUIRE(s.count == 20);
    REQUIRE(s.avgMs == Approx(10.5));
    REQUIRE(s.maxMs == Approx(20.0));
    REQUIRE(s.p95Ms == Approx(19.0));
}

TEST_CASE("ParseLaunchOptions: --quality ghi de preset, mac dinh khong ghi de", "[launch]") {
    REQUIRE(ParseLaunchOptions({}).qualityOverride == -1);
    REQUIRE(ParseLaunchOptions({ "--quality=low" }).qualityOverride == 0);
    REQUIRE(ParseLaunchOptions({ "--quality=high" }).qualityOverride == 2);
    REQUIRE_FALSE(ParseLaunchOptions({ "--quality=ultra" }).Ok());
}
