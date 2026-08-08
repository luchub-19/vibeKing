#pragma once
#include "raylib.h"
#include <array>
#include "config.h"

// ==========================================
// PARALLAX STARFIELD - nen sao nhieu lop do sau, ve truoc MOI trang thai (Menu/Playing/
// EndScreen/Paused/Keybind...) trong GameManager::Run(), ngay sau ClearBackground(BLACK)
// va TRUOC switch-case chinh (luon nam duoi cung, khong de vao content). Mau
// instance-owned nhu AudioSystem - GameManager giu 1 the hien `Parallax background;`, goi
// Init() 1 lan trong Run() (khong co logic gi trong constructor), khong destructor tu
// dong don rac (xem game_manager.h/.cpp).
//
// KHONG luu rieng dt/accumulator: vi tri Y cua 1 ngoi sao tai bat ky thoi diem nao la HAM
// THUAN cua GetTime() (cuon xuong roi quay vong ve dinh khi qua day man hinh) - dung y
// het triet ly "transform truoc" cua DrawTitleLogo() (render_system.cpp, dung
// sin(GetTime()) thay vi tu cong don van toc moi frame). Nho vay chi can Draw(), khong
// can 1 Update(dt) rieng goi moi frame.
// ==========================================
class Parallax {
public:
    // Sinh ngau nhien vi tri/kich thuoc/mau cho toan bo sao, phan vao 3 lop do sau (xa/
    // giua/gan - xem parallax.cpp). Goi DUY NHAT 1 LAN trong GameManager::Run(), truoc
    // vong lap chinh (sau InitWindow() vi dung GetRandomValue) - xem audio.Init()/
    // sprites.Load() lam mau vi tri goi.
    void Init();

    // Ve toan bo sao len canvas hien hanh - gia dinh dang trong BeginTextureMode(
    // renderTarget), xem diem goi trong GameManager::Run(). KHONG doi state noi bo, an
    // toan goi moi frame.
    void Draw() const;

    // Ham THUAN, khong dung GetTime()/bat ky trang thai raylib nao - tach rieng de test
    // headless duoc (xem tests/test_parallax.cpp). Tra ve toa do Y hien tai cua 1 sao:
    // cuon xuong theo (speed * time), quay vong ve 0 khi vuot qua screenH.
    static float WrappedY(float baseY, float speed, float time, float screenH);

private:
    struct Star {
        float x = 0.0f;
        float baseY = 0.0f;
        float speed = 0.0f;
        float radius = 0.0f;
        Color color = BLACK;
    };

    std::array<Star, Config::PARALLAX_STAR_COUNT> stars{};
};
