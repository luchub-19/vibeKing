#pragma once
#include "raylib.h"
#include "config.h"
#include <cstddef>
#include <cstdlib>
#include <cmath>

namespace ParticleMath {
    constexpr float PI_F = 3.14159265358979323846f;
}

class Particle {
private:
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    Color color;
    bool active = false;

public:
    void Spawn(Vector2 p, Vector2 v, float lifeTime, Color c) {
        pos = p; vel = v; life = lifeTime; maxLife = lifeTime; color = c; active = true;
    }

    void Update(float dt) {
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        vel.y += Config::PARTICLE_GRAVITY * dt; // Trọng lực nhẹ để mảnh vỡ rơi tự nhiên thay vì bay thẳng
        life -= dt;
        if (life <= 0.0f) active = false;
    }

    void Draw() const {
        float alpha = life / maxLife;
        if (alpha < 0.0f) alpha = 0.0f;
        Color c = color;
        c.a = (unsigned char)(255 * alpha);
        DrawRectangle((int)pos.x, (int)pos.y, 3, 3, c);
    }

    bool IsActive() const { return active; }
};

// Cùng thuật toán swap-and-pop với BulletPool — dùng lại pattern đã kiểm chứng
// thay vì tự sáng tạo cách quản lý mới, giữ codebase nhất quán và dễ maintain.
template <size_t MAX_PARTICLES>
class ParticlePool {
private:
    Particle pool[MAX_PARTICLES];
    size_t activeCount = 0;

public:
    void Reset() { activeCount = 0; }

    void Spawn(Vector2 pos, Vector2 vel, float life, Color color) {
        if (activeCount >= MAX_PARTICLES) return;
        pool[activeCount].Spawn(pos, vel, life, color);
        activeCount++;
    }

    // Nổ 1 cụm hạt bắn tứ phía tại vị trí cho trước — dùng khi enemy/player bị phá hủy
    void Burst(Vector2 origin, int count, Color color) {
        for (int i = 0; i < count; i++) {
            float angle = (float)GetRandomValue(0, 359) * (ParticleMath::PI_F / 180.0f);
            float speed = (float)GetRandomValue(60, 220);
            Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
            float lifeTime = (float)GetRandomValue(3, 6) / 10.0f;
            Spawn(origin, vel, lifeTime, color);
        }
    }

    void Destroy(size_t index) {
        if (index >= activeCount) return;
        activeCount--;
        Particle temp = pool[index];
        pool[index] = pool[activeCount];
        pool[activeCount] = temp;
    }

    void Update(float dt) {
        for (size_t i = 0; i < activeCount; ) {
            pool[i].Update(dt);
            if (!pool[i].IsActive()) Destroy(i);
            else i++;
        }
    }

    void Draw() const {
        for (size_t i = 0; i < activeCount; i++) pool[i].Draw();
    }

    size_t GetActiveCount() const { return activeCount; }
};
