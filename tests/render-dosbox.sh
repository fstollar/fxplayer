#!/usr/bin/env bash
# Render a module to WAV by running FX.EXE in DOSBox-X.
#
# Usage: tests/render-dosbox.sh [--native] [-t seconds] <module> [output.wav]
#
# --native     Use FX.EXE's built-in WAV writer (-w:/-n: switches).
#              No soundcard emulation or PulseAudio capture needed.
#              Renders as fast as possible — deterministic output.
#              This is the preferred mode for reference renders.
#
# (default)    Capture audio via parecord from the PulseAudio/PipeWire default
#              sink monitor. DOSBox-X SB16 emulation → SDL audio → PA sink.
#              Required: parecord (pulseaudio-utils).
#
# -t seconds   Max render/recording time (default 120). Required for looping
#              modules in either mode; non-looping modules exit on their own.
#
# Requirements: dosbox-x (Flatpak or system)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK_DIR="$REPO_ROOT/_work"

DURATION=120
MODE=pulseaudio   # "pulseaudio" or "native"

# Parse options
while [[ $# -gt 0 && "$1" == -* ]]; do
    case "$1" in
        -t)       DURATION="$2"; shift 2 ;;
        --native) MODE=native; shift ;;
        *)        echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

if [ $# -lt 1 ]; then
    echo "Usage: $0 [--native] [-t seconds] <module.s3m|.mod|.669> [output.wav]" >&2
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
trap 'rm -f "$TMPCONF"' EXIT

# Locate dosbox-x: use flatpak run (not the export wrapper which exits early).
if command -v dosbox-x &>/dev/null && [[ "$(command -v dosbox-x)" != */flatpak/exports/* ]]; then
    DOSBOX_CMD=dosbox-x
else
    DOSBOX_CMD="flatpak run com.dosbox_x.DOSBox-X"
fi

echo "==> Rendering: $MODULE_FILE  (max ${DURATION}s, mode=${MODE})"
echo "    Work dir: $WORK_DIR"
echo "    Output:   $OUTPUT_WAV"

# ---------------------------------------------------------------------------
# Native WAV mode: FX.EXE writes the WAV itself — no audio device needed.
# ---------------------------------------------------------------------------
if [ "$MODE" = native ]; then
    # DOS 8.3 temp name on the C: drive (= WORK_DIR on the host).
    DOSOUT="FXOUT.WAV"
    HOST_OUT="$WORK_DIR/$DOSOUT"
    rm -f "$HOST_OUT"

    # If the module isn't already in WORK_DIR, copy it there temporarily.
    COPIED_MODULE=0
    if [ "$(realpath "$MODULE_DIR")" != "$(realpath "$WORK_DIR")" ]; then
        cp "$MODULE_PATH" "$WORK_DIR/$MODULE_FILE"
        COPIED_MODULE=1
    fi

    cat > "$TMPCONF" <<EOF
[dosbox]
fastbioslogo = true
startbanner = false

[cpu]
core = dynamic
cycles = max

[autoexec]
mount c $WORK_DIR
c:
FX /w:$DOSOUT /n:$DURATION $MODULE_FILE
exit
EOF

    $DOSBOX_CMD -conf "$TMPCONF" &
    DBPID=$!

    ELAPSED=0
    while [ "$ELAPSED" -lt "$DURATION" ]; do
        kill -0 "$DBPID" 2>/dev/null || { echo "    DOSBox-X exited after ${ELAPSED}s"; break; }
        sleep 1
        ELAPSED=$((ELAPSED + 1))
    done

    if [ "$ELAPSED" -ge "$DURATION" ]; then
        echo "    Timeout after ${DURATION}s — killing DOSBox-X"
    fi

    kill -- -"$DBPID" 2>/dev/null || true
    sleep 1
    kill -9 -- -"$DBPID" 2>/dev/null || true
    pkill -9 -x "dosbox-x" 2>/dev/null || true
    wait "$DBPID" 2>/dev/null || true

    if [ $COPIED_MODULE -eq 1 ]; then
        rm -f "$WORK_DIR/$MODULE_FILE"
    fi

    if [ ! -s "$HOST_OUT" ]; then
        echo "ERROR: $HOST_OUT is missing or empty — did FX.EXE start?" >&2
        exit 1
    fi

    mv "$HOST_OUT" "$OUTPUT_WAV"
    echo "==> Output: $OUTPUT_WAV"
    sha256sum "$OUTPUT_WAV"
    exit 0
fi

# ---------------------------------------------------------------------------
# PulseAudio capture mode (original behaviour).
# ---------------------------------------------------------------------------
TMPWAV="$(mktemp "$HOME/.fx-capture-XXXXXX.wav")"
trap 'rm -f "$TMPCONF" "$TMPWAV"' EXIT

# If the module isn't already in WORK_DIR, copy it there temporarily.
COPIED_MODULE=0
if [ "$(realpath "$MODULE_DIR")" != "$(realpath "$WORK_DIR")" ]; then
    cp "$MODULE_PATH" "$WORK_DIR/$MODULE_FILE"
    COPIED_MODULE=1
fi

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
c:
FX /t:1 /d:5 /i:7 $MODULE_FILE
exit
EOF

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

# Kill DOSBox-X completely. The Flatpak sandbox creates a bwrap process tree:
#   flatpak run (DBPID) → bwrap → dosbox-x (inner binary)
# Killing DBPID alone orphans the inner process. We need three steps:
#   1. Kill the entire process group (catches flatpak run + bwrap + dosbox-x)
#   2. Escalate to SIGKILL after 1s
#   3. pkill the inner binary by exact name as a final sweep
kill -- -"$DBPID" 2>/dev/null || true   # SIGTERM to whole process group
sleep 1
kill -9 -- -"$DBPID" 2>/dev/null || true  # SIGKILL the group
pkill -9 -x "dosbox-x" 2>/dev/null || true  # inner binary by exact name
wait "$DBPID" 2>/dev/null || true

# Stop recording; flush the final audio buffer.
sleep 0.5
kill "$PARECORD_PID" 2>/dev/null || true
wait "$PARECORD_PID" 2>/dev/null || true

if [ $COPIED_MODULE -eq 1 ]; then
    rm -f "$WORK_DIR/$MODULE_FILE"
fi

if [ ! -s "$TMPWAV" ]; then
    echo "ERROR: captured WAV is empty — is SB16 emulation working?" >&2
    exit 1
fi

mv "$TMPWAV" "$OUTPUT_WAV"
echo "==> Output: $OUTPUT_WAV"
sha256sum "$OUTPUT_WAV"
