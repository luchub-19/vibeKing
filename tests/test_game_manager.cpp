#include "thirdparty/catch.hpp"
#include "game_manager_test_access.h"
#include "input_system.h"
#include "upgrade_types.h"
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
    //
    // BUG FIX: truoc day ham nay KHONG tu don dep, va `CleanupGuard` o tren tuy duoc dinh
    // nghia nhung KHONG TEST_CASE NAO khoi tao no ca (`grep -c "CleanupGuard guard"` = 0) -
    // dead code im lang. Hau qua: moi lan chay unit_tests lai rot 2 file .dat vao cwd va de
    // nguyen do, lam ban working tree (git status khong con sach sau ctest). Gio guard duoc
    // DAT NGAY TRONG ham nay duoi dang bien static function-local: no song den luc chuong
    // trinh thoat roi moi xoa 2 file - dung mot lan cho ca bo test, khong phu thuoc vao viec
    // tung TEST_CASE co nho khai bao hay khong (chinh cho da quen).
    void QuarantinePersistence(GameManager& gm) {
        static CleanupGuard guard; // Xoa 2 file tam khi process ket thuc - xem giai thich tren
        (void)guard;
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

// ==========================================
// NANG CAP SAU WAVE (Track C - Nguoi 2, Phase 3) - 2 test duoi day dung CHINH XAC cung
// khuon voi "MENU -> PLAYING" o tren: KHONG goi UpdateEndScreen() truc tiep (PollMenu()
// doc phan cung that, luon false luc test headless), ma goi thang chuoi ma nhanh
// Confirm==true trong UpdateEndScreen() thuc hien (xem game_manager.cpp) - test nham vao
// CO CHE ap dung nang cap + chuyen trang thai, khong phai lop doc phim.
// ==========================================
TEST_CASE("WAVE_CLEAR -> PLAYING (khong phai boss): ap dung nang cap dung 1 lan, wave khong bi reset ve 1", "[game_manager][state_machine][upgrade]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::WAVE_CLEAR);
    GTA::SetWave(gm, 3); // gm.wave DA la wave SAP choi (xem comment dau nhanh WAVE_CLEAR trong UpdateEndScreen()) - 3 khong chia het BOSS_WAVE_INTERVAL(5)
    GTA::SetSelectedUpgrade(gm, 1); // ExtraLife (index 1 - xem enum UpgradeType, upgrade_types.h)

    int livesBefore = GTA::PlayerRef(gm).GetLives();
    UpgradeType chosen = (UpgradeType)GTA::SelectedUpgrade(gm);
    GTA::PlayerRef(gm).ApplyRunUpgrade(chosen);
    if (GTA::Wave(gm) % Config::BOSS_WAVE_INTERVAL == 0) GTA::PlayerRef(gm).ApplyRunUpgrade(chosen);
    GTA::CallInitLevel(gm, false);
    GTA::CallRequestTransition(gm, GameState::PLAYING);

    REQUIRE(GTA::PlayerRef(gm).GetLives() == livesBefore + 1); // dung 1 lan, khong phai 2
    REQUIRE(GTA::PlayerRef(gm).GetUpgradeStacks(chosen) == 1);
    REQUIRE(GTA::Wave(gm) == 3); // InitLevel(false) KHONG reset wave ve 1 (khac newGame=true)
    REQUIRE(GTA::PendingState(gm) == GameState::PLAYING);
}

TEST_CASE("WAVE_CLEAR -> PLAYING, wave sap toi LA boss: nang cap duoc ap dung 2 LAN thay vi 1", "[game_manager][state_machine][upgrade]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::WAVE_CLEAR);
    GTA::SetWave(gm, Config::BOSS_WAVE_INTERVAL); // gm.wave DA la wave SAP choi - chia het BOSS_WAVE_INTERVAL (vd 5)
    GTA::SetSelectedUpgrade(gm, 2); // BonusScore (index 2)

    int scoreBefore = GTA::PlayerRef(gm).GetScore();
    UpgradeType chosen = (UpgradeType)GTA::SelectedUpgrade(gm);
    GTA::PlayerRef(gm).ApplyRunUpgrade(chosen);
    if (GTA::Wave(gm) % Config::BOSS_WAVE_INTERVAL == 0) GTA::PlayerRef(gm).ApplyRunUpgrade(chosen);
    GTA::CallInitLevel(gm, false);
    GTA::CallRequestTransition(gm, GameState::PLAYING);

    REQUIRE(GTA::PlayerRef(gm).GetUpgradeStacks(chosen) == 2); // wave boss -> ap dung 2 lan
    REQUIRE(GTA::PlayerRef(gm).GetScore() == scoreBefore + 2 * (int)Config::UPGRADE_BONUS_SCORE);
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

// ==========================================
// HOI QUY - HANG DOI SU KIEN (pendingEvents)
//
// 2 test duoi day khoa lai 2 bug thuc su tung ton tai cung 1 luc trong duong xu ly event,
// deu da duoc xac minh bang cong cu chu khong phai suy luan:
//   1. CheckCollisions() clear() hang doi ngay dau ham -> nuot sach event ma UpdateKamikaze()
//      (chay TRUOC no trong cung frame) vua day vao. Do bang probe: player mat 1 mang ma
//      pendingEvents rong tron sau ca UpdatePlaying().
//   2. ProcessEvents() duyet bang range-for trong khi than vong lap co the push_back them
//      (ApplyComboAndScore -> +1 mang tai moc diem) -> heap-use-after-free, bat duoc bang
//      AddressSanitizer.
// ==========================================
TEST_CASE("UpdatePlaying: Kamikaze lao trung player van sinh event no/sfx - CheckCollisions() KHONG duoc nuot event day vao truoc no", "[game_manager][events][kamikaze]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);

    // Giu 1 Basic con song de UpdateEnemies() khong kich hoat nhanh WAVE_CLEAR va return som.
    BasicEnemy keepAlive{};
    keepAlive.rect = { 10.0f, 10.0f, 40.0f, 25.0f };
    keepAlive.column = 0;
    GTA::BasicEnemies(gm).Spawn(keepAlive);

    // 1 Kamikaze dat CHONG KHOP len player, van toc 0 -> chac chan va cham ngay frame nay.
    Rectangle pr = GTA::PlayerRef(gm).GetRect();
    GTA::KamikazeEnemies(gm).Clear();
    GTA::KamikazeEnemies(gm).Spawn(KamikazeEnemy{ { pr.x, pr.y, 20.0f, 20.0f }, RED, { 0.0f, 0.0f } });

    const int livesBefore = GTA::PlayerRef(gm).GetLives();
    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::PlayerRef(gm).GetLives() == livesBefore - 1); // va cham THUC SU xay ra
    // KHONG kiem tra KamikazeEnemies().Size()==0 o day: UpdateKamikaze() bat dau bang viec
    // spawn theo lich (kamikazeSpawnTimer cua 1 GameManager tuoi la 0 -> spawn ngay), nen
    // con vua lao trung bi Destroy() thi lai co 1 con MOI thay cho. So luong pool khong phai
    // tin hieu on dinh cho test nay - va cham da duoc chung minh bang so mang o tren.

    // ProcessEvents() da chay va xa hang doi cuoi UpdatePlaying(), nen khong the doc lai
    // pendingEvents o day. Dung particle lam bang chung: event no cua Kamikaze mang
    // particleCount=16, va CHI ProcessEvents() moi bien no thanh particles.Burst() that su
    // (particles.Update() da chay TRUOC do trong frame nen khong con gi khac sinh hat -
    // player khong ban duoc phat nao khi chay headless). Truoc ban sua, hang doi bi
    // CheckCollisions() xoa sach -> 0 hat, khong no, khong tieng, khong rung.
    REQUIRE(GTA::ParticlesRef(gm).GetActiveCount() >= 16);
}

TEST_CASE("ProcessEvents: event lam vuot moc +1 mang KHONG lam hong vong lap va van duoc xu ly trong cung luot", "[game_manager][events][extra_life]") {
    GameManager gm;
    QuarantinePersistence(gm);

    // Dat diem sat duoi moc +1 mang, de event ghi diem ben duoi day player vuot qua no.
    Player& p = GTA::PlayerRef(gm);
    p.AddScore(Config::EXTRA_LIFE_SCORE_THRESHOLD - 1);
    const int livesBefore = p.GetLives();
    REQUIRE(livesBefore < Config::MAX_LIVES); // moc +1 mang chi co y nghia khi chua kich tran

    // DUNG 1 event trong hang doi -> capacity==1, nen push_back cua ApplyComboAndScore()
    // (+1 mang) CHAC CHAN realloc. Day chinh la dieu kien lam range-for cu treo con tro.
    auto& q = GTA::PendingEvents(gm);
    q.clear();
    GameEvent scoring;
    scoring.position = { 100.0f, 100.0f };
    scoring.scoreValue = Config::EXTRA_LIFE_SCORE_THRESHOLD; // du de vuot moc
    q.push_back(scoring);
    REQUIRE(q.size() == 1);

    GTA::CallProcessEvents(gm);

    REQUIRE(p.GetLives() == livesBefore + 1);          // moc +1 mang da an
    REQUIRE(GTA::PendingEvents(gm).empty());           // hang doi duoc xa sach cuoi ham
    // Event "+1 mang" tu sinh phai duoc xu ly NGAY trong luot nay (particle GOLD burst).
    // Truoc ban sua, range-for khong bao gio duyet toi no va clear() cuoi ham xoa luon -
    // moc +1 mang hoan toan khong co phan hoi hinh anh/am thanh.
    REQUIRE(GTA::ParticlesRef(gm).GetActiveCount() >= 20); // event "+1 mang" mang particleCount=20
}

TEST_CASE("ProcessEvents: popup diem/combo hien tai VI TRI HA GUC, khong phai tai phi thuyen", "[game_manager][events][floating_text]") {
    // Truoc ban sua, ApplyComboAndScore() hardcode player.GetCenter() lam vi tri popup, nen
    // moi con diem deu bay len tu chinh phi thuyen - nhieu don ha guc gan nhau lam cac popup
    // chong DE LEN NHAU o dung 1 diem (doc khong ra) va mat luon thong tin "an diem O DAU".
    // Da thay ro trong anh chup game that: "+12", "COMBO x3", "COMBO x4" xep chong o goc
    // duoi-trai, tren dau phi thuyen.
    GameManager gm;
    QuarantinePersistence(gm);

    const Vector2 killPos{ 640.0f, 120.0f }; // Xa han vi tri phi thuyen (day man hinh, gan giua)
    Vector2 playerCenter = GTA::PlayerRef(gm).GetCenter();
    REQUIRE(killPos.y != playerCenter.y); // tien de cua test: 2 vi tri phai khac nhau that su

    auto& q = GTA::PendingEvents(gm);
    q.clear();
    GameEvent kill;
    kill.position = killPos;
    kill.scoreValue = 100;
    q.push_back(kill);

    auto& texts = GTA::FloatingTextsRef(gm);
    texts.Reset();
    GTA::CallProcessEvents(gm);

    REQUIRE(texts.GetActiveCount() == 1);
    REQUIRE(texts.GetPosition(0).x == killPos.x);
    REQUIRE(texts.GetPosition(0).y == killPos.y);
}

TEST_CASE("Goi y phim: dem nguoc tu HUD_HINT_DURATION o van MOI, ve 0 sau do, va KHONG hoi lai moi wave", "[game_manager][hud]") {
    // Truoc ban sua, dong "P: PAUSE  R: RESTART" hien VINH VIEN giua dinh man hinh suot ca
    // van choi. Gio no chi song Config::HUD_HINT_DURATION giay dau cua 1 van MOI.
    GameManager gm;
    QuarantinePersistence(gm);

    GTA::CallInitLevel(gm, /*newGame=*/true);
    REQUIRE(GTA::HintTimer(gm) == Config::HUD_HINT_DURATION);

    GTA::SetState(gm, GameState::PLAYING);
    // Giu 1 dich song de UpdatePlaying() khong roi vao nhanh WAVE_CLEAR va return som.
    GTA::BasicEnemies(gm).Clear();
    BasicEnemy keepAlive{};
    keepAlive.rect = { 10.0f, 10.0f, 40.0f, 25.0f };
    GTA::BasicEnemies(gm).Spawn(keepAlive);

    GTA::CallUpdatePlaying(gm, 1.0f);
    REQUIRE(GTA::HintTimer(gm) == Approx(Config::HUD_HINT_DURATION - 1.0f));

    // Chay qua het thoi luong -> ve <= 0 va KHONG di xuong am vo han (dem nguoc co chan).
    for (int f = 0; f < 60; f++) GTA::CallUpdatePlaying(gm, 0.5f);
    REQUIRE(GTA::HintTimer(gm) <= 0.0f);
    REQUIRE(GTA::HintTimer(gm) > -1.0f); // khong troi tu do sau khi da ve 0

    // Sang wave ke tiep (InitLevel(false)) KHONG lam goi y hien lai - no la huong dan cho
    // nguoi choi MOI, khong phai thong bao moi wave.
    GTA::CallInitLevel(gm, /*newGame=*/false);
    REQUIRE(GTA::HintTimer(gm) <= 0.0f);
}

// ==========================================
// TONG KET RUN + BANNER DAU WAVE (Phase Graphics/UX)
// ==========================================
TEST_CASE("Tong ket run: kills/best combo/CR kiem duoc duoc ghi nhan va reset dung o van MOI", "[game_manager][summary]") {
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::CallInitLevel(gm, /*newGame=*/true);
    REQUIRE(GTA::RunKills(gm) == 0);
    REQUIRE(GTA::RunBestCombo(gm) == 0);

    // 3 event ghi diem lien tiep -> 3 kill, combo len bac 3.
    auto& q = GTA::PendingEvents(gm);
    for (int i = 0; i < 3; i++) {
        q.clear();
        GameEvent kill;
        kill.position = { 100.0f + (float)i * 20.0f, 100.0f };
        kill.scoreValue = 100;
        q.push_back(kill);
        GTA::CallProcessEvents(gm);
    }
    REQUIRE(GTA::RunKills(gm) == 3);
    REQUIRE(GTA::RunBestCombo(gm) == 3);

    // Sang wave ke tiep KHONG duoc reset - day la thong ke cua ca VAN, khong phai cua wave.
    GTA::CallInitLevel(gm, /*newGame=*/false);
    REQUIRE(GTA::RunKills(gm) == 3);
    REQUIRE(GTA::RunBestCombo(gm) == 3);

    // Van MOI thi reset sach.
    GTA::CallInitLevel(gm, /*newGame=*/true);
    REQUIRE(GTA::RunKills(gm) == 0);
    REQUIRE(GTA::RunBestCombo(gm) == 0);
    REQUIRE(GTA::RunCurrencyEarned(gm) == 0);
}

TEST_CASE("Tong ket run: runCurrencyEarned KHOP dung gia tri AwardCurrency() tra ve luc GAME_OVER", "[game_manager][summary][currency]") {
    // Khoa lai rang bang tong ket doc CON SO THAT tu MetaProgress chu khong tu tinh lai
    // score/META_SCORE_TO_CURRENCY_RATE o tang hien thi (2 noi tinh se lech nhau khi doi RATE).
    GameManager gm;
    QuarantinePersistence(gm);
    GTA::SetState(gm, GameState::PLAYING);

    const int score = Config::META_SCORE_TO_CURRENCY_RATE * 7 + 13; // 7 CR + phan du bi lam tron xuong
    GTA::PlayerRef(gm).AddScore(score);
    REQUIRE(GTA::PlayerRef(gm).GetScore() == score);
    int currencyBefore = GTA::MetaProgressRef(gm).GetCurrency();

    // Dua lives ve 0 qua API cong khai cua Player, dung khuon test GAME_OVER o tren (phai
    // Update() qua het cua so bat tu giua 2 lan TakeDamage).
    Player& p = GTA::PlayerRef(gm);
    BulletPool<Config::MAX_PLAYER_BULLETS>& bullets = GTA::PlayerBullets(gm);
    for (int guard = 0; guard < 10 && p.GetLives() > 0; guard++) {
        REQUIRE(p.TakeDamage());
        if (p.GetLives() <= 0) break;
        p.Update(Config::INVINCIBLE_TIME + 0.1f, InputState{}, bullets);
    }
    REQUIRE(p.GetLives() <= 0);

    // Giu 1 dich song, dat xa ca 2 bien va xa hang player, de UpdateEnemies() khong kich
    // hoat WAVE_CLEAR/GAME_OVER-do-cham-day truoc khi ham chay toi doan kiem tra lives.
    GTA::BasicEnemies(gm).Clear();
    BasicEnemy keepAlive{};
    keepAlive.rect = { 400.0f, 100.0f, 40.0f, 25.0f };
    GTA::BasicEnemies(gm).Spawn(keepAlive);
    GTA::CallUpdatePlaying(gm, 0.016f);

    REQUIRE(GTA::PendingState(gm) == GameState::GAME_OVER);
    REQUIRE(GTA::RunCurrencyEarned(gm) == 7);
    REQUIRE(GTA::MetaProgressRef(gm).GetCurrency() == currencyBefore + 7);
}

TEST_CASE("Banner dau wave: hien o MOI wave (ca wave dau lan wave sau) roi tu tat", "[game_manager][banner]") {
    GameManager gm;
    QuarantinePersistence(gm);

    GTA::CallInitLevel(gm, /*newGame=*/true);
    REQUIRE(GTA::WaveBannerTimer(gm) == Config::WAVE_BANNER_DURATION);

    GTA::SetState(gm, GameState::PLAYING);
    GTA::BasicEnemies(gm).Clear();
    BasicEnemy keepAlive{};
    keepAlive.rect = { 10.0f, 10.0f, 40.0f, 25.0f };
    GTA::BasicEnemies(gm).Spawn(keepAlive);
    for (int f = 0; f < 40; f++) GTA::CallUpdatePlaying(gm, 0.1f); // 4s > WAVE_BANNER_DURATION
    REQUIRE(GTA::WaveBannerTimer(gm) <= 0.0f);

    // KHAC hintTimer (chi hien o van moi): banner phai hien LAI moi wave.
    GTA::CallInitLevel(gm, /*newGame=*/false);
    REQUIRE(GTA::WaveBannerTimer(gm) == Config::WAVE_BANNER_DURATION);
}
