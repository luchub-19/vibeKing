#include "localization.h"
#include <cstdlib>
#include <algorithm>

namespace {
    // Sinh tu CUNG danh sach LOC_STRINGS voi enum Str (localization.h) -> thu tu luon khop.
    struct Entry { const char* text[LANGUAGE_COUNT]; };
    const Entry kTable[STR_COUNT] = {
#define LOC_ROW(id, en, vi) { { en, vi } },
        LOC_STRINGS(LOC_ROW)
#undef LOC_ROW
    };

    Language g_language = Language::EN;

    bool StartsWithVi(const char* s) {
        return (s[0] == 'v' || s[0] == 'V') && (s[1] == 'i' || s[1] == 'I') && (s[2] == '\0' || s[2] == '_' || s[2] == '-' || s[2] == '.');
    }

    // Chu cai tieng Viet co dau (thuong + hoa) ngoai ASCII. Ky hieu bo sung (×, •, …) co trong
    // ca 2 font - de danh cho chuoi dong (khong nam trong bang dich).
    constexpr const char* kVietnameseLetters =
        "ăâđêôơưàảãáạằẳẵắặầẩẫấậèẻẽéẹềểễếệìỉĩíịòỏõóọồổỗốộờởỡớợùủũúụừửữứựỳỷỹýỵ"
        "ĂÂĐÊÔƠƯÀẢÃÁẠẰẲẴẮẶẦẨẪẤẬÈẺẼÉẸỀỂỄẾỆÌỈĨÍỊÒỎÕÓỌỒỔỖỐỘỜỞỠỚỢÙỦŨÚỤỪỬỮỨỰỲỶỸÝỴ"
        "×•…";
}

namespace Loc {
    void SetLanguage(Language lang) { g_language = lang; }
    Language GetLanguage() { return g_language; }

    const char* Get(Str id, Language lang) {
        int i = (int)id;
        int l = (int)lang;
        if (i < 0 || i >= STR_COUNT) return "";
        if (l < 0 || l >= LANGUAGE_COUNT) l = 0;
        return kTable[i].text[l];
    }

    const char* LanguageCode(Language lang) { return lang == Language::VI ? "VI" : "EN"; }
    const char* LanguageDisplayName(Language lang) { return lang == Language::VI ? "TIẾNG VIỆT" : "ENGLISH"; }

    Language LanguageFromCode(std::string_view code, Language fallback) {
        auto eq = [&](const char* c) {
            if (code.size() != 2) return false;
            return (code[0] | 0x20) == (c[0] | 0x20) && (code[1] | 0x20) == (c[1] | 0x20);
        };
        if (eq("EN")) return Language::EN;
        if (eq("VI")) return Language::VI;
        return fallback;
    }

    Language DetectFromEnv(const char* lcAll, const char* lcMessages, const char* lang) {
        // Bien KHAC RONG dau tien quyet dinh - dung thu tu uu tien cua POSIX setlocale().
        for (const char* v : { lcAll, lcMessages, lang }) {
            if (v == nullptr || v[0] == '\0') continue;
            return StartsWithVi(v) ? Language::VI : Language::EN;
        }
        return Language::EN;
    }

    Language DetectSystemLanguage() {
        return DetectFromEnv(std::getenv("LC_ALL"), std::getenv("LC_MESSAGES"), std::getenv("LANG"));
    }

    std::vector<int> DecodeUtf8(std::string_view s) {
        std::vector<int> out;
        out.reserve(s.size());
        size_t i = 0;
        while (i < s.size()) {
            unsigned char c = (unsigned char)s[i];
            int cp = 0xFFFD;
            size_t len = 1;
            if (c < 0x80) { cp = c; }
            else if ((c >> 5) == 0x6 && i + 1 < s.size()) { cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F); len = 2; }
            else if ((c >> 4) == 0xE && i + 2 < s.size()) { cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F); len = 3; }
            else if ((c >> 3) == 0x1E && i + 3 < s.size()) {
                cp = ((c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F); len = 4;
            }
            out.push_back(cp);
            i += len;
        }
        return out;
    }

    const std::vector<int>& FontCharset() {
        static const std::vector<int> charset = [] {
            std::vector<int> cps;
            for (int c = 32; c <= 126; c++) cps.push_back(c);
            for (int cp : DecodeUtf8(kVietnameseLetters)) cps.push_back(cp);
            std::sort(cps.begin(), cps.end());
            cps.erase(std::unique(cps.begin(), cps.end()), cps.end());
            return cps;
        }();
        return charset;
    }
}
