#include "leaderboard.h"
#include "raylib.h"
#include "save_checksum.h"
#include <fstream>
#include <sstream>
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

    // BAO MAT FILE SAVE: dong dau tien PHAI la "SIG <hex>" - checksum FNV-1a cua toan
    // bo phan con lai cua file (xem save_checksum.h). Thieu dong nay (file cu tu ban
    // truoc khi co checksum, hoac file gia mao khong biet dinh dang) -> tu choi nap,
    // coi nhu khong co du lieu thay vi tin tuong mu quang.
    std::string sigLine;
    if (!std::getline(file, sigLine) || sigLine.rfind("SIG ", 0) != 0) {
        TraceLog(LOG_WARNING, "Leaderboard: file '%s' thieu checksum hop le (dinh dang cu hoac bi hong) - bo qua, bat dau danh sach rong", path.c_str());
        return;
    }
    std::string expectedHex = sigLine.substr(4);

    std::ostringstream bodyBuf;
    bodyBuf << file.rdbuf();
    std::string body = bodyBuf.str();

    std::string actualHex = SaveChecksum::ToHex(SaveChecksum::Fnv1a64(body));
    if (actualHex != expectedHex) {
        // KHONG KHOP: noi dung da bi sua sau khi ghi (sua tay bang text editor la truong
        // hop pho bien nhat) - tu choi toan bo file thay vi co gang nap phan "con doc
        // duoc", tranh nua tin nua ngo mot du lieu khong con dang tin cay.
        TraceLog(LOG_WARNING, "Leaderboard: file '%s' KHONG KHOP checksum (co the da bi chinh sua thu cong) - tu choi nap, bat dau danh sach rong", path.c_str());
        return;
    }

    std::istringstream bodyIn(body);
    int score, wave;
    while (bodyIn >> score >> wave) {
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
    // 1 van 0 diem KHONG phai thanh tich. Truoc day danh sach rong lam `isNewRecord` luon
    // dung, nen lan chet dau tien cua nguoi choi moi - ke ca chet o wave 1 voi dung 0 diem,
    // chua ban trung gi - van duoc chuc mung bang bang "NEW RECORD! (#1)" (da thay trong
    // anh chup game that). Vua la loi khen rong, vua nhet 1 dong 0 diem vao Top 10 vinh vien.
    // Chan o day thay vi o RenderSystem de MOI noi goi TrySubmit() deu duoc bao ve giong nhau,
    // va de danh sach khong bao gio chua ban ghi rac.
    if (score <= 0) return SubmitResult::NotQualified;

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

    // Xay noi dung "than" file (khong ke dong checksum) TRUOC de tinh hash tren dung
    // chuoi se duoc ghi xuong - tranh lech giua noi dung thuc te va hash neu format
    // thay doi sau nay.
    std::ostringstream bodyBuf;
    for (const LeaderboardEntry& e : entries) {
        bodyBuf << e.score << " " << e.wave << "\n";
    }
    std::string body = bodyBuf.str();
    std::string sigHex = SaveChecksum::ToHex(SaveChecksum::Fnv1a64(body));

    {
        std::ofstream file(tmpPath, std::ios::trunc);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "Leaderboard: khong the ghi file tam '%s'", tmpPath.c_str());
            return;
        }
        file << "SIG " << sigHex << "\n" << body;
    } // Đóng scope -> ofstream flush + đóng file trước khi rename bên dưới

    if (std::rename(tmpPath.c_str(), path.c_str()) != 0) {
        TraceLog(LOG_WARNING, "Leaderboard: rename '%s' -> '%s' that bai, giu nguyen file cu",
                  tmpPath.c_str(), path.c_str());
    }
}
