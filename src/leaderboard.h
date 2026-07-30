#pragma once
#include <string>
#include <vector>
#include "config.h"

struct LeaderboardEntry {
    int score = 0;
    int wave = 0;
};

enum class SubmitResult { NotQualified, MadeTop10, NewRecord };

// Top Config::LEADERBOARD_MAX_ENTRIES điểm cao nhất, kèm wave đạt được - thay thế hệ
// thống chỉ lưu đúng 1 mốc HighScore duy nhất trước đây. Ghi file theo cơ chế atomic
// (.tmp + rename) giống Settings - xem SaveToFile() trong leaderboard.cpp.
class Leaderboard {
private:
    std::vector<LeaderboardEntry> entries; // Luôn giữ sắp xếp giảm dần theo score, tối đa LEADERBOARD_MAX_ENTRIES phần tử
    std::string filePath;

    void SaveToFile(const std::string& path) const;

public:
    void Load(const std::string& path);

    // NewRecord: điểm này giờ là #1. MadeTop10: lọt vào danh sách nhưng không phải #1.
    // NotQualified: không đủ điểm để lọt top (RAM và file đều không đổi).
    SubmitResult TrySubmit(int score, int wave);

    const std::vector<LeaderboardEntry>& GetEntries() const { return entries; }
    int GetTopScore() const { return entries.empty() ? 0 : entries[0].score; }
};
