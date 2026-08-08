#include "game_manager.h"
#include <cmath>
#include "input_system.h"
#include "physics_system.h"
#include "render_system.h"
#include "file_logger.h"
#include "wave_generator.h"

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
        // DDA (Track B2): dong bo lai theo mang THAT SU cua player vua Reset(), khong dung
        // Config::MAX_LIVES (do la TRAN treo, khong phai mang bat dau - xem Player::Player(),
        // mang bat dau la 1 hang so rieng = 3). Truoc fix nay, GameManager moi tao co
        // ddaLastKnownLives=5 nhung player.lives that su=3 -> frame dau tien cua UpdatePlaying()
        // tu ghi nham "+2 mang da mat" vao ddaLivesLostSinceCheck ngay ca khi nguoi choi
        // chua he bi trung don nao - lech nay khong tu sua truoc checkpoint DDA dau tien
        // (wave 5) vi ddaLivesLostSinceCheck chi reset O checkpoint, khong o dau khac. Cung
        // reset ddaSpeedMul/ddaLivesLostSinceCheck o day de 1 van MOI luon bat dau tu muc do
        // kho goc, khong mang theo dieu chinh cua lan choi truoc (neu GameManager duoc tai
        // su dung qua nhieu "New Game" thay vi luon khoi tao lai tu dau).
        ddaLastKnownLives = player.GetLives();
        ddaLivesLostSinceCheck = 0;
        ddaSpeedMul = 1.0f;
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
    floatingTexts.Reset();
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
        // WAVE/FORMATION PROCEDURAL (B3): WaveGenerator (wave_generator.h/.cpp) quyet
        // dinh "o nao co dich loai gi" - THUAN TUY, khong biet GameManager ton tai (xem
        // comment dau wave_generator.h, cung tinh than level_config.h). InitLevel() chi
        // con viec duyet ket qua va goi dung EnemyPool::Spawn() tuong ung - mau/kich
        // thuoc/hp mac dinh cua tung loai VAN o day, dung cho vi tri no da o san truoc
        // gio (khong thuoc ve "hinh dang doi hinh").
        for (const FormationSpawn& spawn : WaveGenerator::Generate(wave, levelGrid)) {
            switch (spawn.kind) {
                case FormationEnemyKind::Zigzag:
                    zigzagEnemies.Spawn(ZigzagEnemy{ {spawn.x, spawn.y, 36.0f, 22.0f}, SKYBLUE, spawn.column, 0.0f, 0.0f });
                    break;
                case FormationEnemyKind::Tanky:
                    tankyEnemies.Spawn(TankyEnemy{ {spawn.x, spawn.y, 44.0f, 30.0f}, MAROON, spawn.column, TankyEnemy::HP });
                    break;
                case FormationEnemyKind::Basic: {
                    Color col = (spawn.row % 2 == 0) ? PURPLE : VIOLET;
                    basicEnemies.Spawn(BasicEnemy{ {spawn.x, spawn.y, 40.0f, 25.0f}, col, spawn.column });
                    break;
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

    if (newGame) ApplyLoadoutBonus(); // Bonus loadout la "bat dau 1 VAN moi" - KHONG lap lai moi wave (newGame=false)
}

// ==========================================
// LOADOUT BONUS - doc loadout dang duoc chon trong Menu (selectedLoadout, xem UpdateMenu)
// va ap dung dung 1 lan luc bat dau 1 van MOI (xem lenh goi trong InitLevel() o tren).
// Kiem tra lai IsUnlocked() o day (khong chi tin selectedLoadout) de phong truong hop
// nguoi choi dang "xem thu" (cycle toi) 1 loadout chua du currency mo khoa that su - luc
// do van choi dung nhu Standard, khong "quyt" duoc bonus chua tra tien.
// ==========================================
void GameManager::ApplyLoadoutBonus() {
    LoadoutType chosen = (LoadoutType)selectedLoadout;
    if (chosen != LoadoutType::Standard && !metaProgress.IsUnlocked(chosen)) return;
    player.ApplyStartBonus(chosen);
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
    floatingTexts.Spawn(player.GetCenter(), finalScore, comboCount); // Chi hien thi - khong dung toi phep tinh diem o tren

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

    // XOAY VONG 3 LOAI BOSS (wave 5=Vanguard, wave 10=Sentinel, wave 15=Swarmer, wave
    // 20=Vanguard lai...) - moi loai 1 kieu di chuyen/tan cong rieng (xem enemy_types.h
    // va PhysicsSystem::UpdateBoss), tranh cam giac "chi la 1 con Boss ngay cang nhieu
    // mau" lap lai moi Config::BOSS_WAVE_INTERVAL wave.
    BossType type = (BossType)((bossIndex - 1) % 3);
    float startX = (Config::SCREEN_W - Config::BOSS_WIDTH) / 2.0f;

    Boss b;
    b.rect = { startX, Config::BOSS_Y, Config::BOSS_WIDTH, Config::BOSS_HEIGHT };
    b.hp = hp;
    b.maxHp = hp;
    b.direction = 1;
    b.fireTimer = 0.0f;
    b.type = type;
    b.baseX = startX;
    b.phaseAccum = 0.0f;
    b.phaseTimer = Config::BOSS_SENTINEL_SHIELD_INTERVAL;   // Sentinel: lan bat khien dau tien sau dung 1 chu ky
    b.shieldActive = false;
    b.summonTimer = Config::BOSS_SWARMER_SUMMON_INTERVAL;   // Swarmer: lan trieu hoi dau tien sau dung 1 chu ky

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

    // LOADOUT SELECT: cycle 3 lua chon (Standard/Vanguard/Overcharge) bang Q/E - phim
    // rieng, KHONG trung voi Trai/Phai doi do kho. Dung tren 1 loadout dang KHOA ma du
    // currency -> tu dong TryUnlock ngay (tru tien + luu file); chua du thi chi "xem thu"
    // tien do (hien qua DrawLoadoutSelect trong render_system.cpp), khong lam gi ca.
    if (input.CycleLoadoutLeft || input.CycleLoadoutRight) {
        int dir = input.CycleLoadoutRight ? 1 : -1;
        selectedLoadout = (selectedLoadout + dir + 3) % 3;
        LoadoutType chosen = (LoadoutType)selectedLoadout;
        if (chosen != LoadoutType::Standard && !metaProgress.IsUnlocked(chosen)) {
            metaProgress.TryUnlock(chosen, GetLoadoutUnlockCost(chosen));
        }
    }

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
    hitStop.Update(dt);
    if (hitStop.IsActive()) return; // Dong bang toan bo logic ben duoi - Run() ngoai vong lap van goi Draw() binh thuong nen hinh khong dung, chi gameplay dung khung trong choc lat

    MenuInput menuInput = InputSystem::PollMenu(settings);
    if (menuInput.PauseToggle) { state = GameState::PAUSED; return; }
    if (menuInput.Restart) { InitLevel(true); return; }
    if (menuInput.ToggleFullscreen) ToggleFullscreen();

    screenShake.Update(dt);
    particles.Update(dt);
    floatingTexts.Update(dt);
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
        // MUZZLE FLASH (Nguoi 3 - Audio & UI): dau nong = giua-tren rect player, dung
        // CHINH vi tri playerBullets.Fire() dat vien dan dau tien (xem player.cpp).
        Rectangle pr = player.GetRect();
        particles.Burst({ pr.x + pr.width / 2.0f, pr.y }, Config::MUZZLE_FLASH_PARTICLE_COUNT, YELLOW);
    }

    if (isBossWave) PhysicsSystem::UpdateBoss(*this, dt);
    else PhysicsSystem::UpdateEnemies(*this, dt);
    if (state != GameState::PLAYING) return; // UpdateEnemies/danh boss co the trigger WAVE_CLEAR/GAME_OVER

    playerBullets.Update(dt);
    enemyBullets.Update(dt);
    PhysicsSystem::CheckCollisions(*this);
    ProcessEvents(); // Xu ly tach biet moi hieu ung/he qua ma CheckCollisions() vua ghi nhan

    // DYNAMIC DIFFICULTY ADJUSTMENT: cong don so mang mat trong frame nay (neu co) vao
    // chu ky Boss hien tai. So sanh CHENH LECH voi ddaLastKnownLives thay vi moc vao tung
    // diem va cham rieng le trong PhysicsSystem (TakeDamage() duoc goi tu ca
    // CheckCollisions lan UpdateKamikaze) - 1 diem ghi nhan DUY NHAT o day gon hon nhieu.
    // Chi cong don khi GIAM (mat mang) - tang (vd +1 mang tu moc diem, xem Player::AddScore)
    // khong bi tinh nham la "mat mang".
    int currentLives = player.GetLives();
    if (currentLives < ddaLastKnownLives) ddaLivesLostSinceCheck += (ddaLastKnownLives - currentLives);
    ddaLastKnownLives = currentLives;

    // BOSS DEFEAT: dung CHUNG dinh nghia "con song" voi moi pool khac (Size()>0) - khong
    // con bool `bossActive` rieng phai kiem tra dong bo voi hp.
    if (isBossWave && bossPool.Size() > 0 && bossPool[0].hp <= 0) {
        Vector2 bossCenter = EnemyCenter(bossPool[0].rect);
        bossPool.Destroy(0);
        audio.PlayBossDefeat();
        particles.Burst(bossCenter, 40, RED);
        screenShake.Trigger(0.4f, 12.0f);
        hitStop.Trigger(0.1f); // Nang do hon dong bang thuong (0.04f) - xem physics_system.cpp
        ApplyComboAndScore(Config::BOSS_SCORE_VALUE);
        wave++;
        lastSubmitResult = leaderboard.TrySubmit(player.GetScore(), wave);

        // DYNAMIC DIFFICULTY ADJUSTMENT: CHECKPOINT moi chu ky Boss (khong phai moi wave
        // thuong) - doc lai ddaLivesLostSinceCheck vua cong don o tren de dieu chinh
        // ddaSpeedMul cho chu ky ke tiep (PhysicsSystem::UpdateEnemies() ap dung thuc su,
        // xem physics_system.cpp), roi RESET ve 0 cho chu ky moi. Khong mat mang nao ca
        // chu ky -> nguoi choi lam chu qua tot -> tang nhe; mat tu Config::
        // DDA_STRUGGLE_THRESHOLD tro len -> dang vat lon -> giam NHIEU HON (uu tien "cuu"
        // nguoi choi kip thoi hon la thu thach them nguoi choi da gioi). O giua 2 nguong
        // nay (vd mat dung 1 mang khi threshold=2) -> giu nguyen, tranh rung lac do kho
        // vi 1 lan trung don ngau nhien.
        if (ddaLivesLostSinceCheck == 0) {
            ddaSpeedMul = fminf(ddaSpeedMul + Config::DDA_STEP_UP, Config::DDA_MAX_MUL);
        } else if (ddaLivesLostSinceCheck >= Config::DDA_STRUGGLE_THRESHOLD) {
            ddaSpeedMul = fmaxf(ddaSpeedMul - Config::DDA_STEP_DOWN, Config::DDA_MIN_MUL);
        }
        ddaLivesLostSinceCheck = 0;

        RequestTransition(GameState::WAVE_CLEAR);
        return;
    }

    DifficultyStats stats = GetDifficultyStats(difficulty);
    if (!isBossWave) {
        audio.UpdateBassline(dt, enemySpeed, stats.enemySpeedMax);
    }

    // NHAC NEN PROCEDURAL (Nguoi 3 - Audio & UI): goi MOI frame, KHONG boc trong
    // if(!isBossWave) nhu UpdateBassline o tren - chinh vi bassline IM LANG luc danh Boss
    // (nhanh if ngay tren) nen nhac nen tro thanh lop am nen DUY NHAT con lai luc do, can
    // tiep tuc chay (va tang cuong do qua MusicContext::bossActive, xem FillMusicBuffer()
    // trong audio_system.cpp) thay vi cung tat theo.
    MusicContext musicCtx;
    musicCtx.enemySpeedRatio = (stats.enemySpeedMax > 0.0f) ? fminf(enemySpeed / stats.enemySpeedMax, 1.0f) : 0.0f;
    musicCtx.wave = wave;
    musicCtx.bossActive = isBossWave;
    musicCtx.livesRemaining = player.GetLives();
    musicCtx.comboCount = comboCount;
    audio.UpdateMusic(dt, musicCtx);

    if (player.GetLives() <= 0) {
        audio.PlayGameOver();
        lastSubmitResult = leaderboard.TrySubmit(player.GetScore(), wave);
        metaProgress.AwardCurrency(player.GetScore());
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
        // HIT-FLASH (Nguoi 3 - Audio & UI): cum particle TRANG rieng, CONG DON voi burst
        // mau thuong o tren neu co (khong thay the) - bao "chi trung", tach voi burst mau
        // dang bao "loai gi/khien hay khong" (xem events.h + physics_system.cpp).
        if (ev.flashOnHit) particles.Burst(ev.position, Config::HITFLASH_PARTICLE_COUNT, WHITE);

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

    // Khong gia dinh InitWindow() luon thanh cong - neu khong co man hinh/driver do hoa hop
    // le (chay qua SSH khong forward X11, may headless khong co Xvfb, driver GPU loi...)
    // GLFW se khong tao duoc context, va MOI loi goi do hoa ngay sau day (SetExitKey,
    // LoadFontEx, sprites.Load(), LoadRenderTexture...) se deref tren context khong ton tai
    // -> segfault kho hieu thay vi 1 thong bao loi ro rang. Cung triet ly voi
    // IsSoundValid()/IsFontValid() da dung o noi khac trong file nay - kiem tra ket qua
    // truoc khi dung tiep, khong gia dinh thanh cong. Chua co tai nguyen nao (font/sprite/
    // audio/renderTarget) duoc cap phat tinh den day nen chi can dong FileLogger roi thoat,
    // khong goi CloseWindow()/Unload* (chua co gi de giai phong).
    if (!IsWindowReady()) {
        TraceLog(LOG_FATAL, "Khong the khoi tao cua so do hoa (InitWindow that bai) - kiem tra driver GPU/DISPLAY, thoat.");
        FileLogger::Shutdown();
        return;
    }

    // BUG FIX: raylib mac dinh gan KEY_ESCAPE lam "exit key" - tu dong goi
    // glfwSetWindowShouldClose() ngay o tang GLFW callback, HOAN TOAN doc lap voi
    // InputSystem::PollMenu(). Vi game nay dung chinh ESC lam phim Pause (xem
    // InputSystem::PollMenu, man Pause ghi "P/ESC: TIEP TUC"), neu khong tat hanh vi
    // mac dinh nay thi nhan ESC de Pause se VO TINH dong luon ca cua so game (thoat
    // hoan toan) thay vi chi tam dung. PHAI goi truoc khi vong lap While(!WindowShouldClose())
    // o duoi bat dau chay.
    SetExitKey(KEY_NULL);

    SetTargetFPS(60);
    audio.Init();
    sprites.Load();
    leaderboard.Load(Config::LeaderboardFilePath());
    metaProgress.Load("meta_progress.dat");
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
