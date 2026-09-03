# Hardcore Space Invaders

Game bắn súng kiểu Space Invaders viết bằng C++ và [raylib](https://www.raylib.com/).

## Trò chơi có gì

Một ván chạy liên tục qua nhiều **wave**, không có màn "thắng" cuối - dọn sạch wave này
thì sang wave sau, đội hình đông và nhanh hơn, giữ nguyên điểm/mạng. Cứ mỗi 5 wave
(`Config::BOSS_WAVE_INTERVAL`) thì thay đội hình bằng một **Boss**.

**9 loại địch.** Năm loại thuộc lưới đội hình, di chuyển và tụt hàng cùng nhịp:

| Loại | Đặc điểm |
|---|---|
| Basic | 1 máu, quân số đông |
| Tanky | Nhiều máu, phải bắn vài phát |
| Zigzag | Dao động ngang hình sin quanh vị trí đội hình |
| Warden | Nhiều máu; **lúc chết sinh ra quân tiếp viện** tại chỗ |
| Medic | 1 máu, không bắn, nhưng **hồi máu cho Tanky gần nhất** - diệt healer trước là lựa chọn chiến thuật thật sự |

Bốn loại còn lại "thoát lưới", có nhịp xuất hiện riêng: **Kamikaze** (tách khỏi đội hình
lao thẳng vào bạn, có bám đuôi giới hạn góc xoay), **Weaver** (bay ngang theo đường sin,
khó bắn trúng), **Bomber** (bay ngang thẳng, định kỳ thả bom xuống), và **UFO** (mystery
ship bay ngang đỉnh màn hình, thưởng điểm ngẫu nhiên).

**3 loại Boss** xoay vòng, mỗi loại một cơ chế riêng: *Vanguard* (đi hết chiều rộng màn
hình, bắn nhanh dần theo % máu), *Sentinel* (gần như đứng yên, định kỳ bật khiên bất khả
xâm phạm - buộc bạn chờ đúng nhịp thay vì giữ nút bắn), *Swarmer* (lắc thất thường, định
kỳ triệu hồi tiếp viện ngẫu nhiên từ Kamikaze/Weaver/Bomber).

**6 power-up** rơi ngẫu nhiên khi hạ địch: Rapid Fire, Shield, Piercing (đạn xuyên nhiều
mục tiêu), Spread Shot (3 tia), Overdrive (bắn nhanh hơn nhưng trúng đòn mất 2 mạng thay
vì 1), và Cleanser (xoá sạch đạn địch đang bay - bom cứu nạn).

**Tiến trình.** Trong ván: combo nhân điểm khi hạ liên tiếp; sau mỗi wave chọn 1 trong 3
nâng cấp (tốc độ di chuyển / +1 mạng / thưởng điểm), cộng dồn cả ván, và wave boss sắp tới
thì nâng cấp được áp dụng 2 lần. Xuyên nhiều ván: điểm quy đổi thành **currency** để mở
khoá loadout *Vanguard* (150 CR, bắt đầu với +1 mạng) và *Overcharge* (400 CR, bắt đầu với
Rapid Fire sẵn). Bảng **Top 10** lưu kèm wave đạt được. Kết thúc mỗi ván có **bảng tổng
kết**: điểm, wave, số địch hạ, combo cao nhất, CR vừa kiếm được, tổng CR, và thanh tiến độ
tới lần mở khoá kế tiếp.

**Độ khó.** Chọn EASY/NORMAL/HARD trong menu, và bên trên đó còn một tầng **DDA** (dynamic
difficulty adjustment) tự điều chỉnh: mỗi lần hạ Boss, game xem bạn mất bao nhiêu mạng
trong chu kỳ vừa rồi rồi nhích tốc độ/nhịp bắn của địch lên hoặc xuống.

## Yêu cầu

- CMake >= 3.16
- Trình biên dịch hỗ trợ C++17 (g++, clang++, MSVC...)
- [raylib](https://github.com/raysan5/raylib) đã cài trên máy (`find_package(raylib REQUIRED)`)

### Cài raylib

**Linux (build từ source):**

Dùng CMake để build raylib (không dùng `make` trực tiếp trong `src/`) - cách build bằng
Makefile CÓ CÀI được raylib nhưng KHÔNG sinh ra `raylib-config.cmake`, khiến
`find_package(raylib REQUIRED)` mà CMakeLists.txt của project này cần (xem mục Yêu cầu
ở trên) bị lỗi ngay bước `cmake ..` phía dưới. Đây cũng chính là cách `.github/workflows/ci.yml`
đang cài raylib cho CI, đã xác nhận chạy được.
```bash
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
cmake -S raylib -B raylib/build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF
cmake --build raylib/build -j$(nproc)
sudo cmake --install raylib/build
```

**macOS (Homebrew):**
```bash
brew install raylib
```

**Windows:** xem hướng dẫn cài đặt tại [raylib wiki](https://github.com/raysan5/raylib/wiki).

## Build

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

Nếu CMake không tự tìm thấy raylib, chỉ đường dẫn thủ công:
```bash
cmake -DCMAKE_PREFIX_PATH=/duong/dan/toi/raylib-install \
      -Draylib_INCLUDE_DIR=/duong/dan/toi/raylib-install/include \
      -Draylib_LIBRARY=/duong/dan/toi/raylib-install/lib/libraylib.a \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## Chạy

Game đọc MỌI đường dẫn tương đối với **thư mục hiện tại lúc chạy**, không phải vị trí file
thực thi. Cách gọn nhất là chạy từ gốc repo — `assets/` và `level.cfg` đã có sẵn ở đó:

```bash
./build/space_invaders
```

Nếu muốn chạy từ trong `build/` thì phải copy `assets/` (font, sprite atlas, shader,
`balance.json`) và `level.cfg` sang trước, nếu không game vẫn chạy nhưng bằng giá trị mặc
định chứ không phải cấu hình thật:

```bash
cd build && cp -r ../assets ../level.cfg . && ./space_invaders
```

Các file save (`settings.cfg`, `leaderboard.dat`, `meta_progress.dat`) được **ghi ra chính
thư mục đang chạy** — nên chạy từ gốc repo và chạy từ `build/` là hai bộ save khác nhau.
Cả ba đều tuỳ chọn và đều nằm trong `.gitignore`; thiếu thì game bắt đầu từ trạng thái
trắng, không crash.

## Điều khiển

| Phím | Chức năng |
|---|---|
| `A` / `D` hoặc mũi tên trái/phải | Di chuyển (`A`/`D` đổi được, mũi tên luôn là phím dự phòng cố định) |
| `Space` | Bắn (đổi được) |
| `P` / `Esc` | Tạm dừng (`P` đổi được, `Esc` luôn là phím dự phòng cố định) |
| `K` (lúc đang Pause) | Đổi phím điều khiển (Trái/Phải/Bắn/Pause) |
| `R` | Chơi lại từ đầu |
| `F11` | Bật/tắt Fullscreen |
| `F3` | Bật/tắt lớp phủ đo lường (FPS, số thực thể, RAM) — hoạt động ở MỌI màn hình |
| `Enter` | Xác nhận (menu, next wave...) |
| Trái/Phải | Trong MENU: đổi độ khó. Ở màn hình WAVE CLEAR: chọn nâng cấp |
| `Q` / `E` | Trong MENU: đổi loadout (dừng trên loadout đang khoá mà đủ currency thì tự mở khoá luôn) |
| Lên/Xuống | Tăng/giảm âm lượng (menu và lúc Pause) |

Màn hình đổi phím (`K` lúc Pause): bấm `1`-`4` để chọn hành động, rồi bấm phím mới muốn
gán. `0` hoặc `R` khôi phục cả 4 về mặc định. `Esc` để huỷ/quay lại. Phím hệ thống
(`Esc`/`Enter`/`R`/`F3`/`F11`/`K`/mũi tên) không thể gán đè lên - tránh tự khoá mình
khỏi menu. Lưu lại vào `settings.cfg`, còn nguyên sau khi tắt/mở lại game.

Có hỗ trợ tay cầm (gamepad) nếu cắm sẵn - hoạt động ở cả lúc chơi (stick trái/D-pad di
chuyển, A/Cross bắn) lẫn menu/pause/end-screen (D-pad đổi độ khó/âm lượng, A/Cross xác
nhận, X/Square chơi lại, Start tạm dừng). Màn hình đổi phím bàn phím ở trên và `F11`
(fullscreen) là 2 chỗ còn giới hạn bàn phím.