# F/X Player — Change Log

## host/web — Worker + MessageChannel architecture (proper fix)

---

### 2026-06-24 — WASM moved to Web Worker; AudioWorklet is a dumb audio sink

The ring-buffer-in-worklet approach only masked the problem: even large buffers
eventually drain when Android thermally throttles the CPU, because the WASM
render was still on the audio thread with its 2.67 ms deadline.

The correct fix (described in the Chrome "Audio Worklet Design Pattern" article)
is to move all computation off the audio thread entirely.

#### Architecture

```
  Main thread        Worker thread         Audio thread
  ───────────        ─────────────         ────────────
  FxPlayer  ──cmd──▶ fx-worker.js         fx-worklet.js
            ◀─event─  │  owns WASM         │  queue only
                       │  renders 100ms chunks  │
                       └──Float32 chunks──▶ │  process()
                          (MessageChannel,       reads 128
                           zero-copy transfer)   frames/call
```

- **`fx-worker.js`** (new) — DedicatedWorkerGlobalScope; owns WASM; renders
  4800-frame (100 ms) chunks via `setInterval`; pre-fills 500 ms on load; sends
  chunks zero-copy via `MessageChannel` port directly to the AudioWorklet.
- **`fx-worklet.js`** (rewritten) — thin audio sink; no WASM; `process()` drains
  a chunk queue; zero allocations in the hot path.
- **`fx-main.js`** (rewritten) — creates Worker + MessageChannel; routes all
  commands to Worker; Worker sends state updates and events back to main.

#### Why this works where buffering didn't

The Worker thread has no deadline — it can take as long as it needs to render.
`process()` is now just a queue drain (array copy), well inside 2.67 ms even
on a heavily throttled Android CPU.

#### Files changed

| File | Change |
|---|---|
| `src/host/web/fx-worker.js` | New — Web Worker owning WASM + render loop |
| `src/host/web/fx-worklet.js` | Rewritten — queue consumer only |
| `src/host/web/fx-main.js` | Rewritten — orchestrates Worker + AudioWorklet |
| `build-web.sh` | Deploy `fx-worker.js` |
| `.github/workflows/deploy-web.yml` | Stage `fx-worker.js` |
| `ROADMAP.md` | Documents option 1 (SAB) for future reference |

---

## host/web — Android Chrome audio buffer size increase

---

### 2026-06-24 — RENDER_FRAMES 2048 → 8192 (170 ms lookahead)

42 ms lookahead was insufficient to absorb Android's thermal CPU throttling:
after a few seconds of sustained load the CPU slows 5–10×, the WASM render
exceeds the quantum budget, and the audio stutters then bursts.

Increased `RENDER_FRAMES` from 2048 to 8192 in `fxcore_wasm.c`. The
AudioWorklet already reads the buffer size dynamically via
`wasm_render_buf_frames()`, so it picks up 170 ms of lookahead automatically
with no JS changes. The static S16 buffer grows by 24 KB in WASM linear
memory; the `.wasm` binary is unaffected (BSS).

---

## host/web — Android Chrome audio performance fixes

---

### 2026-06-24 — AudioWorklet lookahead buffer; latencyHint; GC allocation fixes

Three separate issues caused the "plays in chunks, then 10× too fast" symptom
on Android Chrome.

#### Root causes

| # | Cause | Effect |
|---|---|---|
| 1 | `new Int16Array(...)` created on every `process()` call (~375×/sec) | GC pressure causes the audio thread to miss its 2.67 ms deadline → buffer starvation then catch-up burst |
| 2 | `new AudioContext()` without `latencyHint` defaults to `'interactive'` | Tightest possible deadline; Android's audio HAL needs more headroom |
| 3 | `wasm_render(128)` called every quantum | WASM call overhead paid 375×/sec instead of 23×/sec |

#### Fixes

| File | Change |
|---|---|
| `fx-worklet.js` | All typed-array views (`Int16Array` into WASM render buf, `Uint32Array` over playback state) pre-allocated once in `_init()` — zero per-callback allocations in the hot path |
| `fx-worklet.js` | 2048-frame Float32 lookahead buffer: WASM renders 2048 frames at once, `process()` copies 128 frames per call — WASM call rate reduced 16× |
| `fx-main.js` | `new AudioContext({ latencyHint: 'playback' })` — larger internal buffer, tolerant of GC jitter on low-end Android |

---

## core/ + host/web + host/cli — fx_init() API; Android sample-rate fix

---

### 2026-06-24 — fx_init(); correct TickLength on non-48 kHz devices

#### Root cause

`fx_load()` called format loaders that computed `TickLength` using `g_MixSpeed`,
but `g_MixSpeed` was only written inside `fx_render_frames()` — after the load.
On first load `g_MixSpeed` held its C global initializer (48000), so any device
whose `AudioContext.sampleRate` differed (e.g. Android at 44100 Hz) loaded with
the wrong tick length, playing at the wrong tempo.  The same latent bug existed
in the CLI for non-default `-r` rates.

#### Fix

New public API call `fx_init(const fx_config *cfg)` — mirrors the original
DOS initialisation pattern.  Must be called once before `fx_load`; applies
`sample_rate`, stereo, interpolation, and soft-clip flags to the mixer globals
immediately so format loaders see the correct mix rate.

| File | What changed |
|---|---|
| `src/core/include/fx/fx.h` | `void fx_init(const fx_config *cfg)` declared; `fx_load` doc updated |
| `src/core/engine/fx.c` | `fx_init` implemented; sets `g_MixSpeed` + flags from cfg |
| `src/host/web/fxcore_wasm.c` | `wasm_load` calls `fx_init(&g_config)` before `fx_load` |
| `src/host/cli/main.cpp` | `ctx.cfg` built before `fx_init` + `fx_load` (fixes latent CLI bug) |
| `tests/render/main.c` | `fx_init(&cfg)` added before `fx_load` |

6/6 CTests still pass bit-exact.

---

## host/web — browser demo page (bare Wasm32 + AudioWorklet)

---

### 2026-06-24 — Repo made public; GitHub Pages live

Repo set to public. Web demo is live at **https://fstollar.github.io/fxplayer**
(`gh-pages` branch, root). README updated with the demo link. Future deploys
via `./build-web.sh`.

---

### 2026-06-24 — GitHub repo created; gh-pages branch pushed

Private repo `https://github.com/fstollar/fxplayer` created. Both `main` and
`gh-pages` branches pushed (HTTPS remote).

---

### 2026-06-23 — Web host initial implementation

Self-contained static demo page. The C99 core is compiled to bare Wasm32 with
`clang --target=wasm32-unknown-unknown -nostdlib` (requires `lld-23`); no
Emscripten, no Rust, no npm. An `AudioWorkletProcessor` hosts the Wasm instance
on the audio thread and calls `fx_render_frames` every 128 frames, converting
S16 interleaved output to float32 stereo for Web Audio. GitHub Pages deploy
deferred; sources are in `src/host/web/` and locally tested.

#### Files added

| File | Role |
|---|---|
| `src/host/web/fxcore_wasm.c` | C shim: `memset`/`memcpy`/`memcmp` polyfills, 4 MB static arenas, `wasm_*` wrapper exports |
| `src/host/web/include/string.h` | Minimal `string.h` stub for the `-nostdlib` build |
| `src/host/web/fx-worklet.js` | `AudioWorkletProcessor`: Wasm init, render loop, play/pause/stop/volume/order commands |
| `src/host/web/fx-main.js` | Main thread: `AudioContext` setup, file loading, UI state |
| `src/host/web/index.html` | Demo page: play/pause toggle, stop, order nav, volume, module dropdown, drag-and-drop |
| `src/host/web/modules/` | 5 bundled tracks: `64mania.s3m`, `skyrider.s3m`, `hul.mod`, `purple.669`, `unreal ][.s3m` |
| `build-web.sh` | Compile + deploy script; `--no-deploy` flag for compile-only |

#### Non-obvious implementation notes

- `AudioWorkletNode.port` requires `addEventListener` + explicit `.start()` —
  assigning `onmessage` alone does not deliver messages in all browsers.
- `TextDecoder` is absent from `AudioWorkletGlobalScope`; song titles decoded
  with `String.fromCharCode` instead.
- `AudioContext.resume()` must be called after async init work even when the
  context was created inside a user-gesture handler.
- Stop resets playback position by re-calling `wasm_load(lastModuleSize)` —
  the module bytes remain in `g_module_buf` so no re-transfer is needed.
- The module selector is enabled before Start so the user can choose a track
  first; the change handler is wired only after `init()` succeeds.

---

## host/cli — C++ command-line player (miniaudio)

---

### 2026-06-23 — Loop counter in status line

`(looped: xN)` appended to the status line after the first loop event,
absent entirely during a no-loop or pre-loop play.

| File | What |
|---|---|
| `main.cpp` | `st_loops` atomic on `AudioCtx`; set in loop-detection block; snprintf'd only when > 0 |

---

### 2026-06-23 — Loop-count off-by-one fix

`-l N` is "take the loop N times" (play N+1 times total).  The previous
guard `cur_loops >= N` fired on the first wrap for `-l 1`, stopping after
a single play.  Fixed to `cur_loops > N` so the engine stops only after
the (N+1)th loop event.

---

### 2026-06-23 — Rich banner + live playback status line

`fxplayer` now shows a formatted startup banner and a status line that
updates in-place every 50 ms:

```
F/X Player 0.67.0
module.s3m  [S3M · 48000 Hz · stereo · interp · soft-clip · no loop]
"Song Title"
[space] pause  [←/,  →/.] order  [↑/+  ↓/-] volume  [q/Esc] quit
  Ord  0/15  Pat  3  Row 42/64  Ch 6/8  Vol 64  [PAUSED]
```

#### What was added / changed

| File | What |
|---|---|
| `fx.h` | `fx_playback_state` struct; `fx_get_playback_state()`; `fx_song_title()` |
| `fx.c` | Implements both — reads format globals, counts `ChannelActiv[]` for active channels |
| `s3m.c` / `mod.c` / `m669.c` | Each stores song title at load time; exposes via `*_song_title()` |
| `s3m.h` / `mod.h` / `m669.h` | Declarations for `*_song_title()` |
| `main.cpp` | Banner with loop/interp/softclip labels; 6 `atomic<uint32_t>` fields on `AudioCtx` snapshotted in the audio callback; UI reads them every 50 ms; `\r\033[K` in-place update |

#### Design notes

- `fx_get_playback_state()` is audio-callback-only (same constraint as
  `fx_song_loops()`). The callback snapshots into per-field atomics; the
  UI thread reads them relaxed — a one-frame stale read is fine for display.
- Song title: S3M 28-char field at buf[0..27]; MOD 20-char field at
  buf[0..19]; 669 message at buf[2] (already null-terminated by the loader
  at buf[109]). Blank/all-spaces titles are suppressed.
- Status line uses `\r\033[K` (carriage-return + erase-to-EOL) and is
  erased cleanly on exit, leaving no residue in the terminal.

---

### 2026-06-20 — Real-time S3M playback via miniaudio

`build/host/cli/fxplayer <module.s3m>` — plays S3M files in real time
through the system audio device.  Ctrl+C to stop; auto-stops at song end.

#### What was added

| File | What |
|---|---|
| `host/cli/third_party/miniaudio/miniaudio.h` | Vendored miniaudio 0.11.21 (single-header audio library) |
| `host/cli/miniaudio.c` | `MINIAUDIO_IMPLEMENTATION` translation unit compiled as C |
| `host/cli/CMakeLists.txt` | Updated: adds miniaudio TU; links `Threads::Threads`, `dl`, `m` |
| `host/cli/main.cpp` | Arg parse → file read → `fx_load` → `ma_device` callback loop |

#### Design notes

- **Buffer ownership**: `s3m_load` does `memcpy(workspace, data, size)` on
  entry, so the file buffer can be freed immediately after `fx_load`.
  Only the workspace buffer must survive until `fx_close`.
- **Audio thread**: `ma_device` data callback calls `fx_render_frames` on
  the miniaudio audio thread.  No engine calls are made from the main thread
  after `fx_load`.
- **Song end**: `fx_render_frames` returns fewer frames than requested when
  `s3m_is_done()` is set (end-of-order reached without looping back).  The
  callback zero-fills the tail and sets `g_stop`; the main thread exits on
  the next 50 ms poll.
- `ma_sleep` is internal to miniaudio and not part of the public API; the
  main-thread idle loop uses `std::this_thread::sleep_for` instead.
- Config: 48 kHz, stereo, S16, linear interpolation, soft-clip — matches
  the reference render settings used in the CTest bit-exact check.

---

## core/ — C99 port (cross-platform engine library)

---

### 2026-06-22 — Deferred-jump unification + S3M bounds fix

#### Deferred order-jump for 669; goRowOrder normalized across all formats

669 previously applied order jumps immediately inside `fx_order_jump`
(direct assignment to `M669_Order`/`M669_Pattern`/`M669_row`).  MOD and
S3M already used a deferred approach: set `nextorder`/`nextrow`/`jump` and
apply at end-of-row in `goRowOrder`.  669 now follows the same pattern.

Additional normalization applied to all three formats:

- Branch structure unified to `if (jump == 0) { normal } else { jump }`.
  669 used an inverted early-return guard; now all three are consistent.
- `nextrow`/`nextorder` are always cleared after the jump branch, even when
  the bounds check rejects the target.  MOD and S3M previously returned
  early on an out-of-bounds jump, leaving stale values in those variables.
- Dead code removed from MOD: the post-apply
  `if (MOD_Order >= MOD_OrderNum)` block could never be true because the
  entry guard already checked `MOD_nextorder < MOD_OrderNum`.

#### S3M order-jump off-by-one fix (see BUGS.md O-3)

The jump-branch guard used `S3M_nextorder <= S3M_OrderNum` (note: `<=`).
`S3M_OrderNum` is the count of entries in the order list; valid indices are
`0 … OrderNum-1`.  Accepting `nextorder == OrderNum` reads one byte past
the order list — into the instrument parapointer table.

Two real paths could produce `nextorder == OrderNum`:
- Effect `Bxx` with operand equal to `OrderNum` (no clamping in the effect handler).
- Effect `Cxx` (break to row) on the **last order position**:
  sets `nextorder = CurrentOrder + 1`, which equals `OrderNum` when
  `CurrentOrder == OrderNum - 1`.

Changed to strict `<`.  Correct songs are unaffected: normal end-of-song
wrapping is handled by the `Pattern == 255` sentinel check that runs inside
the guard.  Documented in `BUGS.md` as O-3 for follow-up investigation in
the original DOS source.

---

### 2026-06-20 — MOD + 669 playback ported; bit-exact CTests pass

MOD (4- and 8-channel) and 669 formats added to the C99 core.
`fx_detect_format` now sniffs all three formats; `fx_load` /
`fx_render_frames` dispatch automatically.  All six CTests pass
byte-for-byte: `compare_s3m`, `compare_mod` (hul.mod, 8-channel "8CHN"),
`compare_669` (purple.669).

#### What was ported

| Original (DOS) | C99 port | Notes |
|---|---|---|
| `dat_mod.cpp` | `core/format/mod.c` / `mod.h` | Loader, big-endian field swap, period-table octave expansion, render block |
| `efc_mod.cpp` | `core/effect/efc_mod.c` / `efc_mod.h` | Full MOD effect set (0–15 + Exx extended) |
| `dat_669.cpp` | `core/format/m669.c` / `m669.h` | 669 loader (magic `0x6669` "if"), render block |
| `efc_669.cpp` | `core/effect/efc_669.c` / `efc_669.h` | 669 effects 0–7: portamento, glissando, vibrato, retrig |

#### Non-obvious porting decisions

- **MOD samples are signed 8-bit** (Amiga Paula native) — no conversion.
  Only 669 samples are unsigned; XOR'd with 0x80 during the workspace copy,
  matching the original `ConvU8MtoS8M`.
- **`g_master_vol_table`** moved from `s3m.c` to `mixer_scalar.c` so all
  three format loaders can set it without cross-module dependencies.
- **Format detection:** S3M ("SCRM" @44), 669 ("if" @0), MOD (8 known
  4-char IDs @1080 — 31-sample variants only; 15-sample MODs not sniffed).
- **MOD big-endian** 16-bit fields byte-swapped into the workspace copy,
  never in-place on the caller's buffer.
- **MOD default panning** is LRRL (`ch%4`: 0=L,1=R,2=R,3=L); master volume
  scales by channel count: `(0x50 - channels*4) * 256`.

#### Bugs found reaching bit-exactness (full detail in `BUGS.md`)

- **Master-vol soft-clip table (O-2)** — the dominant divergence.  The
  original `calcMasterVolume32` lets `test` persist (~⅔·MasterVolume) past
  table index `2*val` rather than resetting to 128 (no `else` branch).  An
  early port added `else test=128`, which only loud modules (669,
  MasterVolume 12288) ever reached — quiet/mono S3M modules masked it.
- **MOD effects** — corrected arpeggio `PeriodeAdjust` formula, fine
  portamento using the full `Info` byte, volume-slide ordering
  (`dec(y)` then `add(x)`), and note-delay bitmask.
- **UB guards** — bounds-check `S3M_NotePeriodes[]` reads and guard
  divide-by-zero in `Calc_st3periode`.  The original relies on DOS having
  no memory protection; `stars.s3m` actually crashes the original DOS
  player (its WAV is left with an unpatched header — `closeWAV` never ran).
  The C99 port is more robust and is bit-exact up to the DOS crash point.
- **Misaligned 16-bit reads** in `s3m_load` (odd `ord_num`) → byte-wise.

#### Test harness

- `tests/render_mod/` runs `compare_mod` (hul.mod) and `compare_669`
  (purple.669) against hardcoded reference SHA-256s.
- `render-dosbox.sh --native` now copies the module into `_work/` and refers
  to it by **8.3-safe** name on C: — DOS truncates long names, and the old
  cross-drive `D:\` path failed silently, producing empty WAVs.
- Reference source modules committed to `tests/reference_renders/`
  (`hul.mod`, `purple.669`); large scratch set under `tests/_test_mods/`
  is gitignored.  New `BUGS.md` logs original-vs-port bugs + module status.

#### Validation

```
ctest --test-dir build   # 6/6 pass
```

- `compare_mod`: sha256 == `dfa8b22ccc142463dc837d82bcee9411d491d6ec906c489633abfef6a80dec3a`
- `compare_669`: sha256 == `10089bc3010836b22cd9afe89a74b27524060afb96c953fa9efbfc32a752ff5d`

---

### 2026-06-20 — S3M playback path ported to C99; CTest passes bit-exact

First working milestone of the cross-platform port.  `core/` now builds
as a pure C99 static library (`libfxcore.a`) that plays S3M modules and
produces output **bit-identical** to the original DOS build.

#### What was ported

| Original (DOS) | C99 port | Notes |
|---|---|---|
| `_work/dat_s3m.cpp` | `core/format/s3m.c` | Buffer-based loader; `uintptr_t` for sample pointers |
| `_work/efc_s3m.cpp` | `core/effect/efc_s3m.c` | Full S3M effect set (effects A–Z) |
| `_work/mixer.cpp` + ASM kernels | `core/mixer/mixer_scalar.c` | 32-bit scalar mixing path; soft-clip master volume |
| `_work/dat_calc.cpp` (`#pragma aux`) | `core/util/calc.c` | `divide_64bit` → plain C `uint64_t` arithmetic |
| `interrupt_S3M()` in `dat_s3m.cpp` | `s3m_render_block()` | Tick/row/order advance loop, mixer bridge |
| `DoMasterVolumeCalculations()` | `mixer_convert_to_s16()` | `MasterVol16for32ss` path: soft-clip + signed S16 output |

#### Public API added (`core/include/fx/fx.h`)

```c
fx_err  fx_workspace_size(data, size, *out_bytes);  /* caller allocates */
fx_err  fx_load(data, size, workspace, ws_size);
size_t  fx_render_frames(out, frame_count, *cfg);
void    fx_close(void);
```

No `malloc` inside the engine.  Caller supplies workspace; size is
computed from the module header by `fx_workspace_size`.

#### Non-obvious porting decisions

- **`uintptr_t` for sample addresses** — the original DOS code stored
  raw flat-model pointers in `unsigned long` (32-bit on Watcom).  On
  64-bit Linux the upper 32 bits would be silently truncated if kept as
  `uint32_t`.  `S3M_SampleAddress` and `g_ChannelSampleAddress` use
  `uintptr_t` throughout.

- **`max_frames` cap in test binary** — `TEST.S3M` loops forever;
  `render_s3m` accepts an optional third argument.  The CTest passes
  `1441792` (352 × 4096), which matches the DOS `-n:30` render cap used
  to produce `tests/reference_renders/TEST.wav`.

#### Validation

```
cmake -B build -DFX_BUILD_TESTS=ON && cmake --build build
ctest --test-dir build   # 2/2 pass
```

`compare_s3m` asserts:

```
sha256(out.wav) == 5edd49d54cd161dfac8c88ffbec30bf6fcadea4a2e3cd053db3dbb1c68c30834
```

Bit-exact match achieved on first run.

---

### 2026-06-19 — DEV_WAV file-output device + render harness

#### New: `dev_wav.cpp` / `dev_wav.h` — WAV file output (CardType 4)

Adds a fourth output device alongside SB, WSS, and GUS that writes the
mixed audio directly to a WAV file instead of a soundcard.  No DMA, no
IRQ, no hardware detection — runs as fast as the CPU allows.

How it works:
- `initWAV(buf_len)` opens the output file, writes a 44-byte placeholder
  RIFF/WAV header, and allocates a plain `malloc` buffer for `DMApointer`.
  `initDMAbuffer()` then sets `Mixerpointer1/2` from that buffer exactly
  as it would for real DMA memory.
- `writeWAV()` fwrites the current `Mixerpointer` block to the file after
  each `PlainInterrupt()` call.
- `closeWAV()` seeks back to offset 4 and 40 to patch the two RIFF size
  fields, then frees the buffer.

Output is standard PCM WAV (format tag 1).  16-bit signed stereo at the
configured `MixSpeed` is the tested path; 8-bit and mono work too.

#### Changes to existing files

**`device.cpp`**
- Added `#define WAV 4`.
- `writeCard()` — prints filename instead of hardware info.
- `checkCard()` — WAV case returns 0 immediately (no hardware to detect).
- `initCard()` — WAV case calls `initWAV()` and returns early, bypassing
  the `setNewIRQ` / `DMAalloc` / `initDMA` calls that follow the switch.
- `startCard()` — WAV case is a no-op (MixSpeed already set by caller).
- `stopCard()` — early return for WAV before the unconditional `stopDMA`.
- `clearCard()` — early return for WAV calling `closeWAV()`, skipping
  `IRQClose()` / `DMAfree()`.
- `PlainInterrupt()` case WAV — toggles `Mixer_Block` so the
  buffer-pointer flip picks alternating halves (harmless for file output,
  keeps the logic symmetric with the real-hardware path).

**`fx.cpp`**
- Includes `dev_wav.h`.
- Main loop branches on `CardType == 4`: calls `PlainInterrupt()` +
  `writeWAV()` in a tight synchronous loop instead of the `kbhit()` wait.
- Time limit: `wav_max_seconds > 0` stops the loop when
  `wav_data_bytes >= wav_max_seconds * MixSpeed * channels * bytes_per_sample`.

**`command.cpp`**
- `-w:FILENAME` — sets `wav_outfile` and forces `CardType = 4`.
  No `-t:1` or hardware flags needed when `-w:` is present.
- `-n:SECONDS` — sets `wav_max_seconds` for a render-time cap.
  Note: `-t:` was already taken for card **T**ype, so the time-limit
  switch uses `-n:` (Number of seconds).
- Both switches documented in `helpline()`.

**`build-dos.linux.sh`**
- `dev_wav.cpp` added to the compile list.

#### New: `tests/render-dosbox.sh --native` mode

The render harness now has two modes:

| Mode | Command | How |
|---|---|---|
| `--native` | `FX /w:FXOUT.WAV /n:N MODULE` | FX.EXE writes WAV itself; ~20× realtime; deterministic |
| (default) | `FX /t:1 /d:5 /i:7 MODULE` + parecord | PulseAudio capture from SB16 emulation; realtime |

`--native` is now the preferred path for reference renders: no soundcard
emulation, no audio capture, output is byte-for-byte reproducible across
runs.

#### Reference renders produced

| File | Duration | SHA-256 |
|---|---|---|
| `tests/reference_renders/TEST.wav` | 30 s (capped) | `5edd49d5…` |
| `tests/reference_renders/64mania.wav` | 120 s (capped) | `78987972…` |

Generated with: `tests/render-dosbox.sh --native -t 30 _work/TEST.S3M`
and `tests/render-dosbox.sh --native -t 120 _dosbox/C/64mania.s3m`.

---

### 2026-06-19 — Source tree normalised to lowercase

All `_work/` filenames (`.CPP`, `.H`, `.ASM`, `.OBJ`) lowercased for
Linux toolchain compatibility.  `#include` directives updated to match.
`build-dos.linux.sh` updated.  No code changes.

---

### 2026-06-19 — DOSBox-X render harness (`tests/render-dosbox.sh`)

Initial version: PulseAudio-capture mode only.  Mounts `_work/` as C:
and a separate module directory as D:, runs FX.EXE with SB16 params, and
records from the default sink monitor via `parecord`.

---

### 2026-06-19 — First successful build with OpenWatcom V2 on Linux

`FX.EXE` (156 K) compiled and linked cleanly from `_work/` using
`wpp386` + `wasm` + `wlink system pmodew`.  Verified playback of
`64mania.s3m` in DOSBox-X.

Fixes required vs. original source:
- TASM local-label syntax (`@@label` → `@label` style) in three ASM files.
- Several implicit C++ casts tightened.
- Header symlinks for case-insensitive vs. case-sensitive filenames.
- OBJ output naming made explicit (`-fo=`).
