#include "thirdparty/catch.hpp"
#include "ui_nav.h"
#include "ui_layout.h"
#include "settings_menu.h"
#include "leaderboard.h"
#include "game_manager_test_access.h"
#include <cstdio>
#include <fstream>
#include <string>

// ==========================================
// GUI (nang cap GUI) - phan THUAN cua giao dien: dieu huong, doi toa do chuot, hieu ung go chu,
// bang dong Cai dat, bo cuc nut, va 2 thay doi du lieu di kem (settings.cfg moi, ASSIST tren bang
// xep hang). Phan VE khong test duoc - xem anh chup (scripts/capture_showcase.sh).
// ==========================================

namespace {
    const char* SettingsPath() { return "test_ui_settings_tmp.cfg"; }
    const char* BoardPath() { return "test_ui_leaderboard_tmp.dat"; }
    struct CleanupGuard {
        ~CleanupGuard() { std::remove(SettingsPath()); std::remove(BoardPath()); }
    };
    bool Overlaps(Rectangle a, Rectangle b) {
        return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
    }
    bool Inside(Rectangle inner, Rectangle outer) {
        return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.width <= outer.x + outer.width + 0.01f
               && inner.y + inner.height <= outer.y + outer.height + 0.01f;
    }
    const Rectangle kScreen{ 0.0f, 0.0f, (float)Config::SCREEN_W, (float)Config::SCREEN_H };
}

TEST_CASE("UiNav::Wrap vong quanh 2 dau danh sach", "[ui]") {
    REQUIRE(UiNav::Wrap(0, -1, 6) == 5);
    REQUIRE(UiNav::Wrap(5, 1, 6) == 0);
    REQUIRE(UiNav::Wrap(2, 1, 6) == 3);
    REQUIRE(UiNav::Wrap(3, 0, 0) == 0); // Danh sach rong khong chia cho 0
}

TEST_CASE("UiNav::ScreenToVirtual khop phep upscale letterbox cua Run()", "[ui][mouse]") {
    // Cua so dung kich thuoc canvas -> giu nguyen
    UiNav::VirtualPoint a = UiNav::ScreenToVirtual({ 400.0f, 300.0f }, 800.0f, 600.0f, 800.0f, 600.0f);
    REQUIRE(a.pos.x == Approx(400.0f));
    REQUIRE(a.inside);
    // 1920x1080: scale = 1.8, vien den 2 ben = (1920 - 1440)/2 = 240px
    UiNav::VirtualPoint b = UiNav::ScreenToVirtual({ 240.0f + 1.8f * 100.0f, 1.8f * 50.0f }, 1920.0f, 1080.0f, 800.0f, 600.0f);
    REQUIRE(b.pos.x == Approx(100.0f));
    REQUIRE(b.pos.y == Approx(50.0f));
    // Chuot tren vien den -> ngoai canvas (khong duoc bam trung nut nao)
    UiNav::VirtualPoint c = UiNav::ScreenToVirtual({ 100.0f, 500.0f }, 1920.0f, 1080.0f, 800.0f, 600.0f);
    REQUIRE_FALSE(c.inside);
}

TEST_CASE("UiNav::TypewriterBytes khong bao gio cat giua 1 chu tieng Viet nhieu byte", "[ui][loc]") {
    const std::string s = "Việt Nam"; // 'ệ' = 3 byte
    // Sau 2 ky tu ("Vi") -> 2 byte; sau 3 ky tu ("Việ") -> 5 byte, khong phai 3
    REQUIRE(UiNav::TypewriterBytes(s, 2.0f, 1.0f) == 2);
    REQUIRE(UiNav::TypewriterBytes(s, 3.0f, 1.0f) == 5);
    REQUIRE(UiNav::TypewriterBytes(s, 100.0f, 1.0f) == s.size());
    REQUIRE(UiNav::TypewriterBytes(s, 0.0f, 1.0f) == 0);
    REQUIRE(UiNav::TypewriterChars(s, 3.0f, 1.0f) == 3);
    REQUIRE(UiNav::TypewriterChars(s, 100.0f, 1.0f) == 8);
}

TEST_CASE("UiNav::BlinkOn: giam nhap nhay -> luon sang; tan so bi kep <= 2Hz", "[ui][a11y]") {
    for (float t = 0.0f; t < 3.0f; t += 0.07f) REQUIRE(UiNav::BlinkOn(t, 1.0f, true));
    int toggles = 0;
    bool prev = UiNav::BlinkOn(0.0f, 50.0f, false);
    for (float t = 0.0f; t < 1.0f; t += 0.001f) {
        bool v = UiNav::BlinkOn(t, 50.0f, false);
        if (v != prev) toggles++;
        prev = v;
    }
    REQUIRE(toggles <= 4); // 2 chu ky/giay = toi da 2 lan sang - duoi nguong 3 lan/giay cua WCAG 2.3.1
}

TEST_CASE("UiNav::SliderValueAt kep 0..1 va lam tron ve buoc", "[ui]") {
    Rectangle bar{ 100.0f, 0.0f, 200.0f, 10.0f };
    REQUIRE(UiNav::SliderValueAt(50.0f, bar, 0.05f) == Approx(0.0f));
    REQUIRE(UiNav::SliderValueAt(400.0f, bar, 0.05f) == Approx(1.0f));
    REQUIRE(UiNav::SliderValueAt(201.0f, bar, 0.05f) == Approx(0.5f));
}

TEST_CASE("UiLayout: nut khong chong len nhau va nam trong man hinh", "[ui][layout]") {
    for (int i = 0; i < UiLayout::MAIN_MENU_ITEMS; i++) {
        REQUIRE(Inside(UiLayout::MainMenuItem(i), kScreen));
        if (i > 0) REQUIRE_FALSE(Overlaps(UiLayout::MainMenuItem(i - 1), UiLayout::MainMenuItem(i)));
    }
    for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
        REQUIRE(Inside(UiLayout::SettingsTab(i), kScreen));
        if (i > 0) REQUIRE_FALSE(Overlaps(UiLayout::SettingsTab(i - 1), UiLayout::SettingsTab(i)));
    }
    for (int r = 0; r < SETTINGS_MAX_ROWS; r++) {
        REQUIRE(Inside(UiLayout::SettingsControl(r), UiLayout::SettingsRow(r)));
        REQUIRE_FALSE(Overlaps(UiLayout::SettingsRow(r), UiLayout::SettingsDescPanel()));
        for (int n = 2; n <= 4; n++) {
            for (int i = 0; i < n; i++) {
                REQUIRE(Inside(UiLayout::SettingsOption(r, i, n), UiLayout::SettingsControl(r)));
                if (i > 0) REQUIRE_FALSE(Overlaps(UiLayout::SettingsOption(r, i - 1, n), UiLayout::SettingsOption(r, i, n)));
            }
        }
    }
    for (int i = 0; i < UiLayout::PAUSE_ITEMS; i++) REQUIRE(Inside(UiLayout::PauseItem(i), UiLayout::PausePanel()));
    REQUIRE(Inside(UiLayout::ConfirmButton(0), UiLayout::ConfirmPanel()));
    REQUIRE(Inside(UiLayout::ConfirmButton(1), UiLayout::ConfirmPanel()));
    REQUIRE_FALSE(Overlaps(UiLayout::ConfirmButton(0), UiLayout::ConfirmButton(1)));
    for (int i = 0; i < 3; i++) {
        REQUIRE(Inside(UiLayout::HangarLoadoutCard(i), kScreen));
        REQUIRE(Inside(UiLayout::UpgradeCard(i), kScreen));
        if (i > 0) REQUIRE_FALSE(Overlaps(UiLayout::HangarDifficultyPill(i - 1), UiLayout::HangarDifficultyPill(i)));
    }
    // Nut QUAY LAI khong de len tieu de/tab
    REQUIRE_FALSE(Overlaps(UiLayout::BackButton(), UiLayout::SettingsTab(0)));
}

TEST_CASE("Cai dat: moi tab co dong, moi dong Choice co nhan doc duoc o ca 2 ngon ngu", "[ui][settings]") {
    for (int t = 0; t < SETTINGS_TAB_COUNT; t++) {
        int count = 0;
        const SettingRowDef* rows = SettingsTabRows((SettingsTab)t, count);
        REQUIRE(count > 0);
        REQUIRE(count <= SETTINGS_MAX_ROWS);
        for (int r = 0; r < count; r++) {
            const SettingRowDef& d = rows[r];
            if (d.kind == SettingKind::Choice) {
                REQUIRE(ChoiceCount(d.id) >= 2);
                for (Language lang : { Language::EN, Language::VI }) {
                    Loc::SetLanguage(lang);
                    for (int i = 0; i < ChoiceCount(d.id); i++) REQUIRE_FALSE(ChoiceLabel(d.id, i).empty());
                }
                Loc::SetLanguage(Language::EN);
            }
            if (d.kind == SettingKind::Key) REQUIRE(KeyField(d.id) != nullptr);
        }
    }
}

TEST_CASE("Cai dat: Trai/Phai tren dong Choice quay vong, tren thanh truot kep 0..1", "[ui][settings]") {
    Settings s;
    s.graphics.quality = GraphicsQuality::High;
    REQUIRE(AdjustSetting(s, SettingId::Quality, 1));
    REQUIRE(s.graphics.quality == GraphicsQuality::Low); // Quay vong

    REQUIRE(AdjustSetting(s, SettingId::Language, 1));
    REQUIRE(s.language == Language::VI);

    s.volume = 1.0f;
    REQUIRE_FALSE(AdjustSetting(s, SettingId::MasterVolume, 1)); // Da toi tran -> khong doi, khong bao "da doi"
    REQUIRE(s.volume == Approx(1.0f));
    REQUIRE(AdjustSetting(s, SettingId::MusicVolume, -1));
    REQUIRE(s.musicVolume == Approx(0.95f));

    REQUIRE(AdjustSetting(s, SettingId::GameSpeed, 1));
    REQUIRE(s.gameSpeedPercent == 90);
    REQUIRE(s.IsAssisted());

    REQUIRE_FALSE(AdjustSetting(s, SettingId::ResetKeys, 1)); // Action/Key khong doi bang Trai/Phai
}

TEST_CASE("Cai dat: dong bat/tat - Phai la BAT", "[ui][settings]") {
    Settings s;
    s.autoFire = false;
    SetChoice(s, SettingId::AutoFire, 1);
    REQUIRE(s.autoFire);
    REQUIRE(ChoiceIndex(s, SettingId::AutoFire) == 1);
    SetChoice(s, SettingId::Hitbox, 99); // Ngoai pham vi -> kep, khong UB
    REQUIRE(s.showHitbox);
}

TEST_CASE("Settings: cac field GUI moi round-trip qua settings.cfg", "[ui][settings]") {
    CleanupGuard guard;
    Settings a;
    a.language = Language::VI;
    a.musicVolume = 0.3f;
    a.sfxVolume = 0.7f;
    a.uiSounds = false;
    a.showFps = true;
    a.autoFire = true;
    a.gameSpeedPercent = 80;
    a.showHitbox = true;
    a.graphics.colorFilter = ColorFilter::Deutan;
    a.SaveToFile(SettingsPath());

    Settings b = Settings::LoadFromFile(SettingsPath());
    REQUIRE(b.language == Language::VI);
    REQUIRE(b.musicVolume == Approx(0.3f));
    REQUIRE(b.sfxVolume == Approx(0.7f));
    REQUIRE_FALSE(b.uiSounds);
    REQUIRE(b.showFps);
    REQUIRE(b.autoFire);
    REQUIRE(b.gameSpeedPercent == 80);
    REQUIRE(b.showHitbox);
    REQUIRE(b.graphics.colorFilter == ColorFilter::Deutan);
}

TEST_CASE("Settings: file CHUA co LANGUAGE -> dung ngon ngu mac dinh truyen vao (he dieu hanh)", "[ui][settings][loc]") {
    CleanupGuard guard;
    { std::ofstream f(SettingsPath()); f << "VOLUME=0.5\n"; }
    REQUIRE(Settings::LoadFromFile(SettingsPath(), Language::VI).language == Language::VI);
    REQUIRE(Settings::LoadFromFile(SettingsPath(), Language::EN).language == Language::EN);
    // Co LANGUAGE thi file thang - lua chon cua nguoi choi luon hon he dieu hanh
    { std::ofstream f(SettingsPath()); f << "LANGUAGE=EN\n"; }
    REQUIRE(Settings::LoadFromFile(SettingsPath(), Language::VI).language == Language::EN);
}

TEST_CASE("Settings: GAME_SPEED / MUSIC_VOLUME sua tay sai -> gia tri an toan", "[ui][settings]") {
    CleanupGuard guard;
    { std::ofstream f(SettingsPath()); f << "GAME_SPEED=5\nMUSIC_VOLUME=nan\nSFX_VOLUME=7\n"; }
    Settings s = Settings::LoadFromFile(SettingsPath());
    REQUIRE(s.gameSpeedPercent == 100); // Khong doan y "rat cham" - ve khong ho tro
    REQUIRE(s.musicVolume == Approx(1.0f));
    REQUIRE(s.sfxVolume == Approx(1.0f)); // Kep ve 1
}

TEST_CASE("Leaderboard: co ASSIST round-trip, file cu 2 cot van doc duoc", "[ui][leaderboard]") {
    CleanupGuard guard;
    {
        Leaderboard lb;
        lb.Load(BoardPath());
        lb.TrySubmit(500, 3, true);
        lb.TrySubmit(900, 4, false);
    }
    Leaderboard back;
    back.Load(BoardPath());
    REQUIRE(back.GetEntries().size() == 2);
    REQUIRE(back.GetEntries()[0].score == 900);
    REQUIRE_FALSE(back.GetEntries()[0].assisted);
    REQUIRE(back.GetEntries()[1].assisted);
}

TEST_CASE("Leaderboard: trong 1 van, da ASSIST 1 lan thi dong cua van giu nhan du lan nop sau khong assist", "[ui][leaderboard]") {
    CleanupGuard guard;
    Leaderboard lb;
    lb.Load(BoardPath());
    lb.BeginRun();
    lb.TrySubmit(300, 2, true);
    lb.TrySubmit(800, 3, false);
    REQUIRE(lb.GetEntries().size() == 1);
    REQUIRE(lb.GetEntries()[0].score == 800);
    REQUIRE(lb.GetEntries()[0].assisted);
}

TEST_CASE("GameManager: toc do game < 100% khi bat dau van -> van mang nhan ASSIST", "[ui][game_manager]") {
    using GTA = GameManagerTestAccess;
    GameManager gm;
    GTA::SettingsRef(gm).gameSpeedPercent = 100;
    GTA::CallInitLevel(gm, true);
    REQUIRE_FALSE(GTA::RunAssisted(gm));
    GTA::SettingsRef(gm).gameSpeedPercent = 80;
    GTA::CallInitLevel(gm, true);
    REQUIRE(GTA::RunAssisted(gm));
}

TEST_CASE("GameManager: dong mo ta dang go doi theo muc menu va ngon ngu", "[ui][game_manager][loc]") {
    using GTA = GameManagerTestAccess;
    GameManager gm;
    GTA::CallGoToScreen(gm, GameState::MENU);
    GTA::Ui(gm).menuIndex = 4;
    Loc::SetLanguage(Language::EN);
    REQUIRE(std::string(GTA::CallUiTypedLine(gm)) == Tr(Str::MenuDescSettings));
    GTA::SettingsRef(gm).language = Language::VI;
    GTA::CallApplySettings(gm); // Headless: khong co cua so -> bo qua fullscreen, chi doi ngon ngu/am luong
    REQUIRE(std::string(GTA::CallUiTypedLine(gm)) == Loc::Get(Str::MenuDescSettings, Language::VI));
    Loc::SetLanguage(Language::EN);
}

TEST_CASE("GameManager: GoToScreen reset dong ho glitch/go chu va dong hop xac nhan", "[ui][game_manager]") {
    using GTA = GameManagerTestAccess;
    GameManager gm;
    GTA::Ui(gm).screenTimer = 9.0f;
    GTA::Ui(gm).rowTimer = 9.0f;
    GTA::Ui(gm).confirm = ConfirmKind::QuitGame;
    GTA::CallGoToScreen(gm, GameState::SETTINGS);
    REQUIRE(GTA::State(gm) == GameState::SETTINGS);
    REQUIRE(GTA::Ui(gm).screenTimer == 0.0f);
    REQUIRE(GTA::Ui(gm).rowTimer == 0.0f);
    REQUIRE(GTA::Ui(gm).confirm == ConfirmKind::None);
}
