#pragma once
#include "raylib.h"

// ==========================================
// BANG MAU - 1 NGUON DUY NHAT cho moi mau xuat hien trong game.
//
// TRUOC DAY moi noi tu goi thang hang so mau mac dinh cua raylib (PURPLE, MAROON, SKYBLUE,
// GREEN, RED, YELLOW, ORANGE, LIME...). Ba van de do:
//   1. Khong co bang mau nao ca - chi la "cai nao nghe hop ly luc do", nen tat ca deu bao
//      hoa toi da va khong cai nao noi bat hon cai nao. Nhin anh chup game that: mat khong
//      biet nhin dau truoc.
//   2. Cung 1 mau mang NHIEU y nghia khac nhau: RED vua la Tanky (dich thuong, ban vai
//      phat la xong) vua la dan dich (cham vao la mat mang). Mau khong con noi len dieu gi.
//   3. Doi mau 1 thu phai lung tim khap 5 file.
//
// QUY LUAT (nguoi dung chon 2026-09-03): LANH = an toan de quan sat, NONG = phai phan ung.
//   - Toan bo NEN va MOI LOAI DICH nam trong dai LANH (tim / xanh duong / xanh lo / xanh ngoc).
//   - Dai NONG (cam / vang / do / trang nong) CHI danh cho dung 3 nhom:
//       (a) dan - ca dan player lan dan dich,
//       (b) moi de doa lao THANG vao player (Kamikaze), va boss dang "enrage",
//       (c) phan thuong - power-up, UFO, diem/combo.
// Nho vay mat nguoi choi tu dong bat mau nong = "thu phai ne hoac phai nhat", khong can
// hoc thuoc bang mau. Day cung la ly do Tanky KHONG con mau do: do gio danh rieng cho
// nguy hiem tuc thi.
//
// HE QUA KY THUAT: bloom (Config::BLOOM_THRESHOLD, config.h) truoc day gan nhu khong lam
// gi vi khong vat the gameplay nao du sang de vuot nguong luma. Dai NONG o day co do sang
// cao han (dan player #FFF4C2 luma ~0.94) nen bloom bat dau THAT SU phat sang dung nhung
// thu can gay chu y - khong phai them 1 dong shader nao.
//
// constexpr, KHONG inline: day la du lieu TRINH BAY (giong SCREEN_W/TRANSITION_DURATION),
// khong phai du lieu can bang gameplay, nen co y KHONG nam trong assets/balance.json /
// Config::LoadBalance() - xem quy uoc constexpr-vs-inline o dau config.h.
// ==========================================
namespace Palette {

// --- NEN ---
constexpr Color Background   = { 6, 7, 15, 255 };      // #06070F - den ngA xanh, khong phai den tuyet doi

// --- DAI LANH: dich (khong bao gio dung mau nong cho 1 loai dich dung yen trong luoi) ---
constexpr Color BasicA       = { 123, 91, 214, 255 };  // #7B5BD6 - hang chan
constexpr Color BasicB       = { 156, 130, 232, 255 }; // #9C82E8 - hang le (xen ke nhu truoc)
constexpr Color Zigzag       = { 69, 169, 214, 255 };  // #45A9D6
constexpr Color Tanky        = { 74, 111, 184, 255 };  // #4A6FB8 - TRUOC DAY la MAROON (do); doi vi do gio danh rieng cho nguy hiem tuc thi
constexpr Color Warden       = { 47, 76, 140, 255 };   // #2F4C8C - navy dam, "day mau" doc ra tu do TOI
constexpr Color Medic        = { 63, 201, 160, 255 };  // #3FC9A0 - xanh ngoc, SANG nhat trong dai lanh -> de nham, dung y "diet healer truoc"
constexpr Color Weaver       = { 127, 227, 240, 255 }; // #7FE3F0 - xanh bang rat sang, duong bay lac nen can de bam mat
constexpr Color Bomber       = { 110, 143, 168, 255 }; // #6E8FA8 - xam thep tram; ban than no khong nguy hiem, BOM cua no moi nguy hiem (va bom la dan dich = mau nong)
constexpr Color Boss         = { 110, 75, 196, 255 };  // #6E4BC4

// --- DAI NONG (a): dan ---
constexpr Color PlayerBullet = { 255, 244, 194, 255 }; // #FFF4C2 - gan trang, luma ~0.94 -> vuot nguong bloom ro rang
constexpr Color EnemyBullet  = { 255, 77, 61, 255 };   // #FF4D3D - DO chi co nghia duy nhat: cham vao la mat mang

// --- DAI NONG (b): de doa tuc thi / boss enrage ---
constexpr Color Kamikaze     = { 255, 122, 47, 255 };  // #FF7A2F - loai DUY NHAT lao thang vao player
constexpr Color BossEnrage1  = { 255, 163, 64, 255 };  // #FFA340 - stage 2
constexpr Color BossEnrage2  = { 255, 77, 61, 255 };   // #FF4D3D - stage 3, trung mau dan dich co y: "moi thu o day deu giet duoc ban"

// --- DAI NONG (c): phan thuong ---
constexpr Color PowerUp      = { 255, 201, 66, 255 };  // #FFC942
constexpr Color Ufo          = { 255, 176, 58, 255 };  // #FFB03A - muc tieu thuong diem, khong phai de doa
constexpr Color ScoreText    = { 255, 216, 77, 255 };  // #FFD84D - popup diem/combo

// --- PLAYER ---
constexpr Color PlayerShip   = { 124, 255, 178, 255 }; // #7CFFB2 - xanh bac ha sang; mau DUY NHAT nay khong thuoc dai nao khac -> mat luon tim thay tau minh
constexpr Color PlayerThrust = { 255, 150, 60, 255 };  // #FF963C - lua day, dai nong (nhung la nguon sang cua chinh nguoi choi)

// --- LA CHAN (bunker): noi suy giua 2 mau nay theo % voxel con nguyen ---
// Nguyen ven -> BunkerIntact (sang, day dan); sap vo -> BunkerCritical (toi, xin xiu).
// Nho vay do ben cua la chan doc duoc BANG MAU tu xa, khong can dem lo thung.
// DO SANG co y de THAP hon Palette::PlayerShip: la chan la vat CHE CO tinh, khong phai tieu
// diem. Ban dau chon #5FD99A cho "nguyen ven" - do sang do vuot nguong bloom manh den muc 4
// la chan phat sang hon ca phi thuyen nguoi choi, keo het su chu y xuong day man hinh (thay
// ro khi chup lai sau khi ap bang mau). Ha xuong dai nay: van doc duoc ro "khoe vs sap vo"
// nhung khong con canh tranh anh sang voi player/dan.
constexpr Color BunkerIntact   = { 71, 181, 131, 255 }; // #47B583
constexpr Color BunkerCritical = { 38, 97, 74, 255 };   // #26614A

// --- UI / HUD ---
constexpr Color UiAccent     = { 255, 201, 66, 255 };  // #FFC942 - tieu de muc, lua chon dang active
constexpr Color UiText       = { 226, 232, 240, 255 }; // #E2E8F0 - trang hoi lanh, do doc tot hon WHITE thuan tren nen toi
constexpr Color UiDim        = { 118, 128, 150, 255 }; // #768096 - chu phu/khong kha dung
constexpr Color UiPanelFill  = { 16, 18, 32, 255 };    // Alpha duoc dat rieng tai noi ve (Config::HUD_PANEL_ALPHA)
constexpr Color UiPanelEdge  = { 82, 92, 122, 255 };
constexpr Color UiDanger     = { 255, 77, 61, 255 };   // GAME OVER - cung do voi dan dich, co y
constexpr Color UiSuccess    = { 63, 201, 160, 255 };  // WAVE CLEARED / da mo khoa

// Tron tuyen tinh 2 mau theo t in [0,1] (0 = a, 1 = b). Dung cho la chan (noi suy theo do
// ben) va bat ky cho nao can 1 dai mau lien tuc thay vi vai bac roi rac.
inline Color Lerp(Color a, Color b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return Color{
        (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t),
        (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t),
        (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t),
        (unsigned char)((float)a.a + ((float)b.a - (float)a.a) * t),
    };
}

// Nhan do sang (giu nguyen alpha) - dung de tao vien sang/bong toi tu 1 mau goc thay vi
// khai bao them 2 hang so mau cho moi vat the.
inline Color Shade(Color c, float mul) {
    auto clamp255 = [](float v) -> unsigned char {
        if (v < 0.0f) v = 0.0f;
        if (v > 255.0f) v = 255.0f;
        return (unsigned char)v;
    };
    return Color{ clamp255((float)c.r * mul), clamp255((float)c.g * mul), clamp255((float)c.b * mul), c.a };
}

} // namespace Palette
