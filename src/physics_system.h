#pragma once

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
    static void UpdateKamikaze(GameManager& gm, float dt);
    static void UpdateUfo(GameManager& gm, float dt);
    static void UpdateBoss(GameManager& gm, float dt);

    static void CheckCollisions(GameManager& gm);

private:
    static void EnemyShoot(GameManager& gm, float x, float y);
    static void FireRadialBurst(GameManager& gm, float x, float y, int count, float speed);
};
