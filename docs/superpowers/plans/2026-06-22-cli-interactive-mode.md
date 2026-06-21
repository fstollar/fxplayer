# CLI Interactive Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add keyboard-driven transport controls (pause, quit, order jump, volume) to the `fxplayer` CLI host using POSIX termios raw mode.

**Architecture:** Three layers: (1) new public core API functions in `fx.h`/`fx.c` for volume and order-jump; (2) a small POSIX TTY helper (`tty.h`/`tty.cpp`) that puts the terminal in raw mode and returns typed `FxKey` values; (3) `main.cpp` changes that wire the two together via command atomics in `AudioCtx` to safely cross the main-thread / audio-callback boundary.

**Tech Stack:** C99 core (no new deps), C++17 host, POSIX termios, miniaudio.

## Global Constraints

- No ncurses, no extra library dependencies.
- `tty.cpp` is POSIX-only — compiled only for hosted (non-DOS) targets; excluded from `makefile.dos`.
- C99 core files (`fx.h`, `fx.c`) use C99 only — no C++ constructs.
- Engine state is modified exclusively from the audio callback thread — the main thread posts commands via `std::atomic` slots in `AudioCtx` and the callback applies them before rendering.
- Volume range exposed to the UI is 0–64 (64 = full). Internally this scales the per-format `g_MasterVolume` value (which is format-specific and NOT in 0–64 range). The per-format value is preserved as a base; UI volume scales it proportionally.
- UI volume step: 4 per keypress (16 steps across the 0–64 range).
- Order jump is clamped to `[0, OrderNum-1]` per format. Jumping beyond the ends is a no-op.
- Key bindings: Space=pause, q/Esc=quit, →/.=order fwd, ←/,=order bwd, ↑/+=vol up, ↓/-=vol down.

---

## File Map

| File | Action | Responsibility |
|---|---|---|
| `src/core/include/fx/fx.h` | Modify | Add `fx_set_volume`, `fx_get_volume`, `fx_order_jump` declarations |
| `src/core/engine/fx.c` | Modify | Implement the three new functions; add `g_ui_volume` static |
| `src/host/cli/tty.h` | Create | `FxKey` enum + `tty_raw_enable/disable/read_key` declarations |
| `src/host/cli/tty.cpp` | Create | POSIX termios implementation |
| `src/host/cli/CMakeLists.txt` | Modify | Add `tty.cpp` to the `fxplayer` target |
| `src/host/cli/main.cpp` | Modify | Interactive loop, pause, command dispatch via `AudioCtx` atomics |

---

## Task 1: Core volume API (`fx_set_volume` / `fx_get_volume`)

**Files:**
- Modify: `src/core/include/fx/fx.h`
- Modify: `src/core/engine/fx.c`

**Interfaces:**
- Produces:
  - `void fx_set_volume(uint8_t vol)` — sets UI volume 0–64, rebuilds master vol table
  - `uint8_t fx_get_volume(void)` — returns current UI volume

**Background — how the master vol table works:**

Each format loader sets `g_MasterVolume` from the module header (e.g. S3M: `header.master_volume * 256`; MOD: channel-count formula; 669: `12288`). It then calls `mixer_calc_master_vol32(g_MasterVolume, g_master_vol_table)` to populate the lookup table used by `mixer_convert_to_s16`. The table is allocated inside the workspace buffer, pointed to by `g_master_vol_table`. We must NOT change `g_MasterVolume` permanently — the formats re-read it on reload. Instead, `fx_set_volume` keeps the original value and calls `mixer_calc_master_vol32` with a scaled copy.

- [ ] **Step 1: Add declarations to `fx.h`**

In `src/core/include/fx/fx.h`, after the `fx_close` declaration (line 85), add:

```c
/*
 * UI volume control: 0 = silent, 64 = full (default after fx_load).
 * Scales the module's own master-volume table entry — the soft-clip knee
 * tracks volume correctly rather than attenuating post-clip output.
 * Thread note: call only from the audio callback thread (see host README).
 */
void    fx_set_volume(uint8_t vol);
uint8_t fx_get_volume(void);
```

- [ ] **Step 2: Implement in `fx.c`**

`fx.c` already includes `mixer/mixer_scalar.h` (needed for `FX_INTERNAL_BLOCK`, `g_MixSpeed`, `mixer_convert_to_s16`, etc.) — no new include is required.

Add before `fx_version_string`:

```c
static uint8_t g_ui_volume = 64;

void fx_set_volume(uint8_t vol)
{
    if (vol > 64) vol = 64;
    g_ui_volume = vol;
    if (g_master_vol_table)
        mixer_calc_master_vol32(g_MasterVolume * vol / 64u, g_master_vol_table);
}

uint8_t fx_get_volume(void)
{
    return g_ui_volume;
}
```

Also reset `g_ui_volume` at the top of `fx_load`, before the format switch, so a reload returns to full volume:

```c
g_ui_volume = 64;
```

- [ ] **Step 3: Build to verify**

```bash
cmake --build build/ --target fxplayer 2>&1 | tail -5
```

Expected: clean build, no errors.

- [ ] **Step 4: Commit**

```bash
git add src/core/include/fx/fx.h src/core/engine/fx.c
git commit -m "feat(core): add fx_set_volume / fx_get_volume to public API"
```

---

## Task 2: Core order-jump API (`fx_order_jump`)

**Files:**
- Modify: `src/core/include/fx/fx.h`
- Modify: `src/core/engine/fx.c`

**Interfaces:**
- Consumes: `g_loaded_fmt`, `S3M_jump`, `S3M_nextorder`, `S3M_nextrow`, `S3M_OrderNum` (from `format/s3m.h`); `MOD_jump`, `MOD_nextorder`, `MOD_nextrow`, `MOD_OrderNum` (from `format/mod.h`); `M669_Order`, `M669_Pattern`, `M669_OrderNum`, `M669_Orderlist`, `M669_row` (from `format/m669.h`)
- Produces: `void fx_order_jump(int delta)`

**Background — per-format jump mechanics:**

- **S3M / MOD**: both have a deferred-jump mechanism: set `*_jump = 1`, `*_nextorder = target`, `*_nextrow = 0`. The sequencer's `*_goRowOrder()` picks this up at the next row boundary — no mid-row glitch.
- **669**: no deferred mechanism. `M669_Order`, `M669_Pattern`, and `M669_row` are written directly. Since `fx_order_jump` will be called from the audio callback (see Task 4), there is no data race.

- [ ] **Step 1: Add declaration to `fx.h`**

After the `fx_get_volume` declaration added in Task 1, add:

```c
/*
 * Jump the order list by delta (-1 = back, +1 = forward).
 * Clamped to [0, OrderNum-1]. Takes effect at the next row boundary
 * (S3M/MOD) or immediately (669). Call only from the audio callback thread.
 */
void fx_order_jump(int delta);
```

- [ ] **Step 2: Add includes to `fx.c`**

`fx.c` already includes `format/s3m.h`, `format/mod.h`, `format/m669.h`. Confirm they are present at the top of the file — no changes needed if so.

- [ ] **Step 3: Implement `fx_order_jump` in `fx.c`**

Add after `fx_get_volume`:

```c
void fx_order_jump(int delta)
{
    int target;

    switch (g_loaded_fmt) {

    case FX_FORMAT_S3M:
        target = (int)S3M_Order + delta;
        if (target < 0) target = 0;
        if (target >= (int)S3M_OrderNum) target = (int)S3M_OrderNum - 1;
        S3M_nextorder = (uint32_t)target;
        S3M_nextrow   = 0;
        S3M_jump      = 1;
        break;

    case FX_FORMAT_MOD:
        target = (int)MOD_Order + delta;
        if (target < 0) target = 0;
        if (target >= (int)MOD_OrderNum) target = (int)MOD_OrderNum - 1;
        MOD_nextorder = (uint32_t)target;
        MOD_nextrow   = 0;
        MOD_jump      = 1;
        break;

    case FX_FORMAT_669:
        target = (int)M669_Order + delta;
        if (target < 0) target = 0;
        if (target >= (int)M669_OrderNum) target = (int)M669_OrderNum - 1;
        M669_Order   = (uint32_t)target;
        M669_Pattern = M669_Orderlist[target];
        M669_row     = 0;
        break;

    default:
        break;
    }
}
```

Note: `MOD_OrderNum` is declared as `uint8_t MOD_OrderNum` in `mod.h`; cast to `int` for the comparison. Same for `M669_OrderNum`.

- [ ] **Step 4: Build to verify**

```bash
cmake --build build/ --target fxplayer 2>&1 | tail -5
```

Expected: clean build, no errors. Run existing CTests to confirm no regression:

```bash
ctest --test-dir build/ -V 2>&1 | tail -20
```

Expected: all 6 tests pass.

- [ ] **Step 5: Commit**

```bash
git add src/core/include/fx/fx.h src/core/engine/fx.c
git commit -m "feat(core): add fx_order_jump to public API"
```

---

## Task 3: TTY input layer + CMake

**Files:**
- Create: `src/host/cli/tty.h`
- Create: `src/host/cli/tty.cpp`
- Modify: `src/host/cli/CMakeLists.txt`

**Interfaces:**
- Produces:
  - `enum class FxKey { NONE, PAUSE, QUIT, ORDER_FWD, ORDER_BWD, VOL_UP, VOL_DOWN }`
  - `void tty_raw_enable()`
  - `void tty_raw_disable()`
  - `FxKey tty_read_key()` — non-blocking, returns `FxKey::NONE` if no input

**Escape sequence disambiguation:**

`tty_read_key` reads one byte. If it's `0x1b` (Esc), it attempts a second non-blocking read:
- Nothing follows → `FxKey::QUIT` (bare Esc key)
- `[` follows → read one more byte: `A`=`VOL_UP`, `B`=`VOL_DOWN`, `C`=`ORDER_FWD`, `D`=`ORDER_BWD`
- Anything else → `FxKey::NONE` (discard unknown escape sequence)

`stdin` fd is set to `O_NONBLOCK` in `tty_raw_enable` and restored in `tty_raw_disable`.

- [ ] **Step 1: Create `tty.h`**

Create `src/host/cli/tty.h`:

```cpp
#pragma once

enum class FxKey {
    NONE,
    PAUSE,
    QUIT,
    ORDER_FWD,
    ORDER_BWD,
    VOL_UP,
    VOL_DOWN
};

void   tty_raw_enable();
void   tty_raw_disable();
FxKey  tty_read_key();
```

- [ ] **Step 2: Create `tty.cpp`**

Create `src/host/cli/tty.cpp`:

```cpp
#include "tty.h"

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static struct termios s_saved_termios;
static int            s_saved_flags = -1;

void tty_raw_enable()
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &s_saved_termios);
    raw = s_saved_termios;
    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    s_saved_flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, s_saved_flags | O_NONBLOCK);
}

void tty_raw_disable()
{
    if (s_saved_flags >= 0) {
        fcntl(STDIN_FILENO, F_SETFL, s_saved_flags);
        s_saved_flags = -1;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &s_saved_termios);
}

FxKey tty_read_key()
{
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1)
        return FxKey::NONE;

    switch (c) {
    case ' ':  return FxKey::PAUSE;
    case 'q':  return FxKey::QUIT;
    case ',':  return FxKey::ORDER_BWD;
    case '.':  return FxKey::ORDER_FWD;
    case '+':  return FxKey::VOL_UP;
    case '-':  return FxKey::VOL_DOWN;

    case 0x1b: {
        unsigned char seq[2] = {0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return FxKey::QUIT;   /* bare Esc */
        if (seq[0] != '[')
            return FxKey::NONE;   /* unknown escape */
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return FxKey::NONE;
        switch (seq[1]) {
        case 'A': return FxKey::VOL_UP;     /* arrow up    */
        case 'B': return FxKey::VOL_DOWN;   /* arrow down  */
        case 'C': return FxKey::ORDER_FWD;  /* arrow right */
        case 'D': return FxKey::ORDER_BWD;  /* arrow left  */
        default:  return FxKey::NONE;
        }
    }

    default: return FxKey::NONE;
    }
}
```

- [ ] **Step 3: Update `CMakeLists.txt`**

In `src/host/cli/CMakeLists.txt`, change:

```cmake
add_executable(fxplayer
    miniaudio.c
    main.cpp
)
```

to:

```cmake
add_executable(fxplayer
    miniaudio.c
    main.cpp
    tty.cpp
)
```

- [ ] **Step 4: Build to verify**

```bash
cmake --build build/ --target fxplayer 2>&1 | tail -5
```

Expected: clean build, `tty.cpp` compiled and linked.

- [ ] **Step 5: Commit**

```bash
git add src/host/cli/tty.h src/host/cli/tty.cpp src/host/cli/CMakeLists.txt
git commit -m "feat(cli): add POSIX termios TTY input layer"
```

---

## Task 4: Interactive main loop

**Files:**
- Modify: `src/host/cli/main.cpp`

**Interfaces:**
- Consumes: `FxKey`, `tty_raw_enable`, `tty_raw_disable`, `tty_read_key` (from `tty.h`); `fx_set_volume`, `fx_get_volume`, `fx_order_jump` (from `fx/fx.h`)

**Thread safety design:**

`fx_set_volume`, `fx_get_volume`, and `fx_order_jump` must only be called from the audio callback thread (they touch engine global state that `fx_render_frames` also touches). The main thread posts intentions via three new atomics in `AudioCtx`:

- `std::atomic<int> vol_cmd{-1}` — -1 = no pending, 0–64 = new volume to apply
- `std::atomic<uint8_t> cur_vol{64}` — current volume; written by callback after applying, read by main thread for ±4 calculation
- `std::atomic<int> order_cmd{0}` — 0 = no pending, -1 = back, +1 = forward

The callback reads and clears these with `exchange` at the start of each call, before `fx_render_frames`.

- [ ] **Step 1: Update `AudioCtx` and `data_callback`**

Replace the current `AudioCtx` struct and `data_callback` function with:

```cpp
struct AudioCtx {
    fx_config cfg;
    std::atomic<bool>    done{false};
    std::atomic<bool>    paused{false};
    std::atomic<int>     vol_cmd{-1};
    std::atomic<uint8_t> cur_vol{64};
    std::atomic<int>     order_cmd{0};
};

static void data_callback(ma_device *dev, void *out, const void * /*in*/, ma_uint32 frame_count)
{
    auto *ctx = static_cast<AudioCtx *>(dev->pUserData);

    /* Apply pending commands before any rendering */
    int vol = ctx->vol_cmd.exchange(-1, std::memory_order_relaxed);
    if (vol >= 0) {
        fx_set_volume(static_cast<uint8_t>(vol));
        ctx->cur_vol.store(static_cast<uint8_t>(vol), std::memory_order_relaxed);
    }
    int ord = ctx->order_cmd.exchange(0, std::memory_order_relaxed);
    if (ord != 0)
        fx_order_jump(ord);

    if (ctx->paused.load(std::memory_order_relaxed) || ctx->done.load(std::memory_order_relaxed)) {
        std::memset(out, 0, frame_count * CHANNELS * sizeof(int16_t));
        return;
    }

    size_t rendered = fx_render_frames(out, frame_count, &ctx->cfg);
    if (rendered < frame_count) {
        size_t tail = (frame_count - rendered) * CHANNELS * sizeof(int16_t);
        std::memset(static_cast<int16_t *>(out) + rendered * CHANNELS, 0, tail);
        ctx->done  = true;
        g_stop     = true;
    }
}
```

- [ ] **Step 2: Add `#include "tty.h"` to `main.cpp`**

After the existing includes at the top of `main.cpp`, add:

```cpp
#include "tty.h"
```

- [ ] **Step 3: Update signal handler to restore terminal**

Replace:

```cpp
static void on_signal(int) { g_stop = true; }
```

with:

```cpp
static void on_signal(int) { g_stop = true; tty_raw_disable(); }
```

- [ ] **Step 4: Replace the main play loop**

Find the block in `main` starting at:

```cpp
std::printf("Playing %s — press Ctrl+C to stop\n", argv[1]);
ma_device_start(&device);

while (!g_stop) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
```

Replace it with:

```cpp
std::printf("Playing: %s\n", argv[1]);
std::printf("[space] pause  [\xe2\x86\x90/,  \xe2\x86\x92/.] order  [\xe2\x86\x91/+  \xe2\x86\x93/-] volume  [q/Esc] quit\n");

ma_device_start(&device);
tty_raw_enable();

while (!g_stop) {
    switch (tty_read_key()) {
    case FxKey::PAUSE:
        ctx.paused.store(!ctx.paused.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
        break;
    case FxKey::QUIT:
        g_stop = true;
        break;
    case FxKey::ORDER_FWD:
        ctx.order_cmd.store(+1, std::memory_order_relaxed);
        break;
    case FxKey::ORDER_BWD:
        ctx.order_cmd.store(-1, std::memory_order_relaxed);
        break;
    case FxKey::VOL_UP: {
        int v = static_cast<int>(ctx.cur_vol.load(std::memory_order_relaxed)) + 4;
        if (v > 64) v = 64;
        ctx.vol_cmd.store(v, std::memory_order_relaxed);
        break;
    }
    case FxKey::VOL_DOWN: {
        int v = static_cast<int>(ctx.cur_vol.load(std::memory_order_relaxed)) - 4;
        if (v < 0) v = 0;
        ctx.vol_cmd.store(v, std::memory_order_relaxed);
        break;
    }
    default: break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

tty_raw_disable();
```

Note: `\xe2\x86\x90` etc. are UTF-8 for ←→↑↓.

- [ ] **Step 5: Build**

```bash
cmake --build build/ --target fxplayer 2>&1 | tail -5
```

Expected: clean build.

- [ ] **Step 6: Run CTests to confirm no regression**

```bash
ctest --test-dir build/ -V 2>&1 | tail -20
```

Expected: all 6 tests pass (render output is not affected by interactive mode).

- [ ] **Step 7: Manual smoke test**

```bash
./build/fxplayer tests/reference_renders/TEST.S3M
```

Verify:
- Hint line is printed
- Music plays
- Space pauses and resumes (silence vs audio)
- `,`/`.` or arrow left/right jump order
- `+`/`-` or arrow up/down change volume (audible)
- `q` or Esc quits cleanly (terminal restored, no raw mode left behind)
- Ctrl+C quits cleanly (terminal restored)

- [ ] **Step 8: Commit**

```bash
git add src/host/cli/main.cpp
git commit -m "feat(cli): add interactive keyboard controls (pause/quit/order/volume)"
```
