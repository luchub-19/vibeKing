#version 330

// TINH VAN PROCEDURAL (GD 1 - docs/GRAPHICS_UPGRADE_PLAN.md). Chay 1 LAN moi lan "nuong"
// (Nebula::Bake - doi chuong wave / doi preset), KHONG chay moi frame: fbm 5 octave x 3 lan
// (domain warp) tren 400x300 la qua dat de lam 60 lan/giay tren iGPU, nhung la 1 lan duy nhat
// thi khong dang ke. Moi frame chi cuon texture da nuong.
//
// TUAN HOAN (tileable) theo ca 2 truc voi chu ky = 1 texture: luoi hash lay mod theo `period`
// cua tung octave, nen texture lat canh nhau khong lo duong noi - cho phep cuon vo han bang
// TEXTURE_WRAP_REPEAT. Domain warp (uv + q*0.35) giu tinh tuan hoan vi q cung tuan hoan.
//
// Dung gl_FragCoord thay fragTexCoord: ve bang DrawRectangle (khong co UV 0..1 thuc su).

uniform vec2 resolution;   // Kich thuoc render texture dang nuong
uniform float seed;        // Moi lop/chuong 1 seed -> hinh khac nhau
uniform vec3 colorA;       // 2 mau dai LANH (Palette::Nebula*), toi
uniform vec3 colorB;
uniform float baseFreq;    // So "dam may" lon tren 1 chu ky

out vec4 finalColor;

float hash(vec2 p, float period) {
    p = mod(p, period);
    return fract(sin(dot(p, vec2(127.1, 311.7)) + seed * 17.13) * 43758.5453);
}

float vnoise(vec2 p, float period) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    float a = hash(i, period);
    float b = hash(i + vec2(1.0, 0.0), period);
    float c = hash(i + vec2(0.0, 1.0), period);
    float d = hash(i + vec2(1.0, 1.0), period);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 uv) {
    float v = 0.0;
    float amp = 0.5;
    float freq = baseFreq;
    for (int o = 0; o < 5; o++) {
        v += amp * vnoise(uv * freq, freq);
        freq *= 2.0;
        amp *= 0.5;
    }
    return v;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution;
    vec2 q = vec2(fbm(uv), fbm(uv + vec2(0.37, 0.71)));
    float n = fbm(uv + q * 0.35);
    // Phan "day" cua nhieu thanh may + 1 lop suong rat mong o phan con lai. Nguong dau tien
    // (0.42..0.82) cho mat do trung binh chi 6.6% - fbm value-noise dồn quanh 0.5, bien do hep -
    // ra 1-2 cot may le loi giua man hinh den. Do bang cach xuat thang texture da nuong.
    float density = smoothstep(0.34, 0.72, n) * 0.85 + smoothstep(0.22, 0.55, n) * 0.15;
    vec3 col = mix(colorA, colorB, clamp(q.x * 1.3 - 0.15, 0.0, 1.0));
    finalColor = vec4(col, density);
}
