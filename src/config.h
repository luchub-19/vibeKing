#pragma once
#include <cstddef>

// ==========================================
// CẤU HÌNH TOÀN CỤC
// Gom mọi hằng số vào 1 chỗ — muốn tune số liệu chỉ sửa ở đây,
// không phải mò khắp codebase.
// ==========================================
namespace Config {
    constexpr int SCREEN_W = 800;
    constexpr int SCREEN_H = 600;
    constexpr float MAX_DT = 1.0f / 30.0f; // Chặn dt spike khi lag/kéo cửa sổ

    constexpr float PLAYER_SPEED     = 400.0f;
    constexpr float PLAYER_FIRE_RATE = 0.2f;
    constexpr float INVINCIBLE_TIME  = 1.2f;

    constexpr float ENEMY_SPEED_INC  = 15.0f;
    constexpr float BULLET_SPEED     = 600.0f;
    constexpr float ENEMY_BULLET_SPEED = -300.0f;

    // Kích thước pool cố định — dùng size_t vì đây là non-type template param
    constexpr size_t MAX_PLAYER_BULLETS = 100;
    constexpr size_t MAX_ENEMY_BULLETS  = 500;
    constexpr size_t MAX_PARTICLES      = 400;

    constexpr float TRANSITION_DURATION = 0.25f; // Thời gian fade giữa các state

    inline const char* HighScoreFilePath() { return "highscore.dat"; }
}

// ==========================================
// ĐỘ KHÓ (DATA-DRIVEN)
// Thay vì if/else rải rác mỗi nơi cần biết "đang chơi khó gì",
// gom hết thông số vào 1 bảng tra cứu duy nhất.
// ==========================================
enum class Difficulty { EASY, NORMAL, HARD };

struct DifficultyStats {
    float enemyBaseSpeed;
    float enemySpeedMax;
    float enemyFireRate;
    const char* label;
};

inline DifficultyStats GetDifficultyStats(Difficulty d) {
    switch (d) {
        case Difficulty::EASY:   return { 40.0f, 180.0f, 1.4f, "EASY"   };
        case Difficulty::HARD:   return { 65.0f, 340.0f, 0.6f, "HARD"   };
        case Difficulty::NORMAL:
        default:                 return { 50.0f, 260.0f, 1.0f, "NORMAL" };
    }
}

inline Difficulty CycleDifficulty(Difficulty d, int dir) {
    int v = (int)d + dir;
    if (v < 0) v = 2;
    if (v > 2) v = 0;
    return (Difficulty)v;
}
