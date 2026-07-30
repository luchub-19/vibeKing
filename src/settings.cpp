#include "settings.h"
#include "raylib.h"
#include "text_utils.h"
#include <fstream>
#include <charconv>
#include <cstdio> // std::rename

using TextUtils::Trim;
using TextUtils::IEquals;

namespace {
    // So khop voi DifficultyStats::label trong config.h - dung chung 1 nguon chu de
    // tranh viet tay 2 bang string de lech nhau. So sanh khong phan biet hoa/thuong
    // truc tiep tren string_view, khong uppercase-copy ra std::string moi.
    Difficulty DifficultyFromLabel(std::string_view label, Difficulty fallback) {
        if (IEquals(label, GetDifficultyStats(Difficulty::EASY).label)) return Difficulty::EASY;
        if (IEquals(label, GetDifficultyStats(Difficulty::HARD).label)) return Difficulty::HARD;
        if (IEquals(label, GetDifficultyStats(Difficulty::NORMAL).label)) return Difficulty::NORMAL;
        return fallback;
    }
}

Settings Settings::LoadFromFile(const std::string& path) {
    Settings cfg; // Bắt đầu từ default - file thiếu key nào thì key đó giữ default

    std::ifstream file(path);
    if (!file.is_open()) {
        TraceLog(LOG_INFO, "Settings: khong tim thay '%s', dung gia tri mac dinh", path.c_str());
        return cfg;
    }

    std::string line; // Buffer doc dong duy nhat - phan con lai lam viec tren string_view
    while (std::getline(file, line)) {
        std::string_view trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        size_t eq = trimmed.find('=');
        if (eq == std::string_view::npos) continue;

        std::string_view key = Trim(trimmed.substr(0, eq));
        std::string_view val = Trim(trimmed.substr(eq + 1));
        if (val.empty()) continue;

        if (IEquals(key, "DIFFICULTY")) {
            cfg.difficulty = DifficultyFromLabel(val, cfg.difficulty);
        } else if (IEquals(key, "VOLUME")) {
            float v;
            auto res = std::from_chars(val.data(), val.data() + val.size(), v);
            if (res.ec == std::errc{}) cfg.volume = v;
            else TraceLog(LOG_WARNING, "Settings: gia tri VOLUME khong hop le");
        }
    }

    if (cfg.volume < 0.0f) cfg.volume = 0.0f;
    if (cfg.volume > 1.0f) cfg.volume = 1.0f;
    return cfg;
}

void Settings::SaveToFile(const std::string& path) const {
    // GHI ATOMIC: ghi ra file .tmp truoc, roi rename() de len file that. rename() la
    // thao tac ATOMIC tren he thong file cua ca POSIX (rename(2)) lan Windows (cung 1
    // lenh MoveFileEx tuong duong) - hoac file .tmp thay the HOAN TOAN file goc, hoac
    // khong co gi xay ra ca. Neu ghi truc tiep de len `path` va process bi kill giua
    // chung (mat dien, crash, force-quit) thi file settings.cfg co the bi cat cut nua
    // dong, lan sau doc len parse loi/mat du lieu - cach nay loai bo hoan toan kha nang
    // do vi file goc khong bao gio bi dung o trang thai "dang ghi do".
    std::string tmpPath = path + ".tmp";

    {
        std::ofstream file(tmpPath, std::ios::trunc);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "Settings: khong the ghi file tam '%s'", tmpPath.c_str());
            return;
        }
        file << "DIFFICULTY=" << GetDifficultyStats(difficulty).label << "\n";
        file << "VOLUME=" << volume << "\n";
    } // Dong scope -> ofstream flush + dong file truoc khi rename ben duoi

    if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {
        TraceLog(LOG_WARNING, "Settings: rename '%s' -> '%s' that bai, giu nguyen file cu",
                  tmpPath.c_str(), path.c_str());
    }
}
