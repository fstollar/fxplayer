#!/usr/bin/env bash
# serve-web.sh — build the web host and serve it locally for development.
# Usage: ./serve-web.sh [--network] [port]
#
#   --network   bind to 0.0.0.0 so other machines on the LAN can connect
#   port        default 8765
#
# Compiles fxcore.wasm, stages all web assets to a temp directory, then
# starts a Python HTTP server. AudioWorklet requires HTTP (not file://).
set -euo pipefail

BIND="127.0.0.1"
PORT="8765"

for arg in "$@"; do
    case "$arg" in
        --network) BIND="0.0.0.0" ;;
        *)         PORT="$arg"    ;;
    esac
done

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

if [[ "$BIND" == "0.0.0.0" ]]; then
    LAN_IP="$(ip route get 1 2>/dev/null | awk '{print $7; exit}' || hostname -I | awk '{print $1}')"
    echo ""
    echo "  Serving at http://localhost:$PORT  (this machine)"
    echo "             http://$LAN_IP:$PORT  (LAN)"
else
    echo ""
    echo "  Serving at http://localhost:$PORT"
fi
echo "  Ctrl-C to stop"
echo ""
python3 -m http.server "$PORT" --bind "$BIND" --directory "$STAGE_DIR"
