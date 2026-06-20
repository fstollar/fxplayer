# FX Player — Cross-platform port

## Project

Porting **F/X Player** — a 1998 DOS MOD/S3M tracker module player by Apollo of STIGMA — to a modern, portable codebase while keeping the original DOS build alive.

The goal is **three build targets from one engine core:**

1. **Modern CLI player** (Linux / macOS / Windows) — C++ host wrapping a C99 core, audio out via miniaudio.
2. **Original DOS build** — still compilable with OpenWatcom + TASM + Pmode/W, using the original SoundBlaster / WSS / DMA / IRQ code unchanged.
3. **Bare-metal embedded** — 32-bit ARM Cortex-M MCUs (e.g. STM32) with I2S DMA out. Whole module loaded into SRAM/XIP/DRAM (no streaming).

## Original source

Location: `_original/`

- **Author:** Apollo of STIGMA, 1996-1998
- **Toolchain:** OpenWatcom C++, TASM, Pmode/W 32-bit DOS extender
- **Formats supported:** MOD (≤8 ch), S3M, 669, WAV (input playback)
- **Soundcards:** SoundBlaster 1.x / 2.x / Pro / 16, Windows Sound System
- **State:** v0.66 alpha, "Mekka 2k-1" — feature-complete for MOD/S3M/669, XM/IT planned but never started
**CRITICAL** Never modify, edit or otherwise change the original source code in this directory! When changes are needed and no independent work copy exists, ask the user how to proceed!

### Original architecture (highlights)

- **State is global parallel arrays** sized `[256]` (`ChannelVolume[256]`, `ChannelSamplePosition[256]`, etc.). Designed for up to 255 channels in preparation for XM/IT.
- **Format separation is clean:** `DAT_*` files load+parse, `EFC_*` files interpret per-row effects, `MIXER.CPP` orchestrates, `MIXR_*.ASM` are the inner kernels.
- **Sample-accurate timing** (no PIT/timer use). Tempo is converted to "samples per tick" at the chosen mix rate → fully deterministic and reproducible.
- **Mix buffer is 32-bit signed integers, 16.8 fixed-point per channel, with 8 bits of headroom** for accumulating up to 256 channels without saturation.
- **16 ASM mixer kernel variants** declared in `MIXER.H`: `{8,16}-bit input × {mono,stereo} output × {16,32}-bit mix buffer × {nearest,linear-interp}`. Three of the four `.ASM` files are populated (`MIXR_16N`, `MIXR_32N`, `MIXR_32I`); the 16-bit interpolated set is declared but unimplemented.
- **The ASM uses self-modifying code** (the `argdd` macro pattern) to patch volume, fraction increments, and buffer addresses directly into the instruction stream — saves a memory load per inner-loop iteration on Pentium U/V pipelines.
- **PMode/W flat 32-bit memory model** — no segment tricks, pointers are flat. This maps cleanly to modern flat memory and Cortex-M.

## Architectural decisions for the port

These are settled — don't relitigate without reason.

- **Engine core is C99, not C++.** Watcom's C++ is anemic, embedded toolchains (IAR/Keil/ARM GCC) are C-first, and a C core can be consumed cleanly from a C++ host.
- **Hard rules for the core:** no `malloc`/`free` (caller passes buffers), no file I/O (caller passes a memory buffer), no `printf`, no OS headers, no threads, fixed-point only, explicit endianness on all parsing, fixed-width integer types only.
- **Initial port keeps the global-array layout** for mechanical translation and bit-exact validation. Encapsulation into a struct comes *after* a passing test suite.
- **Mixer variants are build-time selectable** (no runtime dispatch — MCU builds shouldn't pay for it):
  - `MIXER_X86_P5_ASM` — original ASM (DOS only)
  - `MIXER_X86_SSE2` / `MIXER_X86_AVX2` — intrinsics
  - `MIXER_ARM_DSP` — Cortex-M4F intrinsics (`__SMLAL`, `__SSAT`)
  - `MIXER_SCALAR_C` — portable reference; the bit-exact ground truth all variants must match
- **Self-modifying code stays in the DOS build only.** Modern OSes enforce W^X; on modern OoO cores, SMC is *slower* than a memory load. Modern x86 build replaces patch sites with `rip`-relative loads from a per-mixer state struct.

## Planned repo layout

```
fxplayer/
├── CLAUDE.md                  # this file
├── _original/                 # 1998 source, untouched
├── core/                      # C99, no deps, no I/O, no alloc
│   ├── include/fx/            # public C API headers
│   ├── engine/                # state, channel mgmt, mix dispatch
│   ├── format/                # mod.c, s3m.c, m669.c, wav.c
│   ├── effect/                # efc_mod.c, efc_s3m.c, efc_669.c
│   └── mixer/
│       ├── mixer_scalar.c     # portable reference
│       ├── mixer_x86_sse2.c
│       ├── mixer_x86_avx2.c
│       ├── mixer_armcm_dsp.c
│       └── mixer_x86_p5.asm   # original (DOS only)
├── host/
│   ├── cli/                   # C++ CLI player (miniaudio)
│   ├── dos/                   # DOS host (original DEV_SB/DEV_WSS/DMA/IRQ)
│   └── mcu_example/           # bare-metal STM32 I2S DMA example
├── tests/
│   ├── reference_renders/     # bit-exact WAV ground truths
│   └── ...
├── CMakeLists.txt             # hosted CLI build
└── makefile.dos               # DOS/Watcom build
```

## Validation strategy

The mixer's output is fully deterministic, so:

1. Render reference modules (`TEST.S3M`, `MAZ-TEST.S3M`, etc.) to WAV using the original DOS build.
2. Every port variant must produce **bit-identical** output to the scalar C reference, which itself must match the original DOS WAV.
3. Comparison is `sha256(wav)` — no tolerance, no fuzzy matching.

## Current status & immediate next step

**Status:** DOS build working; DEV_WAV implemented; reference renders generated.

### Completed

- `_work/` builds cleanly with OpenWatcom V2 on Linux → `FX.EXE` (156 K, PMode/W).
- **`dev_wav.cpp` / `dev_wav.h`** — WAV file output device (CardType 4).
  - `-w:FILENAME` CLI switch selects WAV output (implies CardType 4, no hardware needed).
  - `-n:SECONDS` CLI switch caps render time. (Note: `-t:` was already taken for card **T**ype.)
  - Renders as fast as possible — ~20× realtime in DOSBox-X.
- **`tests/render-dosbox.sh --native`** — mounts `_work/` as C:, runs
  `FX /w:FXOUT.WAV /n:N MODULE`, retrieves the WAV. No SB emulation or
  PulseAudio capture needed. Default (non-`--native`) PulseAudio path preserved.
- Reference renders in `tests/reference_renders/`:
  - `TEST.wav` (30 s) — sha256 `5edd49d5…`
  - `64mania.wav` (120 s) — sha256 `78987972…`

### C99 core port — COMPLETE ✓

S3M playback path ported to `core/` as a pure C99 library.  CTest
`compare_s3m` passes: sha256 of rendered `TEST.S3M` matches the DOS
reference render byte-for-byte.

**Files added:**

| Path | What |
|---|---|
| `core/include/fx/fx.h` | Public API: `fx_workspace_size`, `fx_load`, `fx_render_frames`, `fx_close`, `fx_err`, `fx_config` |
| `core/engine/fx.c` | API dispatch — wires format/mixer/engine together |
| `core/util/calc.c` | `s3m_calc_speed`, `s3m_divide_64bit` (replaces `#pragma aux`) |
| `core/format/s3m.c` | Buffer-based S3M loader + pattern decode + render block loop |
| `core/effect/efc_s3m.c` | Full S3M effect set (A–Z) |
| `core/mixer/mixer_scalar.c` | 32-bit scalar mixing path + soft-clip master volume |
| `tests/render_s3m/main.c` | Test binary: renders TEST.S3M → WAV |
| `cmake/check_sha256.cmake` | CTest sha256 assertion helper |

**Key decisions made during port:**
- `S3M_SampleAddress` / `g_ChannelSampleAddress` use `uintptr_t` (not
  `uint32_t`) — the original DOS code assumed 32-bit flat pointers; on
  64-bit Linux the upper 32 bits would be silently truncated otherwise.
- `render_s3m` takes an optional `max_frames` argument.  TEST.S3M loops
  forever; the CTest passes `1441792` (= 352 × 4096, matching the DOS
  `-n:30` cap used to produce the reference render).
- Bit-exact match achieved on first run — no tuning needed.

### host/cli with miniaudio — COMPLETE ✓

`build/host/cli/fxplayer <module.s3m>` plays S3M files in real time via
miniaudio.  Usage: Ctrl+C to stop; auto-stops at song end.

**Files added/changed:**

| Path | What |
|---|---|
| `host/cli/third_party/miniaudio/miniaudio.h` | Vendored miniaudio 0.11.21 (single-header) |
| `host/cli/miniaudio.c` | `MINIAUDIO_IMPLEMENTATION` translation unit (compiled as C) |
| `host/cli/CMakeLists.txt` | Adds miniaudio TU; links `Threads::Threads`, `dl`, `m` |
| `host/cli/main.cpp` | Arg parse → file read → `fx_load` → `ma_device` callback loop |

**Key decisions:**
- `fx_load` copies module data into workspace (`memcpy` at s3m.c:153), so the
  file buffer can be freed immediately after `fx_load` returns.  Only
  workspace needs to live until `fx_close`.
- Config: 48 kHz, stereo S16, interpolation on, soft-clip on — matches the
  reference render settings used for CTest bit-exact validation.
- `data_callback` zero-fills the tail when `fx_render_frames` returns fewer
  frames than requested (song end), then sets `g_stop` to exit the main loop.
- `ma_sleep` is internal to miniaudio; replaced with
  `std::this_thread::sleep_for`.

### MOD / 669 format support — COMPLETE (pending CTest reference renders) ✓

MOD and 669 formats ported to the C99 core.  `fx_detect_format` now sniffs all
three formats; `fx_load` / `fx_render_frames` dispatch automatically.

**Files added:**

| Path | What |
|---|---|
| `core/format/mod.c` / `mod.h` | MOD loader, period-table expansion, render block |
| `core/effect/efc_mod.c` / `efc_mod.h` | Full MOD effect set (0–15 + Exx extended) |
| `core/format/m669.c` / `m669.h` | 669 loader (magic "if"), render block |
| `core/effect/efc_669.c` / `efc_669.h` | 669 effects (0–7): portamento, glissando, vibrato |
| `tests/render_mod/main.c` + `CMakeLists.txt` | CTest render harness (SHA placeholder) |

**Key decisions made during port:**
- MOD samples are **signed 8-bit** (Amiga Paula chip native) — no conversion needed.
  Only 669 samples are unsigned; they are XOR'd with 0x80 during workspace copy,
  matching the original `ConvU8MtoS8M` call.
- `g_master_vol_table` moved from `s3m.c` to `mixer_scalar.c` so all format
  loaders can set it without cross-module deps.
- Format detection: S3M ("SCRM" at 44), 669 ("if" at 0), MOD (8 known 4-char
  IDs at 1080 — 31-sample variants only; 15-sample MODs not auto-detected).
- MOD big-endian 16-bit fields byte-swapped in workspace copy (not in-place on
  caller's buffer).
- CTest SHA256 for MOD and 669 pending reference renders from the DOS build:
  copy a test `.mod` / `.669` to `tests/reference_renders/`, render via
  `tests/render-dosbox.sh --native`, fill in `tests/render_mod/CMakeLists.txt`.

### Next milestone: CTest reference renders for MOD / 669

1. Copy a test MOD to `tests/reference_renders/` (e.g. from `/home/fst/_privat/retro/audio_mods/mod/`).
2. Render via DOS build: `tests/render-dosbox.sh --native <module.mod> 30`
3. Record SHA256 of the output WAV.
4. Fill in `tests/render_mod/CMakeLists.txt` (uncomment and set `REF_MOD`, `REF_SHA`, `MAX_FRAMES`).
5. Repeat for a 669 file.

## Notes on collaborator

- User is the original author (Apollo) — deep expertise in x86 assembler optimization, Pentium U/V pipeline scheduling, and demoscene-era cycle-counting.
- Trust the user's claims about ASM performance — modern compilers will *not* beat hand-tuned P5 code on scalar throughput. The portability win comes from SIMD or different ISAs, not from "compilers got smarter."
