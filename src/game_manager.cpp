#include "game_manager.h"
#include <cmath>

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
    enemies.clear();
    playerBullets.Reset();
    enemyBullets.Reset();
    particles.Reset();

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 10; c++) {
            enemies.emplace_back(
                (float)(65 + c * 60),
                (float)(50 + r * 40),
                r % 2 == 0 ? PURPLE : VIOLET
            );
        }
    }

    DifficultyStats stats = GetDifficultyStats(difficulty);
    enemySpeed = stats.enemyBaseSpeed;
    enemyDirection = 1;
    enemyFireTimer = 0.0f;
    newHighScoreThisRun = false;
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
    int activeCount = 0;

    for (auto& e : enemies) {
        if (!e.IsActive()) continue;
        activeCount++;
        e.MoveX(enemyDirection * enemySpeed * dt);
        if (e.GetX() <= 0 || e.GetX() + e.GetWidth() >= Config::SCREEN_W) hitEdge = true;
    }

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
        for (auto& e : enemies) {
            if (!e.IsActive()) continue;
            e.MoveY(20.0f);
            if (e.GetX() < 0) e.ForceX(0);
            if (e.GetX() + e.GetWidth() > Config::SCREEN_W) e.ForceX(Config::SCREEN_W - e.GetWidth());

            if (e.GetBottom() >= player.GetY()) {
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
        int shooterIndex = -1;
        int randomPick = GetRandomValue(0, activeCount - 1);
        for (size_t i = 0; i < enemies.size(); i++) {
            if (enemies[i].IsActive()) {
                if (randomPick == 0) { shooterIndex = (int)i; break; }
                randomPick--;
            }
        }
        if (shooterIndex != -1) {
            EnemyShoot(enemies[shooterIndex].GetCenterX(), enemies[shooterIndex].GetBottomY());
        }
    }
}

void GameManager::EnemyShoot(float x, float y) {
    enemyBullets.Fire(x, y, Config::ENEMY_BULLET_SPEED);
}

void GameManager::CheckCollisions() {
    // Đạn player vs enemy (swap-and-pop, không i++ khi vừa hủy)
    for (size_t i = 0; i < playerBullets.GetActiveCount(); ) {
        bool bulletHit = false;
        for (auto& e : enemies) {
            if (e.IsActive() && CheckCollisionRecs(playerBullets.GetBullet(i).GetRect(), e.GetRect())) {
                particles.Burst(e.GetCenter(), 14, e.GetColor());
                audio.PlayExplosion();
                screenShake.Trigger(0.12f, 4.0f);

                e.Destroy();
                playerBullets.Destroy(i);
                player.AddScore(10);
                bulletHit = true;
                break;
            }
        }
        if (!bulletHit) i++;
    }

    // Đạn enemy vs player
    for (size_t i = 0; i < enemyBullets.GetActiveCount(); ) {
        if (CheckCollisionRecs(enemyBullets.GetBullet(i).GetRect(), player.GetRect())) {
            enemyBullets.Destroy(i);
            if (player.TakeDamage()) { // false nếu đang bất tử -> không hiệu ứng thừa
                particles.Burst(player.GetCenter(), 18, RED);
                audio.PlayHit();
                screenShake.Trigger(0.22f, 8.0f);
            }
        } else {
            i++;
        }
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
    for (const auto& e : enemies) e.Draw();
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
