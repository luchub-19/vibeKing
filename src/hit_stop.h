#pragma once
#include <cmath>

// ==========================================
// HIT-STOP - dong bang LOGIC gameplay vai chuc mili-giay khi ha guc 1 dich (dac biet la
// Boss) de tang cam giac "dam" cho don danh - ky thuat pho bien trong game hanh dong
// (Smash Bros, hau het beat 'em up deu dung). Ban than struct nay CHI la 1 bo dem thoi
// gian thuan tuy (giong het tinh than ScreenShake: random hoa/dinh thoi nam trong
// Update(), khong lam gi khac); noi thuc su tao ra hieu ung dong bang la
// GameManager::UpdatePlaying() - goi hitStop.Update(dt) roi return SOM neu con
// IsActive(), khien toan bo phan con lai cua ham (input, va cham, spawn...) bi bo qua
// hoan toan cho (cac) frame do.
//
// Draw() nam o vong lap NGOAI UpdatePlaying() (xem GameManager::Run()) nen VAN chay binh
// thuong moi frame du logic dang dong bang - man hinh khong dung hinh/khong giat, chi co
// GAMEPLAY dung khung trong choc lat, dung y "hit-stop" (mot ky thuat lam noi bat 1
// khoanh khac) thay vi "pause toan man hinh".
// ==========================================
struct HitStop {
    float timer = 0.0f;

    void Trigger(float d) { timer = d; }
    // Huy dong bang dang chay. Can khi bat dau 1 wave/van moi (GameManager::InitLevel):
    // hit-stop cua don ha guc CUOI CUNG o wave truoc con sot lai se lam frame dau tien cua
    // wave moi bi dung hinh vo co.
    void Reset() { timer = 0.0f; }
    void Update(float dt) { timer = fmaxf(0.0f, timer - dt); }
    bool IsActive() const { return timer > 0.0f; }
};
