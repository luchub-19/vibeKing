#include "player.h"

Player::Player() { Reset(); }

void Player::Reset() {
    rect = { Config::PLAYER_SPAWN_X, Config::PLAYER_SPAWN_Y, Config::PLAYER_WIDTH, Config::PLAYER_HEIGHT };
    speed = Config::PLAYER_SPEED;
    lives = 3;
    score = 0;
    invincibleTimer = 0.0f;
    shieldTimer = 0.0f;
    rapidFireTimer = 0.0f;
    pierceTimer = 0.0f;
    fireTimer = Config::PLAYER_FIRE_RATE; // Chan spam dan dau game
}

void Player::ResetForNewWave() {
    // Giu nguyen lives/score - chi dua ve vi tri xuat phat va tat het hieu ung/power-up
    // tam thoi con sot lai tu wave truoc, tranh mang "khien mien phi" sang wave moi.
    rect.x = Config::PLAYER_SPAWN_X;
    rect.y = Config::PLAYER_SPAWN_Y;
    invincibleTimer = 0.0f;
    shieldTimer = 0.0f;
    rapidFireTimer = 0.0f;
    pierceTimer = 0.0f;
    fireTimer = Config::PLAYER_FIRE_RATE;
}

bool Player::Update(float dt, const InputState& input, BulletPool<Config::MAX_PLAYER_BULLETS>& bullets) {
    // Khong con doc phan cung o day - moi tin hieu da duoc InputManager gop san thanh
    // Action_* truoc khi truyen vao. Player chi con quan tam "co di chuyen/ban khong",
    // khong quan tam no den tu phim nao hay tay cam nao.
    if (input.Action_MoveRight) rect.x += speed * dt;
    if (input.Action_MoveLeft)  rect.x -= speed * dt;
    if (rect.x < 0) rect.x = 0;
    if (rect.x + rect.width > Config::SCREEN_W) rect.x = Config::SCREEN_W - rect.width;

    if (invincibleTimer > 0.0f) invincibleTimer -= dt;
    if (shieldTimer > 0.0f) shieldTimer -= dt;
    if (rapidFireTimer > 0.0f) rapidFireTimer -= dt;
    if (pierceTimer > 0.0f) pierceTimer -= dt;
    fireTimer += dt;

    // Rapid Fire (power-up) rut ngan khoang cach giua 2 phat ban - khong doi toc do
    // dan (Config::BULLET_SPEED), chi doi nhip ban ra.
    float effectiveFireRate = (rapidFireTimer > 0.0f)
        ? Config::PLAYER_FIRE_RATE * Config::POWERUP_RAPIDFIRE_FIRE_RATE_MUL
        : Config::PLAYER_FIRE_RATE;

    if (input.Action_Shoot && fireTimer >= effectiveFireRate) {
        fireTimer = 0.0f;
        int pierceHits = HasPiercing() ? Config::POWERUP_PIERCE_HITS : 0;
        Vector2 vel = { 0.0f, -Config::BULLET_SPEED }; // Y am = bay len (Y+ la xuong duoi)
        bullets.Fire(rect.x + rect.width / 2 - Config::BULLET_WIDTH / 2.0f, rect.y, vel, pierceHits);
        return true;
    }
    return false;
}

bool Player::TakeDamage() {
    if (invincibleTimer > 0.0f) return false;

    if (shieldTimer > 0.0f) {
        // Khien do dung 1 don roi tat ngay - kem 1 nhip bat tu ngan de tranh mat lien
        // 2 mang trong cung 1 frame neu nhieu dan enemy trung gan nhu dong thoi.
        shieldTimer = 0.0f;
        invincibleTimer = Config::PLAYER_SHIELD_HIT_GRACE;
        return false;
    }

    lives--;
    invincibleTimer = Config::INVINCIBLE_TIME;
    rect.x = Config::PLAYER_SPAWN_X;
    return true;
}

void Player::AddScore(int points) { score += points; }

void Player::Draw() const {
    if (invincibleTimer > 0.0f) {
        if (((int)(invincibleTimer * 10) % 2) != 0) return; // Nhap nhay khi bat tu
    }

    Color tint = GREEN;
    if (HasShield()) tint = SKYBLUE;
    else if (HasPiercing()) tint = MAGENTA;
    else if (HasRapidFire()) tint = ORANGE;

    DrawRectangleRec(rect, tint);
    if (HasShield()) {
        // Vong khien bao quanh - phan biet ro voi mau tint don thuan
        Rectangle ring{ rect.x - 4.0f, rect.y - 4.0f, rect.width + 8.0f, rect.height + 8.0f };
        DrawRectangleLinesEx(ring, 2.0f, SKYBLUE);
    }
}
