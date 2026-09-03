#include "game_manager.h"
#include <cmath>
#include "input_system.h"
#include "physics_system.h"
#include "render_system.h"
#include "file_logger.h"
#include "wave_generator.h"
#include "upgrade_types.h"
#include "palette.h"

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
        hintTimer = Config::HUD_HINT_DURATION; // Goi y phim chi hien o dau van MOI, khong lap lai moi wave
    } else {
        player.ResetForNewWave();
    }

    basicEnemies.Clear();
    tankyEnemies.Clear();
    zigzagEnemies.Clear();
    kamikazeEnemies.Clear();
    wardenEnemies.Clear();
    medicEnemies.Clear();
    weaverEnemies.Clear(); // Phase 2, Nguoi 1
    bomberEnemies.Clear(); // Phase 2, Nguoi 1
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
    RollNextWeaverTimer(); // Phase 2, Nguoi 1
    RollNextBomberTimer(); // Phase 2, Nguoi 1
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
                    zigzagEnemies.Spawn(ZigzagEnemy{ {spawn.x, spawn.y, 36.0f, 22.0f}, Palette::Zigzag, spawn.column, 0.0f, 0.0f });
                    break;
                case FormationEnemyKind::Tanky:
                    tankyEnemies.Spawn(TankyEnemy{ {spawn.x, spawn.y, 44.0f, 30.0f}, Palette::Tanky, spawn.column, TankyEnemy::HP });
                    break;
                case FormationEnemyKind::Basic: {
                    Color col = (spawn.row % 2 == 0) ? Palette::BasicA : Palette::BasicB;
                    basicEnemies.Spawn(BasicEnemy{ {spawn.x, spawn.y, 40.0f, 25.0f}, col, spawn.column });
                    break;
                }
                case FormationEnemyKind::Warden:
                    wardenEnemies.Spawn(WardenEnemy{ {spawn.x, spawn.y, 42.0f, 30.0f}, Palette::Warden, spawn.column, WardenEnemy::HP });
                    break;
                case FormationEnemyKind::Medic:
                    medicEnemies.Spawn(MedicEnemy{ {spawn.x, spawn.y, 34.0f, 24.0f}, Palette::Medic, spawn.column, 0.0f });
                    break;
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
        bunkers.emplace_back(slotCenterX - halfWidth, Config::BUNKER_Y, Palette::BunkerIntact);
    }
}

// Phase 1b (Enemy & Item Revolution, Nguoi 1): chon PowerUpType theo trong so doc tu
// Config::POWERUP_WEIGHT_* (KHONG con GetRandomValue(0,3) deu tuyet doi nhu 4 loai goc)
// - "quay so" 1 gia tri trong [0, tongTrongSo), di qua tung loai cong don trong so cho
// den khi vuot gia tri do. Weight <= 0 cho 1 loai nao do se loai han loai đo khoi vong
// quay (khong bao gio roi) ma khong can sua code, chi can sua balance.json.
static PowerUpType RollWeightedPowerUpType() {
    struct Entry { PowerUpType type; float weight; };
    const Entry table[] = {
        { PowerUpType::RapidFire,  Config::POWERUP_WEIGHT_RAPIDFIRE },
        { PowerUpType::Shield,     Config::POWERUP_WEIGHT_SHIELD },
        { PowerUpType::Piercing,   Config::POWERUP_WEIGHT_PIERCING },
        { PowerUpType::Cleanser,   Config::POWERUP_WEIGHT_CLEANSER },
        { PowerUpType::SpreadShot, Config::POWERUP_WEIGHT_SPREADSHOT },
        { PowerUpType::Overdrive,  Config::POWERUP_WEIGHT_OVERDRIVE },
    };
    float total = 0.0f;
    for (const auto& e : table) total += e.weight;
    if (total <= 0.0f) return PowerUpType::RapidFire; // Toan bo weight <=0 (cau hinh loi) - fallback an toan, khong chia cho 0

    float roll = (float)GetRandomValue(0, 999) / 1000.0f * total;
    float cumulative = 0.0f;
    for (const auto& e : table) {
        cumulative += e.weight;
        if (roll < cumulative) return e.type;
    }
    return table[(sizeof(table) / sizeof(table[0])) - 1].type; // Bo cho sai so lam tron float hiem gap - roi dung ve loai cuoi thay vi UB
}

void GameManager::MaybeDropPowerUp(Vector2 at) {
    if ((float)GetRandomValue(0, 999) / 1000.0f >= Config::POWERUP_DROP_CHANCE) return;
    PowerUpType type = RollWeightedPowerUpType();
    Rectangle rect{ at.x - Config::POWERUP_SIZE / 2.0f, at.y, Config::POWERUP_SIZE, Config::POWERUP_SIZE };
    powerUps.Spawn(PowerUp{ rect, type });
}

int GameManager::ApplyComboAndScore(int baseScore, Vector2 at) {
    // Ha guc them 1 dich trong luc combo timer con hieu luc -> tang bac combo; het
    // thoi gian (khong ha them dich nao) -> combo tu dong reset ve 0 (xem UpdatePlaying).
    if (comboTimer > 0.0f) comboCount++;
    else comboCount = 1;
    comboTimer = Config::COMBO_WINDOW;

    int steps = comboCount - 1;
    if (steps > Config::COMBO_MAX_STEPS) steps = Config::COMBO_MAX_STEPS;
    int finalScore = (int)((float)baseScore * (1.0f + Config::COMBO_BONUS_PER_STEP * (float)steps));
    floatingTexts.Spawn(at, finalScore, comboCount); // Chi hien thi - khong dung toi phep tinh diem o tren

    if (player.AddScore(finalScore)) {
        // +1 MANG tu moc diem (xem Config::EXTRA_LIFE_SCORE_THRESHOLD) - dung mau GOLD
        // + particle burst rieng biet voi pickup power-up thuong (von khong co particle)
        // de nguoi choi nhan ra ngay day la 1 cot moc dang chu y, khong phai nhat power-up.
        GameEvent ev;
        ev.position = player.GetCenter();
        ev.color = Palette::PowerUp;
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

    kamikazeEnemies.Spawn(KamikazeEnemy{ rect, Palette::Kamikaze, vel });
    RollNextKamikazeTimer();
}

// ==========================================
// WEAVER & BOMBER (Phase 2 - Enemy & Item Revolution, Nguoi 1) - dung khuon UFO o tren
// (Spawn*/RollNext*Timer() o day, di chuyen+va cham o PhysicsSystem), KHONG dung khuon
// SpawnKamikaze() ("boc" tu doi hinh) - xem giai thich tren struct WeaverEnemy trong
// enemy_types.h.
// ==========================================
void GameManager::RollNextWeaverTimer() {
    int minMs = (int)(Config::WEAVER_SPAWN_MIN_INTERVAL * 1000.0f);
    int maxMs = (int)(Config::WEAVER_SPAWN_MAX_INTERVAL * 1000.0f);
    weaverSpawnTimer = (float)GetRandomValue(minMs, maxMs) / 1000.0f;
}

void GameManager::SpawnWeaver() {
    int direction = (GetRandomValue(0, 1) == 0) ? 1 : -1;
    float startX = (direction > 0) ? -Config::WEAVER_WIDTH : (float)Config::SCREEN_W;
    // baseY quay ngau nhien moi lan spawn (khac UFO co UFO_Y co dinh) - nhieu Weaver
    // cung luc se bay o nhieu do cao khac nhau, dung tinh than "kho doan duong bay".
    float baseY = (float)GetRandomValue((int)Config::WEAVER_BASE_Y_MIN, (int)Config::WEAVER_BASE_Y_MAX);
    Rectangle rect{ startX, baseY, Config::WEAVER_WIDTH, Config::WEAVER_HEIGHT };
    weaverEnemies.Spawn(WeaverEnemy{ rect, Palette::Weaver, direction, baseY, 0.0f });
    RollNextWeaverTimer();
}

void GameManager::RollNextBomberTimer() {
    int minMs = (int)(Config::BOMBER_SPAWN_MIN_INTERVAL * 1000.0f);
    int maxMs = (int)(Config::BOMBER_SPAWN_MAX_INTERVAL * 1000.0f);
    bomberSpawnTimer = (float)GetRandomValue(minMs, maxMs) / 1000.0f;
}

void GameManager::SpawnBomber() {
    int direction = (GetRandomValue(0, 1) == 0) ? 1 : -1;
    float startX = (direction > 0) ? -Config::BOMBER_WIDTH : (float)Config::SCREEN_W;
    Rectangle rect{ startX, Config::BOMBER_Y, Config::BOMBER_WIDTH, Config::BOMBER_HEIGHT };
    // bombTimer bat dau tu 1 gia tri random trong [0, BOMBER_BOMB_INTERVAL] (khong phai
    // luon = BOMBER_BOMB_INTERVAL) de nhieu Bomber tren man hinh khong tha bom DONG BO
    // cung 1 nhip - cam giac hon loan/tu nhien hon.
    float initialBombTimer = (float)GetRandomValue(0, (int)(Config::BOMBER_BOMB_INTERVAL * 1000.0f)) / 1000.0f;
    bomberEnemies.Spawn(BomberEnemy{ rect, Palette::Bomber, direction, initialBombTimer });
    RollNextBomberTimer();
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
        // NANG CAP SAU WAVE (Track C - Nguoi 2, Phase 3): cycle 3 lua chon bang Trai/Phai -
        // tai dung CycleDifficultyLeft/Right cua MenuInput (RANH trong man hinh nay, chi
        // dung o UpdateMenu() cho DIFFICULTY - xem input_system.h), KHONG them phim moi.
        if (input.CycleDifficultyLeft || input.CycleDifficultyRight) {
            int dir = input.CycleDifficultyRight ? 1 : -1;
            selectedUpgrade = (selectedUpgrade + dir + UPGRADE_TYPE_COUNT) % UPGRADE_TYPE_COUNT;
        }

        if (input.Confirm) {
            // Ap dung nang cap dang chon TRUOC KHI sang wave ke. gm.wave o day DA duoc ++
            // TU TRUOC (xem PhysicsSystem::UpdateEnemies()/UpdatePlaying() nhanh BOSS
            // DEFEAT) - tuc DA LA wave SAP choi, nen check "wave boss sap toi" dung thang
            // duoc, khong can suy nguoc. Wave boss: goi ApplyRunUpgrade() THEM 1 lan cho
            // CUNG 1 luot chon (2 lan tong) thay vi them pool/loai rieng - xem upgrade_types.h.
            UpgradeType chosen = (UpgradeType)selectedUpgrade;
            player.ApplyRunUpgrade(chosen);
            if (wave % Config::BOSS_WAVE_INTERVAL == 0) player.ApplyRunUpgrade(chosen);

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

    if (hintTimer > 0.0f) hintTimer -= dt; // Goi y phim tu tat sau Config::HUD_HINT_DURATION giay

    screenShake.Update(dt);
    particles.Update(dt);
    floatingTexts.Update(dt);
    powerUps.Update(dt, Config::POWERUP_FALL_SPEED, (float)Config::SCREEN_H);
    PhysicsSystem::UpdateUfo(*this, dt);
    PhysicsSystem::UpdateKamikaze(*this, dt);
    PhysicsSystem::UpdateWeaverEnemies(*this, dt); // Phase 2, Nguoi 1 - cung khuon UFO/Kamikaze: chay KHONG DIEU KIEN, ke ca boss wave
    PhysicsSystem::UpdateBomberEnemies(*this, dt); // Phase 2, Nguoi 1
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
        particles.Burst({ pr.x + pr.width / 2.0f, pr.y }, Config::MUZZLE_FLASH_PARTICLE_COUNT, Palette::PlayerBullet);
    }

    // Warden/Medic KHONG con duoc goi rieng o day nua: chung la mot phan cua doi hinh nen
    // UpdateEnemies() so huu luon (di chuyen + tut hang cung nhip voi Basic/Tanky/Zigzag).
    // Khoi cu o cho nay phai tu suy ra "doi hinh vua doi huong chua" / "vua kich hoat
    // transition chua" de tranh doi huong 2 lan - toan bo mo hop do bien mat cung voi
    // nguyen nhan cua no. Xem lich su bug lech hang o dau PhysicsSystem::UpdateEnemies().
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
        particles.Burst(bossCenter, 40, Palette::BossEnrage2);
        screenShake.Trigger(0.4f, 12.0f);
        hitStop.Trigger(0.1f); // Nang do hon dong bang thuong (0.04f) - xem physics_system.cpp
        ApplyComboAndScore(Config::BOSS_SCORE_VALUE, bossCenter);
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
    // BUG FIX - duyet bang INDEX, KHONG bang range-for. Ly do khong hien nhien: than vong
    // lap co the LAM DAI THEM chinh hang doi dang duyet - ApplyComboAndScore() (goi tu nhanh
    // ev.scoreValue > 0 ben duoi) push them 1 event "+1 mang" khi diem vua vuot moc
    // Config::EXTRA_LIFE_SCORE_THRESHOLD. Voi range-for, push_back do realloc vector va lam
    // hong CA tham chieu `ev` lan iterator ket thuc -> heap-use-after-free THAT SU (da bat
    // duoc bang AddressSanitizer, khong phai lo ngai ly thuyet), kich hoat moi 5000 diem.
    //
    // Doc lai pendingEvents[i] + goi lai .size() MOI VONG (khong cache ra bien) de vong lap
    // luon nhin thay bo dem hien tai sau bat ky lan realloc nao. Tac dung phu la 1 sua loi
    // thu hai: event "+1 mang" gio THAT SU duoc xu ly (particle GOLD + sfx). Truoc day
    // range-for khong bao gio duyet toi no va clear() cuoi ham xoa luon - moc +1 mang khong
    // he co phan hoi hinh anh/am thanh nao ca.
    for (size_t i = 0; i < pendingEvents.size(); i++) {
        // BAN SAO, khong phai tham chieu: index-based thoi VAN chua du an toan - than vong
        // lap con doc `ev.dropPowerUp`/`ev.wardenReinforcementCount` SAU khi da goi
        // ApplyComboAndScore(), va chinh lan goi do co the realloc vector lam tham chieu
        // treo. GameEvent la POD nho (~60 byte, khong so huu bo nho dong) nen copy 1 lan
        // moi vong la re va cat dut hoan toan moi rui ro dangling.
        const GameEvent ev = pendingEvents[i];
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
        if (ev.scoreValue > 0) ApplyComboAndScore(ev.scoreValue, ev.position);
        if (ev.dropPowerUp) MaybeDropPowerUp(ev.position);

        // WARDEN (Phase 1a - Enemy & Item Revolution, Nguoi 1): "yeu hon" nghia la spawn
        // BasicEnemy BINH THUONG (loai yeu nhat co san - 1 mau, khong ky nang) tai vi tri
        // Warden vua chet, KHONG phai 1 bien the rieng yeu hon nua - Warden da la 1 khoan
        // "dau tu" (WardenEnemy::HP=2, ban nhieu phat hon Basic/Zigzag) nen ban than viec
        // no chi de lai 2 Basic (thay vi tu no) da la phan thuong/hinh phat can bang du,
        // khong can them field/co che rieng cho BasicEnemy chi vi 1 nguon spawn nay.
        // column=-1: KHONG thuoc cot nao trong luoi doi hinh that (spawn ngoai cong thuc
        // luoi, tai toa do pixel tuy y) - considerColumn() trong UpdateEnemies() tu dong
        // bo qua moi column<0, nen 2 con nay se KHONG canh tranh vao trang thai "tien
        // tuyen ban" cua cot nao ca, dung tinh than "linh sinh ra hon loan, khong co doi
        // hinh to chuc" hon la 1 gia dinh column suy doan tu toa do pixel co the sai.
        for (int k = 0; k < ev.wardenReinforcementCount; k++) {
            float offsetX = ((float)k - (float)(ev.wardenReinforcementCount - 1) / 2.0f) * 18.0f;
            Rectangle rect{ ev.position.x + offsetX - 20.0f, ev.position.y - 12.5f, 40.0f, 25.0f };
            basicEnemies.Spawn(BasicEnemy{ rect, Palette::BasicA, -1 });
        }
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
    postProcess.Init(); // Bloom/CRT (Config::BLOOM_ENABLED/CRT_ENABLED) - xem post_process.h
    background.Init();  // Starfield - xem parallax.h

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
        ClearBackground(Palette::Background);

        background.Draw(); // Starfield - duoi cung MOI trang thai (Menu/Playing/EndScreen...), truoc noi dung tung state

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
        postProcess.Render(renderTarget, src, dst); // Bloom + CRT (neu bat) - fallback ve dung 1 DrawTexturePro nhu truoc neu ca 2 tat/loi luc Init()

        // OVERLAY DO LUONG: ve o TOA DO MAN HINH THAT (ngoai canh render texture noi bo
        // 800x600 vua upscale o tren) - luon sac net va o dung goc man hinh du dang
        // Fullscreen ty le nao, khong bi anh huong boi buoc letterbox/pillarbox.
        if (showDebugOverlay) RenderSystem::DrawDebugOverlay(*this);

        EndDrawing();
    }

    UnloadRenderTexture(renderTarget);
    postProcess.Shutdown();
    if (gameFont.texture.id != GetFontDefault().texture.id) UnloadFont(gameFont); // Chi unload neu KHONG phai font fallback mac dinh cua raylib
    sprites.Unload();
    audio.Shutdown();
    CloseWindow();
    FileLogger::Shutdown();
}
