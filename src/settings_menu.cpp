#include "settings_menu.h"
#include <cmath>

namespace {
    const SettingRowDef kGeneral[] = {
        { SettingId::Language,     SettingKind::Choice, Str::SetLanguage,     Str::SetLanguageDesc },
        { SettingId::MasterVolume, SettingKind::Slider, Str::SetMasterVolume, Str::SetMasterVolumeDesc },
        { SettingId::MusicVolume,  SettingKind::Slider, Str::SetMusicVolume,  Str::SetMusicVolumeDesc },
        { SettingId::SfxVolume,    SettingKind::Slider, Str::SetSfxVolume,    Str::SetSfxVolumeDesc },
        { SettingId::UiSounds,     SettingKind::Choice, Str::SetUiSounds,     Str::SetUiSoundsDesc },
    };
    const SettingRowDef kGraphics[] = {
        { SettingId::Quality,    SettingKind::Choice, Str::SetQuality,    Str::QualityDescMedium }, // Mo ta that doi theo preset - xem RowDescription() o render
        { SettingId::Crt,        SettingKind::Choice, Str::SetCrt,        Str::SetCrtDesc },
        { SettingId::Fullscreen, SettingKind::Choice, Str::SetFullscreen, Str::SetFullscreenDesc },
        { SettingId::ShowFps,    SettingKind::Choice, Str::SetShowFps,    Str::SetShowFpsDesc },
    };
    const SettingRowDef kControls[] = {
        { SettingId::KeyLeft,   SettingKind::Key,    Str::SetKeyLeft,   Str::SetKeyDesc },
        { SettingId::KeyRight,  SettingKind::Key,    Str::SetKeyRight,  Str::SetKeyDesc },
        { SettingId::KeyShoot,  SettingKind::Key,    Str::SetKeyShoot,  Str::SetKeyDesc },
        { SettingId::KeyPause,  SettingKind::Key,    Str::SetKeyPause,  Str::SetKeyDesc },
        { SettingId::AutoFire,  SettingKind::Choice, Str::SetAutoFire,  Str::SetAutoFireDesc },
        { SettingId::ResetKeys, SettingKind::Action, Str::SetResetKeys, Str::SetResetKeysDesc },
    };
    const SettingRowDef kAccessibility[] = {
        { SettingId::ReduceFlashing, SettingKind::Choice, Str::SetReduceFlashing, Str::SetReduceFlashingDesc },
        { SettingId::Shake,          SettingKind::Choice, Str::SetShake,          Str::SetShakeDesc },
        { SettingId::GameSpeed,      SettingKind::Choice, Str::SetGameSpeed,      Str::SetGameSpeedDesc },
        { SettingId::ColorFilter,    SettingKind::Choice, Str::SetColorFilter,    Str::SetColorFilterDesc },
        { SettingId::Hitbox,         SettingKind::Choice, Str::SetHitbox,         Str::SetHitboxDesc },
    };

    template <size_t N>
    const SettingRowDef* Rows(const SettingRowDef (&arr)[N], int& count) {
        static_assert(N <= SETTINGS_MAX_ROWS, "Tab vuot SETTINGS_MAX_ROWS - layout khong du cho");
        count = (int)N;
        return arr;
    }

    // Dong bat/tat: thu tu [TAT, BAT] - Phai = bat, dung chieu "tang" nhu moi dong khac.
    int BoolIndex(bool v) { return v ? 1 : 0; }

    template <typename T, size_t N>
    int IndexOf(const T (&levels)[N], T v) {
        for (size_t i = 0; i < N; i++) if (levels[i] == v) return (int)i;
        return 0;
    }
}

Str SettingsTabLabel(SettingsTab tab) {
    switch (tab) {
        case SettingsTab::Graphics:      return Str::TabGraphics;
        case SettingsTab::Controls:      return Str::TabControls;
        case SettingsTab::Accessibility: return Str::TabAccessibility;
        case SettingsTab::General:       break;
    }
    return Str::TabGeneral;
}

const SettingRowDef* SettingsTabRows(SettingsTab tab, int& count) {
    switch (tab) {
        case SettingsTab::Graphics:      return Rows(kGraphics, count);
        case SettingsTab::Controls:      return Rows(kControls, count);
        case SettingsTab::Accessibility: return Rows(kAccessibility, count);
        case SettingsTab::General:       break;
    }
    return Rows(kGeneral, count);
}

int ChoiceCount(SettingId id) {
    switch (id) {
        case SettingId::Language:    return LANGUAGE_COUNT;
        case SettingId::Quality:     return GRAPHICS_QUALITY_COUNT;
        case SettingId::Shake:       return SHAKE_PERCENT_LEVEL_COUNT;
        case SettingId::GameSpeed:   return GAME_SPEED_LEVEL_COUNT;
        case SettingId::ColorFilter: return COLOR_FILTER_COUNT;
        case SettingId::UiSounds: case SettingId::Crt: case SettingId::Fullscreen: case SettingId::ShowFps:
        case SettingId::AutoFire: case SettingId::ReduceFlashing: case SettingId::Hitbox:
            return 2;
        default: return 0;
    }
}

int ChoiceIndex(const Settings& s, SettingId id) {
    switch (id) {
        case SettingId::Language:       return (int)s.language;
        case SettingId::Quality:        return (int)s.graphics.quality;
        case SettingId::Shake:          return IndexOf(SHAKE_PERCENT_LEVELS, s.graphics.shakePercent);
        case SettingId::GameSpeed:      return IndexOf(GAME_SPEED_LEVELS, s.gameSpeedPercent);
        case SettingId::ColorFilter:    return (int)s.graphics.colorFilter;
        case SettingId::UiSounds:       return BoolIndex(s.uiSounds);
        case SettingId::Crt:            return BoolIndex(s.graphics.crtEnabled);
        case SettingId::Fullscreen:     return BoolIndex(s.fullscreen);
        case SettingId::ShowFps:        return BoolIndex(s.showFps);
        case SettingId::AutoFire:       return BoolIndex(s.autoFire);
        case SettingId::ReduceFlashing: return BoolIndex(s.graphics.reduceFlashing);
        case SettingId::Hitbox:         return BoolIndex(s.showHitbox);
        default: return 0;
    }
}

void SetChoice(Settings& s, SettingId id, int index) {
    int n = ChoiceCount(id);
    if (n <= 0) return;
    index = index < 0 ? 0 : (index >= n ? n - 1 : index);
    switch (id) {
        case SettingId::Language:       s.language = (Language)index; break;
        case SettingId::Quality:        s.graphics.quality = (GraphicsQuality)index; break;
        case SettingId::Shake:          s.graphics.shakePercent = SHAKE_PERCENT_LEVELS[index]; break;
        case SettingId::GameSpeed:      s.gameSpeedPercent = GAME_SPEED_LEVELS[index]; break;
        case SettingId::ColorFilter:    s.graphics.colorFilter = (ColorFilter)index; break;
        case SettingId::UiSounds:       s.uiSounds = index == 1; break;
        case SettingId::Crt:            s.graphics.crtEnabled = index == 1; break;
        case SettingId::Fullscreen:     s.fullscreen = index == 1; break;
        case SettingId::ShowFps:        s.showFps = index == 1; break;
        case SettingId::AutoFire:       s.autoFire = index == 1; break;
        case SettingId::ReduceFlashing: s.graphics.reduceFlashing = index == 1; break;
        case SettingId::Hitbox:         s.showHitbox = index == 1; break;
        default: break;
    }
}

std::string ChoiceLabel(SettingId id, int index) {
    switch (id) {
        case SettingId::Language:
            return Loc::LanguageDisplayName((Language)index); // Ten ngon ngu luon viet bang CHINH no - nguoi lac vao ngon ngu la van tim duoc
        case SettingId::Quality: {
            const Str q[] = { Str::QualityLow, Str::QualityMedium, Str::QualityHigh };
            return Tr(q[index < 0 ? 0 : (index > 2 ? 2 : index)]);
        }
        case SettingId::Shake: {
            int v = SHAKE_PERCENT_LEVELS[index < 0 ? 0 : (index >= SHAKE_PERCENT_LEVEL_COUNT ? SHAKE_PERCENT_LEVEL_COUNT - 1 : index)];
            return v == 0 ? std::string(Tr(Str::Off)) : std::to_string(v) + "%";
        }
        case SettingId::GameSpeed: {
            int v = GAME_SPEED_LEVELS[index < 0 ? 0 : (index >= GAME_SPEED_LEVEL_COUNT ? GAME_SPEED_LEVEL_COUNT - 1 : index)];
            return std::to_string(v) + "%";
        }
        case SettingId::ColorFilter: {
            const Str f[] = { Str::Off, Str::FilterProtan, Str::FilterDeutan, Str::FilterTritan };
            return Tr(f[index < 0 ? 0 : (index > 3 ? 3 : index)]);
        }
        default:
            return Tr(index == 1 ? Str::On : Str::Off);
    }
}

float SliderValue(const Settings& s, SettingId id) {
    switch (id) {
        case SettingId::MasterVolume: return s.volume;
        case SettingId::MusicVolume:  return s.musicVolume;
        case SettingId::SfxVolume:    return s.sfxVolume;
        default: return 0.0f;
    }
}

void SetSlider(Settings& s, SettingId id, float value) {
    value = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    value = roundf(value / SLIDER_STEP) * SLIDER_STEP;
    if (value > 1.0f) value = 1.0f;
    switch (id) {
        case SettingId::MasterVolume: s.volume = value; break;
        case SettingId::MusicVolume:  s.musicVolume = value; break;
        case SettingId::SfxVolume:    s.sfxVolume = value; break;
        default: break;
    }
}

int Settings::* KeyField(SettingId id) {
    switch (id) {
        case SettingId::KeyLeft:  return &Settings::keyMoveLeft;
        case SettingId::KeyRight: return &Settings::keyMoveRight;
        case SettingId::KeyShoot: return &Settings::keyShoot;
        case SettingId::KeyPause: return &Settings::keyPause;
        default: return nullptr;
    }
}

bool AdjustSetting(Settings& s, SettingId id, int dir) {
    if (dir == 0) return false;
    int n = ChoiceCount(id);
    if (n > 0) {
        int before = ChoiceIndex(s, id);
        int after = (before + dir % n + n) % n;
        SetChoice(s, id, after);
        return after != before;
    }
    if (id == SettingId::MasterVolume || id == SettingId::MusicVolume || id == SettingId::SfxVolume) {
        float before = SliderValue(s, id);
        SetSlider(s, id, before + (float)dir * SLIDER_STEP);
        return fabsf(SliderValue(s, id) - before) > 1e-4f;
    }
    return false;
}
