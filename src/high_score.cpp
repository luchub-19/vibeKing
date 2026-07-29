#include "high_score.h"
#include <fstream>

void HighScore::Load(const std::string& path) {
    filePath = path;
    std::ifstream file(filePath);
    if (file.is_open()) {
        file >> value;
        if (file.fail()) value = 0; // File tồn tại nhưng nội dung hỏng -> coi như chưa có điểm
        file.close();
    } else {
        value = 0; // Chưa từng chơi -> chưa có file, không phải lỗi
    }
}

bool HighScore::TrySubmit(int score) {
    if (score <= value) return false;
    value = score;
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << value;
        file.close();
    }
    return true;
}
