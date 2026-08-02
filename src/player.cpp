#include "player.h"
#include "sprites.h"

Player::Player() { Reset(); }

void Player::Reset() {
    rect = { Config::PLAYER_SPAWN_X, Config::PLAYER_SPAWN_Y, Config::PLAYER_WIDTH, Config::PLAYER_HEIGHT };
    speed = Config::PLAYER_SPEED;
    lives = 3;
    score = 0;
    nextExtraLifeScore = Config::EXTRA_LIFE_SCORE_THRESHOLD;
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

bool Player::AddScore(int points) {
    score += points;
    bool grantedExtraLife = false;
    while (score >= nextExtraLifeScore) {
        nextExtraLifeScore += Config::EXTRA_LIFE_SCORE_THRESHOLD;
        if (lives < Config::MAX_LIVES) {
            lives++;
            grantedExtraLife = true;
        }
    }
    return grantedExtraLife;
}

void Player::Draw(const Texture2D& sprite) const {
    if (invincibleTimer > 0.0f) {
        if (((int)(invincibleTimer * 10) % 2) != 0) return; // Nhap nhay khi bat tu
    }

    // HOAN THIEN: truoc day than tau doi mau theo THU TU UU TIEN Shield > Piercing >
    // RapidFire - neu 2+ power-up active CUNG LUC (hoan toan co the xay ra, cac
    // timer doc lap nhau) thi chi con power-up uu tien cao nhat con "nhin thay duoc",
    // may lai bi che mat. Gio: than tau LUON mau GREEN co dinh (mau goc cua sprite),
    // moi power-up active co 1 pip mau rieng xep hang duoi tau - nhin duoc DUNG TAP
    // HOP nhung gi dang active, khong gioi han chi 1 loai.
    DrawSprite(sprite, rect, GREEN);

    if (HasShield()) {
        // Vong khien bao quanh - giu lai rieng vi no truyen dat y nghia khac voi pip
        // status thuan tuy (khong gian bao ve THAT SU quanh tau, khong chi la 1 nhan).
        Rectangle ring{ rect.x - 4.0f, rect.y - 4.0f, rect.width + 8.0f, rect.height + 8.0f };
        DrawRectangleLinesEx(ring, 2.0f, SKYBLUE);
    }

    struct PipStatus { bool active; Color color; };
    PipStatus pips[] = {
        { HasShield(),    SKYBLUE },
        { HasPiercing(),  MAGENTA },
        { HasRapidFire(), ORANGE  },
    };
    int activeCount = 0;
    for (const auto& status : pips) if (status.active) activeCount++;
    if (activeCount == 0) return;

    constexpr float pipSize = 5.0f;
    constexpr float pipGap = 3.0f;
    float totalWidth = activeCount * pipSize + (activeCount - 1) * pipGap;
    float startX = rect.x + rect.width / 2.0f - totalWidth / 2.0f;
    float pipY = rect.y + rect.height + 4.0f; // Ngay duoi tau - vung nay luon trong man hinh vi player.y co dinh

    int drawn = 0;
    for (const auto& status : pips) {
        if (!status.active) continue;
        DrawRectangle((int)(startX + drawn * (pipSize + pipGap)), (int)pipY, (int)pipSize, (int)pipSize, status.color);
        drawn++;
    }
}
