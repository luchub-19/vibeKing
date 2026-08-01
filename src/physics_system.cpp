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

// ==========================================
// DOI HINH (Basic/Tanky/Zigzag) + CHON DICH BAN
// ==========================================
void PhysicsSystem::UpdateEnemies(GameManager& gm, float dt) {
    bool hitEdge = false;

    // Basic/Tanky: chi ap dung doi hinh di chuyen ngang, khong co hanh vi phu.
    for (size_t i = 0; i < gm.basicEnemies.Size(); i++) {
        Rectangle& r = gm.basicEnemies[i].rect;
        r.x += gm.enemyDirection * gm.enemySpeed * dt;
        if (r.x <= 0 || r.x + r.width >= Config::SCREEN_W) hitEdge = true;
    }
    for (size_t i = 0; i < gm.tankyEnemies.Size(); i++) {
        Rectangle& r = gm.tankyEnemies[i].rect;
        r.x += gm.enemyDirection * gm.enemySpeed * dt;
        if (r.x <= 0 || r.x + r.width >= Config::SCREEN_W) hitEdge = true;
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

        e.rect.x += gm.enemyDirection * gm.enemySpeed * dt;
        if (e.rect.x <= 0 || e.rect.x + e.rect.width >= Config::SCREEN_W) hitEdge = true;
    }
    // Kamikaze KHONG tham gia vong lap tren (pool + spawn logic hoan toan rieng, xem
    // UpdateKamikaze) - dung nhu thiet ke "khong pha hong logic kiem tra bien luoi doi hinh".

    size_t activeCount = gm.basicEnemies.Size() + gm.tankyEnemies.Size() + gm.zigzagEnemies.Size();
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

    if (hitEdge) {
        gm.enemyDirection *= -1;
        gm.enemySpeed = fminf(gm.enemySpeed + Config::ENEMY_SPEED_INC, stats.enemySpeedMax);

        for (size_t i = 0; i < gm.basicEnemies.Size(); i++) {
            Rectangle& r = gm.basicEnemies[i].rect;
            r.y += 20.0f;
            if (r.x < 0) r.x = 0;
            if (r.x + r.width > Config::SCREEN_W) r.x = Config::SCREEN_W - r.width;
            if (EnemyBottom(r) >= gm.player.GetY()) {
                gm.audio.PlayGameOver();
                gm.lastSubmitResult = gm.leaderboard.TrySubmit(gm.player.GetScore(), gm.wave);
                gm.RequestTransition(GameState::GAME_OVER);
                return;
            }
        }
        for (size_t i = 0; i < gm.tankyEnemies.Size(); i++) {
            Rectangle& r = gm.tankyEnemies[i].rect;
            r.y += 20.0f;
            if (r.x < 0) r.x = 0;
            if (r.x + r.width > Config::SCREEN_W) r.x = Config::SCREEN_W - r.width;
            if (EnemyBottom(r) >= gm.player.GetY()) {
                gm.audio.PlayGameOver();
                gm.lastSubmitResult = gm.leaderboard.TrySubmit(gm.player.GetScore(), gm.wave);
                gm.RequestTransition(GameState::GAME_OVER);
                return;
            }
        }
        for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++) {
            Rectangle& r = gm.zigzagEnemies[i].rect;
            r.y += 20.0f;
            if (r.x < 0) r.x = 0;
            if (r.x + r.width > Config::SCREEN_W) r.x = Config::SCREEN_W - r.width;
            if (EnemyBottom(r) >= gm.player.GetY()) {
                gm.audio.PlayGameOver();
                gm.lastSubmitResult = gm.leaderboard.TrySubmit(gm.player.GetScore(), gm.wave);
                gm.RequestTransition(GameState::GAME_OVER);
                return;
            }
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
// KAMIKAZE
// ==========================================
void PhysicsSystem::UpdateKamikaze(GameManager& gm, float dt) {
    gm.kamikazeSpawnTimer -= dt;
    if (gm.kamikazeSpawnTimer <= 0.0f && gm.kamikazeEnemies.Size() < Config::MAX_KAMIKAZE) {
        gm.SpawnKamikaze();
    }

    for (size_t i = 0; i < gm.kamikazeEnemies.Size(); ) {
        KamikazeEnemy& k = gm.kamikazeEnemies[i];
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
// BOSS - dung chung EnemyPool<Boss,1> nhu moi loai dich khac (xem game_manager.h);
// Size()==0 nghia la chua spawn/da bi ha, Size()==1 nghia la con song - khong con `bool
// bossActive` rieng phai giu dong bo thu cong voi hp.
// ==========================================
void PhysicsSystem::UpdateBoss(GameManager& gm, float dt) {
    if (gm.bossPool.Size() == 0) return;
    Boss& boss = gm.bossPool[0];

    int stage = BossStage(boss);
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
    for (size_t i = 0; i < gm.basicEnemies.Size(); i++)    gm.basicGrid.Insert((int)i, gm.basicEnemies[i].rect);
    for (size_t i = 0; i < gm.tankyEnemies.Size(); i++)    gm.tankyGrid.Insert((int)i, gm.tankyEnemies[i].rect);
    for (size_t i = 0; i < gm.zigzagEnemies.Size(); i++)   gm.zigzagGrid.Insert((int)i, gm.zigzagEnemies[i].rect);
    for (size_t i = 0; i < gm.kamikazeEnemies.Size(); i++) gm.kamikazeGrid.Insert((int)i, gm.kamikazeEnemies[i].rect);

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

        // Trung don ha guc ngay (Basic/Zigzag, luon 1 mau) - danh dau pendingKill, cong
        // diem/hieu ung ngay (khong phu thuoc index nen an toan de lam ngay lap tuc).
        if (!consumed) {
            gm.basicGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (basicPendingKill[idx]) continue; // Da bi bullet khac trong frame nay ha roi
                BasicEnemy& e = gm.basicEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                basicPendingKill[idx] = true;
                gm.pendingEvents.push_back(MakeEnemyKilledEvent(EnemyCenter(e.rect), e.color, BasicEnemy::SCORE_VALUE));

                // PIERCING SHOT: neu dan con xuyen duoc, KHONG Destroy() - no van con o
                // dung index `i`, tiep tuc bay va co the trung them muc tieu khac o
                // frame sau. removed=false -> vong lap ben duoi se KHONG tang i sai
                // (van dung, vi bullet nay khong bi swap).
                removed = !bullet.ConsumePierce();
                if (removed) gm.playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        if (!consumed) {
            gm.zigzagGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (zigzagPendingKill[idx]) continue;
                ZigzagEnemy& e = gm.zigzagEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                zigzagPendingKill[idx] = true;
                gm.pendingEvents.push_back(MakeEnemyKilledEvent(EnemyCenter(e.rect), e.color, ZigzagEnemy::SCORE_VALUE));

                removed = !bullet.ConsumePierce();
                if (removed) gm.playerBullets.Destroy(i);
                consumed = true;
                break;
            }
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
                    gm.pendingEvents.push_back(MakeEnemyKilledEvent(EnemyCenter(e.rect), e.color, TankyEnemy::SCORE_VALUE));
                } else {
                    // Dich mau day van con song sau don nay - phan hoi nhe hon de phan
                    // biet voi don ha guc han
                    GameEvent ev;
                    ev.sfx = SfxType::Hit;
                    ev.shakeDuration = 0.05f;
                    ev.shakeIntensity = 2.0f;
                    gm.pendingEvents.push_back(ev);
                }

                removed = !bullet.ConsumePierce();
                if (removed) gm.playerBullets.Destroy(i);
                consumed = true;
                break;
            }
        }

        // Kamikaze: 1 mau, ha guc ngay - grid rieng, khong lien quan gi toi luoi doi hinh.
        if (!consumed) {
            gm.kamikazeGrid.QueryIndices(bulletRect, candidates);
            for (int idx : candidates) {
                if (kamikazePendingKill[idx]) continue;
                KamikazeEnemy& e = gm.kamikazeEnemies[idx];
                if (!CheckCollisionRecs(bulletRect, e.rect)) continue;

                kamikazePendingKill[idx] = true;
                GameEvent ev = MakeEnemyKilledEvent(EnemyCenter(e.rect), e.color, KamikazeEnemy::SCORE_VALUE);
                ev.particleCount = 16;
                ev.shakeDuration = 0.14f;
                ev.shakeIntensity = 5.0f;
                gm.pendingEvents.push_back(ev);

                removed = !bullet.ConsumePierce();
                if (removed) gm.playerBullets.Destroy(i);
                consumed = true;
                break;
            }
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

                if (boss.hp > 0) boss.hp--;

                GameEvent placeholder; // Placeholder rong - hieu ung that o duoi
                placeholder.position = gm.player.GetCenter();
                placeholder.color = RED;
                placeholder.particleCount = 1;
                gm.pendingEvents.push_back(placeholder);

                GameEvent hitEv;
                hitEv.position = { bulletRect.x, bulletRect.y };
                hitEv.color = RED;
                hitEv.particleCount = 6;
                if (boss.hp > 0) {
                    hitEv.sfx = SfxType::Hit;
                    hitEv.shakeDuration = 0.08f;
                    hitEv.shakeIntensity = 3.0f;
                }
                gm.pendingEvents.push_back(hitEv);

                removed = !bullet.ConsumePierce();
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
