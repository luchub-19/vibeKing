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

    // 1 VÁN = 1 DÒNG. Game nộp điểm ở MỖI lần WAVE_CLEAR (checkpoint - bấm R giữa chừng
    // không mất thành tích) rồi nộp lần cuối ở GAME_OVER. Trước đây mỗi lần nộp là 1 dòng
    // MỚI: 1 ván chơi tới wave 10 tự lấp kín Top 10 bằng 10 dòng của chính nó, đẩy mọi
    // ván khác ra ngoài. Giờ dòng đã nộp trong ván hiện tại được nhớ lại ở đây và lần nộp
    // sau trong CÙNG ván sẽ THAY nó thay vì thêm dòng mới. BeginRun() mở 1 ván mới.
    bool inRun = false;          // false = chưa BeginRun(): mỗi TrySubmit() là 1 dòng độc lập
    bool runHasEntry = false;    // Ván hiện tại đã có dòng nằm trong `entries` chưa
    LeaderboardEntry runEntry;   // Giá trị dòng đó (để tìm lại sau khi sort)

    void SaveToFile(const std::string& path) const;

public:
    void Load(const std::string& path);

    // Bắt đầu 1 ván mới - gọi ở InitLevel(newGame=true). Từ đây mọi TrySubmit() tới lần
    // BeginRun() kế tiếp cập nhật CHUNG 1 dòng.
    void BeginRun() { inRun = true; runHasEntry = false; }

    // NewRecord: điểm này giờ là #1. MadeTop10: lọt vào danh sách nhưng không phải #1.
    // NotQualified: không đủ điểm để lọt top (RAM và file đều không đổi).
    SubmitResult TrySubmit(int score, int wave);

    const std::vector<LeaderboardEntry>& GetEntries() const { return entries; }
    int GetTopScore() const { return entries.empty() ? 0 : entries[0].score; }
};
