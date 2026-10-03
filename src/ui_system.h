#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// ==========================================
// UI CANVAS / WIDGET
// Truoc day moi man hinh (Menu/HUD/EndScreen) goi thang DrawTextEx (qua wrapper
// DrawGameText) va DrawRectangle rai rac tai tung diem trong RenderSystem - "ve 1 dong
// chu" va "ve 1 thanh mau" la 2 khai niem lap di lap lai nhung khong co kieu du lieu
// chung nao dai dien cho chung ca.
//
// UICanvas gom 2 loai WIDGET don gian - Text va Bar (thanh tien trinh, dung cho HP
// boss/volume...) - o CHE DO IMMEDIATE: moi Draw*() cua RenderSystem tu xay 1 canvas
// rieng cho man hinh no dang ve (goi Text()/Bar() de "khai bao" widget), roi goi
// canvas.Draw(font) DUY NHAT 1 LAN o cuoi de thuc su phat lenh ve cho GPU. Nho vay:
//   - RenderSystem khong con phai nho tu goi DrawTextEx voi dung font/spacing moi lan;
//     Canvas la noi DUY NHAT biet cach 1 dong chu/1 thanh tien trinh duoc ve nhu the nao.
//   - Muon doi kieu hien thi toan cuc (vd doi font, them do bong, doi mau vien Bar mac
//     dinh) chi can sua trong UICanvas::Draw(), khong phai lung soan tung Draw*() rieng le.
// ==========================================
struct UIText {
    Vector2 pos;
    std::string text;
    int fontSize;
    Color color;
    // A1: khi true, pos.x dang la TOA DO TAM (center X) mong muon, KHONG phai canh
    // trai. Canh trai thuc su chi tinh duoc trong Draw() (can do rong that qua
    // MeasureTextEx VOI DUNG font se dung de ve - UICanvas khong giu font rieng nen
    // khong the tinh o CenteredText()).
    bool centered = false;
};

struct UIBar {
    Rectangle rect;
    float ratio; // 0..1 - ty le lap day, tu dong clamp trong Draw()
    Color bgColor;
    Color fillColor;
    Color borderColor;
    // GD 5: phan "vua mat" (giua ratio va trail) ve mau trailColor; ticks = vach doc tai ty le
    // do (nguong giai doan boss). trail < 0 / tick < 0 = khong ve.
    float trail = -1.0f;
    Color trailColor{ 0, 0, 0, 0 };
    float ticks[2] = { -1.0f, -1.0f };
};

// PANEL/ICON (Nguoi 3 - Audio & UI): 2 widget THEM MOI, dung
// CHUNG khuon Text/Bar o tren (khai bao qua Panel()/Icon() -> Draw() 1 lan), khong sua
// UIText/UIBar/API cu nao - hoan toan cong them, an toan tuyet doi voi moi noi dang goi
// canvas.Text()/Bar() san co.
struct UIPanel {
    Rectangle rect;
    Color fillColor;    // Thuong dat alpha < 255 (vd Config::HUD_PANEL_ALPHA) - van thay duoc gameplay phia sau
    Color borderColor;
    float borderThickness; // 0 = khong ve vien
    // GD 5: goc ngoac kieu HUD vector - 4 chu L ngan o 4 goc, mau rieng (alpha 0 = khong ve).
    Color cornerColor{ 0, 0, 0, 0 };
};

// texture la BAN SAO handle (Texture2D chi la id/width/height/mipmaps/format - vai
// chuc byte, KHONG so huu GPU resource that su), khong phai con tro - giong het cach
// UIText/UIBar da luu Color theo gia tri. SpriteSheet (sprites.h) van la noi SO HUU va
// giai phong texture that su; canvas chi "muon" handle nay trong 1 frame de ve.
struct UIIcon {
    Rectangle destRect;
    Texture2D texture;
    Color tint;
};

class UICanvas {
private:
    std::vector<UIText> texts;
    std::vector<UIBar> bars;
    std::vector<UIPanel> panels;
    std::vector<UIIcon> icons;

public:
    // Chu vien ban dau du cho ca man hinh phuc tap nhat (Menu co toi da
    // Config::LEADERBOARD_MAX_ENTRIES dong bang xep hang) - tranh phai realloc/grow
    // nhieu lan trong 1 frame.
    UICanvas() {
        texts.reserve(24);
        bars.reserve(4);
        panels.reserve(4); // HUD hien tai can vai panel (score/status/power-up) - it hon Bar, khong can nhieu
        icons.reserve(8);  // Toi da vai icon power-up + phong hoi sau nay, du du khong phai grow giua frame
    }

    void Text(int x, int y, int fontSize, Color color, const std::string& text) {
        texts.push_back({ { (float)x, (float)y }, text, fontSize, color, false });
    }

    // A1: giong Text() nhung `centerX` la TAM ngang mong muon cua dong chu (vd
    // Config::SCREEN_W/2 de can giua man hinh), khong phai canh trai - giai quyet
    // dung 1 lop bug "can le tay bang mat" (vd Bug 1: LOADOUT/DIFFICULTY lech nhau vi
    // 2 dong dung x hardcode khac nhau, khong dong bo voi do rong chu thuc te).
    void CenteredText(int centerX, int y, int fontSize, Color color, const std::string& text) {
        texts.push_back({ { (float)centerX, (float)y }, text, fontSize, color, true });
    }

    // Nhan san 1 chuoi da TextFormat() - giu API goi tuong tu DrawText cu, tranh phai
    // viet lai tung noi goi khi chuyen tu DrawGameText() sang canvas.Text().
    void Bar(Rectangle rect, float ratio, Color bgColor, Color fillColor, Color borderColor) {
        bars.push_back({ rect, ratio, bgColor, fillColor, borderColor });
    }

    // Nen co vien cho 1 khu vuc HUD (vd sau nhom SCORE/WAVE, sau hang icon power-up) -
    // ve TRUOC moi Bar/Icon/Text khac trong Draw() nen luon nam "duoi cung".
    void Panel(Rectangle rect, Color fillColor, Color borderColor, float borderThickness = 1.0f) {
        panels.push_back({ rect, fillColor, borderColor, borderThickness });
    }

    // Panel HUD kieu vector (GD 5): nhu Panel() + goc ngoac sang o 4 goc.
    void FramedPanel(Rectangle rect, Color fillColor, Color borderColor, float borderThickness, Color cornerColor) {
        UIPanel p{ rect, fillColor, borderColor, borderThickness };
        p.cornerColor = cornerColor;
        panels.push_back(p);
    }

    // Thanh mau co vet sat thuong + vach nguong (thanh mau boss, GD 5).
    void TrailBar(Rectangle rect, float ratio, float trail, Color bgColor, Color fillColor, Color trailColor,
                  Color borderColor, float tick1, float tick2) {
        UIBar b{ rect, ratio, bgColor, fillColor, borderColor };
        b.trail = trail;
        b.trailColor = trailColor;
        b.ticks[0] = tick1;
        b.ticks[1] = tick2;
        bars.push_back(b);
    }

    // Badge icon nho (vd trang thai power-up) - texture thuong lay tu SpriteSheet (vd
    // gm.sprites.iconShield), keo/dan gon vao destRect bang DrawTexturePro ben trong
    // Draw(), khong phu thuoc kich thuoc goc cua texture.
    void Icon(Rectangle destRect, Texture2D texture, Color tint = WHITE) {
        icons.push_back({ destRect, texture, tint });
    }

    // Phat toan bo lenh ve GPU cho moi widget da khai bao trong frame nay - goi DUY
    // NHAT 1 LAN o cuoi moi Draw*() cua RenderSystem.
    void Draw(const Font& font) const {
        for (const UIPanel& p : panels) {
            DrawRectangleRec(p.rect, p.fillColor);
            if (p.borderThickness > 0.0f) DrawRectangleLinesEx(p.rect, p.borderThickness, p.borderColor);
            if (p.cornerColor.a > 0) {
                const float L = 7.0f, T = 2.0f;
                const Rectangle r = p.rect;
                // Moi goc = 1 vach ngang + 1 vach doc, chom ra ngoai vien 1px de "bam" lay khung
                DrawRectangleRec({ r.x - 1.0f, r.y - 1.0f, L, T }, p.cornerColor);
                DrawRectangleRec({ r.x - 1.0f, r.y - 1.0f, T, L }, p.cornerColor);
                DrawRectangleRec({ r.x + r.width + 1.0f - L, r.y - 1.0f, L, T }, p.cornerColor);
                DrawRectangleRec({ r.x + r.width - 1.0f, r.y - 1.0f, T, L }, p.cornerColor);
                DrawRectangleRec({ r.x - 1.0f, r.y + r.height - 1.0f, L, T }, p.cornerColor);
                DrawRectangleRec({ r.x - 1.0f, r.y + r.height + 1.0f - L, T, L }, p.cornerColor);
                DrawRectangleRec({ r.x + r.width + 1.0f - L, r.y + r.height - 1.0f, L, T }, p.cornerColor);
                DrawRectangleRec({ r.x + r.width - 1.0f, r.y + r.height + 1.0f - L, T, L }, p.cornerColor);
            }
        }
        for (const UIBar& b : bars) {
            float ratio = b.ratio;
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;

            DrawRectangleRec(b.rect, b.bgColor);
            Rectangle fillRect = b.rect;
            fillRect.width *= ratio;
            if (b.trail > ratio) {
                float trail = b.trail > 1.0f ? 1.0f : b.trail;
                DrawRectangleRec({ b.rect.x + b.rect.width * ratio, b.rect.y, b.rect.width * (trail - ratio), b.rect.height }, b.trailColor);
            }
            DrawRectangleRec(fillRect, b.fillColor);
            for (float tk : b.ticks) {
                if (tk <= 0.0f || tk >= 1.0f) continue;
                float x = b.rect.x + b.rect.width * tk;
                DrawRectangleRec({ x - 1.0f, b.rect.y - 2.0f, 2.0f, b.rect.height + 4.0f }, b.borderColor);
            }
            DrawRectangleLinesEx(b.rect, 1.0f, b.borderColor);
        }
        for (const UIIcon& ic : icons) {
            Rectangle srcRect = { 0.0f, 0.0f, (float)ic.texture.width, (float)ic.texture.height };
            DrawTexturePro(ic.texture, srcRect, ic.destRect, { 0.0f, 0.0f }, 0.0f, ic.tint);
        }
        for (const UIText& t : texts) {
            Vector2 pos = t.pos;
            if (t.centered) {
                // MeasureTextEx CAN dung font/spacing se dung de ve (khong phai uoc
                // luong tho theo fontSize*0.5 hay gi tuong tu) - moi ky tu trong font
                // Texture Atlas nay khong co do rong bang nhau.
                Vector2 size = MeasureTextEx(font, t.text.c_str(), (float)t.fontSize, 1.0f);
                pos.x -= size.x / 2.0f;
            }
            DrawTextEx(font, t.text.c_str(), pos, (float)t.fontSize, 1.0f, t.color);
        }
    }
};
