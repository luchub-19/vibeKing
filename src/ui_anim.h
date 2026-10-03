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
