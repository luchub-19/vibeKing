#pragma once
#include "raylib.h"

// ==========================================
// SCREEN SHAKE - tach random khoi ham render
// TRUOC: GetOffset() (goi trong Draw(), moi frame) tu sinh 1 gia tri random HOAN TOAN
// MOI moi lan goi -> 2 van de: (1) random hoa nam trong ham render thay vi logic update;
// (2) offset nhay giat vo huong tung frame, o FPS cao nhin giong nhieu trang thay vi
// 1 cu "rung" co huong - cang FPS cao cang giat nhanh hon (phu thuoc FPS).
// GIO: moi randomness chuyen het vao Update(dt) (logic, khong phai render). Cu moi
// RESAMPLE_INTERVAL giay thi random 1 diem dich MOI (doc lap FPS - luon 20 lan/giay du
// may chay 30 hay 300 fps), roi noi suy tuyen tinh mUOT theo dt tu offset hien tai ve
// diem dich do. GetOffset() gio chi la 1 getter thuan tuy, khong sinh random.
// ==========================================
class ScreenShake {
private:
    float timer = 0.0f;
    float duration = 0.0f;
    float magnitude = 0.0f;

    Vector2 currentOffset{ 0.0f, 0.0f };
    Vector2 targetOffset{ 0.0f, 0.0f };
    float resampleTimer = 0.0f;

    static constexpr float RESAMPLE_INTERVAL = 0.05f; // Doi huong nhieu 20 lan/giay
    static constexpr float LERP_SPEED = 25.0f;        // Toc do noi suy ve diem dich

public:
    // Rung manh hon se ghi de rung yeu dang chay, tranh 1 va cham nho lam mat hieu ung lon
    void Trigger(float durationSec, float mag) {
        if (mag >= magnitude || timer <= 0.0f) {
            duration = durationSec;
            timer = durationSec;
            magnitude = mag;
            resampleTimer = 0.0f; // Ep resample ngay lap tuc frame ke tiep, khong doi 50ms dau
        }
    }

    void Update(float dt) {
        if (timer <= 0.0f) {
            currentOffset = { 0.0f, 0.0f };
            targetOffset = { 0.0f, 0.0f };
            return;
        }
        timer -= dt;
        resampleTimer -= dt;

        if (resampleTimer <= 0.0f) {
            resampleTimer = RESAMPLE_INTERVAL;
            float ratio = (timer > 0.0f) ? (timer / duration) : 0.0f;
            float amount = magnitude * ratio;
            targetOffset = {
                ((float)GetRandomValue(-100, 100) / 100.0f) * amount,
                ((float)GetRandomValue(-100, 100) / 100.0f) * amount
            };
        }

        // Noi suy theo dt (khung hinh cham hay nhanh deu tien toi target voi cung 1 toc
        // do "thoi gian thuc", khong con phu thuoc so frame da trai qua)
        float lerpT = LERP_SPEED * dt;
        if (lerpT > 1.0f) lerpT = 1.0f;
        currentOffset.x += (targetOffset.x - currentOffset.x) * lerpT;
        currentOffset.y += (targetOffset.y - currentOffset.y) * lerpT;
    }

    // Pure getter - khong con sinh random o day, an toan de goi bao nhieu lan tuy y
    // trong Draw() ma khong lam thay doi trang thai.
    Vector2 GetOffset() const { return currentOffset; }
};
