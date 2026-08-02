#include "thirdparty/catch.hpp"
#include "bunker.h"
#include "config.h"

// ==========================================
// BUNKER - truoc ban sua nay chua co test nao. Bo test nay tap trung vao
// IsFullyDestroyed()/solidRemaining (vua duoc doi tu quet O(COLS*ROWS) moi lan goi
// sang 1 bien dem cap nhat O(1) tai DamageVoxel()/Update()) - dung 1 vong quet luoi
// min qua toan bo GetBounds() de dam bao trung moi voxel it nhat 1 lan, khong phu
// thuoc vao hang so VOXEL_SIZE noi bo (private, test khong duoc biet).
// ==========================================

namespace {
    void DestroyEntireBunker(Bunker& bunker) {
        Rectangle b = bunker.GetBounds();
        for (float y = b.y; y < b.y + b.height; y += 1.0f) {
            for (float x = b.x; x < b.x + b.width; x += 1.0f) {
                bunker.HandleBulletHit({ x, y, 2.0f, 2.0f });
            }
        }
    }
}

TEST_CASE("Bunker vua tao: con nguyen ven, chua bi pha huy", "[bunker]") {
    Bunker bunker(0.0f, 0.0f, GREEN);
    REQUIRE_FALSE(bunker.IsFullyDestroyed());
}

TEST_CASE("Bunker: 1 phat dan trung vung dac (giua canh tren, tranh goc bo tron) lam mat 1 vai voxel nhung chua pha sach", "[bunker]") {
    Bunker bunker(0.0f, 0.0f, GREEN);
    Rectangle b = bunker.GetBounds();
    float midX = b.x + b.width / 2.0f;
    float topY = b.y + 1.0f; // Hang tren cung nhung o GIUA - xa ca 2 goc da bi bo tron luc khoi tao

    REQUIRE(bunker.HandleBulletHit({ midX, topY, 2.0f, 2.0f }));
    REQUIRE_FALSE(bunker.IsFullyDestroyed()); // Chi mat vai o trong tong so hang tram o
}

TEST_CASE("Bunker: quet het toan bo vung -> IsFullyDestroyed() thanh true, va HandleBulletHit tiep theo tra ve false ngay (early-out)", "[bunker]") {
    Bunker bunker(0.0f, 0.0f, GREEN);
    DestroyEntireBunker(bunker);
    REQUIRE(bunker.IsFullyDestroyed());

    // Sau khi da pha sach, goi HandleBulletHit o BAT KY dau cung phai tra ve false
    // NGAY (early-out solidRemaining<=0 o dau ham) - khong con gi de chan dan nua.
    Rectangle b = bunker.GetBounds();
    REQUIRE_FALSE(bunker.HandleBulletHit({ b.x + 10.0f, b.y + 10.0f, 2.0f, 2.0f }));
}

TEST_CASE("Bunker: sau khi pha sach, regen it nhat 1 tick lam IsFullyDestroyed() tro lai false", "[bunker]") {
    Bunker bunker(0.0f, 0.0f, GREEN);
    DestroyEntireBunker(bunker);
    REQUIRE(bunker.IsFullyDestroyed());

    bunker.Update(Config::BUNKER_REGEN_INTERVAL); // Du 1 chu ky de kich hoat regen (hoi phuc toi da BUNKER_REGEN_PER_TICK voxel)
    REQUIRE_FALSE(bunker.IsFullyDestroyed());
}

TEST_CASE("Bunker: dan bay khong cham bounds thi khong tinh la va cham", "[bunker]") {
    Bunker bunker(0.0f, 0.0f, GREEN);
    Rectangle b = bunker.GetBounds();
    // Ban o cach xa han, ngoai han GetBounds() - khong the trung.
    REQUIRE_FALSE(bunker.HandleBulletHit({ b.x + b.width + 500.0f, b.y + b.height + 500.0f, 2.0f, 2.0f }));
    REQUIRE_FALSE(bunker.IsFullyDestroyed());
}
