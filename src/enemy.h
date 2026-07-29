#pragma once
#include "raylib.h"

// ==========================================
// ENEMY - LỚP CƠ SỞ ĐA HÌNH
// Trước đây Enemy là 1 class cụ thể duy nhất, không có chỗ cho "địch máu dày" hay
// "địch bay zig-zag" mà không rải if/else khắp GameManager. Giờ Enemy là abstract
// interface: hành vi riêng (điểm số, số máu, cách di chuyển thêm ngoài đội hình,
// cách vẽ) nằm trong từng subclass override - GameManager chỉ thao tác qua con trỏ
// Enemy* và gọi virtual method, không cần biết đang cầm loại địch cụ thể nào.
// ==========================================
class Enemy {
protected:
    Rectangle rect;
    Color color;
    bool active = true;
    int column = 0;   // Cột trong đội hình lúc spawn - dùng cho AI "line of sight"
    int hp;
    int maxHp;

public:
    Enemy(float x, float y, float w, float h, Color col, int columnIndex, int hitPoints)
        : rect{x, y, w, h}, color(col), column(columnIndex), hp(hitPoints), maxHp(hitPoints) {}

    virtual ~Enemy() = default;

    // Hook cho hành vi di chuyển riêng của từng loại địch (vd zig-zag), được gọi mỗi
    // frame TRƯỚC đội hình di chuyển đồng loạt (MoveX/MoveY do GameManager điều khiển).
    // Mặc định rỗng - địch thường (BasicEnemy) không có hành vi phụ nào.
    virtual void Update(float dt) { (void)dt; }

    virtual void Draw() const {
        if (!active) return;
        DrawRectangleRec(rect, color);
        if (hp < maxHp) {
            // Địch máu dày bị thương -> viền sáng để người chơi thấy rõ đã gây sát thương
            DrawRectangleLinesEx(rect, 2.0f, WHITE);
        }
    }

    // Điểm số khi hạ gục loại địch này - máu càng dày càng đáng giá
    virtual int GetScoreValue() const = 0;

    void MoveX(float dx) { rect.x += dx; }
    void MoveY(float dy) { rect.y += dy; }
    void ForceX(float x) { rect.x = x; }

    // Trả về true nếu đòn đánh này hạ gục địch (hp về 0) - false nếu địch máu dày
    // vẫn còn sống sau khi trúng đạn.
    bool TakeHit() {
        if (hp > 0) hp--;
        if (hp <= 0) {
            active = false;
            return true;
        }
        return false;
    }

    bool IsActive() const { return active; }
    void Destroy() { active = false; }
    int GetColumn() const { return column; }
    int GetHp() const { return hp; }
    int GetMaxHp() const { return maxHp; }
    Rectangle GetRect() const { return rect; }
    Color GetColor() const { return color; }
    float GetX() const { return rect.x; }
    float GetWidth() const { return rect.width; }
    float GetBottom() const { return rect.y + rect.height; }
    float GetCenterX() const { return rect.x + rect.width / 2; }
    float GetBottomY() const { return rect.y + rect.height - 5.0f; }
    Vector2 GetCenter() const { return { rect.x + rect.width / 2, rect.y + rect.height / 2 }; }
};
