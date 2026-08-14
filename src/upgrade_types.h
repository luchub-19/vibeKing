#pragma once
#include <cstdint>
#include "config.h"
#include "localization.h"

// ==========================================
// RUN UPGRADE (Track C - Nguoi 2, Phase 3: he thong chon nang cap sau wave) - so huu
// 100% boi file nay, KHONG ai khac dung vao (xem TASK_SPLIT.md, bang file dung chung).
//
// DATA-DRIVEN, dung DUNG KHUON voi BossTypeDescriptor (enemy_types.h) va
// Config::g_difficultyTable: g_upgradeTypeDescriptors[] index THANG bang (int)UpgradeType,
// KHONG switch/case de tra ten/mo ta. Con tro `coefficient` (KHONG PHAI gia tri sao chep)
// TRO THANG toi Config::UPGRADE_* - balance.json ghi de thi descriptor TU DONG "thay"
// theo, giu dung tinh than BossTypeDescriptor (khong nhan doi so lieu can bang).
//
// 3 loai, nguoi choi chon 1-trong-3 sau MOI wave (GameManager::UpdateEndScreen(), state
// WAVE_CLEAR) - cho phep chon lap lai cung loai (cong don qua Player::runUpgradeStacks[],
// xem player.h/Player::ApplyRunUpgrade()). Rieng wave SAP toi (gm.wave - DA duoc ++ TRUOC
// khi vao WAVE_CLEAR, xem comment dau nhanh WAVE_CLEAR trong UpdateEndScreen()) chia het
// Config::BOSS_WAVE_INTERVAL: ApplyRunUpgrade() duoc goi THEM 1 lan cho CUNG 1 luot chon
// (2 lan tong cong) thay vi tach pool/loai rieng - giu dung 1 bang descriptor DUY NHAT
// cho ca wave thuong lan wave boss, "manh hon" den tu SO STACK chu khong phai tham so/
// loai moi (giu dung chu ky ham ApplyRunUpgrade(UpgradeType) nhu da chot).
// ==========================================
enum class UpgradeType : uint8_t { MoveSpeed, ExtraLife, BonusScore };
constexpr int UPGRADE_TYPE_COUNT = 3;

struct UpgradeTypeDescriptor {
    const char* name;        // Loc:: - ten hien thi ngan, dung trong "< ... >" (xem DrawUpgradeSelect, render_system.cpp)
    const char* description; // Loc:: - mo ta 1 dong, chi la nhan hien thi (so lieu THAT su luon doc tu *coefficient, khong parse tu chuoi nay)

    // Y NGHIA TUY LOAI (xem Player::ApplyRunUpgrade()):
    //   MoveSpeed   -> he so NHAN truc tiep vao Player::speed moi lan chon (vd 1.08 = +8%)
    //   BonusScore  -> diem CONG thang qua Player::AddScore() co san moi lan chon
    //   ExtraLife   -> KHONG dung toi (nullptr, an toan giong cac con tro khong-dung-den
    //                  trong BossTypeDescriptor) - luon dung nghia +1 mang/lan, cap o
    //                  Config::MAX_LIVES (da co san hang so rieng, khong can them 1 nua)
    const float* coefficient;
};

// Index THANG bang (int)UpgradeType (MoveSpeed=0, ExtraLife=1, BonusScore=2) - dung khuon
// voi g_bossTypeDescriptors, KHONG switch/case.
inline UpgradeTypeDescriptor g_upgradeTypeDescriptors[UPGRADE_TYPE_COUNT] = {
    { Loc::UpgradeMoveSpeedName,  Loc::UpgradeMoveSpeedDesc,  &Config::UPGRADE_MOVE_SPEED_MUL },
    { Loc::UpgradeExtraLifeName,  Loc::UpgradeExtraLifeDesc,  nullptr },
    { Loc::UpgradeBonusScoreName, Loc::UpgradeBonusScoreDesc, &Config::UPGRADE_BONUS_SCORE },
};

inline const UpgradeTypeDescriptor& GetUpgradeTypeDescriptor(UpgradeType type) {
    int idx = (int)type;
    if (idx < 0 || idx >= UPGRADE_TYPE_COUNT) idx = 0;
    return g_upgradeTypeDescriptors[idx];
}
