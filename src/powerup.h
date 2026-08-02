#pragma once
#include "raylib.h"
#include <cstddef>
#include <cstdint>

// ==========================================
// POWER-UP
// Rơi thẳng xuống từ vị trí địch vừa chết, người chơi bay ngang qua để nhặt. 4 loại
// đều nhau (xem PhysicsSystem::UpdatePowerUps): RapidFire (bắn nhanh tạm thời), Shield
// (đỡ đúng 1 đòn), Piercing (đạn xuyên nhiều địch) và Cleanser (xoá sạch đạn địch đang
// bay trên màn hình ngay lập tức - "bom cứu nạn").
// Dùng lại đúng thuật toán swap-and-pop với các pool khác trong project (BulletPool,
// ParticlePool, EnemyPool) - không phát minh thêm cách quản lý mới.
// ==========================================
enum class PowerUpType : uint8_t { RapidFire, Shield, Piercing, Cleanser };

struct PowerUp {
    Rectangle rect;
    PowerUpType type;
};

template <size_t Capacity>
class PowerUpPool {
private:
    PowerUp items[Capacity];
    size_t count = 0;

public:
    void Reset() { count = 0; }

    bool Spawn(const PowerUp& p) {
        if (count >= Capacity) return false; // Hết chỗ -> bỏ qua, không tràn mảng
        items[count++] = p;
        return true;
    }

    void Destroy(size_t index) {
        if (index >= count) return;
        count--;
        items[index] = items[count];
    }

    size_t Size() const { return count; }
    PowerUp& operator[](size_t index) { return items[index]; }
    const PowerUp& operator[](size_t index) const { return items[index]; }

    // Rơi thẳng xuống đều; văng khỏi pool khi lọt quá đáy màn hình (không nhặt kịp).
    void Update(float dt, float fallSpeed, float screenH) {
        for (size_t i = 0; i < count; ) {
            items[i].rect.y += fallSpeed * dt;
            if (items[i].rect.y > screenH) Destroy(i);
            else i++;
        }
    }

    void Draw() const {
        for (size_t i = 0; i < count; i++) {
            Color c;
            switch (items[i].type) {
                case PowerUpType::RapidFire: c = ORANGE;  break;
                case PowerUpType::Shield:    c = SKYBLUE; break;
                case PowerUpType::Piercing:  c = MAGENTA; break;
                case PowerUpType::Cleanser:  c = LIME;    break;
                default: c = WHITE; break;
            }
            DrawRectangleRec(items[i].rect, c);
            DrawRectangleLinesEx(items[i].rect, 1.5f, WHITE);
        }
    }
};
