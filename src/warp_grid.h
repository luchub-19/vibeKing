#pragma once
#include "raylib.h"
#include <vector>

// ==========================================
// LUOI NEON LO XO (GD 1 - docs/GRAPHICS_UPGRADE_PLAN.md) - nen kieu Geometry Wars: luoi nam
// duoi moi thu, bi vu no DAY lom ra roi tu dan hoi ve. Bien moi vu no thanh 1 su kien "co
// dia ly" tren ca man hinh, khong chi 1 cum hat tai cho.
//
// Mo hinh (Tuts+ "Make a Neon Vector Shooter in XNA: The Warping Grid"; Kai/KANA "Geometry
// Wars like Spring Grid"):
//   - Moi giao diem la 1 khoi luong; vien ngoai cung la diem NEO (khong bao gio di chuyen).
//   - Lo xo giua 2 diem ke nhau CHI KEO, khong day (nhu day chun) - do dai nghi = 95% khoang
//     cach ban dau nen luoi luon hoi cang, khong chung.
//   - Them lo xo rat yeu tu 1/3 so diem ve vi tri goc -> luoi luon tro ve hinh chu nhat.
//   - Buoc mo phong CO DINH 60Hz (tich luy dt) - lo xo tich phan Euler on dinh hay khong phu
//     thuoc dt, khong duoc de dt bien thien theo FPS.
//
// Phan mo phong (Init/Update/Apply*/PointAt) KHONG goi raylib -> test headless tai
// tests/test_warp_grid.cpp. Chi Draw() ve.
// ==========================================
class WarpGrid {
public:
    // cellSize px giua 2 duong luoi. Goi lai khi doi preset (GraphicsSettings::GridCellSize).
    void Init(float width, float height, float cellSize);

    // simulate=false (preset Low): luoi dung yen, bo qua moi luc - van ve duoc.
    void Update(float dt, bool simulate);

    // Vu no: day cac diem trong `radius` ra xa tam, manh nhat o tam va giam tuyen tinh ra vien.
    // `strength` la van toc tuc thoi (px/buoc) cong vao diem o tam.
    void ApplyExplosiveForce(Vector2 center, float strength, float radius);
    // Hut vao tam (boss tu luc...) - cung cong thuc, nguoc chieu.
    void ApplyImplosiveForce(Vector2 center, float strength, float radius);

    // brightness 0..1 (luat R2: nen diu xuong khi man hinh dong dan - xem RenderSystem).
    void Draw(float brightness) const;

    int Cols() const { return cols; }
    int Rows() const { return rows; }
    float CellSize() const { return cell; }
    Vector2 PointAt(int col, int row) const { return points[(size_t)(row * cols + col)].pos; }
    Vector2 RestAt(int col, int row) const { return points[(size_t)(row * cols + col)].rest; }

    static constexpr float STEP = 1.0f / 60.0f;

    // LUAT R2 (nen diu xuong khi man hinh dong): 1.0 khi <= 10 vien dan dich, giam tuyen tinh
    // con 0.4 o >= 40 vien. Ham thuan - dung chung cho luoi va cac lop nen khac sau nay.
    static float CalmFactor(int enemyBullets) {
        if (enemyBullets <= 10) return 1.0f;
        if (enemyBullets >= 40) return 0.4f;
        return 1.0f - 0.6f * (float)(enemyBullets - 10) / 30.0f;
    }

private:
    struct PointMass {
        Vector2 pos{}, rest{}, vel{}, acc{};
        float invMass = 1.0f; // 0 = neo
    };
    struct Spring {
        int a = 0, b = 0;      // index diem; b == -1: lo xo neo ve vi tri goc cua a
        float restLength = 0.0f;
        float stiffness = 0.0f;
        float damping = 0.0f;
    };

    void Step();
    void ApplyRadialImpulse(Vector2 center, float strength, float radius);

    std::vector<PointMass> points;
    std::vector<Spring> springs;
    int cols = 0, rows = 0;
    float cell = 0.0f;
    float accumulator = 0.0f;
};
