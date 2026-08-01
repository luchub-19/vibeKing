#pragma once
#include "raylib.h"
#include "config.h"

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
    bool PauseToggle = false;          // KEY_P hoac KEY_ESCAPE
    bool ToggleFullscreen = false;     // KEY_F11
};

class InputSystem {
public:
    // Doc toan bo nguon input phan cung dang ho tro (ban phim + gamepad 0 neu co cam)
    // va gop lai thanh 1 InputState duy nhat cho frame nay - dung trong luc PLAYING.
    static InputState Poll() {
        InputState in;
        in.Action_MoveRight = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
        in.Action_MoveLeft  = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
        in.Action_Shoot     = IsKeyDown(KEY_SPACE);

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
    // khi giu phim) vi day deu la cac hanh dong "bam 1 phat".
    static MenuInput PollMenu() {
        MenuInput m;
        m.CycleDifficultyLeft  = IsKeyPressed(KEY_LEFT);
        m.CycleDifficultyRight = IsKeyPressed(KEY_RIGHT);
        m.VolumeUp             = IsKeyPressed(KEY_UP);
        m.VolumeDown           = IsKeyPressed(KEY_DOWN);
        m.Confirm              = IsKeyPressed(KEY_ENTER);
        m.Restart              = IsKeyPressed(KEY_R);
        m.PauseToggle          = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
        m.ToggleFullscreen     = IsKeyPressed(KEY_F11);
        return m;
    }

    // Bat/tat Overlay do luong hieu nang (FPS/frame time/RAM/so luong entity) - phim
    // rieng, hoat dong o MOI trang thai (Menu/Playing/Paused), nen duoc GameManager::Run()
    // doc truc tiep moi frame thay vi qua PollMenu() (chi doc trong Update* cua tung
    // trang thai cu the).
    static bool PollDebugOverlayToggle() {
        return IsKeyPressed(KEY_F3);
    }
};
