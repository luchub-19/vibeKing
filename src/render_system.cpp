#include "render_system.h"
#include "draw_helpers.h"
#include "game_manager.h"
#include "ui_system.h"
#include "culling.h"
#include "process_metrics.h"
#include "input_system.h"
#include "meta_progress.h"
#include "localization.h"
#include "upgrade_types.h"
#include "palette.h"
#include "ui_anim.h"
#include "retro_ui.h"

// IDLE ANIMATION (Phase 1 - Graphics/UI Overhaul, Nguoi 1): transform-THUAN quanh tam 1
// Rectangle theo sin(GetTime()) - bob truc Y y het ky thuat DrawTitleLogo() o tren, cong
// them 1 lop pulse ti le RAT nho DONG PHA voi bob (cung 1 sin() - "phinh to nhe dung luc
// nhap len") de sprite co cam giac "song" thay vi dung yen tuyet doi. CHI tra ve Rectangle
// MOI danh rieng cho DrawSprite() - KHONG duoc dung ket qua nay lam rect that (hitbox) cua
// entity, xem tung noi goi trong DrawPlaying() ben duoi (luon giu nguyen `e.rect`/
// `boss.rect` cho va cham/logic, chi doi bien tam o buoc VE).
// CHOP TRANG khi trung don ma chua chet: tron mau goc ve gan TRANG theo `flash` con lai.
// Day la phan hoi "cham vao duoc" re nhat va doc nhanh nhat trong the loai nay - truoc day
// don khong-chi-mang chi co vai hat particle, gan nhu khong thay gi. Chi 3 loai co trang
// thai do (Tanky/Warden/Boss) goi toi, xem Config::HIT_FLASH_DURATION.
static Color HitFlashTint(Color base, float flash) {
    if (flash <= 0.0f) return base;
    float t = fminf(flash / Config::HIT_FLASH_DURATION, 1.0f);
    return Palette::Lerp(base, Color{ 255, 255, 255, base.a }, t);
}

// PHAN UNG TRUNG DON (GD 2) - chi la HINH VE, hitbox (e.rect) khong doi. Dung lai chinh
// hitFlash (dem nguoc tu HIT_FLASH_DURATION ve 0) lam dong ho: k=1 dung luc trung -> 0.
//   - Giat len 3px: dan player bay tu duoi len, dich bi "day" theo huong do (knockback).
//   - Bep ngang/lun doc 8% quanh tam: squash kieu "Juice it or lose it".
// Mau boss theo giai doan - 1 NGUON cho ca sprite boss (DrawPlaying) lan thanh mau tren HUD:
// thanh mau doi mau CUNG LUC voi con boss, mat thay ngay "no vua sang giai doan moi".
static Color BossTint(int stage) {
    return (stage == 1) ? Palette::Boss : (stage == 2) ? Palette::BossEnrage1 : Palette::BossEnrage2; // Cang yeu cang NONG, bao hieu "enrage" (xem palette.h)
}

static Rectangle HitReact(Rectangle r, float flash) {
    if (flash <= 0.0f) return r;
    float k = fminf(flash / Config::HIT_FLASH_DURATION, 1.0f);
    float w = r.width * (1.0f + 0.08f * k);
    float h = r.height * (1.0f - 0.08f * k);
    return { r.x + (r.width - w) / 2.0f, r.y + (r.height - h) / 2.0f - 3.0f * k, w, h };
}

// VACH HP duoi dich nhieu mau (Tanky/Warden) - thay cho khung DrawRectangleLinesEx TRANG cu:
// khung vuong cung lech tong neon (thay ro o anh GD 0) va chi noi "da trung", khong noi "con
// bao nhieu". Mau lay tu chinh mau dich (dai LANH) - vach con = sang hon, vach mat = mo.
// Dat theo hitbox (khong phai drawRect co wobble) vi day la chi bao, khong phai trang tri.
static void DrawHpPips(Rectangle hitbox, int hp, int maxHp, Color base) {
    if (maxHp <= 1 || hp >= maxHp) return; // Nguyen ven -> khong ve, giu doi hinh gon
    constexpr float pipW = 6.0f, pipH = 2.0f, gap = 2.0f;
    float total = (float)maxHp * pipW + (float)(maxHp - 1) * gap;
    float x = hitbox.x + (hitbox.width - total) / 2.0f;
    float y = hitbox.y + hitbox.height + 3.0f;
    Color lit = Palette::Lerp(base, WHITE, 0.55f);
    Color lost = Fade(base, 0.3f);
    for (int i = 0; i < maxHp; i++) {
        DrawRectangleRec({ x + (float)i * (pipW + gap), y, pipW, pipH }, i < hp ? lit : lost);
    }
}

static Rectangle IdleWobble(Rectangle r, float time, float phase, float bobAmp, float bobFreq, float scaleAmp) {
    float s = sinf(time * bobFreq + phase);
    float bob = bobAmp * s;
    float scale = 1.0f + scaleAmp * s;

    float dw = r.width * (scale - 1.0f);
    float dh = r.height * (scale - 1.0f);
    r.x -= dw * 0.5f;      // Phong to/nho QUANH TAM thay vi tu goc tren-trai
    r.y -= dh * 0.5f;
    r.y += bob;
    r.width += dw;
    r.height += dh;
    return r;
}

// Phase 2 (Enemy & Item Revolution, Nguoi 1): 2 HAM VE MOI rieng cho Weaver/Bomber -
// KHONG dong vao vong lap ve Basic/Tanky/Zigzag/Warden/Medic/Kamikaze o trong
// DrawPlaying() (chi them 2 loi goi MOI o do, xem ben duoi) - cung tinh than "khong sua
// ham ve enemy hien co" ma ke hoach chia viec Phase 2 yeu cau.
void RenderSystem::DrawWeaverEnemies(const GameManager& gm, float animTime) {
    for (size_t i = 0; i < gm.weaverEnemies.Size(); i++) {
        const WeaverEnemy& e = gm.weaverEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        // Dung chinh `phase` dao dong CUA Weaver lam luon "hat giong" cho IdleWobble
        // (thay vi tinh rieng tu rect.x nhu Kamikaze) - 2 chuyen dong hoa hop voi nhau
        // thay vi lech pha ngau nhien.
        Rectangle drawRect = IdleWobble(e.rect, animTime, e.phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.weaver, drawRect, e.color);
    }
}

void RenderSystem::DrawBomberEnemies(const GameManager& gm, float animTime) {
    for (size_t i = 0; i < gm.bomberEnemies.Size(); i++) {
        const BomberEnemy& e = gm.bomberEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        // Khong co truong column (giong Kamikaze) - dung vi tri X hien tai lam hat giong
        // pha rieng, cung khuon Kamikaze (khac Weaver o tren dung duoc phase dao dong co
        // san cua chinh no).
        float phase = e.rect.x * Config::ANIM_IDLE_PHASE_STEP / 100.0f;
        Rectangle drawRect = IdleWobble(e.rect, animTime, phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.bomber, drawRect, e.color);
    }
}

void RenderSystem::DrawPlaying(const GameManager& gm) {
    Camera2D cam{};
    // Co do rung TAI DAY (diem DUY NHAT doc offset) thay vi o tung screenShake.Trigger(): moi
    // nguon rung hien tai va sau nay deu tu dong theo cai dat, ke ca muc 0% = dung yen han.
    const float shakeScale = gm.settings.graphics.ShakeScale();
    // Xoay (screen_shake.h) phai quanh TAM man hinh: Camera2D xoay quanh `target` (dat tai
    // `offset` tren man hinh). target={0,0} nhu cu thi khung hinh xoay quanh goc tren-trai -
    // goc duoi-phai vang ra hang chuc px. Dat target = tam, offset = tam + rung.
    const Vector2 screenCenter{ Config::SCREEN_W / 2.0f, Config::SCREEN_H / 2.0f };
    cam.offset = { screenCenter.x + gm.screenShake.GetOffset().x * shakeScale,
                   screenCenter.y + gm.screenShake.GetOffset().y * shakeScale };
    cam.target = screenCenter;
    cam.rotation = gm.screenShake.GetRotation() * shakeScale;
    cam.zoom = 1.0f;

    BeginMode2D(cam);
    // CULLING: bo qua lenh ve GPU cho bat ky thuc the nao nam hoan toan ngoai vung nhin
    // camera (xem culling.h) - Basic/Tanky/Zigzag hau nhu luon o trong man hinh (bi
    // chan boi logic hitEdge trong PhysicsSystem) nen kiem tra o day chi la 1 phep so
    // sanh AABB re, khong danh doi hieu nang de co loi ich; Kamikaze/UFO/Boss moi la
    // nhung thuc the thuc su co the dung ngoai man hinh 1 khoang thoi gian dang ke.
    // IDLE ANIMATION: 1 lan GetTime() dung chung cho ca frame (xem IdleWobble() o tren) -
    // tranh goi lai nhieu lan khong can thiet cho tung thuc the rieng le.
    float animTime = (float)GetTime();

    for (size_t i = 0; i < gm.basicEnemies.Size(); i++) {
        const BasicEnemy& e = gm.basicEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        float phase = (float)e.column * Config::ANIM_IDLE_PHASE_STEP;
        Rectangle drawRect = IdleWobble(e.rect, animTime, phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.basicAlien, drawRect, e.color);
    }
    for (size_t i = 0; i < gm.tankyEnemies.Size(); i++) {
        const TankyEnemy& e = gm.tankyEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        float phase = (float)e.column * Config::ANIM_IDLE_PHASE_STEP;
        Rectangle drawRect = IdleWobble(e.rect, animTime, phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.tankyAlien, HitReact(drawRect, e.hitFlash), HitFlashTint(e.color, e.hitFlash));
        DrawHpPips(e.rect, e.hp, TankyEnemy::HP, e.color);
    }
    for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++) {
        const ZigzagEnemy& e = gm.zigzagEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        float phase = (float)e.column * Config::ANIM_IDLE_PHASE_STEP;
        Rectangle drawRect = IdleWobble(e.rect, animTime, phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.zigzagAlien, drawRect, e.color);
    }
    // WARDEN/MEDIC (Phase 1a - Enemy & Item Revolution, Nguoi 1): van dung `column` cho
    // phase idle-wobble giong Basic/Tanky/Zigzag (van thuoc doi hinh luoi, khac Kamikaze
    // ben duoi) - xem PhysicsSystem::UpdateWardenEnemies()/UpdateMedicEnemies().
    for (size_t i = 0; i < gm.wardenEnemies.Size(); i++) {
        const WardenEnemy& e = gm.wardenEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        float phase = (float)e.column * Config::ANIM_IDLE_PHASE_STEP;
        Rectangle drawRect = IdleWobble(e.rect, animTime, phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.warden, HitReact(drawRect, e.hitFlash), HitFlashTint(e.color, e.hitFlash));
        DrawHpPips(e.rect, e.hp, WardenEnemy::HP, e.color);
    }
    for (size_t i = 0; i < gm.medicEnemies.Size(); i++) {
        const MedicEnemy& e = gm.medicEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        float phase = (float)e.column * Config::ANIM_IDLE_PHASE_STEP;
        Rectangle drawRect = IdleWobble(e.rect, animTime, phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.medic, drawRect, e.color);
    }
    for (size_t i = 0; i < gm.kamikazeEnemies.Size(); i++) {
        const KamikazeEnemy& e = gm.kamikazeEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        // Khong co truong `column` (khong thuoc doi hinh luoi, xem enemy_types.h) - dung
        // vi tri X hien tai lam "hat giong" pha rieng thay the, du it y nghia hon vi so
        // luong dong thoi thuong chi 1-2 con (xem GameManager::SpawnKamikaze).
        float phase = e.rect.x * Config::ANIM_IDLE_PHASE_STEP / 100.0f;
        Rectangle drawRect = IdleWobble(e.rect, animTime, phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.kamikaze, drawRect, e.color);
    }
    DrawWeaverEnemies(gm, animTime); // Phase 2, Nguoi 1
    DrawBomberEnemies(gm, animTime); // Phase 2, Nguoi 1
    if (gm.ufoActive && Culling::IsVisible(gm.ufoRect)) {
        // Chi 1 UFO ton tai cung luc (xem gm.ufoActive) - khong can lech pha rieng (phase=0).
        Rectangle drawRect = IdleWobble(gm.ufoRect, animTime, 0.0f, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.ufo, drawRect, Palette::Ufo);
    }
    // BOSS: cung 1 kieu Pool nhu moi loai dich khac (EnemyPool<Boss,1>) - Size()>0 nghia
    // la con song, khong con co Bool `bossActive` rieng phai giu dong bo thu cong.
    if (gm.bossPool.Size() > 0) {
        const Boss& boss = gm.bossPool[0];
        if (Culling::IsVisible(boss.rect)) {
            int stage = BossStage(boss);
            Color tint = BossTint(stage);

            const Texture2D& tex = (boss.type == BossType::Sentinel) ? gm.sprites.bossSentinel
                                  : (boss.type == BossType::Swarmer) ? gm.sprites.bossSwarmer
                                  : gm.sprites.boss;
            // Bo hang so RIENG (ANIM_BOSS_IDLE_*, xem config.h) - than lon hon han dich
            // thuong nen cung bien do px se de nhan thay hon; chi 1 Boss ton tai cung luc
            // nen khong can lech pha (phase=0), khac Basic/Tanky/Zigzag o tren.
            Rectangle drawRect = IdleWobble(boss.rect, animTime, 0.0f, Config::ANIM_BOSS_IDLE_BOB_AMPLITUDE,
                                             Config::ANIM_BOSS_IDLE_BOB_FREQUENCY, Config::ANIM_BOSS_IDLE_SCALE_AMPLITUDE);
            DrawSprite(tex, HitReact(drawRect, boss.hitFlash), HitFlashTint(tint, boss.hitFlash));

            // VONG KHIEN: chi ve khi Sentinel dang bat kha xam pham - vien tron xanh bao
            // quanh toan bo rect, bao hieu ro rang "dan khong an thua luc nay" (khop voi
            // logic mien sat thuong trong PhysicsSystem::CheckCollisions()). Dung
            // boss.rect GOC (khong phai drawRect) vi day la chi bao gan voi vung mien sat
            // thuong THAT, khong phai trang tri.
            if (boss.type == BossType::Sentinel && boss.shieldActive) {
                float radius = fmaxf(boss.rect.width, boss.rect.height) * 0.62f;
                DrawHexShield(EnemyCenter(boss.rect), radius, animTime, animTime * 12.0f);
            }
        }
    }
    for (const auto& bunker : gm.bunkers) bunker.Draw();
    // THU TU LOP (luat R1, docs/GRAPHICS_UPGRADE_PLAN.md): particle -> quang dan (additive,
    // PHIA SAU) -> power-up -> loi dan player -> loi dan dich TREN CUNG trong the gioi game ->
    // player. Truoc day particle ve SAU dan nen 1 cum no co the de len dung vien dan dang lao
    // toi - nguy hiem nhat lai bi che boi thu vo hai nhat.
    gm.particles.Draw();
    gm.floatingTexts.Draw(gm.gameFont); // Chu diem/combo cung la hieu ung -> DUOI loi dan
    gm.playerBullets.DrawGlows(Palette::PlayerBullet);
    gm.enemyBullets.DrawGlows(Palette::EnemyBullet);

    // POWER-UP: icon rieng theo tung loai (xem PowerUpType trong powerup.h + 4 ham
    // BuildIcon*() trong sprites.cpp) thay vi hinh chu nhat mau tron - cung khuon chon
    // texture theo loai nhu khoi Boss o tren. KHONG culling (xem culling.h: PowerUp tu
    // huy ngay khi vuot bien man hinh nen luon o gan/trong man hinh suot vong doi active,
    // kiem tra o day chi la chi phi thua, khong loai duoc lenh ve nao ca).
    for (size_t i = 0; i < gm.powerUps.Size(); i++) {
        const PowerUp& p = gm.powerUps[i];
        Texture2D tex;
        // Gan 1 LAN o day, KHONG theo tung case: commit ae951c4 xoa cac vế `tint = ORANGE/...`
        // trong switch nhung quen gan lai -> bien chua khoi tao, power-up ve bang mau rac cua
        // stack (UB, khong test nao bat duoc vi day la ham ve thuan).
        const Color tint = Palette::PowerUp;
        switch (p.type) {
            // MAU: moi power-up ROI TREN MAT DAT deu dung Palette::PowerUp (dai NONG) - "co
            // thu de nhat" la thong tin quan trong nhat luc no dang roi, phan biet LOAI nao
            // thi doc bang HINH icon. Truoc day moi loai 1 mau rieng (ORANGE/SKYBLUE/MAGENTA/
            // LIME/GOLD/RED) - vua pha luat lanh/nong (SKYBLUE/LIME lan sang dai lanh cua dich)
            // vua khien pickup khong co dau hieu thi giac CHUNG nao de nhan ra tu xa.
            case PowerUpType::RapidFire:  tex = gm.sprites.iconRapidFire;  break;
            case PowerUpType::Shield:     tex = gm.sprites.iconShield;     break;
            case PowerUpType::Piercing:   tex = gm.sprites.iconPiercing;   break;
            case PowerUpType::Cleanser:   tex = gm.sprites.iconCleanser;   break;
            case PowerUpType::SpreadShot: tex = gm.sprites.iconSpreadShot; break; // Phase 1b, Nguoi 1
            case PowerUpType::Overdrive:  tex = gm.sprites.iconOverdrive;  break; // Phase 1b, Nguoi 1
            default:                      tex = gm.sprites.iconRapidFire;  break;
        }
        DrawSprite(tex, p.rect, tint);
    }

    gm.playerBullets.DrawCores(Palette::PlayerBullet);
    gm.enemyBullets.DrawCores(Palette::EnemyBullet, animTime, !gm.settings.graphics.reduceFlashing);
    gm.player.Draw(gm.sprites.player, gm.settings.graphics.reduceFlashing);
    EndMode2D();

    // HUD ve ngoai camera de khong bi rung theo
    DrawHUD(gm);

    // Hitbox (Cai dat > Tro nang): vien dung vung va cham THAT cua tau (player.GetRect() - cung
    // rect CheckCollisions() dung), ve ngoai camera rung de nguoi choi doc vi tri chinh xac.
    if (gm.settings.showHitbox) {
        Rectangle hb = gm.player.GetRect();
        DrawRectangleLinesEx(hb, 1.0f, Fade(Palette::UiText, 0.85f));
        DrawRectangleRec({ hb.x + hb.width / 2.0f - 1.0f, hb.y + hb.height / 2.0f - 1.0f, 2.0f, 2.0f }, Palette::UiText);
    }
    if (gm.state == GameState::PAUSED) DrawPauseMenu(gm);
}

// Ten hien thi (da dich) cua loai boss - BossTypeName() (enemy_types.h) giu ten tieng Anh noi bo
// cho test/log; man hinh doc qua day.
static const char* BossDisplayName(BossType t) {
    switch (t) {
        case BossType::Sentinel: return Tr(Str::BossSentinel);
        case BossType::Swarmer:  return Tr(Str::BossSwarmer);
        default:                 return Tr(Str::BossVanguard);
    }
}

// ==========================================
// HUD ARCADE (nang cap GUI) - bo cuc kieu bang diem may Taito/Namco: nhan nho mau NONG o tren,
// so lon 6 chu so co so 0 dau ben duoi ("SCORE<1>  HI-SCORE"). So 0 dau khong phai trang tri: do
// rong co dinh -> so khong "nhay" ngang khi diem tang them 1 chu so.
//   Trai : DIEM | CAO (diem cao nhat bang xep hang - biet ngay con cach ky luc bao xa)
//   Phai : DOT  | tau mini = mang
//   Duoi phai: o power-up co VACH DEM NGUOC (truoc day chi co icon - khong biet sap het hay chua)
// Van giu trong dai Config::HUD_TOP_BAND_H (khong de len hang dich dau - xem lich su bug o duoi).
// ==========================================
void RenderSystem::DrawHUD(const GameManager& gm) {
    UICanvas canvas;
    Color panelFill = Palette::UiPanelFill;
    panelFill.a = (unsigned char)(255.0f * Config::HUD_PANEL_ALPHA);
    const Color panelBorder = Palette::UiPanelEdge;
    const Color corner = Fade(Palette::UiAccent, 0.55f);
    const float bandH = Config::HUD_TOP_BAND_H;
    const float time = (float)GetTime();

    // --- Trai: DIEM + CAO. Panel rong toi da 234px (x 6..240): thanh mau boss giua man bat dau o
    // x=240 - rong hon la de len nhau o wave boss. Panel cao HUD_TOP_BAND_H: panel cu 80px tung de
    // len hang dich tren cung (y=50) suot ca van.
    canvas.FramedPanel({ 6.0f, 6.0f, 228.0f, bandH }, panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS, corner);
    const bool rolling = (int)gm.hudScoreShown != gm.player.GetScore();
    const int hi = gm.leaderboard.GetTopScore() > gm.player.GetScore() ? gm.leaderboard.GetTopScore() : gm.player.GetScore();
    canvas.Text(14, 6, 15, Palette::ScoreText, Tr(Str::HudScore));
    canvas.Text(14, 15, 24, rolling ? Palette::ScoreText : Palette::UiText, TextFormat("%06d", (int)gm.hudScoreShown));
    canvas.Text(122, 6, 15, Palette::UiDim, Tr(Str::HudHi));
    canvas.Text(122, 15, 24, Palette::UiDim, TextFormat("%06d", hi));
    if (gm.comboCount > 1) {
        // Combo: so NONG nhun nhe theo nhip (chu "song") - cung mau voi popup diem
        float bob = gm.settings.graphics.reduceFlashing ? 0.0f : sinf(time * 10.0f) * 1.0f;
        canvas.Text(196, (int)(13.0f + bob), 22, Palette::ScoreText, TextFormat("x%d", gm.comboCount));
    }
    if (gm.runAssisted) canvas.Text(14, 42, 15, Fade(Palette::UiDim, 0.9f), Tr(Str::AssistTag));

    // GOI Y PHIM: chi hien Config::HUD_HINT_DURATION giay dau cua 1 van MOI roi mo dan tat
    // (xem GameManager::hintTimer). An luc Boss active - panel Boss chiem dung vung ngang nay.
    if (gm.bossPool.Size() == 0 && gm.hintTimer > 0.0f) {
        float alpha = (gm.hintTimer < Config::HUD_HINT_FADE) ? (gm.hintTimer / Config::HUD_HINT_FADE) : 1.0f;
        canvas.CenteredText(Config::SCREEN_W / 2, 14, 19, Fade(Palette::UiDim, alpha),
                            TextFormat(Tr(Str::HudHintFmt), InputSystem::KeyName(gm.settings.keyPause)));
    }

    // --- Phai: DOT + mang (tau mini - dem bang mat nhanh hon doc so, khong can nhan) ---
    const float rightX = (float)Config::SCREEN_W - 194.0f;
    canvas.FramedPanel({ rightX, 6.0f, 188.0f, bandH }, panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS, corner);
    canvas.Text((int)rightX + 8, 6, 15, Palette::UiAccent, Tr(Str::ColWave));
    {
        const int lives = gm.player.GetLives();
        const float iconW = 16.0f, iconH = 12.0f, gap = 3.0f;
        const float shipsX = rightX + 92.0f;
        if (lives <= 5) {
            for (int i = 0; i < lives; i++) {
                canvas.Icon({ shipsX + (float)i * (iconW + gap), 16.0f, iconW, iconH }, gm.sprites.player, Palette::PlayerShip);
            }
        } else {
            canvas.Text((int)shipsX, 11, 22, Palette::UiText, TextFormat("x%d", lives));
        }
        // 1 mang cuoi: vien do nhip tho cham (khong nhap nhay gat) - "chi con 1 lan duoc sai"
        if (lives == 1) {
            float a = gm.settings.graphics.reduceFlashing ? 0.6f : 0.35f + 0.3f * sinf(time * 4.0f);
            canvas.Panel({ rightX + 86.0f, 10.0f, 30.0f, bandH - 8.0f }, Color{ 0, 0, 0, 0 }, Fade(Palette::UiDanger, a), 1.0f);
        }
    }
    canvas.Text((int)rightX + 8, 15, 24, Palette::UiText, TextFormat("%02d", gm.wave));

    // --- Power-up: o CO DINH theo thu tu (khong dich trai lap cho trong) + vach dem nguoc ---
    struct Slot { PowerUpType type; const Texture2D* tex; Color tint; float duration; };
    const Slot slots[5] = {
        { PowerUpType::Shield,     &gm.sprites.iconShield,     Palette::ShieldBarrier, Config::POWERUP_SHIELD_DURATION },
        { PowerUpType::RapidFire,  &gm.sprites.iconRapidFire,  Palette::PowerUp,       Config::POWERUP_RAPIDFIRE_DURATION },
        { PowerUpType::Piercing,   &gm.sprites.iconPiercing,   Palette::PowerUp,       Config::POWERUP_PIERCE_DURATION },
        { PowerUpType::SpreadShot, &gm.sprites.iconSpreadShot, Palette::PowerUp,       Config::POWERUP_SPREADSHOT_DURATION },
        { PowerUpType::Overdrive,  &gm.sprites.iconOverdrive,  Palette::EnemyBullet,   Config::POWERUP_OVERDRIVE_DURATION }, // Do = rui ro mat 2 mang
    };
    bool anyActive = false;
    for (const Slot& sl : slots) anyActive = anyActive || gm.player.PowerUpTimeLeft(sl.type) > 0.0f;
    if (anyActive) {
        const float slot = Config::HUD_ICON_SIZE + 4.0f;
        const float iconX = (float)Config::SCREEN_W - 121.0f, iconY = 46.0f;
        canvas.FramedPanel({ (float)Config::SCREEN_W - 130.0f, 40.0f, 124.0f, Config::HUD_ICON_SIZE + 16.0f },
                           panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS, corner);
        for (int i = 0; i < 5; i++) {
            float left = gm.player.PowerUpTimeLeft(slots[i].type);
            if (left <= 0.0f) continue;
            float x = iconX + slot * (float)i;
            float ratio = slots[i].duration > 0.0f ? left / slots[i].duration : 0.0f;
            // 1.5s cuoi: icon mo/sang theo nhip - bao "sap het" (nhip cham, khong phai chop gat)
            Color tint = slots[i].tint;
            if (left < 1.5f && !gm.settings.graphics.reduceFlashing) tint = Fade(tint, 0.45f + 0.55f * (0.5f + 0.5f * sinf(time * 9.0f)));
            canvas.Icon({ x, iconY, Config::HUD_ICON_SIZE, Config::HUD_ICON_SIZE }, *slots[i].tex, tint);
            canvas.Bar({ x, iconY + Config::HUD_ICON_SIZE + 2.0f, Config::HUD_ICON_SIZE, 3.0f }, ratio,
                       Fade(Palette::UiPanelEdge, 0.6f), slots[i].tint, Color{ 0, 0, 0, 0 });
        }
    }

    if (gm.bossPool.Size() > 0) {
        const Boss& boss = gm.bossPool[0];
        float barW = 300.0f;
        float ratio = (boss.maxHp > 0) ? ((float)boss.hp / (float)boss.maxHp) : 0.0f;
        float barX = (Config::SCREEN_W - barW) / 2.0f;
        // GD 5: mau thanh = mau boss theo giai doan (BossTint, cung nguon voi sprite), vach tai 2
        // nguong BOSS_STAGE*_RATIO (cho boss doi hanh vi), vet trang mo = sat thuong vua gay.
        Color barFill = (boss.type == BossType::Sentinel && boss.shieldActive) ? Palette::ShieldBarrier : BossTint(BossStage(boss));
        canvas.FramedPanel({ barX - 10.0f, 4.0f, barW + 20.0f, 36.0f }, panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS, corner);
        canvas.TrailBar({ barX, 8.0f, barW, 14.0f }, ratio, gm.hudBossTrail, Palette::UiPanelFill, barFill,
                        Fade(Palette::UiText, 0.55f), Palette::UiPanelEdge, BOSS_STAGE3_RATIO, BOSS_STAGE2_RATIO);
        canvas.CenteredText((int)(barX + barW / 2.0f), 22, 18, barFill, TextFormat(Tr(Str::HudBossFmt), BossDisplayName(boss.type)));
        if (boss.type == BossType::Sentinel && boss.shieldActive) {
            canvas.Text((int)(barX + barW - 56.0f), 22, 18, Palette::ShieldBarrier, Tr(Str::ShieldTag));
        }
    }

    canvas.Draw(gm.gameFont);
    DrawWaveBanner(gm); // Ve thang (font tieu de + hinh khoi) -> phai SAU canvas de nam tren cung
}

// ==========================================
// BANNER DAU WAVE (GD 5) - ve SAU cung trong HUD nen luon nam tren moi thu khac. Wave boss dung
// chinh ten loai boss (BossTypeName - 1 nguon voi thanh mau) de nguoi choi biet ngay sap gap co
// che nao.
//   - Wave thuong: "WAVE n" neon (titleFont) + 2 vach mong phong ra tu tam (0.35s ease-out).
//   - Wave boss: dai CANH BAO soc cheo do-den troi ngang + "WARNING" + ten boss - kieu arcade
//     (Galaga/Ikaruga). Do = nguy hiem tuc thi theo luat lanh/nong. Soc TROI deu, khong nhap
//     nhay -> khong dung toi reduceFlashing.
// ==========================================
void RenderSystem::DrawWaveBanner(const GameManager& gm) {
    // Chi khi dang CHOI: pause ngay dau wave thi timer dung nhung banner van con -> vach banner cat
    // ngang chu PAUSED (thay o anh chup). Timer dung nen banner hien tiep sau khi tiep tuc.
    if (gm.waveBannerTimer <= 0.0f || gm.state != GameState::PLAYING) return;
    const float alpha = (gm.waveBannerTimer < Config::WAVE_BANNER_FADE)
                          ? (gm.waveBannerTimer / Config::WAVE_BANNER_FADE) : 1.0f;
    const float elapsed = Config::WAVE_BANNER_DURATION - gm.waveBannerTimer;
    const float cx = Config::SCREEN_W / 2.0f;
    const float open = EaseOutCubic(elapsed, 0.35f);

    if (gm.isBossWave && gm.bossPool.Size() > 0) {
        const float bandY = 236.0f, bandH = 74.0f;
        // Nen dai + 2 vien soc cheo (scissor de soc khong tran ra ngoai vien)
        DrawRectangleRec({ 0.0f, bandY, (float)Config::SCREEN_W, bandH }, Fade(Palette::Background, 0.82f * alpha));
        const float stripeH = 9.0f, stripeW = 18.0f, period = 36.0f;
        const float scroll = fmodf(elapsed * 60.0f, period);
        const Color stripe = Fade(Palette::BossEnrage2, 0.85f * alpha);
        for (int edge = 0; edge < 2; edge++) {
            const float y = (edge == 0) ? bandY : bandY + bandH - stripeH;
            BeginScissorMode(0, (int)y, Config::SCREEN_W, (int)stripeH);
            for (float x = -period + (edge == 0 ? scroll : -scroll); x < Config::SCREEN_W + period; x += period) {
                // Hinh binh hanh nghieng 45 do = 2 tam giac (thu tu dinh nguoc chieu kim dong ho cho raylib)
                Vector2 a{ x, y + stripeH }, b{ x + stripeW, y + stripeH }, c{ x + stripeW + stripeH, y }, d{ x + stripeH, y };
                DrawTriangle(a, b, c, stripe);
                DrawTriangle(a, c, d, stripe);
            }
            EndScissorMode();
        }
        // Chu mo rong dan (open) - "WARNING" hien ra tu tam
        DrawNeonText(gm.titleFont, Tr(Str::BossWarning), { cx, bandY + 30.0f }, 34.0f + 6.0f * (1.0f - open), Palette::BossEnrage2, alpha);
        const char* name = TextFormat("%s - %s", BossDisplayName(gm.bossPool[0].type), Tr(Str::BossIncomingHint));
        Vector2 sz = MeasureTextEx(gm.gameFont, name, 21.0f, 1.0f);
        DrawTextEx(gm.gameFont, name, { cx - sz.x / 2.0f, bandY + bandH - stripeH - 24.0f }, 21.0f, 1.0f, Fade(Palette::UiText, alpha));
    } else {
        const char* label = TextFormat(Tr(Str::WaveBannerFmt), gm.wave);
        DrawNeonText(gm.titleFont, label, { cx, 262.0f }, 44.0f, Palette::UiAccent, alpha);
        const float half = 230.0f * open;
        const Color line = Fade(Palette::UiAccent, 0.6f * alpha);
        DrawRectangleRec({ cx - half, 292.0f, half * 2.0f, 2.0f }, line);
        DrawRectangleRec({ cx - half * 0.6f, 232.0f, half * 1.2f, 1.0f }, Fade(line, 0.5f * alpha));
    }
}

void RenderSystem::DrawTransitionWipe(float alpha) {
    if (alpha <= 0.0f) return;
    constexpr int BANDS = 12;
    const float bandH = (float)Config::SCREEN_H / (float)BANDS;
    for (int i = 0; i < BANDS; i++) {
        float cover = WipeBandCoverage(alpha, i, BANDS);
        if (cover <= 0.0f) continue;
        float h = bandH * cover;
        float y = (float)i * bandH + (bandH - h) / 2.0f; // Dong tu GIUA dai ra 2 mep
        DrawRectangleRec({ 0.0f, y, (float)Config::SCREEN_W, h + 0.5f }, Palette::Background);
        // Mep sang mong o 2 bien dai dang dong - "tia quet" CRT; tat khi dai da kin
        if (cover < 1.0f) {
            Color edge = Fade(Palette::Weaver, 0.35f * (1.0f - cover));
            DrawRectangleRec({ 0.0f, y, (float)Config::SCREEN_W, 1.0f }, edge);
            DrawRectangleRec({ 0.0f, y + h - 1.0f, (float)Config::SCREEN_W, 1.0f }, edge);
        }
    }
}

// ==========================================
// OBSERVABILITY / PROFILING OVERLAY
// TAT CA so lieu o day deu la SO DO THUC TE tu chinh tien trinh dang chay - khong phai
// uoc luong ly thuyet:
//   - FPS/Frame time: raylib tu do (GetFPS/GetFrameTime), phan anh dung thoi gian moi
//     vong lap thuc te da mat, KE CA thoi gian cho vsync/hoan doi buffer GPU - vi day la
//     renderer dong bo (khong co hang doi lenh GPU bat dong bo rieng), "frame time" nay
//     THUC CHAT DA LA CPU+GPU cong lai, khong the tach rieng CPU-only/GPU-only ma khong
//     dung OpenGL timer query (raylib khong lo lieu san cai nay) - ghi nhan trung thuc
//     thay vi bia ra 2 con so rieng khong co that.
//   - RAM: VmRSS thuc te doc tu /proc/self/status (xem process_metrics.h), khong phai
//     tinh nhau sizeof() cac struct roi cong lai (con so do KHONG phan anh dung bo nho
//     that su he dieu hanh cap phat, vi con phu thuoc allocator/fragmentation).
//   - So luong entity: dung Size()/GetActiveCount() THAT cua tung pool trong frame hien
//     tai - day chinh la con so chung minh Culling (xem culling.h) co dang hoat dong hay
//     khong (vd so Kamikaze dang song > so duoc VE thuc su neu co con nam ngoai camera).
// ==========================================
void RenderSystem::DrawDebugOverlay(const GameManager& gm) {
    UICanvas canvas;
    int x = 10, y = 90, lineH = 16;

    Rectangle bg{ 6.0f, 84.0f, 220.0f, 210.0f };
    DrawRectangleRec(bg, Fade(Palette::Background, 0.65f));
    DrawRectangleLinesEx(bg, 1.0f, Palette::UiSuccess);

    canvas.Text(x, y, 16, Palette::UiSuccess, "-- PROFILER (F3) --"); y += lineH + 2;
    canvas.Text(x, y, 14, Palette::UiText, TextFormat("FPS: %d", GetFPS())); y += lineH;
    canvas.Text(x, y, 14, Palette::UiText, TextFormat("Frame time: %.2f ms", GetFrameTime() * 1000.0f)); y += lineH;

    long rssKb = GetProcessRssKb();
    if (rssKb >= 0) canvas.Text(x, y, 14, Palette::UiText, TextFormat("RAM (RSS): %ld MB", rssKb / 1024));
    else canvas.Text(x, y, 14, Palette::UiDim, "RAM (RSS): N/A");
    y += lineH + 4;

    canvas.Text(x, y, 14, Palette::Weaver, "-- ENTITIES --"); y += lineH;
    canvas.Text(x, y, 13, Palette::UiText, TextFormat("Basic: %d  Tanky: %d", (int)gm.basicEnemies.Size(), (int)gm.tankyEnemies.Size())); y += lineH;
    canvas.Text(x, y, 13, Palette::UiText, TextFormat("Zigzag: %d  Kamikaze: %d", (int)gm.zigzagEnemies.Size(), (int)gm.kamikazeEnemies.Size())); y += lineH;
    canvas.Text(x, y, 13, Palette::UiText, TextFormat("Boss: %d  UFO: %d", (int)gm.bossPool.Size(), gm.ufoActive ? 1 : 0)); y += lineH;
    canvas.Text(x, y, 13, Palette::UiText, TextFormat("Bullets: %d / %d", (int)gm.playerBullets.GetActiveCount(), (int)gm.enemyBullets.GetActiveCount())); y += lineH;
    canvas.Text(x, y, 13, Palette::UiText, TextFormat("Particles: %d", (int)gm.particles.GetActiveCount())); y += lineH;

    canvas.Draw(gm.gameFont);
}
