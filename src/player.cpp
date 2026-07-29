#include "player.h"

Player::Player() { Reset(); }

void Player::Reset() {
    rect = { 375.0f, 550.0f, 50.0f, 20.0f };
    speed = Config::PLAYER_SPEED;
    lives = 3;
    score = 0;
    invincibleTimer = 0.0f;
    fireTimer = Config::PLAYER_FIRE_RATE; // Chặn spam đạn đầu game
}

bool Player::Update(float dt, BulletPool<Config::MAX_PLAYER_BULLETS>& bullets) {
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) rect.x += speed * dt;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) rect.x -= speed * dt;
    if (rect.x < 0) rect.x = 0;
    if (rect.x + rect.width > Config::SCREEN_W) rect.x = Config::SCREEN_W - rect.width;

    if (invincibleTimer > 0.0f) invincibleTimer -= dt;
    fireTimer += dt;

    if (IsKeyDown(KEY_SPACE) && fireTimer >= Config::PLAYER_FIRE_RATE) {
        fireTimer = 0.0f;
        bullets.Fire(rect.x + rect.width / 2 - 2.5f, rect.y, Config::BULLET_SPEED);
        return true;
    }
    return false;
}

bool Player::TakeDamage() {
    if (invincibleTimer > 0.0f) return false;
    lives--;
    invincibleTimer = Config::INVINCIBLE_TIME;
    rect.x = 375.0f;
    return true;
}

void Player::AddScore(int points) { score += points; }

void Player::Draw() const {
    if (invincibleTimer > 0.0f) {
        if (((int)(invincibleTimer * 10) % 2) != 0) return; // Nhấp nháy khi bất tử
    }
    DrawRectangleRec(rect, GREEN);
}
