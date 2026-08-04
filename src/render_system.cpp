#include "render_system.h"
#include "game_manager.h"
#include "ui_system.h"
#include "culling.h"
#include "process_metrics.h"
#include "input_system.h"

void RenderSystem::DrawMenu(const GameManager& gm) {
    UICanvas canvas;
    canvas.Text(250, 100, 40, GREEN, "SPACE INVADERS");

    // LEADERBOARD: hien thi toi da Config::LEADERBOARD_MAX_ENTRIES dong, xep hang tu
    // 1, kem wave da dat duoc (khong chi mot con so diem tran trui nhu HighScore cu).
    const auto& entries = gm.leaderboard.GetEntries();
    canvas.Text(350, 165, 22, YELLOW, "TOP 10");
    if (entries.empty()) {
        canvas.Text(290, 195, 16, GRAY, "(chua co ky luc nao)");
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
    canvas.Text(260, bottomY, 20, WHITE, TextFormat("< DIFFICULTY: %s >", stats.label));
    canvas.Text(260, bottomY + 35, 18, GRAY, TextFormat("VOLUME: %d%%  (UP/DOWN)", (int)(gm.audio.GetVolume() * 100)));
    canvas.Text(240, bottomY + 80, 20, WHITE, "PRESS ENTER TO START");
    canvas.Text(230, bottomY + 108, 16, GRAY, "LEFT/RIGHT: CHANGE DIFFICULTY");
    canvas.Text(300, bottomY + 130, 16, GRAY, "F11: FULLSCREEN");

    canvas.Draw(gm.gameFont);
}

void RenderSystem::DrawEndScreen(const GameManager& gm) {
    UICanvas canvas;
    bool waveClear = (gm.state == GameState::WAVE_CLEAR);
    if (waveClear) {
        canvas.Text(240, 180, 36, YELLOW, TextFormat("WAVE %d CLEARED!", gm.wave - 1));
        canvas.Text(300, 240, 20, WHITE, TextFormat("SCORE: %d", gm.player.GetScore()));
        canvas.Text(210, 300, 20, GRAY, "ENTER: NEXT WAVE   R: RESTART");
    } else {
        canvas.Text(300, 180, 40, RED, "GAME OVER");
        canvas.Text(190, 240, 20, WHITE, TextFormat("FINAL SCORE: %d   WAVE REACHED: %d", gm.player.GetScore(), gm.wave));

        // 3 trang thai ro rang thay vi 1 bool "co pha ky luc hay khong": NewRecord (giờ
        // la #1), MadeTop10 (lot danh sach nhung khong phai #1), hoac khong lot top nao
        // ca (van hien diem cao nhat hien tai de nguoi choi biet minh con thieu bao nhieu).
        if (gm.lastSubmitResult == SubmitResult::NewRecord) {
            canvas.Text(300, 270, 20, YELLOW, "KY LUC MOI! (#1)");
        } else if (gm.lastSubmitResult == SubmitResult::MadeTop10) {
            canvas.Text(300, 270, 20, LIME, "LOT TOP 10!");
        } else {
            canvas.Text(300, 270, 18, GRAY, TextFormat("TOP SCORE: %d", gm.leaderboard.GetTopScore()));
        }
        canvas.Text(220, 330, 20, GRAY, "ENTER: MENU   R: RESTART");
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
    gm.powerUps.Draw();
    gm.player.Draw(gm.sprites.player);
    EndMode2D();

    // HUD ve ngoai camera de khong bi rung theo
    DrawHUD(gm);

    if (gm.state == GameState::PAUSED) {
        DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(BLACK, 0.6f));
        UICanvas canvas;
        canvas.Text(330, 250, 40, WHITE, "PAUSED");
        canvas.Text(280, 310, 18, GRAY, TextFormat("VOLUME: %d%%  (UP/DOWN)", (int)(gm.audio.GetVolume() * 100)));
        canvas.Text(250, 340, 18, GRAY, "P / ESC: RESUME   F11: FULLSCREEN   K: DOI PHIM DIEU KHIEN");
        canvas.Draw(gm.gameFont);
    } else if (gm.state == GameState::KEYBIND) {
        DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(BLACK, 0.75f));
        UICanvas canvas;
        canvas.Text(230, 90, 32, WHITE, "DOI PHIM DIEU KHIEN");

        const RebindableAction* actions = GetRebindableActions();
        for (int i = 0; i < REBINDABLE_ACTION_COUNT; i++) {
            bool isBeingRebound = (gm.rebindingActionIndex == i);
            Color rowColor = isBeingRebound ? YELLOW : WHITE;
            int currentKey = gm.settings.*(actions[i].keyField);
            std::string line = TextFormat("%d) %-6s: %s", i + 1, actions[i].label,
                                           isBeingRebound ? "..." : InputSystem::KeyName(currentKey));
            canvas.Text(280, 160 + i * 36, 22, rowColor, line);
        }

        if (gm.rebindingActionIndex >= 0) {
            canvas.Text(180, 340, 18, YELLOW,
                        TextFormat("Nhan phim moi cho '%s'... (ESC de huy)",
                                   actions[gm.rebindingActionIndex].label));
        } else {
            canvas.Text(180, 340, 16, GRAY, "Bam 1-4 de doi 1 phim.  0 hoac R: khoi phuc mac dinh.  ESC: quay lai");
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
        canvas.Text((int)barX, 24, 14, barFill, TextFormat("BOSS - %s", BossTypeName(boss.type)));
        if (boss.type == BossType::Sentinel && boss.shieldActive) {
            canvas.Text((int)(barX + barW - 60.0f), 24, 14, SKYBLUE, "KHIEN!");
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
