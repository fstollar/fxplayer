# Web Host Design — GitHub Pages Demo

**Date:** 2026-06-23
**Status:** Approved

## Goal

A self-contained static demo page hosted on GitHub Pages. Users can play a
bundled reference module or drag-and-drop their own `.s3m`/`.mod`/`.669` file.
Transport controls match the CLI: play/pause, stop, volume, order jump.

## Constraints

- GitHub Pages: no control over response headers → no `SharedArrayBuffer`
- Static hosting only: no server-side logic
- YAGNI: basic version first, iterate from there

## Toolchain

Bare `clang --target=wasm32-unknown-unknown -nostdlib`. No Emscripten, no Rust.
The C99 core's design (no malloc, no I/O, no OS headers) maps naturally to bare
Wasm. `memset`/`memcpy` polyfills provided in the shim.

## Deployment

Source on `main` branch in `src/host/web/`. Built artifacts on a separate
`gh-pages` branch (GitHub Pages serves from root of that branch).
`build-web.sh` compiles and deploys in one step via `git worktree`.

## File Layout

```
src/host/web/
├── fxcore_wasm.c     C shim: static arena, Wasm exports
├── fx-worklet.js     AudioWorkletProcessor (audio thread)
├── fx-main.js        Main thread: UI, file loading, commands
└── index.html        The page

gh-pages branch (deployed):
├── fxcore.wasm
├── fx-worklet.js
├── fx-main.js
├── index.html
└── modules/
    └── 64mania.s3m   bundled default module
```

## C Wasm Shim (`fxcore_wasm.c`)

Two static 4 MB arenas in Wasm linear memory:

- `g_module_buf` — JS copies raw module bytes here before calling `wasm_load()`
- `g_workspace` — engine workspace carved by `fx_load()`
- `g_render_buf` — S16 interleaved output; JS reads this after each render call

Exported functions:

| Export | Purpose |
|---|---|
| `wasm_module_buf()` | Returns pointer to module input buffer |
| `wasm_module_buf_size()` | Returns 4 MB capacity |
| `wasm_load(size)` | Calls `fx_workspace_size` + `fx_load`; returns `fx_err` |
| `wasm_render_buf()` | Returns pointer to S16 output buffer |
| `wasm_render_buf_frames()` | Returns frame capacity of render buffer |
| All `fx_*` from `fx.h` | Exported directly (no wrapping needed) |

## AudioWorklet (`fx-worklet.js`)

Wasm instance lives on the audio thread inside `AudioWorkletProcessor`. No
`SharedArrayBuffer` needed — audio data never crosses thread boundaries.

**Init:** Receives `.wasm` bytes via `MessagePort`, instantiates Wasm, signals
`ready`.

**Load:** Receives module bytes, copies to `wasm_module_buf()`, calls
`wasm_load()`, reads title, posts `{ type: 'loaded', title, err }`.

**`process()`:** Called every 128 frames.
- Calls `fx_render_frames(render_buf, 128, cfg)`
- Converts S16 interleaved → float32 deinterleaved (`sample / 32768.0`)
- Every ~19 blocks (~50 ms) posts playback state snapshot to main thread

**Commands received via port:** `init`, `load`, `play`, `pause`, `stop`,
`set_volume`, `order_jump`

Note: `fx_song_title()` returns a pointer into Wasm linear memory; JS reads it
via `TextDecoder` on `wasm.memory.buffer`.

## Main Thread (`fx-main.js`)

- Creates `AudioContext` + `AudioWorkletNode`
- On page load: `fetch('modules/64mania.s3m')` and auto-loads into worklet
- Drag-and-drop zone + file picker button for user modules
- Sends commands to worklet via `node.port.postMessage()`
- Receives state snapshots; updates UI (order, pattern, row, channels, loops,
  volume)

## UI (index.html)

Minimal, functional:
- Song title + format badge
- Play/Pause and Stop buttons
- Volume slider
- Order ◀ ▶ buttons
- Status line: order / pattern / row / active channels / loop count
- Drag-and-drop zone with file picker fallback
- Bundled module loads automatically on page open

## build-web.sh

```sh
# 1. Compile (list source dirs explicitly — avoids needing bash globstar)
SOURCES=$(find src/core -name '*.c') 
clang --target=wasm32-unknown-unknown -nostdlib \
  -Wl,--no-entry -Wl,--export-dynamic \
  -O2 -I src/core/include \
  -o /tmp/fxcore.wasm \
  $SOURCES src/host/web/fxcore_wasm.c

# 2. Deploy to gh-pages via git worktree
git worktree add /tmp/gh-pages gh-pages
cp /tmp/fxcore.wasm /tmp/gh-pages/
cp src/host/web/{fx-worklet.js,fx-main.js,index.html} /tmp/gh-pages/
mkdir -p /tmp/gh-pages/modules
cp src/host/web/modules/64mania.s3m /tmp/gh-pages/modules/
cd /tmp/gh-pages && git add -A && git commit -m "deploy web demo" && git push
git worktree remove /tmp/gh-pages
```

The bundled module (`src/host/web/modules/64mania.s3m`) is a copy of
`tests/_test_mods/64mania.s3m` committed into the web source tree so it travels with the code.

## Out of Scope (first version)

- Waveform / spectrum visualizer
- Pattern display
- Multiple bundled modules / playlist
- SIMD Wasm mixer variant
