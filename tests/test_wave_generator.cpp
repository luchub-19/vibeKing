#include "thirdparty/catch.hpp"
#include "wave_generator.h"
#include "config.h"

namespace {
    LevelGridConfig DefaultGrid() {
        LevelGridConfig g; // rows=4, cols=10 - dung mac dinh giong het level.cfg
        return g;
    }
}

// ==========================================
// WAVE GENERATOR (B3) - test THUAN, khong can GameManager/window/GPU (dung tinh than
// da co san cua tests/ - xem comment dau test_boss.cpp). Vi thuat toan co random
// (GetRandomValue), phan lon test o day kiem tra BAT BIEN dung voi MOI ket qua random co
// the co (capacity, tong so o, "hang Zigzag la CA HANG") thay vi 1 gia tri co dinh.
// ==========================================

TEST_CASE("WaveGenerator::Generate: tong so spawn = rows*cols (wave dau, chua het budget)", "[wave_generator]") {
    LevelGridConfig grid = DefaultGrid();
    auto spawns = WaveGenerator::Generate(1, grid);
    // Wave 1: rows=grid.rows=4 (extraRows=0), cols=10 -> 40 o, con rat xa capacity
    // (MAX_ZIGZAG=20/MAX_TANKY=44/MAX_BASIC=176) nen KHONG co o nao bi bo trong (gap).
    REQUIRE(spawns.size() == 40);
}

TEST_CASE("WaveGenerator::Generate: so hang tang dung cong thuc WAVE_EXTRA_ROW_EVERY, clamp MAX_GRID_ROWS", "[wave_generator]") {
    LevelGridConfig grid = DefaultGrid(); // rows=4, cols=10

    for (int wave : { 1, 2, 4, 7, 10, 50 }) {
        auto spawns = WaveGenerator::Generate(wave, grid);
        int expectedExtra = (wave - 1) / Config::WAVE_EXTRA_ROW_EVERY;
        int expectedRows = grid.rows + expectedExtra;
        if (expectedRows > Config::MAX_GRID_ROWS) expectedRows = Config::MAX_GRID_ROWS;

        int maxRowSeen = -1;
        for (const auto& s : spawns) maxRowSeen = (s.row > maxRowSeen) ? s.row : maxRowSeen;
        REQUIRE(maxRowSeen == expectedRows - 1); // row la 0-based
    }
}

TEST_CASE("WaveGenerator::Generate: khong bao gio vuot capacity pool cua bat ky loai nao, du wave lon toi dau", "[wave_generator]") {
    LevelGridConfig grid = DefaultGrid();

    for (int wave = 1; wave <= 60; wave++) {
        auto spawns = WaveGenerator::Generate(wave, grid);
        size_t zigzag = 0, tanky = 0, basic = 0;
        for (const auto& s : spawns) {
            if (s.kind == FormationEnemyKind::Zigzag) zigzag++;
            else if (s.kind == FormationEnemyKind::Tanky) tanky++;
            else basic++;
        }
        REQUIRE(zigzag <= Config::MAX_ZIGZAG_ENEMIES);
        REQUIRE(tanky <= Config::MAX_TANKY_ENEMIES);
        REQUIRE(basic <= Config::MAX_BASIC_ENEMIES);
    }
}

TEST_CASE("WaveGenerator::Generate: moi hang Zigzag la CA HANG (khong tron lan Tanky/Basic trong cung 1 hang)", "[wave_generator]") {
    LevelGridConfig grid = DefaultGrid();

    for (int wave : { 1, 5, 12, 30 }) {
        auto spawns = WaveGenerator::Generate(wave, grid);

        // Gom kind theo tung hang - 1 hang phai THUAN 1 trong 2 dang: "toan Zigzag" hoac
        // "khong co Zigzag nao" (co the tron Tanky/Basic).
        std::vector<int> zigzagCountPerRow(64, 0), totalCountPerRow(64, 0);
        for (const auto& s : spawns) {
            totalCountPerRow[(size_t)s.row]++;
            if (s.kind == FormationEnemyKind::Zigzag) zigzagCountPerRow[(size_t)s.row]++;
        }
        for (size_t r = 0; r < totalCountPerRow.size(); r++) {
            if (totalCountPerRow[r] == 0) continue;
            bool allZigzag = (zigzagCountPerRow[r] == totalCountPerRow[r]);
            bool noZigzag = (zigzagCountPerRow[r] == 0);
            REQUIRE((allZigzag || noZigzag));
        }
    }
}

TEST_CASE("WaveGenerator::Generate: toa do x/y khop cong thuc luoi (startX/Y + col/row * spacing)", "[wave_generator]") {
    LevelGridConfig grid = DefaultGrid();
    auto spawns = WaveGenerator::Generate(1, grid);
    REQUIRE_FALSE(spawns.empty());

    for (const auto& s : spawns) {
        float expectedX = grid.startX + (float)s.column * grid.spacingX;
        float expectedY = grid.startY + (float)s.row * grid.spacingY;
        REQUIRE(s.x == Approx(expectedX));
        REQUIRE(s.y == Approx(expectedY));
    }
}

TEST_CASE("WaveGenerator::Generate: grid suy bien (rows/cols = 0) tra ve rong, khong crash", "[wave_generator]") {
    LevelGridConfig grid;
    grid.rows = 0;
    grid.cols = 0;
    auto spawns = WaveGenerator::Generate(1, grid);
    REQUIRE(spawns.empty());
}
