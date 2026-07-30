#pragma once
#include <cstddef>

// ==========================================
// CẤU HÌNH TOÀN CỤC
// Gom mọi hằng số vào 1 chỗ — muốn tune số liệu chỉ sửa ở đây,
// không phải mò khắp codebase.
// ==========================================
namespace Config {
    constexpr int SCREEN_W = 800;
    constexpr int SCREEN_H = 600;
    constexpr float MAX_DT = 1.0f / 30.0f; // Chặn dt spike khi lag/kéo cửa sổ

    constexpr float PLAYER_SPEED     = 400.0f;
    constexpr float PLAYER_FIRE_RATE = 0.2f;
    constexpr float INVINCIBLE_TIME  = 1.2f;

    constexpr float ENEMY_SPEED_INC  = 15.0f;
    constexpr float BULLET_SPEED     = 600.0f;
    // Truoc day la -300.0f (am) vi Bullet cu chi co 1 scalar `speed` va tu quy uoc rieng
    // (rect.y -= speed*dt) coi am la "xuong duoi". Bullet gio dung Vector2 vel chuan man
    // hinh (Y+ la xuong duoi that su qua rect.y += vel.y*dt) nen day la 1 BIEN DO DUONG
    // (magnitude), khong con dau am gay nham lan nua.
    constexpr float ENEMY_BULLET_SPEED = 300.0f;
    constexpr float BULLET_WIDTH  = 5.0f;
    constexpr float BULLET_HEIGHT = 15.0f;
    // Biên ngoài màn hình mà đạn bị coi là "đã bay khỏi" (dùng cả 2 phía trên/dưới) -
    // trước đây hardcode rời rạc -20.0f/620.0f ngay trong bullet_pool.h.
    constexpr float BULLET_OFFSCREEN_MARGIN = 20.0f;

    // Vị trí/kích thước xuất phát của player - trước đây hardcode lặp lại 375.0f/550.0f
    // ở CẢ 3 chỗ (Player::Reset, Player::ResetForNewWave, Player::TakeDamage).
    constexpr float PLAYER_SPAWN_X = 375.0f;
    constexpr float PLAYER_SPAWN_Y = 550.0f;
    constexpr float PLAYER_WIDTH   = 50.0f;
    constexpr float PLAYER_HEIGHT  = 20.0f;
    constexpr float PLAYER_SHIELD_HIT_GRACE = 0.3f; // Bất tử ngắn ngay sau khi khiên đỡ đòn, tránh mất 2 mạng cùng frame

    constexpr float GAMEPAD_DEADZONE = 0.3f; // Biên chết trục analog trước khi tính là "có bấm hướng"

    // Trọng lực hạt particle (nổ, vỡ) - trước đây hardcode 260.0f ngay trong particle_pool.h
    constexpr float PARTICLE_GRAVITY = 260.0f;

    // Vị trí bố trí 4 bunker - trước đây hardcode trong GameManager::SpawnBunkers()
    constexpr float BUNKER_Y = 460.0f;
    constexpr float BUNKER_MARGIN_X = 80.0f;

    // Kích thước pool cố định — dùng size_t vì đây là non-type template param
    constexpr size_t MAX_PLAYER_BULLETS = 100;
    constexpr size_t MAX_ENEMY_BULLETS  = 500;
    constexpr size_t MAX_PARTICLES      = 400;

    // ==========================================
    // GIỚI HẠN LƯỚI ĐỘI HÌNH ĐỊCH (dùng chung bởi LevelGridConfig::Clamp() và
    // kích thước tĩnh của 3 Enemy Pool bên dưới - PHẢI đồng bộ 1 nguồn duy nhất
    // ở đây, không hardcode lại số 12/20 ở nơi khác).
    // ==========================================
    constexpr int MAX_GRID_ROWS = 12;
    constexpr int MAX_GRID_COLS = 20;

    // Kích thước tĩnh của từng Enemy Pool - tính đúng theo công thức spawn trong
    // InitLevel() tại giới hạn lưới tối đa ở trên, cộng biên an toàn nhỏ:
    //   - Zigzag: chỉ hàng đầu tiên (r==0)                       -> tối đa MAX_GRID_COLS
    //   - Tanky : mỗi hàng còn lại, cứ 5 cột có 1 (c % 5 == 0)    -> tối đa (rows-1) * ceil(cols/5)
    //   - Basic : phần còn lại của lưới
    constexpr size_t MAX_ZIGZAG_ENEMIES = (size_t)MAX_GRID_COLS;
    constexpr size_t MAX_TANKY_ENEMIES  = (size_t)(MAX_GRID_ROWS - 1) * ((MAX_GRID_COLS + 4) / 5);
    constexpr size_t MAX_BASIC_ENEMIES  = (size_t)(MAX_GRID_ROWS * MAX_GRID_COLS) - MAX_ZIGZAG_ENEMIES - MAX_TANKY_ENEMIES;

    constexpr float TRANSITION_DURATION = 0.25f; // Thời gian fade giữa các state

    inline const char* LeaderboardFilePath() { return "leaderboard.dat"; }
    inline const char* LevelConfigFilePath() { return "level.cfg"; }
    inline const char* SettingsFilePath() { return "settings.cfg"; }
    inline const char* FontFilePath() { return "assets/fonts/DejaVuSansMono.ttf"; }

    // Render o base size lon (LoadFontEx rasterize toan bo glyph thanh 1 texture atlas
    // duy nhat NGAY LUC LOAD - "Texture Atlas chuyen biet" chinh la co che nay, khong
    // phai lam gi them) roi ve nho lai khi can - net hon nhieu so voi ve dung 1 kich
    // thuoc co dinh, vi luon co du "do phan giai du" cho moi cachs scale trong game.
    constexpr int FONT_BASE_SIZE = 48;

    constexpr int LEADERBOARD_MAX_ENTRIES = 10;

    // ==========================================
    // WAVE PROGRESSION - độ khó tăng dần theo từng đợt (wave) đã qua, thay vì
    // 1 màn chơi đi chơi lại y hệt. Xem GameManager::InitLevel().
    // ==========================================
    constexpr int   WAVE_EXTRA_ROW_EVERY = 3;    // Cứ mỗi N wave thì thêm 1 hàng địch
    constexpr float WAVE_SPEED_BONUS_PER = 7.5f; // Cộng thêm tốc độ nền mỗi wave (trước khi clamp theo max của độ khó)
    constexpr float WAVE_FIRE_RATE_STEP  = 0.08f; // Mỗi wave rút ngắn thêm khoảng cách bắn 1 chút (tối thiểu WAVE_FIRE_RATE_MIN_MUL)
    constexpr float WAVE_FIRE_RATE_MIN_MUL = 0.35f;

    // ==========================================
    // POWER-UP - rơi ra ngẫu nhiên khi hạ địch, người chơi bay ngang qua để nhặt.
    // ==========================================
    constexpr size_t MAX_POWERUPS = 8;
    constexpr float POWERUP_DROP_CHANCE = 0.12f;     // 12% mỗi lần hạ gục 1 địch
    constexpr float POWERUP_FALL_SPEED = 90.0f;
    constexpr float POWERUP_SIZE = 20.0f;
    constexpr float POWERUP_RAPIDFIRE_DURATION = 6.0f;
    constexpr float POWERUP_RAPIDFIRE_FIRE_RATE_MUL = 0.4f; // Bắn nhanh gấp 2.5 lần bình thường
    constexpr float POWERUP_SHIELD_DURATION = 8.0f;

    // ==========================================
    // COMBO SCORE - hạ gục liên tiếp trong khoảng thời gian ngắn được nhân điểm.
    // ==========================================
    constexpr float COMBO_WINDOW = 1.5f;       // Giây - hạ thêm 1 địch trong khoảng này thì combo tăng
    constexpr float COMBO_BONUS_PER_STEP = 0.1f; // +10% điểm mỗi bậc combo
    constexpr int   COMBO_MAX_STEPS = 9;        // Combo tối đa +90% điểm (bậc 10)

    // ==========================================
    // MYSTERY SHIP (UFO) - dia bay do bay ngang qua dinh man hinh theo chu ky, thuong
    // dem theo diem thuong ngau nhien - core mechanic kinh dien cua Space Invaders.
    // ==========================================
    constexpr float UFO_SPAWN_MIN_INTERVAL = 12.0f; // Giây - khoảng chờ tối thiểu giữa 2 lần xuất hiện
    constexpr float UFO_SPAWN_MAX_INTERVAL = 22.0f;
    constexpr float UFO_SPEED  = 160.0f;
    constexpr float UFO_WIDTH  = 44.0f;
    constexpr float UFO_HEIGHT = 22.0f;
    constexpr float UFO_Y      = 20.0f;
    constexpr int   UFO_SCORE_MIN = 50;
    constexpr int   UFO_SCORE_MAX = 300;

    // ==========================================
    // KAMIKAZE - tach rieng khoi doi hinh (khong spawn trong luoi, khong tham gia logic
    // kiem tra bien luoi doi hinh o UpdateEnemies) - spawn doc lap theo chu ky nhu UFO,
    // lao thang mot duong toi vi tri nguoi choi luc spawn.
    // ==========================================
    constexpr size_t MAX_KAMIKAZE = 6;
    constexpr float KAMIKAZE_SPAWN_MIN_INTERVAL = 6.0f;
    constexpr float KAMIKAZE_SPAWN_MAX_INTERVAL = 12.0f;
    constexpr float KAMIKAZE_SPEED = 220.0f;
    constexpr float KAMIKAZE_WIDTH  = 28.0f;
    constexpr float KAMIKAZE_HEIGHT = 28.0f;
    constexpr int   KAMIKAZE_SCORE_VALUE = 150;

    // ==========================================
    // PATTERN ĐẠN ĐỊCH - dan grunt thuong (Basic/Tanky/Zigzag) roll ngau nhien giua ban
    // thang xuong (kinh dien) va ban nham thang nguoi choi; Boss dung rieng kieu toa tron.
    // ==========================================
    constexpr float ENEMY_AIMED_SHOT_CHANCE = 0.35f; // 35% la ban nham, con lai ban thang
    constexpr int   RADIAL_BURST_COUNT = 12;         // So dan toa deu 360 do

    // ==========================================
    // BOSS - xuat hien thay the doi hinh thuong tai cac wave la boi so nay, bao phu
    // nhieu o SpatialGrid (rect lon hon nhieu 1 o cellSize=80). 3 giai doan theo % HP
    // con lai, cang yeu cang nhanh + ban day hon.
    // ==========================================
    constexpr int   BOSS_WAVE_INTERVAL = 5; // Wave 5, 10, 15... la boss wave
    constexpr float BOSS_WIDTH  = 180.0f;
    constexpr float BOSS_HEIGHT = 90.0f;
    constexpr float BOSS_Y = 60.0f;
    constexpr int   BOSS_MAX_HP = 40;
    constexpr float BOSS_HP_PER_WAVE_BONUS = 6.0f; // Boss wave sau nhieu mau hon boss wave truoc
    constexpr float BOSS_SPEED_STAGE1 = 60.0f;
    constexpr float BOSS_SPEED_STAGE2 = 100.0f;
    constexpr float BOSS_SPEED_STAGE3 = 160.0f;
    constexpr float BOSS_FIRE_INTERVAL_STAGE1 = 1.8f;
    constexpr float BOSS_FIRE_INTERVAL_STAGE2 = 1.1f;
    constexpr float BOSS_FIRE_INTERVAL_STAGE3 = 0.7f;
    constexpr float BOSS_RADIAL_CHANCE_STAGE2 = 0.4f; // Xac suat ban toa tron thay vi ban nham o stage 2
    constexpr float BOSS_RADIAL_CHANCE_STAGE3 = 0.8f;
    constexpr float BOSS_BULLET_SPEED = 260.0f;
    constexpr int   BOSS_SCORE_VALUE = 1000;

    // ==========================================
    // POWER-UP CAO CẤP - bổ sung Piercing Shot (đạn player xuyên qua nhiều mục tiêu) và
    // Cleanser (bom xóa sạch đạn địch trên màn hình, hiệu ứng tức thời khi nhặt).
    // ==========================================
    constexpr float POWERUP_PIERCE_DURATION = 6.0f;
    constexpr int   POWERUP_PIERCE_HITS = 3; // Xuyên thêm tối đa 3 mục tiêu nữa (tổng 4 lần trúng) trước khi biến mất

    // ==========================================
    // BUNKER LINH HOẠT - tự hồi phục 1 số voxel đã bị khoét theo thời gian (không bao
    // giờ lấp lại vòm cổng/góc bo tròn vốn là thiết kế có chủ đích), và dao động ngang
    // nhẹ quanh vị trí gốc thay vì đứng yên tuyệt đối.
    // ==========================================
    constexpr float BUNKER_REGEN_INTERVAL = 4.0f;  // Giây giữa mỗi đợt hồi phục
    constexpr int   BUNKER_REGEN_PER_TICK = 3;      // Số voxel tối đa hồi phục mỗi đợt
    constexpr float BUNKER_PATROL_AMPLITUDE = 14.0f; // px - biên độ dao động ngang
    constexpr float BUNKER_PATROL_SPEED = 0.8f;      // rad/s
}

// ==========================================
// ĐỘ KHÓ (DATA-DRIVEN)
// Thay vì if/else rải rác mỗi nơi cần biết "đang chơi khó gì",
// gom hết thông số vào 1 bảng tra cứu duy nhất.
// ==========================================
enum class Difficulty { EASY, NORMAL, HARD };

struct DifficultyStats {
    float enemyBaseSpeed;
    float enemySpeedMax;
    float enemyFireRate;
    const char* label;
};

inline DifficultyStats GetDifficultyStats(Difficulty d) {
    switch (d) {
        case Difficulty::EASY:   return { 40.0f, 180.0f, 1.4f, "EASY"   };
        case Difficulty::HARD:   return { 65.0f, 340.0f, 0.6f, "HARD"   };
        case Difficulty::NORMAL:
        default:                 return { 50.0f, 260.0f, 1.0f, "NORMAL" };
    }
}

inline Difficulty CycleDifficulty(Difficulty d, int dir) {
    int v = (int)d + dir;
    if (v < 0) v = 2;
    if (v > 2) v = 0;
    return (Difficulty)v;
}
