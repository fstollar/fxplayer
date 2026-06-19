#!/usr/bin/env bash
# Render a module to WAV by running FX.EXE in DOSBox-X.
#
# Usage: tests/render-dosbox.sh [-t seconds] <module> [output.wav]
#
# -t seconds   Max recording time (default 120). Required for looping modules.
#              FX.EXE exits automatically for non-looping ones.
#
# Audio is captured via parecord on the PulseAudio/PipeWire default sink monitor.
# DOSBox-X audio goes through PulseAudio (SB16 emulation → SDL audio → PA sink).
# No xdotool or window manipulation required.
#
# Requirements: dosbox-x (Flatpak), parecord (pulseaudio-utils)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK_DIR="$REPO_ROOT/_work"

DURATION=120

# Parse -t option
while [[ $# -gt 0 && "$1" == -* ]]; do
    case "$1" in
        -t) DURATION="$2"; shift 2 ;;
        *)  echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

if [ $# -lt 1 ]; then
    echo "Usage: $0 [-t seconds] <module.s3m|.mod|.669> [output.wav]" >&2
    exit 1
fi

MODULE_PATH="$(realpath "$1")"
MODULE_DIR="$(dirname "$MODULE_PATH")"
MODULE_FILE="$(basename "$MODULE_PATH")"
MODULE_STEM="${MODULE_FILE%.*}"

OUTPUT_WAV="${2:-$REPO_ROOT/tests/reference_renders/${MODULE_STEM}.wav}"
mkdir -p "$(dirname "$OUTPUT_WAV")"

# Kill any lingering DOSBox-X from a previous failed run.
pkill -f "com.dosbox_x.DOSBox-X" 2>/dev/null || true
sleep 0.5

TMPCONF="$(mktemp "$HOME/.fx-dosbox-XXXXXX.conf")"
TMPWAV="$(mktemp "$HOME/.fx-capture-XXXXXX.wav")"
trap 'rm -f "$TMPCONF" "$TMPWAV"' EXIT

cat > "$TMPCONF" <<EOF
[dosbox]
fastbioslogo = true
startbanner = false

[cpu]
core = dynamic
cycles = max

[sblaster]
sbtype = sb16
sbbase = 220
irq = 7
dma = 1
hdma = 5

[autoexec]
mount c $WORK_DIR
mount d $MODULE_DIR
c:
FX /t:1 /d:5 /i:7 D:\\$MODULE_FILE
exit
EOF

# Locate dosbox-x: use flatpak run (not the export wrapper which exits early).
if command -v dosbox-x &>/dev/null && [[ "$(command -v dosbox-x)" != */flatpak/exports/* ]]; then
    DOSBOX_CMD=dosbox-x
else
    DOSBOX_CMD="flatpak run com.dosbox_x.DOSBox-X"
fi

echo "==> Rendering: $MODULE_FILE  (max ${DURATION}s)"
echo "    Work dir: $WORK_DIR"
echo "    Output:   $OUTPUT_WAV"

# Start recording the default sink monitor BEFORE DOSBox-X opens,
# so we don't miss the first note.
# Use the explicit monitor name — @DEFAULT_MONITOR@ does not resolve reliably
# under PipeWire's PulseAudio compatibility layer.
MONITOR="$(pactl get-default-sink).monitor"
echo "    Monitor:  $MONITOR"
parecord --file-format=wav -d "$MONITOR" "$TMPWAV" &
PARECORD_PID=$!
sleep 0.3   # let parecord connect to PulseAudio

# Start DOSBox-X.
$DOSBOX_CMD -conf "$TMPCONF" &
DBPID=$!

# Wait for DOSBox-X to exit on its own (non-looping module) OR hit the timeout.
ELAPSED=0
while [ "$ELAPSED" -lt "$DURATION" ]; do
    kill -0 "$DBPID" 2>/dev/null || break   # flatpak wrapper exited cleanly
    sleep 1
    ELAPSED=$((ELAPSED + 1))
done

if [ "$ELAPSED" -ge "$DURATION" ]; then
    echo "    Timeout after ${DURATION}s — killing DOSBox-X"
fi

# Kill DOSBox-X: SIGTERM the flatpak wrapper, wait, then SIGKILL if needed.
# Use the Flatpak app ID in the pattern — NOT a generic "dosbox" substring,
# which would match this script's own path (render-dosbox.sh).
kill "$DBPID" 2>/dev/null || true
sleep 1
kill -0 "$DBPID" 2>/dev/null && kill -9 "$DBPID" 2>/dev/null || true
pkill -9 -f "com.dosbox_x.DOSBox-X" 2>/dev/null || true
wait "$DBPID" 2>/dev/null || true

# Stop recording; flush the final audio buffer.
sleep 0.5
kill "$PARECORD_PID" 2>/dev/null || true
wait "$PARECORD_PID" 2>/dev/null || true

if [ ! -s "$TMPWAV" ]; then
    echo "ERROR: captured WAV is empty — is SB16 emulation working?" >&2
    exit 1
fi

mv "$TMPWAV" "$OUTPUT_WAV"
echo "==> Output: $OUTPUT_WAV"
sha256sum "$OUTPUT_WAV"
