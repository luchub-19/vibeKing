#pragma once
#include <string>
#include "settings.h"
#include "localization.h"

// ==========================================
// MAN CAI DAT 4 TAB - MO TA DONG (thuan du lieu, khong raylib-ve) -> test tai tests/test_ui.cpp.
//
// 1-NGUON-DUY-NHAT: bang SettingsTabRows() quyet dinh tab nao co dong nao, theo thu tu nao,
// kieu gi - CA GameManager::UpdateSettingsScreen() (phim/chuot lam gi) LAN RenderSystem::
// DrawSettings() (ve gi) deu duyet chung bang nay. Truoc day trang GRAPHICS cu co enum
// GraphicsRow rieng + switch rieng o 2 file; them 1 dong phai sua 3 cho. Gio them 1 cai dat moi
// = 1 dong trong bang + 1 case trong 3 ham Choice*/Slider* ben duoi.
//
// Ham o day CHI doi Settings (du lieu). Hieu ung phu (ap am luong vao AudioSystem, doi ngon ngu,
// bat fullscreen...) do GameManager::ApplySettings() lam sau moi lan doi - tach nhu vay de test
// duoc "bam Phai tren dong X thi Settings doi the nao" ma khong can am thanh/cua so that.
//
// Phan tab (tham khao Celeste/Hades/The Last of Us Part II - bo tro nang duoc to chuc thanh 1
// tab rieng, de tim thay chu khong rai rac trong Do hoa/Am thanh):
//   CHUNG     : ngon ngu + 3 tang am luong + am thanh menu
//   DO HOA    : chat luong, CRT, toan man hinh, hien FPS
//   DIEU KHIEN: doi 4 phim, tu dong ban, khoi phuc phim
//   TRO NANG  : giam nhap nhay, rung, toc do game, loc mau, hien hitbox
// ==========================================

enum class SettingsTab : uint8_t { General, Graphics, Controls, Accessibility };
constexpr int SETTINGS_TAB_COUNT = 4;

enum class SettingId : uint8_t {
    Language, MasterVolume, MusicVolume, SfxVolume, UiSounds,
    Quality, Crt, Fullscreen, ShowFps,
    KeyLeft, KeyRight, KeyShoot, KeyPause, AutoFire, ResetKeys,
    ReduceFlashing, Shake, GameSpeed, ColorFilter, Hitbox,
};

// Choice : nhieu lua chon roi rac (Trai/Phai doi, chuot bam thang vao o).
// Slider : 0..1 buoc SLIDER_STEP (Trai/Phai, chuot keo).
// Key    : doi phim - ENTER/bam de vao che do "cho phim moi".
// Action : nut bam 1 lan (khoi phuc phim).
enum class SettingKind : uint8_t { Choice, Slider, Key, Action };

struct SettingRowDef {
    SettingId id;
    SettingKind kind;
    Str label;
    Str desc;
};

constexpr float SLIDER_STEP = 0.05f;
constexpr int SETTINGS_MAX_ROWS = 6; // Tab dai nhat (Dieu khien) - layout chua du cho

Str SettingsTabLabel(SettingsTab tab);
const SettingRowDef* SettingsTabRows(SettingsTab tab, int& count);

// --- Choice ---
int ChoiceCount(SettingId id);
int ChoiceIndex(const Settings& s, SettingId id);
void SetChoice(Settings& s, SettingId id, int index); // index ngoai pham vi -> kep
std::string ChoiceLabel(SettingId id, int index);     // Da dich theo ngon ngu hien hanh

// --- Slider ---
float SliderValue(const Settings& s, SettingId id);
void SetSlider(Settings& s, SettingId id, float value); // Kep 0..1, lam tron ve SLIDER_STEP

// --- Key --- (con tro toi field ma phim trong Settings, nullptr neu khong phai dong Key)
int Settings::* KeyField(SettingId id);

// Trai (-1) / Phai (+1) tren 1 dong bat ky: Choice quay vong, Slider +-SLIDER_STEP, Key/Action
// khong lam gi. Tra ve true neu Settings THAT SU doi (de GameManager biet co can luu/ap dung).
bool AdjustSetting(Settings& s, SettingId id, int dir);
