#pragma once
#include <string>

// ==========================================
// ATOMIC FILE REPLACE - buoc cuoi cua mau "ghi ra <path>.tmp roi doi ten de len <path>" ma
// Leaderboard/MetaProgress/Settings cung dung (xem CLAUDE.md muc "Ghi file atomic").
//
// TAI SAO KHONG GOI THANG std::rename(): tren POSIX, rename(2) thay the file dich mot cach
// atomic - dung y muon. Nhung tren Windows (CRT cua MSVC lan MinGW), std::rename() THAT BAI
// (EEXIST/EACCES) khi file dich DA TON TAI. Hau qua neu goi thang: lan luu DAU TIEN thanh
// cong (chua co file), moi lan luu SAU do deu that bai im lang - leaderboard/currency/
// settings tren Windows khong bao gio cap nhat duoc nua sau lan dau. Comment cu o 3 cho
// goi ghi "rename() atomic tren ca POSIX lan Windows" - sai voi chinh std::rename().
//
// Tach .cpp rieng (khong header-only) vi nhanh Windows can <windows.h>, ma header do xung
// dot ten voi raylib.h (Rectangle, CloseWindow, DrawText...) - khong duoc de no lot vao bat
// ky file nao include raylib.
// ==========================================
namespace AtomicFile {
    // Thay the `dst` bang `tmp` (tmp bien mat). Tra ve false neu that bai - khi do `dst` cu
    // con nguyen ven.
    bool Replace(const std::string& tmp, const std::string& dst);
}
