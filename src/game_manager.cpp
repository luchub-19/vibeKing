#include "game_manager.h"
#include <cmath>
#include <array>

// ==========================================
// TRANSITION (fade giữa các state)
// ==========================================
void GameManager::RequestTransition(GameState next) {
    pendingState = next;
    transitionPhase = TransitionPhase::FADE_OUT;
    transitionTimer = 0.0f;
}

void GameManager::UpdateTransition(float dt) {
    if (transitionPhase == TransitionPhase::NONE) return;
    transitionTimer += dt;

    if (transitionPhase == TransitionPhase::FADE_OUT && transitionTimer >= Config::TRANSITION_DURATION) {
        state = pendingState;
        transitionPhase = TransitionPhase::FADE_IN;
        transitionTimer = 0.0f;
    } else if (transitionPhase == TransitionPhase::FADE_IN && transitionTimer >= Config::TRANSITION_DURATION) {
        transitionPhase = TransitionPhase::NONE;
    }
}

float GameManager::GetTransitionAlpha() const {
    if (transitionPhase == TransitionPhase::FADE_OUT) return transitionTimer / Config::TRANSITION_DURATION;
    if (transitionPhase == TransitionPhase::FADE_IN) return 1.0f - transitionTimer / Config::TRANSITION_DURATION;
    return 0.0f;
}

// ==========================================
// LEVEL INIT
// ==========================================
void GameManager::InitLevel() {
    player.Reset();
    basicEnemies.Clear();
    tankyEnemies.Clear();
    zigzagEnemies.Clear();
    bunkers.clear();
    playerBullets.Reset();
    enemyBullets.Reset();
    particles.Reset();

    // Số hàng/cột + khoảng cách đọc từ level.cfg (LoadFromFile trong Run()) thay vì
    // hardcode r<4,c<10 - đổi độ hình chỉ cần sửa file cấu hình, không phải build lại.
    // Capacity của cả 3 pool được tính đúng theo công thức spawn này tại giới hạn lưới
    // tối đa (xem Config::MAX_*_ENEMIES) nên Spawn() không bao giờ thất bại ở đây.
    for (int r = 0; r < levelGrid.rows; r++) {
        for (int c = 0; c < levelGrid.cols; c++) {
            float x = levelGrid.startX + c * levelGrid.spacingX;
            float y = levelGrid.startY + r * levelGrid.spacingY;
            Color col = (r % 2 == 0) ? PURPLE : VIOLET;

            // Hàng đầu tiên là địch bay zig-zag (khó bắn trúng), cứ mỗi 5 cột có 1 địch
            // máu dày ở giữa đội hình, còn lại là địch thường - chọn đúng pool tĩnh
            // tương ứng thay vì tạo đối tượng đa hình trên heap.
            if (r == 0) {
                zigzagEnemies.Spawn(ZigzagEnemy{ {x, y, 36.0f, 22.0f}, SKYBLUE, c, 0.0f, 0.0f });
            } else if (c % 5 == 0) {
                tankyEnemies.Spawn(TankyEnemy{ {x, y, 44.0f, 30.0f}, MAROON, c, TankyEnemy::HP });
            } else {
                basicEnemies.Spawn(BasicEnemy{ {x, y, 40.0f, 25.0f}, col, c });
            }
        }
    }

    SpawnBunkers();

    DifficultyStats stats = GetDifficultyStats(difficulty);
    enemySpeed = stats.enemyBaseSpeed;
    enemyDirection = 1;
    enemyFireTimer = 0.0f;
    newHighScoreThisRun = false;
}

void GameManager::SpawnBunkers() {
    // 4 lá chắn voxel dàn đều phía trên player, dưới đội hình địch.
    const int bunkerCount = 4;
    const float bunkerY = 460.0f;
    const float margin = 80.0f;
    const float halfWidth = Bunker::DefaultWidth() / 2.0f;
    const float usableWidth = Config::SCREEN_W - margin * 2.0f;

    for (int i = 0; i < bunkerCount; i++) {
        float slotCenterX = margin + usableWidth * ((float)i + 0.5f) / (float)bunkerCount;
        bunkers.emplace_back(slotCenterX - halfWidth, bunkerY, GREEN);
    }
}

// ==========================================
// MENU
// ==========================================
void GameManager::UpdateMenu() {
    if (IsKeyPressed(KEY_LEFT))  difficulty = CycleDifficulty(difficulty, -1);
    if (IsKeyPressed(KEY_RIGHT)) difficulty = CycleDifficulty(difficulty, 1);
    if (IsKeyPressed(KEY_UP))   audio.SetVolume(audio.GetVolume() + 0.1f);
    if (IsKeyPressed(KEY_DOWN)) audio.SetVolume(audio.GetVolume() - 0.1f);

    if (IsKeyPressed(KEY_ENTER)) {
        InitLevel();
        RequestTransition(GameState::PLAYING);
    }
}

// ==========================================
// GAME_OVER / WIN
// ==========================================
void GameManager::UpdateEndScreen() {
    if (IsKeyPressed(KEY_ENTER)) RequestTransition(GameState::MENU);
    if (IsKeyPressed(KEY_R)) {
        InitLevel();
        RequestTransition(GameState::PLAYING);
    }
}

void GameManager::UpdatePaused() {
    if (IsKeyPressed(KEY_UP))   audio.SetVolume(audio.GetVolume() + 0.1f);
    if (IsKeyPressed(KEY_DOWN)) audio.SetVolume(audio.GetVolume() - 0.1f);
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) state = GameState::PLAYING;
}

// ==========================================
// PLAYING
// ==========================================
void GameManager::UpdatePlaying(float dt) {
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) { state = GameState::PAUSED; return; }
    if (IsKeyPressed(KEY_R)) { InitLevel(); return; }

    screenShake.Update(dt);
    particles.Update(dt);

    if (player.Update(dt, playerBullets)) {
        audio.PlayShoot();
    }

    UpdateEnemies(dt);
    if (state != GameState::PLAYING) return; // UpdateEnemies có thể trigger WIN/GAME_OVER

    playerBullets.Update(dt);
    enemyBullets.Update(dt);
    CheckCollisions();

    DifficultyStats stats = GetDifficultyStats(difficulty);
    audio.UpdateBassline(dt, enemySpeed, stats.enemySpeedMax);

    if (player.GetLives() <= 0) {
        audio.PlayGameOver();
        newHighScoreThisRun = highScore.TrySubmit(player.GetScore());
        RequestTransition(GameState::GAME_OVER);
    }
}

void GameManager::UpdateEnemies(float dt) {
    bool hitEdge = false;

    // Basic/Tanky: chỉ áp dụng đội hình di chuyển ngang, không có hành vi phụ.
    for (size_t i = 0; i < basicEnemies.Size(); i++) {
        Rectangle& r = basicEnemies[i].rect;
        r.x += enemyDirection * enemySpeed * dt;
        if (r.x <= 0 || r.x + r.width >= Config::SCREEN_W) hitEdge = true;
    }
    for (size_t i = 0; i < tankyEnemies.Size(); i++) {
        Rectangle& r = tankyEnemies[i].rect;
        r.x += enemyDirection * enemySpeed * dt;
        if (r.x <= 0 || r.x + r.width >= Config::SCREEN_W) hitEdge = true;
    }
    // Zigzag: hành vi riêng (dao động sin) trước khi áp đội hình ngang.
    for (size_t i = 0; i < zigzagEnemies.Size(); i++) {
        ZigzagEnemy& e = zigzagEnemies[i];
        e.Update(dt);
        e.rect.x += enemyDirection * enemySpeed * dt;
        if (e.rect.x <= 0 || e.rect.x + e.rect.width >= Config::SCREEN_W) hitEdge = true;
    }

    size_t activeCount = basicEnemies.Size() + tankyEnemies.Size() + zigzagEnemies.Size();
    if (activeCount == 0) {
        audio.PlayWin();
        newHighScoreThisRun = highScore.TrySubmit(player.GetScore());
        RequestTransition(GameState::WIN);
        return;
    }

    DifficultyStats stats = GetDifficultyStats(difficulty);

    if (hitEdge) {
        enemyDirection *= -1;
        enemySpeed = fminf(enemySpeed + Config::ENEMY_SPEED_INC, stats.enemySpeedMax);

        for (size_t i = 0; i < basicEnemies.Size(); i++) {
            Rectangle& r = basicEnemies[i].rect;
            r.y += 20.0f;
            if (r.x < 0) r.x = 0;
            if (r.x + r.width > Config::SCREEN_W) r.x = Config::SCREEN_W - r.width;
            if (EnemyBottom(r) >= player.GetY()) {
                audio.PlayGameOver();
                newHighScoreThisRun = highScore.TrySubmit(player.GetScore());
                RequestTransition(GameState::GAME_OVER);
                return;
            }
        }
        for (size_t i = 0; i < tankyEnemies.Size(); i++) {
            Rectangle& r = tankyEnemies[i].rect;
            r.y += 20.0f;
            if (r.x < 0) r.x = 0;
            if (r.x + r.width > Config::SCREEN_W) r.x = Config::SCREEN_W - r.width;
            if (EnemyBottom(r) >= player.GetY()) {
                audio.PlayGameOver();
                newHighScoreThisRun = highScore.TrySubmit(player.GetScore());
                RequestTransition(GameState::GAME_OVER);
                return;
            }
        }
        for (size_t i = 0; i < zigzagEnemies.Size(); i++) {
            Rectangle& r = zigzagEnemies[i].rect;
            r.y += 20.0f;
            if (r.x < 0) r.x = 0;
            if (r.x + r.width > Config::SCREEN_W) r.x = Config::SCREEN_W - r.width;
            if (EnemyBottom(r) >= player.GetY()) {
                audio.PlayGameOver();
                newHighScoreThisRun = highScore.TrySubmit(player.GetScore());
                RequestTransition(GameState::GAME_OVER);
                return;
            }
        }
    }

    enemyFireTimer += dt;
    if (enemyFireTimer >= stats.enemyFireRate) {
        enemyFireTimer = 0.0f;

        // AI "line of sight": nhóm địch theo cột gán lúc spawn, xét CHUNG cả 3 pool.
        // Trong mỗi cột, chỉ địch nằm THẤP NHẤT (không bị đồng đội cùng cột che phía
        // trước) mới được cấp quyền bắn - random chọn 1 trong các "tiền tuyến" đó.
        struct Frontline { EnemyKind kind; int index; float bottom; };
        std::vector<Frontline> frontlinePerColumn(levelGrid.cols, { EnemyKind::Basic, -1, 0.0f });

        auto considerColumn = [&](EnemyKind kind, int column, int index, float bottom) {
            if (column < 0 || column >= levelGrid.cols) return;
            Frontline& best = frontlinePerColumn[column];
            if (best.index == -1 || bottom > best.bottom) {
                best = { kind, index, bottom };
            }
        };

        for (size_t i = 0; i < basicEnemies.Size(); i++) {
            considerColumn(EnemyKind::Basic, basicEnemies[i].column, (int)i, EnemyBottom(basicEnemies[i].rect));
        }
        for (size_t i = 0; i < tankyEnemies.Size(); i++) {
            considerColumn(EnemyKind::Tanky, tankyEnemies[i].column, (int)i, EnemyBottom(tankyEnemies[i].rect));
        }
        for (size_t i = 0; i < zigzagEnemies.Size(); i++) {
            considerColumn(EnemyKind::Zigzag, zigzagEnemies[i].column, (int)i, EnemyBottom(zigzagEnemies[i].rect));
        }

        std::vector<Frontline> shooters;
        for (const Frontline& f : frontlinePerColumn) {
            if (f.index != -1) shooters.push_back(f);
        }

        if (!shooters.empty()) {
            const Frontline& pick = shooters[GetRandomValue(0, (int)shooters.size() - 1)];
            Rectangle rect{};
            switch (pick.kind) {
                case EnemyKind::Basic:  rect = basicEnemies[pick.index].rect;  break;
                case EnemyKind::Tanky:  rect = tankyEnemies[pick.index].rect;  break;
                case EnemyKind::Zigzag: rect = zigzagEnemies[pick.index].rect; break;
            }
            EnemyShoot(EnemyCenterX(rect), EnemyBottomY(rect));
        }
    }
}

void GameManager::EnemyShoot(float x, float y) {
    enemyBullets.Fire(x, y, Config::ENEMY_BULLET_SPEED);
}

void GameManager::CheckCollisions() {
    // Băm lại enemy đang sống vào lưới không gian mỗi frame (O(M)) - đổi lại mỗi viên
    // đạn chỉ cần test va chạm với enemy nằm trong (các) ô nó phủ tới, thay vì toàn bộ
    // danh sách enemy (dẹp vòng lặp lồng nhau O(N_bullet x M_enemy) trước đây). Mỗi
    // loại địch có 1 grid riêng, khớp với 3 Pool tĩnh.
    basicGrid.Clear();
    tankyGrid.Clear();
    zigzagGrid.Clear();
    for (size_t i = 0; i < basicEnemies.Size(); i++)  basicGrid.Insert((int)i, basicEnemies[i].rect);
    for (size_t i = 0; i < tankyEnemies.Size(); i++)  tankyGrid.Insert((int)i, tankyEnemies[i].rect);
    for (size_t i = 0; i < zigzagEnemies.Size(); i++) zigzagGrid.Insert((int)i, zigzagEnemies[i].rect);

    // Swap-and-pop đá văng phần tử chết bằng cách ghi đè nó bởi phần tử CUỐI trong pool
    // - nghĩa là index của phần tử cuối đó thay đổi ngay lập tức. Nếu xóa thẳng trong
    // lúc đang duyệt candidates (vốn lấy từ grid đã băm 1 lần ở trên cho CẢ frame), 1
    // enemy còn sống có thể bị "biến mất" khỏi việc dò va chạm cho phần còn lại của
    // frame này (nó bị hoán đổi sang 1 index khác với ô mà grid đã ghi nhận). Do đó ở
    // đây chỉ ĐÁNH DẤU chết (pendingKill, biến cục bộ theo frame - không phải cờ active
    // tồn tại lâu dài trên từng phần tử), rồi mới quét & swap-and-pop 1 lượt DUY NHẤT
    // sau khi đã xử lý xong toàn bộ đạn của frame này.
    std::array<bool, Config::MAX_BASIC_ENEMIES>  basicPendingKill{};
    std::array<bool, Config::MAX_TANKY_ENEMIES>  tankyPendingKill{};
    std::array<bool, Config::MAX_ZIGZAG_ENEMIES> zigzagPendingKill{};

    std::vector<int> candidates; // Tái dùng buffer cho mọi query, tránh cấp phát lặp lại

    // Đạn player: kiểm tra bunker trước (chặn đạn), rồi mới tới enemy trong ô lân cận
    for (size_t i = 0; i < playerBullets.GetActiveCount(); ) {
        Rectangle bulletRect = playerBullets.GetBullet(i).GetRect();
        bool consumed = false;

        for (auto& bunker : bunkers) {
            if (bunker.HandleBulletHit(bulletRect)) {
                playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        // Trúng đòn hạ gục ngay (Basic/Zigzag, luôn 1 máu) - đánh dấu pendingKill, cộng
        // điểm/hiệu ứng ngay (không phụ thuộc index nên an toàn để làm ngay lập tức).
        if (!consumed) {
            basicGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (basicPendingKill[idx]) continue; // Đã bị bullet khác trong frame này hạ rồi
                BasicEnemy& e = basicEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                basicPendingKill[idx] = true;
                particles.Burst(EnemyCenter(e.rect), 14, e.color);
                audio.PlayExplosion();
                screenShake.Trigger(0.12f, 4.0f);
                player.AddScore(BasicEnemy::SCORE_VALUE);

                playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        if (!consumed) {
            zigzagGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (zigzagPendingKill[idx]) continue;
                ZigzagEnemy& e = zigzagEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                zigzagPendingKill[idx] = true;
                particles.Burst(EnemyCenter(e.rect), 14, e.color);
                audio.PlayExplosion();
                screenShake.Trigger(0.12f, 4.0f);
                player.AddScore(ZigzagEnemy::SCORE_VALUE);

                playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        // Tanky: nhiều máu hơn - trừ hp ngay (an toàn, không đụng tới index), chỉ đánh
        // dấu pendingKill khi hp thật sự về 0.
        if (!consumed) {
            tankyGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (tankyPendingKill[idx]) continue;
                TankyEnemy& e = tankyEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                if (e.hp > 0) e.hp--;
                if (e.hp <= 0) {
                    tankyPendingKill[idx] = true;
                    particles.Burst(EnemyCenter(e.rect), 14, e.color);
                    audio.PlayExplosion();
                    screenShake.Trigger(0.12f, 4.0f);
                    player.AddScore(TankyEnemy::SCORE_VALUE);
                } else {
                    // Địch máu dày vẫn còn sống sau đòn này - phản hồi nhẹ hơn để phân
                    // biệt với đòn hạ gục hẳn
                    audio.PlayHit();
                    screenShake.Trigger(0.05f, 2.0f);
                }

                playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        if (!consumed) i++;
    }

    // Quét & swap-and-pop 1 lượt duy nhất sau khi đã xử lý xong toàn bộ đạn của frame
    // này - lúc này việc index bị hoán đổi không còn ảnh hưởng gì tới vòng lặp trên nữa.
    for (size_t i = 0; i < basicEnemies.Size(); ) {
        if (basicPendingKill[i]) {
            basicPendingKill[i] = basicPendingKill[basicEnemies.Size() - 1];
            basicEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < tankyEnemies.Size(); ) {
        if (tankyPendingKill[i]) {
            tankyPendingKill[i] = tankyPendingKill[tankyEnemies.Size() - 1];
            tankyEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < zigzagEnemies.Size(); ) {
        if (zigzagPendingKill[i]) {
            zigzagPendingKill[i] = zigzagPendingKill[zigzagEnemies.Size() - 1];
            zigzagEnemies.Destroy(i);
        } else {
            i++;
        }
    }

    // Đạn enemy: kiểm tra bunker trước, rồi mới tới player
    for (size_t i = 0; i < enemyBullets.GetActiveCount(); ) {
        Rectangle bulletRect = enemyBullets.GetBullet(i).GetRect();
        bool consumed = false;

        for (auto& bunker : bunkers) {
            if (bunker.HandleBulletHit(bulletRect)) {
                enemyBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        if (!consumed && CheckCollisionRecs(bulletRect, player.GetRect())) {
            enemyBullets.Destroy(i);
            consumed = true;
            if (player.TakeDamage()) { // false nếu đang bất tử -> không hiệu ứng thừa
                particles.Burst(player.GetCenter(), 18, RED);
                audio.PlayHit();
                screenShake.Trigger(0.22f, 8.0f);
            }
        }

        if (!consumed) i++;
    }
}

// ==========================================
// DRAW
// ==========================================
void GameManager::DrawMenu() const {
    DrawText("SPACE INVADERS", 250, 160, 40, GREEN);
    DrawText(TextFormat("HIGH SCORE: %d", highScore.Get()), 300, 220, 20, YELLOW);

    DifficultyStats stats = GetDifficultyStats(difficulty);
    DrawText(TextFormat("< DIFFICULTY: %s >", stats.label), 260, 280, 20, WHITE);
    DrawText(TextFormat("VOLUME: %d%%  (UP/DOWN)", (int)(audio.GetVolume() * 100)), 260, 320, 18, GRAY);

    DrawText("PRESS ENTER TO START", 240, 380, 20, WHITE);
    DrawText("LEFT/RIGHT: CHANGE DIFFICULTY", 230, 410, 16, GRAY);
}

void GameManager::DrawEndScreen() const {
    bool win = (state == GameState::WIN);
    DrawText(win ? "YOU WIN!" : "GAME OVER", 300, 180, 40, win ? YELLOW : RED);
    DrawText(TextFormat("FINAL SCORE: %d", player.GetScore()), 300, 240, 20, WHITE);
    if (newHighScoreThisRun) {
        DrawText("NEW HIGH SCORE!", 300, 270, 20, YELLOW);
    } else {
        DrawText(TextFormat("HIGH SCORE: %d", highScore.Get()), 300, 270, 18, GRAY);
    }
    DrawText("ENTER: MENU   R: RESTART", 220, 330, 20, GRAY);
}

void GameManager::DrawPlaying() const {
    Camera2D cam{};
    cam.offset = screenShake.GetOffset();
    cam.target = { 0, 0 };
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;

    BeginMode2D(cam);
    for (size_t i = 0; i < basicEnemies.Size(); i++) {
        DrawRectangleRec(basicEnemies[i].rect, basicEnemies[i].color);
    }
    for (size_t i = 0; i < tankyEnemies.Size(); i++) {
        const TankyEnemy& e = tankyEnemies[i];
        DrawRectangleRec(e.rect, e.color);
        if (e.hp < TankyEnemy::HP) {
            // Địch máu dày bị thương -> viền sáng để người chơi thấy rõ đã gây sát thương
            DrawRectangleLinesEx(e.rect, 2.0f, WHITE);
        }
    }
    for (size_t i = 0; i < zigzagEnemies.Size(); i++) {
        DrawRectangleRec(zigzagEnemies[i].rect, zigzagEnemies[i].color);
    }
    for (const auto& bunker : bunkers) bunker.Draw();
    playerBullets.Draw(YELLOW);
    enemyBullets.Draw(RED);
    particles.Draw();
    player.Draw();
    EndMode2D();

    // HUD vẽ ngoài camera để không bị rung theo
    DrawHUD();

    if (state == GameState::PAUSED) {
        DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(BLACK, 0.6f));
        DrawText("PAUSED", 330, 250, 40, WHITE);
        DrawText(TextFormat("VOLUME: %d%%  (UP/DOWN)", (int)(audio.GetVolume() * 100)), 280, 310, 18, GRAY);
        DrawText("P / ESC: RESUME", 300, 340, 18, GRAY);
    }
}

void GameManager::DrawHUD() const {
    DrawText(TextFormat("SCORE: %d", player.GetScore()), 10, 10, 20, WHITE);
    DrawText(TextFormat("LIVES: %d", player.GetLives()), 700, 10, 20, WHITE);
    DrawText("P: PAUSE   R: RESTART", 300, 10, 16, GRAY);
}

// ==========================================
// MAIN LOOP
// ==========================================
void GameManager::Run() {
    InitWindow(Config::SCREEN_W, Config::SCREEN_H, "Hardcore Space Invaders");
    SetTargetFPS(60);
    audio.Init();
    highScore.Load(Config::HighScoreFilePath());
    levelGrid = LevelGridConfig::LoadFromFile(Config::LevelConfigFilePath());

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > Config::MAX_DT) dt = Config::MAX_DT;

        UpdateTransition(dt);
        // Khi đang fade, đóng băng gameplay để không update/collision trong lúc màn hình đang mờ dần
        bool frozen = (transitionPhase != TransitionPhase::NONE);

        if (!frozen) {
            switch (state) {
                case GameState::MENU:     UpdateMenu(); break;
                case GameState::GAME_OVER:
                case GameState::WIN:      UpdateEndScreen(); break;
                case GameState::PAUSED:   UpdatePaused(); break;
                case GameState::PLAYING:  UpdatePlaying(dt); break;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        switch (state) {
            case GameState::MENU: DrawMenu(); break;
            case GameState::GAME_OVER:
            case GameState::WIN: DrawEndScreen(); break;
            case GameState::PLAYING:
            case GameState::PAUSED: DrawPlaying(); break;
        }

        float alpha = GetTransitionAlpha();
        if (alpha > 0.0f) {
            DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(BLACK, alpha));
        }

        EndDrawing();
    }

    audio.Shutdown();
    CloseWindow();
}
