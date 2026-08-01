#include "thirdparty/catch.hpp"
#include "bullet_pool.h"

// ==========================================
// CCD (Continuous Collision Detection) - Bullet::GetSweptRect()
// Muc tieu: chung minh swept rect chan duoc hien tuong "xuyen tao" (tunneling) - dan
// bay du nhanh trong 1 frame de nhay HOAN TOAN qua 1 muc tieu mong ma khong bao gio
// chong lan no o rect CUOI CUNG cua frame, nhung swept rect (bao trum ca doan duong da
// di) van bat duoc va cham.
// ==========================================
TEST_CASE("Bullet swept rect bat duoc muc tieu bi 'nhay qua' o toc do cao (khong bi xuyen tao)", "[ccd][bullet]") {
    Bullet bullet;
    // Dan bay LEN (Y am), toc do rat cao: 1 frame di chuyen 100px (2000 px/s * 0.05s).
    bullet.Spawn(100.0f, 500.0f, { 0.0f, -2000.0f }, /*seq=*/1);
    bullet.Update(0.05f);

    Rectangle finalRect = bullet.GetRect();
    Rectangle sweptRect = bullet.GetSweptRect();

    // Muc tieu mong nam GIUA vi tri dau (y=500) va vi tri cuoi (y=400) cua dan trong
    // frame nay - dan "luot qua" no ma khong dung lai o dau ca.
    Rectangle target{ 95.0f, 450.0f, 20.0f, 10.0f };

    SECTION("Rect CUOI FRAME (khong CCD) se BO LOT muc tieu - chung minh van de co that") {
        REQUIRE_FALSE(CheckCollisionRecs(finalRect, target));
    }

    SECTION("Swept rect (CO CCD) PHAI bat duoc muc tieu") {
        REQUIRE(CheckCollisionRecs(sweptRect, target));
    }
}

TEST_CASE("Bullet swept rect khong bao ve nham muc tieu qua xa duong bay", "[ccd][bullet]") {
    Bullet bullet;
    bullet.Spawn(100.0f, 500.0f, { 0.0f, -2000.0f }, /*seq=*/1);
    bullet.Update(0.05f);

    Rectangle sweptRect = bullet.GetSweptRect();
    Rectangle farAway{ 700.0f, 50.0f, 20.0f, 20.0f };
    REQUIRE_FALSE(CheckCollisionRecs(sweptRect, farAway));
}

TEST_CASE("Bullet swept rect o frame dau tien (chua di chuyen) trung voi rect thuong", "[ccd][bullet]") {
    Bullet bullet;
    bullet.Spawn(200.0f, 300.0f, { 0.0f, -600.0f }, /*seq=*/1);
    // Spawn() dat prevPos = vi tri spawn -> CHUA goi Update() nen swept rect phai bang
    // dung rect ban dau (khong "phinh to" gia tao truoc khi thuc su di chuyen frame nao).
    Rectangle r = bullet.GetRect();
    Rectangle swept = bullet.GetSweptRect();
    REQUIRE(r.x == Approx(swept.x));
    REQUIRE(r.y == Approx(swept.y));
    REQUIRE(r.width == Approx(swept.width));
    REQUIRE(r.height == Approx(swept.height));
}

TEST_CASE("Bullet::ConsumePierce dung so lan xuyen da cau hinh", "[bullet][pierce]") {
    Bullet bullet;
    bullet.Spawn(0.0f, 0.0f, { 0.0f, -100.0f }, /*seq=*/1, /*pierceHits=*/2);

    REQUIRE(bullet.ConsumePierce() == true);  // Con 2 -> 1, van xuyen tiep duoc
    REQUIRE(bullet.ConsumePierce() == true);  // Con 1 -> 0, van xuyen tiep duoc (lan cuoi)
    REQUIRE(bullet.ConsumePierce() == false); // Het pierce -> lan trung ke tiep phai huy dan
}

TEST_CASE("BulletPool Fire/Destroy dung swap-and-pop O(1)", "[bullet][pool]") {
    BulletPool<8> pool;
    pool.Fire(10.0f, 10.0f, { 0.0f, -1.0f });
    pool.Fire(20.0f, 20.0f, { 0.0f, -1.0f });
    pool.Fire(30.0f, 30.0f, { 0.0f, -1.0f });
    REQUIRE(pool.GetActiveCount() == 3);

    // Xoa vien dau tien (index 0) - phan tu CUOI (index 2, x=30) phai duoc hoan doi vao vi
    // tri 0 (dac trung swap-and-pop), khong dich chuyen toan bo mang.
    pool.Destroy(0);
    REQUIRE(pool.GetActiveCount() == 2);
    REQUIRE(pool.GetBullet(0).GetRect().x == Approx(30.0f));
}

TEST_CASE("BulletPool day thi GHI DE vien dan CU NHAT thay vi bo qua yeu cau ban moi", "[bullet][pool]") {
    BulletPool<2> pool;
    pool.Fire(1.0f, 1.0f, { 0.0f, -1.0f }); // seq 0 - se la "cu nhat"
    pool.Fire(2.0f, 2.0f, { 0.0f, -1.0f }); // seq 1
    REQUIRE(pool.GetActiveCount() == 2);

    pool.Fire(3.0f, 3.0f, { 0.0f, -1.0f }); // Pool day (capacity=2) -> ghi de vien seq 0
    REQUIRE(pool.GetActiveCount() == 2);    // Van dung luong toi da, khong tran bo nho tinh

    // Vien tai x=1 (seq cu nhat) phai da bi thay the boi vien moi (x=3); vien x=2 (seq
    // moi hon) phai con nguyen ven.
    bool foundNew = false, foundKeptOld = false, foundOverwrittenOld = false;
    for (size_t i = 0; i < pool.GetActiveCount(); i++) {
        float x = pool.GetBullet(i).GetRect().x;
        if (x == Approx(3.0f)) foundNew = true;
        if (x == Approx(2.0f)) foundKeptOld = true;
        if (x == Approx(1.0f)) foundOverwrittenOld = true;
    }
    REQUIRE(foundNew);
    REQUIRE(foundKeptOld);
    REQUIRE_FALSE(foundOverwrittenOld);
}
