#pragma once
#include <cstdint>
#include <string>
#include <cstdio>

// ==========================================
// SAVE CHECKSUM
// leaderboard.dat truoc day la file text thuan tuy "diem wave" - mo bang bat ky text
// editor nao va sua thang con so la du de gian lan. Ham bang FNV-1a 64-bit tron voi 1
// SALT bien dich san vao binary duoc dung de tinh 1 chu ky (checksum) tren toan bo noi
// dung file, ghi kem o dong dau tien; luc doc lai, checksum duoc tinh lai va doi chieu -
// khong khop thi TOAN BO file bi coi la khong dang tin va bi bo qua (bat dau danh sach
// rong) thay vi nap mu quang du lieu co the da bi sua tay.
//
// LUU Y VE PHAM VI: day la 1 CHECKSUM TOAN VEN co ban, KHONG PHAI ma hoa/chu ky so cap
// do bao mat that su - ai doc duoc source code (du la mot du an mo) van tinh lai duoc
// hash dung va gia mao file hop le. Muc tieu duy nhat la chan kieu gian lan RE RE nhat
// (sua thang con so trong file .dat bang tay) ma khong can ha tang server-side, phu hop
// voi quy mo 1 game offline single-player nhu du an nay - khong phai chong reverse-
// engineering hay gian lan tinh vi.
// ==========================================
namespace SaveChecksum {
    constexpr uint64_t SALT = 0x9E3779B97F4A7C15ULL; // So bat ky co dinh - khong can bi mat tuyet doi de co tac dung chan sua tay

    // FNV-1a 64-bit - thuat toan bam nhanh, don gian, du dung cho checksum toan ven
    // (khong phai dung cho muc dich mat ma hoc).
    inline uint64_t Fnv1a64(const std::string& data) {
        uint64_t hash = 0xcbf29ce484222325ULL ^ SALT;
        for (unsigned char c : data) {
            hash ^= c;
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

    inline std::string ToHex(uint64_t value) {
        char buf[17];
        std::snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)value);
        return std::string(buf);
    }
}
