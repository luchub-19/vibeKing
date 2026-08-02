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

    // Dung chung cho ca 4 field ma phim moi (KEY_MOVE_LEFT/RIGHT/SHOOT/PAUSE) - tranh
    // lap y het 4 lan cung 1 doan parse std::from_chars.
    void ParseIntKey(std::string_view val, int& target) {
        int v;
        auto res = std::from_chars(val.data(), val.data() + val.size(), v);
        if (res.ec == std::errc{}) target = v;
        else TraceLog(LOG_WARNING, "Settings: gia tri ma phim khong hop le");
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
        } else if (IEquals(key, "KEY_MOVE_LEFT")) {
            ParseIntKey(val, cfg.keyMoveLeft);
        } else if (IEquals(key, "KEY_MOVE_RIGHT")) {
            ParseIntKey(val, cfg.keyMoveRight);
        } else if (IEquals(key, "KEY_SHOOT")) {
            ParseIntKey(val, cfg.keyShoot);
        } else if (IEquals(key, "KEY_PAUSE")) {
            ParseIntKey(val, cfg.keyPause);
        }
    }

    if (cfg.volume < 0.0f) cfg.volume = 0.0f;
    if (cfg.volume > 1.0f) cfg.volume = 1.0f;

    // Chi chap nhan ma phim trong vung hop le cua raylib (MAX_KEYBOARD_KEYS=512, xac
    // nhan truc tiep tu rcore.c) - file bi sua tay/hong voi gia tri vo ly (am, qua lon)
    // se bi tra ve mac dinh thay vi giu 1 ma phim "khong bao gio khop duoc voi phim
    // nao" ve sau.
    auto validOrDefault = [](int key, int def) { return (key > 0 && key < 512) ? key : def; };
    cfg.keyMoveLeft  = validOrDefault(cfg.keyMoveLeft, KEY_A);
    cfg.keyMoveRight = validOrDefault(cfg.keyMoveRight, KEY_D);
    cfg.keyShoot     = validOrDefault(cfg.keyShoot, KEY_SPACE);
    cfg.keyPause     = validOrDefault(cfg.keyPause, KEY_P);

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
        file << "KEY_MOVE_LEFT=" << keyMoveLeft << "\n";
        file << "KEY_MOVE_RIGHT=" << keyMoveRight << "\n";
        file << "KEY_SHOOT=" << keyShoot << "\n";
        file << "KEY_PAUSE=" << keyPause << "\n";
    } // Dong scope -> ofstream flush + dong file truoc khi rename ben duoi

    if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {
        TraceLog(LOG_WARNING, "Settings: rename '%s' -> '%s' that bai, giu nguyen file cu",
                  tmpPath.c_str(), path.c_str());
    }
}
