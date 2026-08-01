#include "config.h"
#include "enemy_types.h"
#include "raylib.h"
#include "thirdparty/json.hpp"
#include <fstream>

using nlohmann::json;

namespace {
    // Ghi de 1 field CHI KHI key ton tai trong JSON - field khong duoc nhac toi trong
    // balance.json thi GIU NGUYEN gia tri mac dinh da khai bao san trong config.h/
    // enemy_types.h (khong phai loi, khong log warning - day la thiet ke co chu dich:
    // designer chi can liet ke NHUNG GI HO MUON DOI, khong bat buoc liet ke toan bo).
    template <typename T>
    void Assign(const json& section, const char* key, T& target) {
        if (section.contains(key)) target = section.at(key).get<T>();
    }

    void LoadPlayer(const json& root) {
        if (!root.contains("player")) return;
        const json& s = root.at("player");
        Assign(s, "speed", Config::PLAYER_SPEED);
        Assign(s, "fire_rate", Config::PLAYER_FIRE_RATE);
        Assign(s, "invincible_time", Config::INVINCIBLE_TIME);
        Assign(s, "shield_hit_grace", Config::PLAYER_SHIELD_HIT_GRACE);
    }

    void LoadEnemyGeneral(const json& root) {
        if (!root.contains("enemy")) return;
        const json& s = root.at("enemy");
        Assign(s, "speed_inc", Config::ENEMY_SPEED_INC);
        Assign(s, "bullet_speed", Config::BULLET_SPEED);
        Assign(s, "enemy_bullet_speed", Config::ENEMY_BULLET_SPEED);
        Assign(s, "aimed_shot_chance", Config::ENEMY_AIMED_SHOT_CHANCE);
        Assign(s, "radial_burst_count", Config::RADIAL_BURST_COUNT);
    }

    void LoadDifficultyEntry(const json& root, const char* key, int tableIndex) {
        if (!root.contains("difficulty")) return;
        const json& diff = root.at("difficulty");
        if (!diff.contains(key)) return;
        const json& s = diff.at(key);
        Assign(s, "base_speed", Config::g_difficultyTable[tableIndex].enemyBaseSpeed);
        Assign(s, "max_speed", Config::g_difficultyTable[tableIndex].enemySpeedMax);
        Assign(s, "fire_rate", Config::g_difficultyTable[tableIndex].enemyFireRate);
    }

    void LoadWaveProgression(const json& root) {
        if (!root.contains("wave_progression")) return;
        const json& s = root.at("wave_progression");
        Assign(s, "extra_row_every", Config::WAVE_EXTRA_ROW_EVERY);
        Assign(s, "speed_bonus_per", Config::WAVE_SPEED_BONUS_PER);
        Assign(s, "fire_rate_step", Config::WAVE_FIRE_RATE_STEP);
        Assign(s, "fire_rate_min_mul", Config::WAVE_FIRE_RATE_MIN_MUL);
    }

    void LoadEnemyStats(const json& root) {
        if (!root.contains("enemy_stats")) return;
        const json& s = root.at("enemy_stats");
        Assign(s, "basic_score", BasicEnemy::SCORE_VALUE);
        Assign(s, "tanky_hp", TankyEnemy::HP);
        Assign(s, "tanky_score", TankyEnemy::SCORE_VALUE);
        Assign(s, "zigzag_frequency", ZigzagEnemy::FREQUENCY);
        Assign(s, "zigzag_amplitude", ZigzagEnemy::AMPLITUDE);
        Assign(s, "zigzag_score", ZigzagEnemy::SCORE_VALUE);
        Assign(s, "kamikaze_score", KamikazeEnemy::SCORE_VALUE);
        Assign(s, "kamikaze_speed", Config::KAMIKAZE_SPEED);
        Assign(s, "kamikaze_spawn_min_interval", Config::KAMIKAZE_SPAWN_MIN_INTERVAL);
        Assign(s, "kamikaze_spawn_max_interval", Config::KAMIKAZE_SPAWN_MAX_INTERVAL);
    }

    void LoadUfo(const json& root) {
        if (!root.contains("ufo")) return;
        const json& s = root.at("ufo");
        Assign(s, "spawn_min_interval", Config::UFO_SPAWN_MIN_INTERVAL);
        Assign(s, "spawn_max_interval", Config::UFO_SPAWN_MAX_INTERVAL);
        Assign(s, "speed", Config::UFO_SPEED);
        Assign(s, "score_min", Config::UFO_SCORE_MIN);
        Assign(s, "score_max", Config::UFO_SCORE_MAX);
    }

    void LoadBoss(const json& root) {
        if (!root.contains("boss")) return;
        const json& s = root.at("boss");
        Assign(s, "wave_interval", Config::BOSS_WAVE_INTERVAL);
        Assign(s, "max_hp", Config::BOSS_MAX_HP);
        Assign(s, "hp_per_wave_bonus", Config::BOSS_HP_PER_WAVE_BONUS);
        Assign(s, "speed_stage1", Config::BOSS_SPEED_STAGE1);
        Assign(s, "speed_stage2", Config::BOSS_SPEED_STAGE2);
        Assign(s, "speed_stage3", Config::BOSS_SPEED_STAGE3);
        Assign(s, "fire_interval_stage1", Config::BOSS_FIRE_INTERVAL_STAGE1);
        Assign(s, "fire_interval_stage2", Config::BOSS_FIRE_INTERVAL_STAGE2);
        Assign(s, "fire_interval_stage3", Config::BOSS_FIRE_INTERVAL_STAGE3);
        Assign(s, "radial_chance_stage2", Config::BOSS_RADIAL_CHANCE_STAGE2);
        Assign(s, "radial_chance_stage3", Config::BOSS_RADIAL_CHANCE_STAGE3);
        Assign(s, "bullet_speed", Config::BOSS_BULLET_SPEED);
        Assign(s, "score_value", Config::BOSS_SCORE_VALUE);
    }

    void LoadPowerup(const json& root) {
        if (!root.contains("powerup")) return;
        const json& s = root.at("powerup");
        Assign(s, "drop_chance", Config::POWERUP_DROP_CHANCE);
        Assign(s, "fall_speed", Config::POWERUP_FALL_SPEED);
        Assign(s, "rapidfire_duration", Config::POWERUP_RAPIDFIRE_DURATION);
        Assign(s, "rapidfire_fire_rate_mul", Config::POWERUP_RAPIDFIRE_FIRE_RATE_MUL);
        Assign(s, "shield_duration", Config::POWERUP_SHIELD_DURATION);
        Assign(s, "pierce_duration", Config::POWERUP_PIERCE_DURATION);
        Assign(s, "pierce_hits", Config::POWERUP_PIERCE_HITS);
    }

    void LoadCombo(const json& root) {
        if (!root.contains("combo")) return;
        const json& s = root.at("combo");
        Assign(s, "window", Config::COMBO_WINDOW);
        Assign(s, "bonus_per_step", Config::COMBO_BONUS_PER_STEP);
        Assign(s, "max_steps", Config::COMBO_MAX_STEPS);
    }

    void LoadBunker(const json& root) {
        if (!root.contains("bunker")) return;
        const json& s = root.at("bunker");
        Assign(s, "regen_interval", Config::BUNKER_REGEN_INTERVAL);
        Assign(s, "regen_per_tick", Config::BUNKER_REGEN_PER_TICK);
        Assign(s, "patrol_amplitude", Config::BUNKER_PATROL_AMPLITUDE);
        Assign(s, "patrol_speed", Config::BUNKER_PATROL_SPEED);
    }
}

void Config::LoadBalance(const char* path) {
    const char* actualPath = path ? path : Config::BalanceFilePath();

    std::ifstream file(actualPath);
    if (!file.is_open()) {
        // Cung triet ly voi settings.cfg/level.cfg: thieu file KHONG PHAI loi - dung
        // toan bo gia tri mac dinh da khai bao san trong config.h/enemy_types.h.
        TraceLog(LOG_INFO, "Balance: khong tim thay '%s', dung gia tri can bang mac dinh", actualPath);
        return;
    }

    json root;
    try {
        file >> root;
    } catch (const json::parse_error& e) {
        // File ton tai nhung sai cu phap JSON - TU CHOI TOAN BO (khong co gang doc nua
        // von), giu nguyen toan bo mac dinh thay vi ap dung 1 phan du lieu co the da bi
        // hong dang do.
        TraceLog(LOG_WARNING, "Balance: '%s' sai dinh dang JSON (%s) - dung gia tri mac dinh", actualPath, e.what());
        return;
    }

    // Moi nhom doc DOC LAP, boc trong try/catch RIENG - 1 nhom bi sai kieu du lieu (vd
    // designer go nham chuoi vao truong so) chi lam nhom DO giu mac dinh, khong keo sap
    // toan bo file (cac nhom khac van duoc ap dung binh thuong).
    auto safeLoad = [&](const char* sectionName, void (*loader)(const json&)) {
        try {
            loader(root);
        } catch (const json::exception& e) {
            TraceLog(LOG_WARNING, "Balance: muc '%s' trong '%s' sai kieu du lieu (%s) - giu mac dinh cho muc nay",
                      sectionName, actualPath, e.what());
        }
    };

    safeLoad("player", LoadPlayer);
    safeLoad("enemy", LoadEnemyGeneral);
    safeLoad("difficulty.easy",   [](const json& r) { LoadDifficultyEntry(r, "easy", 0); });
    safeLoad("difficulty.normal", [](const json& r) { LoadDifficultyEntry(r, "normal", 1); });
    safeLoad("difficulty.hard",   [](const json& r) { LoadDifficultyEntry(r, "hard", 2); });
    safeLoad("wave_progression", LoadWaveProgression);
    safeLoad("enemy_stats", LoadEnemyStats);
    safeLoad("ufo", LoadUfo);
    safeLoad("boss", LoadBoss);
    safeLoad("powerup", LoadPowerup);
    safeLoad("combo", LoadCombo);
    safeLoad("bunker", LoadBunker);

    TraceLog(LOG_INFO, "Balance: da nap du lieu can bang tu '%s'", actualPath);
}
