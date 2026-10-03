#include "player.h"
#include "draw_helpers.h"
#include "palette.h"
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
    ghostCount = 0;
    visualTilt = 0.0f;   // Vi tri xuat phat moi -> khong mang do nghieng/giat cua frame cuoi wave truoc
    recoilTimer = 0.0f;
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
    ghostCount = 0;
    visualTilt = 0.0f;   // Vi tri xuat phat moi -> khong mang do nghieng/giat cua frame cuoi wave truoc
    recoilTimer = 0.0f;
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

    // Hinh anh: nghieng bam theo input (khong theo van toc that - dung sat mep van giu phim
    // thi van nghieng, dung cam giac "dang ghi lai"). Noi suy mu, khong vuot qua dich.
    float targetTilt = (input.Action_MoveRight ? 1.0f : 0.0f) - (input.Action_MoveLeft ? 1.0f : 0.0f);
    visualTilt += (targetTilt - visualTilt) * fminf(1.0f, TILT_RESPONSE * dt);
    if (recoilTimer > 0.0f) recoilTimer = fmaxf(0.0f, recoilTimer - dt);

    ghostTimer += dt;
    if (ghostTimer >= GHOST_INTERVAL) {
        ghostTimer = 0.0f;
        if (fabsf(visualTilt) > 0.6f) {
            // Day lui: [0] = moi nhat. Them 1 bong moi moc thoi gian khi dang luot.
            for (int i = GHOST_COUNT - 1; i > 0; i--) ghostX[(size_t)i] = ghostX[(size_t)(i - 1)];
            ghostX[0] = rect.x;
            if (ghostCount < GHOST_COUNT) ghostCount++;
        } else if (ghostCount > 0) {
            ghostCount--; // Dung lai -> bong cu nhat tat truoc
        }
    }

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
        recoilTimer = RECOIL_DURATION;
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

void Player::Draw(const Texture2D& sprite, bool reduceFlashing) const {
    float bodyAlpha = 1.0f;
    if (invincibleTimer > 0.0f) {
        // Nhap nhay khi bat tu = an/hien 5 lan/GIAY, vuot nguong 3 lan/giay cua WCAG 2.3.1.
        // Che do giam nhap nhay: mo deu 45% - van doc ra "dang bat tu", khong chop.
        if (reduceFlashing) bodyAlpha = 0.45f;
        else if (((int)(invincibleTimer * 10) % 2) != 0) return;
    }

    // HOAN THIEN: truoc day than tau doi mau theo THU TU UU TIEN Shield > Piercing >
    // RapidFire - neu 2+ power-up active CUNG LUC (hoan toan co the xay ra, cac
    // timer doc lap nhau) thi chi con power-up uu tien cao nhat con "nhin thay duoc",
    // may lai bi che mat. Gio: than tau LUON mau CO DINH theo skin dang chon (skinTint,
    // xem player.h - A7; mac dinh GREEN, giu dung mau goc cua sprite nhu truoc A7), moi
    // power-up active co 1 pip mau rieng xep hang duoi tau - nhin duoc DUNG TAP HOP
    // nhung gi dang active, khong gioi han chi 1 loai, VA khong con lam "mat" mau skin
    // nguoi choi da chon du power-up nao dang active.
    // LUA DAY + QUANG SANG (Phase Graphics): phi thuyen truoc day la 1 sprite phang, khong
    // nguon sang, nen chinh vat the nguoi choi phai bam mat suot tran lai la vat the MO NHAT
    // man hinh (thay ro trong anh chup game that). 2 lop them vao, deu ve o BLEND_ADDITIVE
    // nen chi CONG anh sang, khong che mat sprite:
    //   - Lua day: 3 vach ngan duoi than tau, do dai dao dong theo GetTime() -> tau luon
    //     "dang chay" ke ca khi dung yen.
    //   - Quang sang: 1 hinh chu nhat mo phu len than tau, dua do sang tong the len tren
    //     nguong bloom (Config::BLOOM_THRESHOLD) de post-process TU no lam tau phat sang.
    // Thuan trinh bay - KHONG dung toi rect that (hitbox) o bat ky dau.
    {
        float t = (float)GetTime();
        float flicker = 0.7f + 0.3f * sinf(t * 22.0f);
        float cx = rect.x + rect.width / 2.0f;
        float baseY = rect.y + rect.height;

        BeginBlendMode(BLEND_ADDITIVE);
        Color thrust = Palette::PlayerThrust;
        thrust.a = 200;
        for (int i = -1; i <= 1; i++) {
            float len = (i == 0 ? 9.0f : 5.5f) * flicker;
            float x = cx + (float)i * 7.0f;
            // Duoi lua lech NGUOC huong nghieng - lua "bi gio thoi lai" khi tau luot ngang
            DrawLineEx({ x, baseY }, { x - visualTilt * 4.0f, baseY + len }, 2.5f, thrust);
        }
        // QUANG SANG: DrawCircleGradient (mo dan tu tam ra vien), KHONG phai DrawRectangleRec.
        // Ban dau dung 1 hinh chu nhat mo phu len than tau - ket qua nhin thay ro trong anh
        // chup: no doc ra la 1 CAI HOP xanh co canh cung quanh phi thuyen, xau hon han khong
        // co gi. Gradient tron co do roi mem nen mat doc thanh "anh sang", dung y ban dau.
        Color halo = Palette::PlayerShip;
        halo.a = 70;
        // Chu ky DrawCircleGradient DOI giua 2 ban raylib: 5.5 (ban CI/README ghim) nhan
        // (int centerX, int centerY, ...), con 5.6-dev nhan (Vector2 center, ...). Goi kieu
        // Vector2 tron thi build tren may dev (raylib moi) xanh nhung CI 5.5 do - da xay ra
        // that. Re nhanh theo macro version de 1 source build duoc ca 2.
        Vector2 haloCenter{ cx, rect.y + rect.height / 2.0f };
#if RAYLIB_VERSION_MAJOR > 5 || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 6)
        DrawCircleGradient(haloCenter,
                            rect.width * 0.75f, halo, Fade(Palette::PlayerShip, 0.0f));
#else
        DrawCircleGradient((int)haloCenter.x, (int)haloCenter.y,
                            rect.width * 0.75f, halo, Fade(Palette::PlayerShip, 0.0f));
#endif
        EndBlendMode();
    }

    // Bong mo (sau than tau, additive): [0] gan nhat sang nhat. Bo qua neu dang an trong nhip
    // chop bat tu (bodyAlpha < 1 thi van ve, mo theo).
    if (ghostCount > 0) {
        BeginBlendMode(BLEND_ADDITIVE);
        Rectangle src{ 0.0f, 0.0f, (float)sprite.width, (float)sprite.height };
        for (int i = ghostCount - 1; i >= 0; i--) {
            float a = (0.22f - 0.06f * (float)i) * bodyAlpha;
            Rectangle dst{ ghostX[(size_t)i] + rect.width / 2.0f, rect.y + rect.height / 2.0f, rect.width, rect.height };
            DrawTexturePro(sprite, src, dst, { rect.width / 2.0f, rect.height / 2.0f }, visualTilt * TILT_MAX_DEG, Fade(skinTint, a));
        }
        EndBlendMode();
    }

    // Than tau: giat xuong + bep ngang theo recoil, xoay theo visualTilt quanh TAM. Ve bang
    // DrawTexturePro voi origin = tam (DrawSprite khong xoay duoc).
    {
        float k = recoilTimer / RECOIL_DURATION;
        float w = rect.width * (1.0f + 0.05f * k);
        float h = rect.height * (1.0f - 0.07f * k);
        Rectangle dst{ rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f + RECOIL_PX * k, w, h };
        Rectangle src{ 0.0f, 0.0f, (float)sprite.width, (float)sprite.height };
        DrawTexturePro(sprite, src, dst, { w / 2.0f, h / 2.0f }, visualTilt * TILT_MAX_DEG, Fade(skinTint, bodyAlpha));
    }

    if (HasShield()) {
        // Khien luc giac CHUNG kieu voi khien Sentinel (draw_helpers.h) - cung 1 nghia "dan
        // khong xuyen qua". Truoc day la khung vuong SKYBLUE, khac han khien boss (vong tron).
        float t = (float)GetTime();
        DrawHexShield({ rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f },
                      fmaxf(rect.width, rect.height) * 0.72f, t, -t * 20.0f);
    }

    struct PipStatus { bool active; Color color; };
    PipStatus pips[] = {
        // GD 5: qua Palette:: - khien = mau rao chan (khop vong luc giac), 3 power-up tan cong = 3
        // sac NONG khac nhau (phan thuong), Overdrive = do rui ro. Truoc day SKYBLUE/MAGENTA/ORANGE/
        // GOLD goi thang (MAGENTA khong thuoc dai nao trong luat lanh/nong).
        { HasShield(),     Palette::ShieldBarrier },
        { HasPiercing(),   Palette::UiAccent },
        { HasRapidFire(),  Palette::PlayerThrust },
        { HasSpreadShot(), Palette::ScoreText }, // Phase 1b, Nguoi 1
        { HasOverdrive(),  Palette::EnemyBullet }, // Phase 1b, Nguoi 1 - do = nhac nho rui ro "mat 2 mang" dang active
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
