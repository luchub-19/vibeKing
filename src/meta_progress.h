#pragma once
#include <string>
#include <cstdint>

// ==========================================
// META-PROGRESSION & LOADOUT
// Diem tich luy XUYEN SUOT nhieu van choi (khac han Player::GetScore() - von reset ve 0
// moi lan Player::Reset()) - quy doi tu diem so CUOI CUNG cua 1 van (xem AwardCurrency),
// dung de mo khoa loadout khoi dau. Day la ly do de nguoi choi quay lai choi tiep sau khi
// Game Over thay vi tat game: du thua van nay, currency tich duoc van con nguyen cho van
// sau (xem GameManager::metaProgress trong game_manager.h).
//
// Copy dung khuon Leaderboard (checksum FNV-1a qua save_checksum.h + ghi file kieu atomic
// .tmp + rename - xem MetaProgress::Save() trong meta_progress.cpp). Khac Leaderboard o 1
// diem: Leaderboard chi bao gio THEM entry (khong bao gio "tieu" diem da co), con
// MetaProgress::TryUnlock() thuc su TRU currency - nen can kiem tra du dieu kien truoc khi
// tru, khong chi kiem tra checksum luc doc file.
// ==========================================
enum class LoadoutType : uint8_t { Standard, Vanguard, Overcharge };

// Ten hien thi + chi phi mo khoa theo tung loadout - gom vao 2 ham nay de UI
// (RenderSystem::DrawLoadoutSelect, render_system.cpp) va logic chon/mo khoa
// (GameManager::UpdateMenu, game_manager.cpp) luon doc CHUNG 1 nguon, khong the lech nhau
// ve sau (dung tinh than RebindableAction da dung trong game_manager.h).
inline const char* GetLoadoutName(LoadoutType type) {
    switch (type) {
        case LoadoutType::Vanguard:   return "VANGUARD";
        case LoadoutType::Overcharge: return "OVERCHARGE";
        default:                      return "STANDARD"; // Standard va moi gia tri la ngoai du kien
    }
}

inline int GetLoadoutUnlockCost(LoadoutType type) {
    switch (type) {
        case LoadoutType::Vanguard:   return 150;
        case LoadoutType::Overcharge: return 400;
        default:                      return 0; // Standard - mien phi, khong can mo khoa
    }
}

struct MetaProgress {
private:
    int totalCurrency = 0;
    bool unlockedVanguard = false;
    bool unlockedOvercharge = false;
    std::string filePath;

public:
    void Load(const std::string& path);
    void Save(const std::string& path) const;

    // Quy doi diem so CUOI van (Player::GetScore() luc Game Over) ra currency theo
    // Config::META_SCORE_TO_CURRENCY_RATE, cong don vao totalCurrency roi LUU FILE NGAY
    // (khong doi den lan Save() thu cong nao khac - xem game_manager.cpp, noi goi ham nay
    // dung 1 lan duy nhat tai doan code chuyen state sang GAME_OVER).
    void AwardCurrency(int scoreThisRun);

    // Thu mo khoa 1 loadout: neu CHUA mo khoa truoc do VA du currency thi tru currency +
    // bat co unlocked tuong ung + luu file ngay (giong TrySubmit cua Leaderboard - moi
    // thay doi trang thai deu duoc ghi xuong dia ngay lap tuc, khong dem den lan Save()
    // thu cong sau). Tra ve false (khong lam gi ca, khong tru tien) neu: type la Standard
    // (mien phi san, khong co gi de "mo khoa"), da mo khoa tu truoc, hoac khong du currency.
    bool TryUnlock(LoadoutType type, int cost);

    int GetCurrency() const { return totalCurrency; }
    bool IsUnlocked(LoadoutType type) const;
};
