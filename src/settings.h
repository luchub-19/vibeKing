#pragma once
#include <string>
#include "config.h"
#include "raylib.h"
#include "graphics_settings.h"
#include "localization.h"

// ==========================================
// SETTINGS - lưu độ khó & âm lượng ra file KEY=VALUE (giống level.cfg) để không bị
// reset về mặc định mỗi lần mở game lại. Trước đây chỉ HighScore được persist - đây
// là khoảng trống đã nêu trong review, giờ dùng chung 1 pattern RAII load/save.
//
// PHIM DIEU KHIEN CO THE DOI (rebind) - xem GameManager::UpdateKeybindScreen(). Chi 4
// hanh dong quan trong nhat LUC CHOI (di chuyen/ban/pause) cho doi; cac phim con lai
// (Enter/R/F3/F11/mui ten) giu CO DINH lam "phim he thong" khong cho rebind vao - tranh
// xung dot hoac nguoi choi lo tay tu khoa minh khoi menu (vd rebind Ban vao ESC).
// Luu duoi dang MA PHIM raylib (int) truc tiep - da xac nhan IsKeyPressed()/IsKeyDown()
// tu ban than raylib co bounds-check truoc khi doc mang noi bo (rcore.c: `(key > 0) &&
// (key < MAX_KEYBOARD_KEYS)`) nen 1 gia tri int bat ky (ke ca file settings.cfg bi sua
// tay/hong) khong the gay truy cap ngoai vung nho - toi da chi khong khop phim nao.
// ==========================================
// Toc do game cho phep (Cai dat > Tro nang) - roi rac nhu SHAKE_PERCENT_LEVELS, cung ly do: nguoi
// can no muon "cham hon ro rang", khong phai keo thanh truot tung 1%. Duoi 100 -> van choi gan
// nhan ASSIST tren bang xep hang (xem GameManager::runAssisted) - kieu Celeste Assist Mode.
constexpr int GAME_SPEED_LEVELS[] = { 100, 90, 80, 70 };
constexpr int GAME_SPEED_LEVEL_COUNT = 4;

struct Settings {
    Difficulty difficulty = Difficulty::NORMAL;
    // AM LUONG 3 tang: `volume` la TONG (giu ten key VOLUME cu cho file settings.cfg da co),
    // music/sfx la ty le NHAN THEM 0..1 cua rieng tung nhom - xem AudioSystem::SetMix().
    float volume = 0.6f;
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;
    bool uiSounds = true; // Tieng bip dieu huong menu + go chu (retro_ui.h)

    // Ngon ngu - mac dinh LUC NAP la ngon ngu he dieu hanh (xem LoadFromFile), khong phai EN cung.
    Language language = Language::EN;

    bool fullscreen = false; // Dong bo 2 chieu voi F11 - xem GameManager::SetFullscreen()
    bool showFps = false;
    bool autoFire = false;   // Tro nang/dieu khien: ban lien tuc khong can giu phim (InputSystem::Poll)
    int gameSpeedPercent = 100; // Luon la 1 phan tu GAME_SPEED_LEVELS sau LoadFromFile()
    bool showHitbox = false;

    float GameSpeedScale() const { return (float)gameSpeedPercent / 100.0f; }
    bool IsAssisted() const { return gameSpeedPercent < 100; }
    void CycleGameSpeed(int dir) {
        int idx = 0;
        for (int i = 0; i < GAME_SPEED_LEVEL_COUNT; i++) {
            if (GAME_SPEED_LEVELS[i] == gameSpeedPercent) idx = i;
        }
        idx = (idx + dir % GAME_SPEED_LEVEL_COUNT + GAME_SPEED_LEVEL_COUNT) % GAME_SPEED_LEVEL_COUNT;
        gameSpeedPercent = GAME_SPEED_LEVELS[idx];
    }

    int keyMoveLeft  = KEY_A;
    int keyMoveRight = KEY_D;
    int keyShoot     = KEY_SPACE;
    int keyPause     = KEY_P;

    GraphicsSettings graphics; // Trang GRAPHICS (phim G o menu) - xem graphics_settings.h

    // Reset ca 4 phim ve mac dinh - dung khi nguoi choi bam "Khoi phuc mac dinh" o man
    // hinh rebind, tranh phai nho lai tung gia tri mac dinh o nhieu noi khac nhau.
    void ResetKeyBindingsToDefault() {
        keyMoveLeft = KEY_A;
        keyMoveRight = KEY_D;
        keyShoot = KEY_SPACE;
        keyPause = KEY_P;
    }

    // Đọc file tại `path`. Thiếu file hoặc key nào đó lỗi -> giữ giá trị mặc định cho
    // đúng field đó, không crash, không throw (cùng triết lý với LevelGridConfig).
    // `defaultLanguage`: ngon ngu khi file CHUA co key LANGUAGE (lan dau mo game, hoac
    // settings.cfg tu ban truoc khi co da ngon ngu) - GameManager truyen ngon ngu he dieu hanh.
    // Tach thanh tham so (thay vi tu goi getenv) de test khong phu thuoc may dang chay.
    static Settings LoadFromFile(const std::string& path, Language defaultLanguage = Language::EN);

    // Ghi đè toàn bộ file - gọi mỗi khi người chơi đổi độ khó/âm lượng trong menu.
    // Không cảnh báo lỗi ghi ra HUD (khác HighScore) vì đây không phải dữ liệu quan
    // trọng - mất file settings chỉ đồng nghĩa lần mở sau lại dùng mặc định.
    void SaveToFile(const std::string& path) const;
};
