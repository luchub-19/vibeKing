#include "thirdparty/catch.hpp"
#include "game_manager_test_access.h"
#include "input_system.h"
#include <cstdio>

// ==========================================
// TEST_GAME_MANAGER - state machine (MENU/PLAYING/GAME_OVER/WAVE_CLEAR) va thoi diem
// cong currency. Day la luoi an toan cho Track B2 (DDA sua UpdatePlaying()) - moi test o
// day khoa lai HANH VI HIEN TAI cua UpdatePlaying()/UpdateTransition(), khong phai hanh
// vi "dung ra phai vay". Neu B2 co y dinh THAY DOI 1 trong nhung dieu duoi day, sua test
// tuong ung CHU KHONG phai xoa - do la tin hieu can review ky, khong phai regression gia.
//
// Khong goi GameManager::Run()/InitWindow() o bat ky test nao - moi ham duoc goi thang
// qua GameManagerTestAccess (xem file do). Da xac nhan an toan headless: IsKeyDown/
// IsGamepadAvailable/GetRandomValue tra ve 0/false khi chua InitWindow(), va PlaySound()
// tren 1 Sound{} chua tung LoadSound() cung an toan (raylib tu no-op) - xem probe rieng
// da chay thu truoc khi viet file nay, khong phai suy doan.
// ==========================================

namespace {
    const char* MetaTestPath() { return "test_game_manager_meta_tmp.dat"; }
    const char* LeaderboardTestPath() { return "test_game_manager_leaderboard_tmp.dat"; }

    // Dung 1 guard chung cho ca 2 file tam - ca 2 luon di cung nhau trong moi test o day
    // (bat ky duong nao dan toi GAME_OVER/WAVE_CLEAR deu dung ca leaderboard.TrySubmit
    // lan metaProgress, xem UpdatePlaying()/UpdateEnemies()).
    struct CleanupGuard {
        ~CleanupGuard() { std::remove(MetaTestPath()); std::remove(LeaderboardTestPath()); }
    };

    // Dua metaProgress/leaderboard cua 1 GameManager tuoi ve 2 file tam rieng cua bo test
    // nay - PHAI goi truoc bat ky scenario nao co the trigger TrySubmit()/AwardCurrency(),
    // neu khong chung se ghi de len file save THAT cua nguoi choi (duong dan mac dinh
    // hardcode trong GameManager::Run(), xem "meta_progress.dat"/Config::LeaderboardFilePath()).
    void QuarantinePersistence(GameManager& gm) {
        GameManagerTestAccess::MetaProgressRef(gm).Load(MetaTestPath());
        GameManagerTestAccess::LeaderboardRef(gm).Load(LeaderboardTestPath());
    }
}

using GTA = GameManagerTestAccess;

TEST_CASE("GameManager moi tao mac dinh o MENU, khong co fade nao dang chay", "[game_manager][state_machine]") {
    GameManager gm;
    REQUIRE(GTA::State(gm) == GameState::MENU);
    REQUIRE(GTA::Phase(gm) == TransitionPhase::NONE);
}

TEST_CASE("RequestTransition: state KHONG doi ngay - chi doi state that su sau khi UpdateTransition du 1 chu ky FADE_OUT, roi ve NONE sau chu ky FADE_IN", "[game_manager][state_machine]") {
    GameManager gm;
    GTA::CallRequestTransition(gm, GameState::PLAYING);

    // Ngay sau RequestTransition: pendingState da ghi nhan muc tieu, nhung state THAT SU
    // (dang duoc Run() dung de switch/case chon nhanh Update/Draw nao) van chua doi - dung
    // y muon, tranh chuyen canh giat cuc (xem ARCHITECTURE.md muc Fade transition).
    REQUIRE(GTA::State(gm) == GameState::MENU);
    REQUIRE(GTA::PendingState(gm) == GameState::PLAYING);
    REQUIRE(GTA::Phase(gm) == TransitionPhase::FADE_OUT);

    GTA::CallUpdateTransition(gm, Config::TRANSITION_DURATION); // vua du 1 chu ky FADE_OUT
    REQUIRE(GTA::State(gm) == GameState::PLAYING); // state that su da doi
    REQUIRE(GTA::Phase(gm) == TransitionPhase::FADE_IN);

    GTA::CallUpdateTransition(gm, Config::TRANSITION_DURATION); // chu ky FADE_IN
    REQUIRE(GTA::Phase(gm) == TransitionPhase::NONE); // fade xong hoan toan
}

TEST_CASE("MENU -> PLAYING: dung dung chuoi UpdateMenu() thuc hien khi nguoi choi bam Confirm (InitLevel(true) + RequestTransition)", "[game_manager][state_machine]") {
    GameManager gm;
    QuarantinePersistence(gm);

    // Day CHINH XAC la 2 dong GameManager::UpdateMenu() chay khi input.Confirm==true - xem
    // game_manager.cpp. Khong goi UpdateMenu() truc tiep vi no doc phan cung qua
    // InputSystem::PollMenu() (luon tra ve false het trong moi truong headless), nen
    // khong the mo phong "nguoi choi bam Enter" tu ben ngoai - test o day nham vao CO CHE
    // chuyen trang thai, khong phai lop doc phim (xem input_system.h).
    GTA::CallInitLevel(gm, true);
    GTA::CallRequestTransition(gm, GameState::PLAYING);
    REQUIRE(GTA::Wave(gm) == 1); // InitLevel(true) luon bat dau tu wave 1

    GTA::CallUpdateTransition(gm, Config::TRANSITION_DURATION);
    REQUIRE(GTA::State(gm) == GameState::PLAYING);
}

TEST_CASE("PLAYING, khong phai boss wave, doi hinh da don sach (0 dich con lai): UpdateEnemies yeu cau chuyen WAVE_CLEAR, wave tang dung 1, KHONG cong currency", "[game_manager][state_machine][currency]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, false);
    GTA::SetWave(gm, 3);
    // KHONG spawn basic/tanky/zigzag nao ca - activeCount()==0 ngay tu dau, dung kich ban
    // "vua ha dich cuoi cung xong" (enemy da bi CheckCollisions() xoa tu truoc do).

    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();
    PhysicsSystem::UpdateEnemies(gm, 0.016f);

    REQUIRE(GTA::Wave(gm) == 4);
    REQUIRE(GTA::State(gm) == GameState::PLAYING); // chua doi ngay - RequestTransition moi chi dat pendingState
    REQUIRE(GTA::PendingState(gm) == GameState::WAVE_CLEAR);
    REQUIRE(GTA::Phase(gm) == TransitionPhase::FADE_OUT);
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore);
}

TEST_CASE("PLAYING, Basic/Tanky/Zigzag da het nhung Warden/Medic CON SONG: UpdateEnemies KHONG duoc yeu cau chuyen WAVE_CLEAR", "[game_manager][state_machine][warden][medic]") {
    // Phase 1a (Enemy & Item Revolution, Nguoi 1) - REGRESSION TEST cho fix activeCount
    // trong PhysicsSystem::UpdateEnemies() (physics_system.cpp): truoc fix, dong nay CHI
    // dem Basic+Tanky+Zigzag nen se bao "het dich" (sai) ngay ca khi Warden/Medic (co ham
    // Update RIENG, khong nam trong UpdateEnemies()) van con song tren man hinh. 2 test
    // duoi day (Warden song, roi Medic song) khoa lai dung ca 2 nhanh cua dieu kien
    // activeCount moi.
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, false);
    GTA::SetWave(gm, 3);
    GTA::WardenEnemies(gm).Spawn(WardenEnemy{ {100.0f, 100.0f, 42.0f, 30.0f}, DARKBLUE, 0, WardenEnemy::HP });
    // KHONG spawn basic/tanky/zigzag - activeCount cua 3 pool CU = 0, nhung Warden con song.
    GameState pendingBefore = GTA::PendingState(gm); // Ghi lai TRUOC - dung "khong doi" thay vi doan gia tri mac dinh cu the (MENU) de test khong vo tinh sai neu default doi sau nay

    PhysicsSystem::UpdateEnemies(gm, 0.016f);

    REQUIRE(GTA::Wave(gm) == 3);                              // KHONG tang
    REQUIRE(GTA::PendingState(gm) == pendingBefore);            // KHONG co transition nao duoc yeu cau (RequestTransition la noi DUY NHAT ghi pendingState)
    REQUIRE(GTA::Phase(gm) == TransitionPhase::NONE);
    REQUIRE(GTA::WardenEnemies(gm).Size() == 1);                // Warden khong bi dong cham gi (UpdateEnemies khong biet den pool nay)
}

TEST_CASE("PLAYING, Basic/Tanky/Zigzag/Warden da het nhung Medic CON SONG: UpdateEnemies KHONG duoc yeu cau chuyen WAVE_CLEAR", "[game_manager][state_machine][medic]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, false);
    GTA::SetWave(gm, 7);
    GTA::MedicEnemies(gm).Spawn(MedicEnemy{ {200.0f, 100.0f, 34.0f, 24.0f}, LIME, 0, 0.0f });
    GameState pendingBefore = GTA::PendingState(gm);

    PhysicsSystem::UpdateEnemies(gm, 0.016f);

    REQUIRE(GTA::Wave(gm) == 7);
    REQUIRE(GTA::PendingState(gm) == pendingBefore);
    REQUIRE(GTA::Phase(gm) == TransitionPhase::NONE);
}

TEST_CASE("PLAYING, CA 5 pool doi hinh (Basic/Tanky/Zigzag/Warden/Medic) deu rong: UpdateEnemies VAN yeu cau chuyen WAVE_CLEAR nhu truoc", "[game_manager][state_machine][warden][medic]") {
    // Doi xung voi 2 test tren: xac nhan fix KHONG lam mat kha nang phat hien wave-clear
    // that su khi Warden/Medic CUNG da het (khong chi Basic/Tanky/Zigzag) - phong truong
    // hop sua activeCount theo huong nguoc (vd quen mot so hang, hoac dieu kien luon false).
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, false);
    GTA::SetWave(gm, 3);
    // Khong spawn gi ca o CA 5 pool.

    PhysicsSystem::UpdateEnemies(gm, 0.016f);

    REQUIRE(GTA::Wave(gm) == 4);
    REQUIRE(GTA::PendingState(gm) == GameState::WAVE_CLEAR);
    REQUIRE(GTA::Phase(gm) == TransitionPhase::FADE_OUT);
}


TEST_CASE("PLAYING, boss wave, boss HP da ve 0: UpdatePlaying yeu cau chuyen WAVE_CLEAR, wave tang dung 1, KHONG cong currency", "[game_manager][state_machine][currency]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, true);
    GTA::SetWave(gm, 5);

    Boss b{};
    b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
    b.hp = 0; // da bi ha - day la dieu kien UpdatePlaying() kiem tra sau CheckCollisions()
    b.maxHp = 40;
    b.type = BossType::Vanguard;
    GTA::BossPool(gm).Clear();
    GTA::BossPool(gm).Spawn(b);

    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();
    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::Wave(gm) == 6);
    REQUIRE(GTA::BossPool(gm).Size() == 0); // bossPool.Destroy(0) da chay
    REQUIRE(GTA::PendingState(gm) == GameState::WAVE_CLEAR);
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore);
}

TEST_CASE("PLAYING, player het mang (lives<=0): UpdatePlaying yeu cau chuyen GAME_OVER VA cong currency dung theo score hien tai", "[game_manager][state_machine][currency]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, false);

    // Can it nhat 1 dich con song de UpdateEnemies() KHONG tu kich hoat nhanh WAVE_CLEAR
    // (activeCount==0) truoc khi ham chay toi doan kiem tra lives o cuoi - dat o giua man
    // hinh, xa ca 2 bien va xa hang cua player, de mot frame dt nho khong vo tinh cham
    // hitEdge/DescendRowAndCheckGameOver (xem test rieng ben duoi cho duong do).
    BasicEnemy e{};
    e.rect = { 400.0f, 100.0f, 32.0f, 24.0f };
    e.color = WHITE;
    GTA::BasicEnemies(gm).Clear();
    GTA::BasicEnemies(gm).Spawn(e);

    Player& p = GTA::PlayerRef(gm);
    BulletPool<Config::MAX_PLAYER_BULLETS>& bullets = GTA::PlayerBullets(gm);
    int scoreBefore = p.GetScore(); // mac dinh 0 ngay sau Reset(), nhung doc lai cho ro y

    // Dua lives ve 0 qua chinh API cong khai cua Player (TakeDamage/Update) thay vi ghi de
    // truc tiep field private - Player khong nam trong pham vi GameManagerTestAccess (chi
    // GameManager moi can seam nay), va TakeDamage()/Update() da du de mo phong dung "het
    // mang" ma khong can dung toi va cham that. Phai Update() voi dt > Config::INVINCIBLE_TIME
    // giua 2 lan TakeDamage() de qua het cua so bat tu (xem Player::TakeDamage()).
    for (int guard = 0; guard < 10 && p.GetLives() > 0; guard++) {
        bool hit = p.TakeDamage();
        REQUIRE(hit); // moi lan phai thuc su tru mang - neu false nghia la setup sai (van dang bat tu)
        if (p.GetLives() <= 0) break;
        p.Update(Config::INVINCIBLE_TIME + 0.1f, InputState{}, bullets);
    }
    REQUIRE(p.GetLives() <= 0); // xac nhan tien dieu kien dung truoc khi kiem tra hanh vi that

    int expectedCurrencyGain = scoreBefore / Config::META_SCORE_TO_CURRENCY_RATE;
    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();

    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::State(gm) == GameState::PLAYING); // RequestTransition moi dat pendingState, chua doi ngay
    REQUIRE(GTA::PendingState(gm) == GameState::GAME_OVER);
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore + expectedCurrencyGain);
}

TEST_CASE("PLAYING, doi hinh cham day man hinh (DescendRowAndCheckGameOver) cung dan toi GAME_OVER nhung KHONG cong currency - khac voi duong het mang o tren", "[game_manager][state_machine][currency]") {
    // Ghi chu quan trong cho Track B2: UpdatePlaying() hien co 2 duong rieng biet dan toi
    // GAME_OVER - (1) player.GetLives()<=0 o cuoi UpdatePlaying() (CO cong currency, xem
    // test o tren), va (2) doi hinh dich cham xuong toi hang player qua
    // PhysicsSystem::DescendRowAndCheckGameOver() (KHONG cong currency - xem
    // physics_system.cpp, nhanh nay tra ve truoc khi UpdatePlaying() kip chay toi dong
    // AwardCurrency()). Test nay CHI xac nhan hanh vi HIEN TAI (co the la bat doi xung co
    // chu dich, cung co the la 1 gap chua ai de y) - khong tu y "sua" cho giong nhau, vi do
    // la quyet dinh thuoc ve Track B, khong phai Track C.
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, false);
    GTA::SetWave(gm, 2);

    Player& p = GTA::PlayerRef(gm);
    float playerY = p.GetY(); // mac dinh Config::PLAYER_SPAWN_Y (550.0f), doc lai cho khong phu thuoc gia tri hardcode

    BasicEnemy e{};
    // X: dat sat bien phai - voi enemyDirection=1 (mac dinh) va enemySpeed>0, chi 1 buoc
    // di chuyen nho cung du lam formationX+width vuot Config::SCREEN_W -> hitEdge=true.
    e.rect.x = (float)Config::SCREEN_W - 32.0f;
    // Y: dat ngay sat phia tren hang player - sau khi DescendRowAndCheckGameOver() cong
    // them 20px, EnemyBottom() chac chan >= playerY.
    e.rect.y = playerY - 5.0f;
    e.rect.width = 32.0f;
    e.rect.height = 24.0f;
    e.color = WHITE;
    GTA::BasicEnemies(gm).Clear();
    GTA::BasicEnemies(gm).Spawn(e);

    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();
    PhysicsSystem::UpdateEnemies(gm, 0.016f);

    REQUIRE(GTA::PendingState(gm) == GameState::GAME_OVER);
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore); // KHONG cong currency o duong nay
}

// ==========================================
// DDA CHECKPOINT (Track B2) - phan con lai cua luoi an toan da hua o dau file nay.
// test_balance_config.cpp chi xac nhan Config::DDA_* NAP DUNG tu balance.json - cac test
// duoi day moi thuc su goi UpdatePlaying() qua nhanh BOSS DEFEAT va kiem tra ddaSpeedMul
// tang/giam/giu nguyen dung cong thuc (xem game_manager.cpp, ngay truoc RequestTransition
// (WAVE_CLEAR) trong nhanh do). Dung chung 1 helper spawn Boss toi thieu cho ca 5 test.
// ==========================================
namespace {
    // Boss hp=0 (da bi ha) + phaseTimer/summonTimer khoi tao dung nhu GameManager::
    // SpawnBoss() that (khong phai gia tri mac dinh 0.0f cua struct Boss{}) - type khong
    // quan trong voi DDA (chi anh huong desc.movement/shield/summon, khong lien quan gi
    // toi checkpoint), dung Vanguard cho don gian nhat.
    void SpawnDefeatedBossFor(GameManager& gm) {
        Boss b{};
        b.rect = { 350.0f, 80.0f, 100.0f, 60.0f };
        b.hp = 0;
        b.maxHp = 40;
        b.type = BossType::Vanguard;
        b.phaseTimer = Config::BOSS_SENTINEL_SHIELD_INTERVAL;
        b.summonTimer = Config::BOSS_SWARMER_SUMMON_INTERVAL;
        GameManagerTestAccess::BossPool(gm).Clear();
        GameManagerTestAccess::BossPool(gm).Spawn(b);
    }

    // TakeDamage() tra ve false neu con dang trong Config::INVINCIBLE_TIME (1.2s) sau don
    // truoc - goi lien tuc N lan KHONG tao ra N lan mat mang that (chi lan dau tinh). Can
    // "cho" het thoi gian bat tu giua 2 don bang 1 Player::Update() trung tinh (InputState
    // rong = khong di chuyen/khong ban) truoc khi danh don ke tiep.
    void InflictLives(GameManager& gm, int count) {
        InputState neutral{};
        for (int i = 0; i < count; i++) {
            if (i > 0) GameManagerTestAccess::PlayerRef(gm).Update(Config::INVINCIBLE_TIME + 0.01f, neutral, GameManagerTestAccess::PlayerBullets(gm));
            GameManagerTestAccess::PlayerRef(gm).TakeDamage();
        }
    }
}

TEST_CASE("DDA checkpoint: khong mat mang nao trong chu ky Boss -> ddaSpeedMul tang dung DDA_STEP_UP, ddaLivesLostSinceCheck reset ve 0", "[game_manager][dda]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, true);
    GTA::SetDdaLastKnownLives(gm, GTA::PlayerRef(gm).GetLives()); // dong bo dung mang THAT SU (Player bat dau =3, khac Config::MAX_LIVES=5 la TRAN treo - xem fix trong InitLevel())
    GTA::SetDdaSpeedMul(gm, 1.0f);
    SpawnDefeatedBossFor(gm);
    // KHONG goi TakeDamage() - currentLives == ddaLastKnownLives (ca 2 deu mac dinh
    // Config::MAX_LIVES luc GameManager/Player moi tao) -> ddaLivesLostSinceCheck phai la 0.

    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::DdaSpeedMul(gm) == Approx(1.0f + Config::DDA_STEP_UP));
    REQUIRE(GTA::DdaLivesLostSinceCheck(gm) == 0);
}

TEST_CASE("DDA checkpoint: ddaSpeedMul khong bao gio vuot DDA_MAX_MUL du dang gan tran luc checkpoint", "[game_manager][dda]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, true);
    GTA::SetDdaLastKnownLives(gm, GTA::PlayerRef(gm).GetLives()); // dong bo dung mang THAT SU (Player bat dau =3, khac Config::MAX_LIVES=5 la TRAN treo - xem fix trong InitLevel())
    // Gan tran hon 1 buoc DDA_STEP_UP - cong them se vuot DDA_MAX_MUL neu khong clamp.
    GTA::SetDdaSpeedMul(gm, Config::DDA_MAX_MUL - (Config::DDA_STEP_UP / 2.0f));
    SpawnDefeatedBossFor(gm);

    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::DdaSpeedMul(gm) == Approx(Config::DDA_MAX_MUL));
}

TEST_CASE("DDA checkpoint: mat tu DDA_STRUGGLE_THRESHOLD mang tro len trong chu ky Boss -> ddaSpeedMul giam dung DDA_STEP_DOWN", "[game_manager][dda]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, true);
    GTA::SetDdaLastKnownLives(gm, GTA::PlayerRef(gm).GetLives()); // dong bo dung mang THAT SU (Player bat dau =3, khac Config::MAX_LIVES=5 la TRAN treo - xem fix trong InitLevel())
    GTA::SetDdaSpeedMul(gm, 1.0f);
    SpawnDefeatedBossFor(gm);
    int livesBefore = GTA::PlayerRef(gm).GetLives();

    InflictLives(gm, Config::DDA_STRUGGLE_THRESHOLD);
    REQUIRE(GTA::PlayerRef(gm).GetLives() == livesBefore - Config::DDA_STRUGGLE_THRESHOLD);

    // 1 lan UpdatePlaying() DUY NHAT: nhanh cong don o dau ham doc currentLives (da giam o
    // tren) so voi ddaLastKnownLives (da dong bo lai o tren, khong con dung mac dinh sai)
    // TRUOC khi nhanh BOSS DEFEAT phia duoi doc lai ddaLivesLostSinceCheck vua cong - dung
    // thu tu that trong UpdatePlaying(), khong phai gia dinh.
    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::DdaSpeedMul(gm) == Approx(1.0f - Config::DDA_STEP_DOWN));
    REQUIRE(GTA::DdaLivesLostSinceCheck(gm) == 0);
}

TEST_CASE("DDA checkpoint: ddaSpeedMul khong bao gio xuong duoi DDA_MIN_MUL du dang gan san luc checkpoint", "[game_manager][dda]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, true);
    GTA::SetDdaLastKnownLives(gm, GTA::PlayerRef(gm).GetLives()); // dong bo dung mang THAT SU (Player bat dau =3, khac Config::MAX_LIVES=5 la TRAN treo - xem fix trong InitLevel())
    GTA::SetDdaSpeedMul(gm, Config::DDA_MIN_MUL + (Config::DDA_STEP_DOWN / 2.0f));
    SpawnDefeatedBossFor(gm);

    InflictLives(gm, Config::DDA_STRUGGLE_THRESHOLD);
    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::DdaSpeedMul(gm) == Approx(Config::DDA_MIN_MUL));
}

TEST_CASE("DDA checkpoint: mat mang nhung DUOI nguong DDA_STRUGGLE_THRESHOLD -> ddaSpeedMul GIU NGUYEN (vung chet, tranh rung lac vi 1 lan trung don ngau nhien)", "[game_manager][dda]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);
    GTA::SetIsBossWave(gm, true);
    GTA::SetDdaLastKnownLives(gm, GTA::PlayerRef(gm).GetLives()); // dong bo dung mang THAT SU (Player bat dau =3, khac Config::MAX_LIVES=5 la TRAN treo - xem fix trong InitLevel())
    GTA::SetDdaSpeedMul(gm, 1.0f);
    SpawnDefeatedBossFor(gm);

    REQUIRE(Config::DDA_STRUGGLE_THRESHOLD > 1); // gia dinh nen test nay con dung nghia
    GTA::PlayerRef(gm).TakeDamage(); // dung 1 mang - duoi nguong

    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::DdaSpeedMul(gm) == Approx(1.0f)); // khong doi, khong tang cung khong giam
    REQUIRE(GTA::DdaLivesLostSinceCheck(gm) == 0); // van reset du khong dieu chinh gi
}

TEST_CASE("Mo phong 25 wave lien tiep (ca boss wave xen ke): khong crash, khong pool nao vuot capacity, InitLevel Clear dung moi vong", "[game_manager][warden][medic][stability]") {
    // Phase 1a (Enemy & Item Revolution, Nguoi 1) - DoD "choi thu lien tuc 20 wave,
    // Warden/Medic xuat hien dong cung luc, khong crash, khong leak pool". Headless nen
    // khong the "choi that", nhung co the mo phong DUNG chu ky UpdatePlaying() ->
    // WAVE_CLEAR -> fade -> InitLevel() ma game that di qua: moi vong "don sach" wave
    // hien tai bang cach xoa toan bo pool doi hinh/Boss (thay vi gia lap tung vien dan -
    // da co rieng nhieu test o muc va cham/hp ben tren cho tung loai), roi goi
    // CallUpdatePlaying() DUNG NHU game that (khong goi thang UpdateEnemies()/UpdateBoss())
    // de ca 2 nhanh phat hien wave-clear (activeCount==0 cho non-boss, bossPool[0].hp<=0
    // cho boss wave - xem GameManager::UpdatePlaying()) deu duoc kiem tra dung duong that.
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::CallInitLevel(gm, true); // newGame=true -> wave=1

    constexpr int kWavesToSimulate = 25; // > BOSS_WAVE_INTERVAL*4 -> chac chan trai qua nhieu boss wave
    for (int w = 1; w <= kWavesToSimulate; w++) {
        GTA::SetState(gm, GameState::PLAYING);

        // HIT-STOP (hit_stop.h - co san TRUOC Phase 1a, khong lien quan Warden/Medic):
        // chi duoc Update()/giam dan bo trong UpdatePlaying(), ma UpdatePlaying() lai
        // KHONG chay trong luc `frozen` (dang fade - xem Run()). Sau khi ha Boss (wave chia
        // het BOSS_WAVE_INTERVAL), hitStop.Trigger(0.1f) duoc goi NGAY TRUOC
        // RequestTransition(WAVE_CLEAR) - trong game that, ~0.084s con lai do se tu tieu
        // bien vo hinh qua vai frame dau cua wave ke tiep (nguoi choi khong nhan ra). O day
        // vi goi CallUpdateTransition() THANG (bo qua UpdatePlaying(), tuc bo qua ca
        // hitStop.Update()) de tua nhanh fade, hitStop khong duoc tick - reset thu cong
        // truoc moi wave de mo phong dung "du thoi gian da troi qua" thay vi that su di
        // qua tung frame dem nguoc.
        GTA::HitStopRef(gm).timer = 0.0f;

        REQUIRE(GTA::WardenEnemies(gm).Size() <= Config::MAX_WARDEN_ENEMIES);
        REQUIRE(GTA::MedicEnemies(gm).Size() <= Config::MAX_MEDIC_ENEMIES);

        GTA::BasicEnemies(gm).Clear();
        GTA::TankyEnemies(gm).Clear();
        GTA::ZigzagEnemies(gm).Clear();
        GTA::WardenEnemies(gm).Clear();
        GTA::MedicEnemies(gm).Clear();
        if (GTA::BossPool(gm).Size() > 0) GTA::BossPool(gm)[0].hp = 0;

        GTA::CallUpdatePlaying(gm, 0.016f);
        REQUIRE(GTA::PendingState(gm) == GameState::WAVE_CLEAR);

        // Vuot qua ca 2 pha FADE_OUT/FADE_IN de state THAT SU doi (200 frame ~3.2s @60fps
        // - du dai hon nhieu Config::TRANSITION_DURATION*2 bat ke gia tri hien tai la bao nhieu).
        for (int f = 0; f < 200; f++) GTA::CallUpdateTransition(gm, 0.016f);
        REQUIRE(GTA::State(gm) == GameState::WAVE_CLEAR);

        GTA::CallInitLevel(gm, false); // false: dung gm.wave HIEN TAI (da tu tang trong nhanh wave-clear), khong reset ve 1
    }

    REQUIRE(GTA::Wave(gm) == 1 + kWavesToSimulate);
    REQUIRE(GTA::WardenEnemies(gm).Size() <= Config::MAX_WARDEN_ENEMIES);
    REQUIRE(GTA::MedicEnemies(gm).Size() <= Config::MAX_MEDIC_ENEMIES);
}
