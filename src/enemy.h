#pragma once
#include "raylib.h"

class Enemy {
private:
    Rectangle rect;
    Color color;
    bool active;

public:
    Enemy(float x, float y, Color col) : rect{x, y, 40.0f, 25.0f}, color(col), active(true) {}

    void MoveX(float dx) { rect.x += dx; }
    void MoveY(float dy) { rect.y += dy; }
    void ForceX(float x) { rect.x = x; }

    void Draw() const { if (active) DrawRectangleRec(rect, color); }

    bool IsActive() const { return active; }
    void Destroy() { active = false; }
    Rectangle GetRect() const { return rect; }
    Color GetColor() const { return color; }
    float GetX() const { return rect.x; }
    float GetWidth() const { return rect.width; }
    float GetBottom() const { return rect.y + rect.height; }
    float GetCenterX() const { return rect.x + rect.width / 2; }
    float GetBottomY() const { return rect.y + 20.0f; }
    Vector2 GetCenter() const { return { rect.x + rect.width / 2, rect.y + rect.height / 2 }; }
};
