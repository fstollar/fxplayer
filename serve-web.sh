#!/usr/bin/env bash
# serve-web.sh — build the web host and serve it locally for development.
# Usage: ./serve-web.sh [port]
#
# Compiles fxcore.wasm, stages all web assets to a temp directory, then
# starts a Python HTTP server. AudioWorklet requires HTTP (not file://).
set -euo pipefail

PORT="${1:-8765}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEB_SRC="$SCRIPT_DIR/src/host/web"
STAGE_DIR="$(mktemp -d)"

trap "rm -rf '$STAGE_DIR'" EXIT

# Compile (output goes to repo root as fxcore.wasm).
"$SCRIPT_DIR/build-web.sh" --no-deploy

# Stage all assets together so relative fetches (fxcore.wasm, fx-worklet.js, etc.) resolve.
cp "$SCRIPT_DIR/fxcore.wasm"    "$STAGE_DIR/"
cp "$WEB_SRC/fx-worklet.js"     "$STAGE_DIR/"
cp "$WEB_SRC/fx-main.js"        "$STAGE_DIR/"
cp "$WEB_SRC/index.html"        "$STAGE_DIR/"
cp -r "$WEB_SRC/modules"        "$STAGE_DIR/"

echo ""
echo "  Serving at http://localhost:$PORT"
echo "  Ctrl-C to stop"
echo ""
python3 -m http.server "$PORT" --directory "$STAGE_DIR"
