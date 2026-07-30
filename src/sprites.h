#pragma once
#include "raylib.h"

// ==========================================
// SPRITE SHEET - texture sinh bằng Image thao tác trong RAM lúc Load(), không cần file
// .png rời (giữ đúng triết lý "không phụ thuộc asset ngoài" mà AudioManager đã áp dụng
// cho âm thanh procedural). Hình dạng là hình học nguyên bản đơn giản (không sao chép
// pixel art của bất kỳ game nào) - chỉ đủ để phân biệt các loại thực thể bằng mắt thay
// vì toàn hình chữ nhật trơn.
//
// Phải gọi Load() SAU InitWindow() (texture cần GPU context), và Unload() TRƯỚC
// CloseWindow() - xem GameManager::Run().
// ==========================================
class SpriteSheet {
public:
    Texture2D player{};
    Texture2D basicAlien{};
    Texture2D tankyAlien{};
    Texture2D zigzagAlien{};
    Texture2D ufo{};
    Texture2D kamikaze{};
    Texture2D boss{};

    void Load();
    void Unload();
};

// Vẽ texture (thường 16x16) giãn vừa khít 1 Rectangle bất kỳ, nhuộm màu qua tint -
// dùng để không phải nhân bản logic DrawTexturePro ở từng chỗ gọi trong game_manager.cpp.
inline void DrawSprite(const Texture2D& tex, Rectangle dest, Color tint) {
    Rectangle src{ 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    DrawTexturePro(tex, src, dest, { 0.0f, 0.0f }, 0.0f, tint);
}
