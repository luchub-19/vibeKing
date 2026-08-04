#pragma once
#include "raylib.h"
#include "config.h"
#include "culling.h"
#include <cstddef>
#include <cstdlib>
#include <cmath>

namespace ParticleMath {
    constexpr float PI_F = 3.14159265358979323846f;
}

// Hinh dang hat - da dang hoa hieu ung no (truoc day CHI co 1 kieu vuong 3x3 co dinh,
// nhin "phang" du la dich thuong hay Boss). Square: manh vun vuong, kich thuoc NGAU
// NHIEN moi lan Spawn thay vi co dinh. Spark: 1 vach mong keo dai THEO HUONG BAY - mo
// phong tia lua toc do cao, khac han cam giac "manh vun roi" cua Square.
enum class ParticleShape : unsigned char { Square, Spark };

class Particle {
private:
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    Color color;
    ParticleShape shape = ParticleShape::Square;
    float size = 3.0f;
    bool active = false;

public:
    // shape/size co gia tri mac dinh (Square, 3px - giong het hanh vi CU) de KHONG pha
    // bat ky noi goi Spawn() truc tiep nao khac ngoai Burst() (an toan nguoc, khong can
    // sua call site nao khac dang dung 4 tham so cu).
    void Spawn(Vector2 p, Vector2 v, float lifeTime, Color c, ParticleShape sh = ParticleShape::Square, float sz = 3.0f) {
        pos = p; vel = v; life = lifeTime; maxLife = lifeTime; color = c; shape = sh; size = sz; active = true;
    }

    void Update(float dt) {
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        vel.y += Config::PARTICLE_GRAVITY * dt; // Trọng lực nhẹ để mảnh vỡ rơi tự nhiên thay vì bay thẳng
        life -= dt;
        if (life <= 0.0f) active = false;
    }

    void Draw() const {
        // CULLING: trong luc roi (Config::PARTICLE_GRAVITY) mot so hat co the bi day ra
        // ngoai man hinh truoc khi het "life" - bo qua lenh ve GPU cho chung.
        if (!Culling::IsVisible({ pos.x, pos.y, size, size })) return;

        float alpha = life / maxLife;
        if (alpha < 0.0f) alpha = 0.0f;
        Color c = color;
        c.a = (unsigned char)(255 * alpha);

        if (shape == ParticleShape::Spark) {
            // Vach mong keo dai NGUOC huong bay, dai ty le voi toc do hien tai - toc do
            // cao (vua no) keo vach dai/ro ret, cham dan lai theo thoi gian giong het
            // Square (dung chung life/maxLife/gravity o tren), khong can logic rieng nao.
            float speed = sqrtf(vel.x * vel.x + vel.y * vel.y);
            float len = size + fminf(speed * 0.03f, 12.0f);
            Vector2 dir = (speed > 1.0f) ? Vector2{ vel.x / speed, vel.y / speed } : Vector2{ 0.0f, 1.0f };
            Vector2 tail = { pos.x - dir.x * len, pos.y - dir.y * len };
            DrawLineEx(pos, tail, fmaxf(1.0f, size * 0.6f), c);
        } else {
            DrawRectangle((int)pos.x, (int)pos.y, (int)size, (int)size, c);
        }
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

    void Spawn(Vector2 pos, Vector2 vel, float life, Color color, ParticleShape shape = ParticleShape::Square, float size = 3.0f) {
        if (activeCount >= MAX_PARTICLES) return;
        pool[activeCount].Spawn(pos, vel, life, color, shape, size);
        activeCount++;
    }

    // Nổ 1 cụm hạt bắn tứ phía tại vị trí cho trước — dùng khi enemy/player bị phá hủy.
    // ~1/3 là Spark (tia kéo dài), còn lại Square kích thước ngẫu nhiên 2-4px - trộn
    // trong 1 cụm để đa dạng thị giác thay vì toàn hạt giống hệt nhau từng pixel.
    void Burst(Vector2 origin, int count, Color color) {
        for (int i = 0; i < count; i++) {
            float angle = (float)GetRandomValue(0, 359) * (ParticleMath::PI_F / 180.0f);
            float speed = (float)GetRandomValue(60, 220);
            Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
            float lifeTime = (float)GetRandomValue(3, 6) / 10.0f;
            ParticleShape shape = (GetRandomValue(0, 2) == 0) ? ParticleShape::Spark : ParticleShape::Square;
            float size = (float)GetRandomValue(2, 4);
            Spawn(origin, vel, lifeTime, color, shape, size);
        }
    }

    void Destroy(size_t index) {
        if (index >= activeCount) return;
        activeCount--;
        pool[index] = pool[activeCount];
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
