#pragma once
#include "raylib.h"

// ==========================================
// TINH VAN NEN (GD 1 - docs/GRAPHICS_UPGRADE_PLAN.md). Shader fbm (assets/shaders/nebula.fs)
// "nuong" ra render texture nua do phan giai CHI KHI doi chuong/preset (Bake), moi frame chi
// ve lai texture do va cuon cham - chi phi moi frame = 1-2 lan ve 1 texture, khong phai fbm.
//
// Mau instance-owned nhu PostProcess: Init()/Shutdown() goi tay trong GameManager::Run(), test
// khong bao gio goi -> khong dong GPU. Thieu shader / tao texture loi -> tu tat, TraceLog
// WARNING, khong crash (cung triet ly PostProcess).
// ==========================================
class Nebula {
public:
    static constexpr int MAX_LAYERS = 2;

    void Init();
    void Shutdown();

    // Nuong lai NEU (chuong, so lop) khac lan truoc - goi moi frame cung re (so sanh 2 so nguyen).
    void EnsureBaked(int chapter, int layers);
    void Update(float dt);
    // alpha 0..1 (nhan voi do dam toi da cua tung lop; RenderSystem truyen CalmFactor - luat R2)
    void Draw(float alpha) const;

    // Ham thuan: wave -> chuong mau (0..2), moi 5 wave 1 chuong, quay vong. Test duoc.
    static int ChapterForWave(int wave);

private:
    void Bake(int chapter, int layers);

    Shader shader{};
    RenderTexture2D layerTex[MAX_LAYERS]{};
    int resolutionLoc = -1, seedLoc = -1, colorALoc = -1, colorBLoc = -1, baseFreqLoc = -1;
    bool ready = false;
    int bakedChapter = -1;
    int bakedLayers = -1;
    float drift[MAX_LAYERS]{}; // px da cuon cua tung lop (texture tuan hoan -> cuon vo han)
};
