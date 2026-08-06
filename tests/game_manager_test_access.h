#pragma once
#include "game_manager.h"
#include "physics_system.h"

// ==========================================
// GAMEMANAGERTESTACCESS - 1-NGUON-DUY-NHAT cho MOI truy cap private vao GameManager tu
// tests/test_game_manager.cpp va tests/test_physics_system.cpp. Dung chung 1 file thay vi
// moi test .cpp tu dinh nghia rieng mot ban - tranh 2 noi liet ke khac nhau ve "test duoc
// phep dung toi nhung gi" roi lech nhau ve sau (dung chinh triet ly RebindableAction da
// giai thich dau game_manager.h).
//
// TAI SAO CAN FILE NAY (xem giai thich day du hon trong game_manager.h, ngay truoc dinh
// nghia `class GameManagerTestAccess;`): GameManager chi cho PhysicsSystem/RenderSystem
// friend, ep phai co 1 seam rieng cho test. Dung LAI dung khuon "friend" da co san thay vi
// bay ra hang chuc getter/setter chi ton tai de phuc vu rieng 2 file test nay.
//
// RANH GIOI: cac ham o day CHI duoc goi tu tests/test_game_manager.cpp va
// tests/test_physics_system.cpp - khong bao gio duoc include tu bat ky file trong src/.
// Moi ham chi lam DUNG 1 viec (doc 1 field, hoac goi thang 1 ham private) - khong tu them
// logic/tinh toan/gia dinh nao ca, de test doc code that (khong phai code duoc "dan cho de
// test qua").
//
// CACH DUNG DIEN HINH (xem tests/test_game_manager.cpp / tests/test_physics_system.cpp de
// biet vi du day du):
//   GameManager gm;
//   GameManagerTestAccess::MetaProgressRef(gm).Load("mot_file_tam.dat"); // tranh ghi de save that
//   GameManagerTestAccess::BasicEnemies(gm).Spawn(BasicEnemy{ rect, WHITE });
//   PhysicsSystem::CheckCollisions(gm); // ham PUBLIC that su, khong can qua lop nay
// ==========================================
class GameManagerTestAccess {
public:
    // ----- State machine / fade transition -----
    static GameState State(const GameManager& gm) { return gm.state; }
    // Ghi truc tiep `state`, KHONG qua RequestTransition/UpdateTransition - dung de dung san
    // 1 tien dieu kien (vd "dang PLAYING voi 2 dich con song") cho cac test KHONG phai ve ban
    // than co che fade (co che do co bai test rieng ngay duoi day). Khong dong den
    // pendingState/transitionPhase - giu nguyen mac dinh NONE la du, vi UpdatePlaying() duoc
    // goi THANG o day (bo qua vong lap Run() ngoai cung, noi moi kiem tra `frozen`).
    static void SetState(GameManager& gm, GameState s) { gm.state = s; }
    static GameState PendingState(const GameManager& gm) { return gm.pendingState; }
    static TransitionPhase Phase(const GameManager& gm) { return gm.transitionPhase; }

    static int Wave(const GameManager& gm) { return gm.wave; }
    static void SetWave(GameManager& gm, int w) { gm.wave = w; }
    static bool IsBossWave(const GameManager& gm) { return gm.isBossWave; }
    static void SetIsBossWave(GameManager& gm, bool v) { gm.isBossWave = v; }

    // ----- DDA (Dynamic Difficulty Adjustment) - xem ddaSpeedMul/ddaLivesLostSinceCheck
    // trong game_manager.h. Them sau khi phat hien checkpoint DDA (chay ben trong nhanh
    // BOSS DEFEAT cua UpdatePlaying()) chua co test nao dung toi - chi Config::DDA_* (nap
    // tu JSON) duoc test o test_balance_config.cpp, ban than logic +STEP_UP/-STEP_DOWN/
    // vung chet/clamp thi khong. -----
    static float DdaSpeedMul(const GameManager& gm) { return gm.ddaSpeedMul; }
    static void SetDdaSpeedMul(GameManager& gm, float v) { gm.ddaSpeedMul = v; }
    static int DdaLivesLostSinceCheck(const GameManager& gm) { return gm.ddaLivesLostSinceCheck; }
    static void SetDdaLastKnownLives(GameManager& gm, int v) { gm.ddaLastKnownLives = v; }

    static void CallInitLevel(GameManager& gm, bool newGame) { gm.InitLevel(newGame); }
    static void CallRequestTransition(GameManager& gm, GameState next) { gm.RequestTransition(next); }
    // Goi UpdateTransition() du nhieu lan de vuot qua ca 2 pha FADE_OUT/FADE_IN (moi pha dai
    // Config::TRANSITION_DURATION giay) - dung khi test can thay state THAT SU doi (khong
    // chi dung o pendingState) sau 1 RequestTransition().
    static void CallUpdateTransition(GameManager& gm, float dt) { gm.UpdateTransition(dt); }
    static void CallUpdatePlaying(GameManager& gm, float dt) { gm.UpdatePlaying(dt); }
    // Chay rieng buoc xu ly hieu ung (particle/audio/score/power-up) ma CheckCollisions()
    // vua ghi nhan vao pendingEvents - dung khi test can kiem tra HE QUA THAT SU (vd
    // power-up thuc su xuat hien trong powerUps) ma khong can chay lai toan bo UpdatePlaying().
    static void CallProcessEvents(GameManager& gm) { gm.ProcessEvents(); }

    // ----- Du lieu the gioi (giong het tinh than friend PhysicsSystem/RenderSystem) -----
    static Player& PlayerRef(GameManager& gm) { return gm.player; }
    static MetaProgress& MetaProgressRef(GameManager& gm) { return gm.metaProgress; }
    static Leaderboard& LeaderboardRef(GameManager& gm) { return gm.leaderboard; }

    static BulletPool<Config::MAX_PLAYER_BULLETS>& PlayerBullets(GameManager& gm) { return gm.playerBullets; }
    static BulletPool<Config::MAX_ENEMY_BULLETS>& EnemyBullets(GameManager& gm) { return gm.enemyBullets; }

    static EnemyPool<BasicEnemy, Config::MAX_BASIC_ENEMIES>& BasicEnemies(GameManager& gm) { return gm.basicEnemies; }
    static EnemyPool<TankyEnemy, Config::MAX_TANKY_ENEMIES>& TankyEnemies(GameManager& gm) { return gm.tankyEnemies; }
    static EnemyPool<ZigzagEnemy, Config::MAX_ZIGZAG_ENEMIES>& ZigzagEnemies(GameManager& gm) { return gm.zigzagEnemies; }
    static EnemyPool<KamikazeEnemy, Config::MAX_KAMIKAZE>& KamikazeEnemies(GameManager& gm) { return gm.kamikazeEnemies; }
    static EnemyPool<Boss, 1>& BossPool(GameManager& gm) { return gm.bossPool; }

    static PowerUpPool<Config::MAX_POWERUPS>& PowerUps(GameManager& gm) { return gm.powerUps; }
    static std::vector<GameEvent>& PendingEvents(GameManager& gm) { return gm.pendingEvents; }
};
