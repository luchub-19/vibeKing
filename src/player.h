#pragma once
#include "raylib.h"
#include "bullet_pool.h"
#include "config.h"
#include "input_system.h"

class Player {
private:
    Rectangle rect;
    float speed;
    int lives;
    int score;
    float invincibleTimer;
    float fireTimer;

    // Power-up tam thoi (xem powerup.h). Tach rieng khoi invincibleTimer vi 2 co che
    // doc lap: invincibleTimer la "vua trung don, cho tho 1 nhip", con shieldTimer la
    // vat pham chu dong nhat duoc, do dung 1 don roi tat bat ke con thoi gian hay khong.
    float shieldTimer = 0.0f;
    float rapidFireTimer = 0.0f;
    float pierceTimer = 0.0f; // Piercing Shot (power-up) - dan ban ra xuyen qua nhieu muc tieu

public:
    Player();

    // Reset toan bo (van choi moi): mang, diem, vi tri, moi power-up.
    void Reset();

    // Reset khi sang wave ke tiep trong CUNG 1 van: giu nguyen mang/diem, chi dua
    // player ve vi tri xuat phat va tat cac power-up/hieu ung tam thoi con sot lai.
    void ResetForNewWave();

    // Tra ve true neu vua ban ra 1 vien dan moi trong frame nay (de GameManager phat SFX).
    // `input` la tin hieu hanh dong da duoc InputSystem quy doi tu phan cung - Player
    // khong con biet gi ve phim/gamepad cu the nao, chi phan ung voi Action_*.
    bool Update(float dt, const InputState& input, BulletPool<Config::MAX_PLAYER_BULLETS>& bullets);

    // Tra ve true neu damage thuc su duoc ap dung (false neu dang bat tu hoac vua
    // dung khien do don -> mien damage)
    bool TakeDamage();

    void GrantShield(float duration) { shieldTimer = duration; }
    void GrantRapidFire(float duration) { rapidFireTimer = duration; }
    void GrantPiercing(float duration) { pierceTimer = duration; }
    bool HasShield() const { return shieldTimer > 0.0f; }
    bool HasRapidFire() const { return rapidFireTimer > 0.0f; }
    bool HasPiercing() const { return pierceTimer > 0.0f; }

    void AddScore(int points);
    void Draw() const;

    Rectangle GetRect() const { return rect; }
    int GetLives() const { return lives; }
    int GetScore() const { return score; }
    float GetY() const { return rect.y; }
    Vector2 GetCenter() const { return { rect.x + rect.width / 2, rect.y + rect.height / 2 }; }
};
