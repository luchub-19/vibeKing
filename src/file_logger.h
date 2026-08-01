#pragma once
#include <string>

// ==========================================
// FILE LOGGER (rotating, theo muc do INFO/WARN/ERROR)
// TraceLog(...) cua raylib truoc day chi in ra console/stdout - vo tac dung khi game da
// dong goi chay tren may nguoi dung cuoi (khong ai mo terminal de xem). FileLogger dang
// ky lam CALLBACK cua chinh raylib (SetTraceLogCallback) NGAY LUC Init() - nghia la MOI
// loi goi TraceLog(...) da co san khap codebase (Leaderboard, Settings, LevelGridConfig,
// Sprites, fallback font...) TU DONG duoc ghi ra file, khong phai sua lai tung noi goi
// rieng le sang 1 API logging moi.
//
// XOAY VONG (rotation): file dang ghi (logs/game.log) vuot qua nguong dung luong thi bi
// day thanh game.1.log, game.1.log cu thanh game.2.log... toi da giu MAX_BACKUPS ban,
// ban cu nhat bi xoa - dam bao log khong phinh vo han theo thoi gian may nguoi dung
// choi lien tuc nhieu ngay.
//
// Moi dong ghi deu fflush() NGAY (khong doi buffer day) - neu game crash ngay sau do,
// dong log cuoi cung van nam tren dia thay vi mat theo buffer trong RAM chua kip xa.
// ==========================================
namespace FileLogger {
    // Goi 1 LAN DUY NHAT, cang som cang tot trong main loop (ly tuong la truoc
    // InitWindow, de bat luon ca log khoi tao cua chinh raylib).
    void Init(const std::string& logDirectory = "logs");
    void Shutdown();
}
