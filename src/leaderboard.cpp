#include "leaderboard.h"
#include "raylib.h"
#include <fstream>
#include <algorithm>
#include <cstdio> // std::rename

void Leaderboard::Load(const std::string& path) {
    filePath = path;
    entries.clear();

    std::ifstream file(path);
    if (!file.is_open()) {
        TraceLog(LOG_INFO, "Leaderboard: khong tim thay '%s', bat dau danh sach rong", path.c_str());
        return;
    }

    int score, wave;
    while (file >> score >> wave) {
        entries.push_back({ score, wave });
        if ((int)entries.size() >= Config::LEADERBOARD_MAX_ENTRIES) break; // File hỏng/dài bất thường cũng không đọc quá giới hạn
    }

    // Sắp lại cho chắc (phòng trường hợp file bị chỉnh tay sai thứ tự) thay vì tin
    // tưởng mù quáng thứ tự đã lưu trong file.
    std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        return a.score > b.score;
    });
}

SubmitResult Leaderboard::TrySubmit(int score, int wave) {
    bool isNewRecord = entries.empty() || score > entries[0].score;
    bool hasRoom = (int)entries.size() < Config::LEADERBOARD_MAX_ENTRIES;
    bool beatsWeakest = !entries.empty() && score > entries.back().score;

    if (!hasRoom && !beatsWeakest) return SubmitResult::NotQualified;

    entries.push_back({ score, wave });
    std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        return a.score > b.score;
    });
    if ((int)entries.size() > Config::LEADERBOARD_MAX_ENTRIES) {
        entries.resize(Config::LEADERBOARD_MAX_ENTRIES);
    }

    SaveToFile(filePath);
    return isNewRecord ? SubmitResult::NewRecord : SubmitResult::MadeTop10;
}

void Leaderboard::SaveToFile(const std::string& path) const {
    // GHI ATOMIC: cùng cơ chế .tmp + rename() như Settings::SaveToFile() - rename() là
    // thao tác ATOMIC ở cả POSIX lẫn Windows, nên crash giữa chừng không bao giờ để lại
    // 1 file leaderboard.dat bị cắt cụt/nửa dòng (mất sạch cả 10 điểm cao thay vì chỉ 1
    // giá trị như HighScore cũ, nên atomic ở đây quan trọng hơn trước).
    std::string tmpPath = path + ".tmp";

    {
        std::ofstream file(tmpPath, std::ios::trunc);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "Leaderboard: khong the ghi file tam '%s'", tmpPath.c_str());
            return;
        }
        for (const LeaderboardEntry& e : entries) {
            file << e.score << " " << e.wave << "\n";
        }
    } // Đóng scope -> ofstream flush + đóng file trước khi rename bên dưới

    if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {
        TraceLog(LOG_WARNING, "Leaderboard: rename '%s' -> '%s' that bai, giu nguyen file cu",
                  tmpPath.c_str(), path.c_str());
    }
}
