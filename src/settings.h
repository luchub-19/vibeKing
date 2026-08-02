#pragma once
#include <string>
#include "config.h"
#include "raylib.h"

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
struct Settings {
    Difficulty difficulty = Difficulty::NORMAL;
    float volume = 0.6f;

    int keyMoveLeft  = KEY_A;
    int keyMoveRight = KEY_D;
    int keyShoot     = KEY_SPACE;
    int keyPause     = KEY_P;

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
    static Settings LoadFromFile(const std::string& path);

    // Ghi đè toàn bộ file - gọi mỗi khi người chơi đổi độ khó/âm lượng trong menu.
    // Không cảnh báo lỗi ghi ra HUD (khác HighScore) vì đây không phải dữ liệu quan
    // trọng - mất file settings chỉ đồng nghĩa lần mở sau lại dùng mặc định.
    void SaveToFile(const std::string& path) const;
};
