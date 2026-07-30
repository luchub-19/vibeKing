#include "bunker.h"
#include "config.h"
#include <algorithm>
#include <cmath>

Bunker::Bunker(float x, float y, Color col)
    : originX(x), originY(y), baseX(x), patrolPhase((float)GetRandomValue(0, 628) / 100.0f), color(col) {
    voxels.assign((size_t)COLS * ROWS, 1);

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

    // Chụp lại trạng thái NGAY SAU các nhát khoét có chủ đích ở trên - đây là "mức trần"
    // cho regen ở Update(): chỉ hồi phục voxel nào ==1 ở đây mà hiện đang ==0 (tức là hư
    // hại DO ĐẠN BẮN), không bao giờ lấp lại vòm cổng/góc bo tròn.
    originalVoxels = voxels;
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
    for (uint8_t v : voxels) {
        if (v != 0) return false;
    }
    return true;
}

void Bunker::Update(float dt) {
    // PATROL: originX dao dong quanh baseX theo sin - lien tuc moi frame (khong can
    // timer rieng), phase khac nhau giua cac bunker (random luc khoi tao) nen chung
    // khong dong bo mot cach may moc.
    patrolPhase += Config::BUNKER_PATROL_SPEED * dt;
    originX = baseX + sinf(patrolPhase) * Config::BUNKER_PATROL_AMPLITUDE;

    // REGEN: cu moi BUNKER_REGEN_INTERVAL giay, hoi phuc toi da BUNKER_REGEN_PER_TICK
    // voxel NGAU NHIEN trong so cac o dang bi khoet DO DAN BAN (voxels==0 nhung
    // originalVoxels==1) - khong bao gio dung tram (voxels.size()) lam gioi han so lan
    // thu random de tranh vong lap vo han neu khong con gi de hoi phuc.
    regenTimer += dt;
    if (regenTimer < Config::BUNKER_REGEN_INTERVAL) return;
    regenTimer = 0.0f;

    int restored = 0;
    int attempts = 0;
    int totalVoxels = (int)voxels.size();
    while (restored < Config::BUNKER_REGEN_PER_TICK && attempts < totalVoxels) {
        int idx = GetRandomValue(0, totalVoxels - 1);
        if (voxels[idx] == 0 && originalVoxels[idx] == 1) {
            voxels[idx] = 1;
            restored++;
        }
        attempts++;
    }
}
