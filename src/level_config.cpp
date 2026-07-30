#include "level_config.h"
#include "raylib.h"
#include "config.h"
#include "text_utils.h"
#include <fstream>
#include <charconv>

using TextUtils::Trim;
using TextUtils::IEquals;

void LevelGridConfig::Clamp() {
    if (rows < 1) rows = 1;
    if (rows > Config::MAX_GRID_ROWS) rows = Config::MAX_GRID_ROWS;
    if (cols < 1) cols = 1;
    if (cols > Config::MAX_GRID_COLS) cols = Config::MAX_GRID_COLS;
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

    // getline can 1 buffer std::string de doc tung dong - day la copy DUY NHAT khong
    // the tranh (I/O phai co noi chua). Moi buoc xu ly SAU do (Trim, tach key/value)
    // deu lam viec tren string_view TRO THANG vao buffer `line` nay, khong copy them
    // lan nao nua cho toi khi thuc su can std::string (khong cho, xem duoi).
    std::string line;
    while (std::getline(file, line)) {
        std::string_view trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue; // Bỏ qua dòng trống / comment

        size_t eq = trimmed.find('=');
        if (eq == std::string_view::npos) continue; // Dòng không đúng định dạng KEY=VALUE -> bỏ qua

        std::string_view key = Trim(trimmed.substr(0, eq));
        std::string_view val = Trim(trimmed.substr(eq + 1));
        if (val.empty()) continue;

        // std::from_chars parse THẲNG từ buffer gốc - không construct std::string trung
        // gian chỉ để gọi std::stoi/std::stof như trước, và không throw exception (trả
        // về error code) nên không cần try/catch nữa.
        auto tryInt = [&](int& out) {
            auto res = std::from_chars(val.data(), val.data() + val.size(), out);
            return res.ec == std::errc{};
        };
        auto tryFloat = [&](float& out) {
            auto res = std::from_chars(val.data(), val.data() + val.size(), out);
            return res.ec == std::errc{};
        };

        bool ok = true;
        if (IEquals(key, "ROWS"))            ok = tryInt(cfg.rows);
        else if (IEquals(key, "COLS"))       ok = tryInt(cfg.cols);
        else if (IEquals(key, "START_X"))    ok = tryFloat(cfg.startX);
        else if (IEquals(key, "START_Y"))    ok = tryFloat(cfg.startY);
        else if (IEquals(key, "SPACING_X"))  ok = tryFloat(cfg.spacingX);
        else if (IEquals(key, "SPACING_Y"))  ok = tryFloat(cfg.spacingY);
        // Key lạ: bỏ qua thay vì lỗi, để file cấu hình dễ mở rộng về sau

        if (!ok) {
            // Giá trị không parse được (vd "ROWS=abc") -> giữ nguyên default cho field đó.
            // %.*s in dung do dai view, khong can null-terminate rieng.
            TraceLog(LOG_WARNING, "LevelGridConfig: gia tri khong hop le cho key '%.*s'", (int)key.size(), key.data());
        }
    }

    cfg.Clamp();
    return cfg;
}
