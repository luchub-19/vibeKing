#pragma once
#include <string>

// Đọc/ghi điểm cao ra file text đơn giản. Tách riêng để GameManager không phải
// biết gì về I/O - chỉ gọi Load()/TrySubmit(), đúng nguyên tắc single responsibility.
class HighScore {
private:
    int value = 0;
    std::string filePath;
    bool lastWriteFailed = false; // true nếu lần TrySubmit() gần nhất không ghi được xuống đĩa

public:
    void Load(const std::string& path);

    // true nếu đây là kỷ lục mới trong phiên chơi này (điểm hiển thị trong RAM vẫn cập
    // nhật ngay cả khi ghi file thất bại - chỉ có phần lưu lâu dài là bị ảnh hưởng).
    bool TrySubmit(int score);

    int Get() const { return value; }

    // true nếu lần ghi file gần nhất thất bại (ví dụ ổ cứng từ chối quyền ghi/hết dung
    // lượng) - GameManager có thể dùng để cảnh báo người chơi thay vì crash âm thầm.
    bool DidLastWriteFail() const { return lastWriteFailed; }
};
