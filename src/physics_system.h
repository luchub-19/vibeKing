#pragma once
#include "raylib.h"
#include "bullet_pool.h"
#include "spatial_grid.h"
#include <array>
#include <vector>
#include <cstddef>

class GameManager; // Forward declare - PhysicsSystem thao tac truc tiep tren du lieu
                    // the gioi cua GameManager (pools, grids, bunkers, player...) qua
                    // 1 tham chieu duy nhat, khong tu so huu ban sao nao ca.

// ==========================================
// PHYSICS SYSTEM
// Gom toan bo logic "moi frame moi thu di chuyen o dau va cham vao nhau nhu the nao":
// di chuyen doi hinh (Basic/Tanky/Zigzag), Kamikaze, UFO, Boss, va phat hien+giai quyet
// va cham (dan-vs-bunker, dan-vs-dich, dan-vs-player, player-vs-powerup). Day la phan
// "chi tiet" lon nhat truoc day nam trong GameManager - tach rieng giup GameManager::
// UpdatePlaying() chi con la 1 chuoi goi tuan tu (dieu phoi), khong con phai biet toa do
// hay cong thuc va cham nao ca.
//
// Ket qua cua va cham (diem, particle, am thanh, rung man hinh, roi power-up) KHONG
// duoc goi truc tiep o day - moi hieu ung duoc dong goi thanh GameEvent va day vao
// GameManager::pendingEvents, xu ly rieng o GameManager::ProcessEvents() (xem events.h).
// PhysicsSystem vi vay chi con phu thuoc vao hinh hoc/toc do/hp - khong dinh gi toi
// audio/particle - de doc va de test hon nhieu.
//
// BOSS: dung chung EnemyPool<Boss,1> nhu moi loai dich khac (xem game_manager.h) - khoi
// CheckCollisions() khong con nhanh rieng "if (bossActive)" nao nua, chi la 1 pool nua
// duoc xu ly dung KHUON MAU giong het Kamikaze (pool 1-nhieu phan tu, query grid rieng,
// tru hp/danh dau kill, day event) - Size()==0 tu dong bo qua, y het cach moi pool trong
// (Basic/Tanky/Zigzag) rong cung tu dong khong lam gi, khong can code rieng de kiem tra.
// ==========================================
class PhysicsSystem {
public:
    static void UpdateEnemies(GameManager& gm, float dt);   // Doi hinh Basic/Tanky/Zigzag + chon dich ban
    // Warden/Medic (Phase 1a, Nguoi 1): HAM RIENG (khong chung UpdateEnemies() o tren) -
    // van dung khuon doi hinh Basic/Tanky (ApplyFormationMoveX/DescendRowAndCheckGameOver)
    // nhung KHONG tham gia activeCount/frontline-shooting cua UpdateEnemies(). `alreadyFlipped`:
    // true neu UpdateEnemies() da tu doi huong doi hinh trong CHINH frame nay roi (xem
    // GameManager::UpdatePlaying()) - khi do ham nay CHI di chuyen + tut hang (neu can),
    // KHONG doi huong/tang toc THEM lan nua (tranh doi huong 2 lan = coi nhu khong doi,
    // tang toc gap doi ngoai y muon).
    static void UpdateWardenEnemies(GameManager& gm, float dt, bool alreadyFlipped);
    static void UpdateMedicEnemies(GameManager& gm, float dt, bool alreadyFlipped);
    static void UpdateKamikaze(GameManager& gm, float dt);
    static void UpdateUfo(GameManager& gm, float dt);
    static void UpdateBoss(GameManager& gm, float dt);

    static void CheckCollisions(GameManager& gm);

private:
    static void EnemyShoot(GameManager& gm, float x, float y);
    static void FireRadialBurst(GameManager& gm, float x, float y, int count, float speed);

    // Helper noi bo dung de rut gon trung lap trong UpdateEnemies()/CheckCollisions() -
    // xem chi tiet/ly do o dinh nghia dau physics_system.cpp. La private static member
    // (khong phai free function) DE THUA HUONG friend access ma GameManager da cap cho
    // ca class PhysicsSystem - free function thuong se KHONG doc/ghi duoc field private
    // cua GameManager (da tu kiem chung: build loi bien private ngay lan dau thu voi free
    // function).
    static bool ApplyFormationMoveX(Rectangle& r, float direction, float speed, float dt, float formationOffset = 0.0f);
    static bool DescendRowAndCheckGameOver(GameManager& gm, Rectangle& r);
    template <typename PoolT, size_t N, typename CustomizeEventFn>
    static bool ResolveOneHitKillCollision(GameManager& gm, Bullet& bullet, size_t bulletIndex,
                                            PoolT& pool, SpatialGrid& grid, const Rectangle& bulletRect,
                                            std::array<bool, N>& pendingKill, std::vector<int>& candidates,
                                            int scoreValue, bool& removed, CustomizeEventFn customizeEvent);
};
