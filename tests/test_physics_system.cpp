#include "thirdparty/catch.hpp"
#include "game_manager_test_access.h"
#include <cstdio>

// ==========================================
// TEST_PHYSICS_SYSTEM - PhysicsSystem::CheckCollisions() la ham rui ro nhat cho Track B4
// (boss refactor). Cac test o day khoa lai 4 hanh vi cu the duoc yeu cau: dich thuong 1
// phat chet, Tanky nhieu phat, dieu kien roi power-up, va boss chuyen giai doan dung %HP -
// cong 1 test tich hop chay ca CheckCollisions() + ProcessEvents() cho duong power-up (xem
// cuoi file).
//
// PHAM VI: CheckCollisions() con xu ly them va cham dan-dich-vs-player, bunker, va Mystery
// Ship (UFO) - KHONG nam trong pham vi yeu cau ban dau nen khong co test rieng o day; neu
// can, do la 1 bo sung ro rang, khong am tham gop chung vao file nay.
//
// Dung chung tinh than voi tests/test_bullet_ccd.cpp (logic thuan, khong can InitWindow) va
// tests/test_game_manager.cpp (cung seam GameManagerTestAccess) - xem 2 file do de biet
// quy uoc chung.
// ==========================================

using GTA = GameManagerTestAccess;

namespace {
    // Ban 1 vien dan CHONG KHOP HOAN TOAN len goc tren-trai cua 1 rect muc tieu - swept rect
    // frame dau tien luon bang rect thuong (chua Update() nao chay, xem Bullet::Spawn()), nen
    // dam bao trung bat ke kich thuoc that cua dan/muc tieu, khong phu thuoc offset tinh tay.
    template <size_t N>
    void FireBulletAt(BulletPool<N>& pool, const Rectangle& targetRect, int pierceHits = 0) {
        pool.Reset();
        pool.Fire(targetRect.x, targetRect.y, { 0.0f, 0.0f }, pierceHits);
    }
}

// ==========================================
// C3.1 - DICH THUONG (Basic): 1 phat chet
// ==========================================
TEST_CASE("CheckCollisions: dan player ha guc Basic enemy sau dung 1 phat, dan bi tieu thu, event mang dung SCORE_VALUE + dropPowerUp", "[physics][collision][basic]") {
    GameManager gm;
    BasicEnemy e{};
    e.rect = { 100.0f, 100.0f, 32.0f, 24.0f };
    e.color = WHITE;
    GTA::BasicEnemies(gm).Clear();
    GTA::BasicEnemies(gm).Spawn(e);

    FireBulletAt(GTA::PlayerBullets(gm), e.rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::BasicEnemies(gm).Size() == 0);
    REQUIRE(GTA::PlayerBullets(gm).GetActiveCount() == 0); // pierceHits=0 mac dinh -> bi Destroy() ngay khi trung

    const auto& events = GTA::PendingEvents(gm);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].scoreValue == BasicEnemy::SCORE_VALUE);
    REQUIRE(events[0].dropPowerUp == true);
    REQUIRE(events[0].sfx == SfxType::Explosion);
}

// ==========================================
// C3.2 - TANKY: nhieu phat (TankyEnemy::HP), cac phat truoc chi la "trung nhung chua chet"
// ==========================================
TEST_CASE("CheckCollisions: Tanky enemy can dung TankyEnemy::HP phat moi chet - moi phat truoc do KHONG cong diem/roi power-up, chi phat CUOI CUNG moi co", "[physics][collision][tanky]") {
    GameManager gm;
    TankyEnemy t{}; // hp mac dinh = TankyEnemy::HP luc construct
    t.rect = { 200.0f, 150.0f, 32.0f, 32.0f };
    t.color = WHITE;
    GTA::TankyEnemies(gm).Clear();
    GTA::TankyEnemies(gm).Spawn(t);

    const int totalHp = TankyEnemy::HP;
    // Doc gia tri cau hinh THAT SU thay vi hardcode "3" - test van dung y ngay ca khi
    // balance.json/Config::LoadBalance() sau nay chinh lai HP cua Tanky.
    REQUIRE(totalHp >= 2); // kich ban "nhieu phat" chi co y nghia neu HP > 1

    for (int hitNum = 1; hitNum < totalHp; hitNum++) {
        FireBulletAt(GTA::PlayerBullets(gm), GTA::TankyEnemies(gm)[0].rect);
        PhysicsSystem::CheckCollisions(gm);

        INFO("hitNum=" << hitNum << " / totalHp=" << totalHp);
        REQUIRE(GTA::TankyEnemies(gm).Size() == 1); // van con song
        REQUIRE(GTA::TankyEnemies(gm)[0].hp == totalHp - hitNum);
        REQUIRE(GTA::PlayerBullets(gm).GetActiveCount() == 0); // dan van bi tieu thu (khong pierce)

        const auto& events = GTA::PendingEvents(gm);
        REQUIRE(events.size() == 1);
        REQUIRE(events[0].scoreValue == 0);       // CHUA chet -> chua cong diem
        REQUIRE(events[0].dropPowerUp == false);  // CHUA chet -> chua roi power-up
        REQUIRE(events[0].sfx == SfxType::Hit);
    }

    // Phat thu totalHp - ha guc that su
    FireBulletAt(GTA::PlayerBullets(gm), GTA::TankyEnemies(gm)[0].rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::TankyEnemies(gm).Size() == 0);
    const auto& finalEvents = GTA::PendingEvents(gm);
    REQUIRE(finalEvents.size() == 1);
    REQUIRE(finalEvents[0].scoreValue == TankyEnemy::SCORE_VALUE);
    REQUIRE(finalEvents[0].dropPowerUp == true);
    REQUIRE(finalEvents[0].sfx == SfxType::Explosion);
}

// ==========================================
// C3.3 - BOSS: BossStage() phan loai dung 3 giai doan theo % HP con lai
// ==========================================
TEST_CASE("BossStage(): phan loai dung 3 giai doan theo % HP con lai, dung tai ca 2 nguong chuyen tiep (66% va 33%)", "[physics][boss]") {
    Boss b{};
    b.maxHp = 100;

    b.hp = 100; REQUIRE(BossStage(b) == 1);
    b.hp = 67;  REQUIRE(BossStage(b) == 1); // ratio=0.67 > 0.66
    b.hp = 66;  REQUIRE(BossStage(b) == 2); // ratio=0.66 KHONG > 0.66 -> vua qua nguong
    b.hp = 34;  REQUIRE(BossStage(b) == 2); // ratio=0.34 > 0.33
    b.hp = 33;  REQUIRE(BossStage(b) == 3); // ratio=0.33 KHONG > 0.33 -> vua qua nguong
    b.hp = 1;   REQUIRE(BossStage(b) == 3);
    b.hp = 0;   REQUIRE(BossStage(b) == 3);
}

// ==========================================
// C3.3b - BOSS: CheckCollisions tru dung 1 HP moi phat khi khong bi shield chan
// ==========================================
TEST_CASE("CheckCollisions: boss khong shield (Vanguard) mat dung 1 HP moi phat dan, dan bi tieu thu, con song neu chua ve 0", "[physics][collision][boss]") {
    GameManager gm;
    Boss b{};
    b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
    b.maxHp = 40;
    b.hp = 40;
    b.type = BossType::Vanguard; // khong co co che shield
    GTA::BossPool(gm).Clear();
    GTA::BossPool(gm).Spawn(b);

    FireBulletAt(GTA::PlayerBullets(gm), b.rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::BossPool(gm).Size() == 1); // con song (40 -> 39)
    REQUIRE(GTA::BossPool(gm)[0].hp == 39);
    REQUIRE(GTA::PlayerBullets(gm).GetActiveCount() == 0);
}

TEST_CASE("CheckCollisions: boss HP ve 0 sau don cuoi van con trong bossPool (UpdatePlaying(), khong phai CheckCollisions(), moi Destroy() va bao WAVE_CLEAR)", "[physics][collision][boss]") {
    // Doi chieu voi physics_system.cpp: CheckCollisions() chi tru hp ("...UpdatePlaying() se
    // phat hien va xu ly WAVE_CLEAR - xem duoi"); Boss chi thuc su bi bossPool.Destroy(0) va
    // RequestTransition(WAVE_CLEAR) trong GameManager::UpdatePlaying() (xem
    // tests/test_game_manager.cpp cho phan do). Test nay xac nhan ranh gioi trach nhiem dung
    // nhu vay - tranh gia dinh nham "CheckCollisions tu xoa boss khi het mau".
    GameManager gm;
    Boss b{};
    b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
    b.maxHp = 40;
    b.hp = 1; // don ke tiep se ve 0
    b.type = BossType::Vanguard;
    GTA::BossPool(gm).Clear();
    GTA::BossPool(gm).Spawn(b);

    FireBulletAt(GTA::PlayerBullets(gm), b.rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::BossPool(gm).Size() == 1); // CheckCollisions() KHONG tu Destroy()
    REQUIRE(GTA::BossPool(gm)[0].hp == 0);
}

// ==========================================
// C3.3c - BOSS: khien Sentinel chan HOAN TOAN sat thuong
// ==========================================
TEST_CASE("CheckCollisions: khien Sentinel dang active chan HOAN TOAN sat thuong - HP khong doi, dan van bi tieu thu du con pierce, khong phat sfx Hit", "[physics][collision][boss]") {
    GameManager gm;
    Boss b{};
    b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
    b.maxHp = 40;
    b.hp = 40;
    b.type = BossType::Sentinel;
    b.shieldActive = true;
    GTA::BossPool(gm).Clear();
    GTA::BossPool(gm).Spawn(b);

    // pierceHits=5: neu khien KHONG chan hoan toan (chi la "1 hit binh thuong"), dan phai
    // CON XUYEN TIEP (khong bi Destroy) - o day ta xac nhan dieu nguoc lai xay ra.
    FireBulletAt(GTA::PlayerBullets(gm), b.rect, /*pierceHits=*/5);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::BossPool(gm)[0].hp == 40); // KHONG doi - khien hap thu het, khong tru mau
    REQUIRE(GTA::PlayerBullets(gm).GetActiveCount() == 0); // van bi tieu thu HOAN TOAN, bat ke con pierce

    const auto& events = GTA::PendingEvents(gm);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].sfx == SfxType::None); // KHONG phat "Hit" - chi gan sfx=Hit khi (!shielded && hp>0)
}

// ==========================================
// C3.4 - POWER-UP: dieu kien roi (tich hop CheckCollisions + ProcessEvents)
// Config::POWERUP_DROP_CHANCE la `inline` (khong constexpr), ghi de tam thoi duoc trong luc
// test roi phuc hoi lai - dung chinh quy uoc da co san o tests/test_boss.cpp cho cac hang so
// can bang khac (BOSS_SENTINEL_SWAY_AMPLITUDE...), khong phai 1 ky thuat rieng cua file nay.
// ==========================================
TEST_CASE("CheckCollisions + ProcessEvents: POWERUP_DROP_CHANCE=1.0 -> ha 1 dich CHAC CHAN roi power-up", "[physics][integration][powerup]") {
    float origChance = Config::POWERUP_DROP_CHANCE;
    Config::POWERUP_DROP_CHANCE = 1.0f;

    GameManager gm;
    BasicEnemy e{};
    e.rect = { 120.0f, 90.0f, 32.0f, 24.0f };
    e.color = WHITE;
    GTA::BasicEnemies(gm).Clear();
    GTA::BasicEnemies(gm).Spawn(e);

    FireBulletAt(GTA::PlayerBullets(gm), e.rect);
    PhysicsSystem::CheckCollisions(gm); // chi ghi nhan dropPowerUp=true vao pendingEvents
    REQUIRE(GTA::PowerUps(gm).Size() == 0); // chua roi that - MaybeDropPowerUp() chua chay

    GTA::CallProcessEvents(gm); // chay MaybeDropPowerUp() that su, dung roll ngau nhien
    REQUIRE(GTA::PowerUps(gm).Size() == 1);

    Config::POWERUP_DROP_CHANCE = origChance;
}

TEST_CASE("CheckCollisions + ProcessEvents: POWERUP_DROP_CHANCE=0.0 -> khong bao gio roi power-up", "[physics][integration][powerup]") {
    float origChance = Config::POWERUP_DROP_CHANCE;
    Config::POWERUP_DROP_CHANCE = 0.0f;

    GameManager gm;
    BasicEnemy e{};
    e.rect = { 120.0f, 90.0f, 32.0f, 24.0f };
    e.color = WHITE;
    GTA::BasicEnemies(gm).Clear();
    GTA::BasicEnemies(gm).Spawn(e);

    FireBulletAt(GTA::PlayerBullets(gm), e.rect);
    PhysicsSystem::CheckCollisions(gm);
    GTA::CallProcessEvents(gm);
    REQUIRE(GTA::PowerUps(gm).Size() == 0);

    Config::POWERUP_DROP_CHANCE = origChance;
}

// ==========================================
// UPDATEBOSS QUA BOSSTYPEDESCRIPTOR (Track B4) - tests/test_boss.cpp da xac nhan BANG
// g_bossTypeDescriptors[] gan dung con tro/co cho tung BossType va tu cap nhat theo
// LoadBalance() - nhung CHUA test PhysicsSystem::UpdateBoss() co THUC SU dispatch dung
// qua bang do luc chay khong (sway/khien/trieu hoi co xay ra dung nhip khong). 3 test duoi
// day moi la phan do - goi thang UpdateBoss() (ham public static, khong can qua GTA).
// ==========================================
TEST_CASE("UpdateBoss: Sentinel bat/tat khien dung theo BOSS_SENTINEL_SHIELD_INTERVAL/DURATION qua desc.hasShieldMechanic", "[physics][boss][dda_descriptor]") {
    GameManager gm;
    Boss b{};
    b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
    b.hp = 40; b.maxHp = 40;
    b.type = BossType::Sentinel;
    b.baseX = 350.0f;
    b.phaseTimer = Config::BOSS_SENTINEL_SHIELD_INTERVAL; // dung nhu GameManager::SpawnBoss() khoi tao that
    b.shieldActive = false;
    GTA::BossPool(gm).Clear();
    GTA::BossPool(gm).Spawn(b);

    PhysicsSystem::UpdateBoss(gm, Config::BOSS_SENTINEL_SHIELD_INTERVAL);
    REQUIRE(GTA::BossPool(gm)[0].shieldActive == true);
    REQUIRE(GTA::BossPool(gm)[0].phaseTimer == Approx(Config::BOSS_SENTINEL_SHIELD_DURATION));

    PhysicsSystem::UpdateBoss(gm, Config::BOSS_SENTINEL_SHIELD_DURATION);
    REQUIRE(GTA::BossPool(gm)[0].shieldActive == false);
    REQUIRE(GTA::BossPool(gm)[0].phaseTimer == Approx(Config::BOSS_SENTINEL_SHIELD_INTERVAL));
}

TEST_CASE("UpdateBoss: Swarmer trieu hoi dung BOSS_SWARMER_SUMMON_COUNT Kamikaze khi summonTimer het qua desc.hasSummonMechanic", "[physics][boss][dda_descriptor]") {
    GameManager gm;
    Boss b{};
    b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
    b.hp = 40; b.maxHp = 40;
    b.type = BossType::Swarmer;
    b.baseX = 350.0f;
    b.summonTimer = Config::BOSS_SWARMER_SUMMON_INTERVAL; // dung nhu SpawnBoss() khoi tao that
    GTA::BossPool(gm).Clear();
    GTA::BossPool(gm).Spawn(b);
    GTA::KamikazeEnemies(gm).Clear();

    PhysicsSystem::UpdateBoss(gm, Config::BOSS_SWARMER_SUMMON_INTERVAL);

    // Doi hinh (basic/tanky/zigzag) dang trong trong test nay -> SpawnKamikaze() tu roi
    // vao nhanh "spawn tu ngoai man hinh" (xem game_manager.cpp) thay vi "muon" 1 con dang
    // co trong doi hinh - dung y that cua boss wave (InitLevel() de doi hinh trong luc co Boss).
    REQUIRE((int)GTA::KamikazeEnemies(gm).Size() == Config::BOSS_SWARMER_SUMMON_COUNT);
    REQUIRE(GTA::BossPool(gm)[0].summonTimer == Approx(Config::BOSS_SWARMER_SUMMON_INTERVAL));
}

TEST_CASE("UpdateBoss: di chuyen Sway (Sentinel/Swarmer) dao dong QUANH baseX theo cong thuc sin, khong bao gio ra khoi [baseX-amplitude, baseX+amplitude]", "[physics][boss][dda_descriptor]") {
    GameManager gm;
    Boss b{};
    b.rect = { 310.0f, 80.0f, 100.0f, 60.0f }; // width=100 -> baseX 400 con nhieu khong
                                                 // gian truoc khi cham bien man hinh 800px,
                                                 // tranh fminf/fmaxf clamp bien xen vao phep
                                                 // do bien do sway dang test.
    b.rect.x = 400.0f;
    b.hp = 40; b.maxHp = 40;
    b.type = BossType::Sentinel; // amplitude 90px (xem Config::BOSS_SENTINEL_SWAY_AMPLITUDE)
    b.baseX = 400.0f;
    b.phaseAccum = 0.0f;
    GTA::BossPool(gm).Clear();
    GTA::BossPool(gm).Spawn(b);

    for (int i = 0; i < 20; i++) {
        PhysicsSystem::UpdateBoss(gm, 0.1f);
        float x = GTA::BossPool(gm)[0].rect.x;
        REQUIRE(x >= 400.0f - Config::BOSS_SENTINEL_SWAY_AMPLITUDE - 0.01f);
        REQUIRE(x <= 400.0f + Config::BOSS_SENTINEL_SWAY_AMPLITUDE + 0.01f);
    }
    REQUIRE(GTA::BossPool(gm)[0].phaseAccum > 0.0f); // tich luy that, khong dung yen
}
