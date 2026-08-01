#pragma once
#include "raylib.h"

class GameManager; // Forward declare - RenderSystem chi CAN 1 tham chieu const toi "the
                    // gioi" de doc du lieu ve, khong so huu du lieu nao ca.

// ==========================================
// RENDER SYSTEM
// Truoc day 5 ham DrawMenu/DrawEndScreen/DrawPlaying/DrawHUD/DrawGameText nam thang
// trong GameManager, xen giua hang chuc ham logic khac (input, va cham, spawn, wave
// progression...) - "ve" va "cap nhat trang thai" troi lan vao cung 1 class khong ranh
// gioi. Tach thanh RenderSystem: 1 tap ham THUAN VE (khong sua bat ky field nao, moi ham
// deu nhan `const GameManager&` chi de DOC), giup phan biet ro "cai gi thay doi state"
// (systems khac) vs "cai gi chi hien thi state hien tai" (o day).
//
// Text/thanh tien trinh (HP boss, volume...) duoc khai bao qua UICanvas (xem
// ui_system.h) thay vi goi thang DrawTextEx/DrawRectangle rai rac - moi Draw*() ben
// duoi tu xay 1 canvas rieng roi Draw() no 1 lan duy nhat o cuoi.
//
// RenderSystem la friend cua GameManager (xem game_manager.h) de doc thang cac field
// private can thiet (sprites, pools, bunkers, player...) ma khong phai viet hang chuc
// getter chi ton tai de phuc vu rieng viec ve - cach nay pho bien trong cac engine nho
// noi 1 vai "system" duoc tin tuong thao tac truc tiep tren du lieu the gioi dung chia
// se, thay vi bat moi thu phai di qua 1 lop API cong khai day rao can.
// ==========================================
class RenderSystem {
public:
    static void DrawMenu(const GameManager& gm);
    static void DrawEndScreen(const GameManager& gm);
    static void DrawPlaying(const GameManager& gm);
    static void DrawHUD(const GameManager& gm);

    // OBSERVABILITY / PROFILING OVERLAY: FPS, frame time, RAM tien trinh thuc te (xem
    // process_metrics.h), va so luong entity dang song trong tung pool - bat/tat bang
    // F3 (xem InputSystem::PollDebugOverlayToggle). Ve o TOA DO MAN HINH THAT (native
    // GetScreenWidth/Height), NGOAI canh render texture noi bo 800x600 - de van sac net,
    // dung vi tri du dang Fullscreen tren monitor ty le bat ky, khong bi anh huong boi
    // buoc upscale/letterbox (xem GameManager::Run()).
    static void DrawDebugOverlay(const GameManager& gm);
};
