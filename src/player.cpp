#include "player.h"
#include "sprites.h"
#include <cmath> // sinf/cosf - Spread Shot (Phase 1b, Nguoi 1)

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
    spreadShotTimer = 0.0f; // Phase 1b, Nguoi 1
    overdriveTimer = 0.0f;  // Phase 1b, Nguoi 1
    fireTimer = Config::PLAYER_FIRE_RATE; // Chan spam dan dau game
    runUpgradeStacks.fill(0); // Track C Nguoi 2 (Phase 3): van MOI -> xoa sach nang cap van truoc (speed/lives/score da duoc 3 dong tren tu reset ve mac dinh roi)
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
    spreadShotTimer = 0.0f; // Phase 1b, Nguoi 1
    overdriveTimer = 0.0f;  // Phase 1b, Nguoi 1
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
    if (spreadShotTimer > 0.0f) spreadShotTimer -= dt; // Phase 1b, Nguoi 1
    if (overdriveTimer > 0.0f) overdriveTimer -= dt;   // Phase 1b, Nguoi 1
    fireTimer += dt;

    // Rapid Fire (power-up) rut ngan khoang cach giua 2 phat ban - khong doi toc do
    // dan (Config::BULLET_SPEED), chi doi nhip ban ra. Overdrive (Phase 1b, Nguoi 1)
    // lam DUNG VIEC giong RapidFire (nhan them 1 he so vao PLAYER_FIRE_RATE) nhung la
    // power-up doc lap, co the active CUNG LUC voi RapidFire - nhan don ca 2 he so thay
    // vi chon 1 trong 2, de moi power-up anh huong fire rate deu "cong dong" duoc voi
    // nhau thay vi phai phan uu tien.
    float effectiveFireRate = Config::PLAYER_FIRE_RATE;
    if (rapidFireTimer > 0.0f) effectiveFireRate *= Config::POWERUP_RAPIDFIRE_FIRE_RATE_MUL;
    if (overdriveTimer > 0.0f) effectiveFireRate *= Config::POWERUP_OVERDRIVE_FIRE_RATE_MUL;

    if (input.Action_Shoot && fireTimer >= effectiveFireRate) {
        fireTimer = 0.0f;
        int pierceHits = HasPiercing() ? Config::POWERUP_PIERCE_HITS : 0;
        float spawnX = rect.x + rect.width / 2 - Config::BULLET_WIDTH / 2.0f;

        if (HasSpreadShot()) {
            // Spread Shot (Phase 1b, Nguoi 1): 3 tia tu CUNG 1 diem xuat phat (khong
            // lech ngang) - tia giua giu nguyen huong thang len nhu ban thuong, 2 tia
            // ben lech +-SPREAD_SHOT_ANGLE_DEG do. Ca 3 tia deu ke thua pierceHits nhu
            // nhau - Spread Shot khong "tranh chap" voi Piercing, ca 2 cong dong binh
            // thuong giong moi cap power-up khac trong he thong nay.
            float angleRad = Config::SPREAD_SHOT_ANGLE_DEG * DEG2RAD;
            float sideX = sinf(angleRad) * Config::BULLET_SPEED;
            float sideY = -cosf(angleRad) * Config::BULLET_SPEED;
            bullets.Fire(spawnX, rect.y, { 0.0f, -Config::BULLET_SPEED }, pierceHits);
            bullets.Fire(spawnX, rect.y, { -sideX, sideY }, pierceHits);
            bullets.Fire(spawnX, rect.y, { sideX, sideY }, pierceHits);
        } else {
            Vector2 vel = { 0.0f, -Config::BULLET_SPEED }; // Y am = bay len (Y+ la xuong duoi)
            bullets.Fire(spawnX, rect.y, vel, pierceHits);
        }
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

    // Overdrive (power-up, Phase 1b - Nguoi 1): doi lai fire rate cao hon (xem Update()),
    // trung don luc dang active mat 2 mang thay vi 1. Nhanh Shield/bat tu o tren van chan
    // damage HOAN TOAN nhu cu (Overdrive khong lam gi neu Shield da do don) - chi anh
    // huong so mang tru O DAY, khi damage THAT SU duoc ap dung. Clamp ve 0 (khong am) -
    // GAME_OVER se duoc UpdatePlaying() yeu cau ngay sau do dua tren GetLives()<=0.
    lives -= HasOverdrive() ? 2 : 1;
    if (lives < 0) lives = 0;
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

void Player::ApplyStartBonus(LoadoutType type) {
    switch (type) {
        case LoadoutType::Vanguard:
            // +1 mang luc bat dau van - clamp o MAX_LIVES giong dung quy uoc cua AddScore()
            // (khong de loadout ghi de gioi han an toan da dinh nghia cho so mang toi da).
            if (lives < Config::MAX_LIVES) lives++;
            break;
        case LoadoutType::Overcharge:
            // Bat dau van voi RapidFire active san - dung lai CHINH co che power-up nhat
            // duoc (GrantRapidFire) va CHINH thoi luong power-up do dung (POWERUP_RAPIDFIRE_
            // DURATION), thay vi bay them 1 hang so rieng chi de dung 1 lan.
            GrantRapidFire(Config::POWERUP_RAPIDFIRE_DURATION);
            break;
        case LoadoutType::Standard:
        default:
            break; // Giu dung hanh vi hien tai, khong doi gi ca
    }
}

// RUN UPGRADE (Track C - Nguoi 2, Phase 3) - xem khai bao trong player.h va
// UpgradeTypeDescriptor trong upgrade_types.h. Moi nhanh mutate DUNG 1 field, khong dung
// toi Update() (field lien quan deu da duoc Update() doc "as-is" moi frame o dang hien
// co - speed truc tiep, lives/score qua AddScore() co san). Goi ham nay NHIEU LAN cung 1
// UpgradeType (tu GameManager::UpdateEndScreen()) la cach DUY NHAT nang stack - khong co
// tham so "so luong" rieng, giu dung 1 chu ky ham nhu da chot.
void Player::ApplyRunUpgrade(UpgradeType type) {
    int idx = (int)type;
    if (idx < 0 || idx >= UPGRADE_TYPE_COUNT) return;
    runUpgradeStacks[idx]++;

    const UpgradeTypeDescriptor& desc = GetUpgradeTypeDescriptor(type);
    switch (type) {
        case UpgradeType::MoveSpeed:
            // He so NHAN (khong phai cong them) - moi lan chon nhan them 1 lop len speed
            // HIEN TAI (dung y "cong don" - 3 lan lien tiep = nhan lien 3 lan, khong phai
            // +3*step). Update() da doc `speed` nhu 1 field binh thuong tu truoc, khong
            // can sua gi o do.
            speed *= *desc.coefficient;
            break;
        case UpgradeType::ExtraLife:
            // Dung LAI cap Config::MAX_LIVES co san (giong het nhanh Vanguard trong
            // ApplyStartBonus o tren) - khong hardcode 1 tran rieng cho upgrade nay.
            if (lives < Config::MAX_LIVES) lives++;
            break;
        case UpgradeType::BonusScore:
            // Tai dung AddScore() cong khai - vua tranh nhan doi logic, vua tu dong huong
            // luon co che +1 mang tai moc diem (Config::EXTRA_LIFE_SCORE_THRESHOLD) neu
            // diem thuong vua du day qua 1 moc, khong can code gi them.
            AddScore((int)*desc.coefficient);
            break;
    }
}

void Player::Draw(const Texture2D& sprite) const {
    if (invincibleTimer > 0.0f) {
        if (((int)(invincibleTimer * 10) % 2) != 0) return; // Nhap nhay khi bat tu
    }

    // HOAN THIEN: truoc day than tau doi mau theo THU TU UU TIEN Shield > Piercing >
    // RapidFire - neu 2+ power-up active CUNG LUC (hoan toan co the xay ra, cac
    // timer doc lap nhau) thi chi con power-up uu tien cao nhat con "nhin thay duoc",
    // may lai bi che mat. Gio: than tau LUON mau CO DINH theo skin dang chon (skinTint,
    // xem player.h - A7; mac dinh GREEN, giu dung mau goc cua sprite nhu truoc A7), moi
    // power-up active co 1 pip mau rieng xep hang duoi tau - nhin duoc DUNG TAP HOP
    // nhung gi dang active, khong gioi han chi 1 loai, VA khong con lam "mat" mau skin
    // nguoi choi da chon du power-up nao dang active.
    DrawSprite(sprite, rect, skinTint);

    if (HasShield()) {
        // Vong khien bao quanh - giu lai rieng vi no truyen dat y nghia khac voi pip
        // status thuan tuy (khong gian bao ve THAT SU quanh tau, khong chi la 1 nhan).
        Rectangle ring{ rect.x - 4.0f, rect.y - 4.0f, rect.width + 8.0f, rect.height + 8.0f };
        DrawRectangleLinesEx(ring, 2.0f, SKYBLUE);
    }

    struct PipStatus { bool active; Color color; };
    PipStatus pips[] = {
        { HasShield(),     SKYBLUE },
        { HasPiercing(),   MAGENTA },
        { HasRapidFire(),  ORANGE  },
        { HasSpreadShot(), GOLD    }, // Phase 1b, Nguoi 1
        { HasOverdrive(),  RED     }, // Phase 1b, Nguoi 1 - do = nhac nho rui ro "mat 2 mang" dang active
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
