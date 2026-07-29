#pragma once
#include "raylib.h"
#include <cstddef>

class Bullet {
private:
    Rectangle rect;
    float speed;
    bool active;

public:
    Bullet() : rect{0, 0, 5.0f, 15.0f}, speed(0), active(false) {}

    void Spawn(float x, float y, float spd) {
        rect.x = x;
        rect.y = y;
        speed = spd;
        active = true;
    }

    void Update(float dt) {
        rect.y -= speed * dt;
        if (rect.y < -20.0f || rect.y > 620.0f) active = false;
    }

    void Draw(Color color) const { DrawRectangleRec(rect, color); }

    bool IsActive() const { return active; }
    Rectangle GetRect() const { return rect; }
};

// Pool cấp phát 1 lần trên stack, dùng thuật toán swap-and-pop để spawn/destroy O(1)
// mà không cần dịch chuyển toàn bộ mảng. Đạn sống luôn nằm liền khối ở đầu mảng.
template <size_t MAX_BULLETS>
class BulletPool {
private:
    Bullet pool[MAX_BULLETS];
    size_t activeCount = 0;

public:
    void Reset() { activeCount = 0; }

    void Fire(float x, float y, float speed) {
        if (activeCount >= MAX_BULLETS) return; // Hết chỗ -> bỏ qua, không tràn bộ nhớ
        pool[activeCount].Spawn(x, y, speed);
        activeCount++;
    }

    void Destroy(size_t index) {
        if (index >= activeCount) return;
        activeCount--;
        Bullet temp = pool[index];
        pool[index] = pool[activeCount];
        pool[activeCount] = temp;
    }

    void Update(float dt) {
        for (size_t i = 0; i < activeCount; ) {
            pool[i].Update(dt);
            if (!pool[i].IsActive()) Destroy(i); // Không tăng i vì Destroy vừa swap phần tử khác vào đây
            else i++;
        }
    }

    void Draw(Color color) const {
        for (size_t i = 0; i < activeCount; i++) pool[i].Draw(color);
    }

    size_t GetActiveCount() const { return activeCount; }
    Bullet& GetBullet(size_t index) { return pool[index]; }
};
