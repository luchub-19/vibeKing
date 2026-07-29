#pragma once
#include "raylib.h"
#include <vector>
#include <cstdint>

// ==========================================
// BUNKER (LÁ CHẮN VOXEL)
// Thay vì 1 hình chữ nhật nguyên khối biến mất khi trúng đủ đạn, bunker là 1 lưới
// voxel nhỏ (COLS x ROWS ô vuông VOXEL_SIZE px). Mỗi viên đạn trúng chỉ khoét đúng
// vùng nó chạm vào (+ 1 vòng lân cận cho cảm giác "nổ" tự nhiên) - lá chắn vỡ vụn
// dần theo đúng hình dạng va chạm thay vì mất nguyên khối.
// ==========================================
class Bunker {
private:
    static constexpr int COLS = 22;
    static constexpr int ROWS = 14;
    static constexpr float VOXEL_SIZE = 4.0f;

    float originX;
    float originY;
    // std::vector<bool> là 1 bitset đặc biệt hoá (mỗi phần tử chỉ chiếm 1 bit), không
    // phải mảng bool thật - mọi lần đọc/ghi đều phải qua phép dịch bit (bit-shifting)
    // + mask ẩn ngầm để trích xuất đúng bit trong 1 word, và không trả về bool& thật
    // (proxy object) nên trình biên dịch khó tối ưu/vector hoá. Đổi sang uint8_t: mỗi
    // voxel chiếm đúng 1 byte thật, đọc/ghi trực tiếp không cần giải mã bit.
    std::vector<uint8_t> voxels; // 1 = voxel còn nguyên, 0 = đã bị khoét. Index: row * COLS + col
    Color color;

    bool InBounds(int col, int row) const {
        return col >= 0 && col < COLS && row >= 0 && row < ROWS;
    }

    bool IsSolid(int col, int row) const {
        return InBounds(col, row) && voxels[(size_t)row * COLS + col] != 0;
    }

    void CarveVoxel(int col, int row) {
        if (InBounds(col, row)) voxels[(size_t)row * COLS + col] = 0;
    }

public:
    Bunker(float x, float y, Color col);

    static float DefaultWidth() { return COLS * VOXEL_SIZE; }
    static float DefaultHeight() { return ROWS * VOXEL_SIZE; }

    float GetWidth() const { return COLS * VOXEL_SIZE; }
    float GetHeight() const { return ROWS * VOXEL_SIZE; }
    Rectangle GetBounds() const { return { originX, originY, GetWidth(), GetHeight() }; }

    void Draw() const;

    // Kiểm tra + xử lý va chạm với hình chữ nhật của 1 viên đạn. Trả về true nếu có
    // va chạm thật sự (còn voxel nào đó nguyên vẹn trong vùng đạn chạm tới) - khi đó
    // GameManager nên hủy viên đạn. Trả về false nếu đạn bay qua đúng 1 lỗ hổng đã bị
    // khoét từ trước (không còn gì để chặn).
    bool HandleBulletHit(Rectangle bulletRect);

    bool IsFullyDestroyed() const;
};
