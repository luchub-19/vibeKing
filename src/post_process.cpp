#include "post_process.h"
#include "config.h"
#include <initializer_list>

void PostProcess::Init() {
    if (Config::BLOOM_ENABLED) {
        bloomExtractShader = LoadShader(nullptr, Config::BloomExtractShaderPath());
        kawaseDownShader = LoadShader(nullptr, Config::KawaseDownShaderPath());
        kawaseUpShader = LoadShader(nullptr, Config::KawaseUpShaderPath());

        bloomReady = IsShaderValid(bloomExtractShader) && IsShaderValid(kawaseDownShader) && IsShaderValid(kawaseUpShader);
        int w = Config::SCREEN_W / Config::BLOOM_DOWNSAMPLE;
        int h = Config::SCREEN_H / Config::BLOOM_DOWNSAMPLE;
        for (int i = 0; i < BLOOM_MAX_LEVELS && bloomReady; i++) {
            bloomLevels[i] = LoadRenderTexture(w < 1 ? 1 : w, h < 1 ? 1 : h);
            if (!IsRenderTextureValid(bloomLevels[i])) { bloomReady = false; break; }
            // BILINEAR la dieu kien cua Dual Kawase: moi lan doc o nua texel = trung binh 4 texel
            SetTextureFilter(bloomLevels[i].texture, TEXTURE_FILTER_BILINEAR);
            SetTextureWrap(bloomLevels[i].texture, TEXTURE_WRAP_CLAMP); // Khong "cuon" sang tu mep doi dien
            w /= 2;
            h /= 2;
        }
        if (bloomReady) {
            compositeTex = LoadRenderTexture(Config::SCREEN_W, Config::SCREEN_H);
            bloomReady = IsRenderTextureValid(compositeTex);
        }

        if (bloomReady) {
            // Uniform CO DINH throughout 1 phien chay - set 1 lan o day; `halfpixel` thi doi
            // theo tung muc trong MOI lan goi Render() (xem ben duoi).
            int thresholdLoc = GetShaderLocation(bloomExtractShader, "threshold");
            int extractIntensityLoc = GetShaderLocation(bloomExtractShader, "intensity");
            float threshold = Config::BLOOM_THRESHOLD;
            float intensity = Config::BLOOM_INTENSITY;
            SetShaderValue(bloomExtractShader, thresholdLoc, &threshold, SHADER_UNIFORM_FLOAT);
            SetShaderValue(bloomExtractShader, extractIntensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
            kawaseDownHalfpixelLoc = GetShaderLocation(kawaseDownShader, "halfpixel");
            kawaseUpHalfpixelLoc = GetShaderLocation(kawaseUpShader, "halfpixel");
        } else {
            TraceLog(LOG_WARNING, "PostProcess: khong the khoi tao pipeline Bloom (shader hoac render texture loi) - tat Bloom cho phien nay.");
        }
    }

    // Pass cuoi LUON load (khong con gate theo CRT_ENABLED): chinh mau/song xung kich can no ca
    // khi tat CRT. Config::CRT_ENABLED = false gio nghia la "scanline/vignette/flicker = 0".
    finalShader = LoadShader(nullptr, Config::FinalShaderPath());
    finalReady = IsShaderValid(finalShader);
    if (finalReady) {
        fl.resolution = GetShaderLocation(finalShader, "resolution");
        fl.gameSize   = GetShaderLocation(finalShader, "gameSize");
        fl.time       = GetShaderLocation(finalShader, "time");
        fl.scanline   = GetShaderLocation(finalShader, "scanlineStrength");
        fl.vignette   = GetShaderLocation(finalShader, "vignetteStrength");
        fl.flicker    = GetShaderLocation(finalShader, "flickerStrength");
        fl.barrel     = GetShaderLocation(finalShader, "barrel");
        fl.waves      = GetShaderLocation(finalShader, "waves");
        fl.waveCount  = GetShaderLocation(finalShader, "waveCount");
        fl.chromatic  = GetShaderLocation(finalShader, "chromatic");
        fl.enrage     = GetShaderLocation(finalShader, "enrage");
        fl.hurt       = GetShaderLocation(finalShader, "hurt");
        fl.grade      = GetShaderLocation(finalShader, "grade");
        fl.flipY      = GetShaderLocation(finalShader, "flipY");
        float gameSize[2] = { (float)Config::SCREEN_W, (float)Config::SCREEN_H };
        SetShaderValue(finalShader, fl.gameSize, gameSize, SHADER_UNIFORM_VEC2);
    } else {
        TraceLog(LOG_WARNING, "PostProcess: khong the khoi tao final.fs - tat CRT/song xung kich/chinh mau cho phien nay.");
    }
}

void PostProcess::Shutdown() {
    // Giai phong THEO TUNG tai nguyen hop le, khong theo bloomReady: Init() co the load duoc
    // 1 phan roi that bai giua chung (vd muc 3) - phan da load van phai tra lai.
    for (Shader* sh : { &bloomExtractShader, &kawaseDownShader, &kawaseUpShader }) {
        if (IsShaderValid(*sh)) UnloadShader(*sh);
    }
    for (RenderTexture2D& t : bloomLevels) {
        if (IsRenderTextureValid(t)) UnloadRenderTexture(t);
    }
    if (IsRenderTextureValid(compositeTex)) UnloadRenderTexture(compositeTex);
    bloomReady = false;
    if (finalReady) {
        UnloadShader(finalShader);
        finalReady = false;
    }
}

void PostProcess::Render(const RenderTexture2D& source, Rectangle srcRec, Rectangle destRec, const GraphicsSettings& gfx,
                         const PostFxFrame& fx) {
    const RenderTexture2D* finalSource = &source;
    Rectangle finalSrcRec = srcRec;

    if (bloomReady && gfx.BloomEnabled()) {
        // RenderTexture2D bi lat nguoc truc Y khi doc lai (quy uoc OpenGL) - MOI lan doc
        // texture cua 1 RenderTexture2D can chieu cao AM de tra ve dung chieu, xem comment goc
        // tai diem goi trong GameManager::Run(). "Da lat 1 lan roi" KHONG co nghia lan doc sau
        // khong can lat nua - moi lan doc deu can rieng.
        auto fullSrc = [](const RenderTexture2D& t) {
            return Rectangle{ 0.0f, 0.0f, (float)t.texture.width, -(float)t.texture.height };
        };
        auto fullDst = [](const RenderTexture2D& t) {
            return Rectangle{ 0.0f, 0.0f, (float)t.texture.width, (float)t.texture.height };
        };
        // Ve `src` vao `dst` qua `shader` voi halfpixel tinh theo texture NGUON.
        auto pass = [&](const RenderTexture2D& src, const RenderTexture2D& dst, Shader shader, int halfpixelLoc) {
            float hp[2] = { 0.5f / (float)src.texture.width, 0.5f / (float)src.texture.height };
            SetShaderValue(shader, halfpixelLoc, hp, SHADER_UNIFORM_VEC2);
            BeginTextureMode(dst);
                ClearBackground(BLANK);
                BeginShaderMode(shader);
                    DrawTexturePro(src.texture, fullSrc(src), fullDst(dst), { 0.0f, 0.0f }, 0.0f, WHITE);
                EndShaderMode();
            EndTextureMode();
        };

        // 1) Trich vung sang (nguong BLOOM_THRESHOLD) tu source, thu nho ve muc 0.
        BeginTextureMode(bloomLevels[0]);
            ClearBackground(BLANK);
            BeginShaderMode(bloomExtractShader);
                DrawTexturePro(source.texture, srcRec, fullDst(bloomLevels[0]), { 0.0f, 0.0f }, 0.0f, WHITE);
            EndShaderMode();
        EndTextureMode();

        // 2) Dual Kawase: thu nho 0 -> N roi phong nguoc N -> 0 (ghi de, khong cong don - quang
        // rong la nho chuoi muc, khong phai nho cong).
        int levels = gfx.BloomLevels();
        if (levels > BLOOM_MAX_LEVELS - 1) levels = BLOOM_MAX_LEVELS - 1;
        for (int i = 0; i < levels; i++) pass(bloomLevels[i], bloomLevels[i + 1], kawaseDownShader, kawaseDownHalfpixelLoc);
        for (int i = levels; i > 0; i--) pass(bloomLevels[i], bloomLevels[i - 1], kawaseUpShader, kawaseUpHalfpixelLoc);

        // 3) Composite: anh goc (khong shader) + bloom (BLEND_ADDITIVE) vao compositeTex,
        // FULL do phan giai.
        Rectangle compositeFullDst = fullDst(compositeTex);
        BeginTextureMode(compositeTex);
            ClearBackground(BLACK);
            DrawTexturePro(source.texture, srcRec, compositeFullDst, { 0.0f, 0.0f }, 0.0f, WHITE);
            BeginBlendMode(BLEND_ADDITIVE);
                DrawTexturePro(bloomLevels[0].texture, fullSrc(bloomLevels[0]), compositeFullDst, { 0.0f, 0.0f }, 0.0f, WHITE);
            EndBlendMode();
        EndTextureMode();

        finalSource = &compositeTex;
        finalSrcRec = fullSrc(compositeTex);
    }

    if (finalReady) {
        auto setF = [this](int loc, float v) { SetShaderValue(finalShader, loc, &v, SHADER_UNIFORM_FLOAT); };
        const bool crt = gfx.crtEnabled && Config::CRT_ENABLED;
        float res[2] = { destRec.width, destRec.height };
        SetShaderValue(finalShader, fl.resolution, res, SHADER_UNIFORM_VEC2);
        setF(fl.time, (float)GetTime());
        setF(fl.scanline, crt ? Config::CRT_SCANLINE_STRENGTH : 0.0f);
        setF(fl.vignette, crt ? Config::CRT_VIGNETTE_STRENGTH : 0.0f);
        setF(fl.flicker, crt ? Config::CRT_FLICKER_STRENGTH * gfx.CrtFlickerScale() : 0.0f);
        setF(fl.barrel, gfx.BarrelAmount());
        int count = fx.waveCount < gfx.ShockwaveMax() ? fx.waveCount : gfx.ShockwaveMax();
        if (count > 0) SetShaderValueV(finalShader, fl.waves, fx.waves, SHADER_UNIFORM_VEC4, count);
        SetShaderValue(finalShader, fl.waveCount, &count, SHADER_UNIFORM_INT);
        setF(fl.chromatic, gfx.ChromaticShockwave() ? 1.0f : 0.0f);
        setF(fl.enrage, fx.enrage);
        setF(fl.hurt, fx.hurt);
        setF(fl.grade, 1.0f);
        // finalSrcRec luon co chieu cao AM (lat render texture) -> fragTexCoord.y nguoc chieu Y game
        setF(fl.flipY, finalSrcRec.height < 0.0f ? 1.0f : 0.0f);

        BeginShaderMode(finalShader);
            DrawTexturePro(finalSource->texture, finalSrcRec, destRec, { 0.0f, 0.0f }, 0.0f, WHITE);
        EndShaderMode();
    } else {
        DrawTexturePro(finalSource->texture, finalSrcRec, destRec, { 0.0f, 0.0f }, 0.0f, WHITE);
    }
}
