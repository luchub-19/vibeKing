#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// ==========================================
// DA NGON NGU (EN / VI) - doi ngay luc dang chay, khong can khoi dong lai.
//
// LICH SU: truoc day (A3) day la 1 namespace `constexpr const char*` tieng Anh duy nhat - gom
// chuoi ve 1 cho de "sau nay can them ngon ngu thi chi file nay phai doi". Gio ngon ngu thu 2
// da den, nhung hang `constexpr` khong doi duoc luc chay -> doi sang BANG CHUOI + ham Tr().
//
// 1-NGUON-DUY-NHAT (X-macro): MOI chuoi la 1 dong X(Id, "english", "tieng viet") trong
// LOC_STRINGS ben duoi - enum Str VA bang dich trong localization.cpp deu sinh tu CHINH danh
// sach nay, nen khong the co chuoi thieu ban dich hay 2 bang lech thu tu nhau. Them chuoi moi
// = them 1 dong o day, khong dong vao file nao khac.
//
// 3 LUAT (co test khoa trong tests/test_localization.cpp):
//   1. Chuoi co dinh dang (%d/%s) phai co CUNG chuoi dac ta o ca 2 ngon ngu - lech 1 cai la
//      TextFormat doc sai kieu tham so (UB), khong phai chi hien sai chu.
//   2. Moi ky tu trong moi chuoi phai nam trong FontCharset() - font nap qua LoadFontEx voi
//      DUNG bang ma nay; ky tu ngoai bang hien thanh o trong (tofu) AM THAM, raylib khong bao.
//   3. Khong ky hieu ngoai Latin/tieng Viet (mui ten, tam giac...) trong chuoi: VT323 khong co
//      glyph do. Mui ten/con tro ve bang hinh (xem retro_ui.h).
//
// FONT: ca 2 font (VT323 chu thuong, Bungee tieu de) deu du 134 chu cai tieng Viet co dau -
// da kiem bang fontTools truoc khi chon (xem docs/GUI_UPGRADE.md).
// ==========================================

enum class Language : uint8_t { EN, VI };
constexpr int LANGUAGE_COUNT = 2;

#define LOC_STRINGS(X) \
    /* --- Chung --- */ \
    X(On,  "ON",  "BẬT") \
    X(Off, "OFF", "TẮT") \
    X(Yes, "YES", "CÓ") \
    X(No,  "NO",  "KHÔNG") \
    X(Back, "BACK", "QUAY LẠI") \
    X(HintNavigate, "UP/DOWN: SELECT   ENTER: CONFIRM   ESC: BACK", "LÊN/XUỐNG: CHỌN   ENTER: XÁC NHẬN   ESC: QUAY LẠI") \
    X(HintTabs, "Q/E: TAB   UP/DOWN: SELECT   LEFT/RIGHT: CHANGE   ESC: BACK", "Q/E: ĐỔI TAB   LÊN/XUỐNG: CHỌN   TRÁI/PHẢI: ĐỔI   ESC: QUAY LẠI") \
    X(HintPages, "LEFT/RIGHT: PAGE   ESC: BACK", "TRÁI/PHẢI: LẬT TRANG   ESC: QUAY LẠI") \
    X(PtsFmt, "%d PTS", "%d ĐIỂM") \
    X(CreditFmt, "CREDIT %03d", "TÍN DỤNG %03d") \
    X(HiScore, "HI-SCORE", "ĐIỂM CAO") \
    X(OneUp, "1UP", "1UP") \
    \
    /* --- Menu chinh --- */ \
    X(Tagline, "HOLD THE LAST LINE", "GIỮ VỮNG TUYẾN CUỐI") \
    X(MenuPlay, "PLAY", "CHƠI NGAY") \
    X(MenuLeaderboard, "HIGH SCORES", "BẢNG XẾP HẠNG") \
    X(MenuAchievements, "ACHIEVEMENTS", "THÀNH TỰU") \
    X(MenuHowTo, "HOW TO PLAY", "HƯỚNG DẪN") \
    X(MenuSettings, "SETTINGS", "CÀI ĐẶT") \
    X(MenuQuit, "QUIT", "THOÁT") \
    X(MenuDescPlay, "Choose difficulty and ship, then launch.", "Chọn độ khó và tàu chiến rồi xuất kích.") \
    X(MenuDescLeaderboard, "The ten best runs on this machine.", "Mười ván chơi hay nhất trên máy này.") \
    X(MenuDescAchievements, "Goals that pay out extra credits.", "Các mục tiêu thưởng thêm tín dụng.") \
    X(MenuDescHowTo, "Controls, enemies, power-ups and tips.", "Điều khiển, kẻ địch, vật phẩm và mẹo chơi.") \
    X(MenuDescSettings, "Language, audio, graphics, controls, accessibility.", "Ngôn ngữ, âm thanh, đồ họa, phím, trợ năng.") \
    X(MenuDescQuit, "Close the game.", "Đóng trò chơi.") \
    X(ConfirmQuitGame, "QUIT GAME?", "THOÁT GAME?") \
    X(InsertCoin, "INSERT COIN", "NẠP XU") \
    X(PressEnter, "PRESS ENTER", "BẤM ENTER") \
    X(MenuFullscreenHint, "F11: FULLSCREEN", "F11: TOÀN MÀN HÌNH") \
    \
    /* --- Attract mode (de yen menu) --- */ \
    X(AttractScoreTable, "SCORE ADVANCE TABLE", "BẢNG ĐIỂM THƯỞNG") \
    X(AttractMystery, "? MYSTERY", "? BÍ ẨN") \
    X(AttractControls, "CONTROLS", "ĐIỀU KHIỂN") \
    \
    /* --- Hangar (chon do kho + loadout truoc van) --- */ \
    X(HangarTitle, "HANGAR", "XƯỞNG TÀU") \
    X(HangarDifficulty, "DIFFICULTY", "ĐỘ KHÓ") \
    X(HangarLoadout, "LOADOUT", "TRANG BỊ") \
    X(HangarLaunch, "LAUNCH", "XUẤT KÍCH") \
    X(DiffEasy, "EASY", "DỄ") \
    X(DiffNormal, "NORMAL", "THƯỜNG") \
    X(DiffHard, "HARD", "KHÓ") \
    X(DiffDescEasy, "Slower invaders, lazier fire. Learn the patterns.", "Địch chậm, bắn thưa. Hợp để làm quen.") \
    X(DiffDescNormal, "The intended experience.", "Trải nghiệm chuẩn như thiết kế.") \
    X(DiffDescHard, "Fast, aggressive, unforgiving.", "Nhanh, hung hãn, không khoan nhượng.") \
    X(LoadoutStandard, "STANDARD", "TIÊU CHUẨN") \
    X(LoadoutVanguard, "VANGUARD", "TIÊN PHONG") \
    X(LoadoutOvercharge, "OVERCHARGE", "QUÁ TẢI") \
    X(LoadoutDescStandard, "No bonus. Pure skill.", "Không thưởng gì. Thuần kỹ năng.") \
    X(LoadoutDescVanguard, "Start the run with +1 life.", "Bắt đầu ván với thêm 1 mạng.") \
    X(LoadoutDescOvercharge, "Start the run with Rapid Fire active.", "Bắt đầu ván với Bắn Nhanh đã kích hoạt.") \
    X(LoadoutFree, "FREE", "MIỄN PHÍ") \
    X(LoadoutReady, "READY", "SẴN SÀNG") \
    X(LoadoutCostFmt, "%d/%d CR", "%d/%d CR") \
    X(LoadoutUnlockHintFmt, "ENTER: UNLOCK FOR %d CR", "ENTER: MỞ KHÓA VỚI %d CR") \
    X(LoadoutNeedFmt, "NEED %d MORE CR", "CÒN THIẾU %d CR") \
    X(LoadoutLockedPlaysStandard, "Locked - you will fly STANDARD.", "Chưa mở - bạn sẽ bay bản TIÊU CHUẨN.") \
    \
    /* --- Bang xep hang --- */ \
    X(LeaderboardTitle, "HIGH SCORES", "BẢNG XẾP HẠNG") \
    X(ColRank, "RANK", "HẠNG") \
    X(ColScore, "SCORE", "ĐIỂM") \
    X(ColWave, "WAVE", "ĐỢT") \
    X(AssistTag, "ASSIST", "HỖ TRỢ") \
    X(NoRecordsYet, "(no records yet)", "(chưa có kỷ lục)") \
    X(AssistLegend, "ASSIST = game speed was below 100%", "HỖ TRỢ = có chơi với tốc độ game dưới 100%") \
    \
    /* --- Thanh tuu --- */ \
    X(AchievementsTitle, "ACHIEVEMENTS", "THÀNH TỰU") \
    X(AchievementsSummaryFmt, "%d/%d UNLOCKED   LIFETIME KILLS: %d", "ĐÃ MỞ %d/%d   TỔNG SỐ ĐỊCH ĐÃ HẠ: %d") \
    X(AchievementUnlockedTag, "ACHIEVEMENT UNLOCKED", "MỞ KHÓA THÀNH TỰU") \
    X(AchievementUnlockedState, "UNLOCKED", "ĐÃ MỞ") \
    X(AchFirstContact, "FIRST CONTACT", "CHẠM TRÁN ĐẦU") \
    X(AchFirstContactDesc, "Destroy your first enemy", "Hạ kẻ địch đầu tiên") \
    X(AchChainReaction, "CHAIN REACTION", "PHẢN ỨNG DÂY CHUYỀN") \
    X(AchChainReactionDesc, "Reach a x%d combo in one run", "Đạt combo x%d trong một ván") \
    X(AchHoldingTheLine, "HOLDING THE LINE", "GIỮ VỮNG PHÒNG TUYẾN") \
    X(AchHoldingTheLineDesc, "Reach wave %d", "Tới đợt %d") \
    X(AchVeteran, "VETERAN", "CỰU BINH") \
    X(AchVeteranDesc, "Reach wave %d", "Tới đợt %d") \
    X(AchGiantSlayer, "GIANT SLAYER", "DIỆT KHỔNG LỒ") \
    X(AchGiantSlayerDesc, "Defeat a boss", "Hạ một trùm") \
    X(AchUntouchable, "UNTOUCHABLE", "BẤT KHẢ XÂM PHẠM") \
    X(AchUntouchableDesc, "Clear a wave without losing a life", "Qua một đợt mà không mất mạng nào") \
    X(AchHighRoller, "HIGH ROLLER", "CAO THỦ") \
    X(AchHighRollerDesc, "Score %d points in one run", "Ghi %d điểm trong một ván") \
    X(AchExterminator, "EXTERMINATOR", "KẺ HỦY DIỆT") \
    X(AchExterminatorDesc, "Destroy %d enemies in total", "Hạ tổng cộng %d kẻ địch") \
    \
    /* --- Huong dan --- */ \
    X(HowToTitle, "HOW TO PLAY", "HƯỚNG DẪN") \
    X(HowToPageControls, "CONTROLS", "ĐIỀU KHIỂN") \
    X(HowToPageEnemies, "ENEMIES", "KẺ ĐỊCH") \
    X(HowToPagePowerUps, "POWER-UPS", "VẬT PHẨM") \
    X(HowToPageTips, "TIPS", "MẸO") \
    X(CtrlMove, "MOVE", "DI CHUYỂN") \
    X(CtrlShoot, "SHOOT", "BẮN") \
    X(CtrlPause, "PAUSE / MENU", "TẠM DỪNG / MENU") \
    X(CtrlRestart, "RESTART RUN", "CHƠI LẠI VÁN") \
    X(CtrlFullscreen, "FULLSCREEN", "TOÀN MÀN HÌNH") \
    X(CtrlProfiler, "PERFORMANCE OVERLAY", "BẢNG HIỆU NĂNG") \
    X(CtrlGamepad, "GAMEPAD: STICK/D-PAD MOVE, A SHOOT, B BACK, START PAUSE, LB/RB TAB", "TAY CẦM: CẦN/D-PAD DI CHUYỂN, A BẮN, B QUAY LẠI, START TẠM DỪNG, LB/RB ĐỔI TAB") \
    X(CtrlMouse, "MOUSE: HOVER TO SELECT, CLICK TO CONFIRM, RIGHT-CLICK TO GO BACK", "CHUỘT: RÊ ĐỂ CHỌN, NHẤP ĐỂ XÁC NHẬN, NHẤP PHẢI ĐỂ QUAY LẠI") \
    X(EnemyBasic, "DRONE", "LÍNH THƯỜNG") \
    X(EnemyBasicDesc, "One hit. The bulk of every formation.", "Trúng một phát là chết. Lực lượng chính.") \
    X(EnemyTanky, "TANK", "THIẾT GIÁP") \
    X(EnemyTankyDesc, "Takes %d hits. Pips under it show HP left.", "Chịu %d phát. Vạch bên dưới là máu còn lại.") \
    X(EnemyZigzag, "ZIGZAG", "CHẠY LẮT LÉO") \
    X(EnemyZigzagDesc, "Darts sideways - lead your shots.", "Lạng lách ngang - bắn đón đầu.") \
    X(EnemyWarden, "WARDEN", "CAI NGỤC") \
    X(EnemyWardenDesc, "Takes %d hits, drops two drones on death.", "Chịu %d phát, chết thì thả ra hai lính.") \
    X(EnemyMedic, "MEDIC", "QUÂN Y") \
    X(EnemyMedicDesc, "Heals wounded allies. Kill it first.", "Hồi máu cho đồng đội. Hạ nó trước.") \
    X(EnemyKamikaze, "KAMIKAZE", "CẢM TỬ") \
    X(EnemyKamikazeDesc, "Breaks formation and dives at you.", "Rời đội hình, lao thẳng vào bạn.") \
    X(EnemyWeaver, "WEAVER", "THOI DỆT") \
    X(EnemyWeaverDesc, "Crosses the sky on a sine wave.", "Bay ngang trời theo đường lượn sóng.") \
    X(EnemyBomber, "BOMBER", "OANH TẠC") \
    X(EnemyBomberDesc, "Flies over and drops bombs.", "Bay ngang và thả bom.") \
    X(EnemyUfo, "MYSTERY SHIP", "ĐĨA BAY BÍ ẨN") \
    X(EnemyUfoDesc, "Random bonus, %d-%d points.", "Thưởng ngẫu nhiên %d-%d điểm.") \
    X(PuRapidFire, "RAPID FIRE", "BẮN NHANH") \
    X(PuRapidFireDesc, "Fire much faster for a while.", "Bắn nhanh hơn hẳn trong một lúc.") \
    X(PuShield, "SHIELD", "KHIÊN") \
    X(PuShieldDesc, "Absorbs the next hit.", "Đỡ giúp đòn kế tiếp.") \
    X(PuPiercing, "PIERCING", "XUYÊN GIÁP") \
    X(PuPiercingDesc, "Shots punch through several enemies.", "Đạn xuyên qua nhiều kẻ địch.") \
    X(PuCleanser, "CLEANSER", "THANH TẨY") \
    X(PuCleanserDesc, "Instantly erases every enemy bullet.", "Xóa sạch đạn địch trên màn hình.") \
    X(PuSpreadShot, "SPREAD SHOT", "BẮN TỎA") \
    X(PuSpreadShotDesc, "Fire three shots in a fan.", "Bắn ba tia tỏa hình quạt.") \
    X(PuOverdrive, "OVERDRIVE", "QUÁ TẢI") \
    X(PuOverdriveDesc, "Faster fire, but a hit costs 2 lives.", "Bắn nhanh hơn, nhưng trúng đạn mất 2 mạng.") \
    X(Tip1, "Bunkers regrow - hide, but not for too long.", "Ụ chắn tự mọc lại - nấp được, nhưng đừng lâu.") \
    X(Tip2, "Kill in quick succession to build a combo multiplier.", "Hạ địch liên tiếp thật nhanh để nhân combo.") \
    X(Tip3, "Every 5th wave is a boss. Upgrades picked before it count twice.", "Cứ 5 đợt có một trùm. Nâng cấp chọn ngay trước đó được tính gấp đôi.") \
    X(Tip4, "Hot colours (red/orange/gold) mean dodge it or grab it.", "Màu nóng (đỏ/cam/vàng) nghĩa là phải né hoặc phải nhặt.") \
    X(Tip5, "Credits are paid at the end of a run - unlock loadouts in the Hangar.", "Tín dụng trả cuối ván - dùng để mở trang bị trong Xưởng tàu.") \
    X(Tip6, "Struggling? Try Accessibility > Game speed. Runs are tagged ASSIST.", "Khó quá? Thử Trợ năng > Tốc độ game. Ván chơi sẽ gắn nhãn HỖ TRỢ.") \
    \
    /* --- Cai dat --- */ \
    X(SettingsTitle, "SETTINGS", "CÀI ĐẶT") \
    X(TabGeneral, "GENERAL", "CHUNG") \
    X(TabGraphics, "GRAPHICS", "ĐỒ HỌA") \
    X(TabControls, "CONTROLS", "ĐIỀU KHIỂN") \
    X(TabAccessibility, "ACCESSIBILITY", "TRỢ NĂNG") \
    X(SetLanguage, "LANGUAGE", "NGÔN NGỮ") \
    X(SetLanguageDesc, "Applies instantly. Default follows your system.", "Áp dụng ngay. Mặc định theo hệ điều hành.") \
    X(SetMasterVolume, "MASTER VOLUME", "ÂM LƯỢNG TỔNG") \
    X(SetMasterVolumeDesc, "Scales every sound in the game.", "Điều chỉnh mọi âm thanh trong game.") \
    X(SetMusicVolume, "MUSIC", "NHẠC NỀN") \
    X(SetMusicVolumeDesc, "Procedural soundtrack and bassline heartbeat.", "Nhạc nền tự sinh và nhịp bass đội hình.") \
    X(SetSfxVolume, "SOUND EFFECTS", "HIỆU ỨNG ÂM THANH") \
    X(SetSfxVolumeDesc, "Shots, explosions, pickups.", "Tiếng bắn, nổ, nhặt vật phẩm.") \
    X(SetUiSounds, "MENU SOUNDS", "ÂM THANH MENU") \
    X(SetUiSoundsDesc, "Blips when moving through menus and typing text.", "Tiếng bíp khi chọn menu và khi chữ hiện ra.") \
    X(SetQuality, "QUALITY", "CHẤT LƯỢNG") \
    X(QualityLow, "LOW", "THẤP") \
    X(QualityMedium, "MEDIUM", "VỪA") \
    X(QualityHigh, "HIGH", "CAO") \
    X(QualityDescLow, "No bloom/grid/nebula/shockwaves, half particles.", "Tắt bloom/lưới/tinh vân/sóng xung kích, nửa số hạt.") \
    X(QualityDescMedium, "Bloom, reactive grid, nebula, shockwaves.", "Bloom, lưới phản ứng, tinh vân, sóng xung kích.") \
    X(QualityDescHigh, "Wider bloom, denser grid, curved CRT, RGB waves.", "Bloom rộng, lưới dày, CRT cong, sóng tách màu.") \
    X(SetCrt, "CRT SCANLINES", "HIỆU ỨNG CRT") \
    X(SetCrtDesc, "Scanlines + vignette + flicker (curved on HIGH).", "Vạch quét + tối viền + nhấp nháy (cong ở mức CAO).") \
    X(SetFullscreen, "FULLSCREEN", "TOÀN MÀN HÌNH") \
    X(SetFullscreenDesc, "Same as F11. Keeps the 4:3 frame with black bars.", "Giống F11. Giữ khung 4:3, thêm viền đen hai bên.") \
    X(SetShowFps, "SHOW FPS", "HIỆN FPS") \
    X(SetShowFpsDesc, "Small frame counter in the corner (F3 shows full stats).", "Bộ đếm khung hình nhỏ ở góc (F3 hiện đầy đủ).") \
    X(SetKeyLeft, "MOVE LEFT", "SANG TRÁI") \
    X(SetKeyRight, "MOVE RIGHT", "SANG PHẢI") \
    X(SetKeyShoot, "SHOOT", "BẮN") \
    X(SetKeyPause, "PAUSE", "TẠM DỪNG") \
    X(SetKeyDesc, "ENTER/click, then press the new key. Arrows and ESC always work.", "ENTER/nhấp chuột rồi bấm phím mới. Mũi tên và ESC luôn dùng được.") \
    X(SetKeyWaiting, "PRESS A KEY...", "BẤM PHÍM MỚI...") \
    X(SetKeyRejected, "That key is reserved or already used.", "Phím đó đã dành riêng hoặc đang được dùng.") \
    X(SetAutoFire, "AUTO-FIRE", "TỰ ĐỘNG BẮN") \
    X(SetAutoFireDesc, "Keep shooting without holding the button.", "Bắn liên tục mà không cần giữ nút.") \
    X(SetResetKeys, "RESET KEYS", "KHÔI PHỤC PHÍM") \
    X(SetResetKeysDesc, "Back to A / D / SPACE / P.", "Trả về A / D / SPACE / P.") \
    X(SetResetAction, "RESET", "KHÔI PHỤC") \
    X(SetReduceFlashing, "REDUCE FLASHING", "GIẢM NHẤP NHÁY") \
    X(SetReduceFlashingDesc, "Removes screen flicker, blinking and large flashes.", "Bỏ nhấp nháy màn hình, chữ chớp và chớp sáng lớn.") \
    X(SetShake, "SCREEN SHAKE", "RUNG MÀN HÌNH") \
    X(SetShakeDesc, "Camera shake on hits and explosions.", "Rung khung hình khi trúng đạn và nổ.") \
    X(SetGameSpeed, "GAME SPEED", "TỐC ĐỘ GAME") \
    X(SetGameSpeedDesc, "Slows the whole game. Scores are tagged ASSIST.", "Làm chậm toàn bộ game. Điểm sẽ gắn nhãn HỖ TRỢ.") \
    X(SetColorFilter, "COLOR FILTER", "LỌC MÀU") \
    X(SetColorFilterDesc, "Shifts hues for colour-vision deficiency.", "Dịch sắc màu cho người mù màu.") \
    X(FilterProtan, "PROTAN", "MÙ ĐỎ") \
    X(FilterDeutan, "DEUTAN", "MÙ LỤC") \
    X(FilterTritan, "TRITAN", "MÙ LAM") \
    X(SetHitbox, "SHOW HITBOX", "HIỆN VÙNG TRÚNG") \
    X(SetHitboxDesc, "Outline the area of your ship that can be hit.", "Viền vùng trên tàu có thể bị trúng đạn.") \
    \
    /* --- Pause --- */ \
    X(PausedTitle, "PAUSED", "TẠM DỪNG") \
    X(PauseResume, "RESUME", "TIẾP TỤC") \
    X(PauseRestart, "RESTART", "CHƠI LẠI") \
    X(PauseQuitToMenu, "QUIT TO MENU", "VỀ MENU CHÍNH") \
    X(ConfirmRestart, "RESTART THIS RUN?", "CHƠI LẠI TỪ ĐẦU?") \
    X(ConfirmQuitToMenu, "QUIT TO MENU?", "VỀ MENU CHÍNH?") \
    X(ConfirmLoseProgress, "Progress in this run will be lost.", "Tiến trình ván này sẽ mất.") \
    X(PauseRunInfoFmt, "WAVE %d   SCORE %d", "ĐỢT %d   ĐIỂM %d") \
    \
    /* --- Man ket thuc --- */ \
    X(GameOverTitle, "GAME OVER", "KẾT THÚC") \
    X(WaveClearedFmt, "WAVE %d CLEARED!", "HOÀN THÀNH ĐỢT %d!") \
    X(ScoreFmt, "SCORE: %d", "ĐIỂM: %d") \
    X(NewRecordBanner, "NEW RECORD! (#1)", "KỶ LỤC MỚI! (#1)") \
    X(MadeTop10Banner, "TOP 10!", "LỌT TOP 10!") \
    X(TopScoreFmt, "TOP SCORE: %d", "ĐIỂM CAO NHẤT: %d") \
    X(RunSummaryTitle, "RUN SUMMARY", "TỔNG KẾT VÁN") \
    X(RunSummaryScore, "SCORE", "ĐIỂM") \
    X(RunSummaryWave, "WAVE REACHED", "ĐỢT ĐẠT TỚI") \
    X(RunSummaryKills, "ENEMIES DESTROYED", "ĐỊCH ĐÃ HẠ") \
    X(RunSummaryCombo, "BEST COMBO", "COMBO CAO NHẤT") \
    X(RunSummaryEarned, "CREDITS EARNED", "TÍN DỤNG KIẾM ĐƯỢC") \
    X(RunSummaryTotal, "TOTAL", "TỔNG") \
    X(NextUnlockFmt, "NEXT: %s  %d/%d CR", "TIẾP THEO: %s  %d/%d CR") \
    X(AllUnlocked, "ALL LOADOUTS UNLOCKED", "ĐÃ MỞ HẾT TRANG BỊ") \
    X(EndToMenu, "MENU", "MENU") \
    X(EndRestart, "RESTART (R)", "CHƠI LẠI (R)") \
    X(UpgradeMoveSpeedName, "MOVE SPEED", "TỐC ĐỘ BAY") \
    X(UpgradeMoveSpeedDesc, "+8% speed per pick", "+8% tốc độ mỗi lần chọn") \
    X(UpgradeExtraLifeName, "EXTRA LIFE", "THÊM MẠNG") \
    X(UpgradeExtraLifeDesc, "+1 life (max 5)", "+1 mạng (tối đa 5)") \
    X(UpgradeBonusScoreName, "BONUS SCORE", "ĐIỂM THƯỞNG") \
    X(UpgradeBonusScoreDesc, "+1000 points", "+1000 điểm") \
    X(UpgradeOwnedFmt, "x%d owned", "đã có x%d") \
    X(UpgradeSelectHint, "LEFT/RIGHT: SELECT   ENTER: CONFIRM   R: RESTART", "TRÁI/PHẢI: CHỌN   ENTER: XÁC NHẬN   R: CHƠI LẠI") \
    X(BossWaveUpgradeBanner, "BOSS WAVE NEXT - PICK APPLIES x2!", "ĐỢT SAU LÀ TRÙM - NÂNG CẤP TÍNH x2!") \
    \
    /* --- HUD --- */ \
    X(HudScore, "SCORE", "ĐIỂM") \
    X(HudHi, "HI", "CAO") \
    X(HudHintFmt, "%s: PAUSE   R: RESTART", "%s: TẠM DỪNG   R: CHƠI LẠI") \
    X(HudBossFmt, "BOSS - %s", "TRÙM - %s") \
    X(ShieldTag, "SHIELD!", "KHIÊN!") \
    X(WaveBannerFmt, "WAVE %d", "ĐỢT %d") \
    X(BossWarning, "WARNING", "CẢNH BÁO") \
    X(BossIncomingHint, "INCOMING", "ĐANG TỚI") \
    X(BossVanguard, "VANGUARD", "TIÊN PHONG") \
    X(BossSentinel, "SENTINEL", "LÍNH GÁC") \
    X(BossSwarmer, "SWARMER", "CHÚA BẦY")

enum class Str : uint16_t {
#define LOC_ENUM(id, en, vi) id,
    LOC_STRINGS(LOC_ENUM)
#undef LOC_ENUM
    COUNT
};
constexpr int STR_COUNT = (int)Str::COUNT;

namespace Loc {
    // Ngon ngu hien hanh - 1 bien toan cuc duy nhat. GameManager dat theo settings.language
    // (luc nap + moi lan doi trong Cai dat); test co the dat tam roi tra lai.
    void SetLanguage(Language lang);
    Language GetLanguage();

    const char* Get(Str id, Language lang);
    const char* LanguageCode(Language lang);         // "EN"/"VI" - ghi vao settings.cfg, KHONG dich
    const char* LanguageDisplayName(Language lang);  // "ENGLISH"/"TIẾNG VIỆT" - luon hien bang chinh ngon ngu do
    Language LanguageFromCode(std::string_view code, Language fallback);

    // Ngon ngu mac dinh theo he dieu hanh: quy uoc POSIX LC_ALL > LC_MESSAGES > LANG (bien dau
    // tien KHAC RONG thang). "vi", "vi_VN.UTF-8"... -> VI; con lai (ke ca khong co gi) -> EN.
    // Ham thuan (nhan 3 chuoi thay vi tu getenv) de test duoc; tren Windows 3 bien nay thuong
    // khong ton tai nen roi ve EN - doi trong Cai dat la xong.
    Language DetectFromEnv(const char* lcAll, const char* lcMessages, const char* lang);
    Language DetectSystemLanguage(); // Goi DetectFromEnv voi getenv() that

    // Bang ma (codepoint) truyen vao LoadFontEx: ASCII in duoc + toan bo chu cai tieng Viet.
    const std::vector<int>& FontCharset();

    // Giai ma UTF-8 thanh codepoint (ky tu loi -> 0xFFFD). Dung cho kiem tra bang chu va cho
    // hieu ung go chu (dem KY TU, khong dem byte - 1 chu co dau = 2-3 byte).
    std::vector<int> DecodeUtf8(std::string_view s);
}

// Ham dich chinh - moi noi ve chu goi qua day.
inline const char* Tr(Str id) { return Loc::Get(id, Loc::GetLanguage()); }
