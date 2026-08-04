#pragma once
#include "raylib.h"
#include "bullet_pool.h"
#include "config.h"
#include "input_system.h"
#include "meta_progress.h"

class Player {
private:
    Rectangle rect;
    float speed;
    int lives;
    int score;
    int nextExtraLifeScore = Config::EXTRA_LIFE_SCORE_THRESHOLD; // Xem AddScore() + Reset()
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

    // Bonus rieng cua tung loadout (xem meta_progress.h), ap dung 1 lan luc bat dau 1 van
    // MOI (goi tu GameManager::ApplyLoadoutBonus, sau khi Reset() da chay). Standard:
    // khong lam gi (giu dung hanh vi hien tai).
    void ApplyStartBonus(LoadoutType type);
    bool HasShield() const { return shieldTimer > 0.0f; }
    bool HasRapidFire() const { return rapidFireTimer > 0.0f; }
    bool HasPiercing() const { return pierceTimer > 0.0f; }

    // Tra ve true neu vua duoc +1 mang tu moc diem so (xem Config::EXTRA_LIFE_SCORE_
    // THRESHOLD) trong lan cong diem NAY - de caller (GameManager::ApplyComboAndScore,
    // noi duy nhat goi ham nay) biet ma phat hieu ung/am thanh "1UP" tuong ung. Player
    // tu no khong dung GameEvent/pendingEvents (khong biet gi ve he thong render/audio).
    bool AddScore(int points);
    // BUG FIX: truoc day Draw() ve DrawRectangleRec() (hinh chu nhat tron), bo qua han
    // sprite "phi thuyen" da duoc SpriteSheet::Load() dung san (xem sprites.cpp::BuildShip)
    // - moi loai dich khac deu co sprite rieng, chi player la bi sot. Nhan tham so
    // sprite tu ngoai truyen vao (giong cach render_system.cpp truyen gm.sprites.X cho
    // dich) thay vi Player tu include SpriteSheet, giu dung huong "Player khong biet gi
    // ve he thong render ngoai chinh no".
    void Draw(const Texture2D& sprite) const;

    Rectangle GetRect() const { return rect; }
    int GetLives() const { return lives; }
    int GetScore() const { return score; }
    float GetY() const { return rect.y; }
    Vector2 GetCenter() const { return { rect.x + rect.width / 2, rect.y + rect.height / 2 }; }
};
