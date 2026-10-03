// ==========================================
// GAME MANAGER - PHAN GIAO DIEN (nang cap GUI). Tach khoi game_manager.cpp de file luat choi
// khong phinh them ~500 dong dieu huong menu. Van la thanh vien GameManager (cung quyen truy
// cap) - chi la chia file, khong phai 1 he thong moi.
//
// MAU CHUNG cua moi ham Update*Screen():
//   1. Hop xac nhan dang mo -> no "nuot" input truoc (UpdateConfirm).
//   2. Quay lai (ESC/Backspace/B/chuot phai/nut QUAY LAI) -> GoToScreen(man cha).
//   3. Len/Xuong (+ chuot di chuyen tren nut) doi con tro; Trai/Phai doi gia tri.
//   4. Enter / nut A / bam chuot trai vao muc dang chon -> kich hoat.
// Vi tri nut LUON lay tu ui_layout.h - cung nguon voi RenderSystem.
// ==========================================
#include "game_manager.h"
#include "ui_nav.h"
#include "ui_layout.h"
#include "settings_menu.h"
#include "meta_progress.h"
#include "upgrade_types.h"
#include <cmath>

namespace {
    const Str kMenuDesc[UiLayout::MAIN_MENU_ITEMS] = {
        Str::MenuDescPlay, Str::MenuDescLeaderboard, Str::MenuDescAchievements,
        Str::MenuDescHowTo, Str::MenuDescSettings, Str::MenuDescQuit,
    };
    const Str kDiffDesc[3] = { Str::DiffDescEasy, Str::DiffDescNormal, Str::DiffDescHard };
    const Str kLoadoutDesc[3] = { Str::LoadoutDescStandard, Str::LoadoutDescVanguard, Str::LoadoutDescOvercharge };

    bool Clicked(const PointerInput& p, Rectangle r) { return p.clicked && UiNav::PointIn(p.pos, r); }
}

// ==========================================
// HA TANG CHUNG
// ==========================================
void GameManager::GoToScreen(GameState next) {
    state = next;
    ui.screenTimer = 0.0f;
    ui.idleTimer = 0.0f;
    ui.confirm = ConfirmKind::None;
    ResetRowTimer();
}

void GameManager::ResetRowTimer() {
    ui.rowTimer = 0.0f;
    ui.typedChars = 0;
}

void GameManager::MoveSelection(int& index, int dir, int count) {
    int next = UiNav::Wrap(index, dir, count);
    if (next == index) return;
    index = next;
    audio.PlayUiMove();
    ResetRowTimer();
}

void GameManager::SelectWithPointer(int& index, int count, Rectangle (*rectOf)(int)) {
    if (!ui.pointer.moved) return; // Chuot nam yen -> khong gianh con tro cua ban phim (ui_nav.h)
    for (int i = 0; i < count; i++) {
        if (!UiNav::PointIn(ui.pointer.pos, rectOf(i))) continue;
        if (i != index) {
            index = i;
            audio.PlayUiMove();
            ResetRowTimer();
        }
        return;
    }
}

bool GameManager::BackPressed(const MenuInput& input) const {
    return input.Back || ui.pointer.rightClicked || Clicked(ui.pointer, UiLayout::BackButton());
}

void GameManager::OpenSettings(GameState returnTo) {
    ui.settingsReturn = returnTo;
    ui.rebinding = false;
    GoToScreen(GameState::SETTINGS);
}

void GameManager::OpenConfirm(ConfirmKind kind) {
    ui.confirm = kind;
    ui.confirmIndex = 1; // KHONG - xem UiState
    audio.PlayUiConfirm();
}

const char* GameManager::UiTypedLine() const {
    switch (state) {
        case GameState::MENU: return Tr(kMenuDesc[UiNav::Clamp(ui.menuIndex, 0, UiLayout::MAIN_MENU_ITEMS - 1)]);
        case GameState::HANGAR:
            if (ui.hangarRow == 0) return Tr(kDiffDesc[UiNav::Clamp((int)difficulty, 0, 2)]);
            if (ui.hangarRow == 1) return Tr(kLoadoutDesc[UiNav::Clamp(selectedLoadout, 0, 2)]);
            return nullptr;
        case GameState::SETTINGS: {
            int count = 0;
            const SettingRowDef* rows = SettingsTabRows((SettingsTab)ui.settingsTab, count);
            if (count <= 0) return nullptr;
            const SettingRowDef& def = rows[UiNav::Clamp(ui.settingsRow, 0, count - 1)];
            if (def.id == SettingId::Quality) {
                // Mo ta preset doi theo gia tri DANG chon - phai khop GraphicsSettings::BloomEnabled/...
                const Str q[] = { Str::QualityDescLow, Str::QualityDescMedium, Str::QualityDescHigh };
                return Tr(q[UiNav::Clamp((int)settings.graphics.quality, 0, 2)]);
            }
            return Tr(def.desc);
        }
        default: return nullptr;
    }
}

void GameManager::UpdateUi(float dt) {
    ui.screenTimer += dt;
    ui.rowTimer += dt;
    if (ui.rebindRejectTimer > 0.0f) ui.rebindRejectTimer -= dt;

    // --- CHUOT -> toa do canvas 800x600 ---
    PointerInput& p = ui.pointer;
    if (IsWindowReady()) {
        Vector2 raw = GetMousePosition();
        UiNav::VirtualPoint vp = UiNav::ScreenToVirtual(raw, (float)GetScreenWidth(), (float)GetScreenHeight(),
                                                        (float)Config::SCREEN_W, (float)Config::SCREEN_H);
        p.moved = ui.lastMouseRaw.x >= 0.0f && (raw.x != ui.lastMouseRaw.x || raw.y != ui.lastMouseRaw.y);
        ui.lastMouseRaw = raw;
        p.pos = vp.pos;
        p.inside = vp.inside;
        p.clicked = vp.inside && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        p.held = vp.inside && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        p.rightClicked = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        p.wheel = GetMouseWheelMove();
        // Con tro chuot chi vuong khi dang choi - an trong PLAYING, hien lai o moi man menu.
        const bool wantHidden = state == GameState::PLAYING && transitionPhase == TransitionPhase::NONE;
        if (wantHidden && !IsCursorHidden()) HideCursor();
        else if (!wantHidden && IsCursorHidden()) ShowCursor();
    } else {
        p = PointerInput{};
    }

    // --- Tieng go chu: moi 2 ky tu moi hien 1 tieng "tach" rat nho ---
    if (const char* line = UiTypedLine()) {
        int typed = UiNav::TypewriterChars(line, ui.rowTimer, TYPEWRITER_CPS);
        if (typed > ui.typedChars) {
            if (typed / 2 != ui.typedChars / 2) audio.PlayUiType();
            ui.typedChars = typed;
        }
    }

    MenuInput input = InputSystem::PollMenu(settings);
    if (input.ToggleFullscreen) ToggleFullscreenSetting(); // 1 cho duy nhat cho MOI man (truoc day moi Update* tu goi)

    // --- De yen menu chinh -> ATTRACT (kieu may arcade) ---
    if (state == GameState::MENU && ui.confirm == ConfirmKind::None && transitionPhase == TransitionPhase::NONE) {
        bool active = input.AnyKey || p.moved || p.clicked || p.rightClicked || p.wheel != 0.0f;
        ui.idleTimer = active ? 0.0f : ui.idleTimer + dt;
        if (ui.idleTimer >= ATTRACT_IDLE_SECONDS) {
            GoToScreen(GameState::ATTRACT);
            ui.attractPage = 0;
            ui.attractTimer = 0.0f;
        }
    }
}

// Hop Co/Khong. Tra ve true khi hop dang mo (ke ca vua dong frame nay) -> man ben duoi bo qua input.
bool GameManager::UpdateConfirm(const MenuInput& input) {
    if (ui.confirm == ConfirmKind::None) return false;
    if (input.Left || input.Right || input.Up || input.Down) {
        ui.confirmIndex = 1 - ui.confirmIndex;
        audio.PlayUiMove();
    }
    int dummy = ui.confirmIndex;
    SelectWithPointer(dummy, 2, UiLayout::ConfirmButton);
    ui.confirmIndex = dummy;

    bool activate = input.Confirm;
    for (int i = 0; i < 2; i++) {
        if (Clicked(ui.pointer, UiLayout::ConfirmButton(i))) { ui.confirmIndex = i; activate = true; }
    }
    if (input.Back || ui.pointer.rightClicked) {
        ui.confirm = ConfirmKind::None;
        audio.PlayUiBack();
        return true;
    }
    if (!activate) return true;

    ConfirmKind kind = ui.confirm;
    ui.confirm = ConfirmKind::None;
    if (ui.confirmIndex != 0) { audio.PlayUiBack(); return true; }
    audio.PlayUiConfirm();
    switch (kind) {
        case ConfirmKind::QuitGame:   quitRequested = true; break;
        case ConfirmKind::RestartRun: InitLevel(true); RequestTransition(GameState::PLAYING); break;
        // Bo van giua chung: KHONG tinh la GAME_OVER (khong cong CR) - giong phim R. Thuong thanh tuu
        // dang cho van duoc tra o InitLevel(true) ke tiep hoac luc dong cua so (xem CLAUDE.md).
        case ConfirmKind::QuitToMenu: RequestTransition(GameState::MENU); break;
        case ConfirmKind::None: break;
    }
    return true;
}

// ==========================================
// CAI DAT: ap dung + luu
// ==========================================
void GameManager::ApplySettings() {
    Loc::SetLanguage(settings.language);
    audio.SetMix(settings.volume, settings.musicVolume, settings.sfxVolume);
    audio.SetUiSoundsEnabled(settings.uiSounds);
    ApplyGraphicsSettings();
    // Toan man hinh: chi khi co cua so that (test headless khong co) va chi khi LECH - ToggleFullscreen
    // la "dao trang thai", goi thua 1 lan la nguoc y nguoi choi.
    if (IsWindowReady() && IsWindowFullscreen() != settings.fullscreen) ToggleFullscreen();
}

void GameManager::ApplyGraphicsSettings() {
    particles.SetSpawnScale(settings.graphics.ParticleScale());
    // Dung lai luoi CHI khi mat do doi (Init xoa moi bien dang dang co). Ham thuan CPU, an toan
    // headless - goi duoc ca trong test.
    if (warpGrid.CellSize() != settings.graphics.GridCellSize()) {
        warpGrid.Init((float)Config::SCREEN_W, (float)Config::SCREEN_H, settings.graphics.GridCellSize());
    }
}

void GameManager::ToggleFullscreenSetting() {
    settings.fullscreen = !settings.fullscreen;
    ApplySettings();
    SaveSettings();
}

void GameManager::SaveSettings() {
    settings.difficulty = difficulty;
    settings.volume = audio.GetVolume();
    settings.SaveToFile(Config::SettingsFilePath());
}

// ==========================================
// MENU CHINH
// ==========================================
void GameManager::UpdateMenu() {
    MenuInput input = InputSystem::PollMenu(settings);
    if (UpdateConfirm(input)) return;

    if (input.Up)   MoveSelection(ui.menuIndex, -1, UiLayout::MAIN_MENU_ITEMS);
    if (input.Down) MoveSelection(ui.menuIndex, 1, UiLayout::MAIN_MENU_ITEMS);
    SelectWithPointer(ui.menuIndex, UiLayout::MAIN_MENU_ITEMS, UiLayout::MainMenuItem);

    if (input.Back || ui.pointer.rightClicked) { OpenConfirm(ConfirmKind::QuitGame); return; }

    bool activate = input.Confirm || Clicked(ui.pointer, UiLayout::MainMenuItem(ui.menuIndex));
    if (!activate) return;
    audio.PlayUiConfirm();
    switch (ui.menuIndex) {
        case 0: GoToScreen(GameState::HANGAR); break;
        case 1: GoToScreen(GameState::LEADERBOARD); break;
        case 2: GoToScreen(GameState::ACHIEVEMENTS); break;
        case 3: GoToScreen(GameState::HOWTO); break;
        case 4: OpenSettings(GameState::MENU); break;
        default: OpenConfirm(ConfirmKind::QuitGame); break;
    }
}

// ==========================================
// HANGAR - do kho + loadout + xuat kich
// ==========================================
void GameManager::LaunchRun() {
    audio.PlayUiConfirm();
    SaveSettings(); // Do kho vua chon thanh mac dinh lan sau
    InitLevel(true);
    RequestTransition(GameState::PLAYING);
}

// Loadout KHOA + du CR -> mo khoa (tru tien + luu file). Truoc day chi CAN cycle toi la tu tru tien
// - de mat tien chi vi "xem thu". Gio phai bam xac nhan, va hien ro gia truoc khi bam.
void GameManager::TryUnlockSelectedLoadout() {
    LoadoutType chosen = (LoadoutType)selectedLoadout;
    if (chosen == LoadoutType::Standard || metaProgress.IsUnlocked(chosen)) return;
    if (metaProgress.TryUnlock(chosen, GetLoadoutUnlockCost(chosen))) audio.PlayPickup();
    else audio.PlayUiError();
}

void GameManager::UpdateHangar() {
    MenuInput input = InputSystem::PollMenu(settings);
    if (BackPressed(input)) { audio.PlayUiBack(); GoToScreen(GameState::MENU); return; }

    if (input.Up)   MoveSelection(ui.hangarRow, -1, UiLayout::HANGAR_ROWS);
    if (input.Down) MoveSelection(ui.hangarRow, 1, UiLayout::HANGAR_ROWS);
    SelectWithPointer(ui.hangarRow, UiLayout::HANGAR_ROWS, UiLayout::HangarRow);

    int dir = input.Right ? 1 : (input.Left ? -1 : 0);
    if (dir != 0 && ui.hangarRow == 0) {
        difficulty = CycleDifficulty(difficulty, dir);
        audio.PlayUiMove();
        ResetRowTimer();
    } else if (dir != 0 && ui.hangarRow == 1) {
        selectedLoadout = UiNav::Wrap(selectedLoadout, dir, 3);
        audio.PlayUiMove();
        ResetRowTimer();
    }

    // Chuot: bam thang vao pill/the/nut
    for (int i = 0; i < 3; i++) {
        if (Clicked(ui.pointer, UiLayout::HangarDifficultyPill(i)) && (int)difficulty != i) {
            difficulty = (Difficulty)i;
            audio.PlayUiMove();
            ResetRowTimer();
        }
        if (Clicked(ui.pointer, UiLayout::HangarLoadoutCard(i))) {
            if (selectedLoadout == i) TryUnlockSelectedLoadout(); // Bam lan 2 vao the dang chon = mo khoa
            else { selectedLoadout = i; audio.PlayUiMove(); ResetRowTimer(); }
        }
    }
    if (Clicked(ui.pointer, UiLayout::HangarLaunch())) { LaunchRun(); return; }

    if (!input.Confirm) return;
    if (ui.hangarRow == 0) { MoveSelection(ui.hangarRow, 1, UiLayout::HANGAR_ROWS); return; }
    if (ui.hangarRow == 1) {
        LoadoutType chosen = (LoadoutType)selectedLoadout;
        if (chosen != LoadoutType::Standard && !metaProgress.IsUnlocked(chosen)) TryUnlockSelectedLoadout();
        else MoveSelection(ui.hangarRow, 1, UiLayout::HANGAR_ROWS);
        return;
    }
    LaunchRun();
}

// ==========================================
// BANG XEP HANG / THANH TUU - chi xem, moi phim xac nhan/quay lai deu ve MENU
// ==========================================
void GameManager::UpdateLeaderboardScreen() {
    MenuInput input = InputSystem::PollMenu(settings);
    if (BackPressed(input) || input.Confirm) { audio.PlayUiBack(); GoToScreen(GameState::MENU); }
}

void GameManager::UpdateAchievementsScreen() {
    MenuInput input = InputSystem::PollMenu(settings);
    if (BackPressed(input) || input.Confirm) { audio.PlayUiBack(); GoToScreen(GameState::MENU); }
}

// ==========================================
// HUONG DAN - 4 trang, Trai/Phai hoac Q/E lat trang
// ==========================================
void GameManager::UpdateHowToScreen() {
    MenuInput input = InputSystem::PollMenu(settings);
    if (BackPressed(input)) { audio.PlayUiBack(); GoToScreen(GameState::MENU); return; }
    int dir = (input.Right || input.TabNext) ? 1 : ((input.Left || input.TabPrev) ? -1 : 0);
    if (dir != 0) { MoveSelection(ui.howtoPage, dir, UiLayout::HOWTO_PAGES); ui.screenTimer = 0.4f; }
    for (int i = 0; i < UiLayout::HOWTO_PAGES; i++) {
        if (Clicked(ui.pointer, UiLayout::HowToTab(i)) && i != ui.howtoPage) {
            ui.howtoPage = i;
            ui.screenTimer = 0.4f; // Go lai noi dung trang (bo qua glitch tieu de)
            audio.PlayUiMove();
        }
    }
}

// ==========================================
// ATTRACT - trinh dien khi de yen; bat ky input nao -> ve MENU
// ==========================================
void GameManager::UpdateAttract(float dt) {
    ui.attractTimer += dt;
    if (ui.attractTimer >= ATTRACT_PAGE_SECONDS) {
        ui.attractTimer = 0.0f;
        ui.attractPage = (ui.attractPage + 1) % ATTRACT_PAGE_COUNT;
        ui.screenTimer = 0.0f;
    }
    MenuInput input = InputSystem::PollMenu(settings);
    const PointerInput& p = ui.pointer;
    if (input.AnyKey || p.moved || p.clicked || p.rightClicked) {
        GoToScreen(GameState::MENU);
        audio.PlayUiConfirm();
    }
}

// ==========================================
// CAI DAT - 4 tab, dong lay tu settings_menu.h
// ==========================================
void GameManager::UpdateSettingsScreen() {
    if (ui.rebinding) { UpdateRebind(); return; }
    MenuInput input = InputSystem::PollMenu(settings);
    if (BackPressed(input)) {
        audio.PlayUiBack();
        GameState back = ui.settingsReturn;
        GoToScreen(back);
        return;
    }

    // --- Tab ---
    int tabDir = input.TabNext ? 1 : (input.TabPrev ? -1 : 0);
    if (tabDir != 0) { MoveSelection(ui.settingsTab, tabDir, SETTINGS_TAB_COUNT); ui.settingsRow = 0; }
    for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
        if (Clicked(ui.pointer, UiLayout::SettingsTab(i)) && i != ui.settingsTab) {
            ui.settingsTab = i;
            ui.settingsRow = 0;
            audio.PlayUiMove();
            ResetRowTimer();
        }
    }

    int count = 0;
    const SettingRowDef* rows = SettingsTabRows((SettingsTab)ui.settingsTab, count);
    ui.settingsRow = UiNav::Clamp(ui.settingsRow, 0, count - 1);
    if (input.Up)   MoveSelection(ui.settingsRow, -1, count);
    if (input.Down) MoveSelection(ui.settingsRow, 1, count);
    SelectWithPointer(ui.settingsRow, count, UiLayout::SettingsRow);

    auto changed = [&](SettingId id) {
        ApplySettings();
        SaveSettings();
        audio.PlayUiMove();
        if (id == SettingId::Language) { ui.screenTimer = 0.0f; ResetRowTimer(); } // Tieu de glitch + go lai bang ngon ngu moi
        if (id == SettingId::Quality) ResetRowTimer();                            // Mo ta preset doi theo gia tri
    };

    const SettingRowDef& def = rows[ui.settingsRow];
    int dir = input.Right ? 1 : (input.Left ? -1 : 0);
    if (dir != 0 && AdjustSetting(settings, def.id, dir)) changed(def.id);

    // --- Chuot tren TUNG dong (khong chi dong dang chon): bam thang vao o lua chon / keo thanh truot ---
    const PointerInput& p = ui.pointer;
    for (int r = 0; r < count; r++) {
        const SettingRowDef& d = rows[r];
        switch (d.kind) {
            case SettingKind::Choice: {
                int n = ChoiceCount(d.id);
                for (int i = 0; i < n; i++) {
                    if (!Clicked(p, UiLayout::SettingsOption(r, i, n))) continue;
                    ui.settingsRow = r;
                    if (ChoiceIndex(settings, d.id) != i) { SetChoice(settings, d.id, i); changed(d.id); }
                }
                break;
            }
            case SettingKind::Slider: {
                Rectangle bar = UiLayout::SettingsSlider(r);
                Rectangle grab{ bar.x - 6.0f, bar.y - 8.0f, bar.width + 12.0f, bar.height + 16.0f }; // Vung bam rong hon thanh 1 chut
                if (p.held && UiNav::PointIn(p.pos, grab)) {
                    ui.settingsRow = r;
                    float v = UiNav::SliderValueAt(p.pos.x, bar, SLIDER_STEP);
                    if (fabsf(v - SliderValue(settings, d.id)) > 1e-4f) { SetSlider(settings, d.id, v); changed(d.id); }
                }
                break;
            }
            case SettingKind::Key:
                if (Clicked(p, UiLayout::SettingsControl(r))) { ui.settingsRow = r; ui.rebinding = true; audio.PlayUiConfirm(); return; }
                break;
            case SettingKind::Action:
                if (Clicked(p, UiLayout::SettingsControl(r))) {
                    ui.settingsRow = r;
                    settings.ResetKeyBindingsToDefault();
                    SaveSettings();
                    audio.PlayUiConfirm();
                }
                break;
        }
    }
    // Lan chuot tren dong thanh truot = +-1 buoc
    if (p.wheel != 0.0f && def.kind == SettingKind::Slider && AdjustSetting(settings, def.id, p.wheel > 0.0f ? 1 : -1)) changed(def.id);

    if (!input.Confirm) return;
    switch (def.kind) {
        case SettingKind::Choice: if (AdjustSetting(settings, def.id, 1)) changed(def.id); break;
        case SettingKind::Key:    ui.rebinding = true; audio.PlayUiConfirm(); break;
        case SettingKind::Action: settings.ResetKeyBindingsToDefault(); SaveSettings(); audio.PlayUiConfirm(); break;
        case SettingKind::Slider: break;
    }
}

// DOI PHIM - khong dung PollMenu(): MOI phim deu co the la gia tri can ghi nhan. Doc thang
// PollAnyKeyPressed() (hang doi phim cua raylib). Giu nguyen 2 luat cua man KEYBIND cu:
// tu choi phim he thong (khoa minh khoi menu) va phim dang dung cho hanh dong khac.
void GameManager::UpdateRebind() {
    if (IsKeyPressed(KEY_ESCAPE) || ui.pointer.rightClicked) {
        ui.rebinding = false;
        audio.PlayUiBack();
        return;
    }
    int newKey = InputSystem::PollAnyKeyPressed();
    if (newKey == 0) return;

    int count = 0;
    const SettingRowDef* rows = SettingsTabRows((SettingsTab)ui.settingsTab, count);
    int Settings::* field = KeyField(rows[UiNav::Clamp(ui.settingsRow, 0, count - 1)].id);
    if (field == nullptr) { ui.rebinding = false; return; }

    static const int reserved[] = { KEY_ESCAPE, KEY_ENTER, KEY_KP_ENTER, KEY_BACKSPACE, KEY_F3, KEY_F11, KEY_R,
                                    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT };
    bool reject = false;
    for (int r : reserved) reject = reject || (newKey == r);
    int Settings::* all[] = { &Settings::keyMoveLeft, &Settings::keyMoveRight, &Settings::keyShoot, &Settings::keyPause };
    for (int Settings::* f : all) {
        if (f != field && settings.*f == newKey) reject = true;
    }
    if (reject) {
        ui.rebindRejectTimer = 1.6f;
        audio.PlayUiError();
        return; // Van dang cho - nguoi choi bam phim khac
    }
    settings.*field = newKey;
    SaveSettings();
    ui.rebinding = false;
    audio.PlayUiConfirm();
}

// ==========================================
// PAUSE - danh sach Tiep tuc / Cai dat / Choi lai / Ve menu (2 muc cuoi hoi lai)
// ==========================================
void GameManager::UpdatePaused() {
    MenuInput input = InputSystem::PollMenu(settings);
    if (UpdateConfirm(input)) return;
    if (input.PauseToggle || input.Back || ui.pointer.rightClicked) {
        state = GameState::PLAYING;
        audio.PlayUiBack();
        return;
    }
    if (input.Up)   MoveSelection(ui.pauseIndex, -1, UiLayout::PAUSE_ITEMS);
    if (input.Down) MoveSelection(ui.pauseIndex, 1, UiLayout::PAUSE_ITEMS);
    SelectWithPointer(ui.pauseIndex, UiLayout::PAUSE_ITEMS, UiLayout::PauseItem);
    if (input.Restart) { OpenConfirm(ConfirmKind::RestartRun); return; }

    bool activate = input.Confirm || Clicked(ui.pointer, UiLayout::PauseItem(ui.pauseIndex));
    if (!activate) return;
    switch (ui.pauseIndex) {
        case 0: state = GameState::PLAYING; audio.PlayUiConfirm(); break;
        case 1: audio.PlayUiConfirm(); OpenSettings(GameState::PAUSED); break;
        case 2: OpenConfirm(ConfirmKind::RestartRun); break;
        default: OpenConfirm(ConfirmKind::QuitToMenu); break;
    }
}

// ==========================================
// GAME_OVER / WAVE_CLEAR
// ==========================================
void GameManager::UpdateEndScreen() {
    // Dong ho rieng cua man hinh ket thuc - RenderSystem dung de chay so dan trong bang
    // tong ket (xem DrawEndScreen). Dat o day thay vi trong Run() de no chi chay khi that
    // su dang o man hinh nay va khong bi dong bang boi transition fade.
    endScreenTimer += GetFrameTime();
    MenuInput input = InputSystem::PollMenu(settings);

    if (state == GameState::WAVE_CLEAR) {
        // NANG CAP SAU WAVE (Track C - Nguoi 2, Phase 3): 3 the, Trai/Phai hoac chuot chon.
        int dir = input.Right ? 1 : (input.Left ? -1 : 0);
        if (dir != 0) MoveSelection(selectedUpgrade, dir, UPGRADE_TYPE_COUNT);
        SelectWithPointer(selectedUpgrade, UPGRADE_TYPE_COUNT, UiLayout::UpgradeCard);

        if (input.Confirm || Clicked(ui.pointer, UiLayout::UpgradeCard(selectedUpgrade))) {
            // Ap dung nang cap dang chon TRUOC KHI sang wave ke. gm.wave o day DA duoc ++
            // TU TRUOC (xem PhysicsSystem::UpdateEnemies()/UpdatePlaying() nhanh BOSS
            // DEFEAT) - tuc DA LA wave SAP choi, nen check "wave boss sap toi" dung thang
            // duoc, khong can suy nguoc. Wave boss: goi ApplyRunUpgrade() THEM 1 lan cho
            // CUNG 1 luot chon (2 lan tong) thay vi them pool/loai rieng - xem upgrade_types.h.
            UpgradeType chosen = (UpgradeType)selectedUpgrade;
            player.ApplyRunUpgrade(chosen);
            if (wave % Config::BOSS_WAVE_INTERVAL == 0) player.ApplyRunUpgrade(chosen);
            audio.PlayUiConfirm();
            InitLevel(false); // Giu diem/mang, sang wave ke tiep voi do kho cao hon
            RequestTransition(GameState::PLAYING);
            return;
        }
        if (input.Restart) {
            InitLevel(true); // Choi lai tu dau (wave 1, reset diem/mang)
            RequestTransition(GameState::PLAYING);
        }
        return;
    }

    // GAME_OVER: 2 nut [MENU] [CHOI LAI]
    int dir = input.Right ? 1 : (input.Left ? -1 : 0);
    if (dir != 0) MoveSelection(ui.endIndex, dir, 2);
    SelectWithPointer(ui.endIndex, 2, UiLayout::GameOverButton);
    bool activate = input.Confirm || Clicked(ui.pointer, UiLayout::GameOverButton(ui.endIndex));
    if (input.Restart || (activate && ui.endIndex == 1)) {
        audio.PlayUiConfirm();
        InitLevel(true);
        RequestTransition(GameState::PLAYING);
    } else if (activate) {
        audio.PlayUiConfirm();
        RequestTransition(GameState::MENU);
    }
}
