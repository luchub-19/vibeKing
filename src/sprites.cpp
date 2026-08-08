#include "sprites.h"
#include "config.h"
#include "text_utils.h"
#include <fstream>
#include <charconv>
#include <unordered_map>
#include <string>

using TextUtils::Trim;

namespace {
    constexpr int SPRITE_SIZE = 16;

    Texture2D BuildShip() {
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        // Mui nhon o tren, than giua, day loe rong - hinh khoi don gian goi dang "phi
        // thuyen" thay vi 1 khoi chu nhat tron, khong sao chep pixel art cua bat ky
        // game nao (chi la cac hinh chu nhat xep tang).
        ImageDrawRectangle(&img, 7, 0, 2, 5, WHITE);
        ImageDrawRectangle(&img, 4, 5, 8, 5, WHITE);
        ImageDrawRectangle(&img, 0, 10, 16, 5, WHITE);
        ImageDrawRectangle(&img, 0, 10, 3, 3, BLANK); // Vat 2 goc day cho bot "khoi hop"
        ImageDrawRectangle(&img, 13, 10, 3, 3, BLANK);
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildDiamondAlien() {
        // Dich thuong: hinh thoi xep tu cac dai ngang rong dan roi hep lai - de phan
        // biet voi hinh chu nhat thuan tuy o xa.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 6, 1, 4, 2, WHITE);
        ImageDrawRectangle(&img, 3, 3, 10, 2, WHITE);
        ImageDrawRectangle(&img, 1, 5, 14, 4, WHITE);
        ImageDrawRectangle(&img, 3, 9, 10, 2, WHITE);
        ImageDrawRectangle(&img, 6, 11, 4, 2, WHITE);
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildTankyAlien() {
        // Dich mau day: khoi vuong day, vien trong day hon de goi cam giac "boc giap".
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 1, 1, 14, 14, WHITE);
        ImageDrawRectangle(&img, 4, 4, 8, 8, BLANK); // Khoet giua -> nhin ro la "khung", khong phai khoi dac
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildZigzagAlien() {
        // Dich zigzag: hinh tron - goi cam giac "khong on dinh, kho ngam" khac han
        // 2 loai con lai deu la khoi vuong/thoi.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawCircle(&img, 8, 8, 7, WHITE);
        ImageDrawCircle(&img, 8, 8, 3, BLANK);
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildUfo() {
        // Mystery Ship: than dia bay bau duc dep + vom kinh nho o giua - hinh dang
        // nguyen ban rieng biet, khong giong bat ky loai dich nao khac trong game.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 1, 6, 14, 4, WHITE);   // Than dia dep
        ImageDrawRectangle(&img, 3, 4, 10, 2, WHITE);   // Vai tren thu hep dan
        ImageDrawCircle(&img, 8, 5, 3, WHITE);          // Vom kinh (mai vom nho o giua)
        ImageDrawRectangle(&img, 0, 10, 4, 2, WHITE);   // 2 chan/canh 2 ben nho ra
        ImageDrawRectangle(&img, 12, 10, 4, 2, WHITE);
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildKamikaze() {
        // Mui ten nhon huong xuong - goi cam giac "lao thang toi", khac han cac hinh
        // khoi/thoi/tron cua 3 loai dich con lai.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 6, 0, 4, 6, WHITE);
        ImageDrawRectangle(&img, 3, 5, 10, 4, WHITE);
        ImageDrawTriangle(&img, {1, 9}, {8, 15}, {15, 9}, WHITE); // Mui nhon huong xuong duoi
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildBoss() {
        // Khoi lon co cau truc phan tang ro rang (than chinh + 2 canh + loi tam o giua)
        // - hinh dang nguyen ban, khac biet han moi loai dich khac de nguoi choi nhan ra
        // ngay day la 1 muc tieu dac biet. Dung cho BossType::Vanguard.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 2, 4, 12, 8, WHITE);   // Than chinh
        ImageDrawRectangle(&img, 0, 6, 3, 4, WHITE);    // Canh trai
        ImageDrawRectangle(&img, 13, 6, 3, 4, WHITE);   // Canh phai
        ImageDrawRectangle(&img, 6, 1, 4, 4, WHITE);    // Loi tam tren
        ImageDrawRectangle(&img, 5, 12, 6, 3, WHITE);   // Day
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildBossSentinel() {
        // Boss loai 2/3 (xem BossType trong enemy_types.h): than be ngang, thap hon
        // Vanguard, voi 1 VONG TRON lon o giua-tren goi ro "loi khien" - phan biet ngay
        // bang mat voi khoi vuong/nhon cua Vanguard: tron = "phong thu".
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 1, 7, 14, 6, WHITE);   // Than be ngang
        ImageDrawRectangle(&img, 0, 9, 2, 3, WHITE);    // Canh trai ngan
        ImageDrawRectangle(&img, 14, 9, 2, 3, WHITE);   // Canh phai ngan
        ImageDrawCircle(&img, 8, 6, 5, WHITE);          // Loi khien hinh tron
        ImageDrawCircle(&img, 8, 6, 2, BLANK);          // Khoet tam -> ro la "vong", khong phai khoi dac
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildBossSwarmer() {
        // Boss loai 3/3: than trung tam nho hep + 4 "vo" tam giac o 4 goc goi cam giac
        // "to ong dang phong thich quan" - dan trai/nhon hon han khoi tron cua Sentinel
        // hay khoi vuong cua Vanguard, khop voi hanh vi trieu hoi tiep vien cua no.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 6, 5, 4, 6, WHITE);               // Than trung tam
        ImageDrawTriangle(&img, {0, 2}, {5, 5}, {0, 8}, WHITE);    // Vo goc tren-trai
        ImageDrawTriangle(&img, {15, 2}, {10, 5}, {15, 8}, WHITE); // Vo goc tren-phai
        ImageDrawTriangle(&img, {0, 8}, {5, 11}, {0, 14}, WHITE);  // Vo goc duoi-trai
        ImageDrawTriangle(&img, {15, 8}, {10, 11}, {15, 14}, WHITE); // Vo goc duoi-phai
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    // ==========================================
    // ICON POWER-UP - 4 silhouette 16x16 rieng biet cho tung loai (xem PowerUpType trong
    // powerup.h), thay cho hinh chu nhat mau tron truoc day. Cung triet ly "khong asset
    // ngoai" voi moi sprite dich/Boss o tren: chi hinh hoc nguyen ban (rectangle/
    // triangle/circle), ve WHITE roi nhuom mau rieng tung loai luc DrawSprite() (xem
    // RenderSystem::DrawPlaying) - khong bake mau vao texture de tai su dung 1 texture
    // cho moi tint neu can sau nay.
    // ==========================================
    Texture2D BuildIconRapidFire() {
        // RAPID FIRE: 3 tam giac nho xep chong huong len, hoi long vao nhau - goi "nhieu
        // vien dan lien tiep, nhip ban don don" - khac han khoi/thoi/tron cua cac dich.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawTriangle(&img, {2, 5}, {8, 0}, {14, 5}, WHITE);
        ImageDrawTriangle(&img, {2, 10}, {8, 5}, {14, 10}, WHITE);
        ImageDrawTriangle(&img, {2, 15}, {8, 10}, {14, 15}, WHITE);
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildIconShield() {
        // SHIELD: khoi khien co dien - than vuong phia tren thu hep dan xuong 1 diem
        // nhon o day, khoet 1 o nho giua nhu phu hieu - quy mo NHO va dang khac han vong
        // tron rong cua khien Boss Sentinel (bao quanh toan bo Boss, xem BuildBossSentinel).
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 2, 1, 12, 8, WHITE);
        ImageDrawTriangle(&img, {2, 9}, {8, 15}, {14, 9}, WHITE);
        ImageDrawRectangle(&img, 5, 4, 6, 3, BLANK); // Khoet 1 o chu nhat nho - "phu hieu" tren mat khien
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildIconPiercing() {
        // PIERCING: 1 mui ten dai xuyen suot tu tren xuong duoi, xuyen qua 1 vong tron
        // nho o giua - goi truc tiep "dan xuyen qua muc tieu" thay vi chi 1 mui ten don
        // thuan chi huong ban.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawRectangle(&img, 7, 0, 2, 16, WHITE);             // Than mui ten xuyen suot
        ImageDrawTriangle(&img, {4, 5}, {8, 0}, {12, 5}, WHITE);  // Dau nhon tren
        ImageDrawCircle(&img, 8, 10, 5, WHITE);                   // Vong "muc tieu" quanh doan than bi xuyen
        ImageDrawCircle(&img, 8, 10, 2, BLANK);                   // Khoet tam -> ro la vong, khong phai khoi dac
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    Texture2D BuildIconCleanser() {
        // CLEANSER: hinh "tia sang/lap lanh" 4 canh (2 thoi mong vuong goc long vao
        // nhau) - goi cam giac "quet sach/toa sang" man hinh ngay lap tuc, khac han cac
        // hinh khoi dac/mui nhon mang tinh "tan cong" cua 3 icon con lai.
        Image img = GenImageColor(SPRITE_SIZE, SPRITE_SIZE, BLANK);
        ImageDrawTriangle(&img, {8, 0}, {6, 8}, {10, 8}, WHITE);   // Nhon tren
        ImageDrawTriangle(&img, {8, 15}, {10, 8}, {6, 8}, WHITE);  // Nhon duoi
        ImageDrawTriangle(&img, {0, 8}, {8, 6}, {8, 10}, WHITE);   // Nhon trai
        ImageDrawTriangle(&img, {15, 8}, {8, 10}, {8, 6}, WHITE);  // Nhon phai
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        return tex;
    }

    // ==========================================
    // ATLAS THAT (Phase 1 - Graphics/UI Overhaul, Nguoi 1): SpriteSheet::Load() thu nap
    // 13 sprite tu 1 file atlas.png + atlas.cfg (mac dinh: Kenney "Space Shooter Redux",
    // CC0 - xem docs/ASSET_INTEGRATION.md) THAY vi luon ve procedural o tren. Ten nao
    // KHONG co trong atlas.cfg / dong loi / vung toa do vuot bien anh -> FALLBACK ve dung
    // BuildXxx() procedural CHI cho rieng ten do, KHONG bao loi/crash toan bo - cung
    // triet ly "khong bao gio chet vi thieu file/field tuy chon" da dung cho
    // settings.cfg/level.cfg (xem level_config.cpp).
    // ==========================================
    struct AtlasRect { int x, y, w, h; };

    // Dinh dang atlas.cfg: NAME=X,Y,W,H (so nguyen, pixel, goc tren-trai atlas.png) - xem
    // docs/ASSET_INTEGRATION.md. Cung khuon KEY=VALUE + '#' comment nhu
    // LevelGridConfig::LoadFromFile (level_config.cpp), chi khac VALUE la 4 so tach boi
    // dau phay thay vi 1 so don.
    std::unordered_map<std::string, AtlasRect> ParseAtlasConfig(const char* path) {
        std::unordered_map<std::string, AtlasRect> regions;
        std::ifstream file(path);
        if (!file.is_open()) return regions; // Khong co atlas.cfg - caller tu quyet dinh log gi (xem Load() ben duoi)

        std::string line;
        while (std::getline(file, line)) {
            std::string_view trimmed = Trim(line);
            if (trimmed.empty() || trimmed[0] == '#') continue;

            size_t eq = trimmed.find('=');
            if (eq == std::string_view::npos) continue;
            std::string_view key = Trim(trimmed.substr(0, eq));
            std::string_view val = Trim(trimmed.substr(eq + 1));
            if (key.empty() || val.empty()) continue;

            // Tach thu cong qua 3 dau phay (X,Y,W,H) - du nhe cho 1 dong 4 so nguyen,
            // khong can sstream/regex. std::from_chars: khong throw, tra ve error code.
            int parts[4];
            bool ok = true;
            std::string_view rest = val;
            for (int i = 0; i < 4 && ok; i++) {
                size_t comma = (i < 3) ? rest.find(',') : rest.size();
                if (i < 3 && comma == std::string_view::npos) { ok = false; break; }
                std::string_view field = Trim(rest.substr(0, comma));
                auto res = std::from_chars(field.data(), field.data() + field.size(), parts[i]);
                if (res.ec != std::errc{} || res.ptr != field.data() + field.size()) ok = false;
                if (i < 3) rest = rest.substr(comma + 1);
            }

            if (!ok) {
                TraceLog(LOG_WARNING, "SpriteSheet: dong atlas.cfg khong hop le cho ten '%.*s' - dung procedural cho ten nay",
                          (int)key.size(), key.data());
                continue;
            }
            regions[std::string(key)] = AtlasRect{ parts[0], parts[1], parts[2], parts[3] };
        }
        return regions;
    }

    // Cat 1 vung con tu atlas (da nap san trong RAM) thanh Texture2D rieng. `regions`
    // rong (khong co atlas.cfg/atlas.png hop le) hoac thieu ten `name` -> goi
    // buildFallback() ngay, KHONG dung toi atlasImg (an toan ke ca khi atlasImg la Image
    // rong/chua tung nap). Vung toa do vuot bien anh (vd atlas.png bi thay bang ban nho
    // hon ma quen sua .cfg) cung fallback tuong tu, kem canh bao rieng.
    Texture2D LoadAtlasEntry(const Image& atlasImg, const std::unordered_map<std::string, AtlasRect>& regions,
                              const char* name, Texture2D (*buildFallback)()) {
        auto it = regions.find(name);
        if (it == regions.end()) return buildFallback();

        const AtlasRect& r = it->second;
        bool inBounds = r.x >= 0 && r.y >= 0 && r.w > 0 && r.h > 0 &&
                        r.x + r.w <= atlasImg.width && r.y + r.h <= atlasImg.height;
        if (!inBounds) {
            TraceLog(LOG_WARNING, "SpriteSheet: vung toa do cua '%s' vuot bien atlas.png - dung procedural cho ten nay", name);
            return buildFallback();
        }

        Image cropped = ImageFromImage(atlasImg, Rectangle{ (float)r.x, (float)r.y, (float)r.w, (float)r.h });
        Texture2D tex = LoadTextureFromImage(cropped);
        UnloadImage(cropped);
        return tex;
    }
}

void SpriteSheet::Load() {
    // Thu nap atlas.png 1 LAN DUY NHAT trong RAM, dung chung de cat ca 13 ten, thay vi mo
    // lai file cho tung ten rieng le. `regions` CHI khac rong khi atlasImg nap thanh cong
    // (xem nhanh if ben duoi) nen LoadAtlasEntry() doc atlasImg.width/height luon an toan.
    Image atlasImg{};
    std::unordered_map<std::string, AtlasRect> regions;

    if (FileExists(Config::AtlasImagePath())) {
        atlasImg = LoadImage(Config::AtlasImagePath());
        if (atlasImg.data != nullptr) {
            regions = ParseAtlasConfig(Config::AtlasConfigPath());
            TraceLog(LOG_INFO, "SpriteSheet: da nap '%s' (%dx%d) - %zu/13 ten hop le trong atlas.cfg, con lai dung procedural",
                      Config::AtlasImagePath(), atlasImg.width, atlasImg.height, regions.size());
        } else {
            TraceLog(LOG_WARNING, "SpriteSheet: '%s' ton tai nhung khong doc duoc - dung toan bo sprite procedural",
                      Config::AtlasImagePath());
        }
    } else {
        TraceLog(LOG_INFO, "SpriteSheet: khong tim thay '%s', dung toan bo sprite procedural", Config::AtlasImagePath());
    }

    player       = LoadAtlasEntry(atlasImg, regions, "player", BuildShip);
    basicAlien   = LoadAtlasEntry(atlasImg, regions, "basicAlien", BuildDiamondAlien);
    tankyAlien   = LoadAtlasEntry(atlasImg, regions, "tankyAlien", BuildTankyAlien);
    zigzagAlien  = LoadAtlasEntry(atlasImg, regions, "zigzagAlien", BuildZigzagAlien);
    ufo          = LoadAtlasEntry(atlasImg, regions, "ufo", BuildUfo);
    kamikaze     = LoadAtlasEntry(atlasImg, regions, "kamikaze", BuildKamikaze);
    boss         = LoadAtlasEntry(atlasImg, regions, "boss", BuildBoss);
    bossSentinel = LoadAtlasEntry(atlasImg, regions, "bossSentinel", BuildBossSentinel);
    bossSwarmer  = LoadAtlasEntry(atlasImg, regions, "bossSwarmer", BuildBossSwarmer);

    iconRapidFire = LoadAtlasEntry(atlasImg, regions, "iconRapidFire", BuildIconRapidFire);
    iconShield    = LoadAtlasEntry(atlasImg, regions, "iconShield", BuildIconShield);
    iconPiercing  = LoadAtlasEntry(atlasImg, regions, "iconPiercing", BuildIconPiercing);
    iconCleanser  = LoadAtlasEntry(atlasImg, regions, "iconCleanser", BuildIconCleanser);

    if (atlasImg.data != nullptr) UnloadImage(atlasImg);
}

void SpriteSheet::Unload() {
    UnloadTexture(player);
    UnloadTexture(basicAlien);
    UnloadTexture(tankyAlien);
    UnloadTexture(zigzagAlien);
    UnloadTexture(ufo);
    UnloadTexture(kamikaze);
    UnloadTexture(boss);
    UnloadTexture(bossSentinel);
    UnloadTexture(bossSwarmer);

    UnloadTexture(iconRapidFire);
    UnloadTexture(iconShield);
    UnloadTexture(iconPiercing);
    UnloadTexture(iconCleanser);
}
