#include "game_manager.h"
#include <cmath>
#include <array>

// ==========================================
// TRANSITION (fade giua cac state)
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
// SETTINGS
// ==========================================
void GameManager::SaveSettings() {
    settings.difficulty = difficulty;
    settings.volume = audio.GetVolume();
    settings.SaveToFile(Config::SettingsFilePath());
}

// ==========================================
// LEVEL INIT
// ==========================================
void GameManager::InitLevel(bool newGame) {
    if (newGame) {
        wave = 1;
        player.Reset();
    } else {
        player.ResetForNewWave();
    }

    basicEnemies.Clear();
    tankyEnemies.Clear();
    zigzagEnemies.Clear();
    kamikazeEnemies.Clear();
    bunkers.clear();
    playerBullets.Reset();
    enemyBullets.Reset();
    particles.Reset();
    powerUps.Reset();
    comboTimer = 0.0f;
    comboCount = 0;
    ufoActive = false;
    RollNextUfoTimer();
    RollNextKamikazeTimer();
    bossActive = false;

    // BOSS WAVE: cu moi Config::BOSS_WAVE_INTERVAL wave, thay THE HOAN TOAN luoi doi
    // hinh thuong bang 1 Boss duy nhat - wave chi duoc coi la "don sach" khi boss bi ha
    // (xem UpdateEnemies/UpdateBoss), khong phai khi activeCount==0.
    isBossWave = (wave % Config::BOSS_WAVE_INTERVAL == 0);

    if (isBossWave) {
        SpawnBoss();
    } else {
        // WAVE PROGRESSION: cu moi Config::WAVE_EXTRA_ROW_EVERY wave thi them 1 hang
        // dich, clamp theo gioi han pool tinh toi da (Config::MAX_GRID_ROWS) - khong
        // bao gio vuot capacity da tinh san cho 3 EnemyPool.
        int extraRows = (wave - 1) / Config::WAVE_EXTRA_ROW_EVERY;
        int rows = levelGrid.rows + extraRows;
        if (rows > Config::MAX_GRID_ROWS) rows = Config::MAX_GRID_ROWS;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < levelGrid.cols; c++) {
                float x = levelGrid.startX + c * levelGrid.spacingX;
                float y = levelGrid.startY + r * levelGrid.spacingY;
                Color col = (r % 2 == 0) ? PURPLE : VIOLET;

                if (r == 0) {
                    zigzagEnemies.Spawn(ZigzagEnemy{ {x, y, 36.0f, 22.0f}, SKYBLUE, c, 0.0f, 0.0f });
                } else if (c % 5 == 0) {
                    tankyEnemies.Spawn(TankyEnemy{ {x, y, 44.0f, 30.0f}, MAROON, c, TankyEnemy::HP });
                } else {
                    basicEnemies.Spawn(BasicEnemy{ {x, y, 40.0f, 25.0f}, col, c });
                }
            }
        }
    }

    SpawnBunkers();

    DifficultyStats stats = GetDifficultyStats(difficulty);
    float waveSpeedBonus = Config::WAVE_SPEED_BONUS_PER * (float)(wave - 1);
    enemySpeed = fminf(stats.enemyBaseSpeed + waveSpeedBonus, stats.enemySpeedMax);
    enemyDirection = 1;
    enemyFireTimer = 0.0f;
    lastSubmitResult = SubmitResult::NotQualified;

    // Moi wave ban nhanh hon 1 chut, clamp toi thieu Config::WAVE_FIRE_RATE_MIN_MUL de
    // khong bao gio bien thanh "dan bay ra day man hinh" khong the choi noi.
    waveFireRateMul = 1.0f - Config::WAVE_FIRE_RATE_STEP * (float)(wave - 1);
    if (waveFireRateMul < Config::WAVE_FIRE_RATE_MIN_MUL) waveFireRateMul = Config::WAVE_FIRE_RATE_MIN_MUL;
}

void GameManager::SpawnBunkers() {
    // 4 la chan voxel dan deu phia tren player, duoi doi hinh dich.
    const int bunkerCount = 4;
    const float halfWidth = Bunker::DefaultWidth() / 2.0f;
    const float usableWidth = Config::SCREEN_W - Config::BUNKER_MARGIN_X * 2.0f;

    for (int i = 0; i < bunkerCount; i++) {
        float slotCenterX = Config::BUNKER_MARGIN_X + usableWidth * ((float)i + 0.5f) / (float)bunkerCount;
        bunkers.emplace_back(slotCenterX - halfWidth, Config::BUNKER_Y, GREEN);
    }
}

void GameManager::MaybeDropPowerUp(Vector2 at) {
    if ((float)GetRandomValue(0, 999) / 1000.0f >= Config::POWERUP_DROP_CHANCE) return;
    // 4 loai deu nhau: RapidFire, Shield, Piercing, Cleanser
    PowerUpType type = (PowerUpType)GetRandomValue(0, 3);
    Rectangle rect{ at.x - Config::POWERUP_SIZE / 2.0f, at.y, Config::POWERUP_SIZE, Config::POWERUP_SIZE };
    powerUps.Spawn(PowerUp{ rect, type });
}

int GameManager::ApplyComboAndScore(int baseScore) {
    // Ha guc them 1 dich trong luc combo timer con hieu luc -> tang bac combo; het
    // thoi gian (khong ha them dich nao) -> combo tu dong reset ve 0 (xem UpdatePlaying).
    if (comboTimer > 0.0f) comboCount++;
    else comboCount = 1;
    comboTimer = Config::COMBO_WINDOW;

    int steps = comboCount - 1;
    if (steps > Config::COMBO_MAX_STEPS) steps = Config::COMBO_MAX_STEPS;
    int finalScore = (int)((float)baseScore * (1.0f + Config::COMBO_BONUS_PER_STEP * (float)steps));
    player.AddScore(finalScore);
    return finalScore;
}

// ==========================================
// MYSTERY SHIP (UFO)
// ==========================================
void GameManager::RollNextUfoTimer() {
    int minMs = (int)(Config::UFO_SPAWN_MIN_INTERVAL * 1000.0f);
    int maxMs = (int)(Config::UFO_SPAWN_MAX_INTERVAL * 1000.0f);
    ufoSpawnTimer = (float)GetRandomValue(minMs, maxMs) / 1000.0f;
}

void GameManager::SpawnUfo() {
    ufoDirection = (GetRandomValue(0, 1) == 0) ? 1 : -1;
    float startXPos = (ufoDirection > 0) ? -Config::UFO_WIDTH : (float)Config::SCREEN_W;
    ufoRect = { startXPos, Config::UFO_Y, Config::UFO_WIDTH, Config::UFO_HEIGHT };
    ufoScoreValue = GetRandomValue(Config::UFO_SCORE_MIN, Config::UFO_SCORE_MAX);
    ufoActive = true;
    audio.PlayUfoAppear();
}

void GameManager::UpdateUfo(float dt) {
    if (!ufoActive) {
        ufoSpawnTimer -= dt;
        if (ufoSpawnTimer <= 0.0f) SpawnUfo();
        return;
    }

    ufoRect.x += (float)ufoDirection * Config::UFO_SPEED * dt;

    bool exitedRight = (ufoDirection > 0 && ufoRect.x > Config::SCREEN_W);
    bool exitedLeft  = (ufoDirection < 0 && ufoRect.x + ufoRect.width < 0);
    if (exitedRight || exitedLeft) {
        ufoActive = false;
        RollNextUfoTimer(); // Bay het man hinh ma khong bi ban trung -> hen lan sau
    }
}

// ==========================================
// KAMIKAZE
// ==========================================
void GameManager::RollNextKamikazeTimer() {
    int minMs = (int)(Config::KAMIKAZE_SPAWN_MIN_INTERVAL * 1000.0f);
    int maxMs = (int)(Config::KAMIKAZE_SPAWN_MAX_INTERVAL * 1000.0f);
    kamikazeSpawnTimer = (float)GetRandomValue(minMs, maxMs) / 1000.0f;
}

void GameManager::SpawnKamikaze() {
    float startX = (float)GetRandomValue(0, (int)Config::SCREEN_W);
    Rectangle rect{ startX - Config::KAMIKAZE_WIDTH / 2.0f, -Config::KAMIKAZE_HEIGHT,
                    Config::KAMIKAZE_WIDTH, Config::KAMIKAZE_HEIGHT };

    // Nham thang vao vi tri player NGAY LUC SPAWN (khong homing lien tuc sau do) - dan
    // co the ne duoc bang cach di chuyen, dung "khoa muc tieu vinh vien" kieu ho hen.
    Vector2 target = player.GetCenter();
    Vector2 origin = EnemyCenter(rect);
    Vector2 dir{ target.x - origin.x, target.y - origin.y };
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (len < 1.0f) len = 1.0f;
    Vector2 vel{ (dir.x / len) * Config::KAMIKAZE_SPEED, (dir.y / len) * Config::KAMIKAZE_SPEED };

    kamikazeEnemies.Spawn(KamikazeEnemy{ rect, RED, vel });
    RollNextKamikazeTimer();
}

void GameManager::UpdateKamikaze(float dt) {
    kamikazeSpawnTimer -= dt;
    if (kamikazeSpawnTimer <= 0.0f && kamikazeEnemies.Size() < Config::MAX_KAMIKAZE) {
        SpawnKamikaze();
    }

    for (size_t i = 0; i < kamikazeEnemies.Size(); ) {
        KamikazeEnemy& k = kamikazeEnemies[i];
        k.rect.x += k.vel.x * dt;
        k.rect.y += k.vel.y * dt;

        bool offscreen = k.rect.y > Config::SCREEN_H || k.rect.x < -Config::KAMIKAZE_WIDTH ||
                          k.rect.x > Config::SCREEN_W;
        if (offscreen) {
            kamikazeEnemies.Destroy(i); // Bay khoi man hinh ma khong trung ai -> bien mat, khong phat
            continue;
        }

        // Va cham truc tiep (lao vao) voi player - sat thuong kieu ho hen, khac han cac
        // loai dich khac (chi gay sat thuong qua dan cua chung, khong bao gio cham truc
        // tiep vao player).
        if (CheckCollisionRecs(k.rect, player.GetRect())) {
            particles.Burst(EnemyCenter(k.rect), 16, RED);
            audio.PlayExplosion();
            screenShake.Trigger(0.2f, 7.0f);
            if (player.TakeDamage()) {
                audio.PlayHit();
            }
            kamikazeEnemies.Destroy(i);
            continue;
        }

        i++;
    }
}

// ==========================================
// BOSS
// ==========================================
void GameManager::SpawnBoss() {
    int bossIndex = wave / Config::BOSS_WAVE_INTERVAL; // 1, 2, 3... (wave 5 -> 1, wave 10 -> 2...)
    int hp = Config::BOSS_MAX_HP + (int)(Config::BOSS_HP_PER_WAVE_BONUS * (float)(bossIndex - 1));

    boss.rect = { (Config::SCREEN_W - Config::BOSS_WIDTH) / 2.0f, Config::BOSS_Y,
                  Config::BOSS_WIDTH, Config::BOSS_HEIGHT };
    boss.hp = hp;
    boss.maxHp = hp;
    boss.direction = 1;
    boss.fireTimer = 0.0f;
    bossActive = true;
}

void GameManager::UpdateBoss(float dt) {
    if (!bossActive) return;

    int stage = boss.Stage();
    float speed = (stage == 1) ? Config::BOSS_SPEED_STAGE1 : (stage == 2) ? Config::BOSS_SPEED_STAGE2 : Config::BOSS_SPEED_STAGE3;
    float fireInterval = (stage == 1) ? Config::BOSS_FIRE_INTERVAL_STAGE1 : (stage == 2) ? Config::BOSS_FIRE_INTERVAL_STAGE2 : Config::BOSS_FIRE_INTERVAL_STAGE3;

    boss.rect.x += (float)boss.direction * speed * dt;
    if (boss.rect.x <= 0.0f || boss.rect.x + boss.rect.width >= Config::SCREEN_W) {
        boss.direction *= -1;
        boss.rect.x = fmaxf(0.0f, fminf(boss.rect.x, Config::SCREEN_W - boss.rect.width));
    }

    boss.fireTimer += dt;
    if (boss.fireTimer >= fireInterval) {
        boss.fireTimer = 0.0f;
        float originX = EnemyCenterX(boss.rect);
        float originY = boss.rect.y + boss.rect.height;

        float radialChance = (stage == 3) ? Config::BOSS_RADIAL_CHANCE_STAGE3
                            : (stage == 2) ? Config::BOSS_RADIAL_CHANCE_STAGE2 : 0.0f;
        if ((float)GetRandomValue(0, 999) / 1000.0f < radialChance) {
            FireRadialBurst(originX, originY, Config::RADIAL_BURST_COUNT, Config::BOSS_BULLET_SPEED);
        } else {
            Vector2 target = player.GetCenter();
            Vector2 dir{ target.x - originX, target.y - originY };
            float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (len < 1.0f) len = 1.0f;
            Vector2 vel{ (dir.x / len) * Config::BOSS_BULLET_SPEED, (dir.y / len) * Config::BOSS_BULLET_SPEED };
            enemyBullets.Fire(originX, originY, vel);
        }
    }
}

// ==========================================
// MENU
// ==========================================
void GameManager::UpdateMenu() {
    bool changed = false;
    if (IsKeyPressed(KEY_LEFT))  { difficulty = CycleDifficulty(difficulty, -1); changed = true; }
    if (IsKeyPressed(KEY_RIGHT)) { difficulty = CycleDifficulty(difficulty, 1); changed = true; }
    if (IsKeyPressed(KEY_UP))   { audio.SetVolume(audio.GetVolume() + 0.1f); changed = true; }
    if (IsKeyPressed(KEY_DOWN)) { audio.SetVolume(audio.GetVolume() - 0.1f); changed = true; }
    if (changed) SaveSettings();

    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

    if (IsKeyPressed(KEY_ENTER)) {
        InitLevel(true);
        RequestTransition(GameState::PLAYING);
    }
}

// ==========================================
// GAME_OVER / WAVE_CLEAR
// ==========================================
void GameManager::UpdateEndScreen() {
    if (state == GameState::WAVE_CLEAR) {
        if (IsKeyPressed(KEY_ENTER)) {
            InitLevel(false); // Giu diem/mang, sang wave ke tiep voi do kho cao hon
            RequestTransition(GameState::PLAYING);
        }
        if (IsKeyPressed(KEY_R)) {
            InitLevel(true); // Choi lai tu dau (wave 1, reset diem/mang)
            RequestTransition(GameState::PLAYING);
        }
        return;
    }

    // GAME_OVER
    if (IsKeyPressed(KEY_ENTER)) RequestTransition(GameState::MENU);
    if (IsKeyPressed(KEY_R)) {
        InitLevel(true);
        RequestTransition(GameState::PLAYING);
    }
}

void GameManager::UpdatePaused() {
    bool changed = false;
    if (IsKeyPressed(KEY_UP))   { audio.SetVolume(audio.GetVolume() + 0.1f); changed = true; }
    if (IsKeyPressed(KEY_DOWN)) { audio.SetVolume(audio.GetVolume() - 0.1f); changed = true; }
    if (changed) SaveSettings();
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) state = GameState::PLAYING;
}

// ==========================================
// PLAYING
// ==========================================
void GameManager::UpdatePlaying(float dt) {
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) { state = GameState::PAUSED; return; }
    if (IsKeyPressed(KEY_R)) { InitLevel(true); return; }
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

    screenShake.Update(dt);
    particles.Update(dt);
    powerUps.Update(dt, Config::POWERUP_FALL_SPEED, (float)Config::SCREEN_H);
    UpdateUfo(dt);
    UpdateKamikaze(dt);
    for (auto& bunker : bunkers) bunker.Update(dt); // Regen voxel + dao dong ngang

    if (comboTimer > 0.0f) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) comboCount = 0; // Het cua so combo ma khong ha them dich -> reset
    }

    if (player.Update(dt, playerBullets)) {
        audio.PlayShoot();
    }

    if (isBossWave) UpdateBoss(dt);
    else UpdateEnemies(dt);
    if (state != GameState::PLAYING) return; // UpdateEnemies/danh boss co the trigger WAVE_CLEAR/GAME_OVER

    playerBullets.Update(dt);
    enemyBullets.Update(dt);
    CheckCollisions();

    if (isBossWave && bossActive && boss.hp <= 0) {
        bossActive = false;
        audio.PlayBossDefeat();
        particles.Burst(EnemyCenter(boss.rect), 40, RED);
        screenShake.Trigger(0.4f, 12.0f);
        ApplyComboAndScore(Config::BOSS_SCORE_VALUE);
        wave++;
        lastSubmitResult = leaderboard.TrySubmit(player.GetScore(), wave);
        RequestTransition(GameState::WAVE_CLEAR);
        return;
    }

    if (!isBossWave) {
        DifficultyStats stats = GetDifficultyStats(difficulty);
        audio.UpdateBassline(dt, enemySpeed, stats.enemySpeedMax);
    }

    if (player.GetLives() <= 0) {
        audio.PlayGameOver();
        lastSubmitResult = leaderboard.TrySubmit(player.GetScore(), wave);
        RequestTransition(GameState::GAME_OVER);
    }
}

void GameManager::UpdateEnemies(float dt) {
    bool hitEdge = false;

    // Basic/Tanky: chi ap dung doi hinh di chuyen ngang, khong co hanh vi phu.
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
    // Zigzag: hanh vi rieng (dao dong sin) truoc khi ap doi hinh ngang.
    for (size_t i = 0; i < zigzagEnemies.Size(); i++) {
        ZigzagEnemy& e = zigzagEnemies[i];
        e.Update(dt);
        e.rect.x += enemyDirection * enemySpeed * dt;
        if (e.rect.x <= 0 || e.rect.x + e.rect.width >= Config::SCREEN_W) hitEdge = true;
    }
    // Kamikaze KHONG tham gia vong lap tren (pool + spawn logic hoan toan rieng, xem
    // UpdateKamikaze) - dung nhu yeu cau "khong pha hong logic kiem tra bien luoi doi hinh".

    size_t activeCount = basicEnemies.Size() + tankyEnemies.Size() + zigzagEnemies.Size();
    if (activeCount == 0) {
        // WAVE PROGRESSION: khong con la man hinh "WIN" cuoi cung - don sach 1 wave thi
        // sang wave ke tiep, kho hon, giu nguyen diem/mang (xem InitLevel(false)).
        audio.PlayWaveClear();
        wave++;
        lastSubmitResult = leaderboard.TrySubmit(player.GetScore(), wave);
        RequestTransition(GameState::WAVE_CLEAR);
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
                lastSubmitResult = leaderboard.TrySubmit(player.GetScore(), wave);
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
                lastSubmitResult = leaderboard.TrySubmit(player.GetScore(), wave);
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
                lastSubmitResult = leaderboard.TrySubmit(player.GetScore(), wave);
                RequestTransition(GameState::GAME_OVER);
                return;
            }
        }
    }

    enemyFireTimer += dt;
    if (enemyFireTimer >= stats.enemyFireRate * waveFireRateMul) {
        enemyFireTimer = 0.0f;

        // AI "line of sight": nhom dich theo cot gan luc spawn, xet CHUNG ca 3 pool.
        // Trong moi cot, chi dich nam THAP NHAT (khong bi dong doi cung cot che phia
        // truoc) moi duoc cap quyen ban - random chon 1 trong cac "tien tuyen" do.
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
    // Roll ngau nhien giua 2 pattern: thang xuong (kinh dien) hoac nham thang vi tri
    // player hien tai - "khai tu kieu dan chi biet roi thang theo truc Y" nhu yeu cau,
    // nhung van giu pattern co dien lam mac dinh (chiem da so) de khong bien moi phat
    // dan thuong thanh homing missile, mat dac trung "nhu mua" kinh dien cua the loai.
    if ((float)GetRandomValue(0, 999) / 1000.0f < Config::ENEMY_AIMED_SHOT_CHANCE) {
        Vector2 target = player.GetCenter();
        Vector2 dir{ target.x - x, target.y - y };
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (len < 1.0f) len = 1.0f;
        Vector2 vel{ (dir.x / len) * Config::ENEMY_BULLET_SPEED, (dir.y / len) * Config::ENEMY_BULLET_SPEED };
        enemyBullets.Fire(x, y, vel);
    } else {
        enemyBullets.Fire(x, y, { 0.0f, Config::ENEMY_BULLET_SPEED });
    }
}

void GameManager::FireRadialBurst(float x, float y, int count, float speed) {
    for (int i = 0; i < count; i++) {
        float angle = (2.0f * PI * (float)i) / (float)count;
        Vector2 vel{ cosf(angle) * speed, sinf(angle) * speed };
        enemyBullets.Fire(x, y, vel);
    }
}

void GameManager::CheckCollisions() {
    // Bam lai enemy dang song vao luoi khong gian moi frame (O(M)) - doi lai moi vien
    // dan chi can test va cham voi enemy nam trong (cac) o no phu toi, thay vi toan bo
    // danh sach enemy (dep vong lap long nhau O(N_bullet x M_enemy) truoc day). Moi
    // loai dich co 1 grid rieng, khop voi cac Pool tinh.
    basicGrid.Clear();
    tankyGrid.Clear();
    zigzagGrid.Clear();
    kamikazeGrid.Clear();
    for (size_t i = 0; i < basicEnemies.Size(); i++)    basicGrid.Insert((int)i, basicEnemies[i].rect);
    for (size_t i = 0; i < tankyEnemies.Size(); i++)    tankyGrid.Insert((int)i, tankyEnemies[i].rect);
    for (size_t i = 0; i < zigzagEnemies.Size(); i++)   zigzagGrid.Insert((int)i, zigzagEnemies[i].rect);
    for (size_t i = 0; i < kamikazeEnemies.Size(); i++) kamikazeGrid.Insert((int)i, kamikazeEnemies[i].rect);

    if (bossActive) {
        bossGrid.Clear();
        bossGrid.Insert(0, boss.rect); // Rect lon hon 1 o -> tu dong dang ky vao NHIEU o (xem SpatialGrid::Insert)
    }

    // Swap-and-pop da vang phan tu chet bang cach ghi de no boi phan tu CUOI trong pool
    // - nghia la index cua phan tu cuoi do thay doi ngay lap tuc. Neu xoa thang trong
    // luc dang duyet candidates (von lay tu grid da bam 1 lan o tren cho CA frame), 1
    // enemy con song co the bi "bien mat" khoi viec do va cham cho phan con lai cua
    // frame nay (no bi hoan doi sang 1 index khac voi o ma grid da ghi nhan). Do do o
    // day chi DANH DAU chet (pendingKill, bien cuc bo theo frame - khong phai co active
    // ton tai lau dai tren tung phan tu), roi moi quet & swap-and-pop 1 luot DUY NHAT
    // sau khi da xu ly xong toan bo dan cua frame nay.
    std::array<bool, Config::MAX_BASIC_ENEMIES>  basicPendingKill{};
    std::array<bool, Config::MAX_TANKY_ENEMIES>  tankyPendingKill{};
    std::array<bool, Config::MAX_ZIGZAG_ENEMIES> zigzagPendingKill{};
    std::array<bool, Config::MAX_KAMIKAZE>       kamikazePendingKill{};

    std::vector<int> candidates; // Tai dung buffer cho moi query, tranh cap phat lap lai

    // Dan player: kiem tra bunker truoc (chan dan), roi moi toi enemy trong o lan can
    for (size_t i = 0; i < playerBullets.GetActiveCount(); ) {
        Bullet& bullet = playerBullets.GetBullet(i);
        // Dung swept rect (CCD) thay vi rect tinh o vi tri cuoi frame - xem
        // Bullet::GetSweptRect() de biet ly do (xuyen tao voxel/dich mong o toc do cao).
        Rectangle bulletRect = bullet.GetSweptRect();
        bool consumed = false; // Da xu ly xong bullet nay cho frame nay (dung kiem tra tiep cac pool khac)
        bool removed = false;  // Bullet THUC SU bi Destroy() (swap-and-pop) - quyet dinh co tang i hay khong

        for (auto& bunker : bunkers) {
            if (bunker.HandleBulletHit(bulletRect)) {
                playerBullets.Destroy(i);
                consumed = true;
                removed = true;
                break;
            }
        }

        // Trung don ha guc ngay (Basic/Zigzag, luon 1 mau) - danh dau pendingKill, cong
        // diem/hieu ung ngay (khong phu thuoc index nen an toan de lam ngay lap tuc).
        if (!consumed) {
            basicGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (basicPendingKill[idx]) continue; // Da bi bullet khac trong frame nay ha roi
                BasicEnemy& e = basicEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                basicPendingKill[idx] = true;
                particles.Burst(EnemyCenter(e.rect), 14, e.color);
                audio.PlayExplosion();
                screenShake.Trigger(0.12f, 4.0f);
                ApplyComboAndScore(BasicEnemy::SCORE_VALUE);
                MaybeDropPowerUp(EnemyCenter(e.rect));

                // PIERCING SHOT: neu dan con xuyen duoc, KHONG Destroy() - no van con o
                // dung index `i`, tiep tuc bay va co the trung them muc tieu khac o
                // frame sau. removed=false -> vong lap ben duoi se KHONG tang i sai
                // (van dung, vi bullet nay khong bi swap).
                removed = !bullet.ConsumePierce();
                if (removed) playerBullets.Destroy(i);
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
                ApplyComboAndScore(ZigzagEnemy::SCORE_VALUE);
                MaybeDropPowerUp(EnemyCenter(e.rect));

                removed = !bullet.ConsumePierce();
                if (removed) playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        // Tanky: nhieu mau hon - tru hp ngay (an toan, khong dung toi index), chi danh
        // dau pendingKill khi hp that su ve 0.
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
                    ApplyComboAndScore(TankyEnemy::SCORE_VALUE);
                    MaybeDropPowerUp(EnemyCenter(e.rect));
                } else {
                    // Dich mau day van con song sau don nay - phan hoi nhe hon de phan
                    // biet voi don ha guc han
                    audio.PlayHit();
                    screenShake.Trigger(0.05f, 2.0f);
                }

                removed = !bullet.ConsumePierce();
                if (removed) playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        // Kamikaze: 1 mau, ha guc ngay - grid rieng, khong lien quan gi toi luoi doi hinh.
        if (!consumed) {
            kamikazeGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (kamikazePendingKill[idx]) continue;
                KamikazeEnemy& e = kamikazeEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                kamikazePendingKill[idx] = true;
                particles.Burst(EnemyCenter(e.rect), 16, e.color);
                audio.PlayExplosion();
                screenShake.Trigger(0.14f, 5.0f);
                ApplyComboAndScore(KamikazeEnemy::SCORE_VALUE);
                MaybeDropPowerUp(EnemyCenter(e.rect));

                removed = !bullet.ConsumePierce();
                if (removed) playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        // Boss: nhieu mau nhat trong game - tru hp ngay, chi bao "chet" khi boss.hp<=0
        // (UpdatePlaying() phia tren se phat hien va xu ly WAVE_CLEAR).
        if (!consumed && bossActive) {
            bossGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                (void)idx; // Boss luon la index 0 duy nhat, chi dung candidates de biet co trung o nao khong
                if (!CheckCollisionRecs(bulletRect, boss.rect)) continue;

                if (boss.hp > 0) boss.hp--;
                particles.Burst(player.GetCenter(), 1, RED); // Placeholder rong - hieu ung that o duoi
                particles.Burst({ bulletRect.x, bulletRect.y }, 6, RED);
                if (boss.hp > 0) {
                    audio.PlayHit();
                    screenShake.Trigger(0.08f, 3.0f);
                }

                removed = !bullet.ConsumePierce();
                if (removed) playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        if (!removed) i++;
    }

    // Quet & swap-and-pop 1 luot duy nhat sau khi da xu ly xong toan bo dan cua frame
    // nay - luc nay viec index bi hoan doi khong con anh huong gi toi vong lap tren nua.
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
    for (size_t i = 0; i < kamikazeEnemies.Size(); ) {
        if (kamikazePendingKill[i]) {
            kamikazePendingKill[i] = kamikazePendingKill[kamikazeEnemies.Size() - 1];
            kamikazeEnemies.Destroy(i);
        } else {
            i++;
        }
    }

    // Dan enemy: kiem tra bunker truoc, roi moi toi player
    for (size_t i = 0; i < enemyBullets.GetActiveCount(); ) {
        Rectangle bulletRect = enemyBullets.GetBullet(i).GetSweptRect();
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
            if (player.TakeDamage()) { // false neu dang bat tu/co khien -> khong hieu ung thua
                particles.Burst(player.GetCenter(), 18, RED);
                audio.PlayHit();
                screenShake.Trigger(0.22f, 8.0f);
            }
        }

        if (!consumed) i++;
    }

    // Mystery Ship (UFO): kiem tra rieng vi khong nam trong grid/pool nao - chi 1 the
    // hien tai 1 thoi diem nen quet truc tiep khong anh huong hieu nang.
    if (ufoActive) {
        for (size_t i = 0; i < playerBullets.GetActiveCount(); i++) {
            if (CheckCollisionRecs(playerBullets.GetBullet(i).GetSweptRect(), ufoRect)) {
                bool removed = !playerBullets.GetBullet(i).ConsumePierce();
                if (removed) playerBullets.Destroy(i);
                ufoActive = false;
                particles.Burst(EnemyCenter(ufoRect), 20, RED);
                audio.PlayUfoHit();
                screenShake.Trigger(0.18f, 6.0f);
                ApplyComboAndScore(ufoScoreValue);
                RollNextUfoTimer();
                break;
            }
        }
    }

    // Power-up: player bay ngang qua la nhat, khong can bam nut rieng.
    for (size_t i = 0; i < powerUps.Size(); ) {
        if (CheckCollisionRecs(powerUps[i].rect, player.GetRect())) {
            switch (powerUps[i].type) {
                case PowerUpType::RapidFire:
                    player.GrantRapidFire(Config::POWERUP_RAPIDFIRE_DURATION);
                    audio.PlayPickup();
                    break;
                case PowerUpType::Shield:
                    player.GrantShield(Config::POWERUP_SHIELD_DURATION);
                    audio.PlayPickup();
                    break;
                case PowerUpType::Piercing:
                    player.GrantPiercing(Config::POWERUP_PIERCE_DURATION);
                    audio.PlayPickup();
                    break;
                case PowerUpType::Cleanser:
                    // Hieu ung TUC THI: xoa sach toan bo dan dich dang bay tren man
                    // hinh ngay luc nhat - "bom cuu nan" giua tinh huong nguy cap.
                    enemyBullets.Reset();
                    particles.Burst(player.GetCenter(), 30, LIME);
                    screenShake.Trigger(0.25f, 6.0f);
                    audio.PlayCleanser();
                    break;
            }
            powerUps.Destroy(i);
        } else {
            i++;
        }
    }
}

// ==========================================
// DRAW
// ==========================================
void GameManager::DrawMenu() const {
    DrawGameText("SPACE INVADERS", 250, 100, 40, GREEN);

    // LEADERBOARD: hien thi toi da Config::LEADERBOARD_MAX_ENTRIES dong, xep hang tu
    // 1, kem wave da dat duoc (khong chi mot con so diem tran trui nhu HighScore cu).
    const auto& entries = leaderboard.GetEntries();
    DrawGameText("TOP 10", 350, 165, 22, YELLOW);
    if (entries.empty()) {
        DrawGameText("(chua co ky luc nao)", 290, 195, 16, GRAY);
    } else {
        int y = 195;
        for (size_t i = 0; i < entries.size(); i++) {
            Color rowColor = (i == 0) ? YELLOW : WHITE;
            DrawGameText(TextFormat("%2d.  %6d pts   wave %d", (int)i + 1, entries[i].score, entries[i].wave),
                         230, y, 16, rowColor);
            y += 20;
        }
    }

    DifficultyStats stats = GetDifficultyStats(difficulty);
    int bottomY = 195 + (int)entries.size() * 20 + 30;
    if (bottomY < 420) bottomY = 420; // Danh sach rong/ngan van giu bo cuc on dinh, khong bi troi len qua cao
    DrawGameText(TextFormat("< DIFFICULTY: %s >", stats.label), 260, bottomY, 20, WHITE);
    DrawGameText(TextFormat("VOLUME: %d%%  (UP/DOWN)", (int)(audio.GetVolume() * 100)), 260, bottomY + 35, 18, GRAY);
    DrawGameText("PRESS ENTER TO START", 240, bottomY + 80, 20, WHITE);
    DrawGameText("LEFT/RIGHT: CHANGE DIFFICULTY", 230, bottomY + 108, 16, GRAY);
    DrawGameText("F11: FULLSCREEN", 300, bottomY + 130, 16, GRAY);
}

void GameManager::DrawEndScreen() const {
    bool waveClear = (state == GameState::WAVE_CLEAR);
    if (waveClear) {
        DrawGameText(TextFormat("WAVE %d CLEARED!", wave - 1), 240, 180, 36, YELLOW);
        DrawGameText(TextFormat("SCORE: %d", player.GetScore()), 300, 240, 20, WHITE);
        DrawGameText("ENTER: NEXT WAVE   R: RESTART", 210, 300, 20, GRAY);
    } else {
        DrawGameText("GAME OVER", 300, 180, 40, RED);
        DrawGameText(TextFormat("FINAL SCORE: %d   WAVE REACHED: %d", player.GetScore(), wave), 190, 240, 20, WHITE);

        // 3 trang thai ro rang thay vi 1 bool "co pha ky luc hay khong": NewRecord (giờ
        // la #1), MadeTop10 (lot danh sach nhung khong phai #1), hoac khong lot top nao
        // ca (van hien diem cao nhat hien tai de nguoi choi biet minh con thieu bao nhieu).
        if (lastSubmitResult == SubmitResult::NewRecord) {
            DrawGameText("KY LUC MOI! (#1)", 300, 270, 20, YELLOW);
        } else if (lastSubmitResult == SubmitResult::MadeTop10) {
            DrawGameText("LOT TOP 10!", 300, 270, 20, LIME);
        } else {
            DrawGameText(TextFormat("TOP SCORE: %d", leaderboard.GetTopScore()), 300, 270, 18, GRAY);
        }
        DrawGameText("ENTER: MENU   R: RESTART", 220, 330, 20, GRAY);
    }
}

void GameManager::DrawPlaying() const {
    Camera2D cam{};
    cam.offset = screenShake.GetOffset();
    cam.target = { 0, 0 };
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;

    BeginMode2D(cam);
    for (size_t i = 0; i < basicEnemies.Size(); i++) {
        DrawSprite(sprites.basicAlien, basicEnemies[i].rect, basicEnemies[i].color);
    }
    for (size_t i = 0; i < tankyEnemies.Size(); i++) {
        const TankyEnemy& e = tankyEnemies[i];
        DrawSprite(sprites.tankyAlien, e.rect, e.color);
        if (e.hp < TankyEnemy::HP) {
            // Dich mau day bi thuong -> vien sang de nguoi choi thay ro da gay sat thuong
            DrawRectangleLinesEx(e.rect, 2.0f, WHITE);
        }
    }
    for (size_t i = 0; i < zigzagEnemies.Size(); i++) {
        DrawSprite(sprites.zigzagAlien, zigzagEnemies[i].rect, zigzagEnemies[i].color);
    }
    for (size_t i = 0; i < kamikazeEnemies.Size(); i++) {
        DrawSprite(sprites.kamikaze, kamikazeEnemies[i].rect, kamikazeEnemies[i].color);
    }
    if (ufoActive) {
        DrawSprite(sprites.ufo, ufoRect, RED);
    }
    if (bossActive) {
        int stage = boss.Stage();
        Color tint = (stage == 1) ? WHITE : (stage == 2) ? ORANGE : RED; // Cang yeu cang do, bao hieu "enrage"
        DrawSprite(sprites.boss, boss.rect, tint);
    }
    for (const auto& bunker : bunkers) bunker.Draw();
    playerBullets.Draw(YELLOW);
    enemyBullets.Draw(RED);
    particles.Draw();
    powerUps.Draw();
    player.Draw();
    EndMode2D();

    // HUD ve ngoai camera de khong bi rung theo
    DrawHUD();

    if (state == GameState::PAUSED) {
        DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(BLACK, 0.6f));
        DrawGameText("PAUSED", 330, 250, 40, WHITE);
        DrawGameText(TextFormat("VOLUME: %d%%  (UP/DOWN)", (int)(audio.GetVolume() * 100)), 280, 310, 18, GRAY);
        DrawGameText("P / ESC: RESUME   F11: FULLSCREEN", 250, 340, 18, GRAY);
    }
}

void GameManager::DrawHUD() const {
    DrawGameText(TextFormat("SCORE: %d", player.GetScore()), 10, 10, 20, WHITE);
    DrawGameText(TextFormat("WAVE: %d", wave), 10, 35, 18, SKYBLUE);
    DrawGameText(TextFormat("LIVES: %d", player.GetLives()), 700, 10, 20, WHITE);
    DrawGameText("P: PAUSE   R: RESTART", 290, 10, 16, GRAY);

    if (comboCount > 1) {
        DrawGameText(TextFormat("COMBO x%d", comboCount), 330, 35, 18, YELLOW);
    }
    if (player.HasShield()) DrawGameText("SHIELD", 690, 35, 16, SKYBLUE);
    if (player.HasRapidFire()) DrawGameText("RAPID FIRE", 660, 55, 16, ORANGE);
    if (player.HasPiercing()) DrawGameText("PIERCING", 670, 75, 16, MAGENTA);

    if (bossActive) {
        // Thanh mau boss o giua man hinh tren dinh
        float barW = 300.0f;
        float ratio = (boss.maxHp > 0) ? ((float)boss.hp / (float)boss.maxHp) : 0.0f;
        float barX = (Config::SCREEN_W - barW) / 2.0f;
        DrawRectangle((int)barX, 8, (int)barW, 14, DARKGRAY);
        DrawRectangle((int)barX, 8, (int)(barW * ratio), 14, RED);
        DrawRectangleLines((int)barX, 8, (int)barW, 14, WHITE);
        DrawGameText("BOSS", (int)barX, 24, 14, RED);
    }
}

void GameManager::DrawGameText(const char* text, int x, int y, int fontSize, Color color) const {
    DrawTextEx(gameFont, text, { (float)x, (float)y }, (float)fontSize, 1.0f, color);
}

// ==========================================
// MAIN LOOP
// ==========================================
void GameManager::Run() {
    InitWindow(Config::SCREEN_W, Config::SCREEN_H, "Hardcore Space Invaders");
    SetTargetFPS(60);
    audio.Init();
    sprites.Load();
    leaderboard.Load(Config::LeaderboardFilePath());
    levelGrid = LevelGridConfig::LoadFromFile(Config::LevelConfigFilePath());

    // FONT: LoadFontEx rasterize toan bo glyph thanh 1 texture atlas duy nhat NGAY LUC
    // NAY (o FONT_BASE_SIZE=48px - cao hon han moi kich thuoc chu se dung trong game,
    // du "du lieu nguon" de scale NHO lai ma khong ro net/mo). Neu thieu file (chua copy
    // thu muc assets/ theo cung executable) thi fallback ve font mac dinh cua raylib
    // thay vi crash - cung triet ly voi Settings/LevelGridConfig (khong bao gio crash vi
    // thieu 1 file khong bat buoc).
    gameFont = LoadFontEx(Config::FontFilePath(), Config::FONT_BASE_SIZE, nullptr, 0);
    if (!IsFontValid(gameFont)) {
        TraceLog(LOG_WARNING, "Khong tai duoc font '%s' - dung font mac dinh cua raylib", Config::FontFilePath());
        gameFont = GetFontDefault();
    } else {
        SetTextureFilter(gameFont.texture, TEXTURE_FILTER_BILINEAR); // Muot khi ve nho lai tu base size lon
    }

    // RENDER TARGET: toan bo gameplay ve vao canvas noi bo co dinh SCREEN_W x SCREEN_H,
    // sau do upscale nguyen khoi canvas nay len kich thuoc man hinh THAT (co the khac
    // hoan toan 800x600 luc Fullscreen) - giu dung ty le khung hinh bang thanh
    // letterbox/pillarbox 2 ben thay vi keo gian meo mo pixel art nhu truoc (ve thang
    // len window that voi toa do co dinh 800x600 thi Fullscreen tren monitor ty le khac
    // se bi stretch bien dang).
    renderTarget = LoadRenderTexture(Config::SCREEN_W, Config::SCREEN_H);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_BILINEAR);

    settings = Settings::LoadFromFile(Config::SettingsFilePath());
    difficulty = settings.difficulty;
    audio.SetVolume(settings.volume);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > Config::MAX_DT) dt = Config::MAX_DT;

        UpdateTransition(dt);
        // Khi dang fade, dong bang gameplay de khong update/collision trong luc man hinh dang mo dan
        bool frozen = (transitionPhase != TransitionPhase::NONE);

        if (!frozen) {
            switch (state) {
                case GameState::MENU:       UpdateMenu(); break;
                case GameState::GAME_OVER:
                case GameState::WAVE_CLEAR: UpdateEndScreen(); break;
                case GameState::PAUSED:     UpdatePaused(); break;
                case GameState::PLAYING:    UpdatePlaying(dt); break;
            }
        }

        // BUOC 1: ve toan bo gameplay vao canvas noi bo co dinh (khong lien quan gi
        // toi kich thuoc window/monitor that).
        BeginTextureMode(renderTarget);
        ClearBackground(BLACK);

        switch (state) {
            case GameState::MENU: DrawMenu(); break;
            case GameState::GAME_OVER:
            case GameState::WAVE_CLEAR: DrawEndScreen(); break;
            case GameState::PLAYING:
            case GameState::PAUSED: DrawPlaying(); break;
        }

        float alpha = GetTransitionAlpha();
        if (alpha > 0.0f) {
            DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(BLACK, alpha));
        }
        EndTextureMode();

        // BUOC 2: upscale canvas noi bo len window that, giu dung ty le khung hinh.
        // scale = min(theo truc nao gioi han truoc) -> luon vua khit ma khong bao gio
        // bi cat canh nao ca; phan con du duoc lap day boi vien den (letterbox/pillarbox)
        // thay vi keo gian lech ty le.
        BeginDrawing();
        ClearBackground(BLACK);

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        float scale = fminf((float)screenW / (float)Config::SCREEN_W, (float)screenH / (float)Config::SCREEN_H);
        float destW = Config::SCREEN_W * scale;
        float destH = Config::SCREEN_H * scale;
        float destX = ((float)screenW - destW) / 2.0f;
        float destY = ((float)screenH - destH) / 2.0f;

        // Chieu cao am: RenderTexture2D bi lat nguoc theo truc Y so voi man hinh thong
        // thuong (quy uoc OpenGL) - day la buoc lat lai chuan, khong phai 1 hack.
        Rectangle src{ 0.0f, 0.0f, (float)renderTarget.texture.width, -(float)renderTarget.texture.height };
        Rectangle dst{ destX, destY, destW, destH };
        DrawTexturePro(renderTarget.texture, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);

        EndDrawing();
    }

    UnloadRenderTexture(renderTarget);
    if (gameFont.texture.id != GetFontDefault().texture.id) UnloadFont(gameFont); // Chi unload neu KHONG phai font fallback mac dinh cua raylib
    sprites.Unload();
    audio.Shutdown();
    CloseWindow();
}
