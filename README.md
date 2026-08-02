# Hardcore Space Invaders

Game bắn súng kiểu Space Invaders viết bằng C++ và [raylib](https://www.raylib.com/).

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

File thực thi (`space_invaders`) cần thư mục `assets/` (chứa font) nằm **cùng chỗ chạy**. Copy vào thư mục `build` trước khi chạy nếu chưa có:

```bash
cp -r ../assets .
./space_invaders
```

`level.cfg` và các file save (`settings.cfg`, `leaderboard.dat`) là tùy chọn — thiếu thì game tự dùng giá trị mặc định, không crash.

## Điều khiển

| Phím | Chức năng |
|---|---|
| `A` / `D` hoặc mũi tên trái/phải | Di chuyển (`A`/`D` đổi được, mũi tên luôn là phím dự phòng cố định) |
| `Space` | Bắn (đổi được) |
| `P` / `Esc` | Tạm dừng (`P` đổi được, `Esc` luôn là phím dự phòng cố định) |
| `K` (lúc đang Pause) | Đổi phím điều khiển (Trái/Phải/Bắn/Pause) |
| `R` | Chơi lại từ đầu |
| `F11` | Bật/tắt Fullscreen |
| `Enter` | Xác nhận (menu, next wave...) |

Màn hình đổi phím (`K` lúc Pause): bấm `1`-`4` để chọn hành động, rồi bấm phím mới muốn
gán. `0` hoặc `R` khôi phục cả 4 về mặc định. `Esc` để huỷ/quay lại. Phím hệ thống
(`Esc`/`Enter`/`R`/`F3`/`F11`/`K`/mũi tên) không thể gán đè lên - tránh tự khoá mình
khỏi menu. Lưu lại vào `settings.cfg`, còn nguyên sau khi tắt/mở lại game.

Có hỗ trợ tay cầm (gamepad) nếu cắm sẵn - hoạt động ở cả lúc chơi (stick trái/D-pad di
chuyển, A/Cross bắn) lẫn menu/pause/end-screen (D-pad đổi độ khó/âm lượng, A/Cross xác
nhận, X/Square chơi lại, Start tạm dừng). Màn hình đổi phím bàn phím ở trên và `F11`
(fullscreen) là 2 chỗ còn giới hạn bàn phím.