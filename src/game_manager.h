#pragma once
#include "raylib.h"
#include <vector>
#include <memory>
#include "config.h"
#include "bullet_pool.h"
#include "particle_pool.h"
#include "screen_shake.h"
#include "audio_manager.h"
#include "high_score.h"
#include "player.h"
#include "enemy.h"
#include "enemy_types.h"
#include "level_config.h"
#include "spatial_grid.h"
#include "bunker.h"

enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, WIN };
enum class TransitionPhase { NONE, FADE_OUT, FADE_IN };

class GameManager {
private:
    GameState state = GameState::MENU;
    Player player;
    std::vector<std::unique_ptr<Enemy>> enemies; // Đa hình: mỗi phần tử có thể là Basic/Tanky/Zigzag
    std::vector<Bunker> bunkers;
    BulletPool<Config::MAX_PLAYER_BULLETS> playerBullets;
    BulletPool<Config::MAX_ENEMY_BULLETS> enemyBullets;
    ParticlePool<Config::MAX_PARTICLES> particles;
    ScreenShake screenShake;
    AudioManager audio;
    HighScore highScore;
    LevelGridConfig levelGrid; // Đọc từ level.cfg lúc Run() - thay cho hardcode r<4,c<10

    // Băm enemy đang sống mỗi frame vào lưới không gian - CheckCollisions() dùng để
    // chỉ test va chạm với enemy trong cùng ô thay vì toàn bộ danh sách.
    SpatialGrid enemyGrid{ (float)Config::SCREEN_W, (float)Config::SCREEN_H, 80.0f };

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
