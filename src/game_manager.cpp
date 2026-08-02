#include "game_manager.h"
#include <cmath>
#include "input_system.h"
#include "physics_system.h"
#include "render_system.h"
#include "file_logger.h"

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
    bossPool.Clear(); // Thay the `bossActive = false` cu - Size()==0 la dinh nghia DUY NHAT cua "boss chua/khong con song"

    // BOSS WAVE: cu moi Config::BOSS_WAVE_INTERVAL wave, thay THE HOAN TOAN luoi doi
    // hinh thuong bang 1 Boss duy nhat - wave chi duoc coi la "don sach" khi boss bi ha
    // (xem PhysicsSystem::UpdateEnemies/UpdateBoss + UpdatePlaying), khong phai khi
    // activeCount==0.
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

    if (player.AddScore(finalScore)) {
        // +1 MANG tu moc diem (xem Config::EXTRA_LIFE_SCORE_THRESHOLD) - dung mau GOLD
        // + particle burst rieng biet voi pickup power-up thuong (von khong co particle)
        // de nguoi choi nhan ra ngay day la 1 cot moc dang chu y, khong phai nhat power-up.
        GameEvent ev;
        ev.position = player.GetCenter();
        ev.color = GOLD;
        ev.particleCount = 20;
        ev.sfx = SfxType::Pickup;
        pendingEvents.push_back(ev);
    }

    return finalScore;
}

// ==========================================
// MYSTERY SHIP (UFO) - Spawn/lich hen la "luat choi" (khi nao xuat hien), con di chuyen
// va va cham thuc su do PhysicsSystem::UpdateUfo()/CheckCollisions() xu ly.
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

// ==========================================
// KAMIKAZE - cung nguyen tac: Spawn/lich hen o day, di chuyen+va cham o PhysicsSystem.
// ==========================================
void GameManager::RollNextKamikazeTimer() {
    int minMs = (int)(Config::KAMIKAZE_SPAWN_MIN_INTERVAL * 1000.0f);
    int maxMs = (int)(Config::KAMIKAZE_SPAWN_MAX_INTERVAL * 1000.0f);
    kamikazeSpawnTimer = (float)GetRandomValue(minMs, maxMs) / 1000.0f;
}

void GameManager::SpawnKamikaze() {
    size_t totalFormation = basicEnemies.Size() + tankyEnemies.Size() + zigzagEnemies.Size();
    Rectangle rect;

    if (totalFormation > 0) {
        // Chon DONG DEU tren TOAN BO doi hinh (khong phai chon pool truoc roi chon
        // trong pool - se lam sai lech ti le neu 3 pool co so luong khac nhau), roi
        // "boc" chinh phan tu do ra khoi pool goc (Destroy() - xem giai thich an toan
        // o comment tren struct KamikazeEnemy trong enemy_types.h).
        size_t pick = (size_t)GetRandomValue(0, (int)totalFormation - 1);
        if (pick < basicEnemies.Size()) {
            rect = basicEnemies[pick].rect;
            basicEnemies.Destroy(pick);
        } else if (pick < basicEnemies.Size() + tankyEnemies.Size()) {
            size_t i = pick - basicEnemies.Size();
            rect = tankyEnemies[i].rect;
            tankyEnemies.Destroy(i);
        } else {
            size_t i = pick - basicEnemies.Size() - tankyEnemies.Size();
            rect = zigzagEnemies[i].rect;
            zigzagEnemies.Destroy(i);
        }
    } else {
        // Doi hinh dang trong (boss wave, hoac vua don sach) - khong con gi de "boc",
        // quay lai spawn doc lap tu ngoai man hinh nhu ban goc.
        float startX = (float)GetRandomValue(0, (int)Config::SCREEN_W);
        rect = { startX - Config::KAMIKAZE_WIDTH / 2.0f, -Config::KAMIKAZE_HEIGHT,
                 Config::KAMIKAZE_WIDTH, Config::KAMIKAZE_HEIGHT };
    }

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

// ==========================================
// BOSS - Spawn (thiet lap hp/vi tri ban dau) o day; di chuyen+ban+va cham do
// PhysicsSystem::UpdateBoss()/CheckCollisions() xu ly qua chung 1 EnemyPool<Boss,1>.
// ==========================================
void GameManager::SpawnBoss() {
    int bossIndex = wave / Config::BOSS_WAVE_INTERVAL; // 1, 2, 3... (wave 5 -> 1, wave 10 -> 2...)
    int hp = Config::BOSS_MAX_HP + (int)(Config::BOSS_HP_PER_WAVE_BONUS * (float)(bossIndex - 1));

    Boss b;
    b.rect = { (Config::SCREEN_W - Config::BOSS_WIDTH) / 2.0f, Config::BOSS_Y,
               Config::BOSS_WIDTH, Config::BOSS_HEIGHT };
    b.hp = hp;
    b.maxHp = hp;
    b.direction = 1;
    b.fireTimer = 0.0f;

    bossPool.Clear();
    bossPool.Spawn(b);
}

// ==========================================
// MENU
// ==========================================
void GameManager::UpdateMenu() {
    MenuInput input = InputSystem::PollMenu(settings);
    bool changed = false;
    if (input.CycleDifficultyLeft)  { difficulty = CycleDifficulty(difficulty, -1); changed = true; }
    if (input.CycleDifficultyRight) { difficulty = CycleDifficulty(difficulty, 1); changed = true; }
    if (input.VolumeUp)   { audio.SetVolume(audio.GetVolume() + 0.1f); changed = true; }
    if (input.VolumeDown) { audio.SetVolume(audio.GetVolume() - 0.1f); changed = true; }
    if (changed) SaveSettings();

    if (input.ToggleFullscreen) ToggleFullscreen();

    if (input.Confirm) {
        InitLevel(true);
        RequestTransition(GameState::PLAYING);
    }
}

// ==========================================
// GAME_OVER / WAVE_CLEAR
// ==========================================
void GameManager::UpdateEndScreen() {
    MenuInput input = InputSystem::PollMenu(settings);

    if (state == GameState::WAVE_CLEAR) {
        if (input.Confirm) {
            InitLevel(false); // Giu diem/mang, sang wave ke tiep voi do kho cao hon
            RequestTransition(GameState::PLAYING);
        }
        if (input.Restart) {
            InitLevel(true); // Choi lai tu dau (wave 1, reset diem/mang)
            RequestTransition(GameState::PLAYING);
        }
        return;
    }

    // GAME_OVER
    if (input.Confirm) RequestTransition(GameState::MENU);
    if (input.Restart) {
        InitLevel(true);
        RequestTransition(GameState::PLAYING);
    }
}

void GameManager::UpdatePaused() {
    MenuInput input = InputSystem::PollMenu(settings);
    bool changed = false;
    if (input.VolumeUp)   { audio.SetVolume(audio.GetVolume() + 0.1f); changed = true; }
    if (input.VolumeDown) { audio.SetVolume(audio.GetVolume() - 0.1f); changed = true; }
    if (changed) SaveSettings();
    if (input.ToggleFullscreen) ToggleFullscreen();
    if (input.OpenKeybinds) { state = GameState::KEYBIND; return; }
    if (input.PauseToggle) state = GameState::PLAYING;
}

// ==========================================
// KEYBIND - man hinh doi phim dieu khien, vao tu Paused (phim K). Khong dung
// MenuInput/PollMenu() o day: ban chat man hinh nay la "bat ky phim nao cung co the la
// gia tri hop le can GHI NHAN" (dang cho 1 phim MOI), khac han cac man hinh khac chi
// quan tam vai phim CO Y NGHIA CO DINH - nen doc truc tiep IsKeyPressed(KEY_ESCAPE)/
// so + InputSystem::PollAnyKeyPressed() thay vi ep vao khuon MenuInput.
// ==========================================
void GameManager::UpdateKeybindScreen() {
    if (rebindingActionIndex == -1) {
        // Dang hien danh sach 4 hanh dong - cho bam so 1-4 de chon 1 cai de doi.
        if (IsKeyPressed(KEY_ESCAPE)) { state = GameState::PAUSED; return; }
        if (IsKeyPressed(KEY_ONE))   { rebindingActionIndex = 0; return; }
        if (IsKeyPressed(KEY_TWO))   { rebindingActionIndex = 1; return; }
        if (IsKeyPressed(KEY_THREE)) { rebindingActionIndex = 2; return; }
        if (IsKeyPressed(KEY_FOUR))  { rebindingActionIndex = 3; return; }
        if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_R)) {
            settings.ResetKeyBindingsToDefault();
            SaveSettings();
        }
        return;
    }

    // Da chon 1 hanh dong (rebindingActionIndex >= 0) - dang cho phim MOI cho no.
    if (IsKeyPressed(KEY_ESCAPE)) { rebindingActionIndex = -1; return; } // Huy, giu nguyen phim cu

    int newKey = InputSystem::PollAnyKeyPressed();
    if (newKey == 0) return; // Chua co phim nao duoc bam frame nay - tiep tuc cho

    // Tu choi cac phim "he thong" co dinh (Enter/R/F3/F11/mui ten/K/Esc) - day la
    // NHUNG PHIM DUY NHAT khong the rebind vao duoc (xem chu thich Settings/MenuInput),
    // dam bao nguoi choi khong bao gio tu khoa minh khoi menu du rebind be nao.
    static const int reserved[] = { KEY_ESCAPE, KEY_ENTER, KEY_F3, KEY_F11, KEY_R,
                                     KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_K };
    for (int r : reserved) {
        if (newKey == r) return; // Phim he thong - bo qua yeu cau, tiep tuc cho phim khac
    }

    // Tu choi neu phim nay dang duoc 1 TRONG 3 hanh dong CON LAI su dung - tranh 2 hanh
    // dong doi len cung 1 phim (se khong biet dang lam gi khi bam phim do).
    const RebindableAction* actions = GetRebindableActions();
    for (int i = 0; i < REBINDABLE_ACTION_COUNT; i++) {
        if (i == rebindingActionIndex) continue;
        if (settings.*(actions[i].keyField) == newKey) return;
    }

    settings.*(actions[rebindingActionIndex].keyField) = newKey;
    SaveSettings();
    rebindingActionIndex = -1;
}

// ==========================================
// PLAYING - GameManager chi con DIEU PHOI o day: doc input -> goi PhysicsSystem cho
// di chuyen/va cham -> xu ly hang doi hieu ung -> quyet dinh "luat choi" cap cao (het
// mang thi Game Over, boss chet thi Wave Clear...). Khong con phep tinh hinh hoc/va
// cham nao nam truc tiep trong ham nay nua.
// ==========================================
void GameManager::UpdatePlaying(float dt) {
    MenuInput menuInput = InputSystem::PollMenu(settings);
    if (menuInput.PauseToggle) { state = GameState::PAUSED; return; }
    if (menuInput.Restart) { InitLevel(true); return; }
    if (menuInput.ToggleFullscreen) ToggleFullscreen();

    screenShake.Update(dt);
    particles.Update(dt);
    powerUps.Update(dt, Config::POWERUP_FALL_SPEED, (float)Config::SCREEN_H);
    PhysicsSystem::UpdateUfo(*this, dt);
    PhysicsSystem::UpdateKamikaze(*this, dt);
    for (auto& bunker : bunkers) bunker.Update(dt); // Regen voxel + dao dong ngang

    if (comboTimer > 0.0f) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) comboCount = 0; // Het cua so combo ma khong ha them dich -> reset
    }

    // Doc phan cung DUY NHAT o day (InputSystem) roi dua tin hieu da quy doi vao
    // Player - Player khong con goi IsKeyDown/gamepad nao nua (xem player.cpp).
    InputState input = InputSystem::Poll(settings);
    if (player.Update(dt, input, playerBullets)) {
        audio.PlayShoot();
    }

    if (isBossWave) PhysicsSystem::UpdateBoss(*this, dt);
    else PhysicsSystem::UpdateEnemies(*this, dt);
    if (state != GameState::PLAYING) return; // UpdateEnemies/danh boss co the trigger WAVE_CLEAR/GAME_OVER

    playerBullets.Update(dt);
    enemyBullets.Update(dt);
    PhysicsSystem::CheckCollisions(*this);
    ProcessEvents(); // Xu ly tach biet moi hieu ung/he qua ma CheckCollisions() vua ghi nhan

    // BOSS DEFEAT: dung CHUNG dinh nghia "con song" voi moi pool khac (Size()>0) - khong
    // con bool `bossActive` rieng phai kiem tra dong bo voi hp.
    if (isBossWave && bossPool.Size() > 0 && bossPool[0].hp <= 0) {
        Vector2 bossCenter = EnemyCenter(bossPool[0].rect);
        bossPool.Destroy(0);
        audio.PlayBossDefeat();
        particles.Burst(bossCenter, 40, RED);
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

// ==========================================
// EVENT PROCESSING
// Duyet hang doi ma PhysicsSystem::CheckCollisions() vua ghi nhan va thuc thi TAT CA
// hieu ung (particle/audio/screenshake/diem/power-up) mot cach dong nhat, tach han khoi
// logic phat hien va cham. Xem events.h de biet ly do tach.
// ==========================================
void GameManager::ProcessEvents() {
    for (const GameEvent& ev : pendingEvents) {
        if (ev.particleCount > 0) particles.Burst(ev.position, ev.particleCount, ev.color);

        switch (ev.sfx) {
            case SfxType::Explosion: audio.PlayExplosion(); break;
            case SfxType::Hit:       audio.PlayHit();       break;
            case SfxType::UfoHit:    audio.PlayUfoHit();    break;
            case SfxType::Pickup:    audio.PlayPickup();    break;
            case SfxType::Cleanser:  audio.PlayCleanser();  break;
            case SfxType::None:      break;
        }

        if (ev.shakeDuration > 0.0f) screenShake.Trigger(ev.shakeDuration, ev.shakeIntensity);
        if (ev.scoreValue > 0) ApplyComboAndScore(ev.scoreValue);
        if (ev.dropPowerUp) MaybeDropPowerUp(ev.position);
    }
    pendingEvents.clear();
}

// ==========================================
// MAIN LOOP
// ==========================================
void GameManager::Run() {
    // FILE LOGGER: dang ky NGAY DAU TIEN, truoc ca InitWindow() - de bat luon cac dong
    // TraceLog chinh raylib tu phat ra luc khoi tao (vd loi mo man hinh/driver do hoa),
    // dieu se bi bo lot neu chi Init() sau khi InitWindow() da chay xong.
    FileLogger::Init();

    // DATA-DRIVEN BALANCE: nap assets/balance.json de GHI DE moi hang so can bang mac
    // dinh (HP, toc do, wave pattern, hanh vi Boss...) khai bao trong config.h/
    // enemy_types.h - PHAI goi TRUOC bat ky InitLevel()/Spawn*() nao (nhung ham nay doc
    // truc tiep Config::XXX/EnemyType::XXX) de dam bao gia tri tu file duoc ap dung
    // ngay tu wave dau tien, khong phai doi 1 vong lap moi co hieu luc.
    Config::LoadBalance();

    InitWindow(Config::SCREEN_W, Config::SCREEN_H, "Hardcore Space Invaders");

    // BUG FIX: raylib mac dinh gan KEY_ESCAPE lam "exit key" - tu dong goi
    // glfwSetWindowShouldClose() ngay o tang GLFW callback, HOAN TOAN doc lap voi
    // InputSystem::PollMenu(). Vi game nay dung chinh ESC lam phim Pause (xem
    // InputSystem::PollMenu, man Pause ghi "P/ESC: RESUME"), neu khong tat hanh vi
    // mac dinh nay thi nhan ESC de Pause se VO TINH dong luon ca cua so game (thoat
    // hoan toan) thay vi chi tam dung. PHAI goi truoc khi vong lap While(!WindowShouldClose())
    // o duoi bat dau chay.
    SetExitKey(KEY_NULL);

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

        if (InputSystem::PollDebugOverlayToggle()) showDebugOverlay = !showDebugOverlay;

        UpdateTransition(dt);
        // Khi dang fade, dong bang gameplay de khong update/collision trong luc man hinh dang mo dan
        bool frozen = (transitionPhase != TransitionPhase::NONE);

        if (!frozen) {
            switch (state) {
                case GameState::MENU:       UpdateMenu(); break;
                case GameState::GAME_OVER:
                case GameState::WAVE_CLEAR: UpdateEndScreen(); break;
                case GameState::PAUSED:     UpdatePaused(); break;
                case GameState::KEYBIND:    UpdateKeybindScreen(); break;
                case GameState::PLAYING:    UpdatePlaying(dt); break;
            }
        }

        // BUOC 1: ve toan bo gameplay vao canvas noi bo co dinh (khong lien quan gi
        // toi kich thuoc window/monitor that).
        BeginTextureMode(renderTarget);
        ClearBackground(BLACK);

        switch (state) {
            case GameState::MENU: RenderSystem::DrawMenu(*this); break;
            case GameState::GAME_OVER:
            case GameState::WAVE_CLEAR: RenderSystem::DrawEndScreen(*this); break;
            case GameState::PLAYING:
            case GameState::PAUSED:
            case GameState::KEYBIND: RenderSystem::DrawPlaying(*this); break;
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

        // OVERLAY DO LUONG: ve o TOA DO MAN HINH THAT (ngoai canh render texture noi bo
        // 800x600 vua upscale o tren) - luon sac net va o dung goc man hinh du dang
        // Fullscreen ty le nao, khong bi anh huong boi buoc letterbox/pillarbox.
        if (showDebugOverlay) RenderSystem::DrawDebugOverlay(*this);

        EndDrawing();
    }

    UnloadRenderTexture(renderTarget);
    if (gameFont.texture.id != GetFontDefault().texture.id) UnloadFont(gameFont); // Chi unload neu KHONG phai font fallback mac dinh cua raylib
    sprites.Unload();
    audio.Shutdown();
    CloseWindow();
    FileLogger::Shutdown();
}
