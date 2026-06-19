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

**Status:** Planning complete. No port code written yet.

**Next milestone:** Add a `DEV_WAV` (file output) target to the **original** DOS code so we can produce bit-exact ground-truth WAV renders. Plan:

- New files `DEV_WAV.CPP` / `DEV_WAV.H` (distinct from existing `DAT_WAV.CPP` which plays WAV input).
- `DEVICE.CPP` already has an empty `case 4 :` slot in the `PlainInterupt` switch — that becomes the WAV output card type.
- `FX.CPP` main loop branches: when `CardType == WAV`, drive `PlainInterupt()` synchronously instead of waiting for IRQ, then `fwrite(Mixerpointer, ...)` after each block.
- New `-w <filename>` command-line switch and `-t <seconds>` max-render-time.
- Render-as-fast-as-possible only (no realtime+tee).

Step 0 before any of that: confirm the Watcom toolchain still builds the project unchanged, producing a working `FX.EXE` that plays a `.S3M` in DOSBox.

## Notes on collaborator

- User is the original author (Apollo) — deep expertise in x86 assembler optimization, Pentium U/V pipeline scheduling, and demoscene-era cycle-counting.
- Trust the user's claims about ASM performance — modern compilers will *not* beat hand-tuned P5 code on scalar throughput. The portability win comes from SIMD or different ISAs, not from "compilers got smarter."
