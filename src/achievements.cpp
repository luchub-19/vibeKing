#include "achievements.h"
#include "raylib.h"
#include "save_checksum.h"
#include "atomic_file.h"
#include <fstream>
#include <sstream>

const AchievementDescriptor& GetAchievementDescriptor(AchievementId id) {
    // Thứ tự PHẢI khớp enum AchievementId - static_assert bên dưới bắt trường hợp thêm enum
    // mà quên thêm dòng (khi đó index cuối sẽ đọc ra ngoài mảng).
    static const AchievementDescriptor table[] = {
        { Str::AchFirstContact, Str::AchFirstContactDesc, 1,     5 },
        { Str::AchChainReaction, Str::AchChainReactionDesc, 10,    20 },
        { Str::AchHoldingTheLine, Str::AchHoldingTheLineDesc, 5,     15 },
        { Str::AchVeteran, Str::AchVeteranDesc, 10,    40 },
        { Str::AchGiantSlayer, Str::AchGiantSlayerDesc, 1,     25 },
        { Str::AchUntouchable, Str::AchUntouchableDesc, 0,     20 },
        { Str::AchHighRoller, Str::AchHighRollerDesc, 25000, 30 },
        { Str::AchExterminator, Str::AchExterminatorDesc, 1000,  50 },
    };
    static_assert(sizeof(table) / sizeof(table[0]) == ACHIEVEMENT_COUNT,
                  "Moi AchievementId phai co dung 1 dong trong bang descriptor");
    int idx = (int)id;
    if (idx < 0 || idx >= ACHIEVEMENT_COUNT) idx = 0; // Gia tri ngoai du kien - khong doc ra ngoai mang
    return table[idx];
}

bool IsAchievementMet(AchievementId id, const AchievementSnapshot& snap) {
    int t = GetAchievementDescriptor(id).threshold;
    switch (id) {
        case AchievementId::FirstContact:   return snap.lifetimeKills >= t;
        case AchievementId::ChainReaction:  return snap.runBestCombo >= t;
        case AchievementId::HoldingTheLine: return snap.waveReached >= t;
        case AchievementId::Veteran:        return snap.waveReached >= t;
        case AchievementId::GiantSlayer:    return snap.bossDefeated;
        case AchievementId::Untouchable:    return snap.flawlessClear;
        case AchievementId::HighRoller:     return snap.score >= t;
        case AchievementId::Exterminator:   return snap.lifetimeKills >= t;
        default:                            return false;
    }
}

void AchievementProgress::Load(const std::string& path) {
    filePath = path;
    lifetimeKills = 0;
    for (bool& u : unlocked) u = false;

    std::ifstream file(path);
    if (!file.is_open()) {
        TraceLog(LOG_INFO, "Achievements: khong tim thay '%s', bat dau tu 0 thanh tuu", path.c_str());
        return;
    }

    // Cung co che chong sua tay voi MetaProgress::Load() - xem save_checksum.h.
    std::string sigLine;
    if (!std::getline(file, sigLine) || sigLine.rfind("SIG ", 0) != 0) {
        TraceLog(LOG_WARNING, "Achievements: file '%s' thieu checksum hop le - bo qua", path.c_str());
        return;
    }
    std::ostringstream bodyBuf;
    bodyBuf << file.rdbuf();
    std::string body = bodyBuf.str();
    if (SaveChecksum::ToHex(SaveChecksum::Fnv1a64(body)) != sigLine.substr(4)) {
        TraceLog(LOG_WARNING, "Achievements: file '%s' KHONG KHOP checksum - tu choi nap", path.c_str());
        return;
    }

    std::istringstream in(body);
    int kills = 0;
    if (!(in >> kills)) return;
    lifetimeKills = kills > 0 ? kills : 0;
    // Doc lan luot tung co theo thu tu enum. File tu ban CU hon (it thanh tuu hon) se het
    // so som - cac thanh tuu moi them giu false, tuong thich nguoc khong can bump version
    // (cung cach MetaProgress xu ly truong skin them sau).
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        int flag = 0;
        if (!(in >> flag)) break;
        unlocked[i] = (flag != 0);
    }
}

void AchievementProgress::Save(const std::string& path) const {
    if (path.empty()) return; // Chua tung Load() (vd GameManager trong unit test) - khong ghi gi ra cwd

    std::ostringstream bodyBuf;
    bodyBuf << lifetimeKills;
    for (bool u : unlocked) bodyBuf << " " << (u ? 1 : 0);
    bodyBuf << "\n";
    std::string body = bodyBuf.str();

    std::string tmpPath = path + ".tmp";
    {
        std::ofstream file(tmpPath, std::ios::trunc);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "Achievements: khong the ghi file tam '%s'", tmpPath.c_str());
            return;
        }
        file << "SIG " << SaveChecksum::ToHex(SaveChecksum::Fnv1a64(body)) << "\n" << body;
    }
    if (!AtomicFile::Replace(tmpPath, path)) {
        TraceLog(LOG_WARNING, "Achievements: rename '%s' -> '%s' that bai, giu nguyen file cu",
                 tmpPath.c_str(), path.c_str());
    }
}

void AchievementProgress::Flush() const {
    Save(filePath);
}

bool AchievementProgress::IsUnlocked(AchievementId id) const {
    int idx = (int)id;
    return idx >= 0 && idx < ACHIEVEMENT_COUNT && unlocked[idx];
}

int AchievementProgress::UnlockedCount() const {
    int n = 0;
    for (bool u : unlocked) n += u ? 1 : 0;
    return n;
}

std::vector<AchievementId> AchievementProgress::Evaluate(const AchievementSnapshot& snap) {
    std::vector<AchievementId> newlyUnlocked;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        AchievementId id = (AchievementId)i;
        if (unlocked[i] || !IsAchievementMet(id, snap)) continue;
        unlocked[i] = true;
        newlyUnlocked.push_back(id);
    }
    if (!newlyUnlocked.empty()) Save(filePath); // Mo khoa la thay doi hiem va quan trong - ghi ngay
    return newlyUnlocked;
}
