#pragma once
#include "raylib.h"
#include <vector>
#include <cstdint>
#include "config.h"
#include "bullet_pool.h"
#include "particle_pool.h"
#include "powerup.h"
#include "screen_shake.h"
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
#include "events.h"

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
        { "Trai",  &Settings::keyMoveLeft  },
        { "Phai",  &Settings::keyMoveRight },
        { "Ban",   &Settings::keyShoot     },
        { "Pause", &Settings::keyPause     },
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

class GameManager {
    friend class PhysicsSystem;
    friend class RenderSystem;

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
    PowerUpPool<Config::MAX_POWERUPS> powerUps;
    ScreenShake screenShake;
    AudioSystem audio;
    Leaderboard leaderboard;
    MetaProgress metaProgress;
    int selectedLoadout = 0; // Index dang duoc cycle trong Menu (Q/E) - xem UpdateMenu/DrawLoadoutSelect
    Settings settings;
    SpriteSheet sprites;
    Font gameFont{}; // Tai qua LoadFontEx() trong Run() - Texture Atlas rieng thay the font mac dinh mo cua raylib
    RenderTexture2D renderTarget{}; // Canvas noi bo co dinh SCREEN_W x SCREEN_H, upscale len man hinh that trong Run()
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
    float waveFireRateMul = 1.0f; // Tinh 1 lan trong InitLevel() theo wave hien tai
    bool isBossWave = false;      // wave % Config::BOSS_WAVE_INTERVAL == 0 -> spawn Boss thay vi luoi doi hinh

    // COMBO SCORE: ha guc lien tiep trong Config::COMBO_WINDOW giay se duoc nhan diem.
    float comboTimer = 0.0f;
    int comboCount = 0;
    int ApplyComboAndScore(int baseScore); // Cong diem co nhan combo, tra ve diem thuc nhan

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

    void SaveSettings(); // Ghi lai settings.cfg moi khi doi do kho/am luong trong menu/pause

    // Man hinh KEYBIND (vao tu Paused, phim K) - xem GetRebindableActions() o dau file.
    // -1 = dang hien danh sach 4 hanh dong, CHUA cho phim; 0..REBINDABLE_ACTION_COUNT-1
    // = da chon 1 hanh dong, dang CHO nguoi choi bam phim moi cho no.
    int rebindingActionIndex = -1;
    void UpdateKeybindScreen();

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
