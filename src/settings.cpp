#include "settings.h"
#include "raylib.h"
#include "text_utils.h"
#include <fstream>
#include <charconv>
#include <cmath>
#include "atomic_file.h"

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

    void ParseFloatKey(std::string_view val, float& target) {
        float v;
        auto res = std::from_chars(val.data(), val.data() + val.size(), v);
        if (res.ec == std::errc{}) target = v;
        else TraceLog(LOG_WARNING, "Settings: gia tri so thuc khong hop le");
    }

    // NaN (from_chars nhan "nan") lot qua moi phep so sanh -> ve mac dinh truoc, roi kep 0..1.
    float SanitizeVolume(float v, float def) {
        if (std::isnan(v)) return def;
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    // Chi "0"/"1" - gia tri khac (sua tay sai) giu nguyen mac dinh thay vi doan y.
    void ParseBoolKey(std::string_view val, bool& target) {
        if (val == "1") target = true;
        else if (val == "0") target = false;
        else TraceLog(LOG_WARNING, "Settings: gia tri bat/tat khong hop le (chi nhan 0/1)");
    }
}

Settings Settings::LoadFromFile(const std::string& path, Language defaultLanguage) {
    Settings cfg; // Bắt đầu từ default - file thiếu key nào thì key đó giữ default
    cfg.language = defaultLanguage;

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
            ParseFloatKey(val, cfg.volume);
        } else if (IEquals(key, "KEY_MOVE_LEFT")) {
            ParseIntKey(val, cfg.keyMoveLeft);
        } else if (IEquals(key, "KEY_MOVE_RIGHT")) {
            ParseIntKey(val, cfg.keyMoveRight);
        } else if (IEquals(key, "KEY_SHOOT")) {
            ParseIntKey(val, cfg.keyShoot);
        } else if (IEquals(key, "KEY_PAUSE")) {
            ParseIntKey(val, cfg.keyPause);
        } else if (IEquals(key, "GFX_QUALITY")) {
            for (int q = 0; q < GRAPHICS_QUALITY_COUNT; q++) {
                if (IEquals(val, GraphicsQualityLabel((GraphicsQuality)q))) cfg.graphics.quality = (GraphicsQuality)q;
            }
        } else if (IEquals(key, "GFX_CRT")) {
            ParseBoolKey(val, cfg.graphics.crtEnabled);
        } else if (IEquals(key, "GFX_REDUCE_FLASHING")) {
            ParseBoolKey(val, cfg.graphics.reduceFlashing);
        } else if (IEquals(key, "GFX_SHAKE")) {
            ParseIntKey(val, cfg.graphics.shakePercent);
        } else if (IEquals(key, "GFX_COLOR_FILTER")) {
            for (int f = 0; f < COLOR_FILTER_COUNT; f++) {
                if (IEquals(val, ColorFilterCode((ColorFilter)f))) cfg.graphics.colorFilter = (ColorFilter)f;
            }
        } else if (IEquals(key, "MUSIC_VOLUME")) {
            ParseFloatKey(val, cfg.musicVolume);
        } else if (IEquals(key, "SFX_VOLUME")) {
            ParseFloatKey(val, cfg.sfxVolume);
        } else if (IEquals(key, "UI_SOUNDS")) {
            ParseBoolKey(val, cfg.uiSounds);
        } else if (IEquals(key, "LANGUAGE")) {
            cfg.language = Loc::LanguageFromCode(val, cfg.language);
        } else if (IEquals(key, "FULLSCREEN")) {
            ParseBoolKey(val, cfg.fullscreen);
        } else if (IEquals(key, "SHOW_FPS")) {
            ParseBoolKey(val, cfg.showFps);
        } else if (IEquals(key, "AUTO_FIRE")) {
            ParseBoolKey(val, cfg.autoFire);
        } else if (IEquals(key, "GAME_SPEED")) {
            ParseIntKey(val, cfg.gameSpeedPercent);
        } else if (IEquals(key, "SHOW_HITBOX")) {
            ParseBoolKey(val, cfg.showHitbox);
        }
    }

    // from_chars chap nhan ca "nan", va moi phep so sanh voi NaN deu false - 2 dong clamp ben
    // duoi de NaN lot qua, roi di thang vao SetMasterVolume(). NaN = gia tri vo nghia -> mac dinh.
    cfg.volume = SanitizeVolume(cfg.volume, Settings{}.volume);
    cfg.musicVolume = SanitizeVolume(cfg.musicVolume, Settings{}.musicVolume);
    cfg.sfxVolume = SanitizeVolume(cfg.sfxVolume, Settings{}.sfxVolume);

    // GAME_SPEED sua tay ngoai danh sach (vd 5 -> game gan nhu dung yen) -> 100. KHONG lam tron
    // ve muc gan nhat nhu GFX_SHAKE: toc do choi la thu anh huong bang xep hang, gia tri la phai
    // ve "khong ho tro" chu khong phai doan y.
    bool speedOk = false;
    for (int lv : GAME_SPEED_LEVELS) speedOk = speedOk || (lv == cfg.gameSpeedPercent);
    if (!speedOk) cfg.gameSpeedPercent = 100;

    // Chi chap nhan ma phim trong vung hop le cua raylib (MAX_KEYBOARD_KEYS=512, xac
    // nhan truc tiep tu rcore.c) - file bi sua tay/hong voi gia tri vo ly (am, qua lon)
    // se bi tra ve mac dinh thay vi giu 1 ma phim "khong bao gio khop duoc voi phim
    // nao" ve sau.
    auto validOrDefault = [](int key, int def) { return (key > 0 && key < 512) ? key : def; };
    cfg.keyMoveLeft  = validOrDefault(cfg.keyMoveLeft, KEY_A);
    cfg.keyMoveRight = validOrDefault(cfg.keyMoveRight, KEY_D);
    cfg.keyShoot     = validOrDefault(cfg.keyShoot, KEY_SPACE);
    cfg.keyPause     = validOrDefault(cfg.keyPause, KEY_P);

    cfg.graphics.Sanitize();

    return cfg;
}

void Settings::SaveToFile(const std::string& path) const {
    // GHI ATOMIC: ghi ra file .tmp truoc, roi AtomicFile::Replace() de len file that - la
    // rename(2) tren POSIX va MoveFileEx(REPLACE_EXISTING) tren Windows (std::rename() thuan
    // cua Windows tu choi ghi de file da ton tai - xem atomic_file.h) - hoac file .tmp thay the HOAN TOAN file goc, hoac
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
        file << "GFX_QUALITY=" << GraphicsQualityLabel(graphics.quality) << "\n";
        file << "GFX_CRT=" << (graphics.crtEnabled ? 1 : 0) << "\n";
        file << "GFX_REDUCE_FLASHING=" << (graphics.reduceFlashing ? 1 : 0) << "\n";
        file << "GFX_SHAKE=" << graphics.shakePercent << "\n";
        file << "GFX_COLOR_FILTER=" << ColorFilterCode(graphics.colorFilter) << "\n";
        file << "MUSIC_VOLUME=" << musicVolume << "\n";
        file << "SFX_VOLUME=" << sfxVolume << "\n";
        file << "UI_SOUNDS=" << (uiSounds ? 1 : 0) << "\n";
        file << "LANGUAGE=" << Loc::LanguageCode(language) << "\n";
        file << "FULLSCREEN=" << (fullscreen ? 1 : 0) << "\n";
        file << "SHOW_FPS=" << (showFps ? 1 : 0) << "\n";
        file << "AUTO_FIRE=" << (autoFire ? 1 : 0) << "\n";
        file << "GAME_SPEED=" << gameSpeedPercent << "\n";
        file << "SHOW_HITBOX=" << (showHitbox ? 1 : 0) << "\n";
    } // Dong scope -> ofstream flush + dong file truoc khi rename ben duoi

    if (!AtomicFile::Replace(tmpPath, path)) {
        TraceLog(LOG_WARNING, "Settings: rename '%s' -> '%s' that bai, giu nguyen file cu",
                  tmpPath.c_str(), path.c_str());
    }
}
