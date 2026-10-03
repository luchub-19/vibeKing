#pragma once
#include "raylib.h"
#include "config.h"
#include "culling.h"
#include "palette.h"
#include <cstdint>
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
//
// GD 2 (docs/GRAPHICS_UPGRADE_PLAN.md) them 3 lop cho vu no theo mo hinh "5 qua trinh tach
// duoc" (nguon ZH - indienova/CSDN): Glow = loi flash/cau lua (ve ADDITIVE, co lai dan),
// Ring = song xung kich (vong mo rong ease-out, ADDITIVE), Debris = manh vo xoay, cham dan
// do luc can, ton tai lau hon tia lua. Tia lua van la Spark/Square cu cua Burst().
enum class ParticleShape : unsigned char { Square, Spark, Glow, Ring, Debris };

// Co no - GameEvent::explosion (events.h) khai bao, ProcessEvents() goi Explosion().
enum class ExplosionSize : uint8_t { None, Small, Large };

class Particle {
private:
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    Color color;
    ParticleShape shape = ParticleShape::Square;
    float size = 3.0f;
    float endSize = 3.0f; // Glow/Ring: kich thuoc luc het life (Glow co lai, Ring no ra)
    float rot = 0.0f;     // Debris: goc hien tai (do)
    float spin = 0.0f;    // Debris: do/giay
    bool active = false;

public:
    // shape/size co gia tri mac dinh (Square, 3px - giong het hanh vi CU) de KHONG pha
    // bat ky noi goi Spawn() truc tiep nao khac ngoai Burst() (an toan nguoc, khong can
    // sua call site nao khac dang dung 4 tham so cu).
    void Spawn(Vector2 p, Vector2 v, float lifeTime, Color c, ParticleShape sh = ParticleShape::Square, float sz = 3.0f) {
        pos = p; vel = v; life = lifeTime; maxLife = lifeTime; color = c; shape = sh; size = sz;
        endSize = sz; rot = 0.0f; spin = 0.0f; active = true;
    }

    // Glow/Ring: kich thuoc doi tu sz -> szEnd theo ease-out. Debris: spinDeg/giay.
    void SpawnShaped(Vector2 p, Vector2 v, float lifeTime, Color c, ParticleShape sh, float sz, float szEnd, float spinDeg) {
        Spawn(p, v, lifeTime, c, sh, sz);
        endSize = szEnd;
        spin = spinDeg;
    }

    // Ring/Glow la anh sang -> cong don (ADDITIVE); con lai la vat the -> alpha thuong.
    bool IsAdditive() const { return shape == ParticleShape::Ring || shape == ParticleShape::Glow; }

    void Update(float dt) {
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        // Glow/Ring dung yen tai tam vu no - trong luc ma keo xuong se tach vong song khoi
        // dung cho vua no.
        if (!IsAdditive()) vel.y += Config::PARTICLE_GRAVITY * dt; // Trọng lực nhẹ để mảnh vỡ rơi tự nhiên thay vì bay thẳng
        if (shape == ParticleShape::Debris) {
            // Luc can: manh vo bay nhanh luc dau roi cham han - "nhanh ra, cham tan" (nguon ZH)
            float damp = 1.0f / (1.0f + 2.5f * dt);
            vel.x *= damp;
            vel.y *= damp;
            rot += spin * dt;
        }
        life -= dt;
        if (life <= 0.0f) active = false;
    }

    void Draw() const {
        // CULLING: trong luc roi (Config::PARTICLE_GRAVITY) mot so hat co the bi day ra
        // ngoai man hinh truoc khi het "life" - bo qua lenh ve GPU cho chung.
        float extent = fmaxf(size, endSize);
        if (!Culling::IsVisible({ pos.x - extent, pos.y - extent, extent * 2.0f, extent * 2.0f })) return;

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
        } else if (shape == ParticleShape::Glow || shape == ParticleShape::Ring) {
            // t: 0 luc sinh -> 1 luc chet. Ease-out bac 3: no RAT nhanh roi cham han - mat doc
            // duoc "co 1 cu no" ngay frame dau, phan duoi keo dai cho dep (nguon ZH: cham vao-
            // nhanh ra cho no; thoi gian tan > thoi gian bung).
            float t = 1.0f - alpha;
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
            float r = size + (endSize - size) * ease;
            if (shape == ParticleShape::Glow) {
                // 2 dia dong tam thay gradient that (DrawCircleGradient doi chu ky 5.5 -> 5.6)
                // Quang ngoai alpha 0.12: ban dau 0.35 -> anh chup vu no Large ra 1 "dong xu xam"
                // phang ban kinh ~30px (trang-nong cong don o 35% tren nen toi = xam dac), che
                // mat vong song. Quang rong de bloom lo; o day chi can tam sang + mep mem.
                Color outer = c;
                outer.a = (unsigned char)(c.a * 0.12f);
                DrawCircleV(pos, r, outer);
                Color mid = c;
                mid.a = (unsigned char)(c.a * 0.35f);
                DrawCircleV(pos, r * 0.6f, mid);
                DrawCircleV(pos, r * 0.35f, c);
            } else {
                float thickness = fmaxf(1.0f, 3.5f * alpha);
                DrawRing(pos, fmaxf(0.0f, r - thickness), r, 0.0f, 360.0f, 36, c);
            }
        } else if (shape == ParticleShape::Debris) {
            Rectangle rr{ pos.x, pos.y, size, size * 0.6f };
            DrawRectanglePro(rr, { size / 2.0f, size * 0.3f }, rot, c);
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
    float spawnScale = 1.0f;

public:
    void Reset() { activeCount = 0; }

    // He so preset do hoa (GraphicsSettings::ParticleScale) - ap TAI DAY, 1 diem duy nhat,
    // thay vi o tung noi goi Burst() (ProcessEvents, UpdatePlaying, PhysicsSystem...): them
    // noi goi moi sau nay tu dong theo preset, khong the quen.
    void SetSpawnScale(float s) { spawnScale = s; }

    // Lam tron gan nhat nhung KHONG ve 0 khi count > 0: muzzle flash 3 hat x 0.5 van phai con
    // 1 hat - preset Low giam do day, khong duoc xoa mat phan hoi "vua ban/vua trung".
    static int ScaledCount(int count, float scale) {
        if (count <= 0) return 0;
        int n = (int)((float)count * scale + 0.5f);
        return n < 1 ? 1 : n;
    }

    void Spawn(Vector2 pos, Vector2 vel, float life, Color color, ParticleShape shape = ParticleShape::Square, float size = 3.0f) {
        if (activeCount >= MAX_PARTICLES) return;
        pool[activeCount].Spawn(pos, vel, life, color, shape, size);
        activeCount++;
    }

    void SpawnShaped(Vector2 pos, Vector2 vel, float life, Color color, ParticleShape shape, float size, float endSize, float spinDeg) {
        if (activeCount >= MAX_PARTICLES) return;
        pool[activeCount].SpawnShaped(pos, vel, life, color, shape, size, endSize, spinDeg);
        activeCount++;
    }

    // Nổ 1 cụm hạt bắn tứ phía tại vị trí cho trước — dùng khi enemy/player bị phá hủy.
    // ~1/3 là Spark (tia kéo dài), còn lại Square kích thước ngẫu nhiên 2-4px - trộn
    // trong 1 cụm để đa dạng thị giác thay vì toàn hạt giống hệt nhau từng pixel.
    void Burst(Vector2 origin, int count, Color color) {
        count = ScaledCount(count, spawnScale);
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

    // VU NO NHIEU LOP (GD 2). Lop nao cung la 1 cau tra loi cho 1 cau hoi khac cua mat:
    //   Glow  (1)   - "CO no" - flash trang-nong 0.1s, la thu DUY NHAT trong vu no dung dai NONG
    //                 (luat R3: nong ngan = su kien, khong phai mau cua dich)
    //   Ring  (1-2) - "no O DAU, to CO NAO" - vong mau sang hon mau dich, mo rong ease-out
    //   Debris(N)   - "con lai gi" - manh mau dich (dai LANH), xoay, roi, tan cham nhat
    // Tia lua (Burst) do event tu sinh theo particleCount nhu truoc - khong lap lai o day.
    // Glow/Ring KHONG theo preset (moi vu no chi 2-3 hat, va la phan hoi chinh) - Debris thi co.
    void Explosion(Vector2 origin, Color color, ExplosionSize sz) {
        if (sz == ExplosionSize::None) return;
        const bool large = (sz == ExplosionSize::Large);
        const Color ringColor = Palette::Lerp(color, WHITE, 0.45f);

        SpawnShaped(origin, { 0.0f, 0.0f }, large ? 0.16f : 0.1f, Palette::ExplosionCore,
                    ParticleShape::Glow, large ? 34.0f : 15.0f, large ? 8.0f : 4.0f, 0.0f);
        SpawnShaped(origin, { 0.0f, 0.0f }, large ? 0.5f : 0.3f, ringColor,
                    ParticleShape::Ring, 4.0f, large ? 90.0f : 30.0f, 0.0f);
        if (large) {
            // Vong thu 2 cham hon/nho hon - 2 song lech nhip doc ra "vu no lon" thay vi "vu no thuong phong to"
            SpawnShaped(origin, { 0.0f, 0.0f }, 0.38f, color, ParticleShape::Ring, 2.0f, 55.0f, 0.0f);
        }

        int debris = ScaledCount(large ? 14 : 5, spawnScale);
        for (int i = 0; i < debris; i++) {
            float angle = (float)GetRandomValue(0, 359) * (ParticleMath::PI_F / 180.0f);
            float speed = (float)GetRandomValue(large ? 70 : 40, large ? 200 : 130);
            Vector2 v = { cosf(angle) * speed, sinf(angle) * speed };
            float lifeTime = (float)GetRandomValue(large ? 7 : 5, large ? 12 : 8) / 10.0f;
            float sizePx = (float)GetRandomValue(3, large ? 7 : 5);
            float spinDeg = (float)GetRandomValue(-540, 540);
            SpawnShaped(origin, v, lifeTime, color, ParticleShape::Debris, sizePx, sizePx, spinDeg);
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

    // 2 luot: vat the (alpha thuong) roi anh sang (1 khoi ADDITIVE duy nhat) - cung ly do voi
    // BulletPool::DrawGlows: doi blend mode theo tung hat = xa batch theo tung hat.
    void Draw() const {
        bool anyAdditive = false;
        for (size_t i = 0; i < activeCount; i++) {
            if (pool[i].IsAdditive()) anyAdditive = true;
            else pool[i].Draw();
        }
        if (!anyAdditive) return;
        BeginBlendMode(BLEND_ADDITIVE);
        for (size_t i = 0; i < activeCount; i++) {
            if (pool[i].IsAdditive()) pool[i].Draw();
        }
        EndBlendMode();
    }

    size_t GetActiveCount() const { return activeCount; }
};
