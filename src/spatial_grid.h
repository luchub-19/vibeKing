#pragma once
#include "raylib.h"
#include <vector>
#include <algorithm>

// ==========================================
// SPATIAL GRID - FLAT ARRAY + LINKED LIST (KHÔNG CÒN MẢNG LỒNG MẢNG)
// Trước đây mỗi ô là 1 std::vector<int> riêng (cells: vector<vector<int>>) -> mỗi ô là
// 1 lần cấp phát heap rời rạc, phân mảnh bộ nhớ, Cache Miss liên tục khi duyệt. Giờ thay
// bằng kiến trúc bucket linked-list kinh điển trên 2 mảng 1 chiều TĨNH:
//   - `head[cell]`  : index của entry ĐẦU TIÊN trong ô đó (hoặc -1 nếu ô trống)
//   - `next[entry]` : index của entry TIẾP THEO cùng ô (hoặc -1 nếu là entry cuối)
//   - `value[entry]`: dữ liệu thật sự (index enemy trong pool tương ứng)
// Toàn bộ 3 mảng cấp phát ĐÚNG 1 LẦN lúc khởi tạo theo dung lượng tối đa, Clear() mỗi
// frame chỉ là ghi đè head về -1 (O(số ô), không giải phóng/cấp phát lại gì cả).
//
// Mỗi loại địch (Basic/Tanky/Zigzag) dùng 1 SpatialGrid RIÊNG, khớp với kiến trúc "3
// Pool tĩnh tách biệt" - value lưu trong grid luôn là index thuần trong đúng 1 pool đó,
// không cần đóng gói (pack) thêm loại địch vào cùng 1 số nguyên.
// ==========================================
class SpatialGrid {
private:
    float cellSize;
    int colsCount;
    int rowsCount;

    std::vector<int> head;   // size = colsCount * rowsCount, head[cellIdx] = entry đầu tiên
    std::vector<int> value;  // size = maxEntries, value[entryIdx] = index enemy được Insert
    std::vector<int> next;   // size = maxEntries, next[entryIdx] = entry tiếp theo cùng ô
    int entryCount = 0;
    int maxEntries;

    // Dấu vết chống trùng lặp khi 1 query phủ nhiều ô, dùng generation-stamp thay vì
    // clear() lại mỗi lần query. Kích thước CỐ ĐỊNH = dung lượng tối đa của pool tương
    // ứng (truyền vào constructor) - không bao giờ resize() ngầm bên trong QueryIndices
    // như phiên bản cũ (vốn gọi visitedGen.resize(idx + 1, 0) liên tục mỗi lần gặp index
    // mới, rất tốn kém vì phải chạy lại mỗi frame).
    mutable std::vector<int> visitedGen;
    mutable int currentGen = 0;

    int ClampCol(int c) const { return std::clamp(c, 0, colsCount - 1); }
    int ClampRow(int r) const { return std::clamp(r, 0, rowsCount - 1); }

    // Trừ epsilon ở cạnh xa (phải/dưới) trước khi chia cho cellSize: nếu rìa vật thể
    // rơi ĐÚNG lên biên ô (vd x + width == cellSize * k), phép chia không trừ epsilon
    // sẽ cho ra chỉ số ô kế tiếp dù vật thể không hề phủ lên dù chỉ 1 pixel của ô đó ->
    // đăng ký dư thừa (và có thể query dư thừa) sang ô bên cạnh không liên quan.
    static constexpr float EDGE_EPSILON = 0.001f;

public:
    // maxEnemiesIn: dung lượng tối đa CHÍNH XÁC của pool enemy tương ứng (Basic/Tanky/
    // Zigzag) - dùng để pre-size visitedGen đúng bằng lượng Entity tối đa ngay từ đầu.
    // maxEntriesIn: tổng số lượt (entity x ô nó phủ tới) tối đa có thể xảy ra trong 1
    // frame, dùng cho mảng entry (value/next). Người gọi truyền vào = maxEnemiesIn * 4
    // (1 entity chỉ có thể vắt tối đa qua 2x2 = 4 ô khi kích thước entity nhỏ hơn 1 ô,
    // đúng trường hợp ở đây).
    SpatialGrid(float screenW, float screenH, float cellSizeIn, int maxEnemiesIn, int maxEntriesIn)
        : cellSize(cellSizeIn),
          colsCount(std::max(1, (int)((screenW / cellSizeIn) + 1))),
          rowsCount(std::max(1, (int)((screenH / cellSizeIn) + 1))),
          maxEntries(maxEntriesIn) {
        head.assign((size_t)colsCount * rowsCount, -1);
        value.resize((size_t)maxEntries);
        next.resize((size_t)maxEntries);
        visitedGen.assign((size_t)maxEnemiesIn, 0); // Pre-size 1 lần duy nhất ngay từ đầu, đúng bằng max entity
    }

    void Clear() {
        std::fill(head.begin(), head.end(), -1);
        entryCount = 0;
    }

    // Đăng ký 1 enemy (qua index trong pool gốc) vào mọi ô mà bounding box của nó phủ
    // tới - enemy nằm vắt qua biên ô vẫn được tìm thấy đúng.
    void Insert(int enemyIndex, Rectangle rect) {
        int minCol = ClampCol((int)(rect.x / cellSize));
        int maxCol = ClampCol((int)((rect.x + rect.width - EDGE_EPSILON) / cellSize));
        int minRow = ClampRow((int)(rect.y / cellSize));
        int maxRow = ClampRow((int)((rect.y + rect.height - EDGE_EPSILON) / cellSize));

        for (int r = minRow; r <= maxRow; r++) {
            for (int c = minCol; c <= maxCol; c++) {
                if (entryCount >= maxEntries) return; // An toàn: không tràn mảng tĩnh
                int cellIdx = r * colsCount + c;
                int entry = entryCount++;
                value[entry] = enemyIndex;
                next[entry] = head[cellIdx];
                head[cellIdx] = entry;
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
        int maxCol = ClampCol((int)((rect.x + rect.width - EDGE_EPSILON) / cellSize));
        int minRow = ClampRow((int)(rect.y / cellSize));
        int maxRow = ClampRow((int)((rect.y + rect.height - EDGE_EPSILON) / cellSize));

        for (int r = minRow; r <= maxRow; r++) {
            for (int c = minCol; c <= maxCol; c++) {
                for (int e = head[(size_t)r * colsCount + c]; e != -1; e = next[e]) {
                    int idx = value[e];
                    if (visitedGen[idx] == currentGen) continue;
                    visitedGen[idx] = currentGen;
                    outIndices.push_back(idx);
                }
            }
        }
    }
};
