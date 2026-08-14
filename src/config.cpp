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
        Assign(s, "extra_life_score_threshold", Config::EXTRA_LIFE_SCORE_THRESHOLD);
        Assign(s, "max_lives", Config::MAX_LIVES);
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

        Assign(s, "warden_hp", WardenEnemy::HP);
        Assign(s, "warden_score", WardenEnemy::SCORE_VALUE);
        Assign(s, "warden_reinforcement_count", Config::WARDEN_REINFORCEMENT_COUNT);
        Assign(s, "warden_spawn_chance_base", Config::WARDEN_SPAWN_CHANCE_BASE);
        Assign(s, "warden_spawn_chance_max", Config::WARDEN_SPAWN_CHANCE_MAX);
        Assign(s, "warden_spawn_chance_wave_step", Config::WARDEN_SPAWN_CHANCE_WAVE_STEP);
        Assign(s, "medic_score", MedicEnemy::SCORE_VALUE);
        Assign(s, "medic_heal_interval", Config::MEDIC_HEAL_INTERVAL);
        Assign(s, "medic_heal_amount", Config::MEDIC_HEAL_AMOUNT);
        Assign(s, "medic_spawn_chance_base", Config::MEDIC_SPAWN_CHANCE_BASE);
        Assign(s, "medic_spawn_chance_max", Config::MEDIC_SPAWN_CHANCE_MAX);
        Assign(s, "medic_spawn_chance_wave_step", Config::MEDIC_SPAWN_CHANCE_WAVE_STEP);

        // Phase 2 (Enemy & Item Revolution, Nguoi 1): Weaver/Bomber KHONG di qua
        // WaveGenerator (khong co spawn_chance/budget nhu Warden/Medic o tren) - van CUNG
        // muc "enemy_stats" vi van la du lieu can bang dich, chi khac co che spawn.
        Assign(s, "weaver_score", WeaverEnemy::SCORE_VALUE);
        Assign(s, "weaver_spawn_min_interval", Config::WEAVER_SPAWN_MIN_INTERVAL);
        Assign(s, "weaver_spawn_max_interval", Config::WEAVER_SPAWN_MAX_INTERVAL);
        Assign(s, "weaver_speed_x", Config::WEAVER_SPEED_X);
        Assign(s, "weaver_weave_amplitude", Config::WEAVER_WEAVE_AMPLITUDE);
        Assign(s, "weaver_weave_frequency", Config::WEAVER_WEAVE_FREQUENCY);
        Assign(s, "bomber_score", BomberEnemy::SCORE_VALUE);
        Assign(s, "bomber_spawn_min_interval", Config::BOMBER_SPAWN_MIN_INTERVAL);
        Assign(s, "bomber_spawn_max_interval", Config::BOMBER_SPAWN_MAX_INTERVAL);
        Assign(s, "bomber_speed_x", Config::BOMBER_SPEED_X);
        Assign(s, "bomber_bomb_interval", Config::BOMBER_BOMB_INTERVAL);
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

        // SENTINEL / SWARMER: cung nam trong muc "boss" (khong tach section rieng) - ve
        // ban chat van la du lieu can bang hanh vi Boss, chi la cho 2 loai xoay vong moi
        // thay vi loai goc.
        Assign(s, "sentinel_sway_amplitude", Config::BOSS_SENTINEL_SWAY_AMPLITUDE);
        Assign(s, "sentinel_sway_frequency", Config::BOSS_SENTINEL_SWAY_FREQUENCY);
        Assign(s, "sentinel_shield_interval", Config::BOSS_SENTINEL_SHIELD_INTERVAL);
        Assign(s, "sentinel_shield_duration", Config::BOSS_SENTINEL_SHIELD_DURATION);
        Assign(s, "sentinel_shield_fire_interval", Config::BOSS_SENTINEL_SHIELD_FIRE_INTERVAL);
        Assign(s, "swarmer_sway_amplitude", Config::BOSS_SWARMER_SWAY_AMPLITUDE);
        Assign(s, "swarmer_sway_frequency", Config::BOSS_SWARMER_SWAY_FREQUENCY);
        Assign(s, "swarmer_summon_interval", Config::BOSS_SWARMER_SUMMON_INTERVAL);
        Assign(s, "swarmer_summon_count", Config::BOSS_SWARMER_SUMMON_COUNT);
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
        // Phase 1b (Enemy & Item Revolution, Nguoi 1): SpreadShot/Overdrive + trong so roi
        // - cung muc "powerup" nhu tren, KHONG tach muc rieng (van la du lieu can bang cho
        // cung 1 he thong power-up).
        Assign(s, "spreadshot_duration", Config::POWERUP_SPREADSHOT_DURATION);
        Assign(s, "spread_shot_angle_deg", Config::SPREAD_SHOT_ANGLE_DEG);
        Assign(s, "overdrive_duration", Config::POWERUP_OVERDRIVE_DURATION);
        Assign(s, "overdrive_fire_rate_mul", Config::POWERUP_OVERDRIVE_FIRE_RATE_MUL);
        Assign(s, "weight_rapidfire", Config::POWERUP_WEIGHT_RAPIDFIRE);
        Assign(s, "weight_shield", Config::POWERUP_WEIGHT_SHIELD);
        Assign(s, "weight_piercing", Config::POWERUP_WEIGHT_PIERCING);
        Assign(s, "weight_cleanser", Config::POWERUP_WEIGHT_CLEANSER);
        Assign(s, "weight_spreadshot", Config::POWERUP_WEIGHT_SPREADSHOT);
        Assign(s, "weight_overdrive", Config::POWERUP_WEIGHT_OVERDRIVE);
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

    void LoadAnim(const json& root) {
        if (!root.contains("anim")) return;
        const json& s = root.at("anim");
        Assign(s, "idle_bob_amplitude", Config::ANIM_IDLE_BOB_AMPLITUDE);
        Assign(s, "idle_bob_frequency", Config::ANIM_IDLE_BOB_FREQUENCY);
        Assign(s, "idle_scale_amplitude", Config::ANIM_IDLE_SCALE_AMPLITUDE);
        Assign(s, "idle_phase_step", Config::ANIM_IDLE_PHASE_STEP);
        Assign(s, "boss_idle_bob_amplitude", Config::ANIM_BOSS_IDLE_BOB_AMPLITUDE);
        Assign(s, "boss_idle_bob_frequency", Config::ANIM_BOSS_IDLE_BOB_FREQUENCY);
        Assign(s, "boss_idle_scale_amplitude", Config::ANIM_BOSS_IDLE_SCALE_AMPLITUDE);
    }

    void LoadDda(const json& root) {
        if (!root.contains("dda")) return;
        const json& s = root.at("dda");
        Assign(s, "step_up", Config::DDA_STEP_UP);
        Assign(s, "step_down", Config::DDA_STEP_DOWN);
        Assign(s, "min_mul", Config::DDA_MIN_MUL);
        Assign(s, "max_mul", Config::DDA_MAX_MUL);
        Assign(s, "struggle_threshold", Config::DDA_STRUGGLE_THRESHOLD);
    }

    // Track C - Nguoi 2, Phase 3 (xem upgrade_types.h). Khoi RIENG, khong dung toi
    // g_difficultyTable/g_bossTypeDescriptors - cung tinh than voi LoadDda() o tren.
    void LoadUpgrades(const json& root) {
        if (!root.contains("upgrades")) return;
        const json& s = root.at("upgrades");
        Assign(s, "move_speed_mul", Config::UPGRADE_MOVE_SPEED_MUL);
        Assign(s, "bonus_score", Config::UPGRADE_BONUS_SCORE);
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
    safeLoad("anim", LoadAnim);
    safeLoad("dda", LoadDda);
    safeLoad("upgrades", LoadUpgrades);

    TraceLog(LOG_INFO, "Balance: da nap du lieu can bang tu '%s'", actualPath);
}
