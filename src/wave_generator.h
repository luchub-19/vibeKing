#pragma once
#include <vector>
#include <cstdint>
#include "level_config.h"

// ==========================================
// WAVE GENERATOR (B3) - THUAN TUY, KHONG PHU THUOC GameManager (dung tinh than
// level_config.h/enemy_types.h - "tang duoi khong biet tang tren ton tai", xem
// ARCHITECTURE.md muc 1). Nhan (wave, cau hinh luoi) -> tra ve danh sach FormationSpawn
// MO TA "o nao co dich loai gi" - GameManager::InitLevel() la noi DUY NHAT duyet ket qua
// nay va goi dung EnemyPool::Spawn() tuong ung (mau/kich thuoc/hp mac dinh cua tung loai
// la chi tiet "spawn NHU THE NAO", KHONG phai chi tiet "doi hinh trong NHU THE NAO" - giu
// nguyen o InitLevel() dung cho vi tri no da o san, xem game_manager.cpp).
//
// Thay THE HOAN TOAN vong lap tinh cu (hang 0 luon la Zigzag, cot chia het cho 5 luon la
// Tanky - GIONG HET nhau moi wave) bang thuat toan co NGAU NHIEN + scale theo wave, nhung
// van gioi han CHAT theo dung capacity tinh san cua EnemyPool (Config::MAX_ZIGZAG_ENEMIES/
// MAX_TANKY_ENEMIES/MAX_BASIC_ENEMIES trong config.h) BANG CONSTRUCTION (budget tru dan,
// khong chi dua vao xac suat) - khong bao gio de Generate() de nghi nhieu Zigzag/Tanky/
// Basic hon nhung gi pool tuong ung co the chua, du ket qua random co "xui" den dau
// (xem wave_generator.cpp) - tranh EnemyPool::Spawn() lang le tra ve false (an toan,
// khong crash - xem enemy_types.h) roi lam wave "thieu dich" ngoai y muon.
// ==========================================
enum class FormationEnemyKind : uint8_t { Basic, Tanky, Zigzag };

struct FormationSpawn {
    FormationEnemyKind kind;
    int column;
    int row;
    float x;
    float y;
};

namespace WaveGenerator {
    // wave: so thu tu wave hien tai (1-based) - dung de scale ca so hang (CONG THUC GIU
    // NGUYEN nhu ban goc: Config::WAVE_EXTRA_ROW_EVERY, clamp Config::MAX_GRID_ROWS -
    // khong lam lech nhip do kho da can chinh tu truoc) lan ti le Tanky/so hang Zigzag
    // (MOI - xem wave_generator.cpp).
    // grid: cau hinh luoi doc tu level.cfg (rows co so, cols, startX/Y, spacingX/Y).
    std::vector<FormationSpawn> Generate(int wave, const LevelGridConfig& grid);
}
