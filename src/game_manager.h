#pragma once
#include "raylib.h"
#include <vector>
#include <cstdint>
#include "config.h"
#include "bullet_pool.h"
#include "particle_pool.h"
#include "powerup.h"
#include "screen_shake.h"
#include "audio_manager.h"
#include "leaderboard.h"
#include "settings.h"
#include "player.h"
#include "enemy_types.h"
#include "level_config.h"
#include "spatial_grid.h"
#include "bunker.h"
#include "sprites.h"

enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, WAVE_CLEAR };
enum class TransitionPhase { NONE, FADE_OUT, FADE_IN };

// Dinh danh loai dich - dung khi can chon ra 1 muc tieu cu the (vd "tien tuyen" ban
// tra) ma khong biet truoc no thuoc pool nao. Day KHONG phai da hinh runtime - chi la
// 1 tag re tien (1 byte, khong vtable) de switch sang dung pool tinh tuong ung.
enum class EnemyKind : uint8_t { Basic, Tanky, Zigzag };

class GameManager {
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
    AudioManager audio;
    Leaderboard leaderboard;
    Settings settings;
    SpriteSheet sprites;
    Font gameFont{}; // Tai qua LoadFontEx() trong Run() - Texture Atlas rieng thay the font mac dinh mo cua raylib
    RenderTexture2D renderTarget{}; // Canvas noi bo co dinh SCREEN_W x SCREEN_H, upscale len man hinh that trong Run()
    LevelGridConfig levelGrid; // Doc tu level.cfg luc Run() - thay cho hardcode r<4,c<10

    // Bam enemy dang song moi frame vao luoi khong gian - CheckCollisions() dung de chi
    // test va cham voi enemy trong cung o thay vi toan bo danh sach. Moi loai dich co 1
    // SpatialGrid RIENG (khop voi cac Pool tinh) - value luu trong moi grid la index
    // thuan trong dung pool do, khong can dong goi them loai dich vao chung 1 so nguyen.
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
    // pool rieng nhu enemy thuong.
    Rectangle ufoRect{};
    bool ufoActive = false;
    int ufoDirection = 1;
    int ufoScoreValue = 0;
    float ufoSpawnTimer = 0.0f; // Dem nguoc den lan UFO ke tiep (random moi lan)
    void UpdateUfo(float dt);
    void SpawnUfo();
    void RollNextUfoTimer(); // Random lai khoang cho den lan UFO tiep theo

    // KAMIKAZE: Pool + SpatialGrid HOAN TOAN rieng, KHONG spawn trong luoi doi hinh va
    // KHONG tham gia logic hitEdge/activeCount o UpdateEnemies() - spawn doc lap theo
    // chu ky nhu UFO, lao thang mot duong ve vi tri player luc spawn.
    EnemyPool<KamikazeEnemy, Config::MAX_KAMIKAZE> kamikazeEnemies;
    SpatialGrid kamikazeGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f,
                              (int)Config::MAX_KAMIKAZE, (int)Config::MAX_KAMIKAZE * 4 };
    float kamikazeSpawnTimer = 0.0f;
    void UpdateKamikaze(float dt);
    void SpawnKamikaze();
    void RollNextKamikazeTimer();

    // BOSS: 1 the hien duy nhat, xuat hien thay the luoi doi hinh vao cac wave la boi so
    // Config::BOSS_WAVE_INTERVAL. Rect lon hon 1 o SpatialGrid (80px) nen bossGrid minh
    // hoa dung tinh nang multi-cell da co san trong SpatialGrid::Insert().
    Boss boss;
    bool bossActive = false;
    SpatialGrid bossGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f, 1, 16 };
    void SpawnBoss();
    void UpdateBoss(float dt);

    // Fade transition giua cac state, tranh chuyen canh giat cuc
    TransitionPhase transitionPhase = TransitionPhase::NONE;
    float transitionTimer = 0.0f;
    GameState pendingState = GameState::MENU;

    void RequestTransition(GameState next);
    void UpdateTransition(float dt);
    float GetTransitionAlpha() const;

    void SaveSettings(); // Ghi lai settings.cfg moi khi doi do kho/am luong trong menu/pause

    // newGame=true: van choi hoan toan moi (wave=1, reset diem/mang). newGame=false:
    // sang wave ke tiep trong cung 1 van (giu diem/mang, chi doi hinh/toc do kho hon).
    void InitLevel(bool newGame = true);
    void SpawnBunkers();
    void MaybeDropPowerUp(Vector2 at); // Roll ngau nhien khi 1 dich vua bi ha guc
    void UpdateMenu();
    void UpdateEndScreen();
    void UpdatePaused();
    void UpdatePlaying(float dt);
    void UpdateEnemies(float dt);

    // Pattern dan dich: thang xuong (kinh dien) hoac nham thang vao vi tri player luc
    // ban - roll ngau nhien theo Config::ENEMY_AIMED_SHOT_CHANCE.
    void EnemyShoot(float x, float y);
    // Toa dan deu 360 do tu 1 diem - dung rieng cho Boss (stage 2/3).
    void FireRadialBurst(float x, float y, int count, float speed);

    void CheckCollisions();

    void DrawMenu() const;
    void DrawEndScreen() const;
    void DrawPlaying() const;
    void DrawHUD() const;

    // Wrapper mong quanh DrawTextEx(gameFont, ...) - giu nguyen chu ky tham so giong
    // DrawText() cu (text, x, y, fontSize, color) de thay the co hoc, khong phai viet
    // lai tung loi goi.
    void DrawGameText(const char* text, int x, int y, int fontSize, Color color) const;

public:
    void Run();
};
