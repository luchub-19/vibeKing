# Hardcore Space Invaders

Game bắn súng kiểu Space Invaders viết bằng C++ và [raylib](https://www.raylib.com/).

## Yêu cầu

- CMake >= 3.16
- Trình biên dịch hỗ trợ C++17 (g++, clang++, MSVC...)
- [raylib](https://github.com/raysan5/raylib) đã cài trên máy (`find_package(raylib REQUIRED)`)

### Cài raylib

**Linux (build từ source):**
```bash
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
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
| `A` / `D` hoặc mũi tên trái/phải | Di chuyển |
| `Space` | Bắn |
| `P` / `Esc` | Tạm dừng |
| `R` | Chơi lại từ đầu |
| `F11` | Bật/tắt Fullscreen |
| `Enter` | Xác nhận (menu, next wave...) |

Có hỗ trợ tay cầm (gamepad) nếu cắm sẵn.