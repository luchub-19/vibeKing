#include "warp_grid.h"
#include "palette.h"
#include <cmath>

namespace {
    // He so lay tu bai Tuts+ (da kiem chung o tan so 60Hz, cung don vi "px/buoc"):
    constexpr float NEIGHBOR_STIFFNESS = 0.28f;
    constexpr float NEIGHBOR_DAMPING   = 0.06f;
    constexpr float BORDER_STIFFNESS   = 0.10f;  // Diem sat vien -> neo
    constexpr float HOME_STIFFNESS     = 0.002f; // 1/3 so diem ben trong -> vi tri goc
    constexpr float REST_RATIO         = 0.95f;  // Lo xo ngan hon khoang cach that -> luoi cang
    constexpr float VELOCITY_DAMPING   = 0.98f;  // Moi buoc mat 2% van toc
    constexpr int   MAJOR_EVERY        = 4;      // Moi 4 duong 1 duong "chinh" sang hon

    Vector2 Sub(Vector2 a, Vector2 b) { return { a.x - b.x, a.y - b.y }; }
    float Len(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
}

void WarpGrid::Init(float width, float height, float cellSize) {
    cell = cellSize;
    // ceil: luoi phai PHU KIN man hinh. Lam tron xuong (ban dau) voi o 32px tren 800x600 de
    // trong dai 32px ben phai + 24px ben duoi - thay ro trong anh chup. Vien thua nam ngoai
    // man hinh la vo hai (vien la diem neo, dung yen).
    cols = (int)ceilf(width / cellSize) + 1;
    rows = (int)ceilf(height / cellSize) + 1;
    points.assign((size_t)(cols * rows), PointMass{});
    springs.clear();
    accumulator = 0.0f;

    auto idx = [this](int c, int r) { return r * cols + c; };
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            PointMass& p = points[(size_t)idx(c, r)];
            p.rest = p.pos = { (float)c * cellSize, (float)r * cellSize };
            bool border = (c == 0 || r == 0 || c == cols - 1 || r == rows - 1);
            p.invMass = border ? 0.0f : 1.0f;
        }
    }
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int i = idx(c, r);
            if (c + 1 < cols) springs.push_back({ i, idx(c + 1, r), cellSize * REST_RATIO, NEIGHBOR_STIFFNESS, NEIGHBOR_DAMPING });
            if (r + 1 < rows) springs.push_back({ i, idx(c, r + 1), cellSize * REST_RATIO, NEIGHBOR_STIFFNESS, NEIGHBOR_DAMPING });
            bool nearBorder = (c == 1 || r == 1 || c == cols - 2 || r == rows - 2);
            if (points[(size_t)i].invMass > 0.0f) {
                if (nearBorder) springs.push_back({ i, -1, 0.0f, BORDER_STIFFNESS, NEIGHBOR_DAMPING });
                else if (c % 3 == 0 && r % 3 == 0) springs.push_back({ i, -1, 0.0f, HOME_STIFFNESS, NEIGHBOR_DAMPING });
            }
        }
    }
}

void WarpGrid::Step() {
    for (const Spring& s : springs) {
        PointMass& a = points[(size_t)s.a];
        // Lo xo neo: dau kia la vi tri goc dung yen cua chinh diem a
        Vector2 bPos = (s.b < 0) ? a.rest : points[(size_t)s.b].pos;
        Vector2 bVel = (s.b < 0) ? Vector2{ 0.0f, 0.0f } : points[(size_t)s.b].vel;
        Vector2 x = Sub(a.pos, bPos);
        float len = Len(x);
        if (len <= s.restLength || len < 1e-5f) continue; // CHI KEO, khong day
        float stretch = (len - s.restLength) / len;
        Vector2 dv = Sub(bVel, a.vel);
        Vector2 force = { s.stiffness * x.x * stretch - dv.x * s.damping,
                          s.stiffness * x.y * stretch - dv.y * s.damping };
        a.acc.x -= force.x * a.invMass;
        a.acc.y -= force.y * a.invMass;
        if (s.b >= 0) {
            PointMass& b = points[(size_t)s.b];
            b.acc.x += force.x * b.invMass;
            b.acc.y += force.y * b.invMass;
        }
    }
    for (PointMass& p : points) {
        if (p.invMass == 0.0f) { p.vel = { 0.0f, 0.0f }; p.acc = { 0.0f, 0.0f }; continue; }
        p.vel.x += p.acc.x;
        p.vel.y += p.acc.y;
        p.pos.x += p.vel.x;
        p.pos.y += p.vel.y;
        p.acc = { 0.0f, 0.0f };
        p.vel.x *= VELOCITY_DAMPING;
        p.vel.y *= VELOCITY_DAMPING;
        if (fabsf(p.vel.x) < 1e-3f && fabsf(p.vel.y) < 1e-3f) p.vel = { 0.0f, 0.0f };
    }
}

void WarpGrid::Update(float dt, bool simulate) {
    if (!simulate || points.empty()) { accumulator = 0.0f; return; }
    accumulator += dt;
    // Tran cap 4 buoc/frame: frame giat dai (load, debug) khong duoc lam mo phong "tua nhanh"
    // ca giay - tham chi mat on dinh. Bo phan du thay vi don lai.
    int steps = 0;
    while (accumulator >= STEP && steps < 4) { Step(); accumulator -= STEP; steps++; }
    if (steps == 4) accumulator = 0.0f;
}

void WarpGrid::ApplyRadialImpulse(Vector2 center, float strength, float radius) {
    if (points.empty() || radius <= 0.0f) return;
    // Chi quet cac o trong hop bao quanh vong tron - dan player ap luc moi frame, quet ca
    // luoi ~800-1300 diem x 100 vien dan la phi.
    int c0 = (int)fmaxf(0.0f, floorf((center.x - radius) / cell));
    int c1 = (int)fminf((float)(cols - 1), ceilf((center.x + radius) / cell));
    int r0 = (int)fmaxf(0.0f, floorf((center.y - radius) / cell));
    int r1 = (int)fminf((float)(rows - 1), ceilf((center.y + radius) / cell));
    for (int r = r0; r <= r1; r++) {
        for (int c = c0; c <= c1; c++) {
            PointMass& p = points[(size_t)(r * cols + c)];
            if (p.invMass == 0.0f) continue;
            Vector2 d = Sub(p.pos, center);
            float dist = Len(d);
            if (dist >= radius) continue;
            float falloff = 1.0f - dist / radius;
            // Diem trung tam vu no: khong co huong -> bo qua (khong chia cho 0)
            if (dist < 1e-3f) continue;
            p.vel.x += d.x / dist * strength * falloff;
            p.vel.y += d.y / dist * strength * falloff;
        }
    }
}

void WarpGrid::ApplyExplosiveForce(Vector2 center, float strength, float radius) {
    ApplyRadialImpulse(center, strength, radius);
}

void WarpGrid::ApplyImplosiveForce(Vector2 center, float strength, float radius) {
    ApplyRadialImpulse(center, -strength, radius);
}

void WarpGrid::Draw(float brightness) const {
    if (points.empty() || brightness <= 0.0f) return;
    const Color minor = Fade(Palette::GridLine, 0.55f * brightness);
    const Color major = Fade(Palette::GridLineMajor, 0.8f * brightness);
    for (int r = 0; r < rows; r++) {
        const Color rowColor = (r % MAJOR_EVERY == 0) ? major : minor;
        for (int c = 0; c + 1 < cols; c++) DrawLineEx(PointAt(c, r), PointAt(c + 1, r), 1.0f, rowColor);
    }
    for (int c = 0; c < cols; c++) {
        const Color colColor = (c % MAJOR_EVERY == 0) ? major : minor;
        for (int r = 0; r + 1 < rows; r++) DrawLineEx(PointAt(c, r), PointAt(c, r + 1), 1.0f, colColor);
    }
}
