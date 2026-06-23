# F/X Player — Change Log

## host/cli — C++ command-line player (miniaudio)

---

### 2026-06-23 — Rich banner + live playback status line

`fxplayer` now shows a formatted startup banner and a status line that
updates in-place every 50 ms:

```
F/X Player 0.67.0
"Song Title"
module.s3m  [S3M · 48000 Hz · stereo · interp · soft-clip · no loop]
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

**`build-linux.sh`**
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
`build-linux.sh` updated.  No code changes.

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
