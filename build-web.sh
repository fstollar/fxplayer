#!/usr/bin/env bash
# build-web.sh — compile F/X core to Wasm and deploy to gh-pages branch.
# Usage: ./build-web.sh [--no-deploy]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEB_SRC="$SCRIPT_DIR/src/host/web"
BUILD_WEB_DIR="$SCRIPT_DIR/build-web"
WASM_OUT="$BUILD_WEB_DIR/fxcore.wasm"

mkdir -p "$BUILD_WEB_DIR"

# ── compile ───────────────────────────────────────────────────────────
SOURCES=$(find "$SCRIPT_DIR/src/core" -name '*.c' | sort)

clang --target=wasm32-unknown-unknown -nostdlib -fno-builtin \
  -Wl,--no-entry -Wl,--export-all \
  -O2 \
  -I "$WEB_SRC/include" \
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

cp "$WASM_OUT"                             "$GH_PAGES_DIR/"
cp "$WEB_SRC/fx-worker.js"                 "$GH_PAGES_DIR/"
cp "$WEB_SRC/fx-worklet.js"                "$GH_PAGES_DIR/"
cp "$WEB_SRC/fx-main.js"                   "$GH_PAGES_DIR/"
cp "$WEB_SRC/index.html"                   "$GH_PAGES_DIR/"
mkdir -p "$GH_PAGES_DIR/modules"
cp "$WEB_SRC/modules/64mania.s3m"          "$GH_PAGES_DIR/modules/"
cp "$WEB_SRC/modules/skyrider.s3m"         "$GH_PAGES_DIR/modules/"
cp "$WEB_SRC/modules/hul.mod"              "$GH_PAGES_DIR/modules/"
cp "$WEB_SRC/modules/purple.669"           "$GH_PAGES_DIR/modules/"
cp "$WEB_SRC/modules/unreal ][.s3m"        "$GH_PAGES_DIR/modules/"

cd "$GH_PAGES_DIR"
git add -A
git commit -m "deploy: web demo $(date +%Y-%m-%d)"
# Force-push: gh-pages is always a complete deploy snapshot, never hand-edited.
# CI may have pushed between our last local deploy and now.
git push --force origin gh-pages

echo "Deployed to gh-pages."
