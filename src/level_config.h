#pragma once
#include <string>

// ==========================================
// LƯỚI ĐỘI HÌNH ĐỊCH (DATA-DRIVEN)
// Trước đây số hàng/cột bị hardcode thẳng trong vòng lặp InitLevel() (r < 4, c < 10).
// Muốn đổi độ hình phải sửa code + build lại. Giờ đọc từ 1 file cấu hình text đơn giản
// (level.cfg, định dạng KEY=VALUE) - designer chỉnh độ hình mà không đụng vào source.
// ==========================================
struct LevelGridConfig {
    int rows = 4;
    int cols = 10;
    float startX = 65.0f;
    float startY = 50.0f;
    float spacingX = 60.0f;
    float spacingY = 40.0f;

    // Đọc file KEY=VALUE tại `path`. Nếu file không tồn tại hoặc dòng nào đó parse lỗi,
    // giữ nguyên giá trị mặc định cho field đó (không crash, không throw) - game luôn
    // khởi động được kể cả khi chưa có file cấu hình.
    static LevelGridConfig LoadFromFile(const std::string& path);

private:
    void Clamp(); // Chặn giá trị vô lý (0, âm, quá lớn) từ file cấu hình bị chỉnh tay sai
};
