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
        // Xa hang doi bang tay TRUOC moi phat. Trong game that, GameManager::ProcessEvents()
        // lo viec nay moi frame (no la diem xa DUY NHAT - CheckCollisions() co y KHONG con
        // tu clear(), neu khong no se nuot cac event ma UpdateKamikaze() day vao truoc do;
        // xem comment dau CheckCollisions()). Test nay goi CheckCollisions() nhieu lan lien
        // tiep ma khong qua ProcessEvents(), nen phai tu xa de moi vong chi thay event cua
        // rieng phat dan vua ban.
        GTA::PendingEvents(gm).clear();
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
        // Nguoi 3 (Audio & UI) - hit-flash: truoc day nhanh "con song" nay khong set
        // position/flashOnHit gi ca (khong co phan hoi hinh anh nao). Gio phai bat
        // flashOnHit=true VA dat dung vi tri va cham (EnemyCenter that, khong con {0,0}
        // mac dinh) de ProcessEvents() bat flash dung cho.
        REQUIRE(events[0].flashOnHit == true);
        Vector2 expectedCenter = EnemyCenter(GTA::TankyEnemies(gm)[0].rect);
        REQUIRE(events[0].position.x == expectedCenter.x);
        REQUIRE(events[0].position.y == expectedCenter.y);
    }

    // Phat thu totalHp - ha guc that su
    GTA::PendingEvents(gm).clear();
    FireBulletAt(GTA::PlayerBullets(gm), GTA::TankyEnemies(gm)[0].rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::TankyEnemies(gm).Size() == 0);
    const auto& finalEvents = GTA::PendingEvents(gm);
    REQUIRE(finalEvents.size() == 1);
    REQUIRE(finalEvents[0].scoreValue == TankyEnemy::SCORE_VALUE);
    REQUIRE(finalEvents[0].dropPowerUp == true);
    REQUIRE(finalEvents[0].sfx == SfxType::Explosion);
    REQUIRE(finalEvents[0].flashOnHit == false); // Ha guc han dung nhanh khac - hit-flash KHONG ap dung o day
}

// ==========================================
// PHASE 1A (Enemy & Item Revolution, Nguoi 1) - WARDEN & MEDIC
// ==========================================
TEST_CASE("CheckCollisions: Warden can dung WardenEnemy::HP phat moi chet - chi phat CUOI CUNG moi gan wardenReinforcementCount vao event", "[physics][collision][warden]") {
    // Dung KHUON MAU voi test Tanky o tren (nhieu-phat-moi-chet) - CHI khac o cho luc chet
    // that su phai mang them wardenReinforcementCount (GameManager::ProcessEvents() doc
    // field nay de sinh quan tang vien - xem events.h).
    GameManager gm;
    WardenEnemy w{};
    w.rect = { 200.0f, 150.0f, 42.0f, 30.0f };
    w.color = DARKBLUE;
    GTA::WardenEnemies(gm).Clear();
    GTA::WardenEnemies(gm).Spawn(w);

    const int totalHp = WardenEnemy::HP;
    REQUIRE(totalHp >= 2); // kich ban "nhieu phat" chi co y nghia neu HP > 1

    for (int hitNum = 1; hitNum < totalHp; hitNum++) {
        GTA::PendingEvents(gm).clear(); // Xem giai thich o test Tanky ngay tren
        FireBulletAt(GTA::PlayerBullets(gm), GTA::WardenEnemies(gm)[0].rect);
        PhysicsSystem::CheckCollisions(gm);

        INFO("hitNum=" << hitNum << " / totalHp=" << totalHp);
        REQUIRE(GTA::WardenEnemies(gm).Size() == 1);
        REQUIRE(GTA::WardenEnemies(gm)[0].hp == totalHp - hitNum);

        const auto& events = GTA::PendingEvents(gm);
        REQUIRE(events.size() == 1);
        REQUIRE(events[0].scoreValue == 0);
        REQUIRE(events[0].wardenReinforcementCount == 0); // Chua chet han -> CHUA sinh quan tang vien
        REQUIRE(events[0].flashOnHit == true);
    }

    GTA::PendingEvents(gm).clear();
    FireBulletAt(GTA::PlayerBullets(gm), GTA::WardenEnemies(gm)[0].rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::WardenEnemies(gm).Size() == 0);
    const auto& finalEvents = GTA::PendingEvents(gm);
    REQUIRE(finalEvents.size() == 1);
    REQUIRE(finalEvents[0].scoreValue == WardenEnemy::SCORE_VALUE);
    REQUIRE(finalEvents[0].wardenReinforcementCount == Config::WARDEN_REINFORCEMENT_COUNT); // Chet han -> CO sinh quan
}

TEST_CASE("GameManager::ProcessEvents: Warden chet sinh dung Config::WARDEN_REINFORCEMENT_COUNT BasicEnemy tai vi tri cu, column=-1", "[physics][collision][warden][integration]") {
    // Tich hop CheckCollisions() + ProcessEvents() (dung tinh than test POWERUP_DROP_CHANCE
    // o tren) - xac nhan hieu ung THAT SU xuat hien trong basicEnemies, khong chi dung o
    // muc GameEvent.
    GameManager gm;
    WardenEnemy w{};
    w.rect = { 300.0f, 180.0f, 42.0f, 30.0f };
    w.color = DARKBLUE;
    w.hp = 1; // 1 phat la chet - khong can lap qua nhieu phat trong test nay
    GTA::WardenEnemies(gm).Clear();
    GTA::WardenEnemies(gm).Spawn(w);
    GTA::BasicEnemies(gm).Clear();

    FireBulletAt(GTA::PlayerBullets(gm), GTA::WardenEnemies(gm)[0].rect);
    PhysicsSystem::CheckCollisions(gm);
    GTA::CallProcessEvents(gm);

    REQUIRE(GTA::BasicEnemies(gm).Size() == (size_t)Config::WARDEN_REINFORCEMENT_COUNT);
    for (size_t i = 0; i < GTA::BasicEnemies(gm).Size(); i++) {
        REQUIRE(GTA::BasicEnemies(gm)[i].column == -1); // Khong thuoc cot nao trong luoi that - xem GameManager::ProcessEvents()
    }
}

TEST_CASE("CheckCollisions: Medic chet sau dung 1 phat (giong Basic), dan bi tieu thu, event mang dung SCORE_VALUE", "[physics][collision][medic]") {
    GameManager gm;
    MedicEnemy m{};
    m.rect = { 220.0f, 160.0f, 34.0f, 24.0f };
    m.color = LIME;
    GTA::MedicEnemies(gm).Clear();
    GTA::MedicEnemies(gm).Spawn(m);

    FireBulletAt(GTA::PlayerBullets(gm), GTA::MedicEnemies(gm)[0].rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::MedicEnemies(gm).Size() == 0);
    REQUIRE(GTA::PlayerBullets(gm).GetActiveCount() == 0);
    const auto& events = GTA::PendingEvents(gm);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].scoreValue == MedicEnemy::SCORE_VALUE);
    REQUIRE(events[0].dropPowerUp == true);
}

TEST_CASE("UpdateMedicEnemies: het Config::MEDIC_HEAL_INTERVAL, hoi dung Config::MEDIC_HEAL_AMOUNT cho TankyEnemy GAN NHAT con thieu mau, KHONG vuot TankyEnemy::HP", "[physics][medic]") {
    GameManager gm;
    GTA::MedicEnemies(gm).Clear();
    GTA::TankyEnemies(gm).Clear();

    MedicEnemy m{};
    m.rect = { 100.0f, 100.0f, 34.0f, 24.0f };
    m.color = LIME;
    m.healTimer = 0.0f;
    GTA::MedicEnemies(gm).Spawn(m);

    // 2 Tanky: 1 GAN Medic nhung DA DAY mau (khong duoc hoi vi khong can), 1 XA hon nhung
    // con thieu mau (phai la muc tieu duoc chon, du xa hon).
    TankyEnemy full{};
    full.rect = { 110.0f, 100.0f, 32.0f, 32.0f }; // Rat gan Medic
    full.hp = TankyEnemy::HP;
    GTA::TankyEnemies(gm).Spawn(full);

    TankyEnemy hurt{};
    hurt.rect = { 400.0f, 100.0f, 32.0f, 32.0f }; // Xa Medic hon nhieu
    hurt.hp = TankyEnemy::HP - 1;
    GTA::TankyEnemies(gm).Spawn(hurt);

    REQUIRE(Config::MEDIC_HEAL_INTERVAL > 0.0f);
    PhysicsSystem::UpdateMedicEnemies(gm, Config::MEDIC_HEAL_INTERVAL); // 1 buoc du het dung 1 chu ky

    REQUIRE(GTA::TankyEnemies(gm)[0].hp == TankyEnemy::HP);                 // "full" khong doi
    REQUIRE(GTA::TankyEnemies(gm)[1].hp == TankyEnemy::HP - 1 + Config::MEDIC_HEAL_AMOUNT); // "hurt" duoc hoi

    // Hoi lien tuc nhieu chu ky nua: khong bao gio vuot HP goc du con lai thieu it hon
    // MEDIC_HEAL_AMOUNT.
    for (int i = 0; i < 10; i++) {
        PhysicsSystem::UpdateMedicEnemies(gm, Config::MEDIC_HEAL_INTERVAL);
    }
    REQUIRE(GTA::TankyEnemies(gm)[1].hp == TankyEnemy::HP);
    REQUIRE(GTA::TankyEnemies(gm)[0].hp == TankyEnemy::HP);
}

// ==========================================
// HOI QUY: Warden/Medic phai tut hang CUNG NHIP voi Basic/Tanky/Zigzag.
//
// Test cu o cho nay khoa co che `alreadyFlipped` (chong doi huong 2 lan giua UpdateEnemies()
// va 2 ham Warden/Medic goi rieng tu UpdatePlaying()). Co che do da bi go bo cung voi nguyen
// nhan cua no: Warden/Medic gio nam TRONG UpdateEnemies(), khong con ham nao goi rieng nen
// khong con kha nang doi huong 2 lan de ma phai chong.
//
// Thay bang test bat DUNG cai bug ma co che cu khong he cham toi: Warden/Medic nam GIUA luoi
// khong bao gio tu cham bien man hinh, nen truoc day KHONG BAO GIO tut hang - doi hinh chinh
// hanh quan xuong bo lai chung lo lung phia tren (do that: wave 7, basic y 50->350 trong khi
// warden dung yen o 90).
// ==========================================
TEST_CASE("UpdateEnemies: Warden/Medic nam GIUA luoi tut hang DUNG BANG Basic khi doi hinh cham bien", "[physics][warden][medic][formation]") {
    GameManager gm;
    GTA::BasicEnemies(gm).Clear();
    GTA::WardenEnemies(gm).Clear();
    GTA::MedicEnemies(gm).Clear();
    GTA::SetEnemyDirection(gm, 1);
    GTA::SetEnemySpeed(gm, 50.0f);

    const float startY = 100.0f;

    // Basic sat canh PHAI - day la con duy nhat cham bien, tuc la con quyet dinh khi nao ca
    // doi hinh doi huong/tut hang.
    BasicEnemy b{};
    b.rect = { (float)Config::SCREEN_W - 5.0f, startY, 40.0f, 25.0f };
    b.column = 0;
    GTA::BasicEnemies(gm).Spawn(b);

    // Warden va Medic o GIUA man hinh - khong bao gio tu cham bien duoc.
    WardenEnemy w{};
    w.rect = { 380.0f, startY, 42.0f, 30.0f };
    w.column = 1;
    w.hp = WardenEnemy::HP;
    GTA::WardenEnemies(gm).Spawn(w);

    MedicEnemy m{};
    m.rect = { 300.0f, startY, 34.0f, 24.0f };
    m.column = 2;
    GTA::MedicEnemies(gm).Spawn(m);

    PhysicsSystem::UpdateEnemies(gm, 0.5f); // dt du lon de Basic cham bien ngay buoc dau

    REQUIRE(GTA::EnemyDirection(gm) == -1);  // doi hinh da doi huong
    const float basicY = GTA::BasicEnemies(gm)[0].rect.y;
    REQUIRE(basicY > startY);                // Basic da tut hang

    // Mau chot: ca 3 phai o CUNG mot hang. Truoc ban sua, warden/medic van dung nguyen startY.
    REQUIRE(GTA::WardenEnemies(gm)[0].rect.y == basicY);
    REQUIRE(GTA::MedicEnemies(gm)[0].rect.y == basicY);
}

TEST_CASE("UpdateEnemies: doi hinh chi doi huong DUNG 1 LAN moi frame du Warden cung cham bien cung luc", "[physics][warden][medic][formation]") {
    // Mat con lai cua cung van de: truoc day Warden/Medic tu doi huong trong ham rieng cua
    // chung, nen 1 Warden cham bien CUNG frame voi Basic co the lat huong lan 2 (= khong doi)
    // va tang toc gap doi. Gio chi co DUNG 1 cho doi huong trong toan bo doi hinh.
    GameManager gm;
    GTA::BasicEnemies(gm).Clear();
    GTA::WardenEnemies(gm).Clear();
    GTA::MedicEnemies(gm).Clear();
    GTA::SetEnemyDirection(gm, 1);
    GTA::SetEnemySpeed(gm, 50.0f);

    BasicEnemy b{};
    b.rect = { (float)Config::SCREEN_W - 5.0f, 100.0f, 40.0f, 25.0f };
    b.column = 0;
    GTA::BasicEnemies(gm).Spawn(b);

    WardenEnemy w{};                                    // CUNG sat bien phai nhu Basic
    w.rect = { (float)Config::SCREEN_W - 5.0f, 100.0f, 42.0f, 30.0f };
    w.column = 1;
    w.hp = WardenEnemy::HP;
    GTA::WardenEnemies(gm).Spawn(w);

    PhysicsSystem::UpdateEnemies(gm, 0.5f);

    REQUIRE(GTA::EnemyDirection(gm) == -1);             // doi DUNG 1 lan (khong phai 2 lan = quay ve 1)
    REQUIRE(GTA::EnemySpeed(gm) == 50.0f + Config::ENEMY_SPEED_INC); // tang DUNG 1 buoc
}

// ==========================================
// PHASE 2 (Enemy & Item Revolution, Nguoi 1): Weaver & Bomber - dung khuon Kamikaze/UFO
// (grid rieng hoan toan, khong dinh gi den doi hinh/WaveGenerator - xem enemy_types.h).
// ==========================================
TEST_CASE("CheckCollisions: Weaver chet sau dung 1 phat (giong Basic/Medic), dan bi tieu thu, event mang dung SCORE_VALUE", "[physics][collision][weaver]") {
    GameManager gm;
    WeaverEnemy w{};
    w.rect = { 220.0f, 160.0f, 32.0f, 22.0f };
    w.color = YELLOW;
    GTA::WeaverEnemies(gm).Clear();
    GTA::WeaverEnemies(gm).Spawn(w);

    FireBulletAt(GTA::PlayerBullets(gm), GTA::WeaverEnemies(gm)[0].rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::WeaverEnemies(gm).Size() == 0);
    REQUIRE(GTA::PlayerBullets(gm).GetActiveCount() == 0);
    const auto& events = GTA::PendingEvents(gm);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].scoreValue == WeaverEnemy::SCORE_VALUE);
}

TEST_CASE("CheckCollisions: Bomber chet sau dung 1 phat, dung grid RIENG - khong anh huong Weaver dung song cung frame", "[physics][collision][bomber]") {
    GameManager gm;
    BomberEnemy b{};
    b.rect = { 300.0f, 100.0f, 34.0f, 24.0f };
    b.color = ORANGE;
    GTA::BomberEnemies(gm).Clear();
    GTA::BomberEnemies(gm).Spawn(b);
    // Weaver o vi tri KHAC HAN, cung song trong frame nay - xac nhan dan chi trung dung
    // Bomber (grid rieng), khong vo tinh "lan" qua pool khac o gan do.
    WeaverEnemy w{};
    w.rect = { 500.0f, 400.0f, 32.0f, 22.0f };
    GTA::WeaverEnemies(gm).Clear();
    GTA::WeaverEnemies(gm).Spawn(w);

    FireBulletAt(GTA::PlayerBullets(gm), GTA::BomberEnemies(gm)[0].rect);
    PhysicsSystem::CheckCollisions(gm);

    REQUIRE(GTA::BomberEnemies(gm).Size() == 0);
    REQUIRE(GTA::WeaverEnemies(gm).Size() == 1); // Khong bi anh huong
    const auto& events = GTA::PendingEvents(gm);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].scoreValue == BomberEnemy::SCORE_VALUE);
}

TEST_CASE("UpdateWeaverEnemies/UpdateBomberEnemies: het spawn timer thi spawn 1 con MOI, timer quay lai dung khoang MIN/MAX_INTERVAL", "[physics][weaver][bomber]") {
    GameManager gm;
    GTA::WeaverEnemies(gm).Clear();
    GTA::BomberEnemies(gm).Clear();
    GTA::SetWeaverSpawnTimer(gm, 0.0f);
    GTA::SetBomberSpawnTimer(gm, 0.0f);

    PhysicsSystem::UpdateWeaverEnemies(gm, 0.016f);
    PhysicsSystem::UpdateBomberEnemies(gm, 0.016f);

    REQUIRE(GTA::WeaverEnemies(gm).Size() == 1);
    REQUIRE(GTA::BomberEnemies(gm).Size() == 1);
    REQUIRE(GTA::WeaverSpawnTimer(gm) >= Config::WEAVER_SPAWN_MIN_INTERVAL);
    REQUIRE(GTA::WeaverSpawnTimer(gm) <= Config::WEAVER_SPAWN_MAX_INTERVAL);
    REQUIRE(GTA::BomberSpawnTimer(gm) >= Config::BOMBER_SPAWN_MIN_INTERVAL);
    REQUIRE(GTA::BomberSpawnTimer(gm) <= Config::BOMBER_SPAWN_MAX_INTERVAL);
}

TEST_CASE("UpdateWeaverEnemies/UpdateBomberEnemies: pool DA DAY thi het timer cung KHONG spawn them, khong bao gio vuot capacity", "[physics][weaver][bomber]") {
    GameManager gm;
    GTA::WeaverEnemies(gm).Clear();
    GTA::BomberEnemies(gm).Clear();
    for (size_t i = 0; i < Config::MAX_WEAVER_ENEMIES; i++) {
        GTA::WeaverEnemies(gm).Spawn(WeaverEnemy{ {100.0f, 100.0f, 32.0f, 22.0f}, YELLOW, 1, 100.0f, 0.0f });
    }
    for (size_t i = 0; i < Config::MAX_BOMBER_ENEMIES; i++) {
        GTA::BomberEnemies(gm).Spawn(BomberEnemy{ {100.0f, 70.0f, 34.0f, 24.0f}, ORANGE, 1, 1.0f });
    }
    GTA::SetWeaverSpawnTimer(gm, 0.0f);
    GTA::SetBomberSpawnTimer(gm, 0.0f);

    PhysicsSystem::UpdateWeaverEnemies(gm, 0.016f);
    PhysicsSystem::UpdateBomberEnemies(gm, 0.016f);

    REQUIRE(GTA::WeaverEnemies(gm).Size() == Config::MAX_WEAVER_ENEMIES);
    REQUIRE(GTA::BomberEnemies(gm).Size() == Config::MAX_BOMBER_ENEMIES);
}

TEST_CASE("UpdateWeaverEnemies: bay het canh phai man hinh thi bien mat, khong con trong pool", "[physics][weaver]") {
    GameManager gm;
    GTA::WeaverEnemies(gm).Clear();
    WeaverEnemy w{};
    w.rect = { (float)Config::SCREEN_W + 1.0f, 100.0f, 32.0f, 22.0f }; // Da qua canh phai
    w.direction = 1;
    w.baseY = 100.0f;
    GTA::WeaverEnemies(gm).Spawn(w);
    GTA::SetWeaverSpawnTimer(gm, 999.0f); // Khoa spawn moi - chi kiem tra dung hanh vi thoat man hinh

    PhysicsSystem::UpdateWeaverEnemies(gm, 0.016f);

    REQUIRE(GTA::WeaverEnemies(gm).Size() == 0);
}

TEST_CASE("UpdateWeaverEnemies: Y dao dong quanh baseY, KHONG bao gio vuot [baseY-amplitude, baseY+amplitude] du chay nhieu frame", "[physics][weaver]") {
    GameManager gm;
    GTA::WeaverEnemies(gm).Clear();
    WeaverEnemy w{};
    w.rect = { 400.0f, 100.0f, 32.0f, 22.0f }; // Giua man hinh - khong thoat canh trong luc test
    w.direction = 1;
    w.baseY = 100.0f;
    w.phase = 0.0f;
    GTA::WeaverEnemies(gm).Spawn(w);
    GTA::SetWeaverSpawnTimer(gm, 999.0f);

    float minY = 1e9f, maxY = -1e9f;
    for (int i = 0; i < 300; i++) { // ~5s o 60fps - du phu ca 1 chu ky day cua sin
        PhysicsSystem::UpdateWeaverEnemies(gm, 1.0f / 60.0f);
        if (GTA::WeaverEnemies(gm).Size() == 0) break; // Lo bay ra canh (khong nen xay ra o giua man hinh, nhung thoat som neu co)
        float y = GTA::WeaverEnemies(gm)[0].rect.y;
        minY = fminf(minY, y);
        maxY = fmaxf(maxY, y);
    }

    REQUIRE(minY >= 100.0f - Config::WEAVER_WEAVE_AMPLITUDE - 0.01f);
    REQUIRE(maxY <= 100.0f + Config::WEAVER_WEAVE_AMPLITUDE + 0.01f);
}

TEST_CASE("UpdateBomberEnemies: het BOMBER_BOMB_INTERVAL thi tha dung 1 dan enemy ban thang xuong", "[physics][bomber]") {
    GameManager gm;
    GTA::BomberEnemies(gm).Clear();
    GTA::EnemyBullets(gm).Reset();
    BomberEnemy b{};
    b.rect = { 400.0f, 70.0f, 34.0f, 24.0f };
    b.direction = 1;
    b.bombTimer = 0.0f; // Het han ngay frame dau
    GTA::BomberEnemies(gm).Spawn(b);
    GTA::SetBomberSpawnTimer(gm, 999.0f); // Khoa spawn moi, chi test hanh vi tha bom

    size_t before = GTA::EnemyBullets(gm).GetActiveCount();
    PhysicsSystem::UpdateBomberEnemies(gm, 0.016f);
    size_t after = GTA::EnemyBullets(gm).GetActiveCount();

    REQUIRE(after == before + 1);
    Vector2 vel = GTA::EnemyBullets(gm).GetBullet(after - 1).GetVel();
    REQUIRE(vel.x == Approx(0.0f).margin(0.01f));
    REQUIRE(vel.y > 0.0f); // Roi XUONG (Y+ la xuong duoi)
}

// Phase 4 (Enemy & Item Revolution, "don no"): truoc Phase 4, Weaver/Bomber dung hang so
// toc do RIENG (khong qua GetDifficultyStats()) nen HOAN TOAN mien nhiem voi DDA - 2 test
// duoi xac nhan lo hong do da duoc va, dong bo voi Basic/Tanky/Zigzag/Warden/Medic.
TEST_CASE("UpdateWeaverEnemies: ddaSpeedMul nhan truc tiep vao toc do ngang (khong con mien nhiem DDA nhu truoc Phase 4)", "[physics][weaver][dda]") {
    GameManager gm;
    GTA::WeaverEnemies(gm).Clear();
    GTA::SetWeaverSpawnTimer(gm, 999.0f);
    WeaverEnemy w{};
    w.rect = { 400.0f, 100.0f, 32.0f, 22.0f };
    w.direction = 1;
    w.baseY = 100.0f;
    GTA::WeaverEnemies(gm).Spawn(w);
    float xBefore = GTA::WeaverEnemies(gm)[0].rect.x;

    GTA::SetDdaSpeedMul(gm, 2.0f); // Gia lap DDA dang "thuong" nguoi choi choi tot
    PhysicsSystem::UpdateWeaverEnemies(gm, 0.1f);
    float dxDoubled = GTA::WeaverEnemies(gm)[0].rect.x - xBefore;

    GTA::WeaverEnemies(gm).Clear();
    GTA::WeaverEnemies(gm).Spawn(w); // Spawn lai tu dau, cung diem xuat phat
    GTA::SetDdaSpeedMul(gm, 1.0f); // DDA trung tinh
    PhysicsSystem::UpdateWeaverEnemies(gm, 0.1f);
    float dxNormal = GTA::WeaverEnemies(gm)[0].rect.x - xBefore;

    REQUIRE(dxDoubled == Approx(dxNormal * 2.0f).epsilon(0.001));
}

TEST_CASE("UpdateBomberEnemies: ddaSpeedMul nhan vao toc do ngang VA chia nguoc vao khoang cho tha bom (khong con mien nhiem DDA nhu truoc Phase 4)", "[physics][bomber][dda]") {
    GameManager gm;
    GTA::BomberEnemies(gm).Clear();
    GTA::SetBomberSpawnTimer(gm, 999.0f);
    BomberEnemy b{};
    b.rect = { 400.0f, 70.0f, 34.0f, 24.0f };
    b.direction = 1;
    b.bombTimer = 0.0f; // Het han ngay - kich hoat nhanh reset bombTimer ben duoi
    GTA::BomberEnemies(gm).Spawn(b);
    float xBefore = GTA::BomberEnemies(gm)[0].rect.x;

    GTA::SetDdaSpeedMul(gm, 2.0f);
    PhysicsSystem::UpdateBomberEnemies(gm, 0.1f);
    float dx = GTA::BomberEnemies(gm)[0].rect.x - xBefore;
    float bombTimerAfter = GTA::BomberEnemies(gm)[0].bombTimer;

    // Toc do ngang: dda=2.0 phai di gap DOI quang duong so voi dda=1.0 trong cung 0.1s.
    REQUIRE(dx == Approx(Config::BOMBER_SPEED_X * 2.0f * 0.1f).epsilon(0.001));
    // Khoang cho tha bom: dda=2.0 phai CHIA (khong phai nhan) - khoang cho con lai LUON
    // BOMBER_BOMB_INTERVAL/2.0, ngan hon binh thuong (tha bom nhanh hon khi DDA "thuong").
    REQUIRE(bombTimerAfter == Approx(Config::BOMBER_BOMB_INTERVAL / 2.0f).epsilon(0.001));
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

    // Nguoi 3 (Audio & UI) - hit-flash: duong "boss trung, con song, khong shield" la 1
    // trong 2 diem noi flashOnHit duoc dat theo ke hoach chia viec (diem con lai la Tanky,
    // xem test C3.2 o tren) - truoc gio chua co test nao kiem noi dung pendingEvents cho
    // đúng nhánh nay ca, chi kiem HP/dan.
    const auto& events = GTA::PendingEvents(gm);
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].sfx == SfxType::Hit);
    REQUIRE(events[0].flashOnHit == true);
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
    REQUIRE(events[0].flashOnHit == false);  // Nguoi 3: cung 1 dieu kien (!shielded && hp>0) nen khien chan CA hit-flash, khong chi sfx
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

TEST_CASE("UpdateBoss: Swarmer trieu hoi dung BOSS_SWARMER_SUMMON_COUNT con, CUNG 1 loai duy nhat khi summonTimer het", "[physics][boss][dda_descriptor]") {
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
    GTA::WeaverEnemies(gm).Clear();
    GTA::BomberEnemies(gm).Clear();

    PhysicsSystem::UpdateBoss(gm, Config::BOSS_SWARMER_SUMMON_INTERVAL);

    // Doi hinh (basic/tanky/zigzag) dang trong trong test nay -> moi ham Spawn* tu roi vao
    // nhanh "spawn tu ngoai man hinh" (xem game_manager.cpp) thay vi "muon" 1 con dang co
    // trong doi hinh - dung y that cua boss wave (InitLevel() de doi hinh trong luc co Boss).
    //
    // Phase 4: Swarmer gio random 1 trong {Kamikaze,Weaver,Bomber} MOI DOT (truoc day LUON
    // Kamikaze) - test nay khong con gia dinh loai cu the, chi xac nhan bat bien KHONG DOI:
    // (1) TONG so con moi sinh dung BOSS_SWARMER_SUMMON_COUNT, (2) ca dot dong nhat 1 loai
    // (chi 1 trong 3 pool tang, 2 pool con lai = 0), khong tron lan nhieu loai trong 1 dot.
    // Test rieng ben duoi xac nhan CA 3 loai deu co the roi trung (khong con luon la
    // Kamikaze) bang thong ke qua nhieu dot doc lap.
    size_t kCount = GTA::KamikazeEnemies(gm).Size();
    size_t wCount = GTA::WeaverEnemies(gm).Size();
    size_t bCount = GTA::BomberEnemies(gm).Size();
    REQUIRE((int)(kCount + wCount + bCount) == Config::BOSS_SWARMER_SUMMON_COUNT);
    int nonZeroPools = (kCount > 0 ? 1 : 0) + (wCount > 0 ? 1 : 0) + (bCount > 0 ? 1 : 0);
    REQUIRE(nonZeroPools == 1);
    REQUIRE(GTA::BossPool(gm)[0].summonTimer == Approx(Config::BOSS_SWARMER_SUMMON_INTERVAL));
}

TEST_CASE("UpdateBoss: qua nhieu dot doc lap, Swarmer trieu hoi ra CA 3 loai (Kamikaze/Weaver/Bomber) - khong con LUON la Kamikaze nhu truoc Phase 4", "[physics][boss][dda_descriptor]") {
    // Test thong ke: 60 dot DOC LAP (moi dot 1 GameManager rieng de khong cong don pool
    // giua cac dot, nhung dung chung 1 luong random toan cuc cua raylib xuyen suot vi
    // GetRandomValue() khong reset theo tung GameManager - moi dot la 1 lan roi that su
    // moi). Xac suat 1/3 moi loai/dot NEU code dung; neu ai do lo hardcode nham lai
    // Kamikaze, xac suat test nay bao "van con bug" (thay it nhat 1 loai bi thieu hoan
    // toan sau 60 dot) xap xi 100% - guong an toan chac chan, khong phai may rui.
    bool sawKamikaze = false, sawWeaver = false, sawBomber = false;
    for (int trial = 0; trial < 60; trial++) {
        GameManager gm;
        Boss b{};
        b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
        b.hp = 40; b.maxHp = 40;
        b.type = BossType::Swarmer;
        b.baseX = 350.0f;
        b.summonTimer = Config::BOSS_SWARMER_SUMMON_INTERVAL;
        GTA::BossPool(gm).Clear();
        GTA::BossPool(gm).Spawn(b);
        GTA::KamikazeEnemies(gm).Clear();
        GTA::WeaverEnemies(gm).Clear();
        GTA::BomberEnemies(gm).Clear();

        PhysicsSystem::UpdateBoss(gm, Config::BOSS_SWARMER_SUMMON_INTERVAL);

        if (GTA::KamikazeEnemies(gm).Size() > 0) sawKamikaze = true;
        if (GTA::WeaverEnemies(gm).Size() > 0) sawWeaver = true;
        if (GTA::BomberEnemies(gm).Size() > 0) sawBomber = true;
    }
    REQUIRE(sawKamikaze);
    REQUIRE(sawWeaver);
    REQUIRE(sawBomber);
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
