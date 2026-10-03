#pragma once
#include "raylib.h"
#include "graphics_settings.h"
#include "post_fx.h"

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
    //
    // `gfx` (trang GRAPHICS) quyet dinh MOI FRAME pass nao chay: doi preset/tat CRT co hieu
    // luc ngay, khong can khoi dong lai. Init() load san moi thu Config cho phep bat ke preset
    // hien tai, de bat lai Bloom giua chung khong phai load shader luc dang choi.
    void Render(const RenderTexture2D& source, Rectangle srcRec, Rectangle destRec, const GraphicsSettings& gfx,
                const PostFxFrame& fx);

private:
    // Muc 0 = anh vung sang o 1/BLOOM_DOWNSAMPLE do phan giai; muc i+1 = nua muc i. Dual Kawase
    // (GD 3): thu nho 0->1->..->N roi phong nguoc N->..->0 - xem kawase_down.fs/kawase_up.fs.
    // Truoc day: 2 pass Gauss 9 mau (ngang/doc) o muc 0 - phan lon chi phi bloom.
    static constexpr int BLOOM_MAX_LEVELS = 4; // muc 0 + toi da 3 muc thu nho (preset High)

    Shader bloomExtractShader{};
    Shader kawaseDownShader{};
    Shader kawaseUpShader{};
    Shader finalShader{}; // assets/shaders/final.fs - CRT + song xung kich + chinh mau, 1 pass

    // Vi tri uniform "dong" (doi giua cac lan goi Render()) - cache lai 1 lan trong Init(),
    // tranh GetShaderLocation() (do chuoi ten) moi frame.
    int kawaseDownHalfpixelLoc = -1;
    int kawaseUpHalfpixelLoc = -1;
    struct FinalLocs {
        int resolution = -1, gameSize = -1, time = -1, scanline = -1, vignette = -1, flicker = -1;
        int barrel = -1, waves = -1, waveCount = -1, chromatic = -1, enrage = -1, hurt = -1, grade = -1, flipY = -1;
        int colorFilter = -1;
    } fl;

    RenderTexture2D bloomLevels[BLOOM_MAX_LEVELS]{};

    // Anh FULL do phan giai sau khi cong don (additive) anh goc + bloom - dau vao cho
    // buoc CRT cuoi cung (hoac ve thang ra man hinh neu CRT tat).
    RenderTexture2D compositeTex{};

    bool bloomReady = false; // true neu CA shader lan render texture cua Bloom deu load thanh cong
    bool finalReady = false; // true neu final.fs load thanh cong - khong thi ve thang (khong hieu ung nao)
};
