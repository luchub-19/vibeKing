#include "wave_generator.h"
#include "config.h"
#include "raylib.h" // GetRandomValue - khong can InitWindow/GPU context, an toan dung trong ca unit test

namespace {
    // So hang Zigzag cua wave nay: bat dau 1, +1 moi (Config::WAVE_EXTRA_ROW_EVERY * 3)
    // wave - tang CHAM HON HAN so hang thuong (extraRows moi WAVE_EXTRA_ROW_EVERY wave)
    // de khong bao gio vuot qua nhanh capacity Config::MAX_ZIGZAG_ENEMIES. Gioi han CUNG
    // (khong chi "ky vong") boi ca so hang thuc te lan capacity pool - moi hang Zigzag
    // dung DUNG `cols` phan tu nen zigzagRowCount * cols <= MAX_ZIGZAG_ENEMIES LUON DUNG
    // sau ham nay, khong phu thuoc random.
    int PickZigzagRowCount(int wave, int rows, int cols) {
        int wanted = 1 + (wave - 1) / (Config::WAVE_EXTRA_ROW_EVERY * 3);

        int maxByRows = rows / 3;
        if (maxByRows < 1) maxByRows = 1;

        int maxByPool = (cols > 0) ? (int)(Config::MAX_ZIGZAG_ENEMIES / (size_t)cols) : 1;
        if (maxByPool < 1) maxByPool = 1;

        int cap = (maxByRows < maxByPool) ? maxByRows : maxByPool;
        if (cap > rows) cap = rows; // Khong bao gio chon nhieu hang hon so hang thuc te co
        return (wanted < cap) ? wanted : cap;
    }
}

std::vector<FormationSpawn> WaveGenerator::Generate(int wave, const LevelGridConfig& grid) {
    std::vector<FormationSpawn> result;

    // SO HANG: giu NGUYEN cong thuc goc tung nam trong InitLevel() (khong doi nhip do kho
    // da can chinh) - chi phan "trong moi o la loai dich gi" ben duoi moi thuc su
    // procedural. grid.rows/grid.cols da duoc LevelGridConfig::Clamp() dam bao nam trong
    // [1, MAX_GRID_ROWS]/[1, MAX_GRID_COLS] tu luc doc file - khong can clamp lai o day.
    int extraRows = (wave - 1) / Config::WAVE_EXTRA_ROW_EVERY;
    int rows = grid.rows + extraRows;
    if (rows > Config::MAX_GRID_ROWS) rows = Config::MAX_GRID_ROWS;
    if (rows < 1) return result; // Phong thu (khong nen xay ra sau Clamp()) - tra ve rong thay vi crash

    int cols = grid.cols;
    if (cols < 1) return result;

    result.reserve((size_t)rows * (size_t)cols);

    // --- Chon NGAU NHIEN cac hang se la "hang Zigzag" (thay vi luon co dinh hang 0) ---
    int zigzagRowCount = PickZigzagRowCount(wave, rows, cols);
    std::vector<bool> isZigzagRow((size_t)rows, false);
    int assigned = 0;
    int guard = 0; // Chan vong lap ly thuyet neu GetRandomValue cu trung lai hang da chon
    while (assigned < zigzagRowCount && guard < rows * 4) {
        int r = GetRandomValue(0, rows - 1);
        if (!isZigzagRow[(size_t)r]) { isZigzagRow[(size_t)r] = true; assigned++; }
        guard++;
    }

    // --- Ti le Tanky tren cac o KHONG thuoc hang Zigzag, tang nhe theo wave (20% ->
    // toi da 45%, khop tinh than voi WAVE_SPEED_BONUS_PER/WAVE_FIRE_RATE_STEP - cung tang
    // tuyen tinh theo wave o config.h) - co budget CUNG (tankyBudget/basicBudget) tru dan
    // MOI LAN chon trung de KHONG BAO GIO vuot capacity pool tuong ung du xac suat co "xui"
    // den dau, khong chi dua vao ky vong thong ke.
    float tankyChance = 0.20f + 0.01f * (float)(wave - 1);
    if (tankyChance > 0.45f) tankyChance = 0.45f;
    size_t tankyBudget = Config::MAX_TANKY_ENEMIES;
    size_t basicBudget = Config::MAX_BASIC_ENEMIES;

    // WARDEN/MEDIC (Phase 1a - Enemy & Item Revolution, Nguoi 1): giu NGUYEN thuat toan
    // "trừ budget dần" hien co - chi la 2 nhanh roll THEM vao chuoi, xet SAU Tanky (uu
    // tien Tanky truoc, dung ty le hien tai cua no) va TRUOC fallback Basic. Dung Config::
    // (khong hardcode truc tiep nhu tankyChance o tren) - DoD Phase 1a yeu cau ro "khong
    // co so ma thuat ngoai Config/balance.json", chat hon muc Tanky da co tu truoc.
    float wardenChance = Config::WARDEN_SPAWN_CHANCE_BASE + Config::WARDEN_SPAWN_CHANCE_WAVE_STEP * (float)(wave - 1);
    if (wardenChance > Config::WARDEN_SPAWN_CHANCE_MAX) wardenChance = Config::WARDEN_SPAWN_CHANCE_MAX;
    float medicChance = Config::MEDIC_SPAWN_CHANCE_BASE + Config::MEDIC_SPAWN_CHANCE_WAVE_STEP * (float)(wave - 1);
    if (medicChance > Config::MEDIC_SPAWN_CHANCE_MAX) medicChance = Config::MEDIC_SPAWN_CHANCE_MAX;
    size_t wardenBudget = Config::MAX_WARDEN_ENEMIES;
    size_t medicBudget = Config::MAX_MEDIC_ENEMIES;

    for (int r = 0; r < rows; r++) {
        bool zigzagRow = isZigzagRow[(size_t)r];
        for (int c = 0; c < cols; c++) {
            float x = grid.startX + (float)c * grid.spacingX;
            float y = grid.startY + (float)r * grid.spacingY;

            if (zigzagRow) {
                result.push_back({ FormationEnemyKind::Zigzag, c, r, x, y });
                continue;
            }

            bool rollTanky = tankyBudget > 0 && (float)GetRandomValue(0, 999) / 1000.0f < tankyChance;
            // `&&` short-circuit: chi thuc su GetRandomValue() (tieu thu 1 lan random) khi
            // nhanh truoc da "nhuong" (chua trung) - giu dung tinh than "1 roll cho nhanh
            // nao con dang xet" cua thuat toan goc, khong lang phi so ngau nhien khi Tanky
            // da thang truoc do.
            bool rollWarden = !rollTanky && wardenBudget > 0 && (float)GetRandomValue(0, 999) / 1000.0f < wardenChance;
            bool rollMedic = !rollTanky && !rollWarden && medicBudget > 0 && (float)GetRandomValue(0, 999) / 1000.0f < medicChance;

            if (rollTanky) {
                tankyBudget--;
                result.push_back({ FormationEnemyKind::Tanky, c, r, x, y });
            } else if (rollWarden) {
                wardenBudget--;
                result.push_back({ FormationEnemyKind::Warden, c, r, x, y });
            } else if (rollMedic) {
                medicBudget--;
                result.push_back({ FormationEnemyKind::Medic, c, r, x, y });
            } else if (basicBudget > 0) {
                basicBudget--;
                result.push_back({ FormationEnemyKind::Basic, c, r, x, y });
            }
            // Het budget CA 4 loai cho 1 o (truong hop cuc hiem, xem PickZigzagRowCount
            // + comment dau file): bo trong o do (gap trong doi hinh) thay vi vuot capacity
            // EnemyPool tuong ung - Frontline AI (physics_system.cpp) da xu ly gap an toan
            // tu truoc (index=-1 nghia la "khong ai o cot nay"). Basic VAN la nhanh du
            // phong CUOI CUNG voi budget rieng (MAX_BASIC_ENEMIES) khong doi - viec them 2
            // loai canh tranh moi TRUOC no trong chuoi chi khien Basic ĐƯỢC chọn ÍT hơn,
            // khong bao gio khien no vuot tran (budget cua rieng no van tu tru dan doc lap).
        }
    }

    return result;
}
