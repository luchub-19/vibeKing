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

**Thành tựu.** 8 thành tựu (hạ địch đầu tiên, combo x10, tới wave 5/10, hạ boss, dọn sạch
1 wave không mất mạng, 25.000 điểm, 1.000 địch trọn đời) - mỗi cái thưởng thêm CR. Mở khoá
giữa ván thì hiện thông báo trượt xuống đầu màn hình; phần CR thưởng được trả cùng lúc với
CR quy đổi từ điểm khi ván kết thúc (nên con số "CURRENCY EARNED" ở bảng tổng kết đã gồm
cả hai). Xem danh sách + tiến độ ở mục THÀNH TỰU trong menu chính.

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

Các file save (`settings.cfg`, `leaderboard.dat`, `meta_progress.dat`, `achievements.dat`)
được **ghi ra chính thư mục đang chạy** — nên chạy từ gốc repo và chạy từ `build/` là hai
bộ save khác nhau. Cả bốn đều tuỳ chọn và đều nằm trong `.gitignore`; thiếu thì game bắt đầu từ trạng thái
trắng, không crash.

## Điều khiển

| Phím | Chức năng |
|---|---|
| `A` / `D` hoặc mũi tên trái/phải | Di chuyển (`A`/`D` đổi được, mũi tên luôn là phím dự phòng cố định) |
| `Space` | Bắn (đổi được; có tuỳ chọn **Tự động bắn**) |
| `P` / `Esc` | Tạm dừng → menu Pause: Tiếp tục / Cài đặt / Chơi lại / Về menu chính |
| `R` | Chơi lại từ đầu |
| `F11` | Bật/tắt toàn màn hình (đồng bộ với Cài đặt > Đồ họa) |
| `F3` | Bật/tắt lớp phủ đo lường (FPS, số thực thể, RAM) — hoạt động ở MỌI màn hình |
| Lên/Xuống (`W`/`S`) | Chọn mục trong mọi menu (giữ phím để lặp) |
| Trái/Phải | Đổi giá trị (độ khó, trang bị, cài đặt, thẻ nâng cấp, trang hướng dẫn) |
| `Enter` | Xác nhận |
| `Esc` / `Backspace` | Quay lại màn trước |
| `Q` / `E` (`PgUp`/`PgDn`) | Đổi tab trong Cài đặt / Hướng dẫn |

**Chuột**: rê để chọn, nhấp trái để xác nhận, nhấp phải để quay lại, kéo/lăn để chỉnh thanh âm
lượng, nút "< QUAY LẠI" góc trên trái. Con trỏ tự ẩn khi đang chơi.

**Tay cầm**: stick trái/D-pad di chuyển, A bắn/xác nhận, B quay lại, X chơi lại, Start tạm dừng,
LB/RB đổi tab.

## Giao diện & ngôn ngữ

GUI phong cách **retro arcade** (khung pixel góc bậc thang, con trỏ tam giác nhảy, chữ gõ từng
ký tự kèm tiếng bíp, glitch tách RGB khi vào màn, chữ mục đang chọn nhún kiểu Balatro). Chữ tiêu
đề dùng **Bungee**, chữ thường **VT323** — cả 2 đủ dấu tiếng Việt. Chi tiết thiết kế, nguồn tham
khảo và ý tưởng tiếp theo: [`docs/GUI_UPGRADE.md`](docs/GUI_UPGRADE.md).

- **Menu chính**: Chơi ngay / Bảng xếp hạng / Thành tựu / Hướng dẫn / Cài đặt / Thoát. Để yên
  20 giây → **attract mode** như tủ máy arcade (bảng điểm thưởng, top 10, điều khiển, "INSERT COIN").
- **Xưởng tàu** (sau CHƠI NGAY): chọn độ khó + trang bị; trang bị khoá thì bấm Enter/nhấp lần 2 để
  mở bằng CR (trước đây chỉ cần lướt qua là bị trừ tiền).
- **Hướng dẫn**: điều khiển (phím THẬT đang gán), 9 loại địch kèm điểm, 6 vật phẩm, mẹo chơi.
- **Ngôn ngữ**: English / Tiếng Việt, đổi là áp dụng ngay. Lần đầu mở game tự theo ngôn ngữ hệ
  điều hành (`LC_ALL`/`LC_MESSAGES`/`LANG` bắt đầu bằng `vi` → tiếng Việt).

### Cài đặt (4 tab, lưu ngay vào `settings.cfg`)

| Tab | Tuỳ chọn |
|---|---|
| Chung | Ngôn ngữ, âm lượng tổng / nhạc nền / hiệu ứng, âm thanh menu |
| Đồ họa | Chất lượng Low/Medium/High, CRT, toàn màn hình, hiện FPS |
| Điều khiển | Đổi phím Trái/Phải/Bắn/Tạm dừng (Enter hoặc nhấp rồi bấm phím mới), tự động bắn, khôi phục phím |
| Trợ năng | Giảm nhấp nháy, rung màn hình 100/50/tắt, **tốc độ game** 100/90/80/70%, **lọc màu** cho người mù màu (đỏ/lục/lam), hiện vùng trúng đạn |

Chơi với tốc độ game dưới 100% vẫn được lưu điểm, nhưng dòng đó mang nhãn **HỖ TRỢ/ASSIST** trên
bảng xếp hạng (kiểu Assist Mode của Celeste). Phím hệ thống (`Esc`/`Enter`/`Backspace`/`R`/`F3`/
`F11`/mũi tên) không gán được, và 2 hành động không thể chung 1 phím - tránh tự khoá mình khỏi menu.

## Đồ họa

Phong cách **neon-vector arcade** (Geometry Wars / Galaga Legions), giữ luật màu LẠNH (nền + mọi
loại địch) / NÓNG (đạn, đe doạ, phần thưởng). Chi tiết thiết kế và nguồn tham khảo:
[`docs/GRAPHICS_UPGRADE_PLAN.md`](docs/GRAPHICS_UPGRADE_PLAN.md).

Tab **Đồ họa** trong Cài đặt - đổi là có hiệu lực ngay, lưu vào `settings.cfg`:

| Tuỳ chọn | Low | Medium (mặc định) | High |
|---|---|---|---|
| Bloom | tắt | Dual Kawase 1 mức | 3 mức (quầng rộng) |
| Lưới neon lò xo | tắt | ô 32 px | ô 25 px |
| Tinh vân | tắt | 1 lớp | 2 lớp parallax |
| Sóng xung kích méo hình | tắt | tối đa 4 | tối đa 8 + tách RGB ở mép |
| Hạt hiệu ứng | 0,5x | 1x | 1,5x |
| CRT cong | - | - | có (nếu bật CRT) |

- **Hiệu ứng CRT**: bật/tắt scanline + vignette + nhấp nháy nhẹ.
- **Giảm nhấp nháy** (tab Trợ năng): tắt mọi nhấp nháy (CRT flicker, tàu chớp khi bất tử -> mờ
  đều, sao lấp lánh, đạn địch "thở", chữ INSERT COIN/con trỏ nhấp nháy, glitch tiêu đề, chữ nhún).
  Theo WCAG 2.3.1 / Xbox Accessibility 118.

**Tự đo hiệu năng trên máy bạn** (số đo trong repo là renderer CPU, không phải GPU thật):

```bash
./build/space_invaders --scene=boss --bench=600                # preset đang lưu
./build/space_invaders --scene=boss --quality=low --bench=600  # thử preset khác, không ghi settings.cfg
```

Tham số dòng lệnh khác (chỉ để kiểm chứng đồ họa, xem `src/launch_options.h`):
`--scene=combat|boss|gameover|waveclear|menu|hangar|settings|howto|attract|leaderboard|achievements|pause`
dựng cảnh cố định, `--lang=en|vi` ép ngôn ngữ, `--tab=0..3` chọn tab Cài đặt, `--capture=<file.png>`
chụp rồi thoát; `scripts/capture_showcase.sh` chụp mọi cảnh (cả 2 ngôn ngữ) dưới Xvfb.

## Tài nguyên

- **Đồ hoạ**: [Kenney](https://kenney.nl) — *Space Shooter Remastered* và *Space Shooter
  Extension* (CC0). Ảnh được khử màu rồi nhuộm màu lúc vẽ, nên `atlas.png` là ảnh xám; xem
  `docs/ASSET_INTEGRATION.md` nếu muốn thay pack khác.
- **Font**: VT323 của Peter Hull (chữ thường) và Bungee của David Jonathan Ross (tiêu đề) —
  cả 2 SIL OFL 1.1 (`assets/fonts/LICENSE-VT323.txt`, `assets/fonts/LICENSE-Bungee.txt`).
- **Âm thanh**: không dùng file `.wav` nào — toàn bộ hiệu ứng và nhạc nền được tổng hợp
  bằng code lúc chạy (`src/audio_system.cpp`).
- **Mã nguồn**: MIT (xem `LICENSE`).
