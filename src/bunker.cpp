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
            DamageVoxel(c, r);
        }
    }
    for (int r = minRow; r <= maxRow; r++) {
        for (int c = minCol; c <= maxCol; c++) {
            DamageVoxel(c, r);
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
    // voxel NGAU NHIEN trong so cac o dang bi khoet DO DAN BAN. Truoc day: random 1
    // index tren TOAN BO voxels roi thu lai (vong lap "spin" while(restored<N &&
    // attempts<totalVoxels)) - phi thuoc so lan thu, cang te khi bunker gan hoi phuc
    // het (it o hu hai -> ti le trung ngau nhien rat thap, gan nhu duyet het mang moi
    // tick). Gio: damagedVoxels la danh sach CHI CHUA cac o dang hu hai, nen chi can
    // bat 1 vi tri ngau nhien TRONG CHINH DANH SACH DO (luon trung ngay lan dau) roi
    // swap-and-pop O(1) - tong chi phi dung O(BUNKER_REGEN_PER_TICK), khong phu thuoc
    // kich thuoc bunker hay so o da hoi phuc truoc do.
    regenTimer += dt;
    if (regenTimer < Config::BUNKER_REGEN_INTERVAL) return;
    regenTimer = 0.0f;

    int toRestore = Config::BUNKER_REGEN_PER_TICK;
    if (toRestore > (int)damagedVoxels.size()) toRestore = (int)damagedVoxels.size();

    for (int n = 0; n < toRestore; n++) {
        int pick = GetRandomValue(0, (int)damagedVoxels.size() - 1);
        int idx = damagedVoxels[pick];
        // Swap-and-pop: ghi de vi tri vua bot boi phan tu cuoi roi cat bot mang - khong
        // can dich chuyen (shift) cac phan tu con lai, O(1) moi lan hoi phuc.
        damagedVoxels[pick] = damagedVoxels.back();
        damagedVoxels.pop_back();
        voxels[idx] = 1;
    }
}
