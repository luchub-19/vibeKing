#include "physics_system.h"
#include <cmath>
#include <array>
#include <vector>
#include "game_manager.h"

// Hinh dang hieu ung "ha guc 1 dich thuong" (Basic/Zigzag/Tanky/Kamikaze deu dung
// chung 1 khuon: no to 14 hat + explosion sfx + rung nhe + cong diem co combo + co co
// hoi roi power-up) - cac nhanh Kamikaze can chinh lai so hat/do rung se tu ghi de sau
// khi goi ham nay. Tach thanh helper de khong lap lai 5 dong giong het nhau o nhieu cho.
static GameEvent MakeEnemyKilledEvent(Vector2 position, Color color, int scoreValue) {
    GameEvent ev;
    ev.position = position;
    ev.color = color;
    ev.particleCount = 14;
    ev.sfx = SfxType::Explosion;
    ev.shakeDuration = 0.12f;
    ev.shakeIntensity = 4.0f;
    ev.scoreValue = scoreValue;
    ev.dropPowerUp = true;
    return ev;
}

// Di chuyen doi hinh ngang (dung chung cho Basic/Tanky/Zigzag, thay vi lap lai 3 lan y
// het nhau). `formationOffset` la phan lech KHONG thuoc doi hinh that (vd dao dong sin
// cua Zigzag) da duoc cong san vao r.x TRUOC khi goi ham nay - mac dinh 0 cho Basic/Tanky
// (khong co hieu ung phu nao ca). Van la ham thuong (khong virtual), khop DOD - chi rut
// gon phan than vong lap giong het nhau, khong doi kien truc pool rieng biet.
bool PhysicsSystem::ApplyFormationMoveX(Rectangle& r, float direction, float speed, float dt, float formationOffset) {
    r.x += direction * speed * dt;
    float formationX = r.x - formationOffset;
    return (formationX <= 0.0f || formationX + r.width >= Config::SCREEN_W);
}

// Khi hitEdge: tut 1 hang + kep lai trong man hinh + kiem tra thua cuoc (dich cham day
// player). Tra ve true neu GAME_OVER vua duoc kich hoat - noi goi ngat vong lap ngay
// (return), giu dung hanh vi cu (dich dau tien cham day la dung lai, khong xu ly tiep
// cac dich con lai trong wave).
bool PhysicsSystem::DescendRowAndCheckGameOver(GameManager& gm, Rectangle& r) {
    r.y += 20.0f;
    if (r.x < 0) r.x = 0;
    if (r.x + r.width > Config::SCREEN_W) r.x = Config::SCREEN_W - r.width;
    if (EnemyBottom(r) >= gm.player.GetY()) {
        gm.audio.PlayGameOver();
        gm.lastSubmitResult = gm.leaderboard.TrySubmit(gm.player.GetScore(), gm.wave);
        gm.RequestTransition(GameState::GAME_OVER);
        return true;
    }
    return false;
}

// Va cham dan-player voi 1 pool kieu "1 mau, chet ngay khi trung" - Basic/Zigzag/Kamikaze
// deu khop dung khuon nay (chi khac pool/grid/mang pendingKill/SCORE_VALUE, va vai truong
// GameEvent muon tuy chinh them qua customizeEvent). KHONG dung cho Tanky (co HP, "trung
// nhung chua chet" la 1 nhanh rieng that su khac ve logic) hay Boss (1 the duy nhat toan
// cuc, khong nam trong pool+mang pendingKill nao ca) - ep 2 truong hop do vao chung 1 khuon
// se lam ham kho doc hon la giup ich. Tra ve true neu dan da "tieu thu" xong (goi noi cap
// nhat `consumed` cua vong lap ngoai; `removed` duoc ghi truc tiep qua tham chieu).
template <typename PoolT, size_t N, typename CustomizeEventFn>
bool PhysicsSystem::ResolveOneHitKillCollision(GameManager& gm, Bullet& bullet, size_t bulletIndex,
                                        PoolT& pool, SpatialGrid& grid, const Rectangle& bulletRect,
                                        std::array<bool, N>& pendingKill, std::vector<int>& candidates,
                                        int scoreValue, bool& removed, CustomizeEventFn customizeEvent) {
    grid.QueryIndices(bulletRect, candidates);
    for (int idx : candidates) {
        if (pendingKill[idx]) continue; // Da bi bullet khac trong frame nay ha roi
        auto& e = pool[idx];
        if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

        pendingKill[idx] = true;
        gm.hitStop.Trigger(0.04f);
        GameEvent ev = MakeEnemyKilledEvent(EnemyCenter(e.rect), e.color, scoreValue);
        customizeEvent(ev);
        gm.pendingEvents.push_back(ev);

        // PIERCING SHOT: neu dan con xuyen duoc, KHONG Destroy() - no van con o dung
        // index `bulletIndex`, tiep tuc bay va co the trung them muc tieu khac o frame
        // sau. removed=false -> vong lap ben ngoai se KHONG tang i sai (van dung, vi
        // bullet nay khong bi swap).
        removed = !bullet.ConsumePierce();
        if (removed) gm.playerBullets.Destroy(bulletIndex);
        return true;
    }
    return false;
}
void PhysicsSystem::UpdateEnemies(GameManager& gm, float dt) {
    bool hitEdge = false;

    // Basic/Tanky: chi ap dung doi hinh di chuyen ngang, khong co hanh vi phu.
    for (size_t i = 0; i < gm.basicEnemies.Size(); i++) {
        if (ApplyFormationMoveX(gm.basicEnemies[i].rect, gm.enemyDirection, gm.enemySpeed, dt)) hitEdge = true;
    }
    for (size_t i = 0; i < gm.tankyEnemies.Size(); i++) {
        if (ApplyFormationMoveX(gm.tankyEnemies[i].rect, gm.enemyDirection, gm.enemySpeed, dt)) hitEdge = true;
    }
    // Zigzag: hanh vi rieng (dao dong sin) truoc khi ap doi hinh ngang. CHUAN HOA ECS:
    // ZigzagEnemy khong con tu mang Update() - cong thuc dao dong nam thang o day, noi
    // DUY NHAT sua doi du lieu Zigzag moi frame (xem enemy_types.h).
    for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++) {
        ZigzagEnemy& e = gm.zigzagEnemies[i];
        e.timer += dt;
        // Dao dong ngang quanh vi tri doi hinh bang song sin, cong don delta (khong
        // phai gan tuyet doi) de khong pha vo logic di chuyen doi hinh (dong r.x +=
        // enemyDirection*enemySpeed*dt ngay duoi van ap dung binh thuong).
        float newOffset = sinf(e.timer * ZigzagEnemy::FREQUENCY) * ZigzagEnemy::AMPLITUDE;
        e.rect.x += (newOffset - e.lastOffset); // Chi cong phan thay doi -> khong troi dat tich luy
        e.lastOffset = newOffset;

        // BUG FIX: truyen newOffset lam formationOffset - hitEdge gio duoc xet tren VI TRI
        // DOI HINH THAT (rect.x - newOffset), KHONG con bi anh huong boi bien do dao dong
        // sin (+-AMPLITUDE=18px). Truoc day dung thang rect.x (da bao gom offset sin) nen
        // 1 con Zigzag dao dong ra toi bien co the kich hoat CA DOI HINH doi huong/tut hang
        // som/tre hon dung luc, du doi hinh that chua thuc su cham bien.
        if (ApplyFormationMoveX(e.rect, gm.enemyDirection, gm.enemySpeed, dt, newOffset)) hitEdge = true;
    }
    // Kamikaze KHONG tham gia vong lap tren (pool + spawn logic hoan toan rieng, xem
    // UpdateKamikaze) - dung nhu thiet ke "khong pha hong logic kiem tra bien luoi doi hinh".

    // Phase 1a (Enemy & Item Revolution, Nguoi 1): cong them Warden/Medic vao dieu kien
    // "da don sach wave" - day la 1 NGOAI LE co chu dich duy nhat toi cho phep minh sua
    // trong ham nay (khong dong den bat ky dong nao khac o tren/duoi lien quan toi doi
    // hinh/hitEdge/frontline-shooting cua Basic/Tanky/Zigzag). Ly do bat buoc phai sua:
    // Warden/Medic co ham Update RIENG (UpdateWardenEnemies/UpdateMedicEnemies, xem
    // physics_system.h), nen neu KHONG cong vao day, wave se bi bao "WAVE_CLEAR" (wave++,
    // nop leaderboard) ngay khi Basic/Tanky/Zigzag het, du Warden/Medic van con song tren
    // man hinh luc do - day la bug that (khong phai ly thuyet), da lan theo toan bo duong
    // goi RequestTransition()/UpdatePlaying() de xac nhan truoc khi sua (xem test moi
    // trong test_game_manager.cpp).
    size_t activeCount = gm.basicEnemies.Size() + gm.tankyEnemies.Size() + gm.zigzagEnemies.Size()
                        + gm.wardenEnemies.Size() + gm.medicEnemies.Size();
    if (activeCount == 0) {
        // WAVE PROGRESSION: khong con la man hinh "WIN" cuoi cung - don sach 1 wave thi
        // sang wave ke tiep, kho hon, giu nguyen diem/mang (xem InitLevel(false)).
        gm.audio.PlayWaveClear();
        gm.wave++;
        gm.lastSubmitResult = gm.leaderboard.TrySubmit(gm.player.GetScore(), gm.wave);
        gm.RequestTransition(GameState::WAVE_CLEAR);
        return;
    }

    DifficultyStats stats = GetDifficultyStats(gm.difficulty);

    // DYNAMIC DIFFICULTY ADJUSTMENT (B2): nhan gm.ddaSpeedMul (tinh lai moi checkpoint -
    // xem GameManager::UpdatePlaying() nhanh BOSS DEFEAT trong game_manager.cpp) vao CA 2
    // tran duoi day TRUOC khi dung tiep ben duoi. Sua tren BAN SAO cuc bo `stats`, KHONG
    // dung vao Config::g_difficultyTable - bang goc giu nguyen ven cho muc do kho NGUOI
    // CHOI TU CHON trong Menu, DDA chi la 1 tang dieu chinh CONG THEM rieng cua wave nay.
    // enemyFireRate la KHOANG CACH giua 2 phat (giay) - nho hon = ban nhanh hon, nen CHIA
    // (khong phai NHAN) khi ddaSpeedMul > 1 (dang "thuong" nguoi choi choi tot).
    stats.enemySpeedMax *= gm.ddaSpeedMul;
    stats.enemyFireRate /= gm.ddaSpeedMul;

    if (hitEdge) {
        gm.enemyDirection *= -1;
        gm.enemySpeed = fminf(gm.enemySpeed + Config::ENEMY_SPEED_INC, stats.enemySpeedMax);

        for (size_t i = 0; i < gm.basicEnemies.Size(); i++) {
            if (DescendRowAndCheckGameOver(gm, gm.basicEnemies[i].rect)) return;
        }
        for (size_t i = 0; i < gm.tankyEnemies.Size(); i++) {
            if (DescendRowAndCheckGameOver(gm, gm.tankyEnemies[i].rect)) return;
        }
        for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++) {
            if (DescendRowAndCheckGameOver(gm, gm.zigzagEnemies[i].rect)) return;
        }
    }

    gm.enemyFireTimer += dt;
    if (gm.enemyFireTimer >= stats.enemyFireRate * gm.waveFireRateMul) {
        gm.enemyFireTimer = 0.0f;

        // AI "line of sight": nhom dich theo cot gan luc spawn, xet CHUNG ca 3 pool.
        // Trong moi cot, chi dich nam THAP NHAT (khong bi dong doi cung cot che phia
        // truoc) moi duoc cap quyen ban - random chon 1 trong cac "tien tuyen" do.
        struct Frontline { EnemyKind kind; int index; float bottom; };
        std::vector<Frontline> frontlinePerColumn(gm.levelGrid.cols, { EnemyKind::Basic, -1, 0.0f });

        auto considerColumn = [&](EnemyKind kind, int column, int index, float bottom) {
            if (column < 0 || column >= gm.levelGrid.cols) return;
            Frontline& best = frontlinePerColumn[column];
            if (best.index == -1 || bottom > best.bottom) {
                best = { kind, index, bottom };
            }
        };

        for (size_t i = 0; i < gm.basicEnemies.Size(); i++) {
            considerColumn(EnemyKind::Basic, gm.basicEnemies[i].column, (int)i, EnemyBottom(gm.basicEnemies[i].rect));
        }
        for (size_t i = 0; i < gm.tankyEnemies.Size(); i++) {
            considerColumn(EnemyKind::Tanky, gm.tankyEnemies[i].column, (int)i, EnemyBottom(gm.tankyEnemies[i].rect));
        }
        for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++) {
            considerColumn(EnemyKind::Zigzag, gm.zigzagEnemies[i].column, (int)i, EnemyBottom(gm.zigzagEnemies[i].rect));
        }

        std::vector<Frontline> shooters;
        for (const Frontline& f : frontlinePerColumn) {
            if (f.index != -1) shooters.push_back(f);
        }

        if (!shooters.empty()) {
            const Frontline& pick = shooters[GetRandomValue(0, (int)shooters.size() - 1)];
            Rectangle rect{};
            switch (pick.kind) {
                case EnemyKind::Basic:  rect = gm.basicEnemies[pick.index].rect;  break;
                case EnemyKind::Tanky:  rect = gm.tankyEnemies[pick.index].rect;  break;
                case EnemyKind::Zigzag: rect = gm.zigzagEnemies[pick.index].rect; break;
            }
            EnemyShoot(gm, EnemyCenterX(rect), EnemyBottomY(rect));
        }
    }
}

// ==========================================
// WARDEN/MEDIC (Phase 1a - Enemy & Item Revolution, Nguoi 1): 2 ham RIENG, KHONG goi tu
// ben trong UpdateEnemies() o tren (xem physics_system.h + GameManager::UpdatePlaying() de
// biet ly do/cach tranh doi huong-2-lan). Van dung lai NGUYEN VEN ApplyFormationMoveX()/
// DescendRowAndCheckGameOver() (2 helper thuan, khong rieng cho Basic/Tanky) nen hanh vi
// di chuyen/tut hang giong het, chi khac o cho KHONG tu quyet dinh doi huong khi
// `alreadyFlipped` da la true.
// ==========================================
void PhysicsSystem::UpdateWardenEnemies(GameManager& gm, float dt, bool alreadyFlipped) {
    bool hitEdge = false;
    for (size_t i = 0; i < gm.wardenEnemies.Size(); i++) {
        if (ApplyFormationMoveX(gm.wardenEnemies[i].rect, gm.enemyDirection, gm.enemySpeed, dt)) hitEdge = true;
    }
    if (!hitEdge) return;

    if (!alreadyFlipped) {
        DifficultyStats stats = GetDifficultyStats(gm.difficulty);
        stats.enemySpeedMax *= gm.ddaSpeedMul;
        gm.enemyDirection *= -1;
        gm.enemySpeed = fminf(gm.enemySpeed + Config::ENEMY_SPEED_INC, stats.enemySpeedMax);
    }
    for (size_t i = 0; i < gm.wardenEnemies.Size(); i++) {
        if (DescendRowAndCheckGameOver(gm, gm.wardenEnemies[i].rect)) return;
    }
}

void PhysicsSystem::UpdateMedicEnemies(GameManager& gm, float dt, bool alreadyFlipped) {
    bool hitEdge = false;
    for (size_t i = 0; i < gm.medicEnemies.Size(); i++) {
        MedicEnemy& e = gm.medicEnemies[i];
        if (ApplyFormationMoveX(e.rect, gm.enemyDirection, gm.enemySpeed, dt)) hitEdge = true;

        // HOI MAU: dem LEN moi frame (Config::MEDIC_HEAL_INTERVAL giay/lan, tru dan -
        // khong gan thang ve 0 - de khong bi troi pha neu dt khong chia het interval).
        // Muc tieu la TankyEnemy GAN NHAT (khoang cach Euclid tu tam Medic) trong so
        // NHUNG CON CHUA DAY MAU - bo qua Tanky da full de khong "hoi suong" mai 1 con da
        // day trong khi 1 con khac dang thuong o xa hon van khong duoc hoi (doc "gan
        // nhat con song" theo tinh than co ich cho AI, khong phai nghia den bat ke hp).
        e.healTimer += dt;
        if (e.healTimer >= Config::MEDIC_HEAL_INTERVAL) {
            e.healTimer -= Config::MEDIC_HEAL_INTERVAL;

            int nearestIdx = -1;
            float nearestDistSq = 0.0f;
            Vector2 medicCenter = EnemyCenter(e.rect);
            for (size_t j = 0; j < gm.tankyEnemies.Size(); j++) {
                if (gm.tankyEnemies[j].hp >= TankyEnemy::HP) continue;
                Vector2 c = EnemyCenter(gm.tankyEnemies[j].rect);
                float dx = c.x - medicCenter.x;
                float dy = c.y - medicCenter.y;
                float distSq = dx * dx + dy * dy;
                if (nearestIdx == -1 || distSq < nearestDistSq) {
                    nearestIdx = (int)j;
                    nearestDistSq = distSq;
                }
            }
            if (nearestIdx != -1) {
                TankyEnemy& target = gm.tankyEnemies[(size_t)nearestIdx];
                target.hp += Config::MEDIC_HEAL_AMOUNT;
                if (target.hp > TankyEnemy::HP) target.hp = TankyEnemy::HP; // Khong "sieu hoi" vuot HP goc
            }
        }
    }
    if (!hitEdge) return;

    if (!alreadyFlipped) {
        DifficultyStats stats = GetDifficultyStats(gm.difficulty);
        stats.enemySpeedMax *= gm.ddaSpeedMul;
        gm.enemyDirection *= -1;
        gm.enemySpeed = fminf(gm.enemySpeed + Config::ENEMY_SPEED_INC, stats.enemySpeedMax);
    }
    for (size_t i = 0; i < gm.medicEnemies.Size(); i++) {
        if (DescendRowAndCheckGameOver(gm, gm.medicEnemies[i].rect)) return;
    }
}

void PhysicsSystem::EnemyShoot(GameManager& gm, float x, float y) {
    // Roll ngau nhien giua 2 pattern: thang xuong (kinh dien) hoac nham thang vi tri
    // player hien tai - giu pattern co dien lam mac dinh (chiem da so) de khong bien
    // moi phat dan thuong thanh homing missile, mat dac trung "nhu mua" kinh dien.
    if ((float)GetRandomValue(0, 999) / 1000.0f < Config::ENEMY_AIMED_SHOT_CHANCE) {
        Vector2 target = gm.player.GetCenter();
        Vector2 dir{ target.x - x, target.y - y };
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (len < 1.0f) len = 1.0f;
        Vector2 vel{ (dir.x / len) * Config::ENEMY_BULLET_SPEED, (dir.y / len) * Config::ENEMY_BULLET_SPEED };
        gm.enemyBullets.Fire(x, y, vel);
    } else {
        gm.enemyBullets.Fire(x, y, { 0.0f, Config::ENEMY_BULLET_SPEED });
    }
}

void PhysicsSystem::FireRadialBurst(GameManager& gm, float x, float y, int count, float speed) {
    for (int i = 0; i < count; i++) {
        float angle = (2.0f * PI * (float)i) / (float)count;
        Vector2 vel{ cosf(angle) * speed, sinf(angle) * speed };
        gm.enemyBullets.Fire(x, y, vel);
    }
}

// ==========================================
// KAMIKAZE - HOMING LIEN TUC (B1): gioi han xoay Config-doc-lap KAMIKAZE_TURN_RATE
// (rad/s) - hang so RIENG cua file nay, khong dua len Config::/balance.json vi B1 duoc
// giao lam voc chuc "0 file chung" (xem bang phan cong task). Sau nay neu can designer
// tune tu JSON, chi viec chuyen thanh Config::KAMIKAZE_TURN_RATE + 1 dong Assign() trong
// config.cpp, khong dung gi den cong thuc ben duoi.
// ==========================================
static constexpr float KAMIKAZE_TURN_RATE = 3.5f; // rad/s (~200 do/s) - du "bam duoi" nhung khong aim-bot tuyet doi, nguoi choi van ne duoc bang cach doi huong gap

void PhysicsSystem::UpdateKamikaze(GameManager& gm, float dt) {
    gm.kamikazeSpawnTimer -= dt;
    if (gm.kamikazeSpawnTimer <= 0.0f && gm.kamikazeEnemies.Size() < Config::MAX_KAMIKAZE) {
        gm.SpawnKamikaze();
    }

    for (size_t i = 0; i < gm.kamikazeEnemies.Size(); ) {
        KamikazeEnemy& k = gm.kamikazeEnemies[i];

        // TRUOC DAY: k.vel duoc tinh DUY NHAT 1 LAN luc SpawnKamikaze() ("khoa muc tieu
        // vinh vien" - xem comment cu tren struct KamikazeEnemy trong enemy_types.h/
        // GameManager::SpawnKamikaze(), gio da lac hau nhung giu lai lam lich su vi ngoai
        // pham vi B1). GIO: xoay dan k.vel ve huong player HIEN TAI moi frame, gioi han
        // boi KAMIKAZE_TURN_RATE - khong gan lai huong tuc thi (se thanh aim-bot 100%).
        // Do lon toc do GIU NGUYEN dung bang do lon cua vel hien tai (doc lai tu chinh no
        // thay vi Config::KAMIKAZE_SPEED) de tuong thich san voi bat ky jitter/randomize
        // toc do nao co the them vao luc spawn sau nay ma khong can sua file nay them lan
        // nua - homing CHI xoay HUONG, khong dung toi TOC DO.
        float speed = sqrtf(k.vel.x * k.vel.x + k.vel.y * k.vel.y);
        if (speed > 0.01f) {
            Vector2 toPlayer{ gm.player.GetCenter().x - EnemyCenter(k.rect).x,
                               gm.player.GetCenter().y - EnemyCenter(k.rect).y };
            float desiredAngle = atan2f(toPlayer.y, toPlayer.x);
            float currentAngle  = atan2f(k.vel.y, k.vel.x);

            float angleDiff = desiredAngle - currentAngle;
            while (angleDiff > PI)  angleDiff -= 2.0f * PI; // Chuan hoa ve [-PI, PI] - luon
            while (angleDiff < -PI) angleDiff += 2.0f * PI; // xoay theo duong NGAN hon

            float maxTurn = KAMIKAZE_TURN_RATE * dt;
            float appliedTurn = fmaxf(-maxTurn, fminf(maxTurn, angleDiff));
            float newAngle = currentAngle + appliedTurn;
            k.vel = { cosf(newAngle) * speed, sinf(newAngle) * speed };
        }

        k.rect.x += k.vel.x * dt;
        k.rect.y += k.vel.y * dt;

        bool offscreen = k.rect.y > Config::SCREEN_H || k.rect.x < -Config::KAMIKAZE_WIDTH ||
                          k.rect.x > Config::SCREEN_W;
        if (offscreen) {
            gm.kamikazeEnemies.Destroy(i); // Bay khoi man hinh ma khong trung ai -> bien mat, khong phat
            continue;
        }

        // Va cham truc tiep (lao vao) voi player - sat thuong kieu ho hen, khac han cac
        // loai dich khac (chi gay sat thuong qua dan cua chung, khong bao gio cham truc
        // tiep vao player). Hieu ung day qua event queue, khong goi audio/particle
        // truc tiep o day (xem physics_system.h).
        if (CheckCollisionRecs(k.rect, gm.player.GetRect())) {
            GameEvent ev;
            ev.position = EnemyCenter(k.rect);
            ev.color = RED;
            ev.particleCount = 16;
            ev.sfx = SfxType::Explosion;
            ev.shakeDuration = 0.2f;
            ev.shakeIntensity = 7.0f;
            gm.pendingEvents.push_back(ev);

            if (gm.player.TakeDamage()) {
                GameEvent hitEv;
                hitEv.sfx = SfxType::Hit;
                gm.pendingEvents.push_back(hitEv);
            }
            gm.hitStop.Trigger(0.04f); // Kamikaze chet that (lao vao player) - duong chet con lai ngoai ResolveOneHitKillCollision
            gm.kamikazeEnemies.Destroy(i);
            continue;
        }

        i++;
    }
}

// ==========================================
// MYSTERY SHIP (UFO)
// ==========================================
void PhysicsSystem::UpdateUfo(GameManager& gm, float dt) {
    if (!gm.ufoActive) {
        gm.ufoSpawnTimer -= dt;
        if (gm.ufoSpawnTimer <= 0.0f) gm.SpawnUfo();
        return;
    }

    gm.ufoRect.x += (float)gm.ufoDirection * Config::UFO_SPEED * dt;

    bool exitedRight = (gm.ufoDirection > 0 && gm.ufoRect.x > Config::SCREEN_W);
    bool exitedLeft  = (gm.ufoDirection < 0 && gm.ufoRect.x + gm.ufoRect.width < 0);
    if (exitedRight || exitedLeft) {
        gm.ufoActive = false;
        gm.RollNextUfoTimer(); // Bay het man hinh ma khong bi ban trung -> hen lan sau
    }
}

// ==========================================
// WEAVER (Phase 2 - Enemy & Item Revolution, Nguoi 1): bay ngang xuyen man hinh theo
// duong hinh sin (khong ban) - moi de doa la KHO BAN TRUNG do duong bay lac, khac han
// Bomber (di thang, de ban hon, nhung tha bom ap luc tu tren cao). Khong tham gia
// activeCount/hitEdge cua UpdateEnemies() - dung y Kamikaze/UFO.
// ==========================================
void PhysicsSystem::UpdateWeaverEnemies(GameManager& gm, float dt) {
    gm.weaverSpawnTimer -= dt;
    if (gm.weaverSpawnTimer <= 0.0f && gm.weaverEnemies.Size() < Config::MAX_WEAVER_ENEMIES) {
        gm.SpawnWeaver();
    }

    for (size_t i = 0; i < gm.weaverEnemies.Size(); ) {
        WeaverEnemy& w = gm.weaverEnemies[i];
        w.rect.x += (float)w.direction * Config::WEAVER_SPEED_X * dt;
        w.phase += Config::WEAVER_WEAVE_FREQUENCY * dt;
        // baseY la TAM co dinh (khong doi) - rect.y duoc TINH LAI moi frame tu baseY +
        // sin(phase), khong cong don truc tiep vao rect.y, nen khong the "troi" xa dan
        // khoi bien do mong muon du chay bao lau.
        w.rect.y = w.baseY + sinf(w.phase) * Config::WEAVER_WEAVE_AMPLITUDE;

        bool exitedRight = (w.direction > 0 && w.rect.x > Config::SCREEN_W);
        bool exitedLeft  = (w.direction < 0 && w.rect.x + w.rect.width < 0);
        if (exitedRight || exitedLeft) {
            gm.weaverEnemies.Destroy(i); // Bay het man hinh ma khong trung ai -> bien mat, khong phat (giong UFO/Kamikaze)
            continue;
        }
        i++;
    }
}

// ==========================================
// BOMBER (Phase 2 - Enemy & Item Revolution, Nguoi 1): bay ngang THANG (khong lac nhu
// Weaver - de phan biet 2 loai tu xa), dinh ky tha 1 dan xuong thang qua enemyBullets
// (dung CHUNG pool/toc do voi moi dan dich khac - khong tao rieng khai niem "bom").
// ==========================================
void PhysicsSystem::UpdateBomberEnemies(GameManager& gm, float dt) {
    gm.bomberSpawnTimer -= dt;
    if (gm.bomberSpawnTimer <= 0.0f && gm.bomberEnemies.Size() < Config::MAX_BOMBER_ENEMIES) {
        gm.SpawnBomber();
    }

    for (size_t i = 0; i < gm.bomberEnemies.Size(); ) {
        BomberEnemy& b = gm.bomberEnemies[i];
        b.rect.x += (float)b.direction * Config::BOMBER_SPEED_X * dt;

        b.bombTimer -= dt;
        if (b.bombTimer <= 0.0f) {
            b.bombTimer = Config::BOMBER_BOMB_INTERVAL;
            Vector2 vel{ 0.0f, Config::ENEMY_BULLET_SPEED }; // Y duong = roi xuong (Y+ la xuong duoi)
            gm.enemyBullets.Fire(b.rect.x + b.rect.width / 2.0f - Config::BULLET_WIDTH / 2.0f,
                                  b.rect.y + b.rect.height, vel);
        }

        bool exitedRight = (b.direction > 0 && b.rect.x > Config::SCREEN_W);
        bool exitedLeft  = (b.direction < 0 && b.rect.x + b.rect.width < 0);
        if (exitedRight || exitedLeft) {
            gm.bomberEnemies.Destroy(i);
            continue;
        }
        i++;
    }
}

// ==========================================
// BOSS - dung chung EnemyPool<Boss,1> nhu moi loai dich khac (xem game_manager.h);
// Size()==0 nghia la chua spawn/da bi ha, Size()==1 nghia la con song - khong con `bool
// bossActive` rieng phai giu dong bo thu cong voi hp.
// ==========================================
void PhysicsSystem::UpdateBoss(GameManager& gm, float dt) {
    if (gm.bossPool.Size() == 0) return;
    Boss& boss = gm.bossPool[0];
    int stage = BossStage(boss);
    const BossTypeDescriptor& desc = GetBossTypeDescriptor(boss.type);

    // DI CHUYEN (B4 - DATA-DRIVEN): dispatch qua desc.movement (BossTypeDescriptor, xem
    // enemy_types.h) THAY VI switch(boss.type) truoc day. Vanguard = Pace (pace het chieu
    // rong man hinh, tang toc theo stage - hanh vi GOC, KHONG doi). Sentinel/Swarmer = Sway
    // CUNG 1 cong thuc sin, chi khac bien do/tan so doc qua con tro desc.swayAmplitude/
    // swayFrequency (Sentinel: nhe/cham - ap luc den tu khien ben duoi; Swarmer: rong/nhanh
    // - cam giac that thuong, kho ngam hon han).
    switch (desc.movement) {
        case BossMovementPattern::Pace: {
            float speed = (stage == 1) ? Config::BOSS_SPEED_STAGE1 : (stage == 2) ? Config::BOSS_SPEED_STAGE2 : Config::BOSS_SPEED_STAGE3;
            boss.rect.x += (float)boss.direction * speed * dt;
            if (boss.rect.x <= 0.0f || boss.rect.x + boss.rect.width >= Config::SCREEN_W) {
                boss.direction *= -1;
                boss.rect.x = fmaxf(0.0f, fminf(boss.rect.x, Config::SCREEN_W - boss.rect.width));
            }
            break;
        }
        case BossMovementPattern::Sway: {
            boss.phaseAccum += (*desc.swayFrequency) * dt;
            float sway = sinf(boss.phaseAccum) * (*desc.swayAmplitude);
            boss.rect.x = fmaxf(0.0f, fminf(boss.baseX + sway, Config::SCREEN_W - boss.rect.width));
            break;
        }
    }

    // KHIEN TAM (B4 - DATA-DRIVEN): dispatch qua desc.hasShieldMechanic THAY VI if
    // (boss.type == BossType::Sentinel) truoc day - dinh ky bat/tat, doc lap voi
    // fireTimer/stage, buoc nguoi choi cho dung nhip thay vi giu nut ban suot tran. BossType
    // moi muon co khien chi can bat co nay + gan 3 con tro Config trong
    // g_bossTypeDescriptors[], khong sua gi o day.
    if (desc.hasShieldMechanic) {
        boss.phaseTimer -= dt;
        if (boss.phaseTimer <= 0.0f) {
            boss.shieldActive = !boss.shieldActive;
            boss.phaseTimer = boss.shieldActive ? (*desc.shieldDuration) : (*desc.shieldInterval);
            // Goi am thanh/particle/rung man hinh TRUC TIEP (khong qua gm.pendingEvents)
            // - UpdateBoss() chay TRUOC CheckCollisions() trong UpdatePlaying(), von xoa
            // sach pendingEvents ngay dau ham (xem events.h: hang doi chi danh cho hieu
            // ung PHAT SINH TU va cham). Day vao hang doi o day se bi CheckCollisions()
            // xoa mat truoc khi ProcessEvents() kip xu ly - dung idiom giong het
            // audio.PlayShoot() duoc goi truc tiep tu ket qua Player::Update() trong
            // GameManager::UpdatePlaying(), khong qua event queue.
            gm.audio.PlayBossPhase();
            gm.particles.Burst(EnemyCenter(boss.rect), 20, boss.shieldActive ? SKYBLUE : GRAY);
            gm.screenShake.Trigger(0.15f, 4.0f);
        }
    }

    // TRIEU HOI TIEP VIEN (B4 - DATA-DRIVEN): dispatch qua desc.hasSummonMechanic THAY VI
    // if (boss.type == BossType::Swarmer) truoc day - "muon" lai dung
    // GameManager::SpawnKamikaze() da co san (PhysicsSystem la friend cua GameManager - xem
    // game_manager.h) thay vi tu viet duong bay/muc tieu rieng. Vi doi hinh dang TRONG
    // trong boss wave (xem InitLevel()), SpawnKamikaze() se tu dong roi vao nhanh "spawn tu
    // ngoai man hinh" cua no (xem comment tren struct KamikazeEnemy) - dung y muon.
    if (desc.hasSummonMechanic) {
        boss.summonTimer -= dt;
        if (boss.summonTimer <= 0.0f) {
            for (int i = 0; i < (*desc.summonCount) && gm.kamikazeEnemies.Size() < Config::MAX_KAMIKAZE; i++) {
                gm.SpawnKamikaze();
            }
            boss.summonTimer = *desc.summonInterval;
            gm.audio.PlayBossPhase();
            gm.particles.Burst(EnemyCenter(boss.rect), 14, ORANGE);
        }
    }

    // BAN - cung cong thuc cho ca 3 loai (nhip ban theo stage, doi khi ban toa tron). Boss
    // nao CO khien (desc.hasShieldMechanic) VA dang bat khien thi dung nhip ban RIENG
    // (desc.shieldFireInterval) nhanh hon han - bu lai cho viec khong the bi trung dan luc
    // do (truoc day chi Sentinel, gio tong quat cho MOI BossType co khien).
    float fireInterval = (stage == 1) ? Config::BOSS_FIRE_INTERVAL_STAGE1 : (stage == 2) ? Config::BOSS_FIRE_INTERVAL_STAGE2 : Config::BOSS_FIRE_INTERVAL_STAGE3;
    if (desc.hasShieldMechanic && boss.shieldActive) {
        fireInterval = *desc.shieldFireInterval;
    }

    boss.fireTimer += dt;
    if (boss.fireTimer >= fireInterval) {
        boss.fireTimer = 0.0f;
        float originX = EnemyCenterX(boss.rect);
        float originY = boss.rect.y + boss.rect.height;

        float radialChance = (stage == 3) ? Config::BOSS_RADIAL_CHANCE_STAGE3
                            : (stage == 2) ? Config::BOSS_RADIAL_CHANCE_STAGE2 : 0.0f;
        if ((float)GetRandomValue(0, 999) / 1000.0f < radialChance) {
            FireRadialBurst(gm, originX, originY, Config::RADIAL_BURST_COUNT, Config::BOSS_BULLET_SPEED);
        } else {
            Vector2 target = gm.player.GetCenter();
            Vector2 dir{ target.x - originX, target.y - originY };
            float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
            if (len < 1.0f) len = 1.0f;
            Vector2 vel{ (dir.x / len) * Config::BOSS_BULLET_SPEED, (dir.y / len) * Config::BOSS_BULLET_SPEED };
            gm.enemyBullets.Fire(originX, originY, vel);
        }
    }
}

// ==========================================
// VA CHAM
// ==========================================
void PhysicsSystem::CheckCollisions(GameManager& gm) {
    // Hang doi hieu ung cho frame nay - se duoc GameManager::ProcessEvents() xu ly
    // ngay sau khi ham nay tra ve (xem UpdatePlaying()).
    gm.pendingEvents.clear();

    // Bam lai enemy dang song vao luoi khong gian moi frame (O(M)) - doi lai moi vien
    // dan chi can test va cham voi enemy nam trong (cac) o no phu toi, thay vi toan bo
    // danh sach enemy (dep vong lap long nhau O(N_bullet x M_enemy) truoc day). Moi
    // loai dich co 1 grid rieng, khop voi cac Pool tinh.
    gm.basicGrid.Clear();
    gm.tankyGrid.Clear();
    gm.zigzagGrid.Clear();
    gm.kamikazeGrid.Clear();
    gm.wardenGrid.Clear();
    gm.medicGrid.Clear();
    gm.weaverGrid.Clear(); // Phase 2, Nguoi 1
    gm.bomberGrid.Clear(); // Phase 2, Nguoi 1
    for (size_t i = 0; i < gm.basicEnemies.Size(); i++)    gm.basicGrid.Insert((int)i, gm.basicEnemies[i].rect);
    for (size_t i = 0; i < gm.tankyEnemies.Size(); i++)    gm.tankyGrid.Insert((int)i, gm.tankyEnemies[i].rect);
    for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++)   gm.zigzagGrid.Insert((int)i, gm.zigzagEnemies[i].rect);
    for (size_t i = 0; i < gm.kamikazeEnemies.Size(); i++) gm.kamikazeGrid.Insert((int)i, gm.kamikazeEnemies[i].rect);
    for (size_t i = 0; i < gm.wardenEnemies.Size(); i++)   gm.wardenGrid.Insert((int)i, gm.wardenEnemies[i].rect);
    for (size_t i = 0; i < gm.medicEnemies.Size(); i++)    gm.medicGrid.Insert((int)i, gm.medicEnemies[i].rect);
    for (size_t i = 0; i < gm.weaverEnemies.Size(); i++)   gm.weaverGrid.Insert((int)i, gm.weaverEnemies[i].rect); // Phase 2, Nguoi 1
    for (size_t i = 0; i < gm.bomberEnemies.Size(); i++)   gm.bomberGrid.Insert((int)i, gm.bomberEnemies[i].rect); // Phase 2, Nguoi 1

    // Boss: chi la 1 pool nua (Capacity=1) - khong co nhanh rieng nao kiem tra
    // "isBossWave"; Size()==0 tu dong khong dang ky gi vao grid, y het cach 1 pool rong
    // (vd Tanky khi wave hien tai khong co Tanky nao) tu dong khong dang ky gi ca.
    if (gm.bossPool.Size() > 0) {
        gm.bossGrid.Clear();
        gm.bossGrid.Insert(0, gm.bossPool[0].rect); // Rect lon hon 1 o -> tu dong dang ky vao NHIEU o (xem SpatialGrid::Insert)
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
    std::array<bool, Config::MAX_WARDEN_ENEMIES> wardenPendingKill{};
    std::array<bool, Config::MAX_MEDIC_ENEMIES>  medicPendingKill{};
    std::array<bool, Config::MAX_WEAVER_ENEMIES> weaverPendingKill{}; // Phase 2, Nguoi 1
    std::array<bool, Config::MAX_BOMBER_ENEMIES> bomberPendingKill{}; // Phase 2, Nguoi 1

    std::vector<int> candidates; // Tai dung buffer cho moi query, tranh cap phat lap lai

    // Dan player: kiem tra bunker truoc (chan dan), roi moi toi enemy trong o lan can
    for (size_t i = 0; i < gm.playerBullets.GetActiveCount(); ) {
        Bullet& bullet = gm.playerBullets.GetBullet(i);
        // Dung swept rect (CCD) thay vi rect tinh o vi tri cuoi frame - xem
        // Bullet::GetSweptRect() de biet ly do (xuyen tao voxel/dich mong o toc do cao).
        Rectangle bulletRect = bullet.GetSweptRect();
        bool consumed = false; // Da xu ly xong bullet nay cho frame nay (dung kiem tra tiep cac pool khac)
        bool removed = false;  // Bullet THUC SU bi Destroy() (swap-and-pop) - quyet dinh co tang i hay khong

        for (auto& bunker : gm.bunkers) {
            if (bunker.HandleBulletHit(bulletRect)) {
                gm.playerBullets.Destroy(i);
                consumed = true;
                removed = true;
                break;
            }
        }

        // Trung don ha guc ngay (Basic/Zigzag, luon 1 mau) - dung chung
        // ResolveOneHitKillCollision (dinh nghia o dau file) thay vi lap lai than vong lap.
        if (!consumed) {
            consumed = ResolveOneHitKillCollision(gm, bullet, i, gm.basicEnemies, gm.basicGrid, bulletRect,
                                                   basicPendingKill, candidates, BasicEnemy::SCORE_VALUE,
                                                   removed, [](GameEvent&) {});
        }

        if (!consumed) {
            consumed = ResolveOneHitKillCollision(gm, bullet, i, gm.zigzagEnemies, gm.zigzagGrid, bulletRect,
                                                   zigzagPendingKill, candidates, ZigzagEnemy::SCORE_VALUE,
                                                   removed, [](GameEvent&) {});
        }

        // Medic: 1 mau, ha guc ngay - khong tu ban nen khong the "phan cong lai", chi la 1
        // muc tieu uu tien chien thuat (xem enemy_types.h) - dung chung
        // ResolveOneHitKillCollision nhu Basic/Zigzag, khong can customizeEvent gi rieng.
        if (!consumed) {
            consumed = ResolveOneHitKillCollision(gm, bullet, i, gm.medicEnemies, gm.medicGrid, bulletRect,
                                                   medicPendingKill, candidates, MedicEnemy::SCORE_VALUE,
                                                   removed, [](GameEvent&) {});
        }

        // Tanky: nhieu mau hon - tru hp ngay (an toan, khong dung toi index), chi danh
        // dau pendingKill khi hp that su ve 0.
        if (!consumed) {
            gm.tankyGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (tankyPendingKill[idx]) continue;
                TankyEnemy& e = gm.tankyEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                if (e.hp > 0) e.hp--;
                if (e.hp <= 0) {
                    tankyPendingKill[idx] = true;
                    gm.hitStop.Trigger(0.04f);
                    gm.pendingEvents.push_back(MakeEnemyKilledEvent(EnemyCenter(e.rect), e.color, TankyEnemy::SCORE_VALUE));
                } else {
                    // Dich mau day van con song sau don nay - phan hoi nhe hon de phan
                    // biet voi don ha guc han.
                    // HIT-FLASH: truoc day nhanh nay KHONG set position/particleCount -
                    // trung Tanky ma chua chet hoan toan khong co phan hoi hinh anh nao (chi
                    // sfx+rung). flashOnHit dua vao dung vi tri va cham that (EnemyCenter,
                    // khong phai {0,0} mac dinh) de ProcessEvents() bat 1 flash tai do.
                    GameEvent ev;
                    ev.position = EnemyCenter(e.rect);
                    ev.sfx = SfxType::Hit;
                    ev.shakeDuration = 0.05f;
                    ev.shakeIntensity = 2.0f;
                    ev.flashOnHit = true;
                    gm.pendingEvents.push_back(ev);
                }

                removed = !bullet.ConsumePierce();
                if (removed) gm.playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        // Warden: nhieu mau hon (dung KHUON MAU voi Tanky o tren) - tru hp ngay, chi danh
        // dau pendingKill khi hp that su ve 0. Khac Tanky DUY NHAT o cho: luc CHET THAT (hp
        // <=0), gan them wardenReinforcementCount vao event - GameManager::ProcessEvents()
        // doc field nay de sinh BasicEnemy tang vien tai dung vi tri Warden vua chet (xem
        // events.h).
        if (!consumed) {
            gm.wardenGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (wardenPendingKill[idx]) continue;
                WardenEnemy& e = gm.wardenEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                if (e.hp > 0) e.hp--;
                if (e.hp <= 0) {
                    wardenPendingKill[idx] = true;
                    gm.hitStop.Trigger(0.04f);
                    GameEvent ev = MakeEnemyKilledEvent(EnemyCenter(e.rect), e.color, WardenEnemy::SCORE_VALUE);
                    ev.wardenReinforcementCount = Config::WARDEN_REINFORCEMENT_COUNT;
                    gm.pendingEvents.push_back(ev);
                } else {
                    GameEvent ev;
                    ev.position = EnemyCenter(e.rect);
                    ev.sfx = SfxType::Hit;
                    ev.shakeDuration = 0.05f;
                    ev.shakeIntensity = 2.0f;
                    ev.flashOnHit = true;
                    gm.pendingEvents.push_back(ev);
                }

                removed = !bullet.ConsumePierce();
                if (removed) gm.playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        // Kamikaze: 1 mau, ha guc ngay - grid rieng, khong lien quan gi toi luoi doi hinh.
        // customizeEvent ghi de so hat/do rung manh hon mac dinh (va cham truc tiep, khong
        // chi la 1 vien dan nho).
        if (!consumed) {
            consumed = ResolveOneHitKillCollision(gm, bullet, i, gm.kamikazeEnemies, gm.kamikazeGrid, bulletRect,
                                                   kamikazePendingKill, candidates, KamikazeEnemy::SCORE_VALUE, removed,
                                                   [](GameEvent& ev) {
                                                       ev.particleCount = 16;
                                                       ev.shakeDuration = 0.14f;
                                                       ev.shakeIntensity = 5.0f;
                                                   });
        }

        // Weaver/Bomber (Phase 2, Nguoi 1): cung khuon Kamikaze o tren - 1 mau, grid rieng,
        // khong lien quan gi toi luoi doi hinh, khong customizeEvent gi rieng (dung mac
        // dinh cua MakeEnemyKilledEvent nhu Basic/Zigzag/Medic).
        if (!consumed) {
            consumed = ResolveOneHitKillCollision(gm, bullet, i, gm.weaverEnemies, gm.weaverGrid, bulletRect,
                                                   weaverPendingKill, candidates, WeaverEnemy::SCORE_VALUE, removed,
                                                   [](GameEvent&) {});
        }
        if (!consumed) {
            consumed = ResolveOneHitKillCollision(gm, bullet, i, gm.bomberEnemies, gm.bomberGrid, bulletRect,
                                                   bomberPendingKill, candidates, BomberEnemy::SCORE_VALUE, removed,
                                                   [](GameEvent&) {});
        }

        // Boss: nhieu mau nhat trong game - tru hp ngay, chi bao "chet" khi hp<=0
        // (UpdatePlaying() se phat hien va xu ly WAVE_CLEAR - xem duoi). Cung KHUON MAU
        // voi khoi Kamikaze o tren: query grid rieng -> for candidates -> tru hp/danh
        // dau -> day event -> tieu thu pierce - khong con nhanh dieu kien rieng nao.
        if (!consumed && gm.bossPool.Size() > 0) {
            gm.bossGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                (void)idx; // Boss luon la index 0 duy nhat, chi dung candidates de biet co trung o nao khong
                Boss& boss = gm.bossPool[0];
                if (!CheckCollisionRecs(bulletRect, boss.rect)) continue;

                // KHIEN SENTINEL: bat kha xam pham HOAN TOAN trong luc active (khong tru
                // hp, khong bi pierce xuyen qua - dan bi khien "hap thu" het) - buoc
                // nguoi choi phai cho khien tat thay vi chi dung DPS de vuot qua.
                bool shielded = (boss.type == BossType::Sentinel && boss.shieldActive);
                if (!shielded && boss.hp > 0) boss.hp--;

                // BUG FIX: truoc day co 1 "placeholder" GameEvent thua o day, vo tinh
                // gan particleCount=1 + position=player.GetCenter() -> no lam 1 hat do
                // xuat hien ngay tai vi tri phi thuyen NGUOI CHOI moi lan dan trung Boss,
                // khong lien quan gi vi tri va cham that. hitEv ben duoi moi la hieu ung
                // that, dat dung vi tri bulletRect.
                GameEvent hitEv;
                hitEv.position = { bulletRect.x, bulletRect.y };
                hitEv.color = shielded ? SKYBLUE : RED;
                hitEv.particleCount = shielded ? 3 : 6; // Khien: bat lai it hat hon - cam giac "va be mat cung" thay vi "trung don"
                if (!shielded && boss.hp > 0) {
                    hitEv.sfx = SfxType::Hit;
                    hitEv.shakeDuration = 0.08f;
                    hitEv.shakeIntensity = 3.0f;
                    hitEv.flashOnHit = true; // Cong don voi burst mau (particleCount o tren) - flash trang rieng
                }
                gm.pendingEvents.push_back(hitEv);

                removed = shielded ? true : !bullet.ConsumePierce();
                if (removed) gm.playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        if (!removed) i++;
    }

    // Quet & swap-and-pop 1 luot duy nhat sau khi da xu ly xong toan bo dan cua frame
    // nay - luc nay viec index bi hoan doi khong con anh huong gi toi vong lap tren nua.
    for (size_t i = 0; i < gm.basicEnemies.Size(); ) {
        if (basicPendingKill[i]) {
            basicPendingKill[i] = basicPendingKill[gm.basicEnemies.Size() - 1];
            gm.basicEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < gm.tankyEnemies.Size(); ) {
        if (tankyPendingKill[i]) {
            tankyPendingKill[i] = tankyPendingKill[gm.tankyEnemies.Size() - 1];
            gm.tankyEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < gm.zigzagEnemies.Size(); ) {
        if (zigzagPendingKill[i]) {
            zigzagPendingKill[i] = zigzagPendingKill[gm.zigzagEnemies.Size() - 1];
            gm.zigzagEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < gm.kamikazeEnemies.Size(); ) {
        if (kamikazePendingKill[i]) {
            kamikazePendingKill[i] = kamikazePendingKill[gm.kamikazeEnemies.Size() - 1];
            gm.kamikazeEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < gm.wardenEnemies.Size(); ) {
        if (wardenPendingKill[i]) {
            wardenPendingKill[i] = wardenPendingKill[gm.wardenEnemies.Size() - 1];
            gm.wardenEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < gm.medicEnemies.Size(); ) {
        if (medicPendingKill[i]) {
            medicPendingKill[i] = medicPendingKill[gm.medicEnemies.Size() - 1];
            gm.medicEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < gm.weaverEnemies.Size(); ) { // Phase 2, Nguoi 1
        if (weaverPendingKill[i]) {
            weaverPendingKill[i] = weaverPendingKill[gm.weaverEnemies.Size() - 1];
            gm.weaverEnemies.Destroy(i);
        } else {
            i++;
        }
    }
    for (size_t i = 0; i < gm.bomberEnemies.Size(); ) { // Phase 2, Nguoi 1
        if (bomberPendingKill[i]) {
            bomberPendingKill[i] = bomberPendingKill[gm.bomberEnemies.Size() - 1];
            gm.bomberEnemies.Destroy(i);
        } else {
            i++;
        }
    }

    // Dan enemy: kiem tra bunker truoc, roi moi toi player
    for (size_t i = 0; i < gm.enemyBullets.GetActiveCount(); ) {
        Rectangle bulletRect = gm.enemyBullets.GetBullet(i).GetSweptRect();
        bool consumed = false;

        for (auto& bunker : gm.bunkers) {
            if (bunker.HandleBulletHit(bulletRect)) {
                gm.enemyBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        if (!consumed && CheckCollisionRecs(bulletRect, gm.player.GetRect())) {
            gm.enemyBullets.Destroy(i);
            consumed = true;
            if (gm.player.TakeDamage()) { // false neu dang bat tu/co khien -> khong hieu ung thua
                GameEvent ev;
                ev.position = gm.player.GetCenter();
                ev.color = RED;
                ev.particleCount = 18;
                ev.sfx = SfxType::Hit;
                ev.shakeDuration = 0.22f;
                ev.shakeIntensity = 8.0f;
                gm.pendingEvents.push_back(ev);
            }
        }

        if (!consumed) i++;
    }

    // Mystery Ship (UFO): kiem tra rieng vi khong nam trong grid/pool nao - chi 1 the
    // hien tai 1 thoi diem nen quet truc tiep khong anh huong hieu nang.
    if (gm.ufoActive) {
        for (size_t i = 0; i < gm.playerBullets.GetActiveCount(); i++) {
            if (CheckCollisionRecs(gm.playerBullets.GetBullet(i).GetSweptRect(), gm.ufoRect)) {
                bool removed = !gm.playerBullets.GetBullet(i).ConsumePierce();
                if (removed) gm.playerBullets.Destroy(i);
                gm.ufoActive = false;

                GameEvent ev;
                ev.position = EnemyCenter(gm.ufoRect);
                ev.color = RED;
                ev.particleCount = 20;
                ev.sfx = SfxType::UfoHit;
                ev.shakeDuration = 0.18f;
                ev.shakeIntensity = 6.0f;
                ev.scoreValue = gm.ufoScoreValue;
                gm.pendingEvents.push_back(ev);

                gm.RollNextUfoTimer(); // Lich hen UFO ke tiep - state thuan tuy, khong phai hieu ung nen giu lai o day
                break;
            }
        }
    }

    // Power-up: player bay ngang qua la nhat, khong can bam nut rieng.
    for (size_t i = 0; i < gm.powerUps.Size(); ) {
        if (CheckCollisionRecs(gm.powerUps[i].rect, gm.player.GetRect())) {
            GameEvent ev;
            ev.sfx = SfxType::Pickup;
            switch (gm.powerUps[i].type) {
                case PowerUpType::RapidFire:
                    gm.player.GrantRapidFire(Config::POWERUP_RAPIDFIRE_DURATION);
                    break;
                case PowerUpType::Shield:
                    gm.player.GrantShield(Config::POWERUP_SHIELD_DURATION);
                    break;
                case PowerUpType::Piercing:
                    gm.player.GrantPiercing(Config::POWERUP_PIERCE_DURATION);
                    break;
                case PowerUpType::SpreadShot: // Phase 1b, Nguoi 1
                    gm.player.GrantSpreadShot(Config::POWERUP_SPREADSHOT_DURATION);
                    break;
                case PowerUpType::Overdrive: // Phase 1b, Nguoi 1
                    gm.player.GrantOverdrive(Config::POWERUP_OVERDRIVE_DURATION);
                    break;
                case PowerUpType::Cleanser:
                    // Hieu ung TUC THI: xoa sach toan bo dan dich dang bay tren man
                    // hinh ngay luc nhat - "bom cuu nan" giua tinh huong nguy cap.
                    gm.enemyBullets.Reset();
                    ev.position = gm.player.GetCenter();
                    ev.color = LIME;
                    ev.particleCount = 30;
                    ev.sfx = SfxType::Cleanser;
                    ev.shakeDuration = 0.25f;
                    ev.shakeIntensity = 6.0f;
                    break;
            }
            gm.pendingEvents.push_back(ev);
            gm.powerUps.Destroy(i);
        } else {
            i++;
        }
    }
}
