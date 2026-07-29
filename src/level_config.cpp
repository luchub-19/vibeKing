#include "level_config.h"
#include "raylib.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {
    std::string Trim(const std::string& s) {
        size_t begin = s.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(begin, end - begin + 1);
    }
}

void LevelGridConfig::Clamp() {
    if (rows < 1) rows = 1;
    if (rows > 12) rows = 12;
    if (cols < 1) cols = 1;
    if (cols > 20) cols = 20;
    if (spacingX < 1.0f) spacingX = 1.0f;
    if (spacingY < 1.0f) spacingY = 1.0f;
    if (startX < 0.0f) startX = 0.0f;
    if (startY < 0.0f) startY = 0.0f;
}

LevelGridConfig LevelGridConfig::LoadFromFile(const std::string& path) {
    LevelGridConfig cfg; // Bắt đầu từ default - file thiếu key nào thì key đó giữ default

    std::ifstream file(path);
    if (!file.is_open()) {
        TraceLog(LOG_INFO, "LevelGridConfig: khong tim thay '%s', dung gia tri mac dinh", path.c_str());
        return cfg;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue; // Bỏ qua dòng trống / comment

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue; // Dòng không đúng định dạng KEY=VALUE -> bỏ qua

        std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));
        if (val.empty()) continue;

        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::toupper(c); });

        try {
            if (key == "ROWS")          cfg.rows = std::stoi(val);
            else if (key == "COLS")     cfg.cols = std::stoi(val);
            else if (key == "START_X")  cfg.startX = std::stof(val);
            else if (key == "START_Y")  cfg.startY = std::stof(val);
            else if (key == "SPACING_X") cfg.spacingX = std::stof(val);
            else if (key == "SPACING_Y") cfg.spacingY = std::stof(val);
            // Key lạ: bỏ qua thay vì lỗi, để file cấu hình dễ mở rộng về sau
        } catch (...) {
            // Giá trị không parse được (vd "ROWS=abc") -> giữ nguyên default cho field đó
            TraceLog(LOG_WARNING, "LevelGridConfig: gia tri khong hop le cho key '%s'", key.c_str());
        }
    }

    cfg.Clamp();
    return cfg;
}
