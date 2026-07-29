#include "bunker.h"
#include <algorithm>
#include <cmath>

Bunker::Bunker(float x, float y, Color col) : originX(x), originY(y), color(col) {
    voxels.assign((size_t)COLS * ROWS, true);

    // Khoét sẵn 1 vòm cổng ở đáy - chữ U ngược - giống silhouette bunker cổ điển của
    // Space Invaders thay vì 1 khối chữ nhật trơn.
    int archWidth = 6;
    int archStartCol = (COLS - archWidth) / 2;
    int archHeight = 5;
    for (int r = ROWS - archHeight; r < ROWS; r++) {
        for (int c = archStartCol; c < archStartCol + archWidth; c++) {
            CarveVoxel(c, r);
        }
    }

    // Bo góc trên cho mềm mại hơn thay vì góc vuông cứng
    CarveVoxel(0, 0);
    CarveVoxel(COLS - 1, 0);
}

void Bunker::Draw() const {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (!IsSolid(c, r)) continue;
            DrawRectangle(
                (int)(originX + c * VOXEL_SIZE),
                (int)(originY + r * VOXEL_SIZE),
                (int)VOXEL_SIZE, (int)VOXEL_SIZE,
                color
            );
        }
    }
}

bool Bunker::HandleBulletHit(Rectangle bulletRect) {
    Rectangle bounds = GetBounds();
    if (!CheckCollisionRecs(bulletRect, bounds)) return false;

    int minCol = (int)((bulletRect.x - originX) / VOXEL_SIZE);
    int maxCol = (int)((bulletRect.x + bulletRect.width - originX) / VOXEL_SIZE);
    int minRow = (int)((bulletRect.y - originY) / VOXEL_SIZE);
    int maxRow = (int)((bulletRect.y + bulletRect.height - originY) / VOXEL_SIZE);

    minCol = std::clamp(minCol, 0, COLS - 1);
    maxCol = std::clamp(maxCol, 0, COLS - 1);
    minRow = std::clamp(minRow, 0, ROWS - 1);
    maxRow = std::clamp(maxRow, 0, ROWS - 1);

    bool hitSomething = false;
    for (int r = minRow; r <= maxRow; r++) {
        for (int c = minCol; c <= maxCol; c++) {
            if (IsSolid(c, r)) hitSomething = true;
        }
    }
    if (!hitSomething) return false; // Đạn bay qua đúng lỗ hổng trống -> không chặn

    // Khoét vùng đạn chạm tới + 1 vòng lân cận quanh tâm va chạm cho cảm giác mảnh vỡ
    // văng ra thay vì 1 lỗ vuông vức đúng bằng viên đạn.
    int centerCol = (minCol + maxCol) / 2;
    int centerRow = (minRow + maxRow) / 2;
    const int splashRadius = 1;
    for (int r = centerRow - splashRadius; r <= centerRow + splashRadius; r++) {
        for (int c = centerCol - splashRadius; c <= centerCol + splashRadius; c++) {
            CarveVoxel(c, r);
        }
    }
    for (int r = minRow; r <= maxRow; r++) {
        for (int c = minCol; c <= maxCol; c++) {
            CarveVoxel(c, r);
        }
    }

    return true;
}

bool Bunker::IsFullyDestroyed() const {
    for (bool v : voxels) {
        if (v) return false;
    }
    return true;
}
