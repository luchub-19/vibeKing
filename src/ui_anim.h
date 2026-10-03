#pragma once
#include <cmath>

// ==========================================
// HOAT CANH UI THUAN (GD 5) - ham thuan theo dt, khong raylib -> test tai tests/test_post_fx.cpp
// ([ui]). Trang thai (gia tri dang hien) do GameManager giu.
// ==========================================

// DIEM "LAN SO": tien ve dich theo ham mu (nhanh khi con xa, cham dan khi gan) nhung khong cham
// hon `minRate` don vi/giay - neu khong, 3 diem cuoi lê thê ca giay. Khong bao gio vuot dich.
inline float RollToward(float shown, float target, float dt, float response = 10.0f, float minRate = 120.0f) {
    float diff = target - shown;
    if (diff == 0.0f) return target;
    float step = fmaxf(fabsf(diff) * response * dt, minRate * dt);
    if (step >= fabsf(diff)) return target;
    return shown + (diff > 0.0f ? step : -step);
}

// "VET SAT THUONG" cua thanh mau boss: khi mau tut, phan vua mat con hien trang mo roi rut ve
// voi toc do co dinh -> nguoi choi thay CU vua roi ton bao nhieu. Hoi mau (Medic... sau nay) thi
// vet nhay theo ngay, khong "rut nguoc".
inline float DamageTrail(float trail, float actual, float dt, float drainPerSec = 0.35f) {
    if (trail <= actual) return actual;
    float t = trail - drainPerSec * dt;
    return t < actual ? actual : t;
}

// CHUYEN CANH DANG "MANH SAP" (GD 5): man hinh chia bandCount dai ngang, moi dai dong lai tu
// giua ra 2 mep, lech nhip tu tren xuong (dai duoi bat dau muon hon toi 1/3 hanh trinh). Thay cho
// fade den deu: cung thoi gian, cung "toi han" o alpha = 1, nhung co huong chuyen dong kieu CRT.
// Tra ve ty le che 0..1 cua dai `band`. alpha = 0 -> 0 moi dai; alpha = 1 -> 1 moi dai.
inline float WipeBandCoverage(float alpha, int band, int bandCount) {
    if (bandCount <= 1) return alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
    float lag = 0.5f * (float)band / (float)(bandCount - 1);
    float t = alpha * 1.5f - lag;
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}

// Ease-out bac 3 cua `elapsed` tren `duration` - dung cho vach banner "phong ra" tu tam.
inline float EaseOutCubic(float elapsed, float duration) {
    if (duration <= 0.0f || elapsed >= duration) return 1.0f;
    if (elapsed <= 0.0f) return 0.0f;
    float k = 1.0f - elapsed / duration;
    return 1.0f - k * k * k;
}
