#pragma once
#include "raylib.h"
#include "bullet_pool.h"
#include "config.h"

class Player {
private:
    Rectangle rect;
    float speed;
    int lives;
    int score;
    float invincibleTimer;
    float fireTimer;

public:
    Player();
    void Reset();

    // Trả về true nếu vừa bắn ra 1 viên đạn mới trong frame này (để GameManager phát SFX)
    bool Update(float dt, BulletPool<Config::MAX_PLAYER_BULLETS>& bullets);

    // Trả về true nếu damage thực sự được áp dụng (false nếu đang bất tử -> miễn damage)
    bool TakeDamage();

    void AddScore(int points);
    void Draw() const;

    Rectangle GetRect() const { return rect; }
    int GetLives() const { return lives; }
    int GetScore() const { return score; }
    float GetY() const { return rect.y; }
    Vector2 GetCenter() const { return { rect.x + rect.width / 2, rect.y + rect.height / 2 }; }
};
