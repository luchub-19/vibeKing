#include "sprites.h"

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
        // ngay day la 1 muc tieu dac biet.
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
}

void SpriteSheet::Load() {
    player      = BuildShip();
    basicAlien  = BuildDiamondAlien();
    tankyAlien  = BuildTankyAlien();
    zigzagAlien = BuildZigzagAlien();
    ufo         = BuildUfo();
    kamikaze    = BuildKamikaze();
    boss        = BuildBoss();
}

void SpriteSheet::Unload() {
    UnloadTexture(player);
    UnloadTexture(basicAlien);
    UnloadTexture(tankyAlien);
    UnloadTexture(zigzagAlien);
    UnloadTexture(ufo);
    UnloadTexture(kamikaze);
    UnloadTexture(boss);
}
