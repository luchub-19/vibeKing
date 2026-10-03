#version 330

// DUAL KAWASE - buoc THU NHO (GD 3, docs/GRAPHICS_UPGRADE_PLAN.md). Nguon: Marius Bjorge (ARM),
// "Bandwidth-efficient rendering", SIGGRAPH 2015; giai thich tai frost.kiwi/dual-kawase.
// Ve texture nguon vao render texture NHO BANG NUA: 1 mau tam (trong so 4) + 4 mau cheo o
// khoang nua texel - nho loc bilinear, 5 lan doc ~ trung binh 16 texel. Chuoi 1/2 -> 1/4 -> 1/8
// cho quang sang rong voi ~7% bang thong cua Gauss tuyen tinh - dung thu iGPU can.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 halfpixel; // 0.5 / kich thuoc texture NGUON

out vec4 finalColor;

void main()
{
    vec4 sum = texture(texture0, fragTexCoord) * 4.0;
    sum += texture(texture0, fragTexCoord - halfpixel);
    sum += texture(texture0, fragTexCoord + halfpixel);
    sum += texture(texture0, fragTexCoord + vec2(halfpixel.x, -halfpixel.y));
    sum += texture(texture0, fragTexCoord - vec2(halfpixel.x, -halfpixel.y));
    finalColor = vec4((sum / 8.0).rgb, 1.0);
}
