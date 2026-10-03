#include "thirdparty/catch.hpp"
#include "localization.h"
#include <algorithm>
#include <string>
#include <vector>

// ==========================================
// DA NGON NGU - khoa 3 luat cua bang chuoi (xem localization.h). Ca 3 deu la loi AM THAM o luc
// chay (khong crash, khong log): thieu ban dich -> chuoi rong; lech %d/%s -> TextFormat doc sai
// kieu tham so; ky tu ngoai bang ma font -> o trong. Chi test moi bat duoc truoc khi nguoi choi thay.
// ==========================================

namespace {
    // Rut chuoi dac ta printf ("%d", "%02d", "%s"...) theo thu tu. Co y KHONG nhan co ' ' (space):
    // "+8% speed" la chu hien thi thuong, khong phai dac ta "% s".
    std::vector<std::string> FormatSpecs(const std::string& s) {
        std::vector<std::string> out;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] != '%') continue;
            size_t j = i + 1;
            if (j < s.size() && s[j] == '%') { i = j; continue; }
            while (j < s.size() && (s[j] == '-' || s[j] == '+' || s[j] == '0' || s[j] == '#')) j++;
            while (j < s.size() && (s[j] >= '0' && s[j] <= '9')) j++;
            if (j < s.size() && std::string("dsifuxc").find(s[j]) != std::string::npos) {
                out.push_back(s.substr(i, j - i + 1));
                i = j;
            }
        }
        return out;
    }

    struct LanguageGuard { // Moi test tu dat ngon ngu - tra lai EN de khong ro ri sang test khac
        ~LanguageGuard() { Loc::SetLanguage(Language::EN); }
    };
}

TEST_CASE("Loc: moi chuoi co ban dich KHAC RONG o ca 2 ngon ngu", "[loc]") {
    for (int i = 0; i < STR_COUNT; i++) {
        INFO("Str index " << i);
        REQUIRE(std::string(Loc::Get((Str)i, Language::EN)).size() > 0);
        REQUIRE(std::string(Loc::Get((Str)i, Language::VI)).size() > 0);
    }
}

TEST_CASE("Loc: chuoi dinh dang co CUNG dac ta printf o ca 2 ngon ngu", "[loc]") {
    for (int i = 0; i < STR_COUNT; i++) {
        std::string en = Loc::Get((Str)i, Language::EN), vi = Loc::Get((Str)i, Language::VI);
        INFO("EN: " << en << " | VI: " << vi);
        REQUIRE(FormatSpecs(en) == FormatSpecs(vi));
    }
    // Tu kiem bo tach: phai thay dac ta that, va KHONG coi "+8% speed" la dac ta
    REQUIRE(FormatSpecs("WAVE %02d %s") == std::vector<std::string>{ "%02d", "%s" });
    REQUIRE(FormatSpecs("+8% speed").empty());
    REQUIRE(FormatSpecs("%d%%").size() == 1);
}

TEST_CASE("Loc: moi ky tu trong bang dich deu nam trong bang ma font (khong o trong/tofu)", "[loc][font]") {
    const std::vector<int>& cs = Loc::FontCharset();
    REQUIRE(std::is_sorted(cs.begin(), cs.end()));
    for (int i = 0; i < STR_COUNT; i++) {
        for (Language lang : { Language::EN, Language::VI }) {
            for (int cp : Loc::DecodeUtf8(Loc::Get((Str)i, lang))) {
                INFO("Str " << i << " codepoint U+" << std::hex << cp);
                REQUIRE(std::binary_search(cs.begin(), cs.end(), cp));
            }
        }
    }
    // Ten ngon ngu (hien trong Cai dat) cung phai ve duoc
    for (int cp : Loc::DecodeUtf8(Loc::LanguageDisplayName(Language::VI))) REQUIRE(std::binary_search(cs.begin(), cs.end(), cp));
}

TEST_CASE("Loc: bang ma co du 134 chu cai tieng Viet co dau + ASCII", "[loc][font]") {
    const std::vector<int>& cs = Loc::FontCharset();
    REQUIRE(cs.size() >= 95 + 134);
    for (int cp : Loc::DecodeUtf8("ỆỷđĐơƯẫ")) REQUIRE(std::binary_search(cs.begin(), cs.end(), cp));
}

TEST_CASE("Loc: Tr() doi theo ngon ngu hien hanh, doi ngay khong can nap lai", "[loc]") {
    LanguageGuard guard;
    Loc::SetLanguage(Language::EN);
    REQUIRE(std::string(Tr(Str::MenuSettings)) == "SETTINGS");
    Loc::SetLanguage(Language::VI);
    REQUIRE(std::string(Tr(Str::MenuSettings)) == "CÀI ĐẶT");
}

TEST_CASE("Loc: ma ngon ngu trong file cau hinh - khong phan biet hoa thuong, sai -> fallback", "[loc]") {
    REQUIRE(Loc::LanguageFromCode("vi", Language::EN) == Language::VI);
    REQUIRE(Loc::LanguageFromCode("EN", Language::VI) == Language::EN);
    REQUIRE(Loc::LanguageFromCode("fr", Language::VI) == Language::VI);
    REQUIRE(Loc::LanguageFromCode("", Language::EN) == Language::EN);
    REQUIRE(std::string(Loc::LanguageCode(Language::VI)) == "VI");
}

TEST_CASE("Loc: ngon ngu he dieu hanh theo thu tu uu tien POSIX LC_ALL > LC_MESSAGES > LANG", "[loc]") {
    REQUIRE(Loc::DetectFromEnv(nullptr, nullptr, "vi_VN.UTF-8") == Language::VI);
    REQUIRE(Loc::DetectFromEnv(nullptr, nullptr, "en_US.UTF-8") == Language::EN);
    REQUIRE(Loc::DetectFromEnv("en_US.UTF-8", nullptr, "vi_VN.UTF-8") == Language::EN); // LC_ALL thang
    REQUIRE(Loc::DetectFromEnv("", "vi", "en_US") == Language::VI);                     // Rong = bo qua
    REQUIRE(Loc::DetectFromEnv(nullptr, nullptr, nullptr) == Language::EN);
    REQUIRE(Loc::DetectFromEnv(nullptr, nullptr, "vietnamese") == Language::EN);         // Chi "vi" + phan cach moi tinh
    REQUIRE(Loc::DetectFromEnv(nullptr, nullptr, "C") == Language::EN);
}

TEST_CASE("Loc: DecodeUtf8 dem KY TU, khong dem byte", "[loc]") {
    REQUIRE(Loc::DecodeUtf8("Việt").size() == 4);
    REQUIRE(Loc::DecodeUtf8("ABC").size() == 3);
    REQUIRE(Loc::DecodeUtf8("Đ")[0] == 0x0110);
}
