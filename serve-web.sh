#!/usr/bin/env bash
# serve-web.sh — build the web host and serve it locally for development.
# Usage: ./serve-web.sh [--network] [port]
#
#   --network   expose on LAN via HTTPS (AudioWorklet needs a secure context
#               on non-localhost origins; a self-signed cert is generated
#               automatically — click through the browser warning once)
#   port        default 8765
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

# Stage all assets together so relative fetches resolve.
cp "$SCRIPT_DIR/fxcore.wasm"    "$STAGE_DIR/"
cp "$WEB_SRC/fx-worklet.js"     "$STAGE_DIR/"
cp "$WEB_SRC/fx-main.js"        "$STAGE_DIR/"
cp "$WEB_SRC/index.html"        "$STAGE_DIR/"
cp -r "$WEB_SRC/modules"        "$STAGE_DIR/"

if [[ "$BIND" == "0.0.0.0" ]]; then
    LAN_IP="$(ip route get 1 2>/dev/null | awk '{print $7; exit}' || hostname -I | awk '{print $1}')"
    CERT="$STAGE_DIR/cert.pem"
    KEY="$STAGE_DIR/key.pem"

    # Generate a temporary self-signed certificate (valid 1 day).
    # AudioWorklet is blocked on plain HTTP for non-localhost origins.
    openssl req -x509 -newkey rsa:2048 \
        -keyout "$KEY" -out "$CERT" \
        -days 1 -nodes \
        -subj "/CN=$LAN_IP" 2>/dev/null

    # Write the HTTPS server to a file — avoids heredoc-to-stdin fragility.
    cat > "$STAGE_DIR/https_server.py" << 'PYEOF'
import sys, ssl, http.server, os, signal

port  = int(sys.argv[1])
bind  = sys.argv[2]
root  = sys.argv[3]
cert  = sys.argv[4]
key   = sys.argv[5]

os.chdir(root)
httpd = http.server.HTTPServer((bind, port), http.server.SimpleHTTPRequestHandler)
ctx   = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ctx.load_cert_chain(cert, key)
httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True)
try:
    httpd.serve_forever()
except KeyboardInterrupt:
    pass
PYEOF

    echo ""
    echo "  Serving at https://localhost:$PORT  (this machine)"
    echo "             https://$LAN_IP:$PORT  (LAN)"
    echo ""
    echo "  Browser will warn about the self-signed certificate —"
    echo "  click Advanced → Accept the Risk (Firefox)"
    echo "  or   Advanced → Proceed anyway  (Chrome/Edge)."
    echo ""
    echo "  Ctrl-C to stop"
    echo ""
    python3 "$STAGE_DIR/https_server.py" "$PORT" "$BIND" "$STAGE_DIR" "$CERT" "$KEY"

else
    echo ""
    echo "  Serving at http://localhost:$PORT"
    echo "  Ctrl-C to stop"
    echo ""
    python3 -m http.server "$PORT" --bind "$BIND" --directory "$STAGE_DIR"
fi
