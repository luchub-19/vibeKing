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

**CI ghim đúng 5.5 - đừng chỉ build với raylib mới hơn trên máy mình.** Vài API đổi chữ ký
giữa 5.5 và 5.6-dev (vd `DrawCircleGradient`: `(int, int, ...)` -> `(Vector2, ...)`); code
viết theo bản mới build xanh tại máy nhưng đỏ trên CI. Từng xảy ra ở `player.cpp` - giờ rẽ
nhánh theo `RAYLIB_VERSION_MAJOR/MINOR`. Dùng API vẽ mới thì kiểm lại header của bản 5.5.

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

## Chạy game

Binary đọc MỌI đường dẫn tương đối với **thư mục hiện tại lúc chạy**, không phải vị trí
của file thực thi (xem `Config::*FilePath()` trong config.h). Nghĩa là chạy từ `build/` và
chạy từ gốc repo là 2 bộ save/asset khác nhau:

```bash
# Từ gốc repo - dùng level.cfg + assets/ có sẵn trong repo, save ghi vào gốc repo
./build/space_invaders
```

Cần `assets/` (font/sprite/shader/balance.json) và `level.cfg` ở cwd; thiếu thì game vẫn
chạy được với giá trị mặc định (fallback có chủ đích, không crash) nhưng khác hẳn cấu hình
thật. `settings.cfg`/`leaderboard.dat`/`meta_progress.dat`/`achievements.dat` được ghi ra chính
cwd đó và đều nằm trong `.gitignore`. Ngôn ngữ lần đầu theo `LC_ALL`/`LC_MESSAGES`/`LANG`
(`LANG=vi_VN.UTF-8 ./build/space_invaders` để thử tiếng Việt), sau đó theo `LANGUAGE=` trong
`settings.cfg`.

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
  biến `stage` rời rạc dễ lệch; `SettingsTabRows()` (settings_menu.h) liệt kê dòng Cài đặt
  1 lần cho cả `UpdateSettingsScreen()` lẫn `DrawSettings()`, và `ui_layout.h` là toạ độ DUY
  NHẤT của mọi nút GUI (hit-test chuột và code vẽ cùng đọc - lệch 1px = bấm nút A kích hoạt
  nút B). Cùng tinh thần: **`hitEdge` của
  đội hình là 1 quyết định DUY NHẤT**, gom từ cả 5 pool (Basic/Tanky/Zigzag/Warden/Medic)
  rồi mới đổi hướng + tụt hàng một lần cho tất cả trong `PhysicsSystem::UpdateEnemies()`.
  Warden/Medic từng tự tính `hitEdge` riêng trong 2 hàm gọi riêng — hệ quả là con nào nằm
  giữa lưới không bao giờ tự chạm biên, tức không bao giờ tụt hàng, và bị đội hình bỏ lại
  lơ lửng phía trên.
- **Event queue tách phát hiện va chạm khỏi hiệu ứng**: `PhysicsSystem::CheckCollisions()`
  chỉ xác định va chạm + cập nhật state tối thiểu, đóng gói hệ quả (particle/âm
  thanh/rung màn hình/điểm/power-up) thành `GameEvent` đẩy vào `pendingEvents`;
  `GameManager::ProcessEvents()` mới thực thi. Nhờ vậy `CheckCollisions()` test được
  độc lập, không dính raylib audio/particle thật.
  **2 luật của hàng đợi này, cả 2 đều đã bị vi phạm và gây bug thật:**
  (a) **Điểm xả DUY NHẤT là cuối `ProcessEvents()`** — không hàm nào khác được `clear()`
  nó. `CheckCollisions()` từng clear ở đầu hàm và nuốt sạch event mà `UpdateKamikaze()`
  (chạy trước nó cùng frame) vừa đẩy vào: người chơi mất 1 mạng mà không nổ, không tiếng,
  không rung.
  (b) **`ProcessEvents()` phải duyệt bằng index + bản sao, KHÔNG range-for** — thân vòng
  lặp làm dài thêm chính hàng đợi (`ApplyComboAndScore()` đẩy event "+1 mạng" khi vượt mốc
  điểm), range-for ở đó là heap-use-after-free, đã bắt được bằng AddressSanitizer.
- **Màu đi qua `Palette::` (palette.h), không gọi thẳng hằng số raylib**: luật là LẠNH
  (nền + MỌI loại địch) vs NÓNG (chỉ đạn, đe doạ lao thẳng vào người chơi, và phần
  thưởng) - nhờ vậy mắt người chơi tự bắt "màu nóng = phải né hoặc phải nhặt" mà không
  cần học bảng màu. Đó là lý do Tanky KHÔNG còn màu đỏ: đỏ dành riêng cho nguy hiểm tức
  thì. Hệ quả kỹ thuật: bloom (`Config::BLOOM_THRESHOLD`) trước đây gần như vô dụng vì
  không vật thể nào đủ sáng vượt ngưỡng luma; dải NÓNG sáng hơn nên bloom mới thật sự
  phát sáng đúng chỗ cần. `constexpr` (dữ liệu trình bày), cố ý KHÔNG nằm trong
  balance.json.
- **Cài đặt đồ họa đi qua `GraphicsSettings` (graphics_settings.h)**: preset Low/Medium/High,
  CRT, giảm nhấp nháy, độ rung - tùy chọn NGƯỜI CHƠI lưu trong `settings.cfg`, không phải
  balance data (không vào balance.json). Muốn biết "preset này có bật X không" thì gọi hàm của
  struct đó (`BloomEnabled()`, `ParticleScale()`...), đừng tự so `quality == High`. Mọi flash
  toàn màn hình mới phải tôn trọng `reduceFlashing` (WCAG 2.3.1: không quá 3 lần/giây).
- **2 font + 2 ngôn ngữ**: `gameFont` (VT323, pixel kiểu CRT) cho MỌI chữ thường; `titleFont`
  (Bungee, qua `DrawNeonText()`/`RetroUi::GlitchTitle()`) cho tiêu đề/banner. Cả 2 đủ dấu tiếng
  Việt, NHƯNG chỉ khi nạp với `Loc::FontCharset()` - `LoadFontEx(..., nullptr, 0)` chỉ có ASCII và
  chữ có dấu ra ô trống ÂM THẦM. Mọi chuỗi hiển thị đi qua `Tr(Str::X)` (localization.h, bảng
  X-macro `LOC_STRINGS` - 1 dòng = id + EN + VI); `test_localization.cpp` khoá: đủ bản dịch, cùng
  `%d/%s` ở 2 ngôn ngữ, mọi ký tự nằm trong bảng mã font. VT323 KHÔNG có mũi tên/tam giác - vẽ
  bằng hình (`RetroUi::PixelArrow`), đừng đưa ký hiệu đó vào chuỗi. Bảng mô tả tĩnh giữ `Str`,
  không giữ `const char*` đã dịch (sẽ kẹt ngôn ngữ lúc khởi động).
- **GUI** (chi tiết + các bẫy đã gặp: `docs/GUI_UPGRADE.md`): mỗi màn menu là 1 `GameState`
  với `Update*Screen()` trong `game_manager_ui.cpp` + `Draw*()` trong `render_screens.cpp`; trạng
  thái con trỏ/đồng hồ UI gom trong `GameManager::ui` (`UiState`). Đổi màn menu dùng
  `GoToScreen()` (không fade, reset glitch/chữ gõ), vào/ra gameplay vẫn `RequestTransition()`.
  Chuột chỉ giành con trỏ khi THẬT SỰ di chuyển. F11 xử lý 1 chỗ trong `UpdateUi()` - đừng gọi
  `ToggleFullscreen()` ở `Update*()` khác (bật/tắt 2 lần 1 frame). Đừng gọi `GetKeyPressed()`
  ngoài màn đổi phím: nó LẤY RA khỏi hàng đợi phím. Tốc độ game (Trợ năng) chỉ nhân `dt` của
  `UpdatePlaying()`; ván có lúc < 100% mang nhãn ASSIST (`runAssisted`, theo VÁN).
- **Fade transition 2 pha**: `RequestTransition()` KHÔNG đổi `state` ngay - chỉ đặt
  `pendingState` + bắt đầu `FADE_OUT`; `state` đổi thật bên trong `UpdateTransition()`
  sau đủ `Config::TRANSITION_DURATION` giây, rồi `FADE_IN` trước khi về `NONE`.
- **Config engine vs balance data**: hằng số chia 2 loại - `constexpr` (kỹ thuật, cố
  định, vd `SCREEN_W`) và `inline` (dữ liệu cân bằng - HP/tốc độ/wave pattern... - bị
  `Config::LoadBalance()` ghi đè từ `assets/balance.json` lúc `Run()`). Đừng đổi
  `inline` thành `constexpr` chỉ vì gọn hơn - sẽ làm `balance.json` hết tác dụng.
  **`inline` là một LỜI HỨA, không phải cách viết**: mỗi hằng `inline` phải có 1 dòng
  `Assign()` trong `config.cpp` VÀ 1 key trong `balance.json`, nếu không quy ước trên chỉ
  đúng trên giấy. Từng có 17/118 hằng `inline` (kích thước Kamikaze/Weaver/Bomber/UFO/Boss,
  `POWERUP_SIZE`, `PARTICLE_GRAVITY`, dải độ cao spawn của Weaver) chưa bao giờ được nạp -
  designer chỉnh `balance.json` thì không có gì xảy ra, không báo lỗi. Kiểm nhanh bằng cách
  đối chiếu tên `inline` trong `config.h` với các tên `Config::XXX` xuất hiện trong
  `config.cpp`; hiện đang 118/118.
- **Ghi file atomic + checksum**: `Leaderboard`/`MetaProgress`/`AchievementProgress`/`Settings`
  đều ghi ra `<path>.tmp` rồi `AtomicFile::Replace()` (atomic_file.h); 3 cái đầu có checksum chống
  sửa tay (xem save_checksum.h). Theo mẫu này nếu thêm hệ thống lưu file mới. **Đừng gọi
  thẳng `std::rename()`**: trên Windows nó từ chối ghi đè file đã tồn tại, nên mọi lần lưu
  sau lần đầu thất bại im lặng (đã xảy ra với cả 3 file save).
- **Leaderboard: 1 ván = 1 dòng**: điểm được nộp ở MỖI WAVE_CLEAR (checkpoint) và ở
  GAME_OVER, nhưng `Leaderboard::BeginRun()` (gọi trong `InitLevel(true)`) làm các lần nộp
  trong cùng ván cập nhật CHUNG 1 dòng. Trước đó 1 ván tới wave 10 tự lấp kín Top 10.
- **Đạn xuyên nhớ mục tiêu đã đi qua** (`Bullet::HasPierced`, UID từ `EnemyPool::UidAt`):
  đạn xuyên nằm chồng lên 1 địch nhiều máu thêm vài frame; không nhớ thì mỗi frame đó là 1
  lần trúng mới. Thêm loại địch nhiều máu mới thì áp cùng mẫu như Tanky/Warden/Boss.

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
Định nghĩa `CleanupGuard` thôi thì CHƯA đủ — phải thật sự khởi tạo một biến kiểu đó, nếu
không nó chỉ là dead code im lặng (đã từng xảy ra ở test_game_manager.cpp: guard được định
nghĩa nhưng không TEST_CASE nào dùng, mỗi lần chạy test lại rớt 2 file `.dat` vào cwd và để
nguyên). **Kiểm chứng: `git status` sau `ctest` phải sạch.**
Hằng số cân bằng `inline` (vd `Config::POWERUP_DROP_CHANCE`) ghi đè tạm được ngay trong
test (lưu giá trị gốc, ghi đè, chạy, phục hồi cuối bài) - xem test_boss.cpp.

Đã kiểm chứng bằng probe thật (không phải giả định): gọi thẳng
`UpdatePlaying()`/`CheckCollisions()`/v.v. an toàn HOÀN TOÀN headless, không cần
`InitWindow()`/Xvfb - `IsKeyDown`/`IsGamepadAvailable`/`GetRandomValue` trả về
0/false khi raylib chưa init, và `PlaySound()` trên 1 `Sound{}` chưa từng `LoadSound()`
cũng tự no-op (raylib tự kiểm tra `IsSoundValid()` nội bộ), không crash.

## Trạng thái chỉ sống trong 1 ván vs 1 wave

`GameManager` có 2 nhóm field dễ nhầm nhau, phân biệt bằng chỗ chúng được reset trong
`InitLevel()`:

- **Theo VÁN** - chỉ reset ở nhánh `newGame == true`: `runKills`/`runBestCombo`/
  `runCurrencyEarned` (bảng tổng kết Game Over), `runAssisted` (nhãn ASSIST trên bảng xếp hạng), `hintTimer` (gợi ý phím, chỉ hiện cho
  người chơi mới), bộ DDA (`ddaSpeedMul`/`ddaLivesLostSinceCheck`/`ddaLastKnownLives`), và
  `runAchievementBonus` (CR thưởng thành tựu chờ trả - được TRẢ chứ không chỉ xoá: xem dưới).
- **Theo WAVE** - reset ở CẢ 2 nhánh: mọi pool địch, bullet/particle/power-up, combo,
  `waveBannerTimer` (banner phải hiện lại mỗi wave), `enemySpeed`/`waveFireRateMul`,
  `waveLivesLost` (xét thành tựu UNTOUCHABLE - KHÔNG dùng lại `ddaLivesLostSinceCheck`, biến
  đó reset theo chu kỳ boss 5 wave).

Đặt nhầm nhóm không gây lỗi build và test cũ vẫn xanh - nó chỉ hiện ra dưới dạng "sao
banner không hiện lại ở wave 2" hoặc "sao tổng kết đếm cả ván trước". Có test khoá riêng
cho cả 2 nhóm trong `test_game_manager.cpp` (`[summary]`, `[banner]`).

## Trước khi sửa UpdatePlaying()/CheckCollisions()

**Mọi đường thua cuộc phải đi qua `GameManager::TriggerGameOver()`** - đó là điểm vào duy
nhất, lo cả 4 việc: phát âm thanh, nộp leaderboard, cộng currency, mở màn hình tổng kết.
Thêm một đường thua cuộc thứ ba (ví dụ hết giờ) thì gọi hàm đó, đừng chép lại từng bước.
Hàm này **idempotent** (cờ `gameOverTriggered`): `RequestTransition()` không đổi `state`
ngay, nên guard `if (state != PLAYING) return` trong `UpdatePlaying()` KHÔNG chặn được
trường hợp đội hình chạm đáy và người chơi hết mạng trong cùng một frame - thiếu cờ đó thì
currency bị cộng 2 lần.

**Mọi đường WAVE_CLEAR phải gọi `GameManager::OnWaveCleared()`** (sau `wave++`): hiện có 2
đường (đội hình sạch trong `PhysicsSystem::UpdateEnemies()`, boss gục trong
`UpdatePlaying()`). Hàm đó xét thành tựu theo sự kiện (GIANT SLAYER/UNTOUCHABLE) và tự gọi
`SyncLivesLost()` trước - đường của `UpdateEnemies()` chạy TRƯỚC điểm ghi nhận mạng mất
thường lệ trong frame, thiếu bước này thì mạng vừa mất lọt qua và UNTOUCHABLE mở sai.

**CR vào ví ở ĐÚNG 1 nhịp - kết thúc ván.** Thưởng thành tựu mở giữa ván dồn vào
`runAchievementBonus`, trả qua `PayOutAchievementBonus()` tại `TriggerGameOver()` (cộng vào
`runCurrencyEarned`), ở `InitLevel(true)` (bỏ ván giữa chừng bằng R) và lúc đóng cửa sổ.
Đừng gọi `metaProgress.AddBonusCurrency()` ngay lúc mở khoá - sẽ phá test khoá "WAVE_CLEAR
không cộng currency".

Lịch sử: trước 2026-09-03 có 2 đường GAME_OVER làm 2 việc khác nhau - hết mạng thì được
CR, đội hình chạm đáy thì không. `AwardCurrency()` vẫn KHÔNG được gọi khi WAVE_CLEAR (dọn
sạch đội hình hay hạ boss) - đó vẫn là hành vi hiện tại và có test khoá.

### Bài học: một test có thể xanh mà không kiểm tra gì cả

Test khoá nhánh "đội hình chạm đáy KHÔNG cộng currency" đặt `score = 0` (player vừa
`Reset()`). Mà `AwardCurrency(0)` = 0 CR - nên khẳng định "currency không đổi" **đúng cả
khi đã cộng currency**. Lúc gộp 2 đường lại, test đó vẫn xanh thay vì đỏ lên như thiết kế:
lưới an toàn tưởng là có thật hoá ra rỗng.

Khi viết test kiểu "X không xảy ra", luôn tự hỏi: **nếu X xảy ra thật thì test này có đỏ
không?** Nếu giá trị đang dùng làm cho cả 2 nhánh cho cùng kết quả (0, chuỗi rỗng, list
rỗng...) thì phải đổi sang giá trị phân biệt được. `tests/test_game_manager.cpp` + `tests/test_physics_system.cpp` là lưới an toàn
cho refactor UpdatePlaying()/boss (stage/shield) - chạy 2 file này
(`./unit_tests "[game_manager],[physics]"`) trước/sau khi sửa 2 file src đó.

## Sprite: tìm ảnh thật TRƯỚC, viết `BuildXxx()` sau

Đường mặc định để thêm/đổi một sprite là **1 dòng toạ độ trong `assets/sprites/atlas.cfg`**,
không phải hàm vẽ hình bằng `ImageDrawRectangle` trong `sprites.cpp`. Hiện 18/19 tên dùng ảnh
thật; `iconSpreadShot` là ngoại lệ duy nhất và có ghi lý do tại chỗ.

Ba niềm tin sai từng khiến 8 sprite phải vẽ bằng code - đừng lặp lại:

1. *"atlas.png đã kín chỗ"* - không có gì hardcode kích thước atlas, `LoadAtlasEntry()` đọc
   `atlasImg.width/height` lúc chạy. Hết chỗ thì làm ảnh to hơn.
2. *"pack Kenney không còn dáng nào"* - kết luận đó rút ra từ ĐÚNG MỘT thư mục
   (`Enemies` của bản Remastered). Bản Extension có `Sprites/Ships` với 9 tàu vẽ liền, chưa
   ai mở. **Kiểm hết mọi thư mục của mọi pack trước khi kết luận "không còn gì".**
3. *"tăng tương phản là cải thiện"* - `sigmoidal-contrast` đẩy chỉ số độ lệch chuẩn lên
   thật nhưng xoá sạch cấu trúc thành khối đặc. Đo `sd` là biến quan sát, không phải mục
   tiêu tối ưu.

Quy tắc xử lý ảnh + bảng tint đầy đủ ở `docs/ASSET_INTEGRATION.md`. Lưu ý cơ chế fallback
diễn ra ÂM THẦM: gõ sai tên trong `atlas.cfg` không báo lỗi, chỉ lộ ra ở dòng log
`SpriteSheet: ... n/19 ten hop le` lúc khởi động - đọc dòng đó sau mỗi lần sửa atlas.

## Hàm VẼ thuần: không có test, phải chụp ảnh mới biết đúng/sai

`Bullet::Draw()`, `Bunker::Draw()`, `Player::Draw()`, `RenderSystem::*` không trả về giá
trị và không đổi state - không viết được test tự động, và **đọc code không phát hiện được
lỗi trong đó**. Một ví dụ thật đã tốn công tìm: vệt sáng viên đạn dùng
`fmaxf(rect.width, rect.height)` để tính bề dày. Đọc thì hợp lý; thay số thật (đạn 5x15px)
thì `fmaxf` trả về CHIỀU DÀI dọc hướng bay chứ không phải tiết diện, ra bề dày 27px - rộng
gấp 5,4 lần viên đạn và rộng hơn cả độ dài vệt, nhìn ra một tấm ván đặt ngang phía sau.
Chỉ lộ ra khi phóng to ảnh chụp 5x.

Nên: sửa bất cứ thứ gì thuộc khâu vẽ thì **chạy game và chụp lại màn hình**, đừng dừng ở
"build sạch, test xanh". Trên máy này chụp được bằng `grim -g "<x,y wxh>"` (lấy toạ độ cửa
sổ từ `swaymsg -t get_tree`), và phóng to bằng
`magick <file> -crop <vùng> +repage -filter point -resize 500%` để soi từng pixel. Nhớ
export khoá GPU (`~/.config/environment.d/50-gpu-lock.conf`) trước khi chạy game từ shell,
nếu không dGPU sẽ thức - xem mục GPU lai trong CLAUDE.md toàn cục.

Trong container không có màn hình (cloud/CI): `scripts/capture_showcase.sh [binary] [out]`
dựng các cảnh cố định (`--scene=combat|boss|...` + 8 cảnh GUI × `--lang=en|vi`), chụp ảnh sau post-process và đo frame time
(`--bench`) - chỉ cần Xvfb. Đó là cách chuẩn để có ảnh trước/sau cho mọi thay đổi khâu vẽ (xem
`src/launch_options.h`, `docs/GRAPHICS_UPGRADE_PLAN.md`). Số liệu bench dưới Xvfb là llvmpipe
(render bằng CPU): chỉ so tương đối, không phải frame time trên GPU thật. **Đo A/B XEN KẼ trên
cùng một lần chạy máy** (build commit gốc vào `git worktree` riêng, chạy base/new lần lượt 3 lần):
container có thể khởi động lại trên phần cứng khác giữa 2 phiên, số đo của phiên trước KHÔNG còn
là mốc (đã gặp: cùng 1 binary, mốc cảnh boss đổi từ ~17,0 sang ~17,9 ms sau khi container khởi
động lại, và độ dao động giữa các lần chạy tăng từ <3% lên ~10%). Khi nhiễu cao: **ít nhất 8 lần
mỗi bên, so TRUNG VỊ và MIN**, không so trung bình 3-5 lần - đã từng kết luận "Kawase nhanh hơn
Gauss 5%" từ trung bình 5 lần, đo lại 8 lần thì thực ra CHẬM hơn 5%.

## Đối chiếu tài liệu với code thật

ARCHITECTURE.md từng vài lần mô tả lệch so với code thật (từng nhắc một CI workflow
chưa hề tồn tại tại thời điểm đó). Khi đọc ARCHITECTURE.md/README.md để lấy thông tin
quan trọng cho 1 thay đổi, đối chiếu nhanh với code/CMakeLists.txt/CI thật thay vì tin
100%, và cập nhật tài liệu ngay khi phát hiện lệch thay vì để dồn.
