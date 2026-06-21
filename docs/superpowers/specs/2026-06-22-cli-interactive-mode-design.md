# CLI Interactive Mode — Design Spec

**Date:** 2026-06-22  
**Status:** Approved

## Summary

Add keyboard-driven transport controls to the `fxplayer` CLI host: pause/resume, quit, order-list jump, and volume. Terminal raw mode via POSIX termios; no ncurses dependency.

## Key Bindings

| Key(s)         | Action              |
|----------------|---------------------|
| `Space`        | Pause / Resume      |
| `q` / `Esc`    | Quit                |
| `→` / `.`      | Jump order forward  |
| `←` / `,`      | Jump order backward |
| `↑` / `+`      | Volume up           |
| `↓` / `-`      | Volume down         |

## 1. Core API additions

**Files:** `src/core/include/fx/fx.h`, `src/core/engine/fx.c`, `src/core/mixer/mixer_scalar.c`

### Volume

```c
void    fx_set_volume(uint8_t vol);   /* 0–64; 64 = full */
uint8_t fx_get_volume(void);
```

- `fx_set_volume` updates `g_MasterVolume` then calls `mixer_calc_master_vol32()` to rebuild the soft-clip table. This is faithful to the original engine path — the soft-clip knee scales with volume rather than post-clipping the output.
- Range 0–64 matches the existing `g_MasterVolume` domain. UI step size: 4 per keypress (16 steps total).
- Initial volume is 64 (full) after `fx_load`.

### Order jump

```c
void fx_order_jump(int delta);   /* -1 = back, +1 = forward */
```

- Implemented in `fx.c` with a `switch (g_loaded_fmt)` branch mirroring the existing pattern.
- Writes directly to the format's `*_nextorder` variable (`S3M_nextorder`, `MOD_nextorder`, `M669_nextorder`), which the tick sequencer already respects at the next row boundary. No mid-row artifacts.
- Clamped to `[0, OrderNum-1]` per format. Jumping backward from order 0 stays at 0; jumping forward from the last order stays there.
- Also resets `*_nextrow` to 0 so the jump always lands at the top of the target pattern.

## 2. Terminal input layer

**New files:** `src/host/cli/tty.h`, `src/host/cli/tty.cpp`

Compiled only for hosted (non-DOS) builds via `CMakeLists.txt`.

### API

```cpp
void   tty_raw_enable();    // save termios, apply raw + noecho
void   tty_raw_disable();   // restore saved termios
FxKey  tty_read_key();      // non-blocking; returns KEY_NONE if no input
```

### `FxKey` enum

```cpp
enum class FxKey {
    NONE,
    PAUSE,
    QUIT,
    ORDER_FWD,
    ORDER_BWD,
    VOL_UP,
    VOL_DOWN
};
```

### Escape sequence handling

`tty_read_key` reads one byte non-blocking. If it sees `0x1b` (Esc):

1. Attempt a second non-blocking `read` for `[`.
2. If nothing follows → `KEY_QUIT` (bare Esc).
3. If `[` follows → read one more byte for the final character: `A`=up, `B`=down, `C`=right, `D`=left → map to `VOL_UP`, `VOL_DOWN`, `ORDER_FWD`, `ORDER_BWD`.

`stdin` is set to `O_NONBLOCK` after `tty_raw_enable`. All reads use `read(STDIN_FILENO, ...)` directly.

### Cleanup

`tty_raw_disable()` is called:
- At normal exit before `return 0`.
- In the `SIGINT`/`SIGTERM` signal handler (after setting `g_stop = true`).

## 3. `main.cpp` changes

### Startup hint

After `ma_device_start`, print one line:

```
[space] pause  [←/,  →/.] order  [↑/+  ↓/-] volume  [q/Esc] quit
```

### Pause

Add `std::atomic<bool> g_paused{false}` to `AudioCtx`. In `data_callback`, when `ctx->paused` is true, output silence and return without calling `fx_render_frames` — engine state does not advance.

### Poll loop

Replace the bare `sleep_for(50ms)` loop with:

```cpp
while (!g_stop) {
    switch (tty_read_key()) {
    case FxKey::PAUSE:     ctx.paused ^= 1;    break;
    case FxKey::QUIT:      g_stop = true;      break;
    case FxKey::ORDER_FWD: fx_order_jump(+1);  break;
    case FxKey::ORDER_BWD: fx_order_jump(-1);  break;
    case FxKey::VOL_UP:    fx_set_volume(std::min(64, fx_get_volume() + 4)); break;
    case FxKey::VOL_DOWN:  fx_set_volume(std::max(0,  fx_get_volume() - 4)); break;
    default: break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
```

`tty_raw_enable()` is called just before this loop; `tty_raw_disable()` just after.

## 4. CMake

`tty.cpp` added to the `fxplayer` target in `src/host/cli/CMakeLists.txt`. No new dependencies — POSIX termios is available on all hosted targets (Linux, macOS).

## Out of scope

- Status line / live playback position display (CLI UX milestone).
- Windows console input (`SetConsoleMode` / `ReadConsoleInput`) — deferred; the feature is POSIX-only for now.
- ncurses.
