#pragma once
#include "raylib.h"
#include "config.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cmath>

class Bullet {
private:
    Rectangle rect;
    Vector2 prevPos;  // Vi tri truoc lan Update() gan nhat - dung cho CCD (xem GetSweptRect)
    Vector2 vel;      // Van toc 2D (px/s) - quy uoc chuan man hinh: Y+ la xuong duoi.
                       // TRUOC: chi co 1 float `speed` (am/duong) => dan CHI di chuyen
                       // duoc theo truc Y. GIO: vel co ca 2 truc -> ho tro dan nham (aimed),
                       // dan toa tron (radial burst), hoac bat ky huong nao khac.
    int pierceRemaining = 0; // 0 = dan thuong (huy ngay khi trung 1 muc tieu)
    bool active;
    uint32_t spawnSeq = 0; // Thu tu sinh ra - dung de tim "vien dan cu nhat" khi pool day

public:
    Bullet() : rect{0, 0, Config::BULLET_WIDTH, Config::BULLET_HEIGHT}, prevPos{0, 0}, vel{0, 0}, active(false) {}

    void Spawn(float x, float y, Vector2 velocity, uint32_t seq, int pierceHits = 0) {
        rect.x = x;
        rect.y = y;
        prevPos = { x, y }; // Frame dau tien: chua di chuyen, swept rect = rect thuong
        vel = velocity;
        pierceRemaining = pierceHits;
        active = true;
        spawnSeq = seq;
    }

    void Update(float dt) {
        prevPos = { rect.x, rect.y };
        rect.x += vel.x * dt;
        rect.y += vel.y * dt;

        bool offVertical = (rect.y < -Config::BULLET_OFFSCREEN_MARGIN) ||
                            (rect.y > (float)Config::SCREEN_H + Config::BULLET_OFFSCREEN_MARGIN);
        bool offHorizontal = (rect.x < -Config::BULLET_OFFSCREEN_MARGIN) ||
                              (rect.x > (float)Config::SCREEN_W + Config::BULLET_OFFSCREEN_MARGIN);
        if (offVertical || offHorizontal) active = false; // Can horizontal check vi gio dan co the bay cheo/ngang (aimed, radial)
    }

    // BULLET GLOW (Nguoi 3 - Audio & UI): 1 vet mo NGUOC huong bay, dung LAI dung ky
    // thuat ParticleShape::Spark (xem particle_pool.h: DrawLineEx theo huong van toc) -
    // ve TRUOC loi dan dac ben duoi (blend cong/additive rieng CHI cho vet nay, tra ve
    // blend mac dinh truoc khi ve loi) de loi dan van 100% net/dac, vet chi "hao quang"
    // phia sau. Hoan toan tu chua (Bullet da co san `vel`), khong them tham so/khong dung
    // toi he thong nao khac.
    //
    // ==========================================
    // 2 BUG DA SUA O DAY (phat hien bang cach phong to anh chup game that, doc code khong
    // thay - ca 2 deu chi lo ra khi bang mau moi lam dan du sang de bloom bat duoc):
    //
    // BUG 1 - BE DAY LAY NHAM CHIEU. Cong thuc cu la
    //     fmaxf(rect.width, rect.height) * BULLET_GLOW_THICKNESS_MUL
    // ma dan la 5x15 px (BULLET_WIDTH x BULLET_HEIGHT) => fmaxf tra ve 15, tuc CHIEU DAI
    // doc theo huong bay, roi nhan 1.8 thanh be day 27px. Vet sang vi vay rong gap 5,4 lan
    // chinh vien dan (5px) VA rong hon ca do dai cua no (14px) - nhin ra 1 tam van dat
    // ngang phia sau dan chu khong phai 1 vet luot. Thu can lay la TIET DIEN vuong goc voi
    // huong bay, tuc chieu NGAN: fminf. Voi dan bay cheo (aimed shot/radial burst) fminf
    // van dung vi truc dai cua dan luon la truc bay.
    //
    // BUG 2 - VET KHONG NHAT DAN, KET THUC BANG CANH CUNG. DrawLineEx voi 1 be day lon la
    // dung 1 hinh chu nhat dac, alpha deu tu dau den duoi. Vet luot phai MO DAN ve phia
    // duoi moi doc ra la "chuyen dong", con canh cat ngang dot ngot thi doc ra la "1 mieng
    // hinh hoc". Gio chia lam GLOW_SEGMENTS doan, moi doan mong hon va mo hon doan truoc.
    //
    // KHONG co test tu dong cho phan nay: day la ham VE thuan (khong tra ve gia tri, khong
    // doi state), chi kiem chung duoc bang mat. Doi 2 hang so ben duoi thi chup lai anh de
    // xac nhan, dung tin code doc suong la dung.
    // ==========================================
    void Draw(Color color) const {
        float speed = sqrtf(vel.x * vel.x + vel.y * vel.y);
        if (speed > 1.0f) {
            constexpr int GLOW_SEGMENTS = 4; // Du de mat doc ra do nhat dan, khong du nhieu de ton lenh ve

            Vector2 center = { rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f };
            Vector2 dir = { vel.x / speed, vel.y / speed };
            // fminf: TIET DIEN vuong goc huong bay (xem BUG 1 o tren), khong phai fmaxf.
            float headThickness = fminf(rect.width, rect.height) * Config::BULLET_GLOW_THICKNESS_MUL;

            BeginBlendMode(BLEND_ADDITIVE); // Cong don anh sang thay vi che phu - moi ra cam giac "phat sang"
            for (int i = 0; i < GLOW_SEGMENTS; i++) {
                float t0 = (float)i / (float)GLOW_SEGMENTS;
                float t1 = (float)(i + 1) / (float)GLOW_SEGMENTS;
                // Lay do mo/be day tai DIEM GIUA doan: neu lay tai t1 thi doan cuoi cung
                // luon co he so 0 (vo hinh), phi mat 1/4 chieu dai vet.
                float falloff = 1.0f - (t0 + t1) * 0.5f;

                Vector2 p0 = { center.x - dir.x * Config::BULLET_GLOW_TRAIL_LENGTH * t0,
                                center.y - dir.y * Config::BULLET_GLOW_TRAIL_LENGTH * t0 };
                Vector2 p1 = { center.x - dir.x * Config::BULLET_GLOW_TRAIL_LENGTH * t1,
                                center.y - dir.y * Config::BULLET_GLOW_TRAIL_LENGTH * t1 };

                Color glow = color;
                glow.a = (unsigned char)(255.0f * Config::BULLET_GLOW_ALPHA * falloff);
                DrawLineEx(p0, p1, headThickness * (0.3f + 0.7f * falloff), glow);
            }
            EndBlendMode();
        }
        DrawRectangleRec(rect, color);
    }

    bool IsActive() const { return active; }
    Rectangle GetRect() const { return rect; }
    uint32_t GetSpawnSeq() const { return spawnSeq; }
    Vector2 GetVel() const { return vel; } // Phase 1b (Nguoi 1): can cong khai de test xac minh goc ban Spread Shot - truoc day chua co getter nao cho vel

    // Goi khi dan vua trung 1 muc tieu. Tra ve true neu dan CON XUYEN TIEP DUOC (con
    // pierceRemaining > 0, da tru di 1) - trong truong hop nay KHONG duoc huy dan, no
    // van con hoat dong o dung index hien tai va tiep tuc bay. Tra ve false neu day la
    // lan trung cuoi cung (dan thuong, hoac pierce da het) - luc do caller can Destroy().
    bool ConsumePierce() {
        if (pierceRemaining > 0) {
            pierceRemaining--;
            return true;
        }
        return false;
    }

    // CCD (Continuous Collision Detection) tong quat cho MOI huong bay (truoc day chi
    // xu ly truc Y thuan tuy vi dan chi roi thang; gio dan co the bay cheo/ngang nen phai
    // quet ca 2 truc): bounding box bao trum TOAN BO doan duong da di trong 1 frame, tu
    // prevPos den vi tri hien tai, thay vi chi test AABB tinh tai vi tri CUOI frame (co
    // the "nhay qua" hoan toan 1 muc tieu mong/nhanh ma khong bao gio chong lan no o bat
    // ky frame nao - dac biet ro o toc do cao/FPS thap).
    Rectangle GetSweptRect() const {
        float minX = (rect.x < prevPos.x) ? rect.x : prevPos.x;
        float maxX = (rect.x > prevPos.x) ? (rect.x + rect.width) : (prevPos.x + rect.width);
        float minY = (rect.y < prevPos.y) ? rect.y : prevPos.y;
        float maxY = (rect.y > prevPos.y) ? (rect.y + rect.height) : (prevPos.y + rect.height);
        return { minX, minY, maxX - minX, maxY - minY };
    }
};

// Pool cấp phát 1 lần trên stack, dùng thuật toán swap-and-pop để spawn/destroy O(1)
// mà không cần dịch chuyển toàn bộ mảng. Đạn sống luôn nằm liền khối ở đầu mảng.
template <size_t MAX_BULLETS>
class BulletPool {
private:
    Bullet pool[MAX_BULLETS];
    size_t activeCount = 0;
    uint32_t nextSeq = 0; // Tang don dieu, dung de xac dinh "cu nhat" bat ke thu tu vat ly trong mang

public:
    void Reset() { activeCount = 0; nextSeq = 0; }

    void Fire(float x, float y, Vector2 vel, int pierceHits = 0) {
        if (activeCount < MAX_BULLETS) {
            pool[activeCount].Spawn(x, y, vel, nextSeq++, pierceHits);
            activeCount++;
            return;
        }

        // OLDEST-OVERRIDE: pool day - thay vi am tham bo qua yeu cau ban moi, ghi de
        // dung VIEN DAN CU NHAT (spawnSeq nho nhat). Quet O(MAX_BULLETS) CHI xay ra luc
        // pool thuc su bao hoa - khong anh huong duong di thuong (con cho trong) o tren.
        size_t oldestIdx = 0;
        uint32_t oldestSeq = pool[0].GetSpawnSeq();
        for (size_t i = 1; i < MAX_BULLETS; i++) {
            if (pool[i].GetSpawnSeq() < oldestSeq) {
                oldestSeq = pool[i].GetSpawnSeq();
                oldestIdx = i;
            }
        }
        pool[oldestIdx].Spawn(x, y, vel, nextSeq++, pierceHits);
    }

    void Destroy(size_t index) {
        if (index >= activeCount) return;
        activeCount--;
        pool[index] = pool[activeCount];
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
    Bullet& GetBullet(size_t index) {
        assert(index < activeCount && "GetBullet: index >= activeCount - doc vien dan da Destroy()/chua active");
        return pool[index];
    }
};
