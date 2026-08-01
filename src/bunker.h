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

    // BUNKER LINH HOẠT:
    // - originalVoxels: ảnh chụp trạng thái NGAY SAU khi khoét vòm cổng/góc bo tròn ban
    //   đầu (thiết kế có chủ đích) - dùng làm "mức trần" cho regen, để cơ chế hồi phục
    //   KHÔNG BAO GIỜ lấp lại những lỗ đó (chúng vốn không phải "hư hại do đạn bắn").
    // - baseX/patrolPhase: originX dao động quanh baseX theo sóng sin thay vì đứng yên
    //   tuyệt đối - patrolPhase lệch pha ngẫu nhiên giữa các bunker để chúng không đung
    //   đưa đồng bộ trông máy móc.
    std::vector<uint8_t> originalVoxels;
    float baseX;
    float patrolPhase;
    float regenTimer = 0.0f;

    // DANH SACH O BI KHOET DO DAN (khong tinh vom cong/goc bo tron thiet ke san): moi
    // phan tu la 1 index (row*COLS+col) dang == 0 nhung originalVoxels tai do == 1.
    // Update() truoc day random 1 index bat ky trong toan bo voxels roi thu lai (vong
    // lap "spin") den khi trung o hu hai - O(so lan thu), toi te dan khi bunker gan
    // hoi phuc het (it o hu hai con lai -> ti le trung ngau nhien rat thap). Duy tri
    // mang nay giup regen chi can bat 1 index NGAU NHIEN TRONG CHINH DANH SACH NAY roi
    // swap-and-pop - luon dung O(1) moi voxel hoi phuc, bat ke bunker con nguyen hay
    // gan nat vun.
    std::vector<int> damagedVoxels;

    Color color;

    bool InBounds(int col, int row) const {
        return col >= 0 && col < COLS && row >= 0 && row < ROWS;
    }

    bool IsSolid(int col, int row) const {
        return InBounds(col, row) && voxels[(size_t)row * COLS + col] != 0;
    }

    // Khoet "tho" luc khoi tao (vom cong, goc bo tron) - khong dua vao damagedVoxels vi
    // day la thiet ke co chu dich, khong phai hu hai do dan ban, va originalVoxels chua
    // duoc chup luc goi ham nay.
    void CarveVoxel(int col, int row) {
        if (InBounds(col, row)) voxels[(size_t)row * COLS + col] = 0;
    }

    // Khoet do TRUNG DAN trong gameplay (dung trong HandleBulletHit): chi carve + ghi
    // nhan vao damagedVoxels neu o do dang con nguyen (voxels==1) VA von di la o "that"
    // (originalVoxels==1) - tu dong bo qua o da bi khoet truoc do (khong ghi trung) va
    // o von la lo thiet ke san (vom cong/goc bo tron, khong bao gio duoc tinh la "hu
    // hai" nen khong duoc dua vao hang cho regen).
    void DamageVoxel(int col, int row) {
        if (!InBounds(col, row)) return;
        size_t idx = (size_t)row * COLS + col;
        if (voxels[idx] == 0 || originalVoxels[idx] == 0) return;
        voxels[idx] = 0;
        damagedVoxels.push_back((int)idx);
    }

public:
    Bunker(float x, float y, Color col);

    static float DefaultWidth() { return COLS * VOXEL_SIZE; }
    static float DefaultHeight() { return ROWS * VOXEL_SIZE; }

    float GetWidth() const { return COLS * VOXEL_SIZE; }
    float GetHeight() const { return ROWS * VOXEL_SIZE; }
    Rectangle GetBounds() const { return { originX, originY, GetWidth(), GetHeight() }; }

    void Draw() const;

    // Goi moi frame: hoi phuc dan 1 so voxel bi khoet (regen) + dao dong ngang nhe
    // (patrol). Xem cai dat trong bunker.cpp.
    void Update(float dt);

    // Kiểm tra + xử lý va chạm với hình chữ nhật của 1 viên đạn. Trả về true nếu có
    // va chạm thật sự (còn voxel nào đó nguyên vẹn trong vùng đạn chạm tới) - khi đó
    // GameManager nên hủy viên đạn. Trả về false nếu đạn bay qua đúng 1 lỗ hổng đã bị
    // khoét từ trước (không còn gì để chặn).
    bool HandleBulletHit(Rectangle bulletRect);

    bool IsFullyDestroyed() const;
};
