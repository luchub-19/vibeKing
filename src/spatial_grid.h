#pragma once
#include "raylib.h"
#include <vector>
#include <algorithm>

// ==========================================
// SPATIAL GRID
// Băm màn hình thành các ô vuông cố định. Mỗi frame, băm vị trí các enemy đang sống
// vào ô tương ứng; khi cần kiểm tra va chạm cho 1 viên đạn, chỉ duyệt index enemy nằm
// trong (các) ô mà đạn phủ tới, thay vì duyệt toàn bộ enemy. Độ phức tạp trung bình
// giảm từ O(N_bullet * M_enemy) xuống gần O(N_bullet * số_enemy_trong_1_ô_lân_cận).
// ==========================================
class SpatialGrid {
private:
    float cellSize;
    int colsCount;
    int rowsCount;
    std::vector<std::vector<int>> cells; // cells[row * colsCount + col] = danh sách index enemy

    // Dấu vết chống trùng lặp khi 1 query phủ nhiều ô (dùng generation-stamp thay vì
    // clear() 1 vector mới mỗi lần query, để tránh cấp phát bộ nhớ liên tục).
    mutable std::vector<int> visitedGen;
    mutable int currentGen = 0;

    int ClampCol(int c) const { return std::clamp(c, 0, colsCount - 1); }
    int ClampRow(int r) const { return std::clamp(r, 0, rowsCount - 1); }

public:
    SpatialGrid(float screenW, float screenH, float cellSizeIn)
        : cellSize(cellSizeIn),
          colsCount(std::max(1, (int)((screenW / cellSizeIn) + 1))),
          rowsCount(std::max(1, (int)((screenH / cellSizeIn) + 1))) {
        cells.resize((size_t)colsCount * rowsCount);
    }

    void Clear() {
        for (auto& cell : cells) cell.clear();
    }

    // Đăng ký 1 enemy (qua index trong mảng enemies gốc) vào mọi ô mà bounding box
    // của nó phủ tới - enemy nằm vắt qua biên ô vẫn được tìm thấy đúng.
    void Insert(int enemyIndex, Rectangle rect) {
        int minCol = ClampCol((int)(rect.x / cellSize));
        int maxCol = ClampCol((int)((rect.x + rect.width) / cellSize));
        int minRow = ClampRow((int)(rect.y / cellSize));
        int maxRow = ClampRow((int)((rect.y + rect.height) / cellSize));

        for (int r = minRow; r <= maxRow; r++) {
            for (int c = minCol; c <= maxCol; c++) {
                cells[(size_t)r * colsCount + c].push_back(enemyIndex);
            }
        }
    }

    // Trả về (qua outIndices) danh sách index enemy tiềm năng va chạm với `rect` (đạn),
    // đã khử trùng lặp giữa các ô. Đây chỉ là broad-phase - vẫn cần CheckCollisionRecs()
    // chính xác ở bước sau.
    void QueryIndices(Rectangle rect, std::vector<int>& outIndices) const {
        outIndices.clear();
        currentGen++;

        int minCol = ClampCol((int)(rect.x / cellSize));
        int maxCol = ClampCol((int)((rect.x + rect.width) / cellSize));
        int minRow = ClampRow((int)(rect.y / cellSize));
        int maxRow = ClampRow((int)((rect.y + rect.height) / cellSize));

        for (int r = minRow; r <= maxRow; r++) {
            for (int c = minCol; c <= maxCol; c++) {
                for (int idx : cells[(size_t)r * colsCount + c]) {
                    if (idx < (int)visitedGen.size() && visitedGen[idx] == currentGen) continue;
                    if (idx >= (int)visitedGen.size()) visitedGen.resize(idx + 1, 0);
                    visitedGen[idx] = currentGen;
                    outIndices.push_back(idx);
                }
            }
        }
    }
};
