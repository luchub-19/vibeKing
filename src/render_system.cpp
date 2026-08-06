#include "render_system.h"
#include "game_manager.h"
#include "ui_system.h"
#include "culling.h"
#include "process_metrics.h"
#include "input_system.h"
#include "meta_progress.h"
#include "localization.h"

// Logo tieu de MENU: hang alien nho nhap nhoi len xuong theo sin(GetTime()) - hinh hoc
// thuan (DrawRectangle truc tiep), KHONG dung SpriteSheet vi can animation LIEN TUC theo
// thoi gian thuc, khac voi sprite gameplay chi nap 1 lan tinh luc SpriteSheet::Load().
static void DrawTitleLogo() {
    float t = (float)GetTime();
    const int alienCount = 5;
    const float spacing = 44.0f;
    const float startX = (float)Config::SCREEN_W / 2.0f - spacing * (float)(alienCount - 1) / 2.0f;
    const float baseY = 50.0f;

    for (int i = 0; i < alienCount; i++) {
        // Moi con lac len xuong LECH PHA nhau (offset theo i) - tranh cam giac "ca hang
        // dong bo cung luc" cung nhac, giong dang song lac dac trung cua the loai game nay.
        float bob = sinf(t * 2.5f + (float)i * 0.6f) * 5.0f;
        float x = startX + (float)i * spacing;
        float y = baseY + bob;
        Color c = (i % 2 == 0) ? GREEN : LIME;

        // Silhouette alien don gian: than + 2 "cang" 2 ben + 2 "chan" nho - ve truc tiep,
        // khong qua BuildXxx()/texture tinh nao ca (logo nay DONG, sprite gameplay TINH).
        DrawRectangle((int)(x - 10.0f), (int)(y - 6.0f), 20, 10, c);
        DrawRectangle((int)(x - 14.0f), (int)(y - 2.0f), 6, 6, c);
        DrawRectangle((int)(x + 8.0f), (int)(y - 2.0f), 6, 6, c);
        DrawRectangle((int)(x - 5.0f), (int)(y + 4.0f), 3, 4, c);
        DrawRectangle((int)(x + 2.0f), (int)(y + 4.0f), 3, 4, c);
    }
}

// Icon mui ten nho, nhap nhay nhe (sin theo thoi gian) dat canh dong menu dang duoc
// "chon"/dieu chinh - HIEN CHI gan cho dong DIFFICULTY (xem ghi chu tai noi goi trong
// DrawMenu ve viec LOADOUT chua co icon rieng). Tach thanh ham rieng de tai su dung cho
// cac dong menu dieu huong duoc khac sau nay thay vi copy lai code ve tam giac nay.
static void DrawMenuSelectorIcon(float x, float y, Color color) {
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);
    Color c = Fade(color, 0.6f + 0.4f * pulse);
    float half = 4.0f + pulse * 1.0f;
    DrawTriangle({ x, y - half }, { x, y + half }, { x + half * 2.0f, y }, c);
}

// LOADOUT SELECT - ve dong "LOADOUT: <ten> (...)" ngay duoi dong DIFFICULTY trong Menu.
// Nhan SAN cac gia tri da doc tu GameManager (khong nhan thang GameManager&) - dung tinh
// than MakeEnemyKilledEvent trong physics_system.cpp: 1 helper `static` chi thao tac tren
// gia tri thuan. Bat buoc phai vay: day KHONG phai ham thanh vien RenderSystem nen KHONG
// duoc huong quyen `friend class RenderSystem` ma GameManager cap (xem game_manager.h) -
// doc gm.selectedLoadout/gm.metaProgress phai xay ra trong DrawMenu() (ham thanh vien
// that su) roi truyen gia tri da trich xuat vao day.
static void DrawLoadoutSelect(UICanvas& canvas, int y, LoadoutType chosen, bool unlockedOrFree, int currency, int cost, float pulse) {
    std::string status;
    if (chosen == LoadoutType::Standard) status = "FREE";
    else if (unlockedOrFree) status = "UNLOCKED";
    else status = TextFormat(Loc::UnlockCostFmt, currency, cost);

    // BUG FIX (Bug 1): truoc day dong nay dung size 18 va x=200 hardcode, trong khi
    // dong DIFFICULTY ngay TREN no (xem DrawMenu) dung size 20 va x=260 - 2 con so
    // lech nhau tu 1 merge, khong phai chu y (khong co ly do thiet ke nao de 2 dong
    // dieu huong CUNG cap (Q/E vs LEFT/RIGHT) trong CUNG 1 khoi menu lai khac co chu
    // nhau). Chon lai size 20 cho DONG BO voi DIFFICULTY, va CenteredText (xem A1) de
    // ca 2 dong luon nam giua man hinh du do dai chuoi (ten loadout, trang thai...)
    // thay doi the nao.
    // A6: chi pulse khi dong nay dang "kha dung" (WHITE) - GRAY (dang khoa) giu tinh.
    Color color = unlockedOrFree ? Fade(WHITE, 0.7f + 0.3f * pulse) : GRAY;
    canvas.CenteredText(Config::SCREEN_W / 2, y, 20, color,
                         TextFormat("< LOADOUT: %s (%s) >", GetLoadoutName(chosen), status.c_str()));
}

void RenderSystem::DrawMenu(const GameManager& gm) {
    DrawTitleLogo();

    UICanvas canvas;
    canvas.Text(250, 100, 40, GREEN, "SPACE INVADERS");

    // LEADERBOARD: hien thi toi da Config::LEADERBOARD_MAX_ENTRIES dong, xep hang tu
    // 1, kem wave da dat duoc (khong chi mot con so diem tran trui nhu HighScore cu).
    const auto& entries = gm.leaderboard.GetEntries();
    canvas.Text(350, 165, 22, YELLOW, "TOP 10");
    if (entries.empty()) {
        canvas.Text(290, 195, 16, GRAY, Loc::NoRecordsYet);
    } else {
        int y = 195;
        for (size_t i = 0; i < entries.size(); i++) {
            Color rowColor = (i == 0) ? YELLOW : WHITE;
            canvas.Text(230, y, 16, rowColor,
                        TextFormat("%2d.  %6d pts   wave %d", (int)i + 1, entries[i].score, entries[i].wave));
            y += 20;
        }
    }

    DifficultyStats stats = GetDifficultyStats(gm.difficulty);
    int bottomY = 195 + (int)entries.size() * 20 + 30;
    if (bottomY < 420) bottomY = 420; // Danh sach rong/ngan van giu bo cuc on dinh, khong bi troi len qua cao

    // A2/A4: dong DIFFICULTY gio CENTERED (do rong thay doi theo label - "EASY"/"HARD"
    // (4 ky tu) ngan hon "NORMAL" (6 ky tu)) thay vi hardcode x=260 nhu truoc. Icon mui
    // ten PHAI do lai canh trai THAT SU cua dong nay (MeasureTextEx, cung font/size se
    // dung de ve) roi neo theo do, khong con duoc phep hardcode 1 x co dinh (238) nhu
    // truoc nua - hardcode se lech dan theo label moi lan doi do kho (vd dung tot voi
    // NORMAL nhung lech ~1 chu voi EASY/HARD do do rong chuoi khac nhau).
    //
    // TODO(sau merge Nguoi A + Nguoi B, van con nguyen tu ban truoc A6): icon nay hien
    // CHI tro vao dong DIFFICULTY. Menu co 2 dong dieu huong duoc (DIFFICULTY + LOADOUT,
    // xem DrawLoadoutSelect ben duoi) nhung chi 1 co icon - van can ban bac xem co nen
    // doi icon theo dong dang "active" hay them 1 icon thu 2 cho LOADOUT; A6 (menu
    // pulse) khong tu quyet dinh thay cau hoi nay, chi lam CA 2 dong cung "tho" nhe.
    std::string difficultyLine = TextFormat("< DIFFICULTY: %s >", stats.label);
    Vector2 difficultySize = MeasureTextEx(gm.gameFont, difficultyLine.c_str(), 20.0f, 1.0f);
    float difficultyLeftX = (float)Config::SCREEN_W / 2.0f - difficultySize.x / 2.0f;
    DrawMenuSelectorIcon(difficultyLeftX - 22.0f, (float)bottomY + 10.0f, YELLOW); // -22: giu dung khoang cach icon<->chu nhu ban cu (238 vs x=260 hardcode truoc day)

    // A6 - MENU JUICE: 2 dong dieu huong duoc (DIFFICULTY qua LEFT/RIGHT, LOADOUT qua
    // Q/E) "tho" nhe theo thoi gian thuc, cung cong thuc pulse ma DrawMenuSelectorIcon()
    // da dung cho icon tam giac ben tren - dong bo pha giua icon va chu thay vi 2 nhip
    // rieng biet nhin roi mat. Day la cong viec THUAN VE (chi doi do sang/toi cua mau
    // moi frame, khong doi state gi), nen khong can dung/cham GameManager - da kiem tra
    // truoc: TransitionPhase (game_manager.h) DA duoc GameManager::Run() ve lam man
    // hinh den fade roi (xem `float alpha = GetTransitionAlpha()` trong game_manager.cpp),
    // nen khong phat sinh viec moi nao can bao Track B ve day.
    float navPulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);
    Color difficultyColor = Fade(WHITE, 0.7f + 0.3f * navPulse);
    canvas.CenteredText(Config::SCREEN_W / 2, bottomY, 20, difficultyColor, difficultyLine);

    LoadoutType chosenLoadout = (LoadoutType)gm.selectedLoadout;
    bool loadoutAvailable = (chosenLoadout == LoadoutType::Standard) || gm.metaProgress.IsUnlocked(chosenLoadout);
    DrawLoadoutSelect(canvas, bottomY + 24, chosenLoadout,
                       loadoutAvailable, gm.metaProgress.GetCurrency(), GetLoadoutUnlockCost(chosenLoadout), navPulse);

    canvas.CenteredText(Config::SCREEN_W / 2, bottomY + 52, 18, GRAY, TextFormat("VOLUME: %d%%  (UP/DOWN)", (int)(gm.audio.GetVolume() * 100)));
    canvas.CenteredText(Config::SCREEN_W / 2, bottomY + 90, 20, WHITE, "PRESS ENTER TO START");
    canvas.CenteredText(Config::SCREEN_W / 2, bottomY + 118, 14, GRAY, "LEFT/RIGHT: DIFFICULTY    Q/E: LOADOUT");
    canvas.CenteredText(Config::SCREEN_W / 2, bottomY + 138, 16, GRAY, Loc::MenuFullscreenHint);

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
    for (size_t i = 0; i < gm.basicEnemies.Size(); i++) {
        const BasicEnemy& e = gm.basicEnemies[i];
        if (Culling::IsVisible(e.rect)) DrawSprite(gm.sprites.basicAlien, e.rect, e.color);
    }
    for (size_t i = 0; i < gm.tankyEnemies.Size(); i++) {
        const TankyEnemy& e = gm.tankyEnemies[i];
        if (!Culling::IsVisible(e.rect)) continue;
        DrawSprite(gm.sprites.tankyAlien, e.rect, e.color);
        if (e.hp < TankyEnemy::HP) {
            // Dich mau day bi thuong -> vien sang de nguoi choi thay ro da gay sat thuong
            DrawRectangleLinesEx(e.rect, 2.0f, WHITE);
        }
    }
    for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++) {
        const ZigzagEnemy& e = gm.zigzagEnemies[i];
        if (Culling::IsVisible(e.rect)) DrawSprite(gm.sprites.zigzagAlien, e.rect, e.color);
    }
    for (size_t i = 0; i < gm.kamikazeEnemies.Size(); i++) {
        const KamikazeEnemy& e = gm.kamikazeEnemies[i];
        if (Culling::IsVisible(e.rect)) DrawSprite(gm.sprites.kamikaze, e.rect, e.color);
    }
    if (gm.ufoActive && Culling::IsVisible(gm.ufoRect)) {
        DrawSprite(gm.sprites.ufo, gm.ufoRect, RED);
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
            DrawSprite(tex, boss.rect, tint);

            // VONG KHIEN: chi ve khi Sentinel dang bat kha xam pham - vien tron xanh bao
            // quanh toan bo rect, bao hieu ro rang "dan khong an thua luc nay" (khop voi
            // logic mien sat thuong trong PhysicsSystem::CheckCollisions()).
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
    canvas.Text(10, 10, 20, WHITE, TextFormat("SCORE: %d", gm.player.GetScore()));
    canvas.Text(10, 35, 18, SKYBLUE, TextFormat("WAVE: %d", gm.wave));
    canvas.Text(700, 10, 20, WHITE, TextFormat("LIVES: %d", gm.player.GetLives()));
    canvas.Text(290, 10, 16, GRAY, "P: PAUSE   R: RESTART");

    if (gm.comboCount > 1) {
        canvas.Text(330, 35, 18, YELLOW, TextFormat("COMBO x%d", gm.comboCount));
    }
    if (gm.player.HasShield()) canvas.Text(690, 35, 16, SKYBLUE, "SHIELD");
    if (gm.player.HasRapidFire()) canvas.Text(660, 55, 16, ORANGE, "RAPID FIRE");
    if (gm.player.HasPiercing()) canvas.Text(670, 75, 16, MAGENTA, "PIERCING");

    if (gm.bossPool.Size() > 0) {
        const Boss& boss = gm.bossPool[0];
        // Thanh mau boss o giua man hinh tren dinh - 1 widget Bar() duy nhat thay vi 3
        // loi goi DrawRectangle/DrawRectangleLines rieng le nhu truoc.
        float barW = 300.0f;
        float ratio = (boss.maxHp > 0) ? ((float)boss.hp / (float)boss.maxHp) : 0.0f;
        float barX = (Config::SCREEN_W - barW) / 2.0f;
        Color barFill = (boss.type == BossType::Sentinel && boss.shieldActive) ? SKYBLUE : RED;
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
