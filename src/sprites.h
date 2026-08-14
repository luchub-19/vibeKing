#pragma once
#include "raylib.h"

// ==========================================
// SPRITE SHEET - Load() uu tien doc atlas that (assets/sprites/atlas.cfg, Kenney Space
// Shooter Redux - xem docs/ASSET_INTEGRATION.md) cho tung ten sprite; ten nao KHONG co
// trong atlas.cfg (hoac file/entry loi) se tu dong fallback ve hinh hoc procedural sinh
// trong RAM (khong crash, khong bao loi) - xem LoadAtlasEntry()/BuildXxx() trong
// sprites.cpp. Warden/Medic (Phase 1a) hien dung fallback procedural vi atlas.png (Kenney)
// da kin cho, khong con vung trong phu hop de map them 2 hinh moi ma khong tai su dung
// coordinate cua loai khac (se lam giam kha nang phan biet bang mat - xem sprites.cpp).
//
// Phải gọi Load() SAU InitWindow() (texture cần GPU context), và Unload() TRƯỚC
// CloseWindow() - xem GameManager::Run().
// ==========================================
class SpriteSheet {
public:
    Texture2D player{};
    Texture2D basicAlien{};
    Texture2D tankyAlien{};
    Texture2D zigzagAlien{};
    Texture2D ufo{};
    Texture2D kamikaze{};
    Texture2D boss{};
    Texture2D bossSentinel{};
    Texture2D bossSwarmer{};
    Texture2D warden{};  // Phase 1a (Enemy & Item Revolution, Nguoi 1) - xem BuildWarden() trong sprites.cpp
    Texture2D medic{};   // Phase 1a (Enemy & Item Revolution, Nguoi 1) - xem BuildMedic() trong sprites.cpp
    Texture2D iconSpreadShot{}; // Phase 1b (Enemy & Item Revolution, Nguoi 1) - xem BuildIconSpreadShot()
    Texture2D iconOverdrive{};  // Phase 1b (Enemy & Item Revolution, Nguoi 1) - xem BuildIconOverdrive()
    Texture2D weaver{}; // Phase 2 (Enemy & Item Revolution, Nguoi 1) - xem BuildWeaver() trong sprites.cpp
    Texture2D bomber{}; // Phase 2 (Enemy & Item Revolution, Nguoi 1) - xem BuildBomber() trong sprites.cpp

    // ICON POWER-UP (16x16, silhouette hinh hoc thuan - cung triet ly voi cac sprite
    // dich/Boss o tren) - thay cho DrawRectangle mau tron truoc day (xem PowerUpType
    // trong powerup.h va vong ve trong RenderSystem::DrawPlaying).
    Texture2D iconRapidFire{};
    Texture2D iconShield{};
    Texture2D iconPiercing{};
    Texture2D iconCleanser{};

    void Load();
    void Unload();
};

// Vẽ texture (thường 16x16) giãn vừa khít 1 Rectangle bất kỳ, nhuộm màu qua tint -
// dùng để không phải nhân bản logic DrawTexturePro ở từng chỗ gọi trong game_manager.cpp.
inline void DrawSprite(const Texture2D& tex, Rectangle dest, Color tint) {
    Rectangle src{ 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    DrawTexturePro(tex, src, dest, { 0.0f, 0.0f }, 0.0f, tint);
}
