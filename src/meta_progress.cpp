#include "meta_progress.h"
#include "raylib.h"
#include "config.h"
#include "save_checksum.h"
#include <fstream>
#include <sstream>
#include <cstdio> // std::rename

void MetaProgress::Load(const std::string& path) {
    filePath = path;
    totalCurrency = 0;
    unlockedVanguard = false;
    unlockedOvercharge = false;

    std::ifstream file(path);
    if (!file.is_open()) {
        TraceLog(LOG_INFO, "MetaProgress: khong tim thay '%s', bat dau tu 0 currency", path.c_str());
        return;
    }

    // BAO MAT FILE SAVE: dung y het co che cua Leaderboard::Load() (xem leaderboard.cpp) -
    // dong dau tien PHAI la "SIG <hex>", checksum FNV-1a cua toan bo phan con lai cua file
    // (xem save_checksum.h). Thieu dong nay hoac khong khop -> tu choi nap TOAN BO, coi nhu
    // chua tung choi thay vi tin tuong mu quang du lieu co the da bi sua tay.
    std::string sigLine;
    if (!std::getline(file, sigLine) || sigLine.rfind("SIG ", 0) != 0) {
        TraceLog(LOG_WARNING, "MetaProgress: file '%s' thieu checksum hop le (dinh dang cu hoac bi hong) - bo qua, bat dau tu 0 currency", path.c_str());
        return;
    }
    std::string expectedHex = sigLine.substr(4);

    std::ostringstream bodyBuf;
    bodyBuf << file.rdbuf();
    std::string body = bodyBuf.str();

    std::string actualHex = SaveChecksum::ToHex(SaveChecksum::Fnv1a64(body));
    if (actualHex != expectedHex) {
        TraceLog(LOG_WARNING, "MetaProgress: file '%s' KHONG KHOP checksum (co the da bi chinh sua thu cong) - tu choi nap, bat dau tu 0 currency", path.c_str());
        return;
    }

    std::istringstream bodyIn(body);
    int currency = 0, vanguardFlag = 0, overchargeFlag = 0;
    if (bodyIn >> currency >> vanguardFlag >> overchargeFlag) {
        totalCurrency = currency;
        unlockedVanguard = (vanguardFlag != 0);
        unlockedOvercharge = (overchargeFlag != 0);
    }
    // Doc thieu/sai dinh dang (vd file bi cat cut dung luc) -> giu nguyen mac dinh da dat
    // o dau ham, khong nap "nua tin nua ngo" 1 phan gia tri.
}

void MetaProgress::Save(const std::string& path) const {
    // Ghi ATOMIC: cung co che .tmp + rename() nhu Leaderboard::SaveToFile()/Settings::
    // SaveToFile() - rename() la thao tac ATOMIC ca POSIX lan Windows, crash giua chung
    // khong bao gio de lai 1 file meta_progress.dat bi cat cut/nua dong.
    std::string tmpPath = path + ".tmp";

    // Xay noi dung "than" file (khong ke dong checksum) TRUOC de tinh hash tren dung chuoi
    // se duoc ghi xuong - tranh lech giua noi dung thuc te va hash neu format thay doi sau.
    std::ostringstream bodyBuf;
    bodyBuf << totalCurrency << " " << (unlockedVanguard ? 1 : 0) << " " << (unlockedOvercharge ? 1 : 0) << "\n";
    std::string body = bodyBuf.str();
    std::string sigHex = SaveChecksum::ToHex(SaveChecksum::Fnv1a64(body));

    {
        std::ofstream file(tmpPath, std::ios::trunc);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "MetaProgress: khong the ghi file tam '%s'", tmpPath.c_str());
            return;
        }
        file << "SIG " << sigHex << "\n" << body;
    } // Dong scope -> ofstream flush + dong file truoc khi rename ben duoi

    if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {
        TraceLog(LOG_WARNING, "MetaProgress: rename '%s' -> '%s' that bai, giu nguyen file cu",
                  tmpPath.c_str(), path.c_str());
    }
}

void MetaProgress::AwardCurrency(int scoreThisRun) {
    int earned = scoreThisRun / Config::META_SCORE_TO_CURRENCY_RATE;
    if (earned > 0) totalCurrency += earned;
    Save(filePath); // Luu ngay khi van ket thuc (khong doi Save() thu cong nao khac)
}

bool MetaProgress::TryUnlock(LoadoutType type, int cost) {
    if (type == LoadoutType::Standard || IsUnlocked(type)) return false; // Mien phi san / da mo khoa - khong lam gi
    if (totalCurrency < cost) return false; // Chua du currency

    totalCurrency -= cost;
    if (type == LoadoutType::Vanguard) unlockedVanguard = true;
    else if (type == LoadoutType::Overcharge) unlockedOvercharge = true;

    Save(filePath);
    return true;
}

bool MetaProgress::IsUnlocked(LoadoutType type) const {
    switch (type) {
        case LoadoutType::Vanguard:   return unlockedVanguard;
        case LoadoutType::Overcharge: return unlockedOvercharge;
        default:                      return true; // Standard - mien phi, luon san sang
    }
}
