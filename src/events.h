#pragma once
#include "raylib.h"
#include <cstdint>
#include "powerup.h"

// ==========================================
// GAME EVENT QUEUE
// CheckCollisions() truoc day lam 2 viec cung luc trong cung 1 vong lap: (1) XAC DINH
// va cham (ai trung ai, con song hay chet) va (2) THUC THI HE QUA cua va cham do (tru
// mau hien thi, cong diem/combo, sinh particle, phat am thanh, rung man hinh, roi
// power-up) - ca hai troi vao nhau nen ham va cham vua dai vua kho doc/test, moi lan
// them 1 hieu ung moi lai phai sua thang vao logic phat hien va cham.
//
// Gio tach lam 2 buoc:
//   - CheckCollisions() CHI con lam viec (1): xac dinh va cham + cap nhat state toi
//     thieu can thiet de biet KET QUA va cham (hp con lai, con song hay da chet, bullet
//     con xuyen duoc hay khong...). Thay vi goi thang particles/audio/screenShake/
//     score, no dong goi hieu ung thanh 1 GameEvent va day vao hang doi.
//   - GameManager::ProcessEvents() duyet hang doi NGAY SAU DO trong cung 1 frame va
//     thuc thi tat ca hieu ung mot cach dong nhat.
//
// Loi ich: logic va cham doc gon, de test rieng (khong dinh audio/particle); muon them
// 1 hieu ung moi (vd rung tay cam, ghi log analytics) chi can sua ProcessEvents(),
// khong phai ra soat lai tung nhanh va cham.
// ==========================================

enum class SfxType : uint8_t {
    None,
    Explosion,
    Hit,
    UfoHit,
    Pickup,
    Cleanser,
};

struct GameEvent {
    Vector2 position{};
    Color color{ WHITE };

    int particleCount = 0;      // 0 = khong sinh particle

    SfxType sfx = SfxType::None;

    float shakeDuration = 0.0f; // 0 = khong rung man hinh
    float shakeIntensity = 0.0f;

    int scoreValue = 0;         // 0 = khong cong diem (diem GOC, ProcessEvents se ap combo)
    bool dropPowerUp = false;   // true = roll ngau nhien roi power-up tai `position`
};
