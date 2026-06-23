#!/usr/bin/env bash
# build-web.sh — compile F/X core to Wasm and deploy to gh-pages branch.
# Usage: ./build-web.sh [--no-deploy]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEB_SRC="$SCRIPT_DIR/src/host/web"
WASM_OUT="$SCRIPT_DIR/fxcore.wasm"

# ── compile ───────────────────────────────────────────────────────────
SOURCES=$(find "$SCRIPT_DIR/src/core" -name '*.c' | sort)

clang --target=wasm32-unknown-unknown -nostdlib -fno-builtin \
  -Wl,--no-entry -Wl,--export-all -Wl,--export=memory \
  -O2 \
  -I "$SCRIPT_DIR/src/core/include" \
  -I "$SCRIPT_DIR/src/core" \
  -o "$WASM_OUT" \
  $SOURCES "$WEB_SRC/fxcore_wasm.c"

echo "Compiled: $WASM_OUT ($(wc -c < "$WASM_OUT") bytes)"

[[ "${1:-}" == "--no-deploy" ]] && exit 0

# ── deploy to gh-pages ────────────────────────────────────────────────
GH_PAGES_DIR="$(mktemp -d)"
trap "git worktree remove --force '$GH_PAGES_DIR' 2>/dev/null; rm -rf '$GH_PAGES_DIR'" EXIT

git worktree add "$GH_PAGES_DIR" gh-pages

cp "$WASM_OUT"                              "$GH_PAGES_DIR/"
cp "$WEB_SRC/fx-worklet.js"                "$GH_PAGES_DIR/"
cp "$WEB_SRC/fx-main.js"                   "$GH_PAGES_DIR/"
cp "$WEB_SRC/index.html"                   "$GH_PAGES_DIR/"
mkdir -p "$GH_PAGES_DIR/modules"
cp "$WEB_SRC/modules/64mania.s3m"          "$GH_PAGES_DIR/modules/"

cd "$GH_PAGES_DIR"
git add -A
git commit -m "deploy: web demo $(date +%Y-%m-%d)"
git push origin gh-pages

echo "Deployed to gh-pages."
