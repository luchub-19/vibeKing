#include "post_process.h"
#include "config.h"

void PostProcess::Init() {
    if (Config::BLOOM_ENABLED) {
        bloomExtractShader = LoadShader(nullptr, Config::BloomExtractShaderPath());
        blurShader = LoadShader(nullptr, Config::BlurShaderPath());

        int bloomW = Config::SCREEN_W / Config::BLOOM_DOWNSAMPLE;
        int bloomH = Config::SCREEN_H / Config::BLOOM_DOWNSAMPLE;
        if (bloomW < 1) bloomW = 1;
        if (bloomH < 1) bloomH = 1;
        bloomTexA = LoadRenderTexture(bloomW, bloomH);
        bloomTexB = LoadRenderTexture(bloomW, bloomH);
        compositeTex = LoadRenderTexture(Config::SCREEN_W, Config::SCREEN_H);

        bloomReady = IsShaderValid(bloomExtractShader) && IsShaderValid(blurShader) &&
                     IsRenderTextureValid(bloomTexA) && IsRenderTextureValid(bloomTexB) &&
                     IsRenderTextureValid(compositeTex);

        if (bloomReady) {
            SetTextureFilter(bloomTexA.texture, TEXTURE_FILTER_BILINEAR);
            SetTextureFilter(bloomTexB.texture, TEXTURE_FILTER_BILINEAR);

            // Uniform CO DINH throughout 1 phien chay - set 1 lan o day, khac voi
            // `direction` (blurShader) phai doi giua 2 pass ngang/doc trong MOI lan goi
            // Render() (xem ben duoi).
            int thresholdLoc = GetShaderLocation(bloomExtractShader, "threshold");
            int extractIntensityLoc = GetShaderLocation(bloomExtractShader, "intensity");
            float threshold = Config::BLOOM_THRESHOLD;
            float intensity = Config::BLOOM_INTENSITY;
            SetShaderValue(bloomExtractShader, thresholdLoc, &threshold, SHADER_UNIFORM_FLOAT);
            SetShaderValue(bloomExtractShader, extractIntensityLoc, &intensity, SHADER_UNIFORM_FLOAT);

            blurDirectionLoc = GetShaderLocation(blurShader, "direction");
            int texelSizeLoc = GetShaderLocation(blurShader, "texelSize");
            int spreadLoc = GetShaderLocation(blurShader, "spread");
            float texelSize[2] = { 1.0f / (float)bloomW, 1.0f / (float)bloomH };
            float spread = Config::BLOOM_BLUR_SPREAD;
            SetShaderValue(blurShader, texelSizeLoc, texelSize, SHADER_UNIFORM_VEC2);
            SetShaderValue(blurShader, spreadLoc, &spread, SHADER_UNIFORM_FLOAT);
        } else {
            TraceLog(LOG_WARNING, "PostProcess: khong the khoi tao pipeline Bloom (shader hoac render texture loi) - tat Bloom cho phien nay.");
        }
    }

    if (Config::CRT_ENABLED) {
        crtShader = LoadShader(nullptr, Config::CrtShaderPath());
        crtReady = IsShaderValid(crtShader);

        if (crtReady) {
            crtTimeLoc = GetShaderLocation(crtShader, "time");
            crtResolutionLoc = GetShaderLocation(crtShader, "resolution");
            int scanlineLoc = GetShaderLocation(crtShader, "scanlineStrength");
            int vignetteLoc = GetShaderLocation(crtShader, "vignetteStrength");
            int flickerLoc = GetShaderLocation(crtShader, "flickerStrength");
            float scanline = Config::CRT_SCANLINE_STRENGTH;
            float vignette = Config::CRT_VIGNETTE_STRENGTH;
            float flicker = Config::CRT_FLICKER_STRENGTH;
            SetShaderValue(crtShader, scanlineLoc, &scanline, SHADER_UNIFORM_FLOAT);
            SetShaderValue(crtShader, vignetteLoc, &vignette, SHADER_UNIFORM_FLOAT);
            SetShaderValue(crtShader, flickerLoc, &flicker, SHADER_UNIFORM_FLOAT);
        } else {
            TraceLog(LOG_WARNING, "PostProcess: khong the khoi tao shader CRT - tat CRT cho phien nay.");
        }
    }
}

void PostProcess::Shutdown() {
    if (bloomReady) {
        UnloadShader(bloomExtractShader);
        UnloadShader(blurShader);
        UnloadRenderTexture(bloomTexA);
        UnloadRenderTexture(bloomTexB);
        UnloadRenderTexture(compositeTex);
        bloomReady = false;
    }
    if (crtReady) {
        UnloadShader(crtShader);
        crtReady = false;
    }
}

void PostProcess::Render(const RenderTexture2D& source, Rectangle srcRec, Rectangle destRec) {
    const RenderTexture2D* finalSource = &source;
    Rectangle finalSrcRec = srcRec;

    if (bloomReady) {
        // RenderTexture2D bi lat nguoc truc Y khi doc lai (quy uoc OpenGL) - MOI lan doc
        // texture cua 1 RenderTexture2D (bat ke no duoc ve boi buoc nao truoc do) can
        // chieu cao AM de tra ve dung chieu, xem comment goc tai diem goi trong
        // GameManager::Run(). Ap dung dong nhat ca 6 lan doc trong ham nay - "da lat 1
        // lan roi" KHONG co nghia lan doc sau khong can lat nua, moi lan doc deu can rieng.
        Rectangle bloomFullSrc{ 0.0f, 0.0f, (float)bloomTexA.texture.width, -(float)bloomTexA.texture.height };
        Rectangle bloomFullDst{ 0.0f, 0.0f, (float)bloomTexA.texture.width, (float)bloomTexA.texture.height };

        // 1) Trich vung sang (nguong BLOOM_THRESHOLD) tu source, thu nho ve bloomTexA.
        BeginTextureMode(bloomTexA);
            ClearBackground(BLANK);
            BeginShaderMode(bloomExtractShader);
                DrawTexturePro(source.texture, srcRec, bloomFullDst, { 0.0f, 0.0f }, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 2) Blur ngang: bloomTexA -> bloomTexB.
        BeginTextureMode(bloomTexB);
            ClearBackground(BLANK);
            BeginShaderMode(blurShader);
                float dirH[2] = { 1.0f, 0.0f };
                SetShaderValue(blurShader, blurDirectionLoc, dirH, SHADER_UNIFORM_VEC2);
                DrawTexturePro(bloomTexA.texture, bloomFullSrc, bloomFullDst, { 0.0f, 0.0f }, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 3) Blur doc: bloomTexB -> bloomTexA (dung lai, khong can texture trung gian thu 3).
        BeginTextureMode(bloomTexA);
            ClearBackground(BLANK);
            BeginShaderMode(blurShader);
                float dirV[2] = { 0.0f, 1.0f };
                SetShaderValue(blurShader, blurDirectionLoc, dirV, SHADER_UNIFORM_VEC2);
                DrawTexturePro(bloomTexB.texture, bloomFullSrc, bloomFullDst, { 0.0f, 0.0f }, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 4) Composite: anh goc (khong shader) + bloom (BLEND_ADDITIVE) vao compositeTex,
        // FULL do phan giai.
        Rectangle compositeFullDst{ 0.0f, 0.0f, (float)compositeTex.texture.width, (float)compositeTex.texture.height };
        BeginTextureMode(compositeTex);
            ClearBackground(BLACK);
            DrawTexturePro(source.texture, srcRec, compositeFullDst, { 0.0f, 0.0f }, 0.0f, WHITE);
            BeginBlendMode(BLEND_ADDITIVE);
                DrawTexturePro(bloomTexA.texture, bloomFullSrc, compositeFullDst, { 0.0f, 0.0f }, 0.0f, WHITE);
            EndBlendMode();
        EndTextureMode();

        finalSource = &compositeTex;
        finalSrcRec = { 0.0f, 0.0f, (float)compositeTex.texture.width, -(float)compositeTex.texture.height };
    }

    if (crtReady) {
        float t = (float)GetTime();
        float res[2] = { destRec.width, destRec.height };
        SetShaderValue(crtShader, crtTimeLoc, &t, SHADER_UNIFORM_FLOAT);
        SetShaderValue(crtShader, crtResolutionLoc, res, SHADER_UNIFORM_VEC2);

        BeginShaderMode(crtShader);
            DrawTexturePro(finalSource->texture, finalSrcRec, destRec, { 0.0f, 0.0f }, 0.0f, WHITE);
        EndShaderMode();
    } else {
        DrawTexturePro(finalSource->texture, finalSrcRec, destRec, { 0.0f, 0.0f }, 0.0f, WHITE);
    }
}
