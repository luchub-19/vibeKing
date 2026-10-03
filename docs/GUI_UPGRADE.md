# Nâng cấp GUI toàn diện — retro arcade + EN/VI

Tài liệu thiết kế cho đợt nâng cấp giao diện (menu, cài đặt, pause, hướng dẫn, HUD, đa ngôn
ngữ). Ảnh trước/sau: `scripts/capture_showcase.sh` (cảnh `--scene=menu|hangar|settings|howto|
attract|leaderboard|achievements|pause`, `--lang=en|vi`).

## 1. Quyết định đã chốt với người dùng

| Câu hỏi | Lựa chọn |
|---|---|
| Kiểu menu chính | **Danh sách dọc** điều hướng Lên/Xuống + Enter (bỏ phím tắt rải rác TAB/G/K/Q/E) |
| Chuột | **Có** — chuột + bàn phím + tay cầm cùng điều khiển 1 con trỏ |
| Ngôn ngữ mặc định | **Theo hệ điều hành** (`LC_ALL` > `LC_MESSAGES` > `LANG`), đổi được trong Cài đặt |
| Đổi ngôn ngữ | **Áp dụng ngay**, không khởi động lại (nạp sẵn glyph cả 2 ngôn ngữ) |
| Tiêu đề khi tiếng Việt | **Tìm font tiêu đề có dấu** → Bungee (thay Audiowide chỉ có Latin cơ bản) |
| Chữ thường | **VT323** (font pixel kiểu màn CRT terminal) thay DejaVu Sans Mono |
| Tab Cài đặt | Chung + Âm thanh / Đồ họa + Hiển thị / Điều khiển / Trợ năng |
| Phạm vi thêm | Menu Pause đầy đủ, màn Hướng dẫn, làm lại HUD — **giữ chất retro** |
| Trợ năng tốc độ < 100% | Vẫn lưu điểm, **gắn nhãn ASSIST** (kiểu Celeste) |
| Hiệu ứng | Chữ gõ + con trỏ nhấp nháy, attract mode + INSERT COIN, chữ nhún + glitch |

Font: 4 ứng viên retro có đủ 134 chữ cái tiếng Việt (kiểm bằng fontTools trên file thật, không
tin nhãn subset của Google Fonts): VT323, Bungee, Handjet, Tektur. Pixelify Sans / Press Start 2P /
Silkscreen / Tiny5 bị loại vì **không có** dấu tiếng Việt.

## 2. Nguồn tham khảo (nhiều ngôn ngữ / cộng đồng)

- **Nhật** — tủ máy Taito/Namco cuối 70-80 (Space Invaders, Galaga): `SCORE ADVANCE TABLE` ở
  attract mode, `INSERT COIN` nhấp nháy, `CREDIT` ở góc, điểm 6 chữ số có số 0 đầu, nhãn `1UP` /
  `HI-SCORE` màu nóng phía trên số. JRPG Famicom (Dragon Quest — "コマンドウィンドウ", cửa sổ
  lệnh): khung viền pixel góc bậc thang, con trỏ tam giác bên trái mục chọn, chữ hiện từng ký tự
  kèm tiếng bíp.
- **Hàn** — bài "히트스탑" của Jubei's Lab (đã dùng cho hit-stop trước đây) và văn hoá UI game
  arcade/PC bang Hàn: thanh âm lượng chia vạch LED rời rạc đọc được số bước bằng mắt.
- **Anh/Mỹ** — Celeste Assist Mode (trợ năng không phạt người chơi, chỉ gắn nhãn), The Last of
  Us Part II (trợ năng là 1 tab riêng dễ tìm), Hades (menu Pause gọn 4 mục + hỏi lại trước khi bỏ
  ván), Balatro (chữ mục đang chọn nhún theo sóng sin từng ký tự), Hotline Miami (glitch tách
  RGB), Enter the Gungeon (bestiary trong màn hướng dẫn), WCAG 2.3.1 / Xbox Accessibility
  Guideline 118 (không quá 3 lần chớp sáng/giây).
- **Việt** — thói quen gọi tên của người chơi Việt: "xu" (nạp xu), "trùm" (boss), "đợt" (wave),
  "vùng trúng" (hitbox), "mù đỏ/lục/lam" cho 3 loại thiếu sắc giác.
- **Khoa học màu** — daltonize của Fidaner, Lin & Ozguven (2005): mô phỏng mắt thiếu sắc giác,
  lấy phần thông tin màu bị mất rồi dồn sang kênh còn phân biệt được.

## 3. Kiến trúc

```
InputSystem::PollMenu ─┐                           ┌─> RenderSystem (render_screens.cpp)
chuột (UpdateUi)       ├─> GameManager::Update*Screen()                 │
                       │      (game_manager_ui.cpp)                     │
ui_layout.h  ──────────┴────── vị trí nút: 1 nguồn cho cả 2 ─────────────┘
settings_menu.h ─────────────── dòng Cài đặt: 1 nguồn cho cả 2
localization.h (X-macro) ────── mọi chuỗi EN/VI
```

| File | Vai trò |
|---|---|
| `localization.h/.cpp` | Bảng chuỗi `LOC_STRINGS(X)` → sinh cả `enum class Str` lẫn bảng dịch, `Tr()`, phát hiện ngôn ngữ OS, bảng mã font |
| `ui_nav.h` | Hàm thuần: vòng quanh danh sách, đổi toạ độ chuột cửa sổ → canvas 800x600, gõ chữ theo ký tự UTF-8, nhấp nháy an toàn, thanh trượt |
| `ui_layout.h` | `Rectangle` của mọi nút — dùng chung bởi hit-test chuột và code vẽ |
| `settings_menu.h/.cpp` | 4 tab × các dòng (Choice/Slider/Key/Action) + thao tác thuần trên `Settings` |
| `retro_ui.h` | Widget vẽ: khung pixel, mũi tên pixel, chữ nhún, tiêu đề glitch, chữ gõ, thanh LED, keycap, dải quét CRT |
| `game_manager_ui.cpp` | Logic điều hướng mọi màn (`UpdateMenu/Hangar/Settings/...`), hộp xác nhận, `ApplySettings()` |
| `render_screens.cpp` | Vẽ mọi màn GUI; `render_system.cpp` giữ phần thế giới game + HUD |

Trạng thái màn: `MENU → HANGAR → (fade) PLAYING ⇄ PAUSED ⇄ SETTINGS`, `MENU ⇄ LEADERBOARD /
ACHIEVEMENTS / HOWTO / SETTINGS`, `MENU →(20 giây yên) ATTRACT →(bất kỳ phím) MENU`. Màn KEYBIND
và GRAPHICS cũ đã gộp vào SETTINGS.

### Các bẫy đã gặp khi làm (đừng lặp lại)

1. **`GetKeyPressed()` LẤY RA khỏi hàng đợi phím của raylib.** Dùng nó để tính "có phím nào
   không" trong `PollMenu()` thì màn đổi phím (`PollAnyKeyPressed`) không bao giờ thấy phím nào.
   `AnyKey` quét `IsKeyPressed()` (chỉ đọc).
2. **Chuột chỉ giành con trỏ khi THẬT SỰ di chuyển** (`PointerInput::moved`) — nếu không, người
   dùng bàn phím thấy con trỏ giật về chỗ chuột đang nằm sau mỗi lần bấm. Khi test bằng `xdotool`:
   `windowfocus` tự đưa chuột vào giữa cửa sổ (đè lên mục menu thứ 2) và `xdotool key` nhả phím
   trong cùng 1 khung hình nên đôi khi rơi phím — dùng `keydown`/`keyup` cách nhau ~60ms, hoặc
   điều khiển bằng `mousemove` + `mousedown/mouseup`.
3. **Bảng mô tả tĩnh (`static const`) không được giữ `const char*` đã dịch** — khởi tạo 1 lần
   lúc nạp chương trình, đổi ngôn ngữ xong vẫn kẹt chữ cũ. Upgrade/Achievement descriptor giờ giữ
   `Str` và gọi `Tr()` lúc vẽ.
4. **Font nạp `LoadFontEx(..., nullptr, 0)` chỉ có ASCII** — chữ có dấu ra ô trống âm thầm.
   Truyền `Loc::FontCharset()`; `test_localization.cpp` khoá mọi ký tự trong bảng dịch phải nằm
   trong bảng mã đó.
5. **VT323 không có ký hiệu mũi tên/tam giác** — con trỏ/mũi tên vẽ bằng hình (`PixelArrow`).
6. Sóng xung kích bị "đông cứng" giữa chừng khi Pause làm méo vĩnh viễn menu Pause → chỉ gửi
   sóng lên shader khi đang PLAYING (`BuildPostFxFrame`).

## 4. Ý tưởng cho đợt sau (chưa làm)

- **Nhập tên 3 chữ cái** khi lọt Top 10 (AAA bằng Lên/Xuống kiểu arcade) — cần đổi định dạng
  `leaderboard.dat` (thêm cột tên, vẫn đọc được file cũ như cách đã làm với cột ASSIST).
- **Demo tự chơi thật trong attract mode** (AI đơn giản bám X địch gần nhất) — phải cô lập hoàn
  toàn khỏi leaderboard/currency/thành tựu.
- **Thêm ngôn ngữ** (日本語/한국어): chỉ cần thêm cột trong `LOC_STRINGS` + font có CJK (VT323/
  Bungee không có) — bảng mã lớn, cân nhắc nạp font theo ngôn ngữ.
- **Cỡ chữ UI** (Trợ năng): nhân hệ số toàn cục cho `gameFont`; cần kiểm lại mọi bố cục cố định.
- **Rung tay cầm** (haptic) theo cường độ rung màn hình.
- **Mô phỏng mù màu** (xem trước như người mù màu thấy) cạnh tuỳ chọn lọc màu, để dev kiểm palette.
