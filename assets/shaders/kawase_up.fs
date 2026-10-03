#version 330

// DUAL KAWASE - buoc PHONG LON (xem kawase_down.fs). 8 mau tren vong kim cuong ban kinh 1 texel
// cua texture NHO (4 canh trong so 1, 4 cheo trong so 2) - lam mem ranh gioi khoi vuong khi
// phong to, thay cho upscale bilinear tran (se ra cac khoi sang vuong vuc).

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 halfpixel; // 0.5 / kich thuoc texture NGUON (texture nho hon)

out vec4 finalColor;

void main()
{
    vec2 hp = halfpixel;
    vec4 sum = texture(texture0, fragTexCoord + vec2(-hp.x * 2.0, 0.0));
    sum += texture(texture0, fragTexCoord + vec2(-hp.x, hp.y)) * 2.0;
    sum += texture(texture0, fragTexCoord + vec2(0.0, hp.y * 2.0));
    sum += texture(texture0, fragTexCoord + vec2(hp.x, hp.y)) * 2.0;
    sum += texture(texture0, fragTexCoord + vec2(hp.x * 2.0, 0.0));
    sum += texture(texture0, fragTexCoord + vec2(hp.x, -hp.y)) * 2.0;
    sum += texture(texture0, fragTexCoord + vec2(0.0, -hp.y * 2.0));
    sum += texture(texture0, fragTexCoord + vec2(-hp.x, -hp.y)) * 2.0;
    finalColor = vec4((sum / 12.0).rgb, 1.0);
}
