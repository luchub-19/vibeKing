#pragma once
#include <string>
#include "config.h"

// ==========================================
// SETTINGS - lưu độ khó & âm lượng ra file KEY=VALUE (giống level.cfg) để không bị
// reset về mặc định mỗi lần mở game lại. Trước đây chỉ HighScore được persist - đây
// là khoảng trống đã nêu trong review, giờ dùng chung 1 pattern RAII load/save.
// ==========================================
struct Settings {
    Difficulty difficulty = Difficulty::NORMAL;
    float volume = 0.6f;

    // Đọc file tại `path`. Thiếu file hoặc key nào đó lỗi -> giữ giá trị mặc định cho
    // đúng field đó, không crash, không throw (cùng triết lý với LevelGridConfig).
    static Settings LoadFromFile(const std::string& path);

    // Ghi đè toàn bộ file - gọi mỗi khi người chơi đổi độ khó/âm lượng trong menu.
    // Không cảnh báo lỗi ghi ra HUD (khác HighScore) vì đây không phải dữ liệu quan
    // trọng - mất file settings chỉ đồng nghĩa lần mở sau lại dùng mặc định.
    void SaveToFile(const std::string& path) const;
};
