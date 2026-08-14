#include "thirdparty/catch.hpp"
#include "player.h"
#include "config.h"
#include <cmath> // std::fabs - kiem tra goc ban Spread Shot (Phase 1b, Nguoi 1)

// ==========================================
// PLAYER - truoc ban sua nay chua co test nao. Tap trung vao AddScore() (logic +1
// mang theo moc diem vua them) va TakeDamage() (Shield/invincible) - ca 2 deu la logic
// THUAN, khong dung InitWindow/GPU nen chay duoc trong unit_tests binh thuong. Phase 1b
// (Nguoi 1) them 1 test cho Update() (goc ban Spread Shot) - InputState/BulletPool
// KHONG can GPU (chi Draw() moi can Texture2D that, van KHONG test o day).
// ==========================================

TEST_CASE("Player::Reset() dat dung trang thai ban dau", "[player]") {
    Player p;
    REQUIRE(p.GetLives() == 3);
    REQUIRE(p.GetScore() == 0);
}

TEST_CASE("AddScore: chua toi nguong thi khong len mang, tra ve false", "[player]") {
    Player p;
    bool granted = p.AddScore(Config::EXTRA_LIFE_SCORE_THRESHOLD - 1);
    REQUIRE_FALSE(granted);
    REQUIRE(p.GetLives() == 3);
    REQUIRE(p.GetScore() == Config::EXTRA_LIFE_SCORE_THRESHOLD - 1);
}

TEST_CASE("AddScore: vuot dung 1 nguong thi +1 mang, tra ve true", "[player]") {
    Player p;
    bool granted = p.AddScore(Config::EXTRA_LIFE_SCORE_THRESHOLD);
    REQUIRE(granted);
    REQUIRE(p.GetLives() == 4);
}

TEST_CASE("AddScore: 1 lan cong diem nhay qua NHIEU nguong cung luc (vd combo/boss diem cao) cong dung so mang", "[player]") {
    Player p;
    int startingLives = p.GetLives();
    // 3 lan EXTRA_LIFE_SCORE_THRESHOLD trong DUY NHAT 1 lan goi AddScore - phai cong
    // dung 3 mang (toi da MAX_LIVES) chu khong phai chi +1 (kiem tra vong while, khong
    // phai if, trong AddScore()).
    bool granted = p.AddScore(Config::EXTRA_LIFE_SCORE_THRESHOLD * 3);
    REQUIRE(granted);
    int expectedLives = startingLives + 3;
    if (expectedLives > Config::MAX_LIVES) expectedLives = Config::MAX_LIVES;
    REQUIRE(p.GetLives() == expectedLives);
}

TEST_CASE("AddScore: mang khong bao gio vuot qua MAX_LIVES du diem tiep tuc tang", "[player]") {
    Player p;
    // Cong diem gap nhieu lan MAX_LIVES * threshold - qua du de vuot tran nhieu lan.
    p.AddScore(Config::EXTRA_LIFE_SCORE_THRESHOLD * (Config::MAX_LIVES + 5));
    REQUIRE(p.GetLives() == Config::MAX_LIVES);
}

TEST_CASE("AddScore: sau khi dat MAX_LIVES, mat 1 mang KHONG duoc tu dong hoan lai (phai kiem diem moi that su)", "[player]") {
    Player p;
    p.AddScore(Config::EXTRA_LIFE_SCORE_THRESHOLD * (Config::MAX_LIVES + 5)); // Ep dat tran
    REQUIRE(p.GetLives() == Config::MAX_LIVES);

    p.TakeDamage(); // Mat 1 mang that (khong Shield/invincible luc nay)
    int livesAfterHit = p.GetLives();
    REQUIRE(livesAfterHit == Config::MAX_LIVES - 1);

    // Cong 1 diem nho (chua toi nguong tiep theo, vi nguong da duoc day xa vuot qua
    // score hien tai boi cac lan +MAX_LIVES*threshold o tren) - KHONG duoc tra lai
    // mang ngay lap tuc chi vi truoc do da "no" mang o muc tran.
    bool granted = p.AddScore(1);
    REQUIRE_FALSE(granted);
    REQUIRE(p.GetLives() == livesAfterHit);
}

TEST_CASE("TakeDamage: co Shield thi khong mat mang, Shield bi tieu hao ngay", "[player]") {
    Player p;
    p.GrantShield(5.0f);
    REQUIRE(p.HasShield());

    bool tookDamage = p.TakeDamage();
    REQUIRE_FALSE(tookDamage); // Shield do don, khong tinh la mat mang
    REQUIRE(p.GetLives() == 3); // Khong doi
    REQUIRE_FALSE(p.HasShield()); // Da tieu hao ngay sau khi do 1 don
}

TEST_CASE("TakeDamage: dang bat tu (vua trung don truoc do) thi khong mat mang them", "[player]") {
    Player p;
    p.TakeDamage(); // Lan 1: mat mang that, kich hoat invincibleTimer
    REQUIRE(p.GetLives() == 2);

    bool tookDamageAgain = p.TakeDamage(); // Lan 2 ngay sau: dang bat tu
    REQUIRE_FALSE(tookDamageAgain);
    REQUIRE(p.GetLives() == 2); // Khong mat them
}

// Phase 1b (Enemy & Item Revolution, Nguoi 1)
TEST_CASE("TakeDamage: co Overdrive dang active thi mat 2 mang thay vi 1", "[player]") {
    Player p;
    p.GrantOverdrive(5.0f);
    REQUIRE(p.HasOverdrive());

    bool tookDamage = p.TakeDamage();
    REQUIRE(tookDamage);
    REQUIRE(p.GetLives() == 1); // 3 - 2 = 1, khong phai 3 - 1 = 2 nhu binh thuong
}

TEST_CASE("TakeDamage: Shield van chan damage HOAN TOAN du Overdrive dang active (Shield uu tien truoc)", "[player]") {
    Player p;
    p.GrantShield(5.0f);
    p.GrantOverdrive(5.0f); // Ca 2 cung active - Shield van phai thang, khong mat mang nao
    bool tookDamage = p.TakeDamage();
    REQUIRE_FALSE(tookDamage);
    REQUIRE(p.GetLives() == 3); // Khong doi - Overdrive chi anh huong nhanh THAT SU mat mang, khong "vuot mat" Shield
}

TEST_CASE("Update: co Spread Shot active thi ban 3 dan cung luc thay vi 1, dung goc doi xung", "[player]") {
    Player p;
    p.GrantSpreadShot(5.0f);
    REQUIRE(p.HasSpreadShot());

    BulletPool<Config::MAX_PLAYER_BULLETS> bullets;
    InputState input{};
    input.Action_Shoot = true;

    // dt=0: fireTimer da duoc Reset() dat dung bang PLAYER_FIRE_RATE (cho phep 1 phat
    // ngay tu dau, xem comment "Chan spam dan dau game" trong player.cpp) nen khong can
    // dt>0 de du dieu kien ban - giu test don gian, chi kiem tra dung SO LUONG/HUONG dan.
    bool fired = p.Update(0.0f, input, bullets);
    REQUIRE(fired);
    REQUIRE(bullets.GetActiveCount() == 3); // 1 tia giua + 2 tia lech, thay vi 1 vien nhu binh thuong

    // Ca 3 tia phai CUNG do lon toc do (BULLET_SPEED) - chi khac HUONG. Dung 1 tia
    // thang len (vel.x ~ 0) va 2 tia lech trai/phai DOI XUNG (tong vel.x ~ 0).
    int centerCount = 0;
    float sumVelX = 0.0f;
    for (size_t i = 0; i < bullets.GetActiveCount(); i++) {
        Vector2 v = bullets.GetBullet(i).GetVel();
        float speedSq = v.x * v.x + v.y * v.y;
        REQUIRE(speedSq == Approx(Config::BULLET_SPEED * Config::BULLET_SPEED).epsilon(0.001));
        REQUIRE(v.y < 0.0f); // Ca 3 deu bay LEN (Y am), khong tia nao bay nguoc xuong
        sumVelX += v.x;
        if (std::fabs(v.x) < 0.01f) centerCount++;
    }
    REQUIRE(centerCount == 1);
    REQUIRE(sumVelX == Approx(0.0f).margin(0.01f));
}
