#pragma once
#include "enemy.h"
#include <cmath>

// Địch thường - 1 máu, không có hành vi phụ. Chiếm đa số đội hình.
class BasicEnemy : public Enemy {
public:
    BasicEnemy(float x, float y, Color col, int column)
        : Enemy(x, y, 40.0f, 25.0f, col, column, 1) {}

    int GetScoreValue() const override { return 10; }
};

// Địch máu dày - chịu nhiều đòn hơn, kích thước lớn hơn 1 chút để dễ nhận diện,
// và cho nhiều điểm hơn khi hạ gục.
class TankyEnemy : public Enemy {
public:
    static constexpr int HP = 3;

    TankyEnemy(float x, float y, Color col, int column)
        : Enemy(x, y, 44.0f, 30.0f, col, column, HP) {}

    int GetScoreValue() const override { return 30; }
};

// Địch bay zig-zag - dao động ngang quanh vị trí đội hình bằng sóng sin, cộng dồn
// delta (không phải gán tuyệt đối) để không phá vỡ logic di chuyển đội hình của
// GameManager (MoveX/MoveY vẫn áp dụng bình thường lên rect.x/y).
class ZigzagEnemy : public Enemy {
private:
    float timer = 0.0f;
    float lastOffset = 0.0f;
    static constexpr float FREQUENCY = 5.0f;   // rad/s
    static constexpr float AMPLITUDE = 18.0f;  // px

public:
    ZigzagEnemy(float x, float y, Color col, int column)
        : Enemy(x, y, 36.0f, 22.0f, col, column, 1) {}

    void Update(float dt) override {
        timer += dt;
        float newOffset = sinf(timer * FREQUENCY) * AMPLITUDE;
        rect.x += (newOffset - lastOffset); // Chỉ cộng phần thay đổi -> không trôi dạt tích lũy
        lastOffset = newOffset;
    }

    int GetScoreValue() const override { return 20; }
};
