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
// GD 1 (docs/GRAPHICS_UPGRADE_PLAN.md) - TRUOC DAY vi tri sao la ham thuan cua GetTime(), khong
// can Update(). Doi sang CONG DON `scrollTime += dt * speedMul` vi hieu ung WARP (bat dau wave
// boss: ca bau troi tang toc roi cham lai) can doi toc do GIUA CHUNG: nhan thang toc do vao
// GetTime() thi moi sao nhay coc toi vi tri "nhu the da bay nhanh tu dau van". WrappedY() van
// la ham thuan - chi doi tham so thoi gian truyen vao.
//   - Lop GAN ve thanh VET doc (dai theo toc do hien tai) thay vi cham tron -> cam giac lao toi.
//   - Lop XA lap lanh rat nhe (pha rieng tung sao) - tat khi reduceFlashing.
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
    void Update(float dt, float speedMul);
    void Draw(bool twinkle) const;

    // He so toc do cuon theo thoi gian warp CON LAI (0 = khong warp). Ham thuan - test duoc.
    // Bao: tang nhanh trong 15% dau, roi giam dan (smoothstep) - "phong" vao tran roi phanh.
    static float WarpSpeedMul(float remaining);
    static constexpr float WARP_DURATION = 1.6f;
    static constexpr float WARP_PEAK_MUL = 8.0f;

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
        unsigned char layer = 0; // 0 xa, 1 giua, 2 gan
        float phase = 0.0f;      // Lech pha lap lanh
    };

    std::array<Star, Config::PARALLAX_STAR_COUNT> stars{};
    float scrollTime = 0.0f;   // Tong dt * speedMul - thay GetTime() trong WrappedY
    float currentMul = 1.0f;   // Toc do frame nay - do dai vet sao gan
    float twinklePhase = 0.0f; // Dong ho rieng cho lap lanh (khong theo warp)
};
