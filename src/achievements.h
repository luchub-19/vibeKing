#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "localization.h"

// ==========================================
// THANH TỰU (ACHIEVEMENTS) - lớp meta-progression thứ hai, song song với MetaProgress
// (currency + loadout). MetaProgress chỉ trả lời "đã kiếm được bao nhiêu CR"; thành tựu trả
// lời "đã LÀM được gì" - cho người chơi mục tiêu cụ thể ngoài việc cày điểm (giữ combo
// x10, qua 1 wave không mất mạng, hạ boss...), và mỗi cái thưởng thêm CR nên nó vẫn nối
// thẳng vào vòng lặp mở khoá loadout sẵn có thay vì là 1 hệ thống trang trí đứng riêng.
//
// Copy đúng khuôn MetaProgress/Leaderboard: checksum FNV-1a (save_checksum.h) + ghi atomic
// .tmp -> AtomicFile::Replace() (atomic_file.h). Khác 1 điểm: Save() là NO-OP khi chưa
// từng Load() (filePath rỗng) - GameManager trong unit test không bao giờ gọi Run() nên
// không Load(); nếu không chặn, mỗi test hạ được 1 địch sẽ rớt file ".tmp" ra cwd.
//
// THƯỞNG CR KHÔNG được cộng ngay lúc mở khoá - xem GameManager::CheckAchievements(): CR chỉ
// đi vào MetaProgress tại 1 thời điểm duy nhất là kết thúc ván (TriggerGameOver), và có
// test khoá "WAVE_CLEAR không cộng currency". Thành tựu mở khoá giữa ván thì phần thưởng
// được dồn lại, trả cùng lúc với bảng tổng kết (nên "CURRENCY EARNED" khớp số thật nhận).
// ==========================================
enum class AchievementId : uint8_t {
    FirstContact,   // Hạ địch đầu tiên (trọn đời)
    ChainReaction,  // Combo xN trong 1 ván
    HoldingTheLine, // Tới wave N
    Veteran,        // Tới wave N (cao hơn)
    GiantSlayer,    // Hạ 1 boss
    Untouchable,    // Dọn sạch 1 wave mà không mất mạng nào
    HighRoller,     // Đạt N điểm trong 1 ván
    Exterminator,   // Hạ N địch (trọn đời)
    COUNT
};
constexpr int ACHIEVEMENT_COUNT = (int)AchievementId::COUNT;

// 1-NGUỒN-DUY-NHẤT cho tên/mô tả/ngưỡng/phần thưởng: `descriptionFmt` nhận đúng `threshold`
// qua TextFormat - con số người chơi đọc trên màn hình và con số IsAchievementMet() so sánh
// là CÙNG 1 field, không thể lệch nhau như Str::Upgrade*Desc (phải đồng bộ tay với config).
// Hardcode ở đây (không qua balance.json) theo cùng tiền lệ GetLoadoutUnlockCost().
// Tên/mô tả là MÃ CHUỖI (localization.h) chứ không phải const char*: bảng tĩnh khởi tạo 1 lần,
// còn ngôn ngữ đổi được lúc chạy - nơi vẽ gọi Tr() mỗi frame.
struct AchievementDescriptor {
    Str name;
    Str descriptionFmt; // Có đúng 1 %d = threshold (hoặc không có % nào nếu threshold vô nghĩa)
    int threshold;
    int rewardCurrency;
};
const AchievementDescriptor& GetAchievementDescriptor(AchievementId id);

// Ảnh chụp số liệu tại 1 thời điểm - GameManager gom lại rồi đưa vào Evaluate(). Tách ra
// struct thuần (không biết GameManager tồn tại) để test được điều kiện từng thành tựu mà
// không cần dựng cả thế giới game.
struct AchievementSnapshot {
    int lifetimeKills = 0;
    int runBestCombo = 0;
    int waveReached = 0;
    int score = 0;
    bool bossDefeated = false;   // Chỉ true đúng lúc boss vừa bị hạ
    bool flawlessClear = false;  // Chỉ true đúng lúc vừa dọn sạch 1 wave với 0 mạng mất trong wave đó
};

bool IsAchievementMet(AchievementId id, const AchievementSnapshot& snap);

struct AchievementProgress {
private:
    bool unlocked[ACHIEVEMENT_COUNT] = {};
    int lifetimeKills = 0;
    std::string filePath;

public:
    void Load(const std::string& path);
    void Save(const std::string& path) const;
    // Ghi lại file đã Load() (no-op nếu chưa Load) - dùng để lưu lifetimeKills, vốn KHÔNG
    // được ghi mỗi lần hạ địch (vài chục lần ghi đĩa/giây giữa trận là vô lý).
    void Flush() const;

    void AddKills(int n) { if (n > 0) lifetimeKills += n; }
    int GetLifetimeKills() const { return lifetimeKills; }

    bool IsUnlocked(AchievementId id) const;
    int UnlockedCount() const;

    // Mở khoá MỌI thành tựu chưa mở mà `snap` thoả, lưu file 1 lần nếu có gì mới, trả về
    // ĐÚNG những cái vừa mở trong lần gọi này (gọi lại với cùng snapshot -> rỗng). Gọi mỗi
    // frame được: chi phí là ACHIEVEMENT_COUNT phép so sánh, không ghi đĩa khi không đổi.
    std::vector<AchievementId> Evaluate(const AchievementSnapshot& snap);
};
