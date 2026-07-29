#pragma once
#include "raylib.h"
#include <cstddef>
#include <cmath>
#include "config.h"

// ==========================================
// ENEMY - XÓA ĐA HÌNH (DE-VIRTUALIZATION)
// Trước đây Enemy là 1 class ảo, GameManager giữ std::vector<std::unique_ptr<Enemy>>.
// Điều này khiến 3 vấn đề: (1) mỗi Enemy nằm rải rác trên heap (con trỏ) -> Cache Miss
// khi lặp; (2) vtable indirection mỗi lần gọi Update()/Draw(); (3) không tận dụng được
// SIMD/prefetch vì kiểu dữ liệu không đồng nhất trong 1 mảng.
//
// Giờ mỗi loại địch là 1 struct dữ liệu thuần (POD, không hàm ảo), sống trong 1 mảng
// tĩnh liền khối riêng (xem EnemyPool bên dưới trong game_manager.h) - lặp qua BasicEnemy
// chỉ đụng vào dữ liệu BasicEnemy, không xen kẽ Tanky/Zigzag -> cache-friendly tuyệt đối.
// Hành vi riêng (Update, điểm số) được xử lý trực tiếp bằng code cụ thể trong
// GameManager thay vì virtual dispatch.
// ==========================================

struct BasicEnemy {
    Rectangle rect;
    Color color;
    int column = 0; // Cột trong đội hình lúc spawn - dùng cho AI "line of sight"
    // Không cần hp: Basic luôn chết sau đúng 1 đòn.

    static constexpr int SCORE_VALUE = 10;
};

struct TankyEnemy {
    static constexpr int HP = 3;
    static constexpr int SCORE_VALUE = 30;

    Rectangle rect;
    Color color;
    int column = 0;
    int hp = HP;
};

struct ZigzagEnemy {
    Rectangle rect;
    Color color;
    int column = 0;
    float timer = 0.0f;
    float lastOffset = 0.0f;
    // Không cần hp: Zigzag luôn chết sau đúng 1 đòn.

    static constexpr float FREQUENCY = 5.0f;   // rad/s
    static constexpr float AMPLITUDE = 18.0f;  // px
    static constexpr int SCORE_VALUE = 20;

    // Dao động ngang quanh vị trí đội hình bằng sóng sin, cộng dồn delta (không phải
    // gán tuyệt đối) để không phá vỡ logic di chuyển đội hình (MoveX của GameManager
    // vẫn áp dụng bình thường lên rect.x).
    void Update(float dt) {
        timer += dt;
        float newOffset = sinf(timer * FREQUENCY) * AMPLITUDE;
        rect.x += (newOffset - lastOffset); // Chỉ cộng phần thay đổi -> không trôi dạt tích lũy
        lastOffset = newOffset;
    }
};

// ==========================================
// ENEMY POOL - SWAP-AND-POP, KHÔNG CÓ CỜ active/ZOMBIE
// Mảng tĩnh cấp phát 1 lần trên stack (như BulletPool/ParticlePool). Không có field
// "active" nào trên từng phần tử - biên giới [0, count) LÀ định nghĩa duy nhất của
// "còn sống". Destroy(i) đá văng phần tử chết ngay lập tức bằng cách ghi đè nó bởi
// phần tử cuối cùng rồi rút gọn count - không để lại xác chết mục nát cho vòng lặp
// Update/Draw phải bước qua mỗi frame.
// ==========================================
template <typename T, size_t Capacity>
class EnemyPool {
private:
    T items[Capacity];
    size_t count = 0;

public:
    void Clear() { count = 0; }

    // Trả về false nếu pool đã đầy (không xảy ra trong thực tế vì Capacity được tính
    // đúng theo giới hạn lưới tối đa - nhưng vẫn chặn tràn mảng để an toàn).
    bool Spawn(const T& item) {
        if (count >= Capacity) return false;
        items[count++] = item;
        return true;
    }

    // Swap-and-pop: ghi đè phần tử tại index bởi phần tử cuối cùng rồi rút gọn count.
    // O(1), không dịch chuyển phần còn lại của mảng.
    void Destroy(size_t index) {
        if (index >= count) return;
        count--;
        items[index] = items[count];
    }

    size_t Size() const { return count; }
    static constexpr size_t MaxCapacity() { return Capacity; }

    T& operator[](size_t index) { return items[index]; }
    const T& operator[](size_t index) const { return items[index]; }
};

inline float EnemyBottom(const Rectangle& r) { return r.y + r.height; }
inline float EnemyBottomY(const Rectangle& r) { return r.y + r.height - 5.0f; }
inline float EnemyCenterX(const Rectangle& r) { return r.x + r.width / 2.0f; }
inline Vector2 EnemyCenter(const Rectangle& r) { return { r.x + r.width / 2.0f, r.y + r.height / 2.0f }; }
