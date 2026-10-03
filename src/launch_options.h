#pragma once
#include <string>
#include <vector>

// ==========================================
// THAM SO DONG LENH - chi phuc vu KIEM CHUNG DO HOA (GD 0 cua docs/GRAPHICS_UPGRADE_PLAN.md),
// nguoi choi binh thuong chay `./space_invaders` khong tham so -> hanh vi y het truoc day.
//
// TAI SAO CAN: luat R5 cua ke hoach do hoa - moi thay doi khau ve phai co anh chup truoc/sau
// SO SANH DUOC. scripts/capture_screens.sh chi toi duoc 4 man dieu huong bang phim; canh dang
// danh nhau that (dan dich day man, no, power-up, boss) phu thuoc AI dich + RNG + thoi gian
// nen moi lan chup 1 khac -> khong the doi chieu "truoc vs sau" tren cung 1 khung hinh.
// --scene dung san 1 canh CO DINH (seed RNG co dinh, gameplay dong bang), --capture chup roi
// thoat, --bench do frame time lam moc hieu nang (luat R4).
//
//   --scene=combat|boss|gameover|waveclear  Vao thang canh trinh dien, gameplay dong bang (khong
//                         ai chet/an diem). gameover/waveclear: man tong ket voi so lieu co dinh,
//                         KHONG di qua TriggerGameOver() -> khong ghi leaderboard/currency.
//   --capture=<file.png>  Chup man hinh SAU post-process o frame --capture-frame roi thoat
//   --capture-frame=<n>   Mac dinh 45 (du cho fade/khoi tao on dinh)
//   --quality=low|medium|high  Ghi de preset do hoa CHI TRONG RAM (khong luu settings.cfg) -
//                         de chup/do tung preset ma khong dong vao cai dat that cua nguoi choi
//   --bench=<n>           Bo gioi han 60 FPS, do n frame (sau 30 frame khoi dong), in thong
//                         ke ra stdout roi thoat
//
// Ham THUAN (khong goi raylib) - test headless tai tests/test_launch_options.cpp.
// ==========================================

// Menu..Pause: canh GUI (nang cap GUI) - chup tung man menu/2 ngon ngu (--lang) ma khong can gui phim.
enum class ShowcaseScene { None, Combat, Boss, GameOver, WaveClear, Menu, Hangar, Settings, HowTo, Attract, Leaderboard, Achievements, Pause };

struct LaunchOptions {
    ShowcaseScene scene = ShowcaseScene::None;
    std::string captureFile;  // rong = khong chup
    int captureFrame = 45;
    int benchFrames = 0;      // 0 = khong do
    int qualityOverride = -1; // -1 = dung settings.cfg; 0/1/2 = GraphicsQuality Low/Medium/High
    int languageOverride = -1; // -1 = dung settings.cfg; 0 = EN, 1 = VI (--lang=en|vi)
    int settingsTab = 0;       // --tab=0..3: tab hien trong canh settings
    std::string error;        // khac rong = tham so sai, main() in usage va thoat ma 2

    bool Ok() const { return error.empty(); }
};

LaunchOptions ParseLaunchOptions(const std::vector<std::string>& args);

// Thong ke frame time cho --bench. Tach thanh ham thuan de test duoc cach tinh p95 (de
// sai off-by-one nhat) ma khong can mo cua so.
struct FrameStats {
    int count = 0;
    double avgMs = 0.0;
    double p95Ms = 0.0;
    double maxMs = 0.0;
};

FrameStats SummarizeFrameTimes(std::vector<double> samplesMs);

const char* LaunchUsage();
