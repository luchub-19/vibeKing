#pragma once
#include "raylib.h"
#include "config.h"
#include "settings.h"

// ==========================================
// INPUT SYSTEM
// Truoc day moi noi trong GameManager (UpdateMenu, UpdateEndScreen, UpdatePaused,
// UpdatePlaying, Player::Update...) deu tu goi thang IsKeyDown/IsKeyPressed/gamepad -
// nghia la "biet doc phan cung" bi rai rac khap code thay vi tap trung 1 cho. Muon doi
// keybind, them tay cam thu 2, hay thay input bang 1 nguon khac (vd replay/AI test)
// deu phai lung soan lai nhieu ham khong lien quan gi toi input.
//
// InputSystem la NOI DUY NHAT trong toan bo code con dung IsKeyDown/IsKeyPressed/
// IsGamepad* - moi noi khac chi nhan lai 1 struct tin hieu THUAN TUY o muc "hanh dong":
//   - InputState : dung trong luc PLAYING (Action_Move*/Action_Shoot) - Player nhan
//     truc tiep, khong biet gi ve phim/nut cu the nao sinh ra no.
//   - MenuInput   : dung cho moi man hinh con lai (Menu/Pause/EndScreen) + cac phim
//     "toan cuc" luon hoat dong khi dang choi (pause, restart, fullscreen).
//
// PHIM CO THE DOI (rebind): Poll()/PollMenu() nhan `const Settings&` de biet dung MA
// PHIM NAO cho Trai/Phai/Ban/Pause thay vi hang cung KEY_A/KEY_D/KEY_SPACE/KEY_P truoc
// day. Mui ten (Trai/Phai) va ESC (Pause) van la FALLBACK CO DINH song song, khong phu
// thuoc Settings - dam bao nguoi choi khong bao gio tu khoa minh khoi dieu khien co ban
// du rebind phim chinh thanh gi (vd rebind Ban trung 1 phim ki la, van luon di chuyen/
// pause duoc qua mui ten/ESC).
// ==========================================
struct InputState {
    bool Action_MoveLeft = false;
    bool Action_MoveRight = false;
    bool Action_Shoot = false;
};

struct MenuInput {
    bool CycleDifficultyLeft = false;  // KEY_LEFT
    bool CycleDifficultyRight = false; // KEY_RIGHT
    bool VolumeUp = false;             // KEY_UP
    bool VolumeDown = false;           // KEY_DOWN
    bool Confirm = false;              // KEY_ENTER
    bool Restart = false;              // KEY_R
    bool PauseToggle = false;          // settings.keyPause hoac KEY_ESCAPE (fallback co dinh)
    bool ToggleFullscreen = false;     // KEY_F11
    bool OpenKeybinds = false;         // KEY_K - chi co y nghia luc dang PAUSED (xem UpdateKeybindScreen)
    bool CycleLoadoutLeft = false;     // KEY_Q - chi co y nghia luc dang MENU (xem UpdateMenu/DrawLoadoutSelect)
    bool CycleLoadoutRight = false;    // KEY_E - chi co y nghia luc dang MENU
};

class InputSystem {
public:
    // Doc toan bo nguon input phan cung dang ho tro (ban phim + gamepad 0 neu co cam)
    // va gop lai thanh 1 InputState duy nhat cho frame nay - dung trong luc PLAYING.
    // `settings` cung cap ma phim CHINH cho Trai/Phai/Ban (co the da bi nguoi choi
    // rebind) - mui ten Trai/Phai luon la FALLBACK CO DINH song song (xem chu thich dau
    // file).
    static InputState Poll(const Settings& settings) {
        InputState in;
        in.Action_MoveRight = IsKeyDown(settings.keyMoveRight) || IsKeyDown(KEY_RIGHT);
        in.Action_MoveLeft  = IsKeyDown(settings.keyMoveLeft)  || IsKeyDown(KEY_LEFT);
        in.Action_Shoot     = IsKeyDown(settings.keyShoot);

        // Gamepad (tuy chon) - stick trai / D-pad de di chuyen, nut A (button 0) de
        // ban. Chi doc khi gamepad 0 thuc su cam vao, khong ep buoc nguoi choi phai
        // co tay cam. CHI gop them tin hieu (OR), khong bao gio ghi de ban phim.
        if (IsGamepadAvailable(0)) {
            float axisX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            if (axisX > Config::GAMEPAD_DEADZONE || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
                in.Action_MoveRight = true;
            if (axisX < -Config::GAMEPAD_DEADZONE || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
                in.Action_MoveLeft = true;
            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
                in.Action_Shoot = true;
        }
        return in;
    }

    // Tin hieu dung cho Menu/Pause/EndScreen va cac phim toan cuc trong luc Playing
    // (pause/restart/fullscreen). Dung IsKeyPressed (canh len, khong lap lai moi frame
    // khi giu phim) vi day deu la cac hanh dong "bam 1 phat". `settings.keyPause` co
    // the da bi rebind - KEY_ESCAPE luon la fallback co dinh song song.
    //
    // Gamepad: truoc day CHI Poll() (luc PLAYING) doc gamepad - PollMenu() thuan ban
    // phim, nghia la nguoi choi chi dung tay cam KHONG THE vao game/pause/restart duoc
    // (du di chuyen/ban trong luc choi van hoat dong binh thuong). Anh xa lai dung quy
    // uoc da dung trong Poll(): D-pad cho dieu huong (Cycle/Volume), nut mat phai DUOI
    // (A/Cross - trung nut Ban luc Playing) cho Confirm, nut mat phai TRAI (X/Square)
    // cho Restart (can tach rieng voi Confirm vi WAVE_CLEAR/GAME_OVER co CA HAI lua
    // chon cung luc), nut Start (MIDDLE_RIGHT) cho Pause - quy uoc chuan hau het game
    // console. Dung IsGamepadButtonPressed (canh len) khop voi IsKeyPressed ben tren,
    // KHONG dung *Down (se lap lai moi frame, sai voi ban chat "bam 1 phat" cua menu).
    // ToggleFullscreen/OpenKeybinds co tinh khai niem PC/desktop (fullscreen) hoac chi
    // dung de mo 1 man hinh REBIND BAN PHIM (vo nghia tren gamepad) - khong anh xa gamepad.
    static MenuInput PollMenu(const Settings& settings) {
        MenuInput m;
        m.CycleDifficultyLeft  = IsKeyPressed(KEY_LEFT);
        m.CycleDifficultyRight = IsKeyPressed(KEY_RIGHT);
        m.VolumeUp             = IsKeyPressed(KEY_UP);
        m.VolumeDown           = IsKeyPressed(KEY_DOWN);
        m.Confirm               = IsKeyPressed(KEY_ENTER);
        m.Restart               = IsKeyPressed(KEY_R);
        m.PauseToggle           = IsKeyPressed(settings.keyPause) || IsKeyPressed(KEY_ESCAPE);
        m.ToggleFullscreen      = IsKeyPressed(KEY_F11);
        m.OpenKeybinds          = IsKeyPressed(KEY_K);
        m.CycleLoadoutLeft      = IsKeyPressed(KEY_Q);
        m.CycleLoadoutRight     = IsKeyPressed(KEY_E);

        if (IsGamepadAvailable(0)) {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))   m.CycleDifficultyLeft  = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))  m.CycleDifficultyRight = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP))     m.VolumeUp   = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN))   m.VolumeDown = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))  m.Confirm    = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT))  m.Restart    = true;
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT))     m.PauseToggle = true;
        }
        return m;
    }

    // Tra ve MA PHIM vua duoc nhan (canh len) trong frame nay, hoac 0 neu khong co phim
    // nao - bao boc GetKeyPressed() cua raylib (thiet ke SAN cho dung "cho phim ke tiep"
    // kieu rebind UI, khong can tu kiem tra tung KeyboardKey 1). CHI dung trong man hinh
    // KEYBIND (xem GameManager::UpdateKeybindScreen) - moi noi khac deu qua Poll()/
    // PollMenu() nhu binh thuong.
    static int PollAnyKeyPressed() {
        return GetKeyPressed();
    }

    // Chuyen 1 ma phim raylib thanh ten hien thi duoc (vd 65 -> "A", 32 -> "SPACE") -
    // dung khi ve man hinh rebind. KEY_A..KEY_Z va KEY_ZERO..KEY_NINE trung THANG voi
    // ma ASCII cua chinh chu cai/chu so do (thiet ke co chu dich cua raylib, xem
    // raylib.h) nen ep kieu truc tiep sang char thay vi liet ke tay 36 truong hop. Cac
    // phim "dac biet" khong co dang ky tu don duoc liet ke rieng; phim nao khong nam
    // trong danh sach nay (rat hiem khi rebind toi) roi ve dang so KEY_<n>.
    static const char* KeyName(int key) {
        if ((key >= KEY_A && key <= KEY_Z) || (key >= KEY_ZERO && key <= KEY_NINE)) {
            return TextFormat("%c", (char)key); // Dung chung co che ring-buffer cua chinh TextFormat (raylib) - da dung o noi khac trong project cho HUD
        }
        switch (key) {
            case KEY_SPACE: return "SPACE";
            case KEY_ENTER: return "ENTER";
            case KEY_ESCAPE: return "ESC";
            case KEY_TAB: return "TAB";
            case KEY_LEFT: return "LEFT";
            case KEY_RIGHT: return "RIGHT";
            case KEY_UP: return "UP";
            case KEY_DOWN: return "DOWN";
            case KEY_LEFT_SHIFT: return "L-SHIFT";
            case KEY_RIGHT_SHIFT: return "R-SHIFT";
            case KEY_LEFT_CONTROL: return "L-CTRL";
            case KEY_RIGHT_CONTROL: return "R-CTRL";
            case KEY_LEFT_ALT: return "L-ALT";
            case KEY_RIGHT_ALT: return "R-ALT";
            case KEY_COMMA: return ",";
            case KEY_PERIOD: return ".";
            case KEY_SEMICOLON: return ";";
            case KEY_APOSTROPHE: return "'";
            case KEY_MINUS: return "-";
            case KEY_EQUAL: return "=";
            case KEY_LEFT_BRACKET: return "[";
            case KEY_RIGHT_BRACKET: return "]";
            case KEY_BACKSLASH: return "\\";
            case KEY_BACKSPACE: return "BACKSPACE";
            case KEY_CAPS_LOCK: return "CAPSLOCK";
            case KEY_F1: return "F1"; case KEY_F2: return "F2"; case KEY_F3: return "F3";
            case KEY_F4: return "F4"; case KEY_F5: return "F5"; case KEY_F6: return "F6";
            case KEY_F7: return "F7"; case KEY_F8: return "F8"; case KEY_F9: return "F9";
            case KEY_F10: return "F10"; case KEY_F11: return "F11"; case KEY_F12: return "F12";
            default: return TextFormat("KEY_%d", key);
        }
    }

    // Bat/tat Overlay do luong hieu nang (FPS/frame time/RAM/so luong entity) - phim
    // rieng, hoat dong o MOI trang thai (Menu/Playing/Paused), nen duoc GameManager::Run()
    // doc truc tiep moi frame thay vi qua PollMenu() (chi doc trong Update* cua tung
    // trang thai cu the).
    static bool PollDebugOverlayToggle() {
        return IsKeyPressed(KEY_F3);
    }
};
