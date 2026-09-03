#pragma once
#include "raylib.h"
#include "palette.h"
#include <array>
#include "bullet_pool.h"
#include "config.h"
#include "input_system.h"
#include "meta_progress.h"
#include "upgrade_types.h"

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
    // Phase 1b (Enemy & Item Revolution, Nguoi 1) - cung "ho" timer power-up tam thoi o
    // tren, dat canh pierceTimer theo dung vi tri da thong nhat o Buoc 0 cua ke hoach
    // chia viec.
    float spreadShotTimer = 0.0f; // Spread Shot: ban 3 tia toa nhe thay vi 1, xem Update()
    float overdriveTimer = 0.0f;  // Overdrive: tang nhip ban, doi lai trung don luc active mat 2 mang thay vi 1, xem TakeDamage()

    // RUN UPGRADE (Track C - Nguoi 2, Phase 3, xem upgrade_types.h): dem so lan DA CHON
    // moi loai trong VAN hien tai - index thang bang (int)UpgradeType, cung khuon voi
    // g_upgradeTypeDescriptors[]. Ton tai het 1 VAN (Reset() xoa - xem duoi), KHONG phai
    // 1 wave (ResetForNewWave() KHONG dong toi field nay, khac han shieldTimer/rapidFireTimer/
    // pierceTimer o tren - dung y muon, nang cap phai ton tai xuyen suot nhieu wave).
    std::array<int, UPGRADE_TYPE_COUNT> runUpgradeStacks{};

    // A7: mau than tau, mac dinh Palette::PlayerShip (truoc day la GREEN thuan cua raylib).
    // DrawSprite() (sprites.h) da nhan san tham so tint tu truoc - Player chi can giu
    // 1 field de "nho" skin dang chon thay vi hardcode mau thang trong Draw().
    Color skinTint = Palette::PlayerShip;

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
    void GrantSpreadShot(float duration) { spreadShotTimer = duration; } // Phase 1b, Nguoi 1
    void GrantOverdrive(float duration) { overdriveTimer = duration; }   // Phase 1b, Nguoi 1

    // Bonus rieng cua tung loadout (xem meta_progress.h), ap dung 1 lan luc bat dau 1 van
    // MOI (goi tu GameManager::ApplyLoadoutBonus, sau khi Reset() da chay). Standard:
    // khong lam gi (giu dung hanh vi hien tai).
    void ApplyStartBonus(LoadoutType type);

    // RUN UPGRADE (Track C - Nguoi 2, Phase 3): ap dung 1 lan chon nang cap sau wave (xem
    // GameManager::UpdateEndScreen(), state WAVE_CLEAR). Moi loai mutate DUNG 1 field lien
    // quan NGAY TAI DAY (speed/lives/score deu da duoc Update() doc san moi frame o dang
    // hien co - KHONG can sua Update(), giu dung ranh gioi voi vung Nguoi 1 dang sua cho
    // Spread Shot/Overdrive). Goi lap lai CUNG 1 UpgradeType de cong don (vd MoveSpeed nhan
    // don theo tung lan, KHONG cong thang he so) - xem g_upgradeTypeDescriptors trong
    // upgrade_types.h cho gia tri chinh xac tung loai.
    void ApplyRunUpgrade(UpgradeType type);

    // A7: chon mau skin hien thi (xem MetaProgress::SkinType, meta_progress.h). Chua
    // wire vao input/menu trong ticket nay - ha tang thuan tuy, goi ham nay tu noi nao
    // do (menu skin select) se doi mau tau ngay lap tuc trong Draw() ben duoi.
    void SetSkinTint(Color tint) { skinTint = tint; }
    Color GetSkinTint() const { return skinTint; }

    bool HasShield() const { return shieldTimer > 0.0f; }
    bool HasRapidFire() const { return rapidFireTimer > 0.0f; }
    bool HasPiercing() const { return pierceTimer > 0.0f; }
    bool HasSpreadShot() const { return spreadShotTimer > 0.0f; } // Phase 1b, Nguoi 1
    bool HasOverdrive() const { return overdriveTimer > 0.0f; }   // Phase 1b, Nguoi 1

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
    float GetSpeed() const { return speed; } // Track C Nguoi 2 (Phase 3): can cong khai de UI/test doc hieu ung MoveSpeed - truoc day chua co getter nao cho speed
    float GetY() const { return rect.y; }
    Vector2 GetCenter() const { return { rect.x + rect.width / 2, rect.y + rect.height / 2 }; }

    // Track C - Nguoi 2 (Phase 3): so lan DA CHON loai nang cap `type` trong van hien tai -
    // dung cho DrawUpgradeSelect (render_system.cpp) hien "da co may cai" va cho test.
    int GetUpgradeStacks(UpgradeType type) const {
        int idx = (int)type;
        if (idx < 0 || idx >= UPGRADE_TYPE_COUNT) return 0;
        return runUpgradeStacks[idx];
    }
};
