#pragma once

// ==========================================
// LOCALIZATION (A3)
// 12 chuoi UI truoc day nam RAI RAC lam literal ngay tai noi goi TextFormat()/
// canvas.Text() trong render_system.cpp + trong GetRebindableActions() (game_manager.h),
// toan bo van con nguyen tieng Viet dua tren quy uoc "khong dau" cua phan con lai trong
// game - vua kho doi dong loat (phai grep qua nhieu file), vua LECH NGON NGU voi phan UI
// da la tieng Anh tu truoc (DIFFICULTY/LOADOUT/VOLUME/PAUSED/GAME OVER...).
//
// QUYET DINH: tieng Anh la ngon ngu MAC DINH cua game - gom toan bo 12 chuoi ve 1
// namespace duy nhat va dich sang tieng Anh, dong bo voi phan UI da co san. Gom ve 1 cho
// con de: sua 1 noi la doi duoc CA game, va neu sau nay can them ngon ngu thu 2 (vi du
// tieng Viet cho nguoi choi muon) thi chi file nay can doi - moi call site trong
// render_system.cpp/game_manager.h khong doi gi ca.
//
// LUU Y FONT: gameFont hien tai nap qua LoadFontEx() KHONG truyen bang codepoint rieng
// (xem GameManager::Run()) -> chi co bang glyph ASCII mac dinh cua raylib. Tieng Anh
// thuan ASCII nen khong bi anh huong - nhung neu sau nay them ban dich co dau (vi du
// tieng Viet co dau, hoac ngon ngu khac ASCII), phai nap them bang codepoint tuong ung
// truoc, neu khong se hien thanh o trong (tofu) hoac mat chu.
// ==========================================
namespace Loc {
    // --- Menu (RenderSystem::DrawMenu / DrawLoadoutSelect) ---
    constexpr const char* NoRecordsYet = "(no records yet)";
    // %d/%d = currency dang co / chi phi mo khoa - khong them verb ("unlock:") de dong
    // bo voi 2 trang thai con lai cua cung 1 cho (FREE/UNLOCKED) cung khong co verb.
    constexpr const char* UnlockCostFmt = "%d/%d currency";
    // Dung 1 cum voi PausedControlsHint ben duoi (cung 1 tinh nang F11, 2 man hinh khac
    // nhau) - tranh lap lai loi truoc day (chi 1 trong 2 cho duoc dich, 2 man hinh hien
    // 2 ngon ngu khac nhau cho CUNG 1 nut).
    constexpr const char* MenuFullscreenHint = "F11: FULLSCREEN";

    // --- End screen (RenderSystem::DrawEndScreen) ---
    constexpr const char* NewRecordBanner = "NEW RECORD! (#1)";
    // --- Bang tong ket run (man hinh GAME OVER) ---
    constexpr const char* RunSummaryTitle   = "RUN SUMMARY";
    constexpr const char* RunSummaryScore   = "SCORE";
    constexpr const char* RunSummaryWave    = "WAVE REACHED";
    constexpr const char* RunSummaryKills   = "ENEMIES DESTROYED";
    constexpr const char* RunSummaryCombo   = "BEST COMBO";
    constexpr const char* RunSummaryEarned  = "CURRENCY EARNED";
    constexpr const char* RunSummaryTotal   = "TOTAL";
    // %s = ten loadout ke tiep, %d/%d = currency dang co / chi phi mo khoa
    constexpr const char* NextUnlockFmt     = "NEXT: %s  %d/%d CR";
    constexpr const char* AllUnlocked       = "ALL LOADOUTS UNLOCKED";
    constexpr const char* MadeTop10Banner = "TOP 10!";

    // --- Upgrade select (RenderSystem::DrawEndScreen, state WAVE_CLEAR - Track C Nguoi 2
    // Phase 3). So lieu trong *Desc la khoi diem hien tai (dong bo tay voi Config::UPGRADE_*
    // trong config.h/balance.json - xem upgrade_types.h) - can chinh can bang thi sua CA 2
    // cho, day chi la nhan hien thi, khong parse nguoc tu chuoi. ---
    constexpr const char* UpgradeMoveSpeedName = "MOVE SPEED";
    constexpr const char* UpgradeMoveSpeedDesc = "+8% speed per pick";
    constexpr const char* UpgradeExtraLifeName = "EXTRA LIFE";
    constexpr const char* UpgradeExtraLifeDesc = "+1 life (max 5)";
    constexpr const char* UpgradeBonusScoreName = "BONUS SCORE";
    constexpr const char* UpgradeBonusScoreDesc = "+1000 points";
    constexpr const char* UpgradeSelectHint = "LEFT/RIGHT: SELECT   ENTER: CONFIRM   R: RESTART";
    constexpr const char* BossWaveUpgradeBanner = "BOSS WAVE NEXT - PICK APPLIES x2!";

    // --- Paused overlay (RenderSystem::DrawPlaying) ---
    constexpr const char* PausedControlsHint = "P / ESC: RESUME   F11: FULLSCREEN   K: REBIND KEYS";

    // --- Keybind screen (RenderSystem::DrawPlaying, state KEYBIND) ---
    constexpr const char* KeybindTitle = "REBIND KEYS";
    constexpr const char* RebindPromptFmt = "Press new key for '%s'... (ESC to cancel)";
    constexpr const char* KeybindHelp = "Press 1-4 to rebind a key.  0 or R: reset to default.  ESC: back";

    // --- HUD (RenderSystem::DrawHUD) ---
    constexpr const char* ShieldTag = "SHIELD!";
    constexpr const char* BossIncomingHint = "INCOMING";

    // --- GetRebindableActions() (game_manager.h) ---
    // 3/4 nhan hanh dong REBIND thuoc pham vi dich cua A3 ("Pause" giu nguyen literal
    // tai game_manager.h theo dung yeu cau - chi cham DUNG 3 string literal, khong doi
    // shape struct RebindableAction).
    constexpr const char* ActionMoveLeft = "LEFT";
    constexpr const char* ActionMoveRight = "RIGHT";
    constexpr const char* ActionShoot = "SHOOT";
}
