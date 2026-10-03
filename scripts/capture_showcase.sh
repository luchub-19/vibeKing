#!/usr/bin/env bash
#
# capture_showcase.sh (GD 0 - docs/GRAPHICS_UPGRADE_PLAN.md) - chup anh + do frame time cac
# CANH TRINH DIEN co dinh (--scene, xem src/launch_options.h), de moi thay doi do hoa co anh
# truoc/sau so duoc tren CUNG 1 bo cuc (luat R5) va so lieu hieu nang so voi moc (luat R4).
#
# Bo sung cho capture_screens.sh (4 man dieu huong bang phim, can xdotool): script nay KHONG
# gui phim nao - game tu dung canh, tu chup, tu thoat. Chi can Xvfb.
#
# CACH DUNG:
#   ./scripts/capture_showcase.sh [binary] [thu_muc_output] [so_frame_bench]
#     binary          Mac dinh <repo>/build/space_invaders
#     thu_muc_output  Mac dinh <repo>/screenshots (nam trong .gitignore)
#     so_frame_bench  Mac dinh 600; 0 = chi chup, khong do
#
# KET QUA: <output>/showcase_combat.png, showcase_boss.png va <output>/bench.txt
#
# LUU Y VE SO LIEU: trong Xvfb, OpenGL chay bang llvmpipe (render bang CPU) - frame time o day
# KHONG phai frame time tren iGPU that. Chi dung de so TUONG DOI: cung may, cung moi truong,
# truoc vs sau 1 thay doi. Moc tuyet doi tren may that do bang --bench chay truc tiep.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BINARY="${1:-$REPO_ROOT/build/space_invaders}"
OUT_DIR="${2:-$REPO_ROOT/screenshots}"
BENCH_FRAMES="${3:-600}"
SCENES=(combat boss)

if ! command -v Xvfb >/dev/null 2>&1; then
    echo "LOI: thieu Xvfb. Cai qua: sudo apt-get install -y xvfb" >&2
    exit 1
fi
if [ ! -x "$BINARY" ]; then
    echo "LOI: khong tim thay binary tai '$BINARY' - build truoc (xem CLAUDE.md)." >&2
    exit 1
fi
mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)" # Duong dan tuyet doi: game chay voi CWD = REPO_ROOT ben duoi

DISPLAY_NUM=":$((90 + RANDOM % 100))"
Xvfb "$DISPLAY_NUM" -screen 0 800x600x24 -nolisten tcp >/tmp/capture_showcase_xvfb.log 2>&1 &
XVFB_PID=$!
trap 'kill "$XVFB_PID" >/dev/null 2>&1 || true' EXIT INT TERM
sleep 1

# CWD = REPO_ROOT: game doc assets/ + level.cfg theo thu muc hien tai (xem CLAUDE.md "Chay game")
run_game() {
    ( cd "$REPO_ROOT" && DISPLAY="$DISPLAY_NUM" timeout 180 "$BINARY" "$@" 2>/dev/null )
}

for scene in "${SCENES[@]}"; do
    out="$OUT_DIR/showcase_$scene.png"
    run_game --scene="$scene" --capture="$out" >/dev/null
    [ -f "$out" ] && echo "  -> $out" || { echo "LOI: khong chup duoc canh $scene" >&2; exit 1; }
done

if [ "$BENCH_FRAMES" -gt 0 ]; then
    : > "$OUT_DIR/bench.txt"
    for scene in "${SCENES[@]}"; do
        line="$(run_game --scene="$scene" --bench="$BENCH_FRAMES" | grep '^BENCH' || true)"
        [ -n "$line" ] || { echo "LOI: bench canh $scene khong in ket qua" >&2; exit 1; }
        echo "$scene: $line" | tee -a "$OUT_DIR/bench.txt"
    done
fi
echo "Xong - $OUT_DIR"
