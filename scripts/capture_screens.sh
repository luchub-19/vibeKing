#!/usr/bin/env bash
#
# capture_screens.sh (A5) - dong Bug 3: "khong chup duoc screenshot cac man hinh UI
# trong moi truong headless (CI, may khong co man hinh that)". raylib/GLFW van CAN mo
# duoc 1 X11 display de InitWindow() thanh cong du KHONG co man hinh vat ly nao ca -
# Xvfb cung cap dung 1 "man hinh gia" nhu vay trong RAM, du de game chay va render binh
# thuong nhu tren man hinh that.
#
# File nay THUAN TOOLING - khong dung/sua bat ky file nguon nao cua game (src/, tests/,
# CMakeLists.txt...).
#
# YEU CAU (cai qua apt tren Debian/Ubuntu):
#   sudo apt-get install -y xvfb imagemagick xdotool
#
# CACH DUNG:
#   ./scripts/capture_screens.sh [duong_dan_binary] [thu_muc_output]
#
#   duong_dan_binary  Mac dinh: <repo>/build/space_invaders
#   thu_muc_output    Mac dinh: <repo>/screenshots
#
# KET QUA: 4 file PNG trong thu_muc_output - 01_menu, 02_playing, 03_paused,
# 04_keybind. Day la 4 man hinh dieu huong duoc BANG PHIM CO DINH, xac dinh (deterministic)
# tu luc vua mo game, KHONG phu thuoc gameplay/RNG.
#
# GHI CHU PHAM VI: EndScreen (GAME_OVER/WAVE_CLEAR) KHONG duoc script nay chup, vi 2
# man hinh do chi vao duoc bang cach THUC SU choi het mang hoac don sach 1 wave - phu
# thuoc AI dich/thoi gian, khong the lam xac dinh (deterministic) chi bang vai phim bam
# co dinh nhu 4 man hinh con lai. Muon xem 2 man hinh nay, chup thu cong hoac dieu
# khien qua chinh xdotool tu ben ngoai script.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BINARY="${1:-$REPO_ROOT/build/space_invaders}"
OUT_DIR="${2:-$REPO_ROOT/screenshots}"
WINDOW_TITLE="Hardcore Space Invaders" # Phai khop CHINH XAC chuoi truyen cho InitWindow() trong game_manager.cpp

for tool in Xvfb import xdotool; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "LOI: thieu lenh '$tool'. Cai qua: sudo apt-get install -y xvfb imagemagick xdotool" >&2
        exit 1
    fi
done

if [ ! -x "$BINARY" ]; then
    echo "LOI: khong tim thay binary tai '$BINARY'." >&2
    echo "  Build truoc: cmake -S '$REPO_ROOT' -B '$REPO_ROOT/build' && cmake --build '$REPO_ROOT/build'" >&2
    echo "  Hoac truyen duong dan binary khac qua tham so 1." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

# Chon so hieu display NGAU NHIEN trong 1 dai rieng - tranh dung do ":99" cung luc voi
# 1 tien trinh Xvfb khac (vd chay 2 job CI song song tren cung may).
DISPLAY_NUM=":$((90 + RANDOM % 100))"
RESOLUTION="800x600x24" # Khop DUNG Config::SCREEN_W x SCREEN_H - InitWindow() khong scale luc moi mo (xem game_manager.cpp)

XVFB_PID=""
GAME_PID=""
cleanup() {
    [ -n "$GAME_PID" ] && kill "$GAME_PID" >/dev/null 2>&1 || true
    [ -n "$XVFB_PID" ] && kill "$XVFB_PID" >/dev/null 2>&1 || true
    wait >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

echo "Khoi dong Xvfb tren display $DISPLAY_NUM ($RESOLUTION)..."
Xvfb "$DISPLAY_NUM" -screen 0 "$RESOLUTION" -nolisten tcp >/tmp/capture_screens_xvfb.log 2>&1 &
XVFB_PID=$!

# Cho socket cua Xvfb san sang - khong co dong log co dinh nao de grep chac chan tren
# moi ban Xvfb, nen poll bang chinh 1 lenh X (getdisplaygeometry) thay vi sleep co dinh
# (vua co the qua ngan tren may cham, vua lang phi thoi gian tren may nhanh).
ready=0
for _ in $(seq 1 40); do
    if DISPLAY="$DISPLAY_NUM" xdotool getdisplaygeometry >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 0.25
done
if [ "$ready" -ne 1 ]; then
    echo "LOI: Xvfb khong san sang sau 10s (xem /tmp/capture_screens_xvfb.log)." >&2
    exit 1
fi

# Game DOC nhieu file tuong doi luc khoi dong (assets/fonts/..., level.cfg, settings.cfg,
# leaderboard.dat, meta_progress.dat - xem Config::*FilePath() trong config.h) -> PHAI
# chay voi CWD = REPO_ROOT, khong phai thu muc chua binary (build/).
echo "Khoi dong game ($BINARY)..."
( cd "$REPO_ROOT" && DISPLAY="$DISPLAY_NUM" "$BINARY" >/tmp/capture_screens_game.log 2>&1 ) &
GAME_PID=$!

# Cho cua so game THAT SU xuat hien (khong chi tien trinh da fork) truoc khi gui phim -
# xdotool gui phim vao "khong khi" se khong co tac dung gi neu cua so chua tao xong.
win_id=""
for _ in $(seq 1 60); do
    win_id="$(DISPLAY="$DISPLAY_NUM" xdotool search --name "$WINDOW_TITLE" 2>/dev/null | head -n1 || true)"
    [ -n "$win_id" ] && break
    if ! kill -0 "$GAME_PID" >/dev/null 2>&1; then
        echo "LOI: tien trinh game thoat som (xem /tmp/capture_screens_game.log)." >&2
        exit 1
    fi
    sleep 0.25
done
if [ -z "$win_id" ]; then
    echo "LOI: khong thay cua so '$WINDOW_TITLE' sau 15s." >&2
    exit 1
fi
DISPLAY="$DISPLAY_NUM" xdotool windowactivate --sync "$win_id" >/dev/null 2>&1 || true

# QUAN TRONG: KHONG dung `xdotool key` (nhan+tha ngay lap tuc, sat nhau chi vai ms).
# raylib IsKeyPressed() phat hien "vua bam" bang CACH SO SANH currentKeyState voi
# previousKeyState giua 2 lan PollInputEvents() (~1 lan/frame). Neu ca press LAN
# release cua `xdotool key` lot vao CHUNG 1 chu ky poll (hoan toan co the xay ra khi
# gui event tong hop, khac hang phim that - CPU render bang phan mem (llvmpipe) trong
# Xvfb cang lam frame time that thuong hon nua), currentKeyState quay ve "tha" truoc
# khi frame do kip ghi nhan canh "vua nhan" -> IsKeyPressed() tra ve false, phim coi
# nhu KHONG duoc bam - LOI NAY DA TAI HIEN DUOC: cung 1 chuoi Enter->p->k, co lan qua
# het 3 man hinh, co lan dung yen o MENU tu dau den cuoi, tuy thuoc may man ve timing.
# Tach keydown/keyup voi 1 khoang GIU PHIM (200ms - du du de an toan tren may cham,
# vd may CI dang tai nang lam giam FPS thuc te sau nhieu so voi 60 FPS muc tieu) buoc
# key o trang thai "dang nhan" xuyen qua IT NHAT 1 ranh gioi frame, dam bao
# IsKeyPressed() bat duoc canh nay on dinh.
press() {
    DISPLAY="$DISPLAY_NUM" xdotool keydown --window "$win_id" --clearmodifiers "$1"
    sleep 0.2
    DISPLAY="$DISPLAY_NUM" xdotool keyup --window "$win_id" --clearmodifiers "$1"
}

snap() {
    local name="$1" settle="${2:-0.4}"
    sleep "$settle" # Cho 1 vai frame de UI on dinh (fade/pulse) truoc khi bat hinh
    DISPLAY="$DISPLAY_NUM" import -window "$win_id" "$OUT_DIR/$name.png"
    echo "  -> $OUT_DIR/$name.png"
}

echo "Dang chup man hinh..."
snap "01_menu" 1.5      # Cho lau hon o buoc dau: LoadFontEx()/SpriteSheet::Load()/AudioSystem::Init() chay 1 lan luc InitLevel/Run

press "Return"           # ENTER (Confirm) tu MENU -> PLAYING (co the co fade - xem RequestTransition)
snap "02_playing" 0.6

press "p"                 # settings.keyPause mac dinh = KEY_P -> PLAYING -> PAUSED
snap "03_paused" 0.4

press "k"                 # KEY_K, chi co y nghia luc PAUSED -> KEYBIND
snap "04_keybind" 0.4

echo "Xong - ${OUT_DIR}"
