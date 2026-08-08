#version 330

// BLOOM - buoc 1/3: trich vung SANG tu anh goc.
//
// Nhan anh goc (renderTarget cua GameManager, chua toan bo gameplay da ve), chi giu lai
// mau tai nhung pixel co do sang (luma) VUOT NGUONG `threshold`, phan con lai tra ve den
// tuyet doi. Ket qua la 1 "mat na" chi chua cac vung se phat sang (dan/muzzle flash/vien
// Boss...), lam dau vao cho 2 buoc blur (blur.fs) ngay sau - xem PostProcess::Render()
// (post_process.cpp).
//
// `threshold`/`intensity` la Config::BLOOM_THRESHOLD/BLOOM_INTENSITY (config.h), set 1
// LAN duy nhat trong PostProcess::Init() - khong doi moi frame nen khong set lai trong
// Render().

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float threshold; // Nguong luma (0..1) - duoi nguong nay tra ve den tuyet doi
uniform float intensity; // He so nhan them vao phan VUOT nguong - cho phep "chay sang" manh hon nguong 1.0

out vec4 finalColor;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    // Do sang theo he so chuan Rec.709 (giong da so engine dung cho luma tu RGB tuyen tinh).
    float luma = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));

    // Soft threshold: 0 duoi nguong, tang dan tuyen tinh tu 0->1 khi luma vuot threshold
    // toi 1.0 - tranh vien cung/rang cua giua vung sang va vung toi so voi hard-cutoff.
    float contribution = clamp((luma - threshold) / max(1.0 - threshold, 0.0001), 0.0, 1.0);

    finalColor = vec4(texColor.rgb * contribution * intensity, 1.0);
}
