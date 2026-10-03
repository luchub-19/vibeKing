#version 330

// PASS CUOI HOP NHAT (GD 3 - docs/GRAPHICS_UPGRADE_PLAN.md). Truoc day file nay la crt.fs chi
// lam scanline/vignette/flicker. Gio gom MOI hieu ung toan man hinh vao 1 lan ve, vi bench tren
// llvmpipe cho thay moi pass them (1 lan doi render target) ton hon chinh phep tinh trong pass:
//   1. CRT cong (barrel) - CHI preset High + CRT bat. Cong ben trong destRec, ngoai cong = den.
//      Toa do nhap/HUD debug khong doi (ve sau pass nay) - xu ly duoc ly do cu "khong cong".
//   2. Song xung kich: dich UV quanh vong dang no ra (toa do game 800x600).
//   3. Tach RGB CHI o mep song (preset High) - khong ap aberration toan man hinh thuong truc
//      vi lam nhoe dan (luat R1).
//   4. Chinh mau nhe: S-curve + bao hoa +8%. 5. Khu bao hoa khi vua trung don.
//   6. Vien ngả DO khi boss enrage (do = nguy hiem, luat lanh/nong).
//   7. Scanline/vignette/flicker cu - strength = 0 khi tat CRT.
//   8. Loc mau mu mau (Cai dat > Tro nang): daltonize - xem graphics_settings.h ColorFilter.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;          // Kich thuoc THAT tren man hinh (destRec) - mat do scanline
uniform vec2 gameSize;            // 800x600 - don vi cua waves
uniform float time;
uniform float scanlineStrength;
uniform float vignetteStrength;
uniform float flickerStrength;
uniform float barrel;             // 0 = phang
uniform vec4 waves[8];            // (x, y, banKinh, doManh) px toa do game
uniform int waveCount;
uniform float chromatic;          // 0/1
uniform float enrage;             // 0..1
uniform float hurt;               // 0..1
uniform float grade;              // 0..1 do manh chinh mau
uniform float flipY;              // 1 neu fragTexCoord.y nguoc chieu Y game (render texture)
uniform int colorFilter;          // 0 tat, 1 protan, 2 deutan, 3 tritan

// DALTONIZE (Fidaner, Lin, Ozguven 2005): RGB -> LMS, bo kenh non bi thieu (mo phong cach mat
// loai do nhin), lay phan SAI KHAC so voi anh goc - tuc thong tin mau ma nguoi do bi mat - roi
// cong sang cac kenh ho van phan biet duoc. Ket qua: do/luc (protan/deutan) hoac lam/vang
// (tritan) tach nhau ro hon thay vi lan vao nhau.
vec3 daltonize(vec3 rgb, int mode) {
    // Ma tran cot (GLSL mat3 nhap theo COT) - gia tri tu bai bao goc.
    mat3 rgb2lms = mat3(17.8824, 3.45565, 0.0299566,
                        43.5161, 27.1554, 0.184309,
                        4.11935, 3.86714, 1.46709);
    mat3 lms2rgb = mat3(0.0809444479, -0.0102485335, -0.000365296938,
                        -0.130504409, 0.0540193266, -0.00412161469,
                        0.116721066, -0.113614708, 0.693511405);
    vec3 lms = rgb2lms * rgb;
    vec3 sim;
    if (mode == 1)      sim = vec3(2.02344 * lms.y - 2.52581 * lms.z, lms.y, lms.z);
    else if (mode == 2) sim = vec3(lms.x, 0.494207 * lms.x + 1.24827 * lms.z, lms.z);
    else                sim = vec3(lms.x, lms.y, -0.395913 * lms.x + 0.801109 * lms.y);
    vec3 err = rgb - lms2rgb * sim;
    vec3 shift = vec3(0.0, 0.7 * err.r + err.g, 0.7 * err.r + err.b);
    return clamp(rgb + shift, 0.0, 1.0);
}

out vec4 finalColor;

void main()
{
    // 1) Barrel: toa do quanh tam, phong ra theo binh phuong khoang cach.
    vec2 uv = fragTexCoord;
    vec2 c = uv - 0.5;
    uv = 0.5 + c * (1.0 + barrel * dot(c, c) * 4.0);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 2) Song xung kich - tinh trong px game de vong tron tron that (khong bi ep theo ty le).
    vec2 gameUV = vec2(uv.x, mix(uv.y, 1.0 - uv.y, flipY));
    vec2 p = gameUV * gameSize;
    vec2 offsetPx = vec2(0.0);
    for (int i = 0; i < 8; i++) {
        if (i >= waveCount) break;
        vec2 d = p - waves[i].xy;
        float dist = length(d);
        float ring = waves[i].z;
        float thickness = 10.0 + ring * 0.15; // Mep song day dan khi no rong
        float x = (dist - ring) / thickness;
        if (abs(x) < 1.0 && dist > 0.001) {
            // Phia trong mep keo vao, phia ngoai day ra -> vien song "lom" nhu thau kinh
            offsetPx += (d / dist) * waves[i].w * sin(x * 3.14159265) * (1.0 - x * x);
        }
    }
    // Dai HUD tren cung (diem/mang/mau boss - Config::HUD_TOP_BAND_H + le): tat dan do meo de so
    // lieu quan trong luon doc duoc. Truoc day song di qua lam HUD "chay" theo (thay o anh GD 5).
    offsetPx *= smoothstep(44.0, 60.0, p.y);
    vec2 offsetUV = offsetPx / gameSize;
    offsetUV.y *= mix(1.0, -1.0, flipY);

    vec3 col;
    if (chromatic > 0.5 && dot(offsetPx, offsetPx) > 0.01) {
        // 3) Kenh do lech nhieu hon, xanh lech it hon -> vien cau vong mong CHI tren mep song
        col.r = texture(texture0, uv - offsetUV * 1.35).r;
        col.g = texture(texture0, uv - offsetUV).g;
        col.b = texture(texture0, uv - offsetUV * 0.65).b;
    } else {
        col = texture(texture0, uv - offsetUV).rgb;
    }
    col *= (colDiffuse * fragColor).rgb;

    // 4) Chinh mau: S-curve nhe (toi toi hon, sang sang hon) + bao hoa them 8%.
    float luma = dot(col, vec3(0.2126, 0.7152, 0.0722));
    col = mix(col, col * col * (3.0 - 2.0 * col), 0.18 * grade);
    col = mix(vec3(luma), col, 1.0 + 0.08 * grade);
    // 5) Vua trung don: gan nhu xam trong khoanh khac (toi da 75%, giu chut mau de dan do van doc)
    col = mix(col, vec3(luma), hurt * 0.75);

    // Vignette dung chung cho CRT va enrage. centered tren toa do da cong (khop hinh dang man).
    vec2 centered = uv - 0.5;
    float edge = clamp(dot(centered, centered) * 2.0, 0.0, 1.0); // 0 tam -> 1 goc
    // 6) Enrage: ngả do o vien, KHONG nhap nhay (khong vi pham reduceFlashing).
    // edge^2 + 0.14: ban dau tuyen tinh voi 0.22 -> anh chup ca nen goc ngả nau do, qua nang.
    col += vec3(0.14, 0.012, 0.0) * enrage * edge * edge;

    // 7) CRT
    float scanline = sin(fragTexCoord.y * resolution.y * 3.14159265);
    col *= mix(1.0, 0.5 + 0.5 * scanline, scanlineStrength);
    col *= clamp(1.0 - dot(centered, centered) * vignetteStrength, 0.0, 1.0);
    col *= 1.0 + sin(time * 30.0) * flickerStrength;

    // 8) Loc mau CUOI cung - sau moi chinh mau khac, de bu dung cai mat se thay.
    if (colorFilter > 0) col = daltonize(clamp(col, 0.0, 1.0), colorFilter);

    finalColor = vec4(col, 1.0);
}
