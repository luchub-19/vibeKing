#pragma once
#include <cstddef>

// ==========================================
// CẤU HÌNH TOÀN CỤC
//
// 2 NHÓM RÕ RỆT trong file này, đừng gộp lẫn:
//   - `constexpr` : hằng số KỸ THUẬT/ĐỘNG CƠ (screen size, dung lượng pool tĩnh, biên
//     an toàn bộ nhớ, đường dẫn file...) - PHẢI biết tại compile-time (dùng làm template
//     non-type param / kích thước std::array) hoặc không phải là thứ NGƯỜI THIẾT KẾ
//     GAME muốn chỉnh (vd MAX_BASIC_ENEMIES là giới hạn AN TOÀN bộ nhớ tĩnh, không phải
//     "độ khó").
//   - `inline` (KHÔNG const) : DỮ LIỆU CÂN BẰNG (balance data) - HP, tốc độ, wave
//     pattern, hành vi Boss, độ khó, power-up, combo, bunker... Giá trị khai báo ở đây
//     CHỈ LÀ MẶC ĐỊNH (dùng khi file balance.json không tồn tại/thiếu field) -
//     Config::LoadBalance() (config.cpp) GHI ĐÈ chúng lúc runtime từ
//     assets/balance.json. Designer sửa CÂN BẰNG game bằng cách sửa file JSON đó, KHÔNG
//     CẦN đụng vào code C++ hay chạy lại quy trình compile.
// ==========================================
namespace Config {
    // ---------- KỸ THUẬT / ĐỘNG CƠ (compile-time, không phải "cân bằng") ----------
    constexpr int SCREEN_W = 800;
    constexpr int SCREEN_H = 600;
    constexpr float MAX_DT = 1.0f / 30.0f; // Chặn dt spike khi lag/kéo cửa sổ

    constexpr float BULLET_WIDTH  = 5.0f;
    constexpr float BULLET_HEIGHT = 15.0f;
    constexpr float BULLET_OFFSCREEN_MARGIN = 20.0f; // Biên ngoài màn hình mà đạn bị coi là "đã bay khỏi"

    constexpr float PLAYER_SPAWN_X = 375.0f;
    constexpr float PLAYER_SPAWN_Y = 550.0f;
    constexpr float PLAYER_WIDTH   = 50.0f;
    constexpr float PLAYER_HEIGHT  = 20.0f;

    constexpr float GAMEPAD_DEADZONE = 0.3f; // Biên chết trục analog - đặc tính phần cứng, không phải cân bằng game

    constexpr float BUNKER_Y = 460.0f;
    constexpr float BUNKER_MARGIN_X = 80.0f;

    // Kích thước pool cố định — PHẢI constexpr (dùng làm template non-type param cho
    // EnemyPool<T,N>/BulletPool<N>/std::array bên dưới) - đây là GIỚI HẠN BỘ NHỚ TĨNH
    // AN TOÀN của engine, không phải 1 con số cân bằng designer muốn "tune" (designer
    // chỉnh SỐ HÀNG/CỘT ĐANG DÙNG trong balance.json, luôn nằm TRONG giới hạn này).
    constexpr size_t MAX_PLAYER_BULLETS = 100;
    constexpr size_t MAX_ENEMY_BULLETS  = 500;
    constexpr size_t MAX_PARTICLES      = 400;
    constexpr int MAX_GRID_ROWS = 12;
    constexpr int MAX_GRID_COLS = 20;
    constexpr size_t MAX_ZIGZAG_ENEMIES = (size_t)MAX_GRID_COLS;
    constexpr size_t MAX_TANKY_ENEMIES  = (size_t)(MAX_GRID_ROWS - 1) * ((MAX_GRID_COLS + 4) / 5);
    constexpr size_t MAX_BASIC_ENEMIES  = (size_t)(MAX_GRID_ROWS * MAX_GRID_COLS) - MAX_ZIGZAG_ENEMIES - MAX_TANKY_ENEMIES;
    constexpr size_t MAX_POWERUPS = 8;
    constexpr size_t MAX_KAMIKAZE = 6;

    constexpr float TRANSITION_DURATION = 0.25f; // Thời gian fade giữa các state - UI/engine, không phải cân bằng
    constexpr int FONT_BASE_SIZE = 48;
    constexpr int LEADERBOARD_MAX_ENTRIES = 10;

    // META-PROGRESSION: quy doi diem so cuoi 1 van thanh currency xuyen-van (xem
    // MetaProgress::AwardCurrency trong meta_progress.cpp) - vd rate=50 tuc 1000 diem =
    // 20 currency. constexpr (khong phai inline/JSON-tunable nhu nhom "DU LIEU CAN BANG"
    // ben duoi) vi day la mau so chia - de constexpr tranh hoan toan kha nang 1 gia tri 0
    // lot vao tu balance.json roi gay chia-cho-0 luc runtime.
    constexpr int META_SCORE_TO_CURRENCY_RATE = 50;

    inline const char* LeaderboardFilePath() { return "leaderboard.dat"; }
    inline const char* LevelConfigFilePath() { return "level.cfg"; }
    inline const char* SettingsFilePath() { return "settings.cfg"; }
    inline const char* FontFilePath() { return "assets/fonts/DejaVuSansMono.ttf"; }
    inline const char* BalanceFilePath() { return "assets/balance.json"; }

    // ==========================================
    // DỮ LIỆU CÂN BẰNG (DATA-DRIVEN) - inline (KHÔNG const/constexpr): giá trị dưới đây
    // chỉ là MẶC ĐỊNH, bị Config::LoadBalance() ghi đè lúc runtime từ balance.json.
    // Không đổi CÚ PHÁP truy cập (`Config::PLAYER_SPEED`...) ở bất kỳ nơi nào khác trong
    // codebase - chỉ đổi CHỖ giá trị đến từ đâu (compile-time literal -> runtime file).
    // ==========================================
    inline float PLAYER_SPEED     = 400.0f;
    inline float PLAYER_FIRE_RATE = 0.2f;
    inline float INVINCIBLE_TIME  = 1.2f;
    inline float PLAYER_SHIELD_HIT_GRACE = 0.3f;

    // EXTRA LIFE - cu moi EXTRA_LIFE_SCORE_THRESHOLD diem (5000, 10000, 15000...)
    // duoc +1 mang, toi da MAX_LIVES (chuan the loai ban sung co dien - vd Galaga/1942)
    // de tranh mang cong don vo han lam trivial hoa do kho. Nguong van tang deu ngay ca
    // khi da dat tran (xem Player::AddScore) - tranh loi "mang bi tich luy ngam" neu
    // khong lam vay: mat mang roi lai duoc hoan lai ngay du khong ghi them diem nao moi.
    inline int   EXTRA_LIFE_SCORE_THRESHOLD = 5000;
    inline int   MAX_LIVES = 5;

    inline float ENEMY_SPEED_INC  = 15.0f;
    inline float BULLET_SPEED     = 600.0f;
    inline float ENEMY_BULLET_SPEED = 300.0f;
    inline float ENEMY_AIMED_SHOT_CHANCE = 0.35f;
    inline int   RADIAL_BURST_COUNT = 12;

    inline float PARTICLE_GRAVITY = 260.0f;

    // WAVE PROGRESSION - "wave pattern" chỉnh độ khó tăng dần theo từng đợt.
    inline int   WAVE_EXTRA_ROW_EVERY = 3;
    inline float WAVE_SPEED_BONUS_PER = 7.5f;
    inline float WAVE_FIRE_RATE_STEP  = 0.08f;
    inline float WAVE_FIRE_RATE_MIN_MUL = 0.35f;

    // POWER-UP
    inline float POWERUP_DROP_CHANCE = 0.12f;
    inline float POWERUP_FALL_SPEED = 90.0f;
    inline float POWERUP_SIZE = 20.0f;
    inline float POWERUP_RAPIDFIRE_DURATION = 6.0f;
    inline float POWERUP_RAPIDFIRE_FIRE_RATE_MUL = 0.4f;
    inline float POWERUP_SHIELD_DURATION = 8.0f;
    inline float POWERUP_PIERCE_DURATION = 6.0f;
    inline int   POWERUP_PIERCE_HITS = 3;

    // COMBO SCORE
    inline float COMBO_WINDOW = 1.5f;
    inline float COMBO_BONUS_PER_STEP = 0.1f;
    inline int   COMBO_MAX_STEPS = 9;

    // MYSTERY SHIP (UFO)
    inline float UFO_SPAWN_MIN_INTERVAL = 12.0f;
    inline float UFO_SPAWN_MAX_INTERVAL = 22.0f;
    inline float UFO_SPEED  = 160.0f;
    inline float UFO_WIDTH  = 44.0f;
    inline float UFO_HEIGHT = 22.0f;
    inline float UFO_Y      = 20.0f;
    inline int   UFO_SCORE_MIN = 50;
    inline int   UFO_SCORE_MAX = 300;

    // KAMIKAZE
    inline float KAMIKAZE_SPAWN_MIN_INTERVAL = 6.0f;
    inline float KAMIKAZE_SPAWN_MAX_INTERVAL = 12.0f;
    inline float KAMIKAZE_SPEED = 220.0f;
    inline float KAMIKAZE_WIDTH  = 28.0f;
    inline float KAMIKAZE_HEIGHT = 28.0f;

    // BOSS BEHAVIOR - toàn bộ hành vi Boss (tốc độ/nhịp bắn theo giai đoạn, HP, điểm)
    // đều data-driven như yêu cầu.
    inline int   BOSS_WAVE_INTERVAL = 5;
    inline float BOSS_WIDTH  = 180.0f;
    inline float BOSS_HEIGHT = 90.0f;
    inline float BOSS_Y = 60.0f;
    inline int   BOSS_MAX_HP = 40;
    inline float BOSS_HP_PER_WAVE_BONUS = 6.0f;
    inline float BOSS_SPEED_STAGE1 = 60.0f;
    inline float BOSS_SPEED_STAGE2 = 100.0f;
    inline float BOSS_SPEED_STAGE3 = 160.0f;
    inline float BOSS_FIRE_INTERVAL_STAGE1 = 1.8f;
    inline float BOSS_FIRE_INTERVAL_STAGE2 = 1.1f;
    inline float BOSS_FIRE_INTERVAL_STAGE3 = 0.7f;
    inline float BOSS_RADIAL_CHANCE_STAGE2 = 0.4f;
    inline float BOSS_RADIAL_CHANCE_STAGE3 = 0.8f;
    inline float BOSS_BULLET_SPEED = 260.0f;
    inline int   BOSS_SCORE_VALUE = 1000;

    // BOSS - SENTINEL (loai 2/3, xoay vong - xem enemy_types.h): gan nhu dung yen, chi
    // lac nhe quanh diem spawn (KHONG dung BOSS_SPEED_STAGE*), dinh ky bat khien tam bat
    // kha xam pham buoc nguoi choi cho dung nhip thay vi giu nut ban lien tuc.
    inline float BOSS_SENTINEL_SWAY_AMPLITUDE = 90.0f;   // px
    inline float BOSS_SENTINEL_SWAY_FREQUENCY = 0.6f;    // rad/s
    inline float BOSS_SENTINEL_SHIELD_INTERVAL = 6.0f;   // Giay KHONG co khien truoc lan bat tiep theo
    inline float BOSS_SENTINEL_SHIELD_DURATION = 2.5f;   // Khien ton tai bao lau moi lan bat
    inline float BOSS_SENTINEL_SHIELD_FIRE_INTERVAL = 0.35f; // Nhip ban RIENG (nhanh hon) trong luc co khien

    // BOSS - SWARMER (loai 3/3, xoay vong): lac NHANH+RONG hon Sentinel han nhieu (cam
    // giac that thuong, kho ngam), dinh ky trieu hoi tiep vien tu pool Kamikaze co san.
    inline float BOSS_SWARMER_SWAY_AMPLITUDE = 220.0f;   // px
    inline float BOSS_SWARMER_SWAY_FREQUENCY = 1.8f;     // rad/s
    inline float BOSS_SWARMER_SUMMON_INTERVAL = 5.0f;    // Giay giua 2 lan trieu hoi
    inline int   BOSS_SWARMER_SUMMON_COUNT = 2;          // So Kamikaze trieu hoi moi lan

    // BUNKER LINH HOẠT
    inline float BUNKER_REGEN_INTERVAL = 4.0f;
    inline int   BUNKER_REGEN_PER_TICK = 3;
    inline float BUNKER_PATROL_AMPLITUDE = 14.0f;
    inline float BUNKER_PATROL_SPEED = 0.8f;

    // Nạp assets/balance.json (hoặc đường dẫn tùy chọn) đè lên MỌI giá trị `inline` ở
    // trên - field nào KHÔNG có trong JSON thì GIỮ NGUYÊN giá trị mặc định phía trên
    // (không phải lỗi, không crash) - cùng triết lý "không bao giờ chết vì thiếu file/
    // field tùy chọn" đã dùng cho settings.cfg/level.cfg trong dự án. Gọi ĐÚNG 1 LẦN,
    // CÀNG SỚM CÀNG TỐT trong GameManager::Run() - trước khi bất kỳ InitLevel()/Spawn*()
    // nào đọc các giá trị này.
    void LoadBalance(const char* path = nullptr);
}

// ==========================================
// ĐỘ KHÓ (DATA-DRIVEN) - bảng RUNTIME (không còn switch/case literal cứng) - designer
// chỉnh trực tiếp trong balance.json (mục "difficulty"), Config::LoadBalance() ghi đè
// từng ô của bảng này. Nhãn hiển thị (label) vẫn là chuỗi hằng biên dịch sẵn - không
// phải số liệu "cân bằng", đổi tên khó (EASY/NORMAL/HARD) không phải việc designer làm
// qua file cấu hình.
// ==========================================
enum class Difficulty { EASY, NORMAL, HARD };

struct DifficultyStats {
    float enemyBaseSpeed;
    float enemySpeedMax;
    float enemyFireRate;
    const char* label;
};

namespace Config {
    inline DifficultyStats g_difficultyTable[3] = {
        { 40.0f, 180.0f, 1.4f, "EASY"   },
        { 50.0f, 260.0f, 1.0f, "NORMAL" },
        { 65.0f, 340.0f, 0.6f, "HARD"   },
    };
}

inline DifficultyStats GetDifficultyStats(Difficulty d) {
    int idx = (int)d;
    if (idx < 0 || idx > 2) idx = 1;
    return Config::g_difficultyTable[idx];
}

inline Difficulty CycleDifficulty(Difficulty d, int dir) {
    int v = (int)d + dir;
    if (v < 0) v = 2;
    if (v > 2) v = 0;
    return (Difficulty)v;
}
