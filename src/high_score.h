#pragma once
#include <string>

// Đọc/ghi điểm cao ra file text đơn giản. Tách riêng để GameManager không phải
// biết gì về I/O - chỉ gọi Load()/TrySubmit(), đúng nguyên tắc single responsibility.
class HighScore {
private:
    int value = 0;
    std::string filePath;

public:
    void Load(const std::string& path);
    bool TrySubmit(int score); // true nếu đây là kỷ lục mới (và đã ghi file thành công)
    int Get() const { return value; }
};
