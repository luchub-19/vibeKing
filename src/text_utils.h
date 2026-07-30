#pragma once
#include <string_view>
#include <cctype>

// Ham tien ich dung chung cho cac parser file cau hinh dang KEY=VALUE (level_config.cpp,
// settings.cpp). TRUOC: Trim() nhan/tra ve std::string -> moi lan goi la 1 lan cap phat +
// copy toan bo noi dung, du chi de cat khoang trang o 2 dau. GIO: lam viec tren
// std::string_view - CHI la 1 cap (con tro, do dai) TRO THANG vao buffer goc, khong copy
// byte nao ca. Caller chi convert sang std::string (neu thuc su can, vd de luu vao map)
// dung 1 LAN DUY NHAT tai diem can, thay vi Trim() tu no da copy roi caller lai copy tiep.
namespace TextUtils {
    inline std::string_view Trim(std::string_view s) {
        size_t begin = s.find_first_not_of(" \t\r\n");
        if (begin == std::string_view::npos) return {};
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(begin, end - begin + 1);
    }

    // So sanh khong phan biet hoa/thuong MA KHONG can uppercase-copy ca 2 chuoi ra
    // std::string moi (nhu cach lam cu voi std::transform + std::string tam) - so tung
    // ky tu truc tiep tren view goc.
    inline bool IEquals(std::string_view a, std::string_view b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); i++) {
            if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
        }
        return true;
    }
}
