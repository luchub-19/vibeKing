#version 330

// CRT: scanline + vignette + nhap nhay nhe, ap dung o buoc VE CUOI CUNG (sau khi da cong
// don Bloom neu Bloom cung bat - xem PostProcess::Render() trong post_process.cpp). Cố
// tinh KHONG lam cong vien man hinh (barrel distortion) - doi UV se keo theo hang loat
// van de vien/toa do khac (destRec letterbox, DrawDebugOverlay ve rieng ben ngoai...),
// khong dang danh doi cho 1 hieu ung "cho vui" o 1 do an nho.
//
// `scanlineStrength`/`vignetteStrength`/`flickerStrength` la Config::CRT_* (config.h),
// set 1 LAN trong PostProcess::Init(). `time`/`resolution` doi moi frame (nhap nhay theo
// GetTime(), mat do scanline theo dung kich thuoc PIXEL THAT tren man hinh - xem
// PostProcess::Render()).

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;          // Kich thuoc THAT tren man hinh (destRec, KHONG phai 800x600 noi bo)
uniform float time;
uniform float scanlineStrength;
uniform float vignetteStrength;
uniform float flickerStrength;

out vec4 finalColor;

void main()
{
    vec4 color = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    // Scanline: sin theo TUNG HANG PIXEL MAN HINH THAT (resolution.y, khong phai texture
    // noi bo) - moi hang xen ke sang/toi, mat do luon dung du Fullscreen ty le nao.
    float scanline = sin(fragTexCoord.y * resolution.y * 3.14159265);
    color.rgb *= mix(1.0, 0.5 + 0.5 * scanline, scanlineStrength);

    // Vignette: toi dan ra vien theo khoang cach binh phuong toi tam, tam man hinh giu
    // nguyen do sang (khoang_cach = 0 -> he so 1.0).
    vec2 centered = fragTexCoord - 0.5;
    float vignette = clamp(1.0 - dot(centered, centered) * vignetteStrength, 0.0, 1.0);
    color.rgb *= vignette;

    // Nhap nhay rat nhe theo thoi gian - gia lap do khong on dinh dien ap CRT that, bien
    // do nho (Config::CRT_FLICKER_STRENGTH mac dinh 0.015) de khong gay kho chiu/loa mat.
    color.rgb *= 1.0 + sin(time * 30.0) * flickerStrength;

    finalColor = color;
}
