#include "nebula.h"
#include "config.h"
#include "palette.h"
#include <cmath>

namespace {
    constexpr int TEX_W = Config::SCREEN_W / 2; // Nua do phan giai: may mo mem, phong to lai van min
    constexpr int TEX_H = Config::SCREEN_H / 2;
    // Moi lop: toc do cuon (px man hinh/giay), do dam toi da, tan so may, lech seed. Lop 2 (High)
    // cuon nhanh hon + mo hon + may nho hon -> 2 lop truot len nhau ra chieu sau.
    struct LayerStyle { float speed; float maxAlpha; float baseFreq; float seedOffset; };
    constexpr LayerStyle STYLES[Nebula::MAX_LAYERS] = {
        { 4.0f,  0.55f, 3.0f, 0.0f },
        { 11.0f, 0.30f, 5.0f, 7.3f },
    };

    void ChapterColors(int chapter, Color& a, Color& b) {
        switch (chapter) {
            case 1:  a = Palette::NebulaChapter1A; b = Palette::NebulaChapter1B; break;
            case 2:  a = Palette::NebulaChapter2A; b = Palette::NebulaChapter2B; break;
            default: a = Palette::NebulaChapter0A; b = Palette::NebulaChapter0B; break;
        }
    }
}

int Nebula::ChapterForWave(int wave) {
    if (wave < 1) wave = 1;
    return ((wave - 1) / 5) % 3;
}

void Nebula::Init() {
    shader = LoadShader(nullptr, Config::NebulaShaderPath());
    ready = IsShaderValid(shader);
    for (int i = 0; i < MAX_LAYERS && ready; i++) {
        layerTex[i] = LoadRenderTexture(TEX_W, TEX_H);
        if (!IsRenderTextureValid(layerTex[i])) { ready = false; break; }
        SetTextureFilter(layerTex[i].texture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(layerTex[i].texture, TEXTURE_WRAP_REPEAT); // Cuon vo han - xem nebula.fs
    }
    if (!ready) {
        TraceLog(LOG_WARNING, "Nebula: khong khoi tao duoc shader/render texture - tat tinh van.");
        return;
    }
    resolutionLoc = GetShaderLocation(shader, "resolution");
    seedLoc = GetShaderLocation(shader, "seed");
    colorALoc = GetShaderLocation(shader, "colorA");
    colorBLoc = GetShaderLocation(shader, "colorB");
    baseFreqLoc = GetShaderLocation(shader, "baseFreq");
}

void Nebula::Shutdown() {
    if (IsShaderValid(shader)) UnloadShader(shader);
    for (RenderTexture2D& t : layerTex) {
        if (IsRenderTextureValid(t)) UnloadRenderTexture(t);
    }
    ready = false;
}

void Nebula::EnsureBaked(int chapter, int layers) {
    if (!ready) return;
    if (chapter == bakedChapter && layers == bakedLayers) return;
    Bake(chapter, layers);
}

void Nebula::Bake(int chapter, int layers) {
    Color ca, cb;
    ChapterColors(chapter, ca, cb);
    float colA[3] = { ca.r / 255.0f, ca.g / 255.0f, ca.b / 255.0f };
    float colB[3] = { cb.r / 255.0f, cb.g / 255.0f, cb.b / 255.0f };
    float res[2] = { (float)TEX_W, (float)TEX_H };
    SetShaderValue(shader, resolutionLoc, res, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, colorALoc, colA, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, colorBLoc, colB, SHADER_UNIFORM_VEC3);

    for (int i = 0; i < layers && i < MAX_LAYERS; i++) {
        float seed = (float)chapter * 3.1f + STYLES[i].seedOffset;
        float freq = STYLES[i].baseFreq;
        SetShaderValue(shader, seedLoc, &seed, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, baseFreqLoc, &freq, SHADER_UNIFORM_FLOAT);
        BeginTextureMode(layerTex[i]);
            ClearBackground(BLANK);
            BeginShaderMode(shader);
                DrawRectangle(0, 0, TEX_W, TEX_H, WHITE);
            EndShaderMode();
        EndTextureMode();
    }
    bakedChapter = chapter;
    bakedLayers = layers;
}

void Nebula::Update(float dt) {
    for (int i = 0; i < MAX_LAYERS; i++) {
        drift[i] = fmodf(drift[i] + STYLES[i].speed * dt, (float)Config::SCREEN_H);
    }
}

void Nebula::Draw(float alpha) const {
    if (!ready || bakedLayers <= 0 || alpha <= 0.0f) return;
    const Rectangle dst{ 0.0f, 0.0f, (float)Config::SCREEN_W, (float)Config::SCREEN_H };
    for (int i = 0; i < bakedLayers && i < MAX_LAYERS; i++) {
        // src.y lui dan theo drift -> anh truot XUONG cung chieu sao. Toa do texture (nua do
        // phan giai) = drift/2. Chieu cao AM: lat truc Y cua RenderTexture (quy uoc OpenGL).
        Rectangle src{ 0.0f, -drift[i] * 0.5f, (float)TEX_W, -(float)TEX_H };
        DrawTexturePro(layerTex[i].texture, src, dst, { 0.0f, 0.0f }, 0.0f,
                       Fade(WHITE, STYLES[i].maxAlpha * alpha));
    }
}
