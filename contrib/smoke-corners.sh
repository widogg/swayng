#!/bin/bash
# Headless check that only outermost tile corners round (needs wezterm, grim).
# Usage: contrib/smoke-corners.sh [build-dir]
set -eu
BUILD=$(realpath "${1:-build}")
TMP=$(mktemp -d)
trap 'kill $PID 2>/dev/null; rm -rf "$TMP"' EXIT

cat >"$TMP/config" <<'EOF'
output HEADLESS-1 resolution 1280x800
default_border normal 2
corner_radius 12
gaps inner 0
gaps outer 20
EOF

env -u WAYLAND_DISPLAY -u SWAYSOCK -u SWAYNGSOCK WLR_BACKENDS=headless \
	WLR_LIBINPUT_NO_DEVICES=1 WLR_RENDERER=gles2 \
	"$BUILD/sway/swayng" -c "$TMP/config" >"$TMP/log" 2>&1 &
PID=$!
SOCK=/run/user/$UID/swayng-ipc.$UID.$PID.sock
for _ in $(seq 50); do [ -S "$SOCK" ] && break; sleep 0.1; done
msg() { SWAYNGSOCK=$SOCK "$BUILD/swaymsg/swayngmsg" "$@" >/dev/null; }

msg "exec wezterm start --always-new-process"
sleep 3
msg "exec wezterm start --always-new-process"
sleep 3
# exec inherits the compositor's WAYLAND_DISPLAY, never the live session's
msg "exec grim -t ppm $TMP/shot.ppm"
for _ in $(seq 50); do [ -s "$TMP/shot.ppm" ] && break; sleep 0.1; done
sleep 0.5

# Two side-by-side tiles spanning x 20..1260, top edge at y 20.
python3 - "$TMP/shot.ppm" <<'EOF'
import sys
data = open(sys.argv[1], 'rb').read()
fields = data.split(maxsplit=4)
w = int(fields[1])
px = fields[4]
def rgb(x, y):
	i = (y * w + x) * 3
	return tuple(px[i:i + 3])
bg = rgb(5, 5)
checks = [
	((21, 21), True, 'outer top-left corner rounds'),
	((1258, 21), True, 'outer top-right corner rounds'),
	((639, 21), False, 'inner junction stays square (left tile)'),
	((641, 21), False, 'inner junction stays square (right tile)'),
]
ok = True
for (x, y), want_bg, what in checks:
	if (rgb(x, y) == bg) != want_bg:
		print(f'FAIL: {what} at {x},{y}: {rgb(x, y)}')
		ok = False
print('ok' if ok else 'failed')
sys.exit(0 if ok else 1)
EOF
