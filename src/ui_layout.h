#pragma once
#include "raylib.h"
#include "config.h"

// ==========================================
// BO CUC GUI - 1-NGUON-DUY-NHAT cho "nut nam o dau". Moi Rectangle ben duoi duoc dung CA boi
// GameManager (kiem tra chuot dang tro/bam vao nut nao) LAN RenderSystem (ve nut o dau). Truoc
// khi co chuot, vi tri chi can dung trong code ve; gio sai lech 1px giua 2 noi = bam vao nut A
// nhung kich hoat nut B. Ham thuan (chi so hoc Rectangle) -> test duoc khong can cua so.
//
// Toa do la canvas noi bo Config::SCREEN_W x SCREEN_H (800x600) - chuot da duoc doi ve he nay
// truoc khi so (UiNav::ScreenToVirtual).
// ==========================================
namespace UiLayout {
    constexpr float W = (float)Config::SCREEN_W;

    // Nut "< QUAY LAI" goc tren-trai, chung cho moi man con (Hangar/Bang/Cai dat/Huong dan/Thanh tuu).
    inline Rectangle BackButton() { return { 16.0f, 14.0f, 150.0f, 30.0f }; }

    // --- MENU CHINH: danh sach doc giua man hinh ---
    constexpr int MAIN_MENU_ITEMS = 6;
    inline Rectangle MainMenuItem(int i) { return { W / 2.0f - 170.0f, 236.0f + (float)i * 40.0f, 340.0f, 34.0f }; }

    // --- HANGAR: 3 hang (do kho / loadout / xuat kich) ---
    constexpr int HANGAR_ROWS = 3;
    inline Rectangle HangarDifficultyPill(int i) { return { 144.0f + (float)i * 176.0f, 132.0f, 160.0f, 40.0f }; }
    inline Rectangle HangarLoadoutCard(int i) { return { 80.0f + (float)i * 220.0f, 250.0f, 200.0f, 112.0f }; }
    inline Rectangle HangarLaunch() { return { W / 2.0f - 160.0f, 462.0f, 320.0f, 50.0f }; }
    // Vung "ca hang" de chuot tro vao bat ky dau tren hang cung chon hang do
    inline Rectangle HangarRow(int row) {
        if (row == 0) return { 130.0f, 100.0f, 540.0f, 110.0f };
        if (row == 1) return { 70.0f, 218.0f, 660.0f, 214.0f };
        return HangarLaunch();
    }

    // --- CAI DAT: 4 tab + toi da 6 dong + vung gia tri ben phai moi dong ---
    inline Rectangle SettingsTab(int i) { return { 38.0f + (float)i * 183.0f, 92.0f, 175.0f, 34.0f }; }
    inline Rectangle SettingsRow(int r) { return { 50.0f, 146.0f + (float)r * 54.0f, W - 100.0f, 46.0f }; }
    inline Rectangle SettingsControl(int r) {
        Rectangle row = SettingsRow(r);
        return { row.x + row.width - 334.0f, row.y + 7.0f, 320.0f, row.height - 14.0f };
    }
    // O lua chon thu `i` trong `count` o - chia deu vung gia tri, cach nhau 6px
    inline Rectangle SettingsOption(int r, int i, int count) {
        Rectangle c = SettingsControl(r);
        if (count <= 0) return c;
        float gap = 6.0f;
        float w = (c.width - gap * (float)(count - 1)) / (float)count;
        return { c.x + (float)i * (w + gap), c.y, w, c.height };
    }
    // Thanh truot: chua cho chu "100%" ben phai
    inline Rectangle SettingsSlider(int r) {
        Rectangle c = SettingsControl(r);
        return { c.x, c.y + 6.0f, c.width - 64.0f, c.height - 12.0f };
    }
    inline Rectangle SettingsDescPanel() { return { 50.0f, 476.0f, W - 100.0f, 66.0f }; }

    // --- HUONG DAN: 4 tab trang ---
    constexpr int HOWTO_PAGES = 4;
    inline Rectangle HowToTab(int i) { return { 65.0f + (float)i * 170.0f, 92.0f, 160.0f, 32.0f }; }

    // --- PAUSE: danh sach doc trong panel giua ---
    constexpr int PAUSE_ITEMS = 4;
    inline Rectangle PausePanel() { return { W / 2.0f - 190.0f, 150.0f, 380.0f, 300.0f }; }
    inline Rectangle PauseItem(int i) { return { W / 2.0f - 150.0f, 250.0f + (float)i * 44.0f, 300.0f, 36.0f }; }

    // --- HOP XAC NHAN (Co/Khong) ---
    inline Rectangle ConfirmPanel() { return { W / 2.0f - 230.0f, 210.0f, 460.0f, 170.0f }; }
    inline Rectangle ConfirmButton(int i) { return { W / 2.0f - 135.0f + (float)i * 150.0f, 318.0f, 120.0f, 38.0f }; }

    // --- MAN KET THUC ---
    inline Rectangle GameOverButton(int i) { return { W / 2.0f - 200.0f + (float)i * 210.0f, 452.0f, 190.0f, 40.0f }; }
    constexpr float UPGRADE_CARD_W = 220.0f, UPGRADE_CARD_H = 92.0f, UPGRADE_CARD_GAP = 14.0f;
    inline Rectangle UpgradeCard(int i) {
        float startX = (W - (3.0f * UPGRADE_CARD_W + 2.0f * UPGRADE_CARD_GAP)) / 2.0f;
        return { startX + (float)i * (UPGRADE_CARD_W + UPGRADE_CARD_GAP), 296.0f, UPGRADE_CARD_W, UPGRADE_CARD_H };
    }
}
