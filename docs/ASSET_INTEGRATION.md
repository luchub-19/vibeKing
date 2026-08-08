# Asset Integration — atlas.png / atlas.cfg

`SpriteSheet::Load()` (`src/sprites.cpp`) thử nạp `assets/sprites/atlas.png` +
`assets/sprites/atlas.cfg` trước. Tên nào không có trong `atlas.cfg`, hoặc atlas.png
không tồn tại/không đọc được, thì tên đó tự động fallback về đúng sprite procedural
(`BuildXxx()`) đang dùng hiện tại — **không lỗi, không crash**, đúng triết lý "không bao
giờ chết vì thiếu 1 file không bắt buộc" đã áp dụng cho `settings.cfg`/`level.cfg`.

## 13 tên bắt buộc

Danh sách DUY NHẤT đúng là 13 field `Texture2D` khai báo trong `class SpriteSheet`
(`src/sprites.h`) — không lấy tên ở đâu khác:

```
player  basicAlien  tankyAlien  zigzagAlien  ufo  kamikaze
boss  bossSentinel  bossSwarmer
iconRapidFire  iconShield  iconPiercing  iconCleanser
```

## Định dạng atlas.cfg

Text thuần, cùng khuôn `KEY=VALUE` + `#` comment như `level.cfg`:

```
# comment
TEN=X,Y,W,H
```

- `TEN` phải khớp (phân biệt hoa/thường) 1 trong 13 tên ở trên. Tên lạ bị bỏ qua.
- `X,Y,W,H` là số nguyên, đơn vị pixel, gốc toạ độ (0,0) ở góc trên-trái `atlas.png`.
- Dòng trống / bắt đầu bằng `#` bị bỏ qua.
- Vùng vượt biên `atlas.png` (`X+W` hoặc `Y+H` > kích thước ảnh), hoặc dòng sai định
  dạng (không đủ 4 số) → **chỉ riêng tên đó** fallback procedural, các tên khác không bị
  ảnh hưởng.
- 2 tên được phép **trỏ chung 1 vùng toạ độ** (dùng chung 1 ảnh vật lý trong atlas.png) —
  atlas.cfg hiện tại làm vậy cho `bossSentinel`↔`tankyAlien` và `bossSwarmer`↔
  `zigzagAlien` (xem lý do chọn trong comment đầu file).

## ⚠️ Toàn bộ 13 texture PHẢI là ảnh khử màu (gray/desaturated)

Đây là điểm dễ làm sai nhất khi thay ảnh khác vào atlas.png: `DrawSprite()`
(`src/sprites.h`) vẽ bằng `DrawTexturePro(tex, ..., tint)` — raylib **nhân** màu texture
với `tint` theo từng kênh RGB. Toàn bộ 13 vai trò hiện đang được gán 1 tint **không phải
màu trắng** ở nơi gọi:

| Vai trò | Tint | Nơi gán |
|---|---|---|
| `basicAlien` | `PURPLE`/`VIOLET` (xen kẽ theo hàng) | `game_manager.cpp::InitLevel` |
| `tankyAlien` | `MAROON` | `game_manager.cpp::InitLevel` |
| `zigzagAlien` | `SKYBLUE` | `game_manager.cpp::InitLevel` |
| `kamikaze` | `RED` | `game_manager.cpp::SpawnKamikaze` |
| `ufo` | `RED` (cố định) | `render_system.cpp::DrawPlaying` |
| `boss`/`bossSentinel`/`bossSwarmer` | `WHITE`→`ORANGE`→`RED` theo stage HP | `render_system.cpp::DrawPlaying` |
| `player` | `skinTint`, mặc định `GREEN`, đổi được qua skin mở khoá | `player.h`/`player.cpp` |
| 4 `icon*` | `ORANGE`/`SKYBLUE`/`MAGENTA`/`LIME` (mỗi loại 1 màu riêng) | `render_system.cpp::DrawPlaying` |

Nếu bake sẵn màu bão hoà vào atlas.png, kết quả trên màn hình sẽ bị "đục" (2 màu nhân
vào nhau) thay vì lên đúng màu như bảng trên. `atlas.png` hiện tại lấy từ Kenney "Space
Shooter Redux" (CC0) rồi **khử màu bằng code** (giữ nguyên shading/khối, bỏ hue) trước
khi ghép — xem lại đúng nguyên tắc "vẽ trắng rồi nhuộm màu riêng từng loại" mà
`BuildXxx()` procedural đã dùng từ đầu (comment trong `sprites.cpp`).

## Đổi sang pack khác

1. Cắt/khử màu 11 ảnh cần dùng (13 vai trò, 2 cặp dùng chung ảnh — xem trên) theo đúng
   nguyên tắc khử màu ở mục trên.
2. Ghép vào 1 `atlas.png` mới, đo toạ độ từng vùng.
3. Sửa `assets/sprites/atlas.cfg` theo định dạng ở trên — không cần sửa code C++ nào cả.
4. Chạy game, **so sánh ảnh chụp trước/sau** (đừng chỉ đọc code rồi cho là xong — gõ sai
   tên sẽ fallback âm thầm, không có thông báo lỗi nào xuất hiện trên màn hình; xem log
   `SpriteSheet: ... N/13 ten hop le trong atlas.cfg` lúc khởi động để biết số tên atlas.cfg
   thực sự nạp được).
