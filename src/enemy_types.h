#pragma once
#include "raylib.h"
#include <cstddef>
#include <cstdint>
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

    // `static inline` (KHONG constexpr) - la DU LIEU CAN BANG, bi Config::LoadBalance()
    // ghi de tu balance.json (muc "enemy_stats.basic_score") giong het cac hang so
    // trong Config namespace - xem config.h de biet ly do/quy uoc chung.
    static inline int SCORE_VALUE = 10;
};

struct TankyEnemy {
    static inline int HP = 3;
    static inline int SCORE_VALUE = 30;

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
    // Khong can hp: Zigzag luon chet sau dung 1 don.

    static inline float FREQUENCY = 5.0f;   // rad/s
    static inline float AMPLITUDE = 18.0f;  // px
    static inline int SCORE_VALUE = 20;

    // KHONG con ham Update() o day - CHUAN HOA ECS: struct nay chi la DU LIEU THUAN
    // (component), khong tu mang theo hanh vi. Cong thuc dao dong sin (dung `timer` +
    // `lastOffset` ben tren) da chuyen sang PhysicsSystem::UpdateEnemies() - noi DUY
    // NHAT duyet qua va thay doi du lieu Zigzag moi frame. Ly do: 1 struct vua la du
    // lieu vua tu Update() minh la mo hinh OOP lai voi kieu du liet cache-friendly ma
    // file nay dang theo (xem chu thich dau file) - system nam ngoai moi doc/ghi du
    // lieu giup dat toan bo "khi nao ai thay doi cai gi" o 1 noi (PhysicsSystem), thay
    // vi rai rac giua goi Update() tren tung the hien va vong lap ben ngoai.
};

// ==========================================
// KAMIKAZE - Pool + SpatialGrid rieng (xem GameManager::kamikazeEnemies/kamikazeGrid),
// KHONG tham gia UpdateEnemies() (hitEdge, enemyDirection...) nen viec them loai dich
// nay khong dung gi den logic kiem tra bien cua luoi doi hinh.
//
// HOAN THIEN (kieu Galaga): GameManager::SpawnKamikaze() GIO co doc/xoa 1 phan tu
// NGAU NHIEN tu basicEnemies/tankyEnemies/zigzagEnemies khi doi hinh con quan (thay vi
// luon spawn tu ngoai man hinh doc lap hoan toan nhu truoc) - "boc" 1 con ra khoi doi
// hinh de no lao xuong tu DUNG vi tri hien tai cua no, dung cam giac "1 con tach doi
// hinh lao xuong" thay vi xuat hien tu hu vo. Van AN TOAN voi UpdateEnemies() vi chi
// dung LAI dung EnemyPool::Destroy() - cung 1 thao tac swap-and-pop xay ra khi player
// ban trung 1 dich (UpdateEnemies() doc lai Size() moi frame, khong biet/khong can biet
// TAI SAO 1 phan tu bien mat). Chi khi doi hinh DA TRONG (boss wave, hoac vua don sach)
// moi quay lai spawn tu ngoai man hinh nhu truoc - xem SpawnKamikaze().
// ==========================================
struct KamikazeEnemy {
    Rectangle rect;
    Color color;
    Vector2 vel; // Vector lao thẳng, tính 1 lần lúc spawn (nhắm vào vị trí player lúc đó)

    static inline int SCORE_VALUE = 150;
};

// ==========================================
// BOSS - 1 thực thể duy nhất (không cần EnemyPool), rect lớn hơn hẳn 1 ô SpatialGrid
// (80px) nên khi Insert() vào bossGrid sẽ tự động đăng ký vào NHIỀU ô cùng lúc (xem
// SpatialGrid::Insert - đã hỗ trợ sẵn multi-cell từ trước, không cần sửa gì thêm).
// 3 giai đoạn suy ra trực tiếp từ % HP còn lại (không lưu "stage" rời rạc riêng - tránh
// state có thể lệch khỏi hp thật).
//
// 3 LOAI BOSS (BossType) - GameManager::SpawnBoss() xoay vong qua ca 3 moi lan gap Boss
// (xem cong thuc trong SpawnBoss()), moi loai mot kieu di chuyen/tan cong RIENG xu ly
// trong PhysicsSystem::UpdateBoss() - tranh cam giac "chi la 1 con Boss ngay cang nhieu
// mau" lap lai moi Config::BOSS_WAVE_INTERVAL wave:
//   - Vanguard: pace het chieu rong man hinh, nhip ban tang theo stage (hanh vi GOC).
//   - Sentinel: gan nhu dung yen (chi lac nhe quanh diem spawn), dinh ky bat khien tam
//     bat kha xam pham - ep nguoi choi cho dung nhip thay vi giu nut ban lien tuc.
//   - Swarmer: lac nhanh/rong that thuong + dinh ky trieu hoi Kamikaze tiep vien.
// Van CHI 1 struct du lieu duy nhat (khong tach 3 class/kieu rieng) - dung dung tinh
// than "khong da hinh runtime" cua toan bo file nay; field nao khong dung toi voi 1 loai
// (vd `direction` voi Sentinel/Swarmer) don gian khong duoc doc toi, khong ton kem gi vi
// Boss chi co DUNG 1 the hien tai 1 thoi diem (EnemyPool<Boss,1>).
// ==========================================
enum class BossType : uint8_t { Vanguard, Sentinel, Swarmer };

struct Boss {
    Rectangle rect{};
    int hp = 0;
    int maxHp = 0;
    int direction = 1;
    float fireTimer = 0.0f;

    BossType type = BossType::Vanguard;
    float baseX = 0.0f;      // Vi tri X luc spawn - Sentinel/Swarmer lac QUANH diem nay
                              // thay vi pace het chieu rong man hinh nhu Vanguard.
    float phaseAccum = 0.0f; // Goc dao dong tich luy dung chung cho Sentinel/Swarmer
                              // (bien do/tan so khac nhau - xem Config::BOSS_SENTINEL_SWAY_*
                              // va BOSS_SWARMER_SWAY_*).

    // SENTINEL: dinh ky bat/tat khien tam bat kha xam pham (doc lap voi fireTimer/stage).
    float phaseTimer = 0.0f;
    bool shieldActive = false;

    // SWARMER: dinh ky trieu hoi tiep vien tu pool KamikazeEnemy co san (xem UpdateBoss()).
    float summonTimer = 0.0f;

    // KHONG con ham Stage() o day - cung ly do voi ZigzagEnemy o tren: struct nay CHI
    // la du lieu. Dung BossStage(boss) (free function ben duoi) o bat ky system nao can
    // suy ra giai doan (PhysicsSystem de chon toc do/nhip ban, RenderSystem de chon mau
    // tint) - 1 cong thuc DUY NHAT, khong the lech nhau giua 2 noi goi.
};

// 3 giai doan suy ra truc tiep tu % HP con lai (khong luu "stage" roi rac rieng - tranh
// state co the lech khoi hp that).
inline int BossStage(const Boss& boss) {
    if (boss.maxHp <= 0) return 1;
    float ratio = (float)boss.hp / (float)boss.maxHp;
    if (ratio > 0.66f) return 1;
    if (ratio > 0.33f) return 2;
    return 3;
}

// Ten hien thi tren HUD (xem RenderSystem::DrawHUD) - 1 noi DUY NHAT anh xa enum sang
// chuoi, tranh switch/case lap lai o nhieu noi neu sau nay can ten Boss o cho khac.
inline const char* BossTypeName(BossType type) {
    switch (type) {
        case BossType::Sentinel: return "SENTINEL";
        case BossType::Swarmer:  return "SWARMER";
        default:                 return "VANGUARD";
    }
}

// ==========================================
// BOSS TYPE DESCRIPTOR (B4) - DATA-DRIVEN: thay the switch(boss.type)/if(boss.type==...)
// rai rac trong PhysicsSystem::UpdateBoss() (truoc day) bang 1 BANG duy nhat, dung KHUON
// voi Config::g_difficultyTable/GetDifficultyStats (config.h) - g_bossTypeDescriptors[]
// index THANG bang (int)BossType, KHONG switch/case. Muon them 1 BossType moi TAI SU
// DUNG 1 trong 2 kieu di chuyen/co che khien/trieu hoi da co (vd Vanguard-pace + co
// khien) chi can 1 dong MOI trong bang duoi day - KHONG dung den nhanh code nao trong
// UpdateBoss(). Chi that su can sua UpdateBoss() khi co 1 CO CHE HOAN TOAN MOI (vd 1 kieu
// di chuyen thu 3 ngoai Pace/Sway).
//
// Cac con tro (KHONG PHAI gia tri sao chep) TRO THANG toi bien Config::BOSS_* co san (vd
// &Config::BOSS_SENTINEL_SWAY_AMPLITUDE) - khong nhan doi so lieu can bang: JSON
// (assets/balance.json) ghi de Config::BOSS_* thi descriptor TU DONG "thay" theo vi chi
// giu DIA CHI, khong giu ban sao. Vi vay file nay KHONG can sua them config.h/config.cpp
// cho B4 - moi so lieu can bang Sentinel/Swarmer da co san tu truoc (xem config.cpp::
// LoadBoss). Lay dia chi 1 bien `inline` hang so - hop le tai thoi diem bien dich/lien
// ket (constant initialization), khong co rui ro thu tu khoi tao giua 2 file.
// ==========================================
enum class BossMovementPattern : uint8_t { Pace, Sway };

struct BossTypeDescriptor {
    BossMovementPattern movement = BossMovementPattern::Pace;

    // SWAY - CHI duoc doc khi movement == Sway (Pace bo qua hoan toan, de nullptr an toan).
    const float* swayAmplitude = nullptr;
    const float* swayFrequency = nullptr;

    // KHIEN TAM dinh ky (rieng Sentinel truoc day) - hasShieldMechanic=false thi 3 con
    // tro duoi khong bao gio duoc doc toi.
    bool hasShieldMechanic = false;
    const float* shieldInterval = nullptr;
    const float* shieldDuration = nullptr;
    const float* shieldFireInterval = nullptr;

    // TRIEU HOI TIEP VIEN dinh ky (rieng Swarmer truoc day).
    bool hasSummonMechanic = false;
    const float* summonInterval = nullptr;
    const int* summonCount = nullptr;
};

// Index THANG bang (int)BossType (Vanguard=0, Sentinel=1, Swarmer=2 - xem enum BossType
// o tren) - dung khuon voi Config::g_difficultyTable, KHONG switch/case.
inline BossTypeDescriptor g_bossTypeDescriptors[3] = {
    // Vanguard: pace het chieu rong man hinh, khong khien, khong trieu hoi.
    { BossMovementPattern::Pace, nullptr, nullptr, false, nullptr, nullptr, nullptr, false, nullptr, nullptr },
    // Sentinel: lac quanh baseX + khien tam dinh ky.
    { BossMovementPattern::Sway, &Config::BOSS_SENTINEL_SWAY_AMPLITUDE, &Config::BOSS_SENTINEL_SWAY_FREQUENCY,
      true, &Config::BOSS_SENTINEL_SHIELD_INTERVAL, &Config::BOSS_SENTINEL_SHIELD_DURATION, &Config::BOSS_SENTINEL_SHIELD_FIRE_INTERVAL,
      false, nullptr, nullptr },
    // Swarmer: lac quanh baseX (bien do/tan so khac Sentinel) + trieu hoi Kamikaze dinh ky.
    { BossMovementPattern::Sway, &Config::BOSS_SWARMER_SWAY_AMPLITUDE, &Config::BOSS_SWARMER_SWAY_FREQUENCY,
      false, nullptr, nullptr, nullptr,
      true, &Config::BOSS_SWARMER_SUMMON_INTERVAL, &Config::BOSS_SWARMER_SUMMON_COUNT },
};

inline const BossTypeDescriptor& GetBossTypeDescriptor(BossType type) {
    int idx = (int)type;
    if (idx < 0 || idx > 2) idx = 0;
    return g_bossTypeDescriptors[idx];
}

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
