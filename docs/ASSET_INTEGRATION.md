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

## Còn bao nhiêu chỗ trong atlas.png?

Nhiều hơn bạn nghĩ. `atlas.cfg` từng ghi *"atlas.png đã kín chỗ"* ở 4 nơi khác nhau, và
**8 sprite đã phải vẽ bằng code vì lời khẳng định đó** — nó sai theo hai tầng:

1. Dải `y=104..137, x>=160` (684×32 px) vốn đã trống trơn — đo được 0 pixel không trong
   suốt. `iconOverdrive` vừa được nhét vào đó mà không cần đụng kích thước ảnh.
2. Kể cả khi hết chỗ thật thì kích thước atlas **không bị hardcode ở đâu cả**:
   `LoadAtlasEntry()` (`sprites.cpp`) đọc `atlasImg.width/height` lúc chạy. Cứ làm ảnh to
   hơn rồi thêm dòng toạ độ là xong, không sửa một dòng C++ nào.

Ràng buộc THẬT không phải chỗ trống mà là **pack không còn dáng địch nào**: Kenney "Space
Shooter Remastered" có đúng 5 dáng (`enemyBlack1..5`) và cả 5 đã dùng hết cho
`basicAlien`/`tankyAlien`/`zigzagAlien`/`kamikaze`/`boss` — đã đối chiếu từng pixel kênh
alpha, khác 0. Mọi sprite địch MỚI đều phải lắp ráp từ `PNG/Parts`, hoặc chấp nhận
procedural.

## ⚠️ Toàn bộ texture PHẢI là ảnh khử màu (gray/desaturated)

Đây là điểm dễ làm sai nhất khi thay ảnh khác vào atlas.png: `DrawSprite()`
(`src/sprites.h`) vẽ bằng `DrawTexturePro(tex, ..., tint)` — raylib **nhân** màu texture
với `tint` theo từng kênh RGB. Toàn bộ 13 vai trò hiện đang được gán 1 tint **không phải
màu trắng** ở nơi gọi:

Mọi tint dưới đây đến từ `Palette::` (`src/palette.h`) — **không còn hằng số màu của
raylib ở đường gameplay nào cả**. Bảng màu chia 2 dải: LẠNH cho nền + mọi loại địch, NÓNG
chỉ cho đạn / đe doạ lao thẳng vào người chơi / phần thưởng (xem luật đầy đủ ở đầu
`palette.h`).

| Vai trò | Tint | Nơi gán |
|---|---|---|
| `basicAlien` | `Palette::BasicA`/`BasicB` (xen kẽ theo hàng) | `game_manager.cpp::InitLevel` |
| `tankyAlien` | `Palette::Tanky` | `game_manager.cpp::InitLevel` |
| `zigzagAlien` | `Palette::Zigzag` | `game_manager.cpp::InitLevel` |
| `warden` / `medic` | `Palette::Warden` / `Palette::Medic` | `game_manager.cpp::InitLevel` |
| `weaver` / `bomber` | `Palette::Weaver` / `Palette::Bomber` | `game_manager.cpp::SpawnWeaver`/`SpawnBomber` |
| `kamikaze` | `Palette::Kamikaze` (dải NÓNG — loại duy nhất lao thẳng vào người chơi) | `game_manager.cpp::SpawnKamikaze` |
| `ufo` | `Palette::Ufo` (dải NÓNG — mục tiêu thưởng điểm) | `render_system.cpp::DrawPlaying` |
| `boss`/`bossSentinel`/`bossSwarmer` | `Palette::Boss` → `BossEnrage1` → `BossEnrage2` theo stage HP (lạnh → nóng dần = "enrage") | `render_system.cpp::DrawPlaying` |
| `player` | `skinTint`, mặc định `Palette::PlayerShip`, đổi được qua skin mở khoá | `player.h`/`player.cpp` |
| 6 `icon*` lúc RƠI trên mặt đất | `Palette::PowerUp` — **cùng 1 màu cho cả 6 loại**; phân biệt loại nào thì đọc bằng HÌNH icon | `render_system.cpp::DrawPlaying` |
| 5 `icon*` trên HUD (đang có hiệu lực) | mỗi loại 1 màu riêng (`SKYBLUE`/`ORANGE`/`MAGENTA`/`GOLD`/`RED`) — đây là nhãn UI, không phải vật thể trong thế giới game | `render_system.cpp::DrawHUD` |

5 trong 6 `icon*` có ảnh Kenney thật; `iconSpreadShot` vẫn là procedural **có chủ đích** —
xem lý do đầy đủ trong `atlas.cfg`, tóm tắt: pack không có dáng nào diễn đạt được "1 phát
toả thành 3 tia", còn hình 3 mũi tên toả ra mà `BuildIconSpreadShot()` vẽ thì nói đúng ý.

Lưu ý chỗ dễ nhầm ở 2 dòng cuối: cùng một texture `icon*` được tô **2 màu khác nhau tuỳ
ngữ cảnh**. Lúc power-up đang rơi, thông tin quan trọng nhất là "có thứ để nhặt" nên cả 6
dùng chung màu vàng của dải NÓNG; trước đây mỗi loại một màu riêng, vừa phá luật lạnh/nóng
(`SKYBLUE`/`LIME` lấn thẳng vào dải màu của địch) vừa khiến pickup không có dấu hiệu thị
giác chung nào để nhận ra từ xa. Còn trên HUD thì ngược lại — bạn cần biết CHÍNH XÁC cái
nào đang chạy, nên giữ màu riêng.

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
