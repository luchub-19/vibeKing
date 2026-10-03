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

    finalColor = vec4(col, 1.0);
}
