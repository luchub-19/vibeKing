// ==========================================
// CAC MAN GUI (nang cap GUI) - menu chinh, hangar, bang xep hang, thanh tuu, huong dan, attract,
// cai dat, pause, hop xac nhan, man ket thuc. Tach khoi render_system.cpp (giu phan ve THE GIOI
// GAME + HUD) vi day la 1 khoi lon, doc lap: chi doc gm.ui + du lieu meta, khong cham pool dich.
//
// PHONG CACH: "retro arcade" theo lua chon cua nguoi dung - khung pixel goc bac thang, con tro
// tam giac nhay, chu Bungee (tieu de) + VT323 (chu thuong, kieu man CRT terminal), chu go tung ky
// tu, glitch tach RGB khi vao man. Xem retro_ui.h (nguon cam hung) va docs/GUI_UPGRADE.md.
//
// Vi tri nut: ui_layout.h (chung voi GameManager - bam chuot dung cho nhin thay).
// Ham VE thuan - kiem bang anh chup: scripts/capture_showcase.sh (canh menu/settings/...).
// ==========================================
#include "render_system.h"
#include "game_manager.h"
#include "retro_ui.h"
#include "ui_layout.h"
#include "ui_nav.h"
#include "settings_menu.h"
#include "meta_progress.h"
#include "upgrade_types.h"
#include "palette.h"
#include <cstdio>

namespace {
    using RetroUi::PixelFrame;
    using RetroUi::TextCentered;

    constexpr float CX = Config::SCREEN_W / 2.0f;

    Color PanelFill(float alpha = Config::HUD_PANEL_ALPHA) {
        Color c = Palette::UiPanelFill;
        c.a = (unsigned char)(255.0f * alpha);
        return c;
    }

    // 1 MUC MENU: dang chon = khung pixel sang + con tro tam giac 2 ben nhay ra vao + chu NHUN tung
    // ky tu (Balatro); khong chon = chu mo, khong khung (bo cuc "danh sach lenh" kieu Dragon Quest).
    void MenuEntry(const Font& font, Rectangle r, const char* label, bool selected, float time, bool reduce,
                   float size = 26.0f, Color accent = Palette::UiAccent) {
        const float ty = r.y + (r.height - size) / 2.0f;
        if (!selected) {
            TextCentered(font, label, r.x + r.width / 2.0f, ty, size, Fade(Palette::UiText, 0.55f));
            return;
        }
        PixelFrame(r, Fade(accent, 0.12f), accent);
        float nudge = reduce ? 0.0f : 3.0f * (0.5f + 0.5f * sinf(time * 8.0f));
        RetroUi::PixelArrow({ r.x + 22.0f + nudge, r.y + r.height / 2.0f }, 14.0f, accent, 1);
        RetroUi::PixelArrow({ r.x + r.width - 22.0f - nudge, r.y + r.height / 2.0f }, 14.0f, accent, -1);
        RetroUi::WavyTextCentered(font, label, r.x + r.width / 2.0f, ty, size, Palette::UiText, time, reduce ? 0.0f : 1.6f);
    }

    // O lua chon / pill: dang chon = nen mau nhan + chu toi (dao mau - de doc nhat tren nen sao).
    void OptionPill(const Font& font, Rectangle r, const char* label, bool chosen, bool rowActive, float size = 21.0f) {
        Color accent = rowActive ? Palette::UiAccent : Fade(Palette::UiAccent, 0.55f);
        if (chosen) PixelFrame(r, accent, accent, 4.0f);
        else PixelFrame(r, PanelFill(0.5f), Fade(Palette::UiPanelEdge, rowActive ? 1.0f : 0.6f), 4.0f);
        Color text = chosen ? Palette::Background : (rowActive ? Fade(Palette::UiText, 0.8f) : Palette::UiDim);
        TextCentered(font, label, r.x + r.width / 2.0f, r.y + (r.height - size) / 2.0f, size, text);
    }

    // So diem 6 chu so kieu bang diem arcade
    const char* Score6(int v) { return TextFormat("%06d", v < 0 ? 0 : (v > 999999 ? 999999 : v)); }

    Str DifficultyStr(int i) {
        const Str s[3] = { Str::DiffEasy, Str::DiffNormal, Str::DiffHard };
        return s[i < 0 ? 0 : (i > 2 ? 2 : i)];
    }
    Str LoadoutStr(LoadoutType t) {
        switch (t) {
            case LoadoutType::Vanguard:   return Str::LoadoutVanguard;
            case LoadoutType::Overcharge: return Str::LoadoutOvercharge;
            default:                      return Str::LoadoutStandard;
        }
    }

    // DANH SACH DICH - 1 nguon cho trang "Ke dich" (Huong dan) lan "Bang diem thuong" (Attract).
    // Diem doc THANG tu SCORE_VALUE (balance.json ghi de duoc) - khong chep tay so lieu vao chuoi.
    struct EnemyInfo { const Texture2D* tex; Color tint; Str name; Str desc; int points; int descArg; float w, h; };
    int BuildEnemyList(const SpriteSheet& sp, EnemyInfo out[9]) {
        out[0] = { &sp.ufo,         Palette::Ufo,      Str::EnemyUfo,      Str::EnemyUfoDesc,      -1, 0, 46.0f, 20.0f };
        out[1] = { &sp.kamikaze,    Palette::Kamikaze, Str::EnemyKamikaze, Str::EnemyKamikazeDesc, KamikazeEnemy::SCORE_VALUE, 0, 30.0f, 26.0f };
        out[2] = { &sp.weaver,      Palette::Weaver,   Str::EnemyWeaver,   Str::EnemyWeaverDesc,   WeaverEnemy::SCORE_VALUE, 0, 36.0f, 24.0f };
        out[3] = { &sp.bomber,      Palette::Bomber,   Str::EnemyBomber,   Str::EnemyBomberDesc,   BomberEnemy::SCORE_VALUE, 0, 40.0f, 24.0f };
        out[4] = { &sp.warden,      Palette::Warden,   Str::EnemyWarden,   Str::EnemyWardenDesc,   WardenEnemy::SCORE_VALUE, WardenEnemy::HP, 36.0f, 26.0f };
        out[5] = { &sp.tankyAlien,  Palette::Tanky,    Str::EnemyTanky,    Str::EnemyTankyDesc,    TankyEnemy::SCORE_VALUE, TankyEnemy::HP, 38.0f, 26.0f };
        out[6] = { &sp.medic,       Palette::Medic,    Str::EnemyMedic,    Str::EnemyMedicDesc,    MedicEnemy::SCORE_VALUE, 0, 32.0f, 24.0f };
        out[7] = { &sp.zigzagAlien, Palette::Zigzag,   Str::EnemyZigzag,   Str::EnemyZigzagDesc,   ZigzagEnemy::SCORE_VALUE, 0, 34.0f, 22.0f };
        out[8] = { &sp.basicAlien,  Palette::BasicA,   Str::EnemyBasic,    Str::EnemyBasicDesc,    BasicEnemy::SCORE_VALUE, 0, 34.0f, 22.0f };
        return 9;
    }

    // HANG DICH DIEU HANH ngang day man (attract + menu) - 7 con "buoc" theo nhip 2 khung kieu
    // Space Invaders goc (nhay 4px moi 0.5s), khong troi muot - dung chat 1978.
    void MarchingSquad(const SpriteSheet& sp, float y, float time, float alpha) {
        const Texture2D* texs[7] = { &sp.basicAlien, &sp.zigzagAlien, &sp.tankyAlien, &sp.warden, &sp.medic, &sp.zigzagAlien, &sp.basicAlien };
        const Color cols[7] = { Palette::BasicA, Palette::Zigzag, Palette::Tanky, Palette::Warden, Palette::Medic, Palette::Zigzag, Palette::BasicB };
        float step = floorf(time * 2.0f);
        float span = Config::SCREEN_W - 7.0f * 44.0f;
        float phase = fmodf(step * 8.0f, span * 2.0f);
        float x0 = phase < span ? phase : span * 2.0f - phase; // Di qua roi di lai, nhu doi hinh cham bien
        for (int i = 0; i < 7; i++) {
            float bob = ((int)step + i) % 2 == 0 ? 0.0f : 3.0f;
            DrawSprite(*texs[i], { x0 + (float)i * 44.0f + 4.0f, y + bob, 32.0f, 22.0f }, Fade(cols[i], alpha));
        }
    }

    // BANG TOP 10 - dung chung man BANG XEP HANG va trang 2 cua attract. `reveal` = giay tu luc
    // hien bang: moi dong truot vao tu phai lech nhau 0.06s (kieu bang diem hien dan tren may arcade).
    void ScoreTable(const Font& font, const Leaderboard& lb, float top, float reveal, float time, bool reduce) {
        const auto& entries = lb.GetEntries();
        const float left = 170.0f, right = 630.0f;
        DrawTextEx(font, Tr(Str::ColRank), { left, top }, 22.0f, 1.0f, Palette::UiAccent);
        RetroUi::TextRight(font, Tr(Str::ColScore), 440.0f, top, 22.0f, Palette::UiAccent);
        RetroUi::TextRight(font, Tr(Str::ColWave), 530.0f, top, 22.0f, Palette::UiAccent);
        for (float x = left; x < right; x += 8.0f) DrawRectangleRec({ x, top + 26.0f, 4.0f, 2.0f }, Fade(Palette::UiAccent, 0.5f));
        if (entries.empty()) {
            TextCentered(font, Tr(Str::NoRecordsYet), CX, top + 140.0f, 24.0f, Palette::UiDim);
            return;
        }
        for (size_t i = 0; i < entries.size(); i++) {
            float k = reduce ? 1.0f : EaseOutCubic(reveal - (float)i * 0.06f, 0.25f);
            if (k <= 0.0f) continue;
            float y = top + 38.0f + (float)i * 31.0f;
            float dx = (1.0f - k) * 120.0f;
            // #1 vang + nhun, top 3 trang, con lai xam - mat bat ngay "ai dang dung dau"
            Color c = (i == 0) ? Palette::UiAccent : (i < 3 ? Palette::UiText : Fade(Palette::UiText, 0.7f));
            c = Fade(c, k);
            const char* rank = TextFormat("%2d", (int)i + 1);
            if (i == 0) RetroUi::WavyText(font, rank, { left + dx, y }, 26.0f, 1.0f, c, time, reduce ? 0.0f : 1.5f);
            else DrawTextEx(font, rank, { left + dx, y }, 26.0f, 1.0f, c);
            RetroUi::TextRight(font, Score6(entries[i].score), 440.0f + dx, y, 26.0f, c);
            RetroUi::TextRight(font, TextFormat("%d", entries[i].wave), 530.0f + dx, y, 26.0f, c);
            if (entries[i].assisted) DrawTextEx(font, Tr(Str::AssistTag), { 548.0f + dx, y + 4.0f }, 19.0f, 1.0f, Fade(Palette::Weaver, k));
        }
    }

    // TRANG DIEU KHIEN - phim THAT dang gan (doi phim trong Cai dat la trang nay doi theo).
    void ControlsPage(const Font& font, const Settings& s, float top) {
        struct Row { Str label; const char* keys[4]; };
        const Row rows[] = {
            { Str::CtrlMove,       { InputSystem::KeyName(s.keyMoveLeft), InputSystem::KeyName(s.keyMoveRight), "LEFT", "RIGHT" } },
            { Str::CtrlShoot,      { InputSystem::KeyName(s.keyShoot), nullptr, nullptr, nullptr } },
            { Str::CtrlPause,      { InputSystem::KeyName(s.keyPause), "ESC", nullptr, nullptr } },
            { Str::CtrlRestart,    { "R", nullptr, nullptr, nullptr } },
            { Str::CtrlFullscreen, { "F11", nullptr, nullptr, nullptr } },
            { Str::CtrlProfiler,   { "F3", nullptr, nullptr, nullptr } },
        };
        float y = top;
        for (const Row& r : rows) {
            float x = 110.0f;
            for (const char* k : r.keys) {
                if (k == nullptr) continue;
                float w = fmaxf(44.0f, RetroUi::TextWidth(font, k, 20.0f) + 22.0f);
                RetroUi::KeyCap(font, k, { x, y, w, 34.0f }, Palette::UiAccent, Palette::UiText, 20.0f);
                x += w + 8.0f;
            }
            DrawTextEx(font, Tr(r.label), { 430.0f, y + 6.0f }, 24.0f, 1.0f, Palette::UiText);
            y += 44.0f;
        }
        TextCentered(font, Tr(Str::CtrlGamepad), CX, y + 14.0f, 19.0f, Palette::UiDim);
        TextCentered(font, Tr(Str::CtrlMouse), CX, y + 40.0f, 19.0f, Palette::UiDim);
    }
}

// ==========================================
// KHUNG CHUNG CUA MAN CON: nut QUAY LAI + tieu de glitch + vach cham
// ==========================================
void RenderSystem::DrawScreenHeader(const GameManager& gm, const char* title) {
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    Rectangle back = UiLayout::BackButton();
    bool hover = UiNav::PointIn(gm.ui.pointer.pos, back);
    Color c = hover ? Palette::UiAccent : Palette::UiDim;
    PixelFrame(back, hover ? Fade(Palette::UiAccent, 0.12f) : PanelFill(0.6f), c, 4.0f);
    RetroUi::PixelArrow({ back.x + 12.0f, back.y + back.height / 2.0f }, 12.0f, c, -1);
    DrawTextEx(gm.gameFont, Tr(Str::Back), { back.x + 24.0f, back.y + 5.0f }, 21.0f, 1.0f, hover ? Palette::UiText : Palette::UiDim);

    RetroUi::GlitchTitle(gm.titleFont, title, { CX, 56.0f }, 36.0f, Palette::UiAccent,
                         RetroUi::GlitchAmount(gm.ui.screenTimer, t, rf), t);
    for (float x = 120.0f; x < Config::SCREEN_W - 120.0f; x += 10.0f) {
        DrawRectangleRec({ x, 80.0f, 6.0f, 2.0f }, Fade(Palette::UiAccent, 0.35f));
    }
}

void RenderSystem::DrawFooterHint(const GameManager& gm, const char* hint) {
    TextCentered(gm.gameFont, hint, CX, 574.0f, 18.0f, Palette::UiDim);
}

// ==========================================
// MENU CHINH
// ==========================================
void RenderSystem::DrawMenu(const GameManager& gm) {
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    const Font& f = gm.gameFont;
    RetroUi::ScanSweep(t, (float)Config::SCREEN_W, (float)Config::SCREEN_H, rf);

    // Dau trang kieu may arcade: HI-SCORE giua tren cung
    TextCentered(f, Tr(Str::HiScore), CX, 4.0f, 19.0f, Palette::UiDanger);
    TextCentered(f, Score6(gm.leaderboard.GetTopScore()), CX, 20.0f, 26.0f, Palette::UiText);

    // Hang dich nhay nhot tren logo - sprite THAT, mau THAT cua gameplay
    const Texture2D* texs[5] = { &gm.sprites.basicAlien, &gm.sprites.zigzagAlien, &gm.sprites.tankyAlien, &gm.sprites.zigzagAlien, &gm.sprites.basicAlien };
    const Color cols[5] = { Palette::BasicA, Palette::Zigzag, Palette::Tanky, Palette::Zigzag, Palette::BasicB };
    for (int i = 0; i < 5; i++) {
        float bob = rf ? 0.0f : sinf(t * 2.5f + (float)i * 0.6f) * 4.0f;
        DrawSprite(*texs[i], { CX - 2.0f * 46.0f + (float)i * 46.0f - 16.0f, 58.0f + bob, 32.0f, 24.0f }, cols[i]);
    }

    // Logo 2 tang: "HARDCORE" vang (NONG - loi canh bao) tren "SPACE INVADERS" xanh neon cua tau
    float glitch = RetroUi::GlitchAmount(gm.ui.screenTimer, t, rf);
    RetroUi::GlitchTitle(gm.titleFont, "HARDCORE", { CX, 116.0f }, 30.0f, Palette::UiAccent, glitch, t);
    RetroUi::GlitchTitle(gm.titleFont, "SPACE INVADERS", { CX, 158.0f }, 50.0f, Palette::PlayerShip, glitch, t + 0.37f);
    TextCentered(f, Tr(Str::Tagline), CX, 194.0f, 21.0f, Palette::UiDim, 4.0f);

    const Str items[UiLayout::MAIN_MENU_ITEMS] = { Str::MenuPlay, Str::MenuLeaderboard, Str::MenuAchievements,
                                                   Str::MenuHowTo, Str::MenuSettings, Str::MenuQuit };
    for (int i = 0; i < UiLayout::MAIN_MENU_ITEMS; i++) {
        MenuEntry(f, UiLayout::MainMenuItem(i), Tr(items[i]), i == gm.ui.menuIndex, t, rf, 27.0f,
                  i == UiLayout::MAIN_MENU_ITEMS - 1 ? Palette::UiDanger : Palette::UiAccent);
    }
    // Thanh tuu: so da mo canh muc
    {
        Rectangle r = UiLayout::MainMenuItem(2);
        DrawTextEx(f, TextFormat("%d/%d", gm.achievements.UnlockedCount(), ACHIEVEMENT_COUNT),
                   { r.x + r.width + 10.0f, r.y + 8.0f }, 20.0f, 1.0f, Palette::UiDim);
    }

    if (const char* line = gm.UiTypedLine()) {
        RetroUi::Typewriter(f, line, { CX, 488.0f }, 22.0f, Fade(Palette::UiText, 0.85f), gm.ui.rowTimer, TYPEWRITER_CPS, t, rf, true);
    }

    // PRESS ENTER / INSERT COIN luan phien moi 2.5s, nhap nhay 1Hz (tat nhap nhay khi giam nhap nhay)
    const char* call = (fmodf(t, 5.0f) < 2.5f) ? Tr(Str::PressEnter) : Tr(Str::InsertCoin);
    if (UiNav::BlinkOn(t, 1.0f, rf)) TextCentered(f, call, CX, 526.0f, 26.0f, Palette::UiAccent, 3.0f);

    DrawFooterHint(gm, Tr(Str::HintNavigate));
    RetroUi::TextRight(f, TextFormat(Tr(Str::CreditFmt), gm.metaProgress.GetCurrency()), (float)Config::SCREEN_W - 10.0f, 552.0f, 19.0f, Palette::UiDim);
}

// ==========================================
// HANGAR
// ==========================================
void RenderSystem::DrawHangar(const GameManager& gm) {
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    const Font& f = gm.gameFont;
    DrawScreenHeader(gm, Tr(Str::HangarTitle));
    RetroUi::TextRight(f, TextFormat("%d CR", gm.metaProgress.GetCurrency()), (float)Config::SCREEN_W - 18.0f, 18.0f, 24.0f, Palette::UiAccent);

    const int row = gm.ui.hangarRow;
    auto rowLabel = [&](const char* text, float x, float y, bool active) {
        if (active) RetroUi::PixelArrow({ x - 8.0f, y + 12.0f }, 12.0f, Palette::UiAccent, 1);
        DrawTextEx(f, text, { x, y }, 24.0f, 1.0f, active ? Palette::UiAccent : Palette::UiDim);
    };

    // --- Do kho ---
    rowLabel(Tr(Str::HangarDifficulty), 144.0f, 100.0f, row == 0);
    for (int i = 0; i < 3; i++) {
        OptionPill(f, UiLayout::HangarDifficultyPill(i), Tr(DifficultyStr(i)), (int)gm.difficulty == i, row == 0, 24.0f);
    }
    const Str diffDesc[3] = { Str::DiffDescEasy, Str::DiffDescNormal, Str::DiffDescHard };
    if (row == 0) RetroUi::Typewriter(f, Tr(diffDesc[(int)gm.difficulty]), { CX, 182.0f }, 21.0f, Palette::UiText, gm.ui.rowTimer, TYPEWRITER_CPS, t, rf, true);
    else TextCentered(f, Tr(diffDesc[(int)gm.difficulty]), CX, 182.0f, 21.0f, Palette::UiDim);

    // --- Loadout: 3 the co hinh tau that ---
    rowLabel(Tr(Str::HangarLoadout), 80.0f, 220.0f, row == 1);
    const LoadoutType types[3] = { LoadoutType::Standard, LoadoutType::Vanguard, LoadoutType::Overcharge };
    for (int i = 0; i < 3; i++) {
        Rectangle r = UiLayout::HangarLoadoutCard(i);
        const bool sel = gm.selectedLoadout == i;
        const bool unlocked = types[i] == LoadoutType::Standard || gm.metaProgress.IsUnlocked(types[i]);
        const float lift = (sel && row == 1 && !rf) ? 3.0f * sinf(t * 4.0f) : 0.0f;
        r.y -= sel ? 4.0f + lift : 0.0f;
        Color border = sel ? (row == 1 ? Palette::UiAccent : Fade(Palette::UiAccent, 0.6f)) : Palette::UiPanelEdge;
        PixelFrame(r, sel ? Fade(Palette::UiAccent, 0.10f) : PanelFill(0.7f), border);
        Color ship = unlocked ? Palette::PlayerShip : Fade(Palette::UiDim, 0.6f);
        DrawSprite(gm.sprites.player, { r.x + r.width / 2.0f - 26.0f, r.y + 12.0f, 52.0f, 38.0f }, ship);
        if (!unlocked) { // O khoa pixel de len tau
            float lx = r.x + r.width / 2.0f - 8.0f, ly = r.y + 26.0f;
            DrawRectangleLinesEx({ lx + 3.0f, ly - 8.0f, 10.0f, 10.0f }, 2.0f, Palette::UiText);
            DrawRectangleRec({ lx, ly, 16.0f, 12.0f }, Palette::UiText);
        }
        TextCentered(f, Tr(LoadoutStr(types[i])), r.x + r.width / 2.0f, r.y + 56.0f, 24.0f, sel ? Palette::UiText : Palette::UiDim);
        const char* status;
        Color sc;
        if (types[i] == LoadoutType::Standard) { status = Tr(Str::LoadoutFree); sc = Fade(Palette::UiText, 0.6f); }
        else if (unlocked)                     { status = Tr(Str::LoadoutReady); sc = Palette::UiSuccess; }
        else { status = TextFormat(Tr(Str::LoadoutCostFmt), gm.metaProgress.GetCurrency(), GetLoadoutUnlockCost(types[i])); sc = Palette::UiDim; }
        TextCentered(f, status, r.x + r.width / 2.0f, r.y + 82.0f, 21.0f, sc);
    }
    const Str loDesc[3] = { Str::LoadoutDescStandard, Str::LoadoutDescVanguard, Str::LoadoutDescOvercharge };
    const int lo = gm.selectedLoadout;
    if (row == 1) RetroUi::Typewriter(f, Tr(loDesc[lo]), { CX, 374.0f }, 21.0f, Palette::UiText, gm.ui.rowTimer, TYPEWRITER_CPS, t, rf, true);
    else TextCentered(f, Tr(loDesc[lo]), CX, 374.0f, 21.0f, Palette::UiDim);
    if (types[lo] != LoadoutType::Standard && !gm.metaProgress.IsUnlocked(types[lo])) {
        int cost = GetLoadoutUnlockCost(types[lo]);
        int have = gm.metaProgress.GetCurrency();
        const char* hint = have >= cost ? TextFormat(Tr(Str::LoadoutUnlockHintFmt), cost) : TextFormat(Tr(Str::LoadoutNeedFmt), cost - have);
        TextCentered(f, hint, CX, 400.0f, 21.0f, have >= cost ? Palette::UiAccent : Palette::UiDanger);
        TextCentered(f, Tr(Str::LoadoutLockedPlaysStandard), CX, 424.0f, 19.0f, Palette::UiDim);
    }

    // --- Nut XUAT KICH: vien dap theo nhip, chu Bungee ---
    Rectangle launch = UiLayout::HangarLaunch();
    const bool active = row == 2;
    float pulse = rf ? 1.0f : 0.5f + 0.5f * sinf(t * 4.0f);
    PixelFrame(launch, active ? Fade(Palette::UiAccent, 0.18f + 0.1f * pulse) : PanelFill(0.7f),
               active ? Palette::UiAccent : Palette::UiPanelEdge, 8.0f, 3.0f);
    if (active) {
        RetroUi::PixelArrow({ launch.x + 26.0f + 3.0f * pulse, launch.y + launch.height / 2.0f }, 18.0f, Palette::UiAccent, 1);
        RetroUi::PixelArrow({ launch.x + launch.width - 26.0f - 3.0f * pulse, launch.y + launch.height / 2.0f }, 18.0f, Palette::UiAccent, -1);
    }
    DrawNeonText(gm.titleFont, Tr(Str::HangarLaunch), { CX, launch.y + launch.height / 2.0f }, 26.0f,
                 active ? Palette::UiText : Palette::UiDim, 1.0f);

    DrawFooterHint(gm, Tr(Str::HintNavigate));
}

// ==========================================
// BANG XEP HANG
// ==========================================
void RenderSystem::DrawLeaderboard(const GameManager& gm) {
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    DrawScreenHeader(gm, Tr(Str::LeaderboardTitle));
    PixelFrame({ 140.0f, 98.0f, 520.0f, 382.0f }, PanelFill(), Palette::UiPanelEdge);
    ScoreTable(gm.gameFont, gm.leaderboard, 112.0f, gm.ui.screenTimer, t, rf);
    bool anyAssist = false;
    for (const auto& e : gm.leaderboard.GetEntries()) anyAssist = anyAssist || e.assisted;
    if (anyAssist) TextCentered(gm.gameFont, Tr(Str::AssistLegend), CX, 494.0f, 19.0f, Palette::UiDim);
    DrawFooterHint(gm, Tr(Str::HintNavigate));
}

// ==========================================
// THANH TUU
// ==========================================
void RenderSystem::DrawAchievements(const GameManager& gm) {
    const Font& f = gm.gameFont;
    const float t = (float)GetTime();
    DrawScreenHeader(gm, Tr(Str::AchievementsTitle));
    TextCentered(f, TextFormat(Tr(Str::AchievementsSummaryFmt), gm.achievements.UnlockedCount(), ACHIEVEMENT_COUNT,
                               gm.achievements.GetLifetimeKills()), CX, 88.0f, 20.0f, Palette::UiDim);

    const float cardX = 70.0f, cardW = (float)Config::SCREEN_W - 140.0f, cardH = 50.0f, gap = 5.0f;
    float y = 114.0f;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        AchievementId id = (AchievementId)i;
        const AchievementDescriptor& d = GetAchievementDescriptor(id);
        bool done = gm.achievements.IsUnlocked(id);
        // The hien dan tu tren xuong khi vao man
        float k = gm.settings.graphics.reduceFlashing ? 1.0f : EaseOutCubic(gm.ui.screenTimer - (float)i * 0.04f, 0.2f);
        Rectangle r{ cardX - (1.0f - k) * 40.0f, y, cardW, cardH };
        PixelFrame(r, PanelFill(0.75f * k), Fade(done ? Palette::UiSuccess : Palette::UiPanelEdge, k), 6.0f);
        // Huy hieu: o vuong pixel - sang (mau thanh cong) neu da mo, rong neu chua
        Rectangle badge{ r.x + 12.0f, r.y + 13.0f, 24.0f, 24.0f };
        if (done) {
            PixelFrame(badge, Palette::UiSuccess, Palette::UiSuccess, 4.0f);
            DrawRectangleRec({ badge.x + 6.0f, badge.y + 12.0f, 4.0f, 4.0f }, Palette::Background); // Dau tich pixel
            DrawRectangleRec({ badge.x + 10.0f, badge.y + 15.0f, 4.0f, 4.0f }, Palette::Background);
            DrawRectangleRec({ badge.x + 14.0f, badge.y + 11.0f, 4.0f, 4.0f }, Palette::Background);
            DrawRectangleRec({ badge.x + 17.0f, badge.y + 7.0f, 4.0f, 4.0f }, Palette::Background);
        } else {
            PixelFrame(badge, Color{ 0, 0, 0, 0 }, Palette::UiDim, 4.0f);
        }
        DrawTextEx(f, Tr(d.name), { r.x + 48.0f, r.y + 3.0f }, 23.0f, 1.0f, Fade(done ? Palette::UiText : Palette::UiDim, k));
        DrawTextEx(f, TextFormat(Tr(d.descriptionFmt), d.threshold), { r.x + 48.0f, r.y + 25.0f }, 19.0f, 1.0f, Fade(Palette::UiDim, k));

        const float rightX = r.x + r.width - 72.0f;
        if (done) {
            TextCentered(f, Tr(Str::AchievementUnlockedState), rightX, r.y + 4.0f, 19.0f, Palette::UiSuccess);
        } else if (id == AchievementId::FirstContact || id == AchievementId::Exterminator) {
            int have = gm.achievements.GetLifetimeKills();
            TextCentered(f, TextFormat("%d/%d", have < d.threshold ? have : d.threshold, d.threshold), rightX, r.y + 4.0f, 19.0f, Palette::UiDim);
        }
        TextCentered(f, TextFormat("+%d CR", d.rewardCurrency), rightX, r.y + 25.0f, 20.0f,
                     done ? Fade(Palette::UiAccent, 0.5f) : Palette::UiAccent);
        y += cardH + gap;
    }
    (void)t;
    DrawFooterHint(gm, Tr(Str::HintNavigate));
}

// TOAST "MO KHOA THANH TUU": the nho truot xuong giua-tren, khung pixel vien vang.
void RenderSystem::DrawAchievementToast(const GameManager& gm) {
    if (gm.toastQueue.empty()) return;
    const AchievementDescriptor& d = GetAchievementDescriptor(gm.toastQueue.front());
    float t = gm.toastTimer;
    float remaining = Config::ACHIEVEMENT_TOAST_DURATION - t;
    float slide = 1.0f;
    if (t < Config::ACHIEVEMENT_TOAST_SLIDE) slide = t / Config::ACHIEVEMENT_TOAST_SLIDE;
    else if (remaining < Config::ACHIEVEMENT_TOAST_SLIDE) slide = remaining / Config::ACHIEVEMENT_TOAST_SLIDE;
    if (slide < 0.0f) slide = 0.0f;
    slide = 1.0f - (1.0f - slide) * (1.0f - slide);

    const float w = 380.0f, h = 54.0f;
    const float restY = Config::HUD_TOP_BAND_H + 10.0f;
    float y = -h + (restY + h) * slide;
    float x = ((float)Config::SCREEN_W - w) / 2.0f;
    // Alpha vua du doc chu nhung VAN thay hang dich phia sau - toast de len hang dich tren cung
    // vai giay; voi game "hardcore" che kin la bat loi that.
    Color fill = Palette::UiPanelFill;
    fill.a = 170;
    PixelFrame({ x, y, w, h }, fill, Palette::UiAccent);
    DrawTextEx(gm.gameFont, Tr(Str::AchievementUnlockedTag), { x + 14.0f, y + 4.0f }, 18.0f, 1.0f, Palette::UiAccent);
    DrawTextEx(gm.gameFont, Tr(d.name), { x + 14.0f, y + 22.0f }, 26.0f, 1.0f, Palette::UiText);
    RetroUi::TextRight(gm.gameFont, TextFormat("+%d CR", d.rewardCurrency), x + w - 14.0f, y + 18.0f, 24.0f, Palette::UiAccent);
}

// ==========================================
// HUONG DAN - 4 trang
// ==========================================
void RenderSystem::DrawHowTo(const GameManager& gm) {
    const Font& f = gm.gameFont;
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    DrawScreenHeader(gm, Tr(Str::HowToTitle));

    const Str tabs[UiLayout::HOWTO_PAGES] = { Str::HowToPageControls, Str::HowToPageEnemies, Str::HowToPagePowerUps, Str::HowToPageTips };
    for (int i = 0; i < UiLayout::HOWTO_PAGES; i++) {
        bool hover = UiNav::PointIn(gm.ui.pointer.pos, UiLayout::HowToTab(i));
        OptionPill(f, UiLayout::HowToTab(i), Tr(tabs[i]), i == gm.ui.howtoPage, hover || i == gm.ui.howtoPage, 21.0f);
    }
    // Trang moi: noi dung "go" vao - moi dong hien sau dong truoc 0.05s
    const float since = gm.ui.screenTimer;
    auto rowAlpha = [&](int i) { return rf ? 1.0f : EaseOutCubic(since - 0.4f - (float)i * 0.05f, 0.15f); };

    switch (gm.ui.howtoPage) {
        case 0:
            ControlsPage(f, gm.settings, 146.0f);
            break;
        case 1: {
            EnemyInfo list[9];
            int n = BuildEnemyList(gm.sprites, list);
            for (int i = 0; i < n; i++) {
                float a = rowAlpha(i);
                float y = 138.0f + (float)i * 46.0f;
                const EnemyInfo& e = list[i];
                DrawSprite(*e.tex, { 96.0f - e.w / 2.0f, y + 20.0f - e.h / 2.0f, e.w, e.h }, Fade(e.tint, a));
                DrawTextEx(f, Tr(e.name), { 140.0f, y }, 23.0f, 1.0f, Fade(e.tint, a));
                const char* desc = (e.name == Str::EnemyUfo) ? TextFormat(Tr(e.desc), Config::UFO_SCORE_MIN, Config::UFO_SCORE_MAX)
                                 : (e.descArg > 0 ? TextFormat(Tr(e.desc), e.descArg) : Tr(e.desc));
                DrawTextEx(f, desc, { 140.0f, y + 21.0f }, 19.0f, 1.0f, Fade(Palette::UiDim, a));
                const char* pts = e.points > 0 ? TextFormat(Tr(Str::PtsFmt), e.points) : "???";
                RetroUi::TextRight(f, pts, 720.0f, y + 8.0f, 23.0f, Fade(Palette::ScoreText, a));
            }
            break;
        }
        case 2: {
            struct Pu { const Texture2D* tex; Color tint; Str name; Str desc; float dur; };
            const Pu list[POWERUP_TYPE_COUNT] = {
                { &gm.sprites.iconRapidFire,  Palette::PowerUp,       Str::PuRapidFire,  Str::PuRapidFireDesc,  Config::POWERUP_RAPIDFIRE_DURATION },
                { &gm.sprites.iconShield,     Palette::ShieldBarrier, Str::PuShield,     Str::PuShieldDesc,     Config::POWERUP_SHIELD_DURATION },
                { &gm.sprites.iconPiercing,   Palette::PowerUp,       Str::PuPiercing,   Str::PuPiercingDesc,   Config::POWERUP_PIERCE_DURATION },
                { &gm.sprites.iconCleanser,   Palette::PowerUp,       Str::PuCleanser,   Str::PuCleanserDesc,   0.0f },
                { &gm.sprites.iconSpreadShot, Palette::PowerUp,       Str::PuSpreadShot, Str::PuSpreadShotDesc, Config::POWERUP_SPREADSHOT_DURATION },
                { &gm.sprites.iconOverdrive,  Palette::EnemyBullet,   Str::PuOverdrive,  Str::PuOverdriveDesc,  Config::POWERUP_OVERDRIVE_DURATION },
            };
            for (int i = 0; i < POWERUP_TYPE_COUNT; i++) {
                float a = rowAlpha(i);
                float y = 146.0f + (float)i * 64.0f;
                PixelFrame({ 80.0f, y - 4.0f, 640.0f, 56.0f }, PanelFill(0.55f * a), Fade(Palette::UiPanelEdge, a), 6.0f);
                float bob = rf ? 0.0f : sinf(t * 3.0f + (float)i) * 2.0f;
                DrawSprite(*list[i].tex, { 98.0f, y + 6.0f + bob, 32.0f, 32.0f }, Fade(list[i].tint, a));
                DrawTextEx(f, Tr(list[i].name), { 150.0f, y }, 24.0f, 1.0f, Fade(list[i].tint, a));
                DrawTextEx(f, Tr(list[i].desc), { 150.0f, y + 24.0f }, 20.0f, 1.0f, Fade(Palette::UiText, 0.8f * a));
                if (list[i].dur > 0.0f) RetroUi::TextRight(f, TextFormat("%.0fs", list[i].dur), 704.0f, y + 12.0f, 24.0f, Fade(Palette::UiDim, a));
            }
            break;
        }
        default: {
            const Str tips[6] = { Str::Tip1, Str::Tip2, Str::Tip3, Str::Tip4, Str::Tip5, Str::Tip6 };
            float start = 0.4f;
            for (int i = 0; i < 6; i++) {
                float y = 150.0f + (float)i * 62.0f;
                PixelFrame({ 60.0f, y - 6.0f, 36.0f, 36.0f }, Palette::UiAccent, Palette::UiAccent, 4.0f);
                TextCentered(f, TextFormat("%d", i + 1), 78.0f, y - 1.0f, 26.0f, Palette::Background);
                // Moi meo go lan luot (meo sau bat dau khi meo truoc da go xong)
                const char* tip = Tr(tips[i]);
                float elapsed = rf ? 99.0f : (since - start) ;
                RetroUi::Typewriter(f, tip, { 112.0f, y }, 22.0f, Palette::UiText, elapsed, TYPEWRITER_CPS * 2.0f, t, true);
                start += (float)strlen(tip) / (TYPEWRITER_CPS * 2.0f) * 0.6f;
            }
            break;
        }
    }
    DrawFooterHint(gm, Tr(Str::HintPages));
}

// ==========================================
// ATTRACT - trinh dien khi de yen menu (may arcade): bang diem thuong / top 10 / dieu khien
// ==========================================
void RenderSystem::DrawAttract(const GameManager& gm) {
    const Font& f = gm.gameFont;
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    RetroUi::ScanSweep(t, (float)Config::SCREEN_W, (float)Config::SCREEN_H, rf);
    TextCentered(f, Tr(Str::HiScore), CX, 4.0f, 19.0f, Palette::UiDanger);
    TextCentered(f, Score6(gm.leaderboard.GetTopScore()), CX, 20.0f, 26.0f, Palette::UiText);
    const float glitch = RetroUi::GlitchAmount(gm.ui.screenTimer, t, rf);
    const float since = gm.ui.screenTimer;

    switch (gm.ui.attractPage) {
        case 0: {
            // "*SCORE ADVANCE TABLE*" - kieu Space Invaders 1978: tung dong go ra "= 30 PTS"
            RetroUi::GlitchTitle(gm.titleFont, Tr(Str::AttractScoreTable), { CX, 86.0f }, 30.0f, Palette::UiAccent, glitch, t);
            EnemyInfo list[9];
            int n = BuildEnemyList(gm.sprites, list);
            for (int i = 0; i < n; i++) {
                float y = 130.0f + (float)i * 40.0f;
                float local = since - 0.3f - (float)i * 0.45f;
                if (local < 0.0f && !rf) continue;
                const EnemyInfo& e = list[i];
                DrawSprite(*e.tex, { 280.0f - e.w / 2.0f, y + 12.0f - e.h / 2.0f, e.w, e.h }, e.tint);
                const char* text = e.points > 0 ? TextFormat("= %s", TextFormat(Tr(Str::PtsFmt), e.points)) : TextFormat("= %s", Tr(Str::AttractMystery));
                RetroUi::Typewriter(f, text, { 320.0f, y }, 26.0f, Palette::UiText, rf ? 99.0f : local, 18.0f, t, true);
            }
            break;
        }
        case 1:
            RetroUi::GlitchTitle(gm.titleFont, Tr(Str::LeaderboardTitle), { CX, 86.0f }, 30.0f, Palette::UiAccent, glitch, t);
            ScoreTable(f, gm.leaderboard, 118.0f, since, t, rf);
            break;
        default:
            RetroUi::GlitchTitle(gm.titleFont, Tr(Str::AttractControls), { CX, 86.0f }, 30.0f, Palette::UiAccent, glitch, t);
            ControlsPage(f, gm.settings, 128.0f);
            break;
    }

    MarchingSquad(gm.sprites, 492.0f, t, 0.8f);
    if (UiNav::BlinkOn(t, 1.0f, rf)) TextCentered(f, Tr(Str::InsertCoin), CX, 530.0f, 30.0f, Palette::UiAccent, 4.0f);
    TextCentered(f, Tr(Str::PressEnter), CX, 566.0f, 20.0f, Palette::UiDim, 2.0f);
    RetroUi::TextRight(f, TextFormat(Tr(Str::CreditFmt), gm.metaProgress.GetCurrency()), (float)Config::SCREEN_W - 10.0f, 576.0f, 19.0f, Palette::UiDim);
}

// ==========================================
// CAI DAT - 4 tab
// ==========================================
void RenderSystem::DrawSettings(const GameManager& gm) {
    const Font& f = gm.gameFont;
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    // Mo tu PAUSE: tran dau van ve phia sau - phu toi gan kin de chu doc ro
    if (gm.ui.settingsReturn == GameState::PAUSED) {
        DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(Palette::Background, 0.88f));
    }
    DrawScreenHeader(gm, Tr(Str::SettingsTitle));

    for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
        bool hover = UiNav::PointIn(gm.ui.pointer.pos, UiLayout::SettingsTab(i));
        OptionPill(f, UiLayout::SettingsTab(i), Tr(SettingsTabLabel((SettingsTab)i)), i == gm.ui.settingsTab, true, hover ? 23.0f : 22.0f);
    }

    int count = 0;
    const SettingRowDef* rows = SettingsTabRows((SettingsTab)gm.ui.settingsTab, count);
    for (int r = 0; r < count; r++) {
        const SettingRowDef& d = rows[r];
        const bool active = r == gm.ui.settingsRow;
        Rectangle row = UiLayout::SettingsRow(r);
        // Dong hien dan khi vao man / doi tab
        float k = rf ? 1.0f : EaseOutCubic(gm.ui.screenTimer - (float)r * 0.035f, 0.18f);
        row.x -= (1.0f - k) * 30.0f;
        PixelFrame(row, active ? Fade(Palette::UiAccent, 0.10f) : PanelFill(0.45f * k), active ? Palette::UiAccent : Fade(Palette::UiPanelEdge, 0.7f * k), 6.0f);
        if (active) RetroUi::PixelArrow({ row.x + 18.0f + (rf ? 0.0f : 2.0f * sinf(t * 8.0f)), row.y + row.height / 2.0f }, 12.0f, Palette::UiAccent, 1);
        DrawTextEx(f, Tr(d.label), { row.x + 30.0f, row.y + (row.height - 25.0f) / 2.0f }, 25.0f, 1.0f,
                   Fade(active ? Palette::UiText : Fade(Palette::UiText, 0.7f), k));

        Rectangle ctl = UiLayout::SettingsControl(r);
        switch (d.kind) {
            case SettingKind::Choice: {
                int n = ChoiceCount(d.id);
                int chosen = ChoiceIndex(gm.settings, d.id);
                for (int i = 0; i < n; i++) {
                    OptionPill(f, UiLayout::SettingsOption(r, i, n), ChoiceLabel(d.id, i).c_str(), i == chosen, active,
                               n >= 4 ? 19.0f : 21.0f);
                }
                break;
            }
            case SettingKind::Slider: {
                float v = SliderValue(gm.settings, d.id);
                RetroUi::LedBar(UiLayout::SettingsSlider(r), v, 20, active ? Palette::Weaver : Fade(Palette::Weaver, 0.6f),
                                Fade(Palette::UiPanelEdge, 0.8f));
                RetroUi::TextRight(f, TextFormat("%d%%", (int)roundf(v * 100.0f)), ctl.x + ctl.width, ctl.y + 4.0f, 24.0f,
                                   active ? Palette::UiText : Palette::UiDim);
                break;
            }
            case SettingKind::Key: {
                int Settings::* field = KeyField(d.id);
                const bool waiting = active && gm.ui.rebinding;
                Rectangle cap{ ctl.x + ctl.width - 170.0f, ctl.y, 170.0f, ctl.height };
                if (waiting) {
                    if (UiNav::BlinkOn(t, 2.0f, rf)) {
                        PixelFrame(cap, Fade(Palette::UiAccent, 0.2f), Palette::UiAccent, 4.0f);
                    }
                    TextCentered(f, Tr(Str::SetKeyWaiting), cap.x + cap.width / 2.0f, cap.y + (cap.height - 21.0f) / 2.0f, 21.0f, Palette::UiAccent);
                } else {
                    RetroUi::KeyCap(f, InputSystem::KeyName(gm.settings.*field), cap, active ? Palette::UiAccent : Palette::UiPanelEdge,
                                    Palette::UiText, 22.0f);
                }
                break;
            }
            case SettingKind::Action: {
                Rectangle btn{ ctl.x + ctl.width - 170.0f, ctl.y, 170.0f, ctl.height };
                bool hover = UiNav::PointIn(gm.ui.pointer.pos, btn);
                PixelFrame(btn, (active || hover) ? Fade(Palette::UiDanger, 0.2f) : PanelFill(0.6f), (active || hover) ? Palette::UiDanger : Palette::UiPanelEdge, 4.0f);
                TextCentered(f, Tr(Str::SetResetAction), btn.x + btn.width / 2.0f, btn.y + (btn.height - 21.0f) / 2.0f, 21.0f, Palette::UiText);
                break;
            }
        }
    }

    // Khung mo ta dong dang chon - chu go tung ky tu (kieu hop thoai JRPG)
    Rectangle dp = UiLayout::SettingsDescPanel();
    PixelFrame(dp, PanelFill(0.8f), Palette::UiPanelEdge);
    if (gm.ui.rebinding && gm.ui.rebindRejectTimer > 0.0f) {
        DrawTextEx(f, Tr(Str::SetKeyRejected), { dp.x + 18.0f, dp.y + 20.0f }, 23.0f, 1.0f, Palette::UiDanger);
    } else if (const char* line = gm.UiTypedLine()) {
        RetroUi::Typewriter(f, line, { dp.x + 18.0f, dp.y + 20.0f }, 23.0f, Palette::UiText, gm.ui.rowTimer, TYPEWRITER_CPS, t, rf);
    }
    DrawFooterHint(gm, Tr(Str::HintTabs));
}

// ==========================================
// PAUSE
// ==========================================
void RenderSystem::DrawPauseMenu(const GameManager& gm) {
    const Font& f = gm.gameFont;
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(Palette::Background, 0.62f));
    Rectangle panel = UiLayout::PausePanel();
    PixelFrame(panel, PanelFill(0.92f), Palette::UiAccent, 8.0f, 3.0f);
    RetroUi::GlitchTitle(gm.titleFont, Tr(Str::PausedTitle), { CX, panel.y + 40.0f }, 34.0f, Palette::UiText,
                         RetroUi::GlitchAmount(gm.ui.screenTimer, t, rf), t);
    TextCentered(f, TextFormat(Tr(Str::PauseRunInfoFmt), gm.wave, gm.player.GetScore()), CX, panel.y + 66.0f, 21.0f, Palette::UiDim);
    const Str items[UiLayout::PAUSE_ITEMS] = { Str::PauseResume, Str::MenuSettings, Str::PauseRestart, Str::PauseQuitToMenu };
    for (int i = 0; i < UiLayout::PAUSE_ITEMS; i++) {
        MenuEntry(f, UiLayout::PauseItem(i), Tr(items[i]), i == gm.ui.pauseIndex, t, rf, 25.0f,
                  i >= 2 ? Palette::UiDanger : Palette::UiAccent);
    }
    DrawFooterHint(gm, Tr(Str::HintNavigate));
}

// ==========================================
// HOP XAC NHAN
// ==========================================
void RenderSystem::DrawConfirm(const GameManager& gm) {
    if (gm.ui.confirm == ConfirmKind::None) return;
    const Font& f = gm.gameFont;
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, Fade(Palette::Background, 0.6f));
    Rectangle p = UiLayout::ConfirmPanel();
    PixelFrame(p, PanelFill(0.97f), Palette::UiDanger, 8.0f, 3.0f);
    Str title = gm.ui.confirm == ConfirmKind::QuitGame ? Str::ConfirmQuitGame
              : (gm.ui.confirm == ConfirmKind::RestartRun ? Str::ConfirmRestart : Str::ConfirmQuitToMenu);
    DrawNeonText(gm.titleFont, Tr(title), { CX, p.y + 34.0f }, 26.0f, Palette::UiText, 1.0f);
    if (gm.ui.confirm != ConfirmKind::QuitGame) TextCentered(f, Tr(Str::ConfirmLoseProgress), CX, p.y + 62.0f, 21.0f, Palette::UiDim);
    for (int i = 0; i < 2; i++) {
        MenuEntry(f, UiLayout::ConfirmButton(i), Tr(i == 0 ? Str::Yes : Str::No), i == gm.ui.confirmIndex, t, rf, 25.0f,
                  i == 0 ? Palette::UiDanger : Palette::UiAccent);
    }
}

void RenderSystem::DrawFpsCounter(const GameManager& gm) {
    if (!gm.settings.showFps) return;
    const char* s = TextFormat("%d FPS", GetFPS());
    float w = RetroUi::TextWidth(gm.gameFont, s, 20.0f);
    DrawRectangleRec({ 4.0f, (float)Config::SCREEN_H - 24.0f, w + 10.0f, 20.0f }, Fade(Palette::Background, 0.7f));
    DrawTextEx(gm.gameFont, s, { 9.0f, (float)Config::SCREEN_H - 24.0f }, 20.0f, 1.0f, Palette::UiSuccess);
}

// ==========================================
// MAN KET THUC: WAVE_CLEAR (chon nang cap) / GAME_OVER (bang tong ket)
// ==========================================
void RenderSystem::DrawEndScreen(const GameManager& gm) {
    UICanvas canvas;
    const int centerX = Config::SCREEN_W / 2;
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    const Font& f = gm.gameFont;
    if (gm.state != GameState::WAVE_CLEAR) {
        DrawRunSummary(canvas, gm, centerX);
        return;
    }
    RetroUi::GlitchTitle(gm.titleFont, TextFormat(Tr(Str::WaveClearedFmt), gm.wave - 1), { CX, 196.0f }, 34.0f, Palette::UiSuccess,
                         RetroUi::GlitchAmount(gm.ui.screenTimer, t, rf), t);
    TextCentered(f, TextFormat(Tr(Str::ScoreFmt), gm.player.GetScore()), CX, 228.0f, 26.0f, Palette::UiText);

    // gm.wave DA duoc ++ TU TRUOC (xem GameManager::UpdateEndScreen()) - tuc DA LA wave SAP choi.
    if (gm.wave % Config::BOSS_WAVE_INTERVAL == 0) {
        RetroUi::WavyTextCentered(f, Tr(Str::BossWaveUpgradeBanner), CX, 262.0f, 22.0f, Palette::UiDanger, t, rf ? 0.0f : 1.5f);
    }
    for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
        const UpgradeTypeDescriptor& d = GetUpgradeTypeDescriptor((UpgradeType)i);
        const bool sel = i == gm.selectedUpgrade;
        Rectangle r = UiLayout::UpgradeCard(i);
        if (sel) r.y -= rf ? 4.0f : 4.0f + 2.0f * sinf(t * 4.0f); // The dang chon "nhac len"
        PixelFrame(r, sel ? Fade(Palette::UiAccent, 0.14f) : PanelFill(), sel ? Palette::UiAccent : Palette::UiPanelEdge);
        const float cx = r.x + r.width / 2.0f;
        if (sel) RetroUi::WavyTextCentered(f, Tr(d.name), cx, r.y + 8.0f, 25.0f, Palette::UiText, t, rf ? 0.0f : 1.4f);
        else TextCentered(f, Tr(d.name), cx, r.y + 8.0f, 25.0f, Palette::UiDim);
        TextCentered(f, Tr(d.description), cx, r.y + 38.0f, 20.0f, sel ? Palette::UiText : Palette::UiDim);
        int owned = gm.player.GetUpgradeStacks((UpgradeType)i);
        TextCentered(f, TextFormat(Tr(Str::UpgradeOwnedFmt), owned), cx, r.y + 64.0f, 19.0f, owned > 0 ? Palette::UiAccent : Palette::UiDim);
    }
    TextCentered(f, Tr(Str::UpgradeSelectHint), CX, 412.0f, 20.0f, Palette::UiDim);
}

// ==========================================
// BANG TONG KET RUN (GAME OVER). CHAY SO: diem/CR dem len tu 0 trong SUMMARY_COUNT_UP_DURATION
// giay dau - keo mat nguoi choi o lai bang du 1 nhip de THAY phan thuong (meta-progression vo
// hinh dung vao luc no tra thuong - ly do bang nay ton tai, xem lich su trong git log).
// ==========================================
void RenderSystem::DrawRunSummary(UICanvas& canvas, const GameManager& gm, int centerX) {
    const float t = (float)GetTime();
    const bool rf = gm.settings.graphics.reduceFlashing;
    const Font& f = gm.gameFont;
    float k = (Config::SUMMARY_COUNT_UP_DURATION > 0.0f) ? fminf(gm.endScreenTimer / Config::SUMMARY_COUNT_UP_DURATION, 1.0f) : 1.0f;
    auto countUp = [k](int v) { return (int)((float)v * k); };

    RetroUi::GlitchTitle(gm.titleFont, Tr(Str::GameOverTitle), { CX, 104.0f }, 44.0f, Palette::UiDanger,
                         RetroUi::GlitchAmount(gm.ui.screenTimer, t, rf), t);

    const float panelW = 420.0f, panelX = (float)centerX - panelW / 2.0f, panelY = 140.0f, panelH = 258.0f;
    PixelFrame({ panelX, panelY, panelW, panelH }, PanelFill(), Palette::UiPanelEdge);
    canvas.CenteredText(centerX, (int)panelY + 8, 25, Palette::UiAccent, Tr(Str::RunSummaryTitle));

    // Moi dong "nhan trai - gia tri phai": canh le bang toa do co dinh 2 ben panel (khong gop 1
    // chuoi TextFormat - gop thi khong canh phai duoc va nhan nhay theo do dai gia tri).
    float rowY = panelY + 40.0f;
    auto row = [&](const char* label, const char* value, Color valueColor, float size = 23.0f) {
        DrawTextEx(f, label, { panelX + 20.0f, rowY }, size, 1.0f, Palette::UiDim);
        RetroUi::TextRight(f, value, panelX + panelW - 20.0f, rowY, size, valueColor);
        rowY += 26.0f;
    };
    row(Tr(Str::RunSummaryScore), Score6(countUp(gm.player.GetScore())), Palette::UiText, 25.0f);
    row(Tr(Str::RunSummaryWave), TextFormat("%d", gm.wave), Palette::UiText);
    row(Tr(Str::RunSummaryKills), TextFormat("%d", countUp(gm.runKills)), Palette::UiText);
    row(Tr(Str::RunSummaryCombo), gm.runBestCombo > 1 ? TextFormat("x%d", gm.runBestCombo) : "-",
        gm.runBestCombo > 1 ? Palette::ScoreText : Palette::UiDim);
    rowY += 4.0f;
    row(Tr(Str::RunSummaryEarned), TextFormat("+%d", countUp(gm.runCurrencyEarned)), gm.runCurrencyEarned > 0 ? Palette::UiAccent : Palette::UiDim, 25.0f);
    row(Tr(Str::RunSummaryTotal), TextFormat("%d CR", countUp(gm.metaProgress.GetCurrency())), Palette::UiText);

    // MUC TIEU KE TIEP: loadout re nhat chua mo + thanh tien do kieu LED (1 nguon voi Hangar).
    LoadoutType next = NextLockedLoadout(gm.metaProgress);
    if (next == LoadoutType::Standard) {
        TextCentered(f, Tr(Str::AllUnlocked), CX, rowY + 6.0f, 21.0f, Palette::UiSuccess);
    } else {
        int cost = GetLoadoutUnlockCost(next);
        int have = gm.metaProgress.GetCurrency();
        TextCentered(f, TextFormat(Tr(Str::NextUnlockFmt), Tr(LoadoutStr(next)), have, cost), CX, rowY + 2.0f, 20.0f, Palette::UiDim);
        RetroUi::LedBar({ panelX + 20.0f, rowY + 26.0f, panelW - 40.0f, 10.0f }, cost > 0 ? fminf((float)have / (float)cost, 1.0f) : 1.0f,
                        24, Palette::UiAccent, Fade(Palette::UiPanelEdge, 0.8f));
    }

    if (gm.lastSubmitResult == SubmitResult::NewRecord) {
        RetroUi::WavyTextCentered(f, Tr(Str::NewRecordBanner), CX, 410.0f, 28.0f, Palette::UiAccent, t, rf ? 0.0f : 2.5f);
    } else if (gm.lastSubmitResult == SubmitResult::MadeTop10) {
        RetroUi::WavyTextCentered(f, Tr(Str::MadeTop10Banner), CX, 410.0f, 28.0f, Palette::UiSuccess, t, rf ? 0.0f : 2.0f);
    } else {
        TextCentered(f, TextFormat(Tr(Str::TopScoreFmt), gm.leaderboard.GetTopScore()), CX, 414.0f, 22.0f, Palette::UiDim);
    }
    for (int i = 0; i < 2; i++) {
        MenuEntry(f, UiLayout::GameOverButton(i), Tr(i == 0 ? Str::EndToMenu : Str::EndRestart), i == gm.ui.endIndex, t, rf, 24.0f);
    }
    canvas.Draw(f);
}
