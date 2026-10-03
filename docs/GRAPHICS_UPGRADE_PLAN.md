# Kế hoạch nâng cấp đồ họa toàn diện - "Neon-vector arcade"

> Trạng thái: **ĐÃ DUYỆT** (2026-10-03). Xong: GĐ 0, 1, 2, 3, 5, 6 + hạ tầng settings (mục 4). **Còn: GĐ 4 - đo trên iGPU thật** (cần máy người dùng).
> Ảnh baseline chụp headless bằng Xvfb (thư mục `screenshots/` nằm trong `.gitignore`, ảnh trước/sau gửi kèm PR thay vì commit).
> Nhánh: `claude/upgrade-vibking-graphics-cwhbn3`. Ngày lập: 2026-10-03.

## 0. Những gì người dùng đã chốt

| Câu hỏi | Lựa chọn |
|---|---|
| Phong cách | **Neon-vector arcade** - giữ nền tảng hiện tại (bloom + bảng màu lạnh/nóng), đẩy mạnh theo hướng Geometry Wars / Tempest / Galaga Legions |
| Phạm vi | Cả 4 mảng: **Nền & parallax, VFX chiến đấu, Post-process/shader, UI/HUD/menu** |
| Asset | Shader/procedural + CC0, **chấp nhận cả CC-BY** (bắt buộc có file `CREDITS.md`) |
| Phần cứng | **GPU tích hợp, 60 fps**; hiệu ứng nặng phải tắt được |
| Cường độ | **Cân bằng** - đã mắt nhưng đạn địch luôn là thứ nổi nhất |
| Settings mới | **Preset chất lượng** (Low/Medium/High), **chế độ giảm nhấp nháy/rung**, **bật/tắt CRT riêng** |
| Không được đổi | **Luật màu LẠNH (nền + địch) / NÓNG (đạn, đe doạ, phần thưởng)** - xem `src/palette.h` |
| Giao nộp | Lập kế hoạch trước, duyệt rồi mới code |
| Thứ tự | **VFX trước**: GĐ 0 -> hạ tầng settings (mục 4) -> GĐ 2 -> GĐ 1 -> GĐ 3 -> GĐ 5 -> GĐ 4/6 |
| CRT cong | **Thêm barrel distortion, chỉ ở preset High** (tắt được) |
| Font tiêu đề | **Audiowide** (OFL) cho logo/tiêu đề; chữ nhỏ giữ DejaVu Sans Mono |

Những thứ KHÔNG bị khoá (có thể đổi nếu có lý do): sprite Kenney, độ phân giải nội bộ 800x600,
hitbox. Tuy vậy kế hoạch dưới đây **cố ý không đụng gameplay/hitbox** - mọi thay đổi là trình bày.

---

## 1. Hiện trạng (đã đọc code, không suy đoán)

- Render nội bộ `renderTarget` 800x600 -> `PostProcess::Render()` (bloom 3 pass ở 1/2 độ phân giải
  với Gauss tách 2 chiều, rồi CRT: scanline + vignette + flicker; **cố ý không có barrel
  distortion** - xem comment đầu `assets/shaders/crt.fs`).
- `Parallax`: 90 chấm sao `DrawCircle`, 3 lớp, vị trí là hàm thuần của `GetTime()`.
- `ParticlePool<400>`: 2 hình dạng (vuông, tia), màu phẳng, alpha giảm tuyến tính, có trọng lực.
- `ScreenShake`: nhiễu `GetRandomValue` lấy mẫu lại 20 lần/giây + lerp; `HitStop` có sẵn.
- Địch: sprite xám nhuộm tint + `IdleWobble` + `HitFlashTint`. Tanky/Warden bị thương vẽ thêm
  `DrawRectangleLinesEx` trắng (khối vuông cứng, lệch tông neon).
- `BLOOM_ENABLED`/`CRT_ENABLED` là `constexpr` -> **hiện không thể bật/tắt lúc chạy**. Muốn có preset
  chất lượng và nút CRT trong Settings thì phải chuyển chúng thành trạng thái runtime (mục 4).

### 1.1. Bug phát hiện khi khảo sát - cần sửa ở Giai đoạn 0

**`src/render_system.cpp:493` - `Color tint;` của power-up KHÔNG BAO GIỜ được gán.** Commit `ae951c4`
("bang mau lanh-nong") xoá các vế `tint = ORANGE/SKYBLUE/...` trong `switch` nhưng quên thêm
`tint = Palette::PowerUp;` như comment ngay bên trên mô tả. Kết quả: biến tự động chưa khởi tạo
truyền vào `DrawSprite()` = hành vi không xác định; power-up hiện màu rác của stack (có thể trong
suốt, có thể đen, đổi theo build). Đúng loại lỗi mà CLAUDE.md cảnh báo: "hàm VẼ thuần không có
test, đọc code không phát hiện được". Sửa 1 dòng, xác nhận bằng ảnh chụp.

Lệch nhỏ khác cùng loại: vòng khiên Sentinel dùng `SKYBLUE` thẳng (render_system.cpp ~dòng 474),
và có 37 chỗ dùng hằng màu raylib (`YELLOW`/`GRAY`/`SKYBLUE`/`GOLD`...) trong `render_system.cpp`,
vi phạm luật "màu đi qua `Palette::`". Gom lại ở Giai đoạn 5 (UI).

---

## 2. Tổng hợp nghiên cứu (nhiều nguồn, nhiều ngôn ngữ)

| Nguồn | Ngôn ngữ | Ý tưởng rút ra | Áp dụng vào |
|---|---|---|---|
| Tuts+ "Neon Vector Shooter: The Warping Grid"; Kai/KANA "Geometry Wars-like Spring Grid" | EN (+ bản DE) | Lưới nền = khối lượng-lò-xo, lò xo **chỉ kéo không đẩy**, viền neo cố định, damping chống dao động; vụ nổ áp lực đẩy/hút điểm lưới | GĐ 1 - lưới nền phản ứng |
| GameMakers.jp - bài IDC2023 về *NeverAwake* | JA | Chia 3 lớp: **tiền cảnh có đổ bóng / lớp sương ngăn cách / hậu cảnh ít tương phản** để đạn luôn đọc được | Nguyên tắc R1-R2, GĐ 1 |
| shmups.system11.org "Bullet Design"; Boghog's shmup 101 (shmups.wiki) | EN | Đạn dùng alpha-blend ở tiền cảnh; **glow cộng sáng vẽ PHÍA SAU đạn**, không phủ lên - chồng additive làm đạn "tan" vào nhau | GĐ 2 - đạn |
| Jan Willem Nijman "The Art of Screenshake" (Vlambeer) | EN | Hit-pause vài frame, shake có hướng, giật lùi khi bắn, vỏ đạn/mảnh vỡ tồn tại lâu, flash trắng | GĐ 2 |
| Jonasson & Purho "Juice it or lose it" (GDC 2012) | EN | Tween + squash/stretch + particle bùng nổ; **mỗi tương tác đều có phản hồi** | GĐ 2, GĐ 5 (UI) |
| indienova "打击感和其常见营造方式"; CSDN "游戏特效制作之节奏篇" | ZH | Nổ thật = 5 quá trình tách được: **cầu lửa, sóng xung kích, khí nóng khúc xạ, mảnh vỡ, khói**; luật **"chậm vào - nhanh ra"**, thời gian tan > thời gian bùng | GĐ 2 - nổ nhiều lớp |
| Jubei's Lab "타격감 표현 연출 론"; bài báo KISTI về 타격감 trong game bắn 2D | KO | Trong hit-stop **hiệu ứng vẫn phải chuyển động** (chỉ thực thể đứng); nghiên cứu: **vật thể nổ quan trọng hơn vật thể bắn** | GĐ 2 - ưu tiên ngân sách cho nổ, sửa hit-stop |
| Habr "Взрывная волна в Unity3D (displacement shader)" | RU | Distortion = mỗi pixel đọc lệch theo vector; nhiều sóng gộp vào 1 pass | GĐ 3 - shockwave |
| gameidea.org "Shockwave Distortion Shader"; lettier "3D Game Shaders - Chromatic Aberration" | EN | Vòng distortion + **tách kênh RGB chỉ ở mép sóng** (không toàn màn hình) | GĐ 3 |
| frost.kiwi "Video Game Blurs"; ARM SIGGRAPH 2015 "Bandwidth-efficient rendering" | EN | **Dual Kawase**: ~7% băng thông so với Gauss tuyến tính - đúng thứ iGPU cần | GĐ 3 - thay blur bloom |
| Reboot Game "Le Juiciness dans les jeux vidéo" | FR | Mỗi loại vũ khí 1 "chữ ký" hình ảnh riêng (tia, vệt, sóng đồng tâm) | GĐ 2 - mỗi power-up 1 chữ ký |
| Shadertoy/Godot Shaders nebula fbm; marian42/starfield | EN | Tinh vân fbm rẻ khi **render 1 lần ra texture**, chỉ cuộn texture mỗi frame | GĐ 1 |
| Thảo luận Steam *Nova Drift* "Visual Noise and Ship Color" | EN | Bài học ngược: wave cao **nhiễu thị giác** che tàu người chơi -> cần tùy chọn "quầng sau tàu" + tắt flash | R3, GĐ 4 |
| Space Invaders Extreme / Galaga Legions DX (Jeff Minter làm visualizer nền) | EN | Nền "sống" theo nhịp game, banner wave/boss mang tính trình diễn | GĐ 1, GĐ 5 |
| Xbox Accessibility Guideline 118; WCAG 2.3.1 | EN | **Không quá 3 lần nhấp nháy/giây**; chế độ nhạy sáng tắt strobe, giảm flash cao tương phản | GĐ 4 |
| Squirrel Eiserloh "Juicing Your Cameras With Math" (GDC 2016) | EN | Shake theo mô hình **trauma²** + nhiễu mượt (Perlin) thay random rời rạc; thêm rung xoay nhỏ | GĐ 2 |

---

## 3. Nguyên tắc ràng buộc cho MỌI giai đoạn

- **R1 - Đạn địch luôn thắng.** Không lớp hiệu ứng nào được vẽ ĐÈ lên `enemyBullets`. Thứ tự vẽ
  mới: nền -> lưới -> địch -> particle/nổ -> glow đạn (additive) -> **lõi đạn (alpha, trên cùng)**
  -> player -> HUD.
- **R2 - Nền im khi màn hình đông.** Độ sáng lưới/tinh vân nhân với hệ số giảm theo số đạn địch
  đang bay (vd. 1.0 khi < 10 viên, xuống 0.4 khi > 40 viên). Ý tưởng "lớp sương" từ NeverAwake.
- **R3 - Luật lạnh/nóng giữ nguyên tuyệt đối.** Nền/lưới/tinh vân chỉ dùng dải LẠNH; mảnh vỡ của
  địch giữ màu LẠNH của chính nó, chỉ **lõi cầu lửa** là trắng-nóng ngắn (< 0.1 s). Mọi màu mới
  thêm vào `Palette::`, không gọi hằng raylib.
- **R4 - Mỗi hiệu ứng có công tắc** và có ở ít nhất 1 preset; Low phải chạy được 60 fps ở 1080p
  trên iGPU.
- **R5 - Kiểm chứng bằng ảnh chụp**, không dừng ở "build sạch, test xanh" (CLAUDE.md). Trong
  container cloud: chạy game dưới `xvfb-run`, chụp bằng `TakeScreenshot()`/`import`, phóng to
  bằng `convert ... -filter point -resize 500%`. Ảnh trước/sau đính kèm mỗi commit.
- **R6 - Logic hiệu ứng tách khỏi vẽ** để test headless: phần mô phỏng (lưới lò xo, danh sách
  shockwave, trauma shake, flash governor) là hàm thuần/struct không gọi raylib vẽ, có
  `tests/test_*.cpp` riêng (nhớ thêm vào CMakeLists.txt).

---

## 4. Hạ tầng: chất lượng đồ họa thành trạng thái runtime - **XONG**

**Đã làm (khác bản đề xuất ở dưới chỗ nào thì ghi rõ):**
- `src/graphics_settings.h`: `GraphicsSettings` (quality / crtEnabled / reduceFlashing /
  shakePercent 100-50-0) + `GraphicsRow`. Là **1 nguồn duy nhất** cho "preset X bật gì":
  `BloomEnabled()`, `ParticleScale()`, `ShakeScale()`, `CrtFlickerScale()`. Hiệu ứng mới ở các
  giai đoạn sau thêm 1 hàm vào đây, không rải `if (quality == High)`.
- Lưu trong `settings.cfg` (`GFX_QUALITY/GFX_CRT/GFX_REDUCE_FLASHING/GFX_SHAKE`); file cũ không có
  key nào -> mặc định = diện mạo trước đây (Medium, CRT bật). Giá trị sửa tay sai -> mặc định,
  `GFX_SHAKE` làm tròn về mức gần nhất.
- `PostProcess::Render()` quyết định MỖI FRAME chạy pass nào; `Init()` vẫn load mọi thứ Config
  cho phép nên đổi preset giữa chừng có hiệu lực ngay. `Config::BLOOM_ENABLED/CRT_ENABLED` giữ
  vai công tắc tổng lúc build.
- Particle co trong `ParticlePool::Burst()` (1 điểm phủ mọi nơi gọi), không bao giờ về 0 hạt.
  Độ rung co ở camera `DrawPlaying()` (1 điểm đọc offset).
- Trang GRAPHICS: phím `G` ở menu chính (Q4 = trang riêng). Lên/Xuống chọn dòng, Trái/Phải đổi.
- `--quality=low|medium|high` để chụp/đo từng preset mà không ghi `settings.cfg`.
- Test: `[graphics]` (round-trip file, file cũ, giá trị sai, preset, làm tròn particle) + 1 test
  `[game_manager][graphics]` đi qua `ProcessEvents()` thật (10 vs 5 hạt - đã thử tắt phần nối
  preset, test đỏ đúng như mong đợi).

**Preset thực tế hiện tại** (bảng đề xuất bên dưới là đích cuối, điền dần theo giai đoạn):

| | Low | Medium | High |
|---|---|---|---|
| Bloom | tắt | bật (Gauss cũ) | bật |
| Particle | 0,5x | 1x | 1x (lên 1,5x ở GĐ 2) |
| Khác | - | - | **chưa khác Medium** - trang GRAPHICS ghi thẳng điều này |

`reduceFlashing` hiện chỉ tắt CRT flicker vì game chưa có flash toàn màn hình nào; mọi flash lớn
thêm ở GĐ 2-4 PHẢI đọc cờ này.

Bench cảnh boss (llvmpipe, 2 lần): **Low 8,0-8,2 ms**, **Medium 15,8-16,3 ms** (= mốc GĐ 0,
không tụt). Bloom là gần một nửa chi phí frame trên renderer CPU - lý do GĐ 3 đổi sang Dual Kawase.

**Bản đề xuất gốc:**

- Thêm `struct GraphicsSettings` vào `Settings` (lưu trong `settings.cfg`, cùng đường
  `AtomicFile::Replace()` sẵn có):
  - `quality`: `Low | Medium | High` (mặc định **Medium**)
  - `crtEnabled`: bool (mặc định true - giữ diện mạo hiện tại)
  - `reduceFlashing`: bool, `screenShakeScale`: 0 / 0.5 / 1.0
- `Config::BLOOM_ENABLED`/`CRT_ENABLED` đổi vai thành "mặc định khi chưa có settings.cfg";
  `PostProcess` nhận `GraphicsSettings` và tự bật/tắt từng pass mỗi frame (vẫn giữ fallback khi
  shader lỗi). Lưu ý quy ước CLAUDE.md: đây là **tùy chọn người chơi**, không phải dữ liệu cân
  bằng -> không đưa vào `balance.json`, không tạo hằng `inline` mới.
- Bảng preset (đề xuất, sẽ đo lại):

| Hạng mục | Low | Medium | High |
|---|---|---|---|
| Bloom | tắt hoặc 1 mức Kawase 1/4 | Dual Kawase 3 mức | Dual Kawase 4 mức + bloom màu |
| Lưới nền lò xo | lưới tĩnh, không mô phỏng | 32x24 điểm | 48x36 điểm |
| Tinh vân | tắt | texture nướng sẵn 1 lớp | 2 lớp parallax |
| Shockwave distortion | tắt | tối đa 4 sóng | tối đa 8 sóng + tách RGB ở mép |
| Hệ số particle | 0.5x | 1x | 1.5x (pool tăng 400 -> 800) |
| Vệt đạn/tàu | tắt | ngắn | đầy đủ |

- Màn Settings/Options: thêm 1 trang "GRAPHICS" (preset, CRT, giảm nhấp nháy, độ rung) theo kiểu
  `DrawSelectPill` sẵn có. **Câu hỏi mở Q4** - xem mục 7.

---

## 5. Các giai đoạn (mỗi giai đoạn = 1 commit riêng, có ảnh trước/sau)

### GĐ 0 - Dọn nền móng - **XONG**
1. ✅ Sửa bug `tint` power-up chưa khởi tạo (mục 1.1) - đã xác nhận bằng ảnh: 6 icon hiện đúng
   màu vàng `Palette::PowerUp`.
2. ✅ Chế độ trình diễn qua tham số dòng lệnh (`src/launch_options.h`, có test `[launch]`):
   `--scene=combat|boss` dựng sẵn 1 cảnh cố định (seed RNG cố định, gameplay đóng băng - không
   chết, không cộng điểm/currency); `--capture=<file>` chụp ảnh SAU post-process rồi thoát;
   `--bench=<n>` bỏ giới hạn FPS và in thống kê frame time. Chạy không tham số = như cũ.
   Script `scripts/capture_showcase.sh` (chỉ cần Xvfb, không gửi phím) chụp cả 2 cảnh + đo.
   - Cảnh `combat`: đủ 9 loại địch (Tanky/Warden đang bị thương), UFO, 24 đạn địch, 5 đạn
     player, 6 loại power-up, 3 cụm mảnh vỡ.
   - Cảnh `boss`: Sentinel wave 10 đang bật khiên + vòng 32 đạn toả tròn.
   - Ảnh giữa 2 lần chạy gần như trùng khớp; chỉ lệch rất nhỏ do `IdleWobble`/`Parallax` đọc
     `GetTime()` - đủ để so trước/sau.
3. ✅ Mốc hiệu năng (`--bench=600`, 2 lần mỗi cảnh, Xvfb + **llvmpipe = render bằng CPU 4 nhân**,
   KHÔNG phải iGPU - chỉ dùng để so TƯƠNG ĐỐI trước/sau mỗi thay đổi, sai số giữa 2 lần < 3%):

   | Cảnh | frame avg | frame p95 | CPU phát lệnh vẽ avg |
   |---|---|---|---|
   | combat | 15,0-15,4 ms | 17,6-18,0 ms | 13,0-13,3 ms |
   | boss | 16,3-16,7 ms | 19,5-20,1 ms | 14,0-14,3 ms |

   Quy ước từ đây: mỗi giai đoạn ghi lại bảng này; tăng > 15% ở preset Medium phải có lý do.
   Mốc tuyệt đối trên iGPU thật cần chạy `--bench` trên máy của bạn (container không có GPU).

**Quan sát từ ảnh GĐ 0 - đưa vào các giai đoạn sau:**
- Đạn địch bắn chéo (vòng đạn boss) vẫn vẽ thành **thanh dọc**, còn vệt đuôi lại đi chéo theo
  vận tốc -> nhìn như que gãy. `Bullet::Draw()` cần xoay theo hướng bay -> GĐ 2 mục 2.
- Viền `DrawRectangleLinesEx` trắng của Tanky/Warden bị thương là khung vuông cứng, lệch tông
  neon rõ rệt -> GĐ 2 mục 4 (đã dự kiến).
- Lá chắn xanh bạc hà khá gần màu tàu người chơi, và là khối sáng lớn nhất nửa dưới màn hình ->
  cân nhắc hạ độ bão hoà / thêm texture voxel ở GĐ 5.
- Vòng đạn boss đè lên thanh HP boss ở HUD -> HUD cần nền mờ/viền tách lớp ở GĐ 5.
- Vòng khiên Sentinel vẽ 1 nét `SKYBLUE` mảnh, khó thấy trên nền tối -> GĐ 2 mục 3 (khiên lục giác).

### GĐ 1 - Nền & parallax "sống" - **XONG**

**Đã làm** (3 commit):
- `WarpGrid` (warp_grid.h/.cpp): lưới khối lượng-lò xo, lò xo chỉ kéo, viền neo, bước cố định
  60 Hz. Nổ Small/Large + đạn player đẩy lưới. Hiện ở MỌI màn hình (Q1 mặc định), dừng khi PAUSE.
  Không lõm theo đội hình (Q1 mặc định - tránh nhiễu nền).
- Sao: lớp gần thành vệt theo tốc độ, lớp xa lấp lánh (tắt khi `reduceFlashing`); **warp** khi
  vào wave boss (x8 rồi phanh trong 1,6 s, chỉ đếm khi đang PLAYING ngoài fade).
- `Nebula` (nebula.h/.cpp + `assets/shaders/nebula.fs`): fbm domain-warp **tuần hoàn**, nướng
  1 lần/chương (5 wave) ra texture 400x300, mỗi frame chỉ cuộn. 3 chương màu lạnh quay vòng.
- Luật R2: `WarpGrid::CalmFactor()` dịu cả lưới lẫn tinh vân còn 40% khi ≥ 40 đạn địch.
- Test: `[warp_grid]` (8), `[nebula]`, `WarpSpeedMul`, preset nền, `warpBoostTimer` theo wave.

**Khác đề xuất, có lý do:**
- Preset Low **tắt hẳn** lưới (đề xuất: "lưới tĩnh") - đo: vẽ ~1000-1600 đoạn thẳng vẫn tốn ngang
  1 pass hậu kỳ trên renderer CPU; máy yếu cần bỏ, không cần một lưới không chuyển động.
- Mật độ: Medium 32 px, High 25 px (đề xuất 32x24 điểm / 48x36 điểm). Medium 25 px ban đầu: +25%.
- Hiệu ứng "nền sáng theo chương" ở boss wave: chưa làm - boss dùng màu chương của nó.

**Hiệu năng - VƯỢT ngưỡng +15% đã đặt, cần bạn quyết:** bench A/B xen kẽ trên CÙNG máy
(container đã khởi động lại giữa chừng nên không so được với số cũ): cảnh boss Medium
**17,9 -> 21,5 ms (+20%)**; trong đó lưới + sao ~14%, tinh vân ~6%. High 23,1 ms, Low 11,6 ms.
Đã thử và bỏ 2 tối ưu: `DrawLineV` (GL_LINES) chậm hơn `DrawLineEx`; nướng tinh vân đục rồi
tắt blending -> CHẬM hơn (24,3 ms) vì đổi trạng thái pipeline. Phần lớn chi phí là tô pixel trên
llvmpipe (render bằng CPU) - trên GPU thật, 1 texture toàn màn hình + ~1000 đoạn thẳng là không
đáng kể, nhưng chưa đo được trên iGPU thật (`--scene=boss --bench=600` trên máy bạn sẽ trả lời).

**Đề xuất gốc:**
1. **Lưới neon lò xo kiểu Geometry Wars** (`src/warp_grid.h/.cpp`): mô phỏng khối lượng-lò-xo
   bước cố định 60 Hz, viền neo, lò xo chỉ kéo. Hàm `ApplyExplosiveForce(pos, force, radius)`
   gọi từ `ProcessEvents()` khi có nổ (đúng luật event-queue: chỉ đọc event, không clear),
   `ApplyImplosiveForce` cho boss tụ lực. Vẽ bằng `DrawLineEx` mảnh, màu `Palette::GridLine`
   (tím-xanh tối, luma thấp để **không** vượt ngưỡng bloom - nền không được phát sáng mạnh hơn
   địch). Đường lưới chính mỗi 4 ô sáng hơn một chút cho cảm giác chiều sâu.
2. **Tinh vân procedural**: shader fbm render **1 lần** lúc vào wave ra texture 1/2 độ phân giải
   (không tính mỗi frame - rẻ cho iGPU), màu đổi theo "chương" (wave 1-5 tím, 6-10 xanh ngọc,
   boss wave đậm hơn) - vẫn trong dải LẠNH.
3. **Sao nâng cấp**: lớp gần vẽ thành vệt ngắn (stretch theo tốc độ), sao lớp xa nhấp nháy chậm
   bằng pha riêng; khi boss xuất hiện tăng tốc cuộn trong 1 s ("warp"), cảm giác lao vào trận.
4. Áp R2 (nền dịu khi đông đạn).

### GĐ 2 - VFX chiến đấu ("đánh trúng phải sướng") - **PHẦN CHÍNH XONG**

**Đã làm** (4 commit, mỗi cái có ảnh trước/sau từ `--scene`):
- Đạn: lõi xoay theo hướng bay (sửa lỗi "que gãy" ở vòng đạn boss), sợi sáng giữa thân, quầng
  additive vẽ PHÍA SAU; cả pool vẽ trong 1 khối blend (trước đây mỗi viên 1 lần đổi blend mode).
  Thứ tự lớp mới theo R1 - lõi đạn địch trên mọi hiệu ứng.
- Nổ nhiều lớp `ParticlePool::Explosion()`: Glow (lõi trắng-nóng ~0.1s) / Ring (sóng ease-out) /
  Debris (xoay, có lực cản) + tia lửa cũ. Boss gục = Large (2 vòng lệch nhịp).
  `GameEvent::explosion` - CheckCollisions chỉ khai báo, ProcessEvents mới sinh hạt.
- Hit-stop: particle/rung/chữ điểm chạy tiếp trong lúc đóng băng (nguồn KO).
- Tanky/Warden: vạch HP thay khung trắng; giật + bẹp khi trúng (chỉ hình vẽ).
- Khiên lục giác CHUNG cho player và Sentinel (`Palette::ShieldBarrier`, `draw_helpers.h`).
- Tàu: nghiêng theo hướng, giật lùi khi bắn; bất tử + `reduceFlashing` = mờ đều thay vì chớp.
- Rung: mô hình trauma² + value noise tất định + xoay ≤1,5° quanh TÂM màn hình.
- Test mới: `[vfx]`, `[shake]`, `[player][vfx]` - mỗi test đã thử làm hỏng code để chắc nó đỏ.

**Mục dời lại - cập nhật sau GĐ 5:** đã làm: High 1,5x hạt (pool 800), đạn địch "thở", đạn
xuyên đổi hình, khiên vỡ có hiệu ứng, bóng mờ khi lướt, player mất mạng -> khử bão hoà (GĐ 3).
**Không làm (có lý do):** boss nổ dây chuyền 1,5 s (phải trì hoãn luồng WAVE_CLEAR = đổi gameplay),
lớp "khói" (nền tối - khói tối không thấy, khói sáng lấn đạn), khiên gợn khi trúng (khiên đỡ 1
đòn rồi vỡ - đã thay bằng hiệu ứng vỡ), slow-motion khi chết (làm chậm luồng Game Over).

**Danh sách gốc lúc dời:**
- Boss gục nổ dây chuyền 1,5 s; lớp "khói"; afterimage khi đổi hướng; khiên gợn khi trúng.
- Đạn địch "thở" độ sáng; đạn Piercing đổi hình dạng (chữ ký riêng từng power-up).
- Player chết: slow-motion + khử bão hoà -> cần pass shader, làm cùng GĐ 3.
- Preset High 1,5x particle (pool 400 -> 800): chưa đổi, High vẫn = Medium.

Bench (llvmpipe): combat 17,1 ms / boss 17,1 ms (GĐ 0: 15,2 / 16,5). Phần tăng chủ yếu vì cảnh
trình diễn giờ có thêm 3 vụ nổ đứng yên; Medium vẫn dưới ngưỡng +15%.

**Đề xuất gốc:**
1. **Nổ nhiều lớp** theo mô hình 5 quá trình (nguồn ZH): lõi flash trắng-nóng 2-3 frame ->
   vòng sóng xung kích mở rộng ease-out -> tia lửa (Spark, giữ màu lạnh của địch) -> mảnh vỡ
   chậm, xoay, tồn tại lâu hơn -> "khói" là chấm mờ lớn tan dần. Particle thêm `Ring`,
   `Debris`(xoay), `Glow`(additive). Kích thước nổ theo loại: Basic nhỏ, Tanky/Warden vừa, Boss
   chuỗi nổ dây chuyền 1.5 s trước khi biến mất.
2. **Đạn**: lõi alpha + quầng additive phía sau (nguồn shmups.system11) + vệt ngắn. Đạn địch thêm
   nhịp "thở" độ sáng nhẹ để mắt bắt chuyển động. Đạn xuyên (Piercing) đổi hình dạng (dài, mảnh)
   chứ không chỉ đổi màu - chữ ký riêng mỗi power-up (nguồn FR).
3. **Tàu người chơi**: lửa đẩy dạng particle liên tục, nghiêng nhẹ theo hướng di chuyển,
   giật lùi 2 px khi bắn, afterimage mờ khi đổi hướng nhanh; tia lửa nhỏ ở nòng (đã có
   `MUZZLE_FLASH_PARTICLE_COUNT`, làm đẹp lại). Khiên: vòng lục giác gợn sóng khi trúng.
4. **Phản hồi trúng đòn**: thay viền `DrawRectangleLinesEx` trắng của Tanky/Warden bị thương bằng
   vạch HP mảnh hoặc "vết nứt" phát sáng theo % máu; địch bị đẩy nhẹ (knockback 2-3 px, chỉ hình
   vẽ, hitbox không đổi); squash ngắn khi trúng.
5. **Hit-stop**: kiểm tra lại để particle/shockwave **vẫn chạy** trong hit-stop (nguồn KO).
6. **Shake**: chuyển sang mô hình trauma² + nhiễu mượt, thêm xoay nhỏ (±1°) cho cú nổ boss; nhân
   `screenShakeScale` của settings.
7. **Chết của người chơi**: slow-motion 0.3 s + vòng sóng lớn + màn hình khử bão hoà ngắn.

### GĐ 3 - Post-process / shader - **XONG**

**Đã làm** (2 commit):
- Bloom **Dual Kawase** (`kawase_down.fs`/`kawase_up.fs`), Medium 1 mức, High 3 mức; xoá `blur.fs`.
- **Pass cuối hợp nhất** `final.fs` (thay `crt.fs`): CRT cong (chỉ High + CRT bật, cong BÊN TRONG
  destRec) -> sóng xung kích (`ShockwaveField`, post_fx.h, Medium 4 / High 8 sóng) -> tách RGB chỉ
  ở mép sóng (High) -> S-curve + bão hoà +8% -> khử bão hoà 0,35 s khi mất mạng
  (`GameEvent::playerHurt`) -> viền ngả đỏ theo `BossStage` (enrage, không nhấp nháy) -> scanline/
  vignette/flicker. Mọi thứ trong 1 lần vẽ vì mỗi pass thêm đều đắt (xem số đo).
- Mục 7 của GĐ 2 (player chết: khử bão hoà) làm ở đây dưới dạng "mỗi lần mất mạng" - dễ thấy hơn
  và không chen vào fade sang GAME_OVER. Slow-motion: KHÔNG làm (sẽ làm chậm luồng Game Over).
- Test `[post_fx]`: vòng đời sóng, thay sóng cũ nhất, giới hạn preset, khử bão hoà, enrage theo
  stage + chỉ khi đang đánh, sóng/đồng hồ là trạng thái THEO WAVE.

**Đính chính + hiệu năng:** commit Kawase ghi "nhanh hơn Gauss ~5%" - **sai**, 5 lần trung bình bị
nhiễu kéo lệch. Đo lại 8 lần xen kẽ, lấy trung vị (llvmpipe, cảnh boss Medium):

| | trung vị | min |
|---|---|---|
| Trước GĐ 3 (Gauss) | 19,1 ms | 17,9 |
| Kawase | 20,1 ms (+5%) | 18,5 |
| + pass cuối hợp nhất | 21,6 ms (+13% so với trước GĐ 3) | 20,4 |

Giữ Kawase vì lý do GPU (tài liệu ARM: ~7% băng thông Gauss) nhưng **chưa đo được trên iGPU thật**.
Cộng dồn với GĐ 1 (+20%), Medium hiện chậm hơn bản trước nâng cấp khoảng 1/3 trên renderer CPU.

**Đề xuất gốc:**
1. **Bloom Dual Kawase** thay Gauss 2 chiều (rẻ hơn rõ trên iGPU, quầng mềm hơn). Giữ ngưỡng luma
   - nhờ bảng NÓNG nó vẫn sáng đúng chỗ.
2. **Shockwave distortion**: 1 pass toàn màn hình nhận mảng uniform `vec4 waves[8]` (x, y, bán kính,
   cường độ). Danh sách sóng do 1 struct thuần quản lý (test headless được). Tách RGB **chỉ ở mép
   vòng sóng**, không áp chromatic aberration toàn màn hình thường trực (làm nhoè đạn - trái R1).
3. **Color grading nhẹ**: LUT/curve đơn giản trong pass cuối - nâng độ tương phản tông lạnh, giữ
   tông nóng bão hoà. Khi boss enrage: vignette ngả đỏ rất nhẹ.
4. **CRT**: thêm tuỳ chọn tắt; flicker = 0 khi `reduceFlashing`. **Barrel distortion chỉ ở preset
   High** (người dùng chốt). Cái giá đã ghi trong `crt.fs`: đổi UV làm lệch viền/toạ độ letterbox
   -> xử lý bằng cách cong ảnh BÊN TRONG `destRec` (vùng ngoài cong tô đen + viền bo góc), không
   đổi toạ độ input/chuột; `DrawDebugOverlay` vẽ sau pass CRT nên không bị cong. Cường độ nhỏ
   (k ~ 0.03-0.05) để đạn ở góc màn hình không bị méo khó đọc (R1).
5. Mọi shader viết `#version 330`, kiểm tra hàm raylib dùng tới đúng chữ ký **raylib 5.5** (bài học
   `DrawCircleGradient` trong CLAUDE.md).

### GĐ 4 - Trợ năng & hiệu năng
1. **Flash governor**: struct thuần đếm flash toàn màn hình/vùng lớn, chặn > 3 lần/giây (WCAG 2.3.1);
   `reduceFlashing` thay flash trắng bằng giảm độ sáng nhẹ, tắt CRT flicker, tắt tách RGB.
2. Tùy chọn **"quầng sau tàu"** - vòng sáng mờ quanh tàu người chơi khi màn hình đông (bài học
   Nova Drift).
3. Đo lại frame time từng preset so với baseline GĐ 0; Low phải giữ 60 fps.

### GĐ 5 - UI / HUD / menu - **XONG**

**Đã làm** (4 commit):
- Font **Audiowide** (SIL OFL, `assets/fonts/LICENSE-Audiowide.txt`) cho logo + tiêu đề MỌI màn
  (GAME OVER / WAVE CLEARED / PAUSED / ACHIEVEMENTS / GRAPHICS / REBIND KEYS / banner). Chữ nhỏ
  vẫn DejaVu. `DrawNeonText` + `NeonPowerOn` ("bật đèn" 2 lần chớp lúc mở game; giảm nhấp
  nháy -> sáng dần).
- HUD vector: `FramedPanel` góc ngoặc; điểm lăn số (`RollToward`); tàu mini thay chữ LIVES;
  thanh máu boss màu theo giai đoạn (`BossTint`, cùng nguồn với sprite) + vạch tại
  `BOSS_STAGE2/3_RATIO` + vệt sát thương (`DamageTrail`).
- Banner: wave thường = neon + vạch phóng ra; boss = dải sọc cảnh báo đỏ-đen trôi + WARNING.
  Chỉ hiện khi đang PLAYING (pause đầu wave từng để vạch banner cắt chữ PAUSED).
- Chuyển cảnh: 12 dải đóng từ giữa, lệch nhịp trên -> dưới (`WipeBandCoverage`).
- Màu: 46 dòng trong `render_system.cpp` + pip power-up + 2 chỗ trong `physics_system.cpp` chuyển
  sang `Palette::`. Còn lại chỉ là `WHITE` làm tint trung tính (vẽ texture/mặt nạ sprite).
- Test `[ui]`: lăn số, vệt sát thương, wipe (alpha 1 phải kín mọi dải), ease, power-on (≤ 3 lần
  bật/giây - WCAG 2.3.1).

**Giới hạn đã biết:** sóng xung kích (GĐ 3) méo CẢ HUD/banner khi vòng sóng đi qua (HUD vẽ vào
cùng render target trước pass cuối). Sóng chỉ sống 0,4-0,8 s; tách HUD ra sau pass cuối cần 1
render target nữa - chưa làm. Màn GAME OVER / WAVE CLEARED: tiêu đề neon đã đổi nhưng chưa chụp
được ảnh (cần chơi tới đó; cảnh trình diễn chưa có 2 màn này).

**Đề xuất gốc:**
1. **Title screen**: logo vẽ bằng nét neon (outline phát sáng + nhấp nháy "khởi động bóng đèn"
   1 lần lúc vào, rồi đứng yên - không strobe), lưới nền GĐ 1 dùng luôn ở menu.
2. **HUD**: khung mảnh kiểu vector, số điểm "lăn" (tween) thay vì nhảy, combo meter phát sáng
   theo cấp, icon mạng là mini-tàu. Thanh HP boss dạng neon chia đoạn theo `BossStage()`.
3. **Banner wave / cảnh báo boss**: dải "WARNING" chạy ngang kiểu arcade, chữ quét sáng.
4. **Chuyển cảnh**: thay fade đen thuần bằng wipe dạng quét scanline/xoá lưới - giữ nguyên cơ chế 2
   pha `RequestTransition()`/`UpdateTransition()`, chỉ đổi cách vẽ.
5. **Màn chọn nâng cấp / toast thành tựu**: thẻ có viền sáng, hover nảy (squash), tween vào.
6. **Font**: giữ DejaVu Sans Mono cho chữ nhỏ (dễ đọc, có dấu tiếng Việt); thêm **Audiowide** (SIL
   OFL, Google Fonts) cho logo/tiêu đề, kèm file license trong `assets/fonts/` như DejaVu. Lưu ý:
   Audiowide chỉ có bộ Latin cơ bản - chỉ dùng cho chuỗi tiếng Anh (tiêu đề hiện đều là tiếng
   Anh); fallback về DejaVu nếu thiếu file.
7. Gom hết hằng màu raylib còn sót trong `render_system.cpp` về `Palette::`.

### GĐ 6 - Tài liệu - **XONG**
README (mục Đồ họa: bảng preset, tuỳ chọn trợ năng, cách tự đo), ARCHITECTURE (sơ đồ pipeline
vẽ 1 frame + thứ tự lớp R1 + module mới), CLAUDE.md (GraphicsSettings, 2 font, cách đo A/B),
CREDITS = mục Tài nguyên của README (Audiowide OFL).

**Đề xuất gốc:**
Cập nhật `ARCHITECTURE.md` (pipeline render mới + thứ tự lớp), `CLAUDE.md` (quy tắc R1-R3, nơi đặt
`GraphicsSettings`), `docs/ASSET_INTEGRATION.md` (font/asset mới), `CREDITS.md` nếu có CC-BY.

---

## 6. Ước lượng chi phí iGPU (sẽ đo thật ở GĐ 0/4)

- Lưới 48x36 = 1.728 điểm, ~3.400 lò xo: CPU < 0,2 ms/frame; vẽ ~3.400 đoạn thẳng, rlgl gom batch.
- Tinh vân: 0 ms/frame (nướng sẵn), chỉ 1 lần vẽ texture.
- Bloom Dual Kawase 3 mức từ 800x600: rẻ hơn Gauss hiện tại.
- Shockwave: +1 pass toàn màn hình 800x600 (rẻ) - chỉ chạy khi có sóng đang sống.
- Rủi ro lớn nhất là số lệnh vẽ particle khi tăng pool; Low/Medium giữ nguyên 400.

---

## 7. Câu hỏi mở

Đã chốt: Q2 (CRT cong ở High), Q3 (Audiowide), Q5 (VFX trước), Q6 (đã sửa bug `tint`).
Còn lại - nếu không có ý kiến khác sẽ dùng mặc định trong ngoặc:

- **Q1 - Lưới lò xo**: hiện cả ở menu lẫn trong trận? Lưới có "lõm" theo đội hình địch?
  (Mặc định: cả menu và trận; KHÔNG lõm theo đội hình - tránh nhiễu nền, R2.)
- **Q4 - Vị trí menu Graphics**: (Mặc định: trang riêng mở từ menu chính, vì panel phải của menu
  hiện tại đã kín - xem ảnh baseline.)

---

## Nguồn tham khảo

- [Make a Neon Vector Shooter in XNA: The Warping Grid](https://gamedevelopment.tutsplus.com/tutorials/make-a-neon-vector-shooter-in-xna-the-warping-grid--gamedev-9904) - EN ([bản DE](https://code.tutsplus.com/de/erstellung-von-einem-neon-vektor-shooter-in-xna-the-warping-grid--gamedev-9904t))
- [Geometry Wars like Spring Grid - Kai/KANA](https://www.kana.games/post/geometry-wars-like-spring-grid) - EN
- [NeverAwake: IDC2023 - GameMakers](https://gamemakers.jp/article/2024_01_12_58805/) - JA
- [Bullet Design - shmups.system11.org](https://shmups.system11.org/viewtopic.php?t=43351) - EN
- [Boghog's bullet hell shmup 101 - Shmups Wiki](https://shmups.wiki/library/Boghog's_bullet_hell_shmup_101) - EN
- [The Art of Screenshake - Jan Willem Nijman (YouTube)](https://www.youtube.com/watch?v=AJdEqssNZ-U) - EN
- [Juice It or Lose It - GDC Vault](https://www.gdcvault.com/play/1016487/juice-it-or-lose) - EN
- [游戏基础知识——"打击感"和其常见营造方式 - indienova](https://indienova.com/indie-game-development/game-basic-the-design-of-impact-feel-and-common-implementation-methods/) - ZH
- [高级游戏特效制作之节奏篇 - CSDN](https://blog.csdn.net/zmk0110_/article/details/145973176) - ZH
- [Jubei's Lab: 타격감 표현 연출 론](http://jubei.egloos.com/v/4291383) - KO
- [2차원 슈팅 게임에서의 타격감에 대한 실험적 분석 - KISTI](https://scienceon.kisti.re.kr/srch/selectPORSrchArticle.do?cn=JAKO201017337332989&dbt=NART) - KO
- [Взрывная волна в Unity3D (displacement shader) - Хабр](https://habr.com/ru/articles/282604/) - RU
- [Shockwave Distortion Shader (2D Space Wrap) - gameidea](https://gameidea.org/2025/01/20/shockwave-distortion-shader-2d-space-wrap/) - EN
- [Chromatic Aberration - 3D Game Shaders for Beginners](https://lettier.github.io/3d-game-shaders-for-beginners/chromatic-aberration.html) - EN
- [Video Game Blurs (and how the best one works) - frost.kiwi](https://blog.frost.kiwi/dual-kawase/) - EN
- [Bandwidth-efficient rendering - ARM, SIGGRAPH 2015](https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_notes.pdf) - EN
- [Le "Juiciness" dans les jeux vidéo - Reboot Game](https://reboot-game.com/dossier-le-juiciness-dans-les-jeux-video/) - FR
- [2d Nebula Shader - Godot Shaders](https://godotshaders.com/shader/2d-nebula-shader/), [marian42/starfield](https://github.com/marian42/starfield) - EN
- [Visual Noise and Ship Color - Nova Drift Steam](https://steamcommunity.com/app/858210/discussions/0/3052863612113136754/) - EN
- [Space Invaders Extreme - Wikipedia](https://en.wikipedia.org/wiki/Space_Invaders_Extreme) - EN
- [Xbox Accessibility Guideline 118](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/118), [WCAG 2.3.1](https://www.digitala11y.com/understanding-sc-2-3-1-three-flashes-or-below-threshold/) - EN
- Squirrel Eiserloh, "Math for Game Programmers: Juicing Your Cameras With Math", GDC 2016 - EN
