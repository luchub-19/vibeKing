#pragma once
#include "raylib.h"
#include <vector>
#include <cstdint>
#include "config.h"
#include "bullet_pool.h"
#include "particle_pool.h"
#include "floating_text.h"
#include "powerup.h"
#include "screen_shake.h"
#include "hit_stop.h"
#include "audio_system.h"
#include "leaderboard.h"
#include "meta_progress.h"
#include "settings.h"
#include "player.h"
#include "enemy_types.h"
#include "level_config.h"
#include "spatial_grid.h"
#include "bunker.h"
#include "sprites.h"
#include "parallax.h"
#include "post_process.h"
#include "events.h"
#include "localization.h"

enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, WAVE_CLEAR, KEYBIND };
enum class TransitionPhase { NONE, FADE_OUT, FADE_IN };

// ==========================================
// 4 HANH DONG CO THE REBIND (man hinh KEYBIND) - DUY NHAT 1 noi liet ke thu tu/nhan,
// dung con tro-thanh-vien (pointer-to-member) de GameManager::UpdateKeybindScreen()
// (ghi ma phim moi) va RenderSystem::DrawKeybindScreen() (doc de hien thi) LUON tham
// chieu CUNG 1 dinh nghia - khong the xay ra tinh huong 2 noi liet ke thu tu khac nhau
// roi lech nhau ve sau (dung y het van de tung sua o powerup.h: comment/code lech nhau).
// ==========================================
struct RebindableAction { const char* label; int Settings::*keyField; };
constexpr int REBINDABLE_ACTION_COUNT = 4;
inline const RebindableAction* GetRebindableActions() {
    static const RebindableAction actions[REBINDABLE_ACTION_COUNT] = {
        { Loc::ActionMoveLeft,  &Settings::keyMoveLeft  },
        { Loc::ActionMoveRight, &Settings::keyMoveRight },
        { Loc::ActionShoot,     &Settings::keyShoot     },
        { "Pause",              &Settings::keyPause     },
    };
    return actions;
}

// Dinh danh loai dich - dung khi can chon ra 1 muc tieu cu the (vd "tien tuyen" ban
// tra) ma khong biet truoc no thuoc pool nao. Day KHONG phai da hinh runtime - chi la
// 1 tag re tien (1 byte, khong vtable) de switch sang dung pool tinh tuong ung.
enum class EnemyKind : uint8_t { Basic, Tanky, Zigzag };

// ==========================================
// GOD OBJECT DA DUOC PHA RA 4 HE THONG DOC LAP:
//   - InputSystem   (input_system.h)   : doc phan cung (ban phim/gamepad), tra ve tin
//                                        hieu hanh dong thuan tuy.
//   - PhysicsSystem (physics_system.h) : di chuyen entity + phat hien/giai quyet va cham.
//   - RenderSystem  (render_system.h)  : ve moi man hinh (Menu/Playing/EndScreen/HUD).
//   - AudioSystem   (audio_system.h)   : sinh & phat am thanh.
//
// GameManager gio CHI con la noi SO HUU du lieu the gioi (pools, player, bunkers...) VA
// DIEU PHOI vong lap chinh (goi he thong nao, theo thu tu nao, luc nao) - khong con tu
// minh tinh toan hinh hoc va cham hay ve pixel nao ca. Nhung gi con lai trong cac ham
// Update*()/InitLevel() cua no la "luat choi" o muc cao (khi nao qua wave, khi nao thua
// cuoc, cong diem the nao) - dieu phoi ket qua tu PhysicsSystem, khong phai tu tinh toan
// va cham.
//
// PhysicsSystem va RenderSystem duoc khai bao `friend` de doc/ghi thang cac field
// private ben duoi (thay vi ep GameManager phai lo hang chuc getter/setter chi de phuc
// vu rieng 2 he thong nay) - xem loi giai thich chi tiet trong physics_system.h va
// render_system.h.
// ==========================================
class PhysicsSystem;
class RenderSystem;

// TEST SEAM - CHI phuc vu tests/game_manager_test_access.h (dung boi tests/test_game_manager.cpp
// va tests/test_physics_system.cpp). Dinh nghia THAT su cua class nay KHONG ton tai o dau trong
// target `space_invaders` (chi trong target `unit_tests`, xem CMakeLists.txt) - forward-declare
// mot class khong bao gio duoc dinh nghia trong build production la hop le trong C++ va khong
// anh huong gi (friend cua 1 class chua tung ton tai don gian khong bao gio duoc dung toi).
//
// TRUOC DAY (xem lich su comment trong tests/test_boss.cpp) du an co chu dich KHONG lam dieu
// nay - ly do la khong he thong nao tung can toi. Gio Track C them GameManagerTestAccess vi
// Track B sap sua UpdatePlaying() (DDA) va boss refactor - can 1 luoi an toan that su goi duoc
// state machine/CheckCollisions() headless truoc khi 2 thay doi rui ro do dung vao, thay vi tiep
// tuc dua vao doc code + chay binary qua Xvfb thu cong. Dung LAI CHINH XAC tinh than "friend thay
// hang chuc getter/setter" da co san cho PhysicsSystem/RenderSystem o tren, khong phai 1 huong di
// moi - xem tests/game_manager_test_access.h de biet chinh xac nhung gi test duoc phep dung toi.
class GameManagerTestAccess;

class GameManager {
    friend class PhysicsSystem;
    friend class RenderSystem;
    friend class GameManagerTestAccess;

private:
    GameState state = GameState::MENU;
    Player player;

    // Xoa Da Hinh: thay std::vector<std::unique_ptr<Enemy>> bang 3 Pool tinh rieng biet
    // theo tung loai dich cu the. Moi mang chi chua 1 kieu du lieu dong nhat -> lap qua
    // hoan toan tuan tu trong bo nho, khong co Cache Miss do nhay theo con tro, khong co
    // vtable indirection khi goi hanh vi rieng.
    EnemyPool<BasicEnemy, Config::MAX_BASIC_ENEMIES> basicEnemies;
    EnemyPool<TankyEnemy, Config::MAX_TANKY_ENEMIES> tankyEnemies;
    EnemyPool<ZigzagEnemy, Config::MAX_ZIGZAG_ENEMIES> zigzagEnemies;

    std::vector<Bunker> bunkers;
    BulletPool<Config::MAX_PLAYER_BULLETS> playerBullets;
    BulletPool<Config::MAX_ENEMY_BULLETS> enemyBullets;
    ParticlePool<Config::MAX_PARTICLES> particles;
    FloatingTextPool<16> floatingTexts; // Popup diem/combo bay len - xem floating_text.h
    PowerUpPool<Config::MAX_POWERUPS> powerUps;
    ScreenShake screenShake;
    HitStop hitStop; // Dong bang logic vai chuc mili-giay khi ha guc dich - xem hit_stop.h + UpdatePlaying()
    AudioSystem audio;
    Leaderboard leaderboard;
    MetaProgress metaProgress;
    int selectedLoadout = 0; // Index dang duoc cycle trong Menu (Q/E) - xem UpdateMenu/DrawLoadoutSelect
    int selectedUpgrade = 0; // Index dang duoc cycle trong man hinh WAVE_CLEAR (Trai/Phai) - xem UpdateEndScreen/DrawUpgradeSelect (Track C Nguoi 2, Phase 3)
    Settings settings;
    SpriteSheet sprites;
    Font gameFont{}; // Tai qua LoadFontEx() trong Run() - Texture Atlas rieng thay the font mac dinh mo cua raylib
    RenderTexture2D renderTarget{}; // Canvas noi bo co dinh SCREEN_W x SCREEN_H, upscale len man hinh that trong Run()
    PostProcess postProcess; // Bloom + CRT ap dung luc upscale renderTarget - xem post_process.h, Config::BLOOM_ENABLED/CRT_ENABLED
    Parallax background;     // Starfield nhieu lop, ve o MOI man hinh (Menu/Playing/EndScreen) truoc switch-case state - xem parallax.h
    LevelGridConfig levelGrid; // Doc tu level.cfg luc Run() - thay cho hardcode r<4,c<10

    // Bam enemy dang song moi frame vao luoi khong gian - PhysicsSystem::CheckCollisions()
    // dung de chi test va cham voi enemy trong cung o thay vi toan bo danh sach. Moi loai
    // dich co 1 SpatialGrid RIENG (khop voi cac Pool tinh) - value luu trong moi grid la
    // index thuan trong dung pool do, khong can dong goi them loai dich vao chung 1 so
    // nguyen.
    SpatialGrid basicGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                            (int)Config::MAX_BASIC_ENEMIES, (int)Config::MAX_BASIC_ENEMIES * 4 };
    SpatialGrid tankyGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                            (int)Config::MAX_TANKY_ENEMIES, (int)Config::MAX_TANKY_ENEMIES * 4 };
    SpatialGrid zigzagGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                             (int)Config::MAX_ZIGZAG_ENEMIES, (int)Config::MAX_ZIGZAG_ENEMIES * 4 };

    Difficulty difficulty = Difficulty::NORMAL;
    float enemySpeed = 50.0f;
    int enemyDirection = 1;
    float enemyFireTimer = 0.0f;
    SubmitResult lastSubmitResult = SubmitResult::NotQualified; // Ket qua nop diem gan nhat - dung de hien banner dung ("Ky luc moi" vs "Lot Top 10")

    // WAVE PROGRESSION: thay vi 1 man choi lap lai y het, moi lan don sach dich thi
    // wave tang len, InitLevel(false) spawn lai voi doi hinh/toc do/nhip ban kho hon,
    // giu nguyen diem/mang cua nguoi choi. Xem InitLevel().
    int wave = 1;
    // BANNER DAU WAVE: dem nguoc tu Config::WAVE_BANNER_DURATION, dat trong InitLevel(),
    // giam trong UpdatePlaying(), doc boi RenderSystem::DrawHUD(). Truoc day wave moi va ca
    // Boss deu bat dau trong im lang tuyet doi - khong co khoanh khac nao danh dau, nhip
    // choi phang li tu wave 1 toi wave 20. KHONG can luu chuoi rieng: RenderSystem tu dung
    // tu `wave` + `isBossWave` + BossTypeName(bossPool[0].type), tranh nhan doi du lieu.
    float waveBannerTimer = 0.0f;
    float waveFireRateMul = 1.0f; // Tinh 1 lan trong InitLevel() theo wave hien tai
    bool isBossWave = false;      // wave % Config::BOSS_WAVE_INTERVAL == 0 -> spawn Boss thay vi luoi doi hinh

    // DYNAMIC DIFFICULTY ADJUSTMENT (DDA) - xem Config::DDA_* (config.h) cho hang so dieu
    // chinh. ddaSpeedMul la he so NHAN THEM vao DifficultyStats::enemySpeedMax/enemyFireRate
    // (doc trong PhysicsSystem::UpdateEnemies() - xem physics_system.cpp - tren 1 BAN SAO
    // cuc bo, KHONG dung vao Config::g_difficultyTable goc). Tinh lai moi CHECKPOINT (moi
    // lan ha Boss, khong phai moi wave thuong) dua tren ddaLivesLostSinceCheck - cong don
    // MOI FRAME trong UpdatePlaying() bang cach so sanh player.GetLives() voi
    // ddaLastKnownLives (KHONG can moc vao tung diem va cham rieng le trong PhysicsSystem::
    // CheckCollisions()/UpdateKamikaze() - 1 diem so sanh DUY NHAT o day gon hon nhieu).
    // Ca 3 field deu KHONG lien quan Difficulty EASY/NORMAL/HARD nguoi choi chon trong
    // Menu - DDA la 1 tang dieu chinh CONG THEM, khong thay the lua chon do.
    float ddaSpeedMul = 1.0f;
    int ddaLivesLostSinceCheck = 0;
    int ddaLastKnownLives = Config::MAX_LIVES;

    // TONG KET RUN (Phase Graphics/UX): 4 so lieu chi ton tai trong 1 van, reset o
    // InitLevel(newGame=true), doc boi RenderSystem::DrawEndScreen() de dung bang tong ket
    // luc Game Over. TRUOC DAY man hinh Game Over chi hien SCORE + WAVE, va dac biet la
    // KHONG he noi nguoi choi vua kiem duoc bao nhieu currency - trong khi AwardCurrency()
    // chay dung tai do. Ca he thong meta-progression (mo khoa loadout 150/400 CR) vi vay
    // vo hinh dung vao khoanh khac no tra thuong.
    int runKills = 0;            // Dem trong ProcessEvents(), moi event co scoreValue > 0 la 1 lan ha guc (gom ca UFO/Boss)
    int runBestCombo = 0;        // Bac combo cao nhat dat duoc, cap nhat trong ApplyComboAndScore()
    int runCurrencyEarned = 0;   // Gia tri TRA VE cua metaProgress.AwardCurrency() - khong tu tinh lai cong thuc quy doi
    float endScreenTimer = 0.0f; // Dem LEN tu 0 khi vao GAME_OVER - dung cho hieu ung chay so cua bang tong ket

    // COMBO SCORE: ha guc lien tiep trong Config::COMBO_WINDOW giay se duoc nhan diem.
    float comboTimer = 0.0f;
    int comboCount = 0;
    // `at` = VI TRI HA GUC (tam cua dich vua chet), dung lam cho spawn popup "+diem/COMBO xN".
    // TRUOC DAY ham nay hardcode player.GetCenter() lam vi tri popup - moi con diem deu bay
    // len tu chinh phi thuyen, nen: (1) nhieu popup cua nhieu don ha guc trong cung 1 khoanh
    // khac chong DE LEN NHAU o dung 1 diem, doc khong ra; (2) mat di thong tin QUAN TRONG
    // nhat cua popup - "vua an diem O DAU". Da xac nhan bang anh chup game that.
    int ApplyComboAndScore(int baseScore, Vector2 at); // Cong diem co nhan combo, tra ve diem thuc nhan

    // MYSTERY SHIP (UFO): dia bay do bay ngang qua dinh man hinh theo chu ky, thuong
    // diem thay doi ngau nhien moi lan spawn. Chi 1 the hien tai 1 thoi diem - khong can
    // pool rieng nhu enemy thuong. Di chuyen/va cham do PhysicsSystem xu ly; GameManager
    // chi giu du lieu + Spawn/RollTimer (quyet dinh "khi nao" la luat choi, khong phai
    // vat ly).
    Rectangle ufoRect{};
    bool ufoActive = false;
    int ufoDirection = 1;
    int ufoScoreValue = 0;
    float ufoSpawnTimer = 0.0f; // Dem nguoc den lan UFO ke tiep (random moi lan)
    void SpawnUfo();
    void RollNextUfoTimer(); // Random lai khoang cho den lan UFO tiep theo

    // KAMIKAZE: Pool + SpatialGrid HOAN TOAN rieng, KHONG spawn trong luoi doi hinh va
    // KHONG tham gia logic hitEdge/activeCount o PhysicsSystem::UpdateEnemies() - spawn
    // doc lap theo chu ky nhu UFO, lao thang mot duong ve vi tri player luc spawn.
    EnemyPool<KamikazeEnemy, Config::MAX_KAMIKAZE> kamikazeEnemies;
    SpatialGrid kamikazeGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                              (int)Config::MAX_KAMIKAZE, (int)Config::MAX_KAMIKAZE * 4 };
    float kamikazeSpawnTimer = 0.0f;
    void SpawnKamikaze();
    void RollNextKamikazeTimer();

    // WARDEN/MEDIC (Phase 1a - Enemy & Item Revolution, Nguoi 1): KHAC Kamikaze o tren -
    // 2 loai nay VAN thuoc luoi doi hinh (spawn qua WaveGenerator/InitLevel() nhu Basic/
    // Tanky/Zigzag, xem ben duoi), chi dat pool/grid o day (canh Kamikaze/truoc Boss) thay
    // vi canh basicEnemies/tankyEnemies/zigzagEnemies o tren, theo dung vi tri da thong
    // nhat o Buoc 0 cua ke hoach chia viec - de gom chung 1 cho voi
    // Weaver/Bomber (Phase 2, cung "thoat luoi" nhu Kamikaze, se them ngay duoi day).
    EnemyPool<WardenEnemy, Config::MAX_WARDEN_ENEMIES> wardenEnemies;
    SpatialGrid wardenGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                            (int)Config::MAX_WARDEN_ENEMIES, (int)Config::MAX_WARDEN_ENEMIES * 4 };
    EnemyPool<MedicEnemy, Config::MAX_MEDIC_ENEMIES> medicEnemies;
    SpatialGrid medicGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                           (int)Config::MAX_MEDIC_ENEMIES, (int)Config::MAX_MEDIC_ENEMIES * 4 };
    // WEAVER/BOMBER (Phase 2 - Enemy & Item Revolution, Nguoi 1): dung khuon Kamikaze o
    // tren (pool+grid+timer rieng, khong dinh gi den WaveGenerator/luoi doi hinh - xem
    // enemy_types.h) - phan spawn (SpawnWeaver/SpawnBomber/RollNext*Timer) dung khuon
    // SpawnUfo/RollNextUfoTimer thay vi SpawnKamikaze (khong "boc" tu doi hinh, luon bay
    // vao tu ngoai man hinh).
    EnemyPool<WeaverEnemy, Config::MAX_WEAVER_ENEMIES> weaverEnemies;
    SpatialGrid weaverGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                            (int)Config::MAX_WEAVER_ENEMIES, (int)Config::MAX_WEAVER_ENEMIES * 4 };
    float weaverSpawnTimer = 0.0f;
    void SpawnWeaver();
    void RollNextWeaverTimer();

    EnemyPool<BomberEnemy, Config::MAX_BOMBER_ENEMIES> bomberEnemies;
    SpatialGrid bomberGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                            (int)Config::MAX_BOMBER_ENEMIES, (int)Config::MAX_BOMBER_ENEMIES * 4 };
    float bomberSpawnTimer = 0.0f;
    void SpawnBomber();
    void RollNextBomberTimer();

    // BOSS: dung CHUNG khuon EnemyPool<T,Capacity> nhu moi loai dich khac (Capacity=1) -
    // KHONG con bool `bossActive` rieng phai giu dong bo tay voi hp: Size()==0 nghia la
    // chua spawn/da bi ha, Size()==1 nghia la con song, dung y het quy uoc "con trong
    // pool = con song" ma BasicEnemy/TankyEnemy/ZigzagEnemy/KamikazeEnemy da dung. Nho
    // do PhysicsSystem::CheckCollisions() xu ly Boss bang CHINH khuon code dung cho
    // Kamikaze, khong can nhanh dieu kien "if (isBossWave)"/"if (bossActive)" nao rieng
    // trong logic va cham nua (xem physics_system.cpp).
    EnemyPool<Boss, 1> bossPool;
    SpatialGrid bossGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f, 1, 16 };
    void SpawnBoss();

    // Fade transition giua cac state, tranh chuyen canh giat cuc
    TransitionPhase transitionPhase = TransitionPhase::NONE;
    float transitionTimer = 0.0f;
    GameState pendingState = GameState::MENU;

    void RequestTransition(GameState next);
    void UpdateTransition(float dt);
    float GetTransitionAlpha() const;

    // ==========================================
    // DIEM VAO DUY NHAT cua GAME OVER - moi duong thua cuoc deu phai di qua day.
    //
    // TRUOC DAY co 2 duong RIENG BIET va lam 2 viec KHAC NHAU: (1) het mang o cuoi
    // UpdatePlaying() - phat am thanh, nop leaderboard, VA cong currency; (2) doi hinh cham
    // day man hinh qua PhysicsSystem::DescendRowAndCheckGameOver() - phat am thanh, nop
    // leaderboard, NHUNG khong cong currency. Bat doi xung nay khong ai de y suot mot thoi
    // gian dai vi no vo hinh; den khi co bang tong ket run thi no hien thang len man hinh
    // ("+0 CR" du vua choi ca mot van), va nguoi dung da chot: duong nao cung duoc CR.
    //
    // Gop lam 1 ham thay vi chep them loi goi AwardCurrency() sang nhanh con lai - de lan
    // sau them 1 duong thua cuoc thu ba (vd het gio) thi no khong the lai quen mat 1 buoc.
    //
    // IDEMPOTENT: `gameOverTriggered` chan kich hoat 2 lan trong CUNG 1 frame. Truong hop
    // that: doi hinh cham day o UpdateEnemies() VA vien dan cuoi cung giet player o
    // CheckCollisions() cung frame do. Luu y RequestTransition() KHONG doi `state` ngay
    // (chi dat pendingState) nen cac guard `if (state != PLAYING) return` trong
    // UpdatePlaying() KHONG chan duoc truong hop nay - neu khong co co nay, currency se
    // duoc cong 2 lan va diem duoc nop leaderboard 2 lan.
    // ==========================================
    bool gameOverTriggered = false;
    void TriggerGameOver();

    void SaveSettings(); // Ghi lai settings.cfg moi khi doi do kho/am luong trong menu/pause

    // Man hinh KEYBIND (vao tu Paused, phim K) - xem GetRebindableActions() o dau file.
    // -1 = dang hien danh sach 4 hanh dong, CHUA cho phim; 0..REBINDABLE_ACTION_COUNT-1
    // = da chon 1 hanh dong, dang CHO nguoi choi bam phim moi cho no.
    int rebindingActionIndex = -1;
    void UpdateKeybindScreen();

    // GOI Y PHIM DIEU KHIEN: dem nguoc tu Config::HUD_HINT_DURATION luc bat dau 1 van MOI
    // (InitLevel(newGame=true)), giam trong UpdatePlaying(). RenderSystem chi ve dong
    // "P: PAUSE  R: RESTART" khi con > 0, mo dan o cuoi. Truoc day dong nay hien VINH VIEN
    // giua dinh man hinh - huong dan cho 10 giay dau nhung o lai suot ca van choi.
    float hintTimer = 0.0f;

    // OBSERVABILITY: bat/tat qua phim F3 (xem InputSystem::PollDebugOverlayToggle),
    // hoat dong o MOI trang thai - Run() tu doc phim nay moi frame thay vi qua
    // UpdateMenu/UpdatePlaying/UpdatePaused (von chi chay dung 1 trong 3 tuy state).
    bool showDebugOverlay = false;

    // newGame=true: van choi hoan toan moi (wave=1, reset diem/mang). newGame=false:
    // sang wave ke tiep trong cung 1 van (giu diem/mang, chi doi hinh/toc do kho hon).
    void InitLevel(bool newGame = true);
    void ApplyLoadoutBonus(); // Ap dung bonus cua loadout dang chon (Vanguard/Overcharge) - goi tu cuoi InitLevel()
    void SpawnBunkers();
    void MaybeDropPowerUp(Vector2 at); // Roll ngau nhien khi 1 dich vua bi ha guc
    void UpdateMenu();
    void UpdateEndScreen();
    void UpdatePaused();
    void UpdatePlaying(float dt);

    // Hang doi hieu ung ma PhysicsSystem::CheckCollisions() ghi nhan trong frame nay -
    // tai dung buffer (clear roi day lai), tranh cap phat lai moi frame. Xem events.h de
    // biet ly do tach rieng buoc nay.
    std::vector<GameEvent> pendingEvents;
    void ProcessEvents();

public:
    void Run();
};
