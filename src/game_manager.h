#pragma once
#include "raylib.h"
#include <vector>
#include <cstdint>
#include "config.h"
#include "bullet_pool.h"
#include "particle_pool.h"
#include "screen_shake.h"
#include "audio_manager.h"
#include "high_score.h"
#include "player.h"
#include "enemy_types.h"
#include "level_config.h"
#include "spatial_grid.h"
#include "bunker.h"

enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, WIN };
enum class TransitionPhase { NONE, FADE_OUT, FADE_IN };

// Định danh loại địch - dùng khi cần chọn ra 1 mục tiêu cụ thể (vd "tiền tuyến" bắn
// trả) mà không biết trước nó thuộc pool nào. Đây KHÔNG phải đa hình runtime - chỉ là
// 1 tag rẻ tiền (1 byte, không vtable) để switch sang đúng pool tĩnh tương ứng.
enum class EnemyKind : uint8_t { Basic, Tanky, Zigzag };

class GameManager {
private:
    GameState state = GameState::MENU;
    Player player;

    // Xóa Đa Hình: thay std::vector<std::unique_ptr<Enemy>> bằng 3 Pool tĩnh riêng biệt
    // theo từng loại địch cụ thể. Mỗi mảng chỉ chứa 1 kiểu dữ liệu đồng nhất -> lặp qua
    // hoàn toàn tuần tự trong bộ nhớ, không có Cache Miss do nhảy theo con trỏ, không có
    // vtable indirection khi gọi hành vi riêng.
    EnemyPool<BasicEnemy, Config::MAX_BASIC_ENEMIES> basicEnemies;
    EnemyPool<TankyEnemy, Config::MAX_TANKY_ENEMIES> tankyEnemies;
    EnemyPool<ZigzagEnemy, Config::MAX_ZIGZAG_ENEMIES> zigzagEnemies;

    std::vector<Bunker> bunkers;
    BulletPool<Config::MAX_PLAYER_BULLETS> playerBullets;
    BulletPool<Config::MAX_ENEMY_BULLETS> enemyBullets;
    ParticlePool<Config::MAX_PARTICLES> particles;
    ScreenShake screenShake;
    AudioManager audio;
    HighScore highScore;
    LevelGridConfig levelGrid; // Đọc từ level.cfg lúc Run() - thay cho hardcode r<4,c<10

    // Băm enemy đang sống mỗi frame vào lưới không gian - CheckCollisions() dùng để chỉ
    // test va chạm với enemy trong cùng ô thay vì toàn bộ danh sách. Mỗi loại địch có 1
    // SpatialGrid RIÊNG (khớp với 3 Pool tĩnh) - value lưu trong mỗi grid là index thuần
    // trong đúng pool đó, không cần đóng gói thêm loại địch vào chung 1 số nguyên.
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
    bool newHighScoreThisRun = false;

    // Fade transition giữa các state, tránh chuyển cảnh giật cục
    TransitionPhase transitionPhase = TransitionPhase::NONE;
    float transitionTimer = 0.0f;
    GameState pendingState = GameState::MENU;

    void RequestTransition(GameState next);
    void UpdateTransition(float dt);
    float GetTransitionAlpha() const;

    void InitLevel();
    void SpawnBunkers();
    void UpdateMenu();
    void UpdateEndScreen();
    void UpdatePaused();
    void UpdatePlaying(float dt);
    void UpdateEnemies(float dt);
    void EnemyShoot(float x, float y);
    void CheckCollisions();

    void DrawMenu() const;
    void DrawEndScreen() const;
    void DrawPlaying() const;
    void DrawHUD() const;

public:
    void Run();
};
