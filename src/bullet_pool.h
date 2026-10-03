#pragma once
#include "raylib.h"
#include "config.h"
#include "palette.h"
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
    // DAN XUYEN NHO "DA DI QUA CON NAO": dan xuyen qua 1 dich NHIEU MAU (Tanky/Warden/Boss)
    // ma dich chua chet thi dan van nam CHONG LEN no them vai frame (dan 600px/s = 10px/frame,
    // Tanky cao 30px, Boss 90px). Truoc day moi frame chong len do lai la 1 lan trung MOI:
    // tru them 1 mau VA tieu them 1 luot xuyen - 1 vien dan xuyen ha guc Tanky 3 mau mot
    // minh, va gay ~4 sat thuong len Boss. "Xuyen" tro thanh "nhan sat thuong" tren 1 muc
    // tieu thay vi "di tiep toi muc tieu KHAC". Luu UID (xem NextEnemyUid, enemy_types.h) cua
    // vai muc tieu gan nhat - du cho so luot xuyen toi da thuc te.
    static constexpr int PIERCE_MEMORY = 4;
    uint32_t piercedUids[PIERCE_MEMORY] = {};
    int piercedNext = 0;
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
        for (uint32_t& u : piercedUids) u = 0;
        piercedNext = 0;
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
    // VE 2 LUOT (GD 2 - docs/GRAPHICS_UPGRADE_PLAN.md): BulletPool::DrawGlows() ve quang
    // sang cua MOI vien trong 1 khoi BLEND_ADDITIVE duy nhat, roi BulletPool::DrawCores() moi
    // ve loi dac. Truoc day Draw() bat/tat blend mode CHO TUNG VIEN - moi lan doi blend mode
    // la 1 lan xa batch GPU, 500 vien dan dich = ~1000 lan xa/frame. Tach luot con cho phep
    // RenderSystem dat loi dan dich TREN moi lop hieu ung (luat R1: dan dich luon doc duoc).

    // Quang sang (gia dinh dang trong BeginBlendMode(BLEND_ADDITIVE) - BulletPool lo viec do).
    // Vach mo nguoc huong bay + 1 quang tron nho o dau dan de mat bat duoc vi tri ngay ca khi
    // vach ngan (dan bay cham).
    void DrawGlow(Color color) const {
        float speed = sqrtf(vel.x * vel.x + vel.y * vel.y);
        if (speed <= 1.0f) return;
        constexpr int GLOW_SEGMENTS = 4; // Du de mat doc ra do nhat dan, khong du nhieu de ton lenh ve

        Vector2 center = { rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f };
        Vector2 dir = { vel.x / speed, vel.y / speed };
        // fminf: TIET DIEN vien dan, khong phai chieu dai (xem bai hoc fmaxf trong CLAUDE.md)
        float headThickness = fminf(rect.width, rect.height) * Config::BULLET_GLOW_THICKNESS_MUL;

        for (int i = 0; i < GLOW_SEGMENTS; i++) {
            float t0 = (float)i / (float)GLOW_SEGMENTS;
            float t1 = (float)(i + 1) / (float)GLOW_SEGMENTS;
            float falloff = 1.0f - (t0 + t1) * 0.5f;

            Vector2 p0 = { center.x - dir.x * Config::BULLET_GLOW_TRAIL_LENGTH * t0,
                            center.y - dir.y * Config::BULLET_GLOW_TRAIL_LENGTH * t0 };
            Vector2 p1 = { center.x - dir.x * Config::BULLET_GLOW_TRAIL_LENGTH * t1,
                            center.y - dir.y * Config::BULLET_GLOW_TRAIL_LENGTH * t1 };

            Color glow = color;
            glow.a = (unsigned char)(255.0f * Config::BULLET_GLOW_ALPHA * falloff);
            DrawLineEx(p0, p1, headThickness * (0.3f + 0.7f * falloff), glow);
        }
        // Quang dau dan: 2 vong dong tam alpha thap thay cho 1 gradient that - DrawCircleGradient
        // doi chu ky giua raylib 5.5 va 5.6 (xem CLAUDE.md), 2 vong la du mem o kich thuoc nay.
        Color halo = color;
        // Ban kinh < nua chieu dai dan: ban dau dung 1.1x headThickness (~10px) -> anh chup cho
        // thay 1 dia do viền cung to hon ca vien dan, nuot mat vach duoi. Quang chi de "viền"
        // loi dan, phan toa sang rong do bloom lo.
        halo.a = (unsigned char)(255.0f * Config::BULLET_GLOW_ALPHA * 0.3f);
        DrawCircleV(center, headThickness * 0.6f, halo);
        DrawCircleV(center, headThickness * 0.38f, halo);
    }

    // Loi dac XOAY theo huong bay. Truoc day DrawRectangleRec() luon dung thang -> dan bay
    // cheo (vong dan boss, dan nham) thanh thanh doc trong khi vach sang di cheo: nhin nhu
    // que gay (thay ro o anh canh boss GD 0). Them 1 soi "nong" sang hon chay doc than dan:
    // kieu ong neon, va giup phan biet loi dan voi quang cua chinh no.
    void DrawCore(Color color) const {
        Vector2 center = { rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f };
        float speed = sqrtf(vel.x * vel.x + vel.y * vel.y);
        // Truc dai cua rect (0,1) sau khi xoay goc theta (raylib xoay theo chieu kim dong ho,
        // Y huong xuong) thanh (-sin, cos) -> muon trung (dx, dy) thi theta = atan2(-dx, dy).
        float angleDeg = (speed > 1.0f) ? atan2f(-vel.x, vel.y) * (180.0f / 3.14159265f) : 0.0f;
        Vector2 axis = (speed > 1.0f) ? Vector2{ vel.x / speed, vel.y / speed } : Vector2{ 0.0f, 1.0f };

        // DAN XUYEN (GD 2, nguon FR "moi vu khi 1 chu ky hinh anh"): dai 1.6x, manh 0.6x + mui
        // nhon hinh thoi o dau - doc ra "xuyen qua" bang HINH DANG, khong chi bang mau. Chi
        // hinh ve; hitbox (rect) giu nguyen.
        const bool piercing = pierceRemaining > 0;
        const float w = piercing ? rect.width * 0.6f : rect.width;
        const float h = piercing ? rect.height * 1.6f : rect.height;
        Rectangle r{ center.x, center.y, w, h };
        DrawRectanglePro(r, { w / 2.0f, h / 2.0f }, angleDeg, color);
        if (piercing) {
            Vector2 tip{ center.x + axis.x * (h * 0.5f + 2.0f), center.y + axis.y * (h * 0.5f + 2.0f) };
            DrawPoly(tip, 4, rect.width * 0.75f, angleDeg, color);
        }

        float half = h * 0.5f - 1.5f;
        Color hot = {
            (unsigned char)(color.r + (255 - color.r) * 0.65f),
            (unsigned char)(color.g + (255 - color.g) * 0.65f),
            (unsigned char)(color.b + (255 - color.b) * 0.65f), 255 };
        DrawLineEx({ center.x - axis.x * half, center.y - axis.y * half },
                   { center.x + axis.x * half, center.y + axis.y * half },
                   fmaxf(1.0f, w * 0.4f), hot);
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

    // Xem giai thich piercedUids o tren. uid=0 nghia la "khong co" - khong bao gio khop.
    bool HasPierced(uint32_t uid) const {
        if (uid == 0) return false;
        for (uint32_t u : piercedUids) if (u == uid) return true;
        return false;
    }
    void RememberPierced(uint32_t uid) {
        piercedUids[piercedNext] = uid;
        piercedNext = (piercedNext + 1) % PIERCE_MEMORY;
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

    // Xem chu thich DrawGlow/DrawCore trong Bullet: 1 khoi additive cho CA pool.
    void DrawGlows(Color color) const {
        if (activeCount == 0) return;
        BeginBlendMode(BLEND_ADDITIVE);
        for (size_t i = 0; i < activeCount; i++) pool[i].DrawGlow(color);
        EndBlendMode();
    }

    // breathe (dan dich, tat khi reduceFlashing): do sang loi dao dong +-9% theo sin 1.4Hz, lech
    // pha theo spawnSeq - ca man dan "song" va mat bat chuyen dong de hon (nguon GD 2). Bien do nho
    // + song sin (khong bat/tat) nen khong phai "nhap nhay" theo WCAG.
    void DrawCores(Color color, float time = 0.0f, bool breathe = false) const {
        for (size_t i = 0; i < activeCount; i++) {
            Color c = color;
            if (breathe) {
                float k = 1.0f + 0.09f * sinf(time * 9.0f + (float)pool[i].GetSpawnSeq() * 0.7f);
                c = Palette::Shade(color, k);
            }
            pool[i].DrawCore(c);
        }
    }

    size_t GetActiveCount() const { return activeCount; }
    Bullet& GetBullet(size_t index) {
        assert(index < activeCount && "GetBullet: index >= activeCount - doc vien dan da Destroy()/chua active");
        return pool[index];
    }
};
