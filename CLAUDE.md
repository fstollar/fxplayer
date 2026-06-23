# FX Player — Cross-platform port

## Project

Porting **F/X Player** — a 1998 DOS MOD/S3M tracker module player by Apollo of STIGMA — to a modern, portable codebase while keeping the original DOS build alive.

The goal is **three build targets from one engine core:**

1. **Modern CLI player** (Linux / macOS / Windows) — C++ host wrapping a C99 core, audio out via miniaudio.
2. **Original DOS build** — still compilable with OpenWatcom + TASM + Pmode/W, using the original SoundBlaster / WSS / DMA / IRQ code unchanged.
3. **Bare-metal embedded** — 32-bit ARM Cortex-M MCUs (e.g. STM32) with I2S DMA out. Whole module loaded into SRAM/XIP/DRAM (no streaming).

## Development Standards

### Brace style — Allman

Always put the opening `{` on its own line: functions, if/else, for, while, struct, class, lambdas — every multi-line block. One-liners (`{ return x; }`, empty `{}`) stay on a single line.

### Naming — no short identifiers

Never use 1–3 letter names (`i`, `it`, `k`, `enc`, `lo`, `hi`, `np`, `r`, `s`, …). Every variable, loop counter, lambda parameter, and struct member must be long enough to be self-explanatory without surrounding context. Rename short names whenever editing existing code that contains them.

### Enum naming convention

- Enum class names: `E` prefix + PascalCase → `ENodeType`, `EPredVariant`
- Enum values: `e` prefix + ALL_CAPS → `eSOURCE`, `eCLAMP`, `eRICE_SHORT`
- Never rename an enum that doesn't follow this convention without explicit user instruction.

### Core rules

- Prefer minimal diffs. Do not rename, reorder, or reformat unrelated code.
- Leave unrelated user changes and untracked artifacts alone.

### Avoid

- Do not make broad formatting passes.
- Do not silently change CMake defaults or feature defaults unless required and approved by the user.
- Do not assume generated outputs, local files, or untracked artifacts are disposable.
- Do not disable tests to get a green run without documenting why.

### Working mindset

- Reproduce the issue before fixing it when practical.
- Understand root cause before writing code. If there is real ambiguity, ask instead of guessing.
- Validate simplifications carefully.
- Centralize duplicated parsing, caching, or helper logic once duplication starts spreading.

### Wisdom

- If code looks unusually complex, assume it handles a real edge case until proven otherwise.
- Simplifications and optimizations are common sources of later reverts; validate them broadly.
- Prefer reproducible regressions over fix-only changes.
- When duplication starts to spread, extract the shared helper before it calcifies.

### Code Comments — Comment to Follow the Code

The codebase is changing fast right now; comments that help a reader follow the flow are wanted. Favor clarity over terseness.

**Comment liberally where it aids understanding.** Always write a *why* comment when the reasoning is non-obvious — a hidden constraint, a subtle invariant, a workaround, an ordering that matters for byte-identity. Beyond that, a short *what/overview* comment on a non-trivial block, function, or file is welcome when it helps a reader navigate (e.g. a one-line banner on each phase of a multi-phase routine, or a file-top summary of the unit's role and its place in the layer tree). Don't comment self-evident one-liners.

**Prefer a short line or two; expand when the topic genuinely needs it.** A non-obvious algorithm, a cross-layer contract, or a tricky concurrency interaction can justify a small paragraph. Avoid multi-paragraph "history of how we got here" blocks — the diff, commit message, and linked issue carry the history.

**Don't reference a transient task/PR number inline.** Patterns like `"Pre-#1030 follow-up…"` belong in the commit body; inline they rot as the code moves. Describing the *current* design and why is fine and encouraged.


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

## _work/ — Working DOS build (OpenWatcom + TASM, runs in DOSBox-X)

All changes are in `_work/`. The `_original/` source is never touched.

## Repo layout

```
fxplayer/
├── CLAUDE.md                  # this file
├── _original/                 # 1998 source, untouched
├── _work/                     # work copy of _original/ with incorporated bug fixes etc
├── src/
│   ├── core/                  # C99, no deps, no I/O, no alloc
│   │   ├── include/fx/        # public C API headers
│   │   ├── engine/            # state, channel mgmt, mix dispatch
│   │   ├── format/            # mod.c, s3m.c, m669.c, wav.c
│   │   ├── effect/            # efc_mod.c, efc_s3m.c, efc_669.c
│   │   └── mixer/
│   │       ├── mixer_scalar.c     # portable reference
│   │       ├── mixer_x86_sse2.c
│   │       ├── mixer_x86_avx2.c
│   │       ├── mixer_armcm_dsp.c
│   │       └── mixer_x86_p5.asm   # original (DOS only)
│   └── host/
│       ├── cli/               # C++ CLI player (miniaudio)
│       ├── dos/               # DOS host (original DEV_SB/DEV_WSS/DMA/IRQ)
│       └── mcu_example/       # bare-metal STM32 I2S DMA example
├── tests/
│   ├── reference_renders/     # bit-exact WAV ground truths
│   └── ...
├── cmake/                     # CMake helper scripts (get_cpm.cmake — CPM 0.42.3)
├── CMakeLists.txt             # hosted CLI build
└── makefile.dos               # DOS/Watcom build
```

## Validation strategy

The mixer's output is fully deterministic, so:

1. Render reference modules (`TEST.S3M`, `MAZ-TEST.S3M`, etc.) to WAV using the original DOS build.
2. Every port variant must produce **bit-identical** output to the scalar C reference, which itself must match the original DOS WAV.
3. Comparison is `sha256(wav)` — no tolerance, no fuzzy matching.

## Current status

**Per-change history lives in `CHANGELOG.md`; known bugs/quirks in `BUGS.md`.**
This section is a one-line-per-milestone index only — don't duplicate the
blow-by-blow logs here.

- **DOS build (`_work/`)** — builds with OpenWatcom V2 on Linux → `FX.EXE`
  (156 K, PMode/W). `DEV_WAV` file-output device (`-w:`/`-n:` switches).
- **C99 core (`src/core/`)** — S3M, MOD (4/8-ch), and 669 all play **bit-exact**
  vs. the DOS reference. `fx_detect_format` dispatches all three. 6/6 CTests
  pass (`compare_s3m` / `compare_mod` / `compare_669`).
- **CLI host (`src/host/cli/`)** — `fxplayer <module>` real-time playback via
  miniaudio. Full arg parsing (`-r`/`-c`/`-l`/`-v`/`--no-interpolation`/`--no-softclip`).
  Interactive keyboard controls: pause/resume (SPACE), quit (Q/Esc),
  order jump (←/→), volume (↑/↓). POSIX termios raw-mode TTY input, no ncurses.
  Startup banner shows song title, format, Hz, channels, interp/softclip/loop flags.
  Live status line (50 ms refresh): order, pattern, row, active channels, volume,
  loop counter. Loop semantics: `-l 0` = no loop, `-l N` = loop N times (N+1 plays),
  `-l -1` = infinite (default).
- **Validation harness** — `tests/render-dosbox.sh --native` renders DOS
  reference WAVs; CTests compare sha256 against hardcoded reference hashes.

**Durable facts worth keeping in context** (rationale in CHANGELOG.md / BUGS.md):
- Sample addresses use `uintptr_t`, not `uint32_t` (64-bit host pointer width).
- `g_master_vol_table` lives in `mixer_scalar.c`, shared by all format loaders.
- MOD samples are signed 8-bit (no conversion); only 669 samples are XOR'd 0x80.
- The mixer's master-volume soft-clip table has a faithful original quirk
  (BUGS.md O-2) — do not "fix" it; it's required for bit-exactness.
- DOS reference renders need **8.3-safe filenames** on the C: mount.
- All three formats use the same deferred-jump pattern: `fx_order_jump` sets
  `nextorder`/`nextrow`/`jump`; `goRowOrder` applies at end-of-row. Never apply
  jumps immediately — all three `goRowOrder` functions must stay symmetric.
- S3M order-jump bounds use strict `<` (not `<=`) against `S3M_OrderNum` —
  the `<=` in the original is an off-by-one OOB read (BUGS.md O-3).
- **CPM** (`cmake/get_cpm.cmake`, v0.42.3) manages host-only deps: miniaudio 0.11.21
  and cxxopts 3.2.0. Both download to `build/_deps/` at configure time. Set
  `CPM_SOURCE_CACHE=~/.cache/cpm` (or env var) to share across projects.

### Next milestones (not yet started)

- SIMD mixer variants (`mixer_x86_sse2.c` / `_avx2.c`) + ARM DSP, each
  validated bit-exact against `mixer_scalar.c`.
- Encapsulate the global-array state into a struct (only after the suite is
  green — it now is).
- XM / IT formats (planned in the original, never implemented).
- **Web host** — WebAssembly build of the C99 core; browser page with basic
  transport controls. Candidate: Emscripten + a small JS/HTML shell, or a
  Rust/wasm-bindgen thin wrapper around the C core.
- **Bug audit** — systematically work through `BUGS.md`, classify each quirk
  as (a) faithful reproduction required for bit-exactness, (b) fixable in the
  C99 core without breaking bit-exactness, or (c) fixable only in the DOS
  original; for (b)/(c) decide whether/how to fix and document the decision.
- **Remove floating-point from `src/core/`** — `mixer_scalar.c` uses `double`
  for the master-volume table init (`build_vol_table`, lines ~604-631). Replace
  with fixed-point arithmetic. Until done, `-ffast-math` is kept on `fxcore`
  (noted in `src/core/CMakeLists.txt`).

## Reference docs

- **`docs/open-watcom-v2.md`** — consult when working on the DOS build, `makefile.dos`,
  `wasm`/`wcc386`/`wcl386` flags, OW V2 C99 compliance, or CMake toolchain integration
  for the DOS target.
- **`docs/openwatcom-v2-bugs.md`** — consult before diagnosing wasm/wpp386 errors; lists
  known OW V2 bugs with workarounds (`@@` local labels, `-wcd=<n>`, `model flat,prolog`).

## Notes on collaborator

- User is the original author (Apollo) — deep expertise in x86 assembler optimization, Pentium U/V pipeline scheduling, and demoscene-era cycle-counting.
- Trust the user's claims about ASM performance — modern compilers will *not* beat hand-tuned P5 code on scalar throughput. The portability win comes from SIMD or different ISAs, not from "compilers got smarter."
