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

    // WARDEN/MEDIC (Phase 1a - Enemy & Item Revolution, Nguoi 1): tran AN TOAN bo nho
    // tinh, DONG THOI cung la ngan sach spawn toi da/wave cua WaveGenerator::Generate()
    // (xem wardenBudget/medicBudget trong wave_generator.cpp - dung KHUON voi cach
    // MAX_TANKY_ENEMIES vua la capacity vua la budget khoi tao moi wave). Co tinh de NHO
    // hon nhieu MAX_TANKY_ENEMIES (44) - day la 2 loai "dac biet/hiem", khong phai quan
    // so luong nhu Basic/Tanky. KHONG anh huong MAX_BASIC_ENEMIES ben tren: Basic van
    // luon la nhanh du phong CUOI CUNG voi ngan sach rieng (basicBudget), tu no da dam
    // bao khong bao gio vuot MAX_BASIC_ENEMIES du them bao nhieu loai canh tranh moi truoc
    // no trong chuoi roll (xem wave_generator.cpp).
    constexpr size_t MAX_WARDEN_ENEMIES = 10;
    constexpr size_t MAX_MEDIC_ENEMIES  = 10;

    // WEAVER/BOMBER (Phase 2 - Enemy & Item Revolution, Nguoi 1): KHAC MAX_WARDEN_ENEMIES/
    // MAX_MEDIC_ENEMIES o tren - day KHONG phai budget cua WaveGenerator (2 loai nay
    // khong di qua WaveGenerator, xem enemy_types.h), chi don thuan la tran an toan cho
    // EnemyPool + SpatialGrid, cung khuon MAX_KAMIKAZE o tren.
    constexpr size_t MAX_WEAVER_ENEMIES = 6;
    constexpr size_t MAX_BOMBER_ENEMIES = 5;

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
    inline const char* AtlasImagePath() { return "assets/sprites/atlas.png"; }
    inline const char* AtlasConfigPath() { return "assets/sprites/atlas.cfg"; }

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

    // ==========================================
    // SPREAD SHOT & OVERDRIVE (Phase 1b - Enemy & Item Revolution, Nguoi 1) - xem
    // Player::Update()/TakeDamage() (player.cpp). SPREAD_SHOT_ANGLE_DEG la goc TUYET DOI
    // (do) cua 2 tia ben so voi tia giua (thang len) - dat trong khoi POWER-UP chung o
    // tren (khong tach rieng) vi cung ho gia tri tam thoi nhat duoc qua nhat/roi tu dich,
    // dung 1 quy uoc voi RAPIDFIRE/PIERCE o tren.
    // ==========================================
    inline float POWERUP_SPREADSHOT_DURATION = 6.0f;
    inline float SPREAD_SHOT_ANGLE_DEG = 15.0f; // Goc moi tia ben lech khoi phuong thang dung
    inline float POWERUP_OVERDRIVE_DURATION = 6.0f;
    inline float POWERUP_OVERDRIVE_FIRE_RATE_MUL = 0.5f; // Nhu RAPIDFIRE_FIRE_RATE_MUL: nhan vao PLAYER_FIRE_RATE, nho hon = ban nhanh hon

    // TRONG SO ROI POWER-UP (Phase 1b, Nguoi 1) - GameManager::MaybeDropPowerUp() dung
    // random co trong so (khong con GetRandomValue(0,3) deu tuyet doi nhu 4 loai goc) de
    // co the "chia lai trong so" qua balance.json ma khong doi code - xem TASK_SPLIT.md.
    // SpreadShot/Overdrive mac dinh THAP hon 4 loai goc (Overdrive thap nhat vi la lua
    // chon rui-ro-cao-loi-ich-cao, khong nen roi qua thuong xuyen).
    inline float POWERUP_WEIGHT_RAPIDFIRE  = 1.0f;
    inline float POWERUP_WEIGHT_SHIELD     = 1.0f;
    inline float POWERUP_WEIGHT_PIERCING   = 1.0f;
    inline float POWERUP_WEIGHT_CLEANSER   = 1.0f;
    inline float POWERUP_WEIGHT_SPREADSHOT = 0.75f;
    inline float POWERUP_WEIGHT_OVERDRIVE  = 0.5f;

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

    // ==========================================
    // WEAVER & BOMBER (Phase 2 - Enemy & Item Revolution, Nguoi 1) - xem enemy_types.h.
    // Spawn/toc do dung khuon UFO (KAMIKAZE_SPEED/KAMIKAZE_WIDTH o tren la khuon Kamikaze,
    // KHONG phai khuon dung cho 2 loai nay - xem comment tren struct WeaverEnemy).
    // ==========================================
    inline float WEAVER_SPAWN_MIN_INTERVAL = 10.0f;
    inline float WEAVER_SPAWN_MAX_INTERVAL = 18.0f;
    inline float WEAVER_SPEED_X = 140.0f;         // Toc do ngang xuyen man hinh (khong doi trong suot doi song)
    inline float WEAVER_WEAVE_AMPLITUDE = 60.0f;  // Bien do dao dong doc quanh baseY
    inline float WEAVER_WEAVE_FREQUENCY = 2.2f;   // Toc do dao dong hinh sin (rad/s)
    inline float WEAVER_WIDTH  = 32.0f;
    inline float WEAVER_HEIGHT = 22.0f;
    inline float WEAVER_BASE_Y_MIN = 60.0f;  // Tam Y dao dong duoc quay ngau nhien trong [MIN,MAX] moi lan spawn - tranh moi Weaver luon bay dung 1 do cao
    inline float WEAVER_BASE_Y_MAX = 160.0f;

    inline float BOMBER_SPAWN_MIN_INTERVAL = 11.0f;
    inline float BOMBER_SPAWN_MAX_INTERVAL = 20.0f;
    inline float BOMBER_SPEED_X = 100.0f;    // Cham hon Weaver - moi de doa cua Bomber la bom roi, khong phai toc do
    inline float BOMBER_BOMB_INTERVAL = 1.6f; // Nhip tha bom (giay/qua) - dem nguoc rieng tung con, KHONG dung chung 1 timer toan cuc
    inline float BOMBER_WIDTH  = 34.0f;
    inline float BOMBER_HEIGHT = 24.0f;
    inline float BOMBER_Y = 70.0f; // Do cao bay CO DINH (khac Weaver co baseY random moi lan) - de phan biet 2 loai tu xa bang mat thuong

    // WARDEN/MEDIC (Phase 1a - Enemy & Item Revolution, Nguoi 1): 2 loai dich LUOI moi -
    // van tham gia doi hinh/di chuyen dung khuon Basic/Tanky (KHONG dao dong rieng nhu
    // Zigzag), nhung KHONG tham gia he thong chon "tien tuyen ban" dung chung trong
    // UpdateEnemies() (giu tinh than Kamikaze/Boss/UFO: tu bat/khong bat rieng, khong chen
    // vao co che chi vi Basic/Tanky/Zigzag) - xem PhysicsSystem::UpdateWardenEnemies()/
    // UpdateMedicEnemies() trong physics_system.cpp. Xac suat spawn (theo % moi wave, dung
    // khuon tankyChance trong WaveGenerator::Generate) o day thay vi hardcode truc tiep
    // trong wave_generator.cpp nhu Tanky - DoD Phase 1a yeu cau "khong co so ma thuat
    // ngoai Config/balance.json" ro rang hon muc Tanky (da co tu truoc) dat ra.
    // (HP/SCORE_VALUE cua rieng Warden/Medic nam TREN struct - xem WardenEnemy::HP/
    // SCORE_VALUE, MedicEnemy::SCORE_VALUE trong enemy_types.h - dung khuon TankyEnemy::HP/
    // KamikazeEnemy::SCORE_VALUE. O day chi con hang so "he thong" dung boi code NGOAI
    // struct: WaveGenerator can xac suat spawn, GameManager::ProcessEvents() can so luong
    // sinh quan, PhysicsSystem::UpdateMedicEnemies() can chu ky hoi mau.)
    inline int   WARDEN_REINFORCEMENT_COUNT = 2; // So BasicEnemy sinh ra tai vi tri cu luc Warden chet - xem GameManager::ProcessEvents()
    inline float WARDEN_SPAWN_CHANCE_BASE = 0.06f;
    inline float WARDEN_SPAWN_CHANCE_MAX = 0.14f;
    inline float WARDEN_SPAWN_CHANCE_WAVE_STEP = 0.008f;
    inline float MEDIC_HEAL_INTERVAL = 3.0f; // Giay giua 2 lan Medic hoi mau cho Tanky gan nhat con song
    inline int   MEDIC_HEAL_AMOUNT = 1;
    inline float MEDIC_SPAWN_CHANCE_BASE = 0.05f;
    inline float MEDIC_SPAWN_CHANCE_MAX = 0.12f;
    inline float MEDIC_SPAWN_CHANCE_WAVE_STEP = 0.007f;

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

    // ==========================================
    // IDLE ANIMATION (Phase 1 - Graphics/UI Overhaul, Nguoi 1): transform-THUAN luc VE
    // cho tung dich trong RenderSystem::DrawPlaying() - CHI doi Rectangle dung de goi
    // DrawSprite(), KHONG dung vao rect that (hitbox) cua entity, y het ky thuat
    // sin(GetTime()) ma DrawTitleLogo() da dung (render_system.cpp). PHASE_STEP nhan voi
    // truong `column` co san cua BasicEnemy/TankyEnemy/ZigzagEnemy (xem enemy_types.h) de
    // ca doi hinh "gon song" thay vi nhap nhay dong loat; Kamikaze/UFO khong co `column`
    // nen dung vi tri hien tai (rect.x) lam "hat giong" pha rieng thay the. Boss co bo
    // hang so RIENG (BOSS_IDLE_*) vi than lon hon han cac dich khac - cung bien do px se
    // AN NHIEU hon, va chi co 1 con tai 1 thoi diem nen khong can PHASE_STEP.
    // ==========================================
    inline float ANIM_IDLE_BOB_AMPLITUDE   = 2.5f;  // px - dich thuong/UFO/kamikaze
    inline float ANIM_IDLE_BOB_FREQUENCY   = 2.2f;  // rad/s
    inline float ANIM_IDLE_SCALE_AMPLITUDE = 0.05f; // ti le phong to/nho quanh tam (+/-5%)
    inline float ANIM_IDLE_PHASE_STEP      = 0.4f;  // rad moi don vi `column` / moi 100px vi tri

    inline float ANIM_BOSS_IDLE_BOB_AMPLITUDE   = 4.0f;  // px
    inline float ANIM_BOSS_IDLE_BOB_FREQUENCY   = 1.2f;  // rad/s - cham/nang hon dich thuong
    inline float ANIM_BOSS_IDLE_SCALE_AMPLITUDE = 0.03f; // than lon hon nhieu - can % nho hon de khoi "phinh to" qua ro

    // BUNKER LINH HOẠT
    inline float BUNKER_REGEN_INTERVAL = 4.0f;
    inline int   BUNKER_REGEN_PER_TICK = 3;
    inline float BUNKER_PATROL_AMPLITUDE = 14.0f;
    inline float BUNKER_PATROL_SPEED = 0.8f;

    // ==========================================
    // DYNAMIC DIFFICULTY ADJUSTMENT (DDA) - KHOI RIENG, KHONG dung chung voi bang do kho
    // g_difficultyTable o cuoi file (nguoi choi TU CHON EASY/NORMAL/HARD trong Menu - DDA
    // la 1 TANG DIEU CHINH THEM o TREN lua chon do, khong thay the). GameManager doc so
    // mang mat tu lan hieu chinh truoc moi khi ha Boss (checkpoint moi Config::
    // BOSS_WAVE_INTERVAL wave - xem UpdatePlaying() nhanh BOSS DEFEAT trong game_manager.cpp)
    // roi tinh lai ddaSpeedMul; PhysicsSystem::UpdateEnemies() nhan he so nay vao ban sao
    // cuc bo DifficultyStats (khong dung vao g_difficultyTable goc - xem physics_system.cpp).
    // ==========================================
    inline float DDA_STEP_UP   = 0.05f; // Khong mat mang nao ca chu ky Boss -> +5%
    inline float DDA_STEP_DOWN = 0.10f; // Vat lon (dat nguong) -> -10%, giam nhanh hon tang de "cuu" nguoi choi kip thoi
    inline float DDA_MIN_MUL   = 0.7f;  // San duoi - khong bao gio de dich cham/ban thua hon 70% muc do kho da chon
    inline float DDA_MAX_MUL   = 1.3f;  // Tran tren - khong bao gio vuot 130% du nguoi choi gioi den dau
    inline int   DDA_STRUGGLE_THRESHOLD = 2; // Mat >= 2 mang trong 1 chu ky Boss moi tinh la "vat lon"

    // ==========================================
    // GRAPHICS/UI OVERHAUL - NGUOI 3 (Audio & UI): hang so cho muzzle flash, hit-flash,
    // bullet glow, nhac nen procedural (AudioStream realtime) va HUD panel/icon moi.
    // constexpr (KHONG phai inline) - day la tinh chinh hinh anh/am thanh ky thuat, khong
    // phai du lieu can bang gameplay ma designer can hot-tune qua balance.json (Assign()
    // trong config.cpp CHUA co entry cho nhom nay - giu constexpr de tranh phai dong
    // thoi sua ca config.cpp, ngoai pham vi 3 file so huu cua Nguoi 3).
    // ==========================================

    // MUZZLE FLASH - 1 particles.Burst() nho tai dau nong moi lan ban (UpdatePlaying(),
    // canh audio.PlayShoot()).
    constexpr int MUZZLE_FLASH_PARTICLE_COUNT = 3;

    // HIT-FLASH - cum particle MAU TRANG rieng (particles.Burst mau WHITE) tai vi tri va
    // cham khi GameEvent::flashOnHit=true (xem ProcessEvents() + physics_system.cpp) -
    // cong don, KHONG thay the burst mau thuong (particleCount/color) - flash bao "chi
    // trung", burst mau bao "loai gi/khien hay khong".
    constexpr int HITFLASH_PARTICLE_COUNT = 4;

    // BULLET GLOW - Bullet::Draw() ve them 1 vach mo (DrawLineEx, dung LAI ky thuat
    // ParticleShape::Spark ben tren) nguoc huong bay TRUOC khi ve loi dan dac, hoan toan
    // tu chua (Bullet da co san `vel`).
    constexpr float BULLET_GLOW_TRAIL_LENGTH = 14.0f;  // px, do dai vach mo phia sau
    constexpr float BULLET_GLOW_THICKNESS_MUL = 1.8f;  // Nhan voi be rong dan -> do day vach
    constexpr float BULLET_GLOW_ALPHA = 0.45f;         // 0..1, do trong vach mo (loi dan van 100% dac)

    // NHAC NEN PROCEDURAL - AudioStream sinh PCM REALTIME moi frame (khac han Sound tinh
    // cua SFX/bassline hien co - xem AudioSystem::UpdateMusic()/FillMusicBuffer() trong
    // audio_system.cpp), phan ung theo wave/Boss/mang con lai/combo thay vi 1 loop co dinh.
    constexpr int MUSIC_STREAM_BUFFER_FRAMES = 2048; // Kich thuoc 1 chunk PCM - ky thuat thuan tuy
    constexpr float MUSIC_ARPEGGIO_INTERVAL_MIN = 0.28f; // Giay/not luc dich nhanh nhat (nhanh gap ~2x bassline)
    constexpr float MUSIC_ARPEGGIO_INTERVAL_MAX = 0.70f; // Giay/not luc dich cham nhat
    constexpr int   MUSIC_WAVE_SECTION_LEN = 3;          // Cu moi 3 wave, giai dieu transpose len 1 bac
    constexpr float MUSIC_TRANSPOSE_SEMITONES_PER_SECTION = 2.0f;
    constexpr int   MUSIC_MAX_TRANSPOSE_SECTIONS = 4;    // Tran tren - qua muc nay giu nguyen, tranh choi tai qua cao
    constexpr int   MUSIC_LOW_LIVES_THRESHOLD = 1;       // Mang <= nguong nay -> bat "tension" (tremolo + detune nhe)
    constexpr float MUSIC_TENSION_TREMOLO_HZ = 6.0f;
    constexpr float MUSIC_BOSS_INTERVAL_MUL = 0.6f;      // Boss active -> arpeggio nhanh hon (nhan them vao interval)
    constexpr float MUSIC_MASTER_GAIN = 0.5f;            // 0..1 - nhac nen nho hon SFX/bassline, khong lan at
    constexpr float MUSIC_PAD_VOLUME_MUL = 0.6f;         // So voi MASTER_GAIN - lop pad/hoa am nen tho hon lead
    constexpr float MUSIC_LEAD_VOLUME_MUL = 1.0f;

    // HUD PANEL/ICON - RenderSystem::DrawHUD() dung UICanvas::Panel()/Icon() moi
    // (ui_system.h) thay text-tren-nen-den truoc day.
    constexpr float HUD_PANEL_ALPHA = 0.55f;         // 0..1 - do phu nen panel (van thay duoc gameplay phia sau)
    constexpr float HUD_PANEL_BORDER_THICKNESS = 2.0f;
    constexpr float HUD_ICON_SIZE = 18.0f;           // px - kich thuoc badge icon trong HUD

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

// ==========================================
// LIGHTING & WORLD (post_process.h/.cpp + parallax.h/.cpp) - hang so TRINH BAY/HINH ANH,
// KHONG phai du lieu can bang gameplay (HP/toc do/wave pattern/...) nen giu constexpr,
// khong dua vao balance.json/LoadBalance() - dung tien le TRANSITION_DURATION o tren
// (constexpr, "UI/engine, khong phai can bang"). Muon tune thi sua truc tiep roi build
// lai, khong phai runtime qua JSON.
//
// BLOOM_ENABLED/CRT_ENABLED: bat/tat TUNG hieu ung DOC LAP, chi can doi true/false roi
// build lai - khong dung Settings/menu rieng cho viec nay.
// ==========================================
namespace Config {
    // --- Bloom (trich sang + blur 2 chieu + cong don/additive vao anh goc) ---
    constexpr bool  BLOOM_ENABLED     = true;
    constexpr float BLOOM_THRESHOLD   = 0.6f;  // Nguong do sang (luma, 0..1) de tinh la vung "bloom". CHU Y: mau XANH LA thuan (chu dao game nay - alien/title) co luma ~0.715 - nguong phai THAP HON gia tri nay thi alien/title moi co bloom (verify bang screenshot thuc te: nguong 0.75 ban dau lam alien/title KHONG bloom gi ca, ha xuong 0.6 moi thay ro)
    constexpr float BLOOM_INTENSITY   = 1.0f;  // He so nhan mau khi trich xuat vung sang, TRUOC khi blur (shader bloom_extract.fs) - co the >1 de "chay sang" manh hon
    constexpr int   BLOOM_DOWNSAMPLE  = 2;     // Chia do phan giai renderTarget cho so nay khi lam texture trung gian (2 = nua do phan giai) - blur re hon, upscale lai cung lam blur "mem" hon tu nhien
    constexpr float BLOOM_BLUR_SPREAD = 1.5f;  // He so nhan them vao buoc lay mau cua Gauss 1 chieu (blur.fs) - lon hon = quang sang loang rong hon

    // --- CRT (scanline + vignette + nhap nhay nhe) ---
    constexpr bool  CRT_ENABLED           = true;
    constexpr float CRT_SCANLINE_STRENGTH = 0.15f;  // Do toi cua hang quet toi xen ke (0 = tat, 1 = den hoan toan)
    constexpr float CRT_VIGNETTE_STRENGTH = 0.6f;   // Do toi dan ra vien man hinh
    constexpr float CRT_FLICKER_STRENGTH  = 0.015f; // Bien do nhap nhay do sang theo thoi gian - rat nho, chi de "song dong", khong gay kho chiu/loa mat

    // --- Parallax starfield (ve truoc MOI trang thai Menu/Playing/EndScreen...) ---
    constexpr int   PARALLAX_STAR_COUNT  = 90; // Tong so sao ca 3 lop cong lai - kich thuoc std::array trong Parallax (xem parallax.h)
    constexpr int   PARALLAX_LAYER_COUNT = 3;  // Lop xa/giua/gan - xem Parallax::Init() (parallax.cpp)
    constexpr float PARALLAX_SPEED_FAR   = 12.0f; // px/giay, lop xa nhat (nho + mo + cham nhat)
    constexpr float PARALLAX_SPEED_NEAR  = 45.0f; // px/giay, lop gan nhat (to + sang + nhanh nhat)

    // --- Duong dan shader (tuong doi so voi thu muc lam viec luc chay executable - cung
    // quy uoc voi FontFilePath()/BalanceFilePath() o tren) ---
    inline const char* BloomExtractShaderPath() { return "assets/shaders/bloom_extract.fs"; }
    inline const char* BlurShaderPath()         { return "assets/shaders/blur.fs"; }
    inline const char* CrtShaderPath()          { return "assets/shaders/crt.fs"; }
}
