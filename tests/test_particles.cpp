#include "thirdparty/catch.hpp"
#include "particle_pool.h"

// ==========================================
// PARTICLE - vu no nhieu lop (GD 2, docs/GRAPHICS_UPGRADE_PLAN.md). Hinh dang/mau la viec
// cua anh chup; o day khoa phan dem duoc: moi co no sinh dung so lop, va preset Low chi cat
// manh vo (lop phu), KHONG cat loi flash/vong song (phan hoi chinh "co no o dau").
// Headless an toan: GetRandomValue chi anh huong huong/toc do, khong anh huong so luong.
// ==========================================

TEST_CASE("Explosion: None khong sinh gi, Small = 1 loi + 1 vong + 5 manh, Large = 1 + 2 + 14", "[particles][vfx]") {
    ParticlePool<64> p;
    p.Explosion({ 100.0f, 100.0f }, WHITE, ExplosionSize::None);
    REQUIRE(p.GetActiveCount() == 0);

    p.Explosion({ 100.0f, 100.0f }, WHITE, ExplosionSize::Small);
    REQUIRE(p.GetActiveCount() == 7);

    p.Reset();
    p.Explosion({ 100.0f, 100.0f }, WHITE, ExplosionSize::Large);
    REQUIRE(p.GetActiveCount() == 17);
}

TEST_CASE("Explosion: preset Low chi giam manh vo, loi + vong song giu nguyen", "[particles][vfx][graphics]") {
    ParticlePool<64> p;
    p.SetSpawnScale(0.5f);
    p.Explosion({ 100.0f, 100.0f }, WHITE, ExplosionSize::Small);
    REQUIRE(p.GetActiveCount() == 2 + 3); // ScaledCount(5, 0.5) = 3
    p.Reset();
    p.Explosion({ 100.0f, 100.0f }, WHITE, ExplosionSize::Large);
    REQUIRE(p.GetActiveCount() == 3 + 7);
}

TEST_CASE("Explosion: loi flash tat truoc vong song, vong song tat truoc manh vo (thu tu tan cua vu no)", "[particles][vfx]") {
    // Nguon ZH: "thoi gian tan > thoi gian bung" - neu lop nao song lau hon lop sau no thi vu no
    // ket thuc bang 1 cuc flash tro troi thay vi tan dan.
    ParticlePool<64> p;
    p.Explosion({ 100.0f, 100.0f }, WHITE, ExplosionSize::Small);
    p.Update(0.11f); // > 0.1s loi flash
    REQUIRE(p.GetActiveCount() == 6);
    p.Update(0.2f);  // tong 0.31 > 0.3s vong song
    REQUIRE(p.GetActiveCount() == 5);
    p.Update(0.5f);  // tong 0.81 > 0.8s manh vo lau nhat
    REQUIRE(p.GetActiveCount() == 0);
}
