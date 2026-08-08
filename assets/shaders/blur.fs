#version 330

// BLOOM - buoc 2/3: blur Gauss 1 CHIEU, dung LAI cho ca 2 pass ngang va doc (doi bang
// `direction` - xem PostProcess::Render() trong post_process.cpp goi 2 lan voi (1,0) roi
// (0,1), dung ky thuat ping-pong 2 texture thay vi 1 pass 2-chieu de re hon). Kernel 9-tap
// chuan (trong so xap xi Gauss sigma~2, tong = 1.0) - du muot cho quy mo game nay, khong
// can toi uu linear-sampling (gop 2 texel/lan doc).
//
// `texelSize`/`spread` la Config::BLOOM_BLUR_SPREAD (config.h) + kich thuoc texture trung
// gian - set 1 LAN trong PostProcess::Init() (khong doi trong 1 phien chay). `direction`
// la uniform DUY NHAT doi moi lan goi trong Render().

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 direction; // (1,0) = pass ngang, (0,1) = pass doc
uniform vec2 texelSize; // 1.0 / kich thuoc texture (px) - offset tinh theo texel, doc lap do phan giai
uniform float spread;   // He so nhan them vao buoc offset - lon hon = quang blur loang rong hon

out vec4 finalColor;

void main()
{
    vec2 step = direction * texelSize * spread;

    vec4 color  = texture(texture0, fragTexCoord)            * 0.227027;
    color      += texture(texture0, fragTexCoord + step)     * 0.1945946;
    color      += texture(texture0, fragTexCoord - step)     * 0.1945946;
    color      += texture(texture0, fragTexCoord + step * 2.0) * 0.1216216;
    color      += texture(texture0, fragTexCoord - step * 2.0) * 0.1216216;
    color      += texture(texture0, fragTexCoord + step * 3.0) * 0.054054;
    color      += texture(texture0, fragTexCoord - step * 3.0) * 0.054054;
    color      += texture(texture0, fragTexCoord + step * 4.0) * 0.016216;
    color      += texture(texture0, fragTexCoord - step * 4.0) * 0.016216;

    finalColor = color * colDiffuse * fragColor;
}
