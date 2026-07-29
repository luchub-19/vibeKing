#pragma once
#include "raylib.h"

// Hiệu ứng rung màn hình: cộng offset ngẫu nhiên giảm dần theo thời gian.
// Không phụ thuộc gì vào GameManager/Player -> tái sử dụng được ở bất kỳ game nào khác.
class ScreenShake {
private:
    float timer = 0.0f;
    float duration = 0.0f;
    float magnitude = 0.0f;

public:
    // Rung mạnh hơn sẽ ghi đè rung yếu đang chạy, tránh 1 va chạm nhỏ làm mất hiệu ứng lớn
    void Trigger(float durationSec, float mag) {
        if (mag >= magnitude || timer <= 0.0f) {
            duration = durationSec;
            timer = durationSec;
            magnitude = mag;
        }
    }

    void Update(float dt) {
        if (timer > 0.0f) timer -= dt;
    }

    Vector2 GetOffset() const {
        if (timer <= 0.0f) return { 0.0f, 0.0f };
        float ratio = timer / duration;
        float amount = magnitude * ratio;
        return {
            ((float)GetRandomValue(-100, 100) / 100.0f) * amount,
            ((float)GetRandomValue(-100, 100) / 100.0f) * amount
        };
    }
};
