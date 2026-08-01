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
};

struct UIBar {
    Rectangle rect;
    float ratio; // 0..1 - ty le lap day, tu dong clamp trong Draw()
    Color bgColor;
    Color fillColor;
    Color borderColor;
};

class UICanvas {
private:
    std::vector<UIText> texts;
    std::vector<UIBar> bars;

public:
    // Chu vien ban dau du cho ca man hinh phuc tap nhat (Menu co toi da
    // Config::LEADERBOARD_MAX_ENTRIES dong bang xep hang) - tranh phai realloc/grow
    // nhieu lan trong 1 frame.
    UICanvas() {
        texts.reserve(24);
        bars.reserve(4);
    }

    void Text(int x, int y, int fontSize, Color color, const std::string& text) {
        texts.push_back({ { (float)x, (float)y }, text, fontSize, color });
    }

    // Nhan san 1 chuoi da TextFormat() - giu API goi tuong tu DrawText cu, tranh phai
    // viet lai tung noi goi khi chuyen tu DrawGameText() sang canvas.Text().
    void Bar(Rectangle rect, float ratio, Color bgColor, Color fillColor, Color borderColor) {
        bars.push_back({ rect, ratio, bgColor, fillColor, borderColor });
    }

    // Phat toan bo lenh ve GPU cho moi widget da khai bao trong frame nay - goi DUY
    // NHAT 1 LAN o cuoi moi Draw*() cua RenderSystem.
    void Draw(const Font& font) const {
        for (const UIBar& b : bars) {
            float ratio = b.ratio;
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;

            DrawRectangleRec(b.rect, b.bgColor);
            Rectangle fillRect = b.rect;
            fillRect.width *= ratio;
            DrawRectangleRec(fillRect, b.fillColor);
            DrawRectangleLinesEx(b.rect, 1.0f, b.borderColor);
        }
        for (const UIText& t : texts) {
            DrawTextEx(font, t.text.c_str(), t.pos, (float)t.fontSize, 1.0f, t.color);
        }
    }
};
