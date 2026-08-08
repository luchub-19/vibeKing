#pragma once
#include "raylib.h"

// ==========================================
// POST-PROCESS: Bloom (trich sang + blur 2 chieu + cong don) va CRT (scanline/vignette/
// nhap nhay nhe), ap dung DUY NHAT tai buoc upscale renderTarget 800x600 len man hinh
// that trong GameManager::Run() - thay the truc tiep DrawTexturePro(renderTarget.texture,
// src, dst, ...) cu bang PostProcess::Render(renderTarget, src, dst).
//
// Config::BLOOM_ENABLED/CRT_ENABLED (config.h) bat/tat TUNG hieu ung DOC LAP - ca 2 tat
// thi Render() ve lai DUNG 1 DrawTexturePro y het hanh vi cu, khong overhead. Neu shader/
// render texture nao load loi luc Init() (vd thieu file assets/shaders/*.fs), tu dong tat
// RIENG hieu ung do (KHONG crash, TraceLog LOG_WARNING) - cung triet ly voi gameFont/
// settings.cfg/level.cfg trong GameManager::Run() (khong bao gio chet vi thieu 1 tai
// nguyen khong bat buoc).
//
// Mau instance-owned nhu AudioSystem: Init()/Shutdown() goi tay trong Run(), KHONG dung
// destructor tu dong giai phong GPU resource - an toan voi viec unit_tests link ca
// game_manager.cpp (goi toi PostProcess::Init/Render/Shutdown ben trong Run()) nhung ban
// than cac test KHONG BAO GIO goi Run() (xem CLAUDE.md/CMakeLists.txt): Init() khong duoc
// goi thi Shutdown() cung khong duoc goi, khong co gi de giai phong sai luc test.
// ==========================================
class PostProcess {
public:
    // Load shader + tao cac RenderTexture2D trung gian cho Bloom/CRT (tuy Config::
    // BLOOM_ENABLED/CRT_ENABLED). Goi DUY NHAT 1 LAN trong GameManager::Run(), SAU
    // LoadRenderTexture(renderTarget) (can biet Config::SCREEN_W/H) va SAU InitWindow()
    // (can GPU context de LoadShader/LoadRenderTexture khong deref tren context rong).
    void Init();

    // Unload toan bo shader/render texture da load trong Init(). Goi DUY NHAT 1 LAN
    // trong GameManager::Run(), truoc CloseWindow() - canh UnloadRenderTexture(renderTarget).
    void Shutdown();

    // Thay the hoan toan DrawTexturePro(source.texture, srcRec, destRec, {0,0}, 0, WHITE)
    // truc tiep. `source` PHAI la RenderTexture2D da ve xong noi dung can hien (srcRec
    // thuong co chieu cao AM - xem comment tai diem goi trong GameManager::Run() ve quy
    // uoc lat truc Y cua RenderTexture2D). Tu quyet dinh chay Bloom/CRT hay khong dua
    // theo Config::BLOOM_ENABLED/CRT_ENABLED VA viec Init() co thanh cong hay khong.
    void Render(const RenderTexture2D& source, Rectangle srcRec, Rectangle destRec);

private:
    Shader bloomExtractShader{};
    Shader blurShader{};
    Shader crtShader{};

    // Vi tri uniform "dong" (doi giua cac lan goi Render(), vd huong blur ngang/doc) -
    // cache lai 1 lan trong Init(), tranh GetShaderLocation() (do chuoi ten) moi frame.
    int blurDirectionLoc = -1;
    int crtTimeLoc = -1;
    int crtResolutionLoc = -1;

    // 2 texture trung gian cho Bloom, dung kieu ping-pong (trich sang -> blur ngang ->
    // blur doc) - xem post_process.cpp:Render(). Ca 2 o do phan giai giam theo Config::
    // BLOOM_DOWNSAMPLE (blur re hon, upscale lai khi cong don cung lam "mem" hon tu nhien).
    RenderTexture2D bloomTexA{};
    RenderTexture2D bloomTexB{};

    // Anh FULL do phan giai sau khi cong don (additive) anh goc + bloom - dau vao cho
    // buoc CRT cuoi cung (hoac ve thang ra man hinh neu CRT tat).
    RenderTexture2D compositeTex{};

    bool bloomReady = false; // true neu CA shader lan render texture cua Bloom deu load thanh cong
    bool crtReady = false;   // true neu shader CRT load thanh cong
};
