#include "thirdparty/catch.hpp"
#include "spatial_grid.h"
#include <algorithm>

static bool Contains(const std::vector<int>& v, int x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}

TEST_CASE("SpatialGrid Insert/QueryIndices tim dung muc tieu trong cung 1 o", "[spatial_grid]") {
    SpatialGrid grid(800.0f, 600.0f, 80.0f, /*maxEnemies=*/10, /*maxEntries=*/40);
    grid.Insert(0, { 10.0f, 10.0f, 20.0f, 20.0f }); // Nam gon trong o (0,0)

    std::vector<int> result;
    grid.QueryIndices({ 15.0f, 15.0f, 5.0f, 5.0f }, result); // Chong lan
    REQUIRE(Contains(result, 0));
}

TEST_CASE("SpatialGrid khong tra ve muc tieu o xa (khac o, khong lien quan)", "[spatial_grid]") {
    SpatialGrid grid(800.0f, 600.0f, 80.0f, 10, 40);
    grid.Insert(0, { 10.0f, 10.0f, 20.0f, 20.0f }); // O (0,0)

    std::vector<int> result;
    grid.QueryIndices({ 700.0f, 500.0f, 10.0f, 10.0f }, result); // O rat xa (9,6)
    REQUIRE_FALSE(Contains(result, 0));
    REQUIRE(result.empty());
}

TEST_CASE("SpatialGrid: thuc the vat qua nhieu o van duoc tim thay tu MOI o no phu toi", "[spatial_grid]") {
    SpatialGrid grid(800.0f, 600.0f, 80.0f, 10, 40);
    // cellSize=80: rect {70,70,40,40} phu tu (70,70) den (110,110) -> vat qua ca 4 o
    // (col 0-1, row 0-1).
    grid.Insert(0, { 70.0f, 70.0f, 40.0f, 40.0f });

    std::vector<int> result;

    // Query o goc PHAI-DUOI cua vung phu (o (1,1), khong giao voi o (0,0)) - van phai
    // tim thay vi entity da duoc dang ky rieng vao o nay.
    grid.QueryIndices({ 100.0f, 100.0f, 5.0f, 5.0f }, result);
    REQUIRE(Contains(result, 0));

    // Query o goc TRAI-TREN (o (0,0)) - cung phai tim thay.
    grid.QueryIndices({ 72.0f, 72.0f, 2.0f, 2.0f }, result);
    REQUIRE(Contains(result, 0));
}

TEST_CASE("SpatialGrid khu trung lap: 1 entity chi xuat hien 1 lan trong ket qua du query phu nhieu o cua no", "[spatial_grid]") {
    SpatialGrid grid(800.0f, 600.0f, 80.0f, 10, 40);
    grid.Insert(0, { 70.0f, 70.0f, 40.0f, 40.0f }); // Vat qua 4 o nhu tren

    std::vector<int> result;
    // Query 1 rect lon bao trum CA 4 o ma entity 0 dang ky vao - phai chi thay index 0
    // DUNG 1 LAN, khong lap lai 4 lan (1 cho moi o).
    grid.QueryIndices({ 60.0f, 60.0f, 60.0f, 60.0f }, result);

    int count = (int)std::count(result.begin(), result.end(), 0);
    REQUIRE(count == 1);
}

TEST_CASE("SpatialGrid::Clear() xoa sach dang ky cu, khong con tim thay o frame sau", "[spatial_grid]") {
    SpatialGrid grid(800.0f, 600.0f, 80.0f, 10, 40);
    grid.Insert(0, { 10.0f, 10.0f, 20.0f, 20.0f });

    std::vector<int> before;
    grid.QueryIndices({ 15.0f, 15.0f, 5.0f, 5.0f }, before);
    REQUIRE(Contains(before, 0));

    grid.Clear(); // Mo phong dau frame moi - CHUA Insert lai gi ca

    std::vector<int> after;
    grid.QueryIndices({ 15.0f, 15.0f, 5.0f, 5.0f }, after);
    REQUIRE(after.empty());
}

TEST_CASE("SpatialGrid phan biet duoc nhieu entity khac nhau trong cung 1 o", "[spatial_grid]") {
    SpatialGrid grid(800.0f, 600.0f, 80.0f, 10, 40);
    grid.Insert(1, { 5.0f, 5.0f, 10.0f, 10.0f });
    grid.Insert(2, { 20.0f, 20.0f, 10.0f, 10.0f });
    grid.Insert(3, { 40.0f, 40.0f, 10.0f, 10.0f });

    std::vector<int> result;
    grid.QueryIndices({ 0.0f, 0.0f, 79.0f, 79.0f }, result); // Ca 3 deu trong o (0,0)
    REQUIRE(result.size() == 3);
    REQUIRE(Contains(result, 1));
    REQUIRE(Contains(result, 2));
    REQUIRE(Contains(result, 3));
}
