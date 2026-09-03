#pragma once
#include "raylib.h"
#include <cstddef>
#include <cstdio>

// ==========================================
// FLOATING TEXT - popup diem/combo bay len ("+50", "+80 COMBO x3") tai vi tri vua ha guc
// dich, phan hoi tuc thi ro rang hon nhieu so voi chi doi HUD SCORE o goc man hinh nhay
// so - dung CHUNG khuon DOD voi ParticlePool (mang tinh MAX_TEXTS phan tu, swap-and-pop,
// khong virtual) nhung moi phan tu mang theo 1 chuoi ky tu ngan thay vi mau/hinh dang
// thuan tuy.
//
// VE BANG DrawTextEx TRUC TIEP (nhan Font lam tham so, khong qua UICanvas): UICanvas
// (xem ui_system.h) sinh ra de gom text cho cac MAN HINH tinh (Menu/HUD/EndScreen), tranh
// lap lai tham so font/spacing o hang chuc noi goi rai rac trong RenderSystem - FloatingText
// khong phai 1 man hinh, no la 1 HIEU UNG GAMEPLAY ngan han giong Particle (vi tri/mau/
// thoi luong da duoc quyet dinh 1 LAN DUY NHAT luc Spawn(), khong co tham so nao "rai
// rac" de UICanvas giai quyet ca) - tu ve truc tiep giong het Particle/Bullet, giu dung
// nguyen tac "moi pool tu biet cach ve chinh no", chi khac la can Font (KHONG can
// SpriteSheet/Culling).
// ==========================================
class FloatingText {
private:
    Vector2 pos;
    float life;
    float maxLife;
    char text[16];
    Color color;
    bool active = false;

    static constexpr float FONT_SIZE = 18.0f;
    static constexpr float RISE_SPEED = 40.0f; // px/s - bay len DEU, khong trong luc/easing, giu don gian

public:
    void Spawn(Vector2 p, float lifeTime, const char* str, Color c) {
        pos = p;
        life = lifeTime;
        maxLife = lifeTime;
        color = c;
        active = true;
        snprintf(text, sizeof(text), "%s", str);
    }

    void Update(float dt) {
        pos.y -= RISE_SPEED * dt;
        life -= dt;
        if (life <= 0.0f) active = false;
    }

    // KHONG culling: Spawn() tai VI TRI HA GUC (ApplyComboAndScore() nhan tham so `at` -
    // xem game_manager.cpp; truoc day hardcode vi tri player nen moi popup deu chong len
    // nhau o dung 1 diem), doi song ngan (<1s) va chi troi len 1 doan nho - cung ly do voi
    // Bullet/PowerUp trong culling.h (tu huy truoc khi kip ra xa man hinh), khac voi
    // Particle (co the bi trong luc day ra ngoai bien man hinh).
    void Draw(const Font& font) const {
        float alpha = life / maxLife;
        if (alpha < 0.0f) alpha = 0.0f;
        Color c = color;
        c.a = (unsigned char)(255 * alpha);

        // Can giua tren pos (ca 2 truc) thay vi ve tu goc trai-tren - "no" ra dung tam vi
        // tri ha guc/player thay vi lech sang 1 ben tuy theo do dai chuoi.
        Vector2 size = MeasureTextEx(font, text, FONT_SIZE, 1.0f);
        Vector2 drawPos = { pos.x - size.x * 0.5f, pos.y - size.y * 0.5f };
        DrawTextEx(font, text, drawPos, FONT_SIZE, 1.0f, c);
    }

    bool IsActive() const { return active; }
    Vector2 GetPosition() const { return pos; }
};

// Cung thuat toan swap-and-pop voi ParticlePool/BulletPool - dung lai pattern da kiem
// chung thay vi tu sang tao cach quan ly moi, giu codebase nhat quan.
template <size_t MAX_TEXTS>
class FloatingTextPool {
private:
    FloatingText pool[MAX_TEXTS];
    size_t activeCount = 0;

    static constexpr float LIFETIME = 0.8f;

public:
    void Reset() { activeCount = 0; }

    // scoreGained: diem THUC NHAN cho lan ha guc nay (da nhan combo, xem
    // GameManager::ApplyComboAndScore). combo>1 -> hien them "COMBO xN" va doi sang mau
    // vang - KHOP mau chu "COMBO xN" tinh tren HUD (xem RenderSystem::DrawHUD) de nguoi
    // choi noi 2 thu voi nhau ngay, khong can doc ky.
    void Spawn(Vector2 pos, int scoreGained, int combo) {
        if (activeCount >= MAX_TEXTS) return;
        // Ep ve khoang hien thi AN TOAN cho buffer 16 byte ("+9999 COMBO x99" = 15 ky tu
        // + null = vua khit) - chi anh huong CHU HIEN THI, khong dung gi den diem/combo
        // THUC te (van tinh dung o ApplyComboAndScore, cho du 1 combo dai bat thuong).
        int dispScore = scoreGained;
        if (dispScore > 9999) dispScore = 9999;
        if (dispScore < 0) dispScore = 0;
        int dispCombo = (combo > 99) ? 99 : combo;
        char buf[16];
        if (dispCombo > 1) snprintf(buf, sizeof(buf), "+%d COMBO x%d", dispScore, dispCombo);
        else snprintf(buf, sizeof(buf), "+%d", dispScore);
        Color c = (combo > 1) ? YELLOW : WHITE;
        pool[activeCount].Spawn(pos, LIFETIME, buf, c);
        activeCount++;
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

    void Draw(const Font& font) const {
        for (size_t i = 0; i < activeCount; i++) pool[i].Draw(font);
    }

    size_t GetActiveCount() const { return activeCount; }

    // Doc vi tri 1 popup dang song - dung boi test khoa lai "popup hien tai vi tri HA GUC,
    // khong phai tai phi thuyen" (xem tests/test_game_manager.cpp). Chi doc, khong cho ghi.
    Vector2 GetPosition(size_t index) const { return pool[index].GetPosition(); }
};
