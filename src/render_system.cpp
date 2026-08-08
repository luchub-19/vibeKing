#include "render_system.h"
#include "game_manager.h"
#include "ui_system.h"
#include "culling.h"
#include "process_metrics.h"
#include "input_system.h"
#include "meta_progress.h"
#include "localization.h"

// Logo tieu de MENU: hang basicAlien THAT (SpriteSheet, dung atlas Kenney neu co - xem
// docs/ASSET_INTEGRATION.md, fallback procedural neu khong - xem sprites.cpp) nhap nhoi
// len xuong theo sin(GetTime()), lech pha tung con - giu NGUYEN kieu bob da co truoc day,
// chi doi phan VE tu hinh hoc thuan (DrawRectangle) sang DrawSprite() that. Nhan thang
// Texture2D (KHONG nhan ca GameManager&) - dung tinh than DrawLoadoutSelect() ngay duoi:
// day la ham static, khong co `friend class RenderSystem`, nen chi thao tac tren gia tri
// DA duoc trich xuat san tu DrawMenu() (noi THAT su co quyen doc gm.sprites).
static void DrawTitleLogo(const Texture2D& alienTex) {
    float t = (float)GetTime();
    const int alienCount = 5;
    const float spacing = 44.0f;
    const float startX = (float)Config::SCREEN_W / 2.0f - spacing * (float)(alienCount - 1) / 2.0f;
    const float baseY = 50.0f;
    const float w = 34.0f, h = 27.0f; // Xap xi ti le atlas that (basicAlien 104x84 - xem assets/sprites/atlas.cfg)

    for (int i = 0; i < alienCount; i++) {
        // Moi con lac len xuong LECH PHA nhau (offset theo i) - tranh cam giac "ca hang
        // dong bo cung luc" cung nhac, giong dang song lac dac trung cua the loai game nay.
        float bob = sinf(t * 2.5f + (float)i * 0.6f) * 5.0f;
        float x = startX + (float)i * spacing;
        float y = baseY + bob;

        // PURPLE/VIOLET xen ke - DUNG mau basicAlien that su dung trong gameplay (xem
        // GameManager::InitLevel(): "Color col = (spawn.row % 2 == 0) ? PURPLE : VIOLET"),
        // khong con GREEN/LIME cu (chi hop ly luc logo la silhouette rieng, khong lien
        // quan mau dich that). Logo gio la 1 "xem truoc" trung thuc, khong phai trang tri
        // tuy y - doi mau dich trong gameplay sau nay thi doi luon o day cho khop.
        Color c = (i % 2 == 0) ? PURPLE : VIOLET;
        DrawSprite(alienTex, { x - w / 2.0f, y - h / 2.0f, w, h }, c);
    }
}

// PILL LUA CHON (dung cho DIFFICULTY) - o nho co vien, sang len khi dang la lua chon HIEN
// TAI. Thay cho kieu "< NORMAL >" cycle an 2 lua chon con lai truoc day: ve ca 3 pill cung
// luc (goi 3 lan voi rect canh nhau) de nguoi choi thay HET lua chon thay vi phai bam thu
// tung huong. Khong can icon mui ten rieng nua - chinh vien/nen sang cua pill dang chon da
// la affordance.
static void DrawSelectPill(UICanvas& canvas, Rectangle rect, const char* label, bool selected) {
    Color fill = selected ? Color{ 40, 36, 12, 200 } : Color{ 16, 16, 26, 140 };
    Color border = selected ? YELLOW : GRAY;
    canvas.Panel(rect, fill, border, selected ? 2.0f : 1.0f);
    Color textColor = selected ? WHITE : Fade(WHITE, 0.45f);
    canvas.CenteredText((int)(rect.x + rect.width / 2.0f), (int)(rect.y + rect.height / 2.0f - 8.0f), 15, textColor, label);
}

// CARD LOADOUT - cung khuon DrawSelectPill nhung 2 dong (ten tren, trang thai duoi), dung
// cho 3 loadout Standard/Vanguard/Overcharge ve canh nhau. Card chua unlock (khong phai
// Standard) hien GRAY va so currency con thieu thay vi chu "UNLOCKED" - giu dung ngu nghia
// mau GRAY = khoa da co tu DrawLoadoutSelect() ban cu.
static void DrawLoadoutCard(UICanvas& canvas, Rectangle rect, LoadoutType type, bool selected, bool available, int currency, int cost) {
    Color fill = selected ? Color{ 40, 36, 12, 200 } : Color{ 16, 16, 26, 140 };
    Color border = selected ? YELLOW : GRAY;
    canvas.Panel(rect, fill, border, selected ? 2.0f : 1.0f);

    Color nameColor = available ? (selected ? WHITE : Fade(WHITE, 0.6f)) : GRAY;
    canvas.CenteredText((int)(rect.x + rect.width / 2.0f), (int)rect.y + 8, 13, nameColor, GetLoadoutName(type));

    std::string status;
    Color statusColor;
    if (type == LoadoutType::Standard)   { status = "FREE";  statusColor = Fade(WHITE, 0.6f); }
    else if (available)                  { status = "READY"; statusColor = LIME; }
    else                                 { status = TextFormat("%d/%d", currency, cost); statusColor = GRAY; }
    canvas.CenteredText((int)(rect.x + rect.width / 2.0f), (int)rect.y + 28, 11, statusColor, status.c_str());
}

// ==========================================
// BO CUC MENU MOI - 3 TANG thay cho 1 cot doc dai truoc day (thao luan voi Dawg ve UI
// overhaul, xem chat): Header (logo+ten) / 2 PANEL canh nhau CHIEU CAO CO DINH (TOP 10 +
// DIFFICULTY-LOADOUT-VOLUME) / nut START dang Panel that o Footer.
//
// Ly do panel CO DINH kich thuoc: ban cu tinh bottomY = 195 + entries.size()*20 + 30 roi
// fallback ve 420 khi rong - chinh phep tinh nay la nguon goc khoang den lon giua man hinh
// luc chua co ky luc nao (truong hop THUONG GAP NHAT - moi lan xoa save/may moi). Panel co
// dinh khong con phu thuoc so dong du lieu, nen khong con "co gian" theo noi dung.
//
// Dung LAI panelFill/panelBorder GIONG HET DrawHUD() ben duoi file nay (cung HUD_PANEL_ALPHA/
// HUD_PANEL_BORDER_THICKNESS) - Menu va HUD gio chung 1 "chat lieu" thi giac thay vi 2 the
// gioi rieng (Menu truoc day khong dung UICanvas::Panel() lan nao).
// ==========================================
void RenderSystem::DrawMenu(const GameManager& gm) {
    DrawTitleLogo(gm.sprites.basicAlien);

    UICanvas canvas;

    // TEN THAT cua game (khop InitWindow() trong game_manager.cpp va README), khong con
    // "SPACE INVADERS" - do la ten THE LOAI, khong phai ten game nay. CenteredText (thay
    // Text voi x=250 hardcode cu) de luon can giua du sau nay doi chuoi.
    canvas.CenteredText(Config::SCREEN_W / 2, 92, 40, GREEN, "HARDCORE SPACE INVADERS");

    Color panelFill = { 16, 16, 26, (unsigned char)(255.0f * Config::HUD_PANEL_ALPHA) };
    Color panelBorder = GRAY;
    const float leftX = 40.0f, rightX = 415.0f, panelW = 345.0f, panelY = 165.0f, panelH = 290.0f;

    // --- Panel trai: TOP 10 ---
    canvas.Panel({ leftX, panelY, panelW, panelH }, panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS);
    canvas.CenteredText((int)(leftX + panelW / 2.0f), (int)panelY + 12, 20, YELLOW, "TOP 10");

    const auto& entries = gm.leaderboard.GetEntries();
    if (entries.empty()) {
        // Canh giua CA CHIEU DOC trong panel - khac ban cu (1 dong xam nho lac long ngay
        // duoi header, phan con lai cua man hinh la khoang den). Panel co dinh kich thuoc
        // nen luon co du cho de canh giua thay vi phai doan vi tri theo noi dung.
        canvas.CenteredText((int)(leftX + panelW / 2.0f), (int)(panelY + panelH / 2.0f), 16, GRAY, Loc::NoRecordsYet);
    } else {
        float y = panelY + 44.0f;
        for (size_t i = 0; i < entries.size(); i++) {
            Color rowColor = (i == 0) ? YELLOW : WHITE;
            canvas.Text((int)leftX + 14, (int)y, 15, rowColor,
                        TextFormat("%2d. %6d pts  wave %d", (int)i + 1, entries[i].score, entries[i].wave));
            y += 22.0f; // 10 dong toi da (Config::LEADERBOARD_MAX_ENTRIES) * 22 = 220, vua trong panelH=290
        }
    }

    // --- Panel phai: DIFFICULTY / LOADOUT / VOLUME ---
    canvas.Panel({ rightX, panelY, panelW, panelH }, panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS);

    canvas.Text((int)rightX + 14, (int)panelY + 12, 15, YELLOW, "DIFFICULTY  (LEFT/RIGHT)");
    const float pillW = 105.0f, pillGap = 10.0f, pillY = panelY + 36.0f;
    for (int i = 0; i < 3; i++) {
        DifficultyStats s = GetDifficultyStats((Difficulty)i);
        Rectangle pillRect = { rightX + 5.0f + (float)i * (pillW + pillGap), pillY, pillW, 32.0f };
        DrawSelectPill(canvas, pillRect, s.label, (Difficulty)i == gm.difficulty);
    }

    canvas.Text((int)rightX + 14, (int)panelY + 82, 15, YELLOW, TextFormat("LOADOUT  (Q/E) - %d CR", gm.metaProgress.GetCurrency()));
    const float cardY = panelY + 106.0f;
    const LoadoutType loadouts[3] = { LoadoutType::Standard, LoadoutType::Vanguard, LoadoutType::Overcharge };
    for (int i = 0; i < 3; i++) {
        LoadoutType type = loadouts[i];
        bool available = (type == LoadoutType::Standard) || gm.metaProgress.IsUnlocked(type);
        Rectangle cardRect = { rightX + 5.0f + (float)i * (pillW + pillGap), cardY, pillW, 50.0f };
        DrawLoadoutCard(canvas, cardRect, type, (int)type == gm.selectedLoadout, available,
                         gm.metaProgress.GetCurrency(), GetLoadoutUnlockCost(type));
    }

    canvas.Text((int)rightX + 14, (int)panelY + 176, 15, YELLOW, "VOLUME  (UP/DOWN)");
    canvas.Bar({ rightX + 14.0f, panelY + 200.0f, panelW - 28.0f, 16.0f }, gm.audio.GetVolume(), DARKGRAY, SKYBLUE, WHITE);
    canvas.Text((int)(rightX + panelW - 46.0f), (int)panelY + 218, 13, GRAY, TextFormat("%d%%", (int)(gm.audio.GetVolume() * 100.0f)));

    // --- Footer: nut START dang Panel that (border pulse) thay vi 1 dong chu doi alpha ---
    float startPulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);
    Rectangle startRect = { Config::SCREEN_W / 2.0f - 140.0f, 480.0f, 280.0f, 52.0f };
    canvas.Panel(startRect, Color{ 16, 16, 26, 180 }, Fade(YELLOW, 0.6f + 0.4f * startPulse), 2.0f + startPulse);
    canvas.CenteredText(Config::SCREEN_W / 2, 496, 20, WHITE, "PRESS ENTER TO START");

    canvas.CenteredText(Config::SCREEN_W / 2, 548, 14, GRAY, "ARROWS / Q,E: ADJUST");
    canvas.CenteredText(Config::SCREEN_W / 2, 568, 14, GRAY, Loc::MenuFullscreenHint);

    canvas.Draw(gm.gameFont);
}

void RenderSystem::DrawEndScreen(const GameManager& gm) {
    UICanvas canvas;
    // A4: toan bo man hinh nay la banner/thong bao mang tinh "trung tam" (khong phai
    // du lieu dang bang/cot can can le trai) - doi sang CenteredText() de luon nam
    // giua man hinh (Config::SCREEN_W/2) bat ke do dai chuoi (vd "SCORE: 12345" vs
    // "SCORE: 5") thay vi toa do x hardcode tung dong nhu truoc.
    int centerX = Config::SCREEN_W / 2;
    bool waveClear = (gm.state == GameState::WAVE_CLEAR);
    if (waveClear) {
        canvas.CenteredText(centerX, 180, 36, YELLOW, TextFormat("WAVE %d CLEARED!", gm.wave - 1));
        canvas.CenteredText(centerX, 240, 20, WHITE, TextFormat("SCORE: %d", gm.player.GetScore()));
        canvas.CenteredText(centerX, 300, 20, GRAY, "ENTER: NEXT WAVE   R: RESTART");
    } else {
        canvas.CenteredText(centerX, 180, 40, RED, "GAME OVER");
        canvas.CenteredText(centerX, 240, 20, WHITE, TextFormat("FINAL SCORE: %d   WAVE REACHED: %d", gm.player.GetScore(), gm.wave));

        // 3 trang thai ro rang thay vi 1 bool "co pha ky luc hay khong": NewRecord (giờ
        // la #1), MadeTop10 (lot danh sach nhung khong phai #1), hoac khong lot top nao
        // ca (van hien diem cao nhat hien tai de nguoi choi biet minh con thieu bao nhieu).
        if (gm.lastSubmitResult == SubmitResult::NewRecord) {
            canvas.CenteredText(centerX, 270, 20, YELLOW, Loc::NewRecordBanner);
        } else if (gm.lastSubmitResult == SubmitResult::MadeTop10) {
            canvas.CenteredText(centerX, 270, 20, LIME, Loc::MadeTop10Banner);
        } else {
            canvas.CenteredText(centerX, 270, 18, GRAY, TextFormat("TOP SCORE: %d", gm.leaderboard.GetTopScore()));
        }
        canvas.CenteredText(centerX, 330, 20, GRAY, "ENTER: MENU   R: RESTART");
    }
    canvas.Draw(gm.gameFont);
}

// IDLE ANIMATION (Phase 1 - Graphics/UI Overhaul, Nguoi 1): transform-THUAN quanh tam 1
// Rectangle theo sin(GetTime()) - bob truc Y y het ky thuat DrawTitleLogo() o tren, cong
// them 1 lop pulse ti le RAT nho DONG PHA voi bob (cung 1 sin() - "phinh to nhe dung luc
// nhap len") de sprite co cam giac "song" thay vi dung yen tuyet doi. CHI tra ve Rectangle
// MOI danh rieng cho DrawSprite() - KHONG duoc dung ket qua nay lam rect that (hitbox) cua
// entity, xem tung noi goi trong DrawPlaying() ben duoi (luon giu nguyen `e.rect`/
// `boss.rect` cho va cham/logic, chi doi bien tam o buoc VE).
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

void RenderSystem::DrawPlaying(const GameManager& gm) {
    Camera2D cam{};
    cam.offset = gm.screenShake.GetOffset();
    cam.target = { 0, 0 };
    cam.rotation = 0.0f;
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
        DrawSprite(gm.sprites.tankyAlien, drawRect, e.color);
        if (e.hp < TankyEnemy::HP) {
            // Dich mau day bi thuong -> vien sang de nguoi choi thay ro da gay sat thuong
            // - dung e.rect GOC (khong phai drawRect) cho vien nay: day la chi bao gan
            // voi hitbox that, khong phai trang tri thuan tuy nhu sprite o tren.
            DrawRectangleLinesEx(e.rect, 2.0f, WHITE);
        }
    }
    for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++) {
        const ZigzagEnemy& e = gm.zigzagEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        float phase = (float)e.column * Config::ANIM_IDLE_PHASE_STEP;
        Rectangle drawRect = IdleWobble(e.rect, animTime, phase, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.zigzagAlien, drawRect, e.color);
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
    if (gm.ufoActive && Culling::IsVisible(gm.ufoRect)) {
        // Chi 1 UFO ton tai cung luc (xem gm.ufoActive) - khong can lech pha rieng (phase=0).
        Rectangle drawRect = IdleWobble(gm.ufoRect, animTime, 0.0f, Config::ANIM_IDLE_BOB_AMPLITUDE,
                                         Config::ANIM_IDLE_BOB_FREQUENCY, Config::ANIM_IDLE_SCALE_AMPLITUDE);
        DrawSprite(gm.sprites.ufo, drawRect, RED);
    }
    // BOSS: cung 1 kieu Pool nhu moi loai dich khac (EnemyPool<Boss,1>) - Size()>0 nghia
    // la con song, khong con co Bool `bossActive` rieng phai giu dong bo thu cong.
    if (gm.bossPool.Size() > 0) {
        const Boss& boss = gm.bossPool[0];
        if (Culling::IsVisible(boss.rect)) {
            int stage = BossStage(boss);
            Color tint = (stage == 1) ? WHITE : (stage == 2) ? ORANGE : RED; // Cang yeu cang do, bao hieu "enrage"

            const Texture2D& tex = (boss.type == BossType::Sentinel) ? gm.sprites.bossSentinel
                                  : (boss.type == BossType::Swarmer) ? gm.sprites.bossSwarmer
                                  : gm.sprites.boss;
            // Bo hang so RIENG (ANIM_BOSS_IDLE_*, xem config.h) - than lon hon han dich
            // thuong nen cung bien do px se de nhan thay hon; chi 1 Boss ton tai cung luc
            // nen khong can lech pha (phase=0), khac Basic/Tanky/Zigzag o tren.
            Rectangle drawRect = IdleWobble(boss.rect, animTime, 0.0f, Config::ANIM_BOSS_IDLE_BOB_AMPLITUDE,
                                             Config::ANIM_BOSS_IDLE_BOB_FREQUENCY, Config::ANIM_BOSS_IDLE_SCALE_AMPLITUDE);
            DrawSprite(tex, drawRect, tint);

            // VONG KHIEN: chi ve khi Sentinel dang bat kha xam pham - vien tron xanh bao
            // quanh toan bo rect, bao hieu ro rang "dan khong an thua luc nay" (khop voi
            // logic mien sat thuong trong PhysicsSystem::CheckCollisions()). Dung
            // boss.rect GOC (khong phai drawRect) vi day la chi bao gan voi vung mien sat
            // thuong THAT, khong phai trang tri.
            if (boss.type == BossType::Sentinel && boss.shieldActive) {
                Vector2 center = EnemyCenter(boss.rect);
                float radius = fmaxf(boss.rect.width, boss.rect.height) * 0.62f;
                DrawCircleLines((int)center.x, (int)center.y, radius, SKYBLUE);
                DrawCircleLines((int)center.x, (int)center.y, radius - 2.0f, Fade(SKYBLUE, 0.5f));
            }
        }
    }
    for (const auto& bunker : gm.bunkers) bunker.Draw();
    gm.playerBullets.Draw(YELLOW);
    gm.enemyBullets.Draw(RED);
    gm.particles.Draw();
    gm.floatingTexts.Draw(gm.gameFont);

    // POWER-UP: icon rieng theo tung loai (xem PowerUpType trong powerup.h + 4 ham
    // BuildIcon*() trong sprites.cpp) thay vi hinh chu nhat mau tron - cung khuon chon
    // texture theo loai nhu khoi Boss o tren. KHONG culling (xem culling.h: PowerUp tu
    // huy ngay khi vuot bien man hinh nen luon o gan/trong man hinh suot vong doi active,
    // kiem tra o day chi la chi phi thua, khong loai duoc lenh ve nao ca).
    for (size_t i = 0; i < gm.powerUps.Size(); i++) {
        const PowerUp& p = gm.powerUps[i];
        Texture2D tex;
        Color tint;
        switch (p.type) {
            case PowerUpType::RapidFire: tex = gm.sprites.iconRapidFire; tint = ORANGE;  break;
            case PowerUpType::Shield:    tex = gm.sprites.iconShield;    tint = SKYBLUE; break;
            case PowerUpType::Piercing:  tex = gm.sprites.iconPiercing;  tint = MAGENTA; break;
            case PowerUpType::Cleanser:  tex = gm.sprites.iconCleanser;  tint = LIME;    break;
            default:                     tex = gm.sprites.iconRapidFire; tint = WHITE;   break;
        }
        DrawSprite(tex, p.rect, tint);
    }

    gm.player.Draw(gm.sprites.player);
    EndMode2D();

    // HUD ve ngoai camera de khong bi rung theo
    DrawHUD(gm);

    int centerX = Config::SCREEN_W / 2;
    if (gm.state == GameState::PAUSED) {
        DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(BLACK, 0.6f));
        UICanvas canvas;
        // A4: CenteredText thay cho toa do x hardcode - "PAUSED" (40pt) va 2 dong gia
        // huong dan (18pt) truoc day dung 3 x khac nhau (330/280/250) uoc luong thu
        // cong theo do dai chuoi, khong con chinh xac neu font/chuoi doi sau nay.
        canvas.CenteredText(centerX, 250, 40, WHITE, "PAUSED");
        canvas.CenteredText(centerX, 310, 18, GRAY, TextFormat("VOLUME: %d%%  (UP/DOWN)", (int)(gm.audio.GetVolume() * 100)));
        canvas.CenteredText(centerX, 340, 18, GRAY, Loc::PausedControlsHint);
        canvas.Draw(gm.gameFont);
    } else if (gm.state == GameState::KEYBIND) {
        DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(BLACK, 0.75f));
        UICanvas canvas;
        canvas.CenteredText(centerX, 90, 32, WHITE, Loc::KeybindTitle);

        const RebindableAction* actions = GetRebindableActions();

        // A4 - FIX: canh giua TUNG dong rebind DOC LAP se lam dau ':' nhay lech giua
        // cac dong co gia tri khac do dai (vd "SPACE" dai hon "A"/"D"/"P" nhieu -> dong
        // do bi keo lech trai de giu TAM rieng no, pha mat cot ':' thang hang von co tu
        // %-6s). Thay vao do: do truoc CA 4 dong, lay dong RONG NHAT lam chuan, roi ve
        // TAT CA left-align chung 1 canh trai (= tam man hinh - rongNhat/2) - vua giu
        // nguyen khoi 4 dong nam GIUA man hinh (dung tinh than CenteredText/A4), vua giu
        // cot ':' thang hang nhu ban goc (Text() hardcode truoc day, chi khac la gio
        // TU DONG can giua ca khoi thay vi 1 x hardcode rieng).
        std::string lines[REBINDABLE_ACTION_COUNT];
        float maxLineWidth = 0.0f;
        for (int i = 0; i < REBINDABLE_ACTION_COUNT; i++) {
            bool isBeingRebound = (gm.rebindingActionIndex == i);
            int currentKey = gm.settings.*(actions[i].keyField);
            lines[i] = TextFormat("%d) %-6s: %s", i + 1, actions[i].label,
                                   isBeingRebound ? "..." : InputSystem::KeyName(currentKey));
            float w = MeasureTextEx(gm.gameFont, lines[i].c_str(), 22.0f, 1.0f).x;
            if (w > maxLineWidth) maxLineWidth = w;
        }
        float rowsLeftX = (float)centerX - maxLineWidth / 2.0f;
        for (int i = 0; i < REBINDABLE_ACTION_COUNT; i++) {
            bool isBeingRebound = (gm.rebindingActionIndex == i);
            Color rowColor = isBeingRebound ? YELLOW : WHITE;
            canvas.Text((int)rowsLeftX, 160 + i * 36, 22, rowColor, lines[i]);
        }

        if (gm.rebindingActionIndex >= 0) {
            canvas.CenteredText(centerX, 340, 18, YELLOW,
                        TextFormat(Loc::RebindPromptFmt, actions[gm.rebindingActionIndex].label));
        } else {
            canvas.CenteredText(centerX, 340, 16, GRAY, Loc::KeybindHelp);
        }
        canvas.Draw(gm.gameFont);
    }
}

void RenderSystem::DrawHUD(const GameManager& gm) {
    UICanvas canvas;

    // PANEL/ICON HUD (Nguoi 3 - Audio & UI, TASK_DIVISION.md): thay nen-den-trong-suot
    // truoc day bang UIPanel (nen toi + vien) quanh TUNG CUM thong tin lien quan, va
    // UIIcon (SpriteSheet::iconShield/iconRapidFire/iconPiercing - CUNG texture/tint da
    // dung cho pickup roi tren mat dat, xem nhanh PowerUpType duoi day trong file nay)
    // thay 3 dong chu SHIELD/RAPID FIRE/PIERCING truoc day. 1 bo mau panel DUY NHAT dung
    // chung ca HUD thay vi hardcode rieng tung noi.
    Color panelFill = { 16, 16, 26, (unsigned char)(255.0f * Config::HUD_PANEL_ALPHA) };
    Color panelBorder = GRAY;

    // --- Diem / Wave / Combo (top-left) ---
    canvas.Panel({ 6.0f, 6.0f, 180.0f, 80.0f }, panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS);
    canvas.Text(16, 14, 20, WHITE, TextFormat("SCORE: %d", gm.player.GetScore()));
    canvas.Text(16, 40, 18, SKYBLUE, TextFormat("WAVE: %d", gm.wave));
    if (gm.comboCount > 1) {
        canvas.Text(16, 62, 18, YELLOW, TextFormat("COMBO x%d", gm.comboCount));
    }

    // A4: an luc Boss active - panel Boss moi (duoi) chiem dung vung ngang nay, de ca 2
    // cung hien se de len nhau (da tung de len ngay ca truoc panel, chi khong ro bang).
    if (gm.bossPool.Size() == 0) {
        canvas.Text(290, 10, 16, GRAY, "P: PAUSE   R: RESTART");
    }

    // --- Mang (top-right) ---
    canvas.Panel({ (float)Config::SCREEN_W - 110.0f, 6.0f, 104.0f, 32.0f }, panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS);
    canvas.Text(Config::SCREEN_W - 100, 14, 20, WHITE, TextFormat("LIVES: %d", gm.player.GetLives()));

    // --- Trang thai power-up: icon badge thay chu, CHI ve panel khi co it nhat 1
    // power-up active (giu HUD trong khi khong co gi active, dung tinh than code cu) -
    // 3 O CO DINH theo THU TU Shield/RapidFire/Piercing (khong dich trai lap khoang
    // trong) de vi tri tung icon on dinh, khong "nhay" khi cac power-up bat/tat khac nhau.
    if (gm.player.HasShield() || gm.player.HasRapidFire() || gm.player.HasPiercing()) {
        float iconY = 46.0f;
        float iconX = (float)Config::SCREEN_W - 102.0f;
        float slot = Config::HUD_ICON_SIZE + 4.0f;
        canvas.Panel({ (float)Config::SCREEN_W - 110.0f, 40.0f, 104.0f, Config::HUD_ICON_SIZE + 12.0f },
                     panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS);
        if (gm.player.HasShield()) {
            canvas.Icon({ iconX, iconY, Config::HUD_ICON_SIZE, Config::HUD_ICON_SIZE }, gm.sprites.iconShield, SKYBLUE);
        }
        if (gm.player.HasRapidFire()) {
            canvas.Icon({ iconX + slot, iconY, Config::HUD_ICON_SIZE, Config::HUD_ICON_SIZE }, gm.sprites.iconRapidFire, ORANGE);
        }
        if (gm.player.HasPiercing()) {
            canvas.Icon({ iconX + slot * 2.0f, iconY, Config::HUD_ICON_SIZE, Config::HUD_ICON_SIZE }, gm.sprites.iconPiercing, MAGENTA);
        }
    }

    if (gm.bossPool.Size() > 0) {
        const Boss& boss = gm.bossPool[0];
        // Thanh mau boss o giua man hinh tren dinh - 1 widget Bar() duy nhat thay vi 3
        // loi goi DrawRectangle/DrawRectangleLines rieng le nhu truoc.
        float barW = 300.0f;
        float ratio = (boss.maxHp > 0) ? ((float)boss.hp / (float)boss.maxHp) : 0.0f;
        float barX = (Config::SCREEN_W - barW) / 2.0f;
        Color barFill = (boss.type == BossType::Sentinel && boss.shieldActive) ? SKYBLUE : RED;
        canvas.Panel({ barX - 10.0f, 4.0f, barW + 20.0f, 36.0f }, panelFill, panelBorder, Config::HUD_PANEL_BORDER_THICKNESS);
        canvas.Bar({ barX, 8.0f, barW, 14.0f }, ratio, DARKGRAY, barFill, WHITE);
        // A4: nhan ten Boss can GIUA thanh mau (truoc day can trai theo canh barX) -
        // nhat quan voi cach cac man hinh khac trong track nay deu can giua theo tam
        // vung lien quan, khong con toa do trai hardcode.
        canvas.CenteredText((int)(barX + barW / 2.0f), 24, 14, barFill, TextFormat("BOSS - %s", BossTypeName(boss.type)));
        if (boss.type == BossType::Sentinel && boss.shieldActive) {
            canvas.Text((int)(barX + barW - 60.0f), 24, 14, SKYBLUE, Loc::ShieldTag);
        }
    }

    canvas.Draw(gm.gameFont);
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
    DrawRectangleRec(bg, Fade(BLACK, 0.65f));
    DrawRectangleLinesEx(bg, 1.0f, GREEN);

    canvas.Text(x, y, 16, GREEN, "-- PROFILER (F3) --"); y += lineH + 2;
    canvas.Text(x, y, 14, WHITE, TextFormat("FPS: %d", GetFPS())); y += lineH;
    canvas.Text(x, y, 14, WHITE, TextFormat("Frame time: %.2f ms", GetFrameTime() * 1000.0f)); y += lineH;

    long rssKb = GetProcessRssKb();
    if (rssKb >= 0) canvas.Text(x, y, 14, WHITE, TextFormat("RAM (RSS): %ld MB", rssKb / 1024));
    else canvas.Text(x, y, 14, GRAY, "RAM (RSS): N/A");
    y += lineH + 4;

    canvas.Text(x, y, 14, SKYBLUE, "-- ENTITIES --"); y += lineH;
    canvas.Text(x, y, 13, WHITE, TextFormat("Basic: %d  Tanky: %d", (int)gm.basicEnemies.Size(), (int)gm.tankyEnemies.Size())); y += lineH;
    canvas.Text(x, y, 13, WHITE, TextFormat("Zigzag: %d  Kamikaze: %d", (int)gm.zigzagEnemies.Size(), (int)gm.kamikazeEnemies.Size())); y += lineH;
    canvas.Text(x, y, 13, WHITE, TextFormat("Boss: %d  UFO: %d", (int)gm.bossPool.Size(), gm.ufoActive ? 1 : 0)); y += lineH;
    canvas.Text(x, y, 13, WHITE, TextFormat("Bullets: %d / %d", (int)gm.playerBullets.GetActiveCount(), (int)gm.enemyBullets.GetActiveCount())); y += lineH;
    canvas.Text(x, y, 13, WHITE, TextFormat("Particles: %d", (int)gm.particles.GetActiveCount())); y += lineH;

    canvas.Draw(gm.gameFont);
}
