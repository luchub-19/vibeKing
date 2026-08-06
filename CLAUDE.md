# CLAUDE.md

Định hướng nhanh cho AI agent (Claude Code hoặc phiên làm việc AI khác) khi làm việc
trong repo này. Đọc file này trước; đào sâu hệ thống cụ thể thì xem ARCHITECTURE.md (sơ
đồ + bản đồ module chi tiết), còn giới thiệu game/cách chơi thì xem README.md.

## Dự án là gì

Hardcore Space Invaders - game bắn súng 2D kiểu Space Invaders/Galaga, C++17 + raylib,
build bằng CMake. Công việc chia theo "Track" (A/B/C...), mỗi Track chia nhỏ thành mục
đánh số kèm ghi chú phụ thuộc/độ ưu tiên - xem yêu cầu gần nhất trong hội thoại hoặc PR
đang mở để biết đang ở Track/mục nào trước khi bắt đầu việc mới, đừng tự đoán.

## Build

raylib 5.5 KHÔNG có sẵn qua apt - phải build từ source (đã kiểm chứng qua CI, xem
.github/workflows/ci.yml):

```bash
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
cmake -S raylib -B raylib/build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF \
      -DCMAKE_INSTALL_PREFIX=<đường-dẫn-cài>
cmake --build raylib/build -j"$(nproc)" && cmake --install raylib/build
```

Cần thêm gói dev X11/GL (Ubuntu): `libxrandr-dev libxinerama-dev libxcursor-dev
libxi-dev libgl1-mesa-dev libglu1-mesa-dev`. Cần GCC >= 11 (std::from_chars<float> -
xem comment đầu CMakeLists.txt, lỗi thiếu bản GCC sẽ khó hiểu nếu không biết trước).

Build chính (sinh ra `space_invaders` và `unit_tests`):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<đường-dẫn-cài-raylib>
cmake --build build -j"$(nproc)"
```

Không set `CMAKE_BUILD_TYPE` = build không tối ưu (~ -O0), phá vỡ tối ưu
SpatialGrid/EnemyPool - luôn chỉ định Release khi cần đo hiệu năng thật.

## Test

```bash
cd build && ctest --output-on-failure
# hoặc lọc theo tag chạy trực tiếp:
./unit_tests "[game_manager]"     # hoặc [physics], [boss], [leaderboard]...
./unit_tests --list-tests         # xem hết test/tag hiện có
```

Catch2 v2 vendor sẵn tại `tests/thirdparty/catch.hpp` (không cần internet lúc build).
Source cho target `unit_tests` liệt kê TƯỜNG MINH trong CMakeLists.txt, không GLOB -
**thêm file `tests/test_*.cpp` mới phải tự thêm dòng tương ứng vào target đó**; nếu
test đụng tới `game_manager.cpp`/`physics_system.cpp` thì cần thêm luôn các .cpp mà
`GameManager::Run()` tham chiếu tới (`render_system`/`audio_system`/`sprites`/
`file_logger`/`level_config` - cả file phải biên dịch được dù test không gọi `Run()`).

Trước khi coi 1 thay đổi là xong: build lại với cảnh báo nghiêm ngặt (CI luôn chạy
bước này, `-Werror` chặn PR nếu có warning mới - hiện codebase đang 0 warning):

```bash
cmake -B build-strict -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Wshadow -Werror" \
      -DCMAKE_PREFIX_PATH=<đường-dẫn-cài-raylib>
cmake --build build-strict -j"$(nproc)"
```

## Nguyên tắc kiến trúc cốt lõi (chi tiết/sơ đồ xem ARCHITECTURE.md)

- **Data-oriented, không đa hình runtime**: mỗi loại địch là 1 struct dữ liệu thuần
  trong `EnemyPool<T,Capacity>` riêng (swap-and-pop, không cờ active/zombie) - đừng
  quay lại `virtual`/`unique_ptr<Enemy>`.
- **GameManager sở hữu dữ liệu, không tự tính/vẽ**: `PhysicsSystem`/`RenderSystem` là
  `friend class` của `GameManager`, đọc/ghi thẳng field private thay vì hàng chục
  getter/setter. Cả 2 đều là hàm `static` nhận `GameManager&` làm tham số đầu.
- **1-nguồn-duy-nhất**: dữ liệu nhiều nơi cùng cần phải đọc từ 1 định nghĩa chung, ví
  dụ `BossStage(boss)` (enemy_types.h) suy giai đoạn boss trực tiếp từ %HP thay vì lưu
  biến `stage` rời rạc dễ lệch; `GetRebindableActions()` (game_manager.h) liệt kê 4
  phím rebind 1 lần cho cả màn KEYBIND lẫn RenderSystem.
- **Event queue tách phát hiện va chạm khỏi hiệu ứng**: `PhysicsSystem::CheckCollisions()`
  chỉ xác định va chạm + cập nhật state tối thiểu, đóng gói hệ quả (particle/âm
  thanh/rung màn hình/điểm/power-up) thành `GameEvent` đẩy vào `pendingEvents`;
  `GameManager::ProcessEvents()` mới thực thi. Nhờ vậy `CheckCollisions()` test được
  độc lập, không dính raylib audio/particle thật.
- **Fade transition 2 pha**: `RequestTransition()` KHÔNG đổi `state` ngay - chỉ đặt
  `pendingState` + bắt đầu `FADE_OUT`; `state` đổi thật bên trong `UpdateTransition()`
  sau đủ `Config::TRANSITION_DURATION` giây, rồi `FADE_IN` trước khi về `NONE`.
- **Config engine vs balance data**: hằng số chia 2 loại - `constexpr` (kỹ thuật, cố
  định, vd `SCREEN_W`) và `inline` (dữ liệu cân bằng - HP/tốc độ/wave pattern... - bị
  `Config::LoadBalance()` ghi đè từ `assets/balance.json` lúc `Run()`). Đừng đổi
  `inline` thành `constexpr` chỉ vì gọn hơn - sẽ làm `balance.json` hết tác dụng.
- **Ghi file atomic + checksum**: `Leaderboard`/`MetaProgress`/`Settings` đều ghi ra
  `<path>.tmp` rồi `rename()`; 2 cái đầu có checksum chống sửa tay (xem
  save_checksum.h). Theo mẫu này nếu thêm hệ thống lưu file mới.

## Quy ước code

- Comment tiếng Việt giải thích LÝ DO, không chỉ mô tả code làm gì. Có dấu lẫn không
  dấu tùy chỗ trong cùng 1 file (không dấu phổ biến hơn ở comment rải rác theo hàm; văn
  bản mở đầu file/section thường có dấu) - giữ đúng phong cách đoạn đang sửa, không áp
  1 chuẩn cứng lên toàn bộ.
- Liệt kê source tường minh trong CMakeLists.txt, không GLOB - nhớ thêm dòng khi thêm
  file .cpp mới (cả target `space_invaders` lẫn `unit_tests`).

## Test: seam riêng cho GameManager/PhysicsSystem

`GameManager`/`PhysicsSystem` hầu hết private/friend, không có getter/setter cho test.
`tests/game_manager_test_access.h` định nghĩa `class GameManagerTestAccess` (friend
riêng của `GameManager`, khai báo trong game_manager.h) làm 1-NGUỒN-DUY-NHẤT cho MỌI
truy cập private từ test - dùng chung bởi `test_game_manager.cpp` và
`test_physics_system.cpp`. Cần truy cập field/hàm private mới cho test? Thêm 1 static
method vào ĐÚNG file này, đừng tự thêm friend/getter rải rác nơi khác.

File test đụng lưu file (Leaderboard/MetaProgress/Settings/Balance) luôn dùng pattern
`TestPath()` + `struct CleanupGuard { ~CleanupGuard() { std::remove(TestPath()); } }`
để không đụng save thật của người chơi và tự dọn dẹp (xem test_leaderboard.cpp làm gốc).
Hằng số cân bằng `inline` (vd `Config::POWERUP_DROP_CHANCE`) ghi đè tạm được ngay trong
test (lưu giá trị gốc, ghi đè, chạy, phục hồi cuối bài) - xem test_boss.cpp.

Đã kiểm chứng bằng probe thật (không phải giả định): gọi thẳng
`UpdatePlaying()`/`CheckCollisions()`/v.v. an toàn HOÀN TOÀN headless, không cần
`InitWindow()`/Xvfb - `IsKeyDown`/`IsGamepadAvailable`/`GetRandomValue` trả về
0/false khi raylib chưa init, và `PlaySound()` trên 1 `Sound{}` chưa từng `LoadSound()`
cũng tự no-op (raylib tự kiểm tra `IsSoundValid()` nội bộ), không crash.

## Trước khi sửa UpdatePlaying()/CheckCollisions()

`metaProgress.AwardCurrency()` HIỆN TẠI chỉ được gọi ở nhánh `player.GetLives()<=0`
cuối `UpdatePlaying()` - KHÔNG gọi khi WAVE_CLEAR (dọn sạch đội hình hay hạ boss), và
KHÔNG gọi khi GAME_OVER qua đường đội hình chạm đáy
(`PhysicsSystem::DescendRowAndCheckGameOver`). Có thể cố ý, có thể là gap chưa ai để ý
- `tests/test_game_manager.cpp` khóa lại đúng hành vi hiện tại ở cả 2 nhánh GAME_OVER
để bất kỳ thay đổi nào (cố ý hay không) đều hiện thành test đỏ thay vì trôi qua âm
thầm. `tests/test_game_manager.cpp` + `tests/test_physics_system.cpp` là lưới an toàn
cho refactor UpdatePlaying()/boss (stage/shield) - chạy 2 file này
(`./unit_tests "[game_manager],[physics]"`) trước/sau khi sửa 2 file src đó.

## Đối chiếu tài liệu với code thật

ARCHITECTURE.md từng vài lần mô tả lệch so với code thật (từng nhắc một CI workflow
chưa hề tồn tại tại thời điểm đó). Khi đọc ARCHITECTURE.md/README.md để lấy thông tin
quan trọng cho 1 thay đổi, đối chiếu nhanh với code/CMakeLists.txt/CI thật thay vì tin
100%, và cập nhật tài liệu ngay khi phát hiện lệch thay vì để dồn.
