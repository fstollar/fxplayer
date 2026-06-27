# F/X Player — Architecture

This document describes both the original 1998 DOS architecture and the
design decisions made for the cross-platform port. Understanding the original
is prerequisite to understanding the port — the two must agree bit-exactly.

---

## Two-layer design

```
┌─────────────────────────────────────────────────────┐
│  Host layer  (src/host/)                            │
│  Platform-specific: audio output, file I/O, UI      │
│  cli/   dos/   web/   mcu_example/                  │
└────────────────────┬────────────────────────────────┘
                     │  C API  (src/core/include/fx/)
┌────────────────────▼────────────────────────────────┐
│  Core layer  (src/core/)                            │
│  C99, no deps, no I/O, no alloc, no OS headers      │
│  format/   effect/   engine/   mixer/               │
└─────────────────────────────────────────────────────┘
```

The core knows nothing about the host. It receives a module as a flat memory
buffer, writes audio into a caller-supplied buffer, and returns. The host
owns all I/O, memory allocation, and threading.

---

## Core layer constraints (hard rules)

These constraints make the core usable on every target — hosted CLI, DOS,
bare-metal MCU, and Wasm — from a single source tree:

- **No `malloc`/`free`** — callers pass all buffers; the core never allocates.
- **No file I/O** — callers load the module into memory and pass a pointer.
- **No `printf` or logging** — no stdio, no OS headers.
- **No threads** — the core is a pure function: call it, get samples back.
- **Fixed-point arithmetic only** — no floating-point in the mix path.
- **Explicit endianness** — all multi-byte fields parsed with manual byte reads.
- **Fixed-width integer types only** — `uint8_t`, `int16_t`, `uint32_t`, etc.

---

## Original 1998 DOS architecture

The original F/X Player (`_original/`, Apollo of STIGMA, 1996–1998) established
the design the port was initially validated against bit-exactly during the porting
phase. Going forward, authenticity to the original trackers (Scream Tracker 3,
ProTracker, etc.) is the correctness standard — not byte-identical reproduction
of the DOS binary's behaviour, including its bugs.

### Global state

State is **global parallel arrays** sized `[256]`:
`ChannelVolume[256]`, `ChannelSamplePosition[256]`, `ChannelFrequency[256]`, etc.
Designed to support up to 256 channels in preparation for XM/IT (never implemented).

The port's initial translation keeps this global-array layout for mechanical
correctness and bit-exact validation. Encapsulation into a context struct is
planned after the test suite is fully green.

### Module pipeline

```
File on disk
     │
     ▼  DAT_*.CPP  (load + parse)
In-memory module data
     │
     ▼  EFC_*.CPP  (per-row effect interpretation, one tick at a time)
Channel state update (frequency, volume, sample position, jumps)
     │
     ▼  MIXER.CPP  (orchestrate: advance channels, fill mix buffer)
32-bit integer mix buffer  (16.8 fixed-point, 256-channel headroom)
     │
     ▼  MIXR_*.ASM  (inner kernels: resample + accumulate per channel)
     │
     ▼  Master volume table  (soft-clip + scale to 16-bit output)
16-bit stereo PCM output
```

### Sample-accurate timing

No PIT or hardware timer. Tempo is converted to **samples per tick** at the
chosen mix rate. The engine advances by exactly that many samples per tick,
making output fully deterministic and reproducible across any host speed.

### Mix buffer format

32-bit signed integers, 16.8 fixed-point per channel, with **8 bits of headroom**
for accumulating up to 256 channels without saturation before the final
master-volume downscale.

### ASM mixer kernel variants

All 16 variants declared in `MIXER.H` are implemented across 3 files.
Columns are input sample format; each cell is the interpolation mode(s) present:

| File            | Mix buf | 8-bit mono     | 8-bit stereo   | 16-bit mono    | 16-bit stereo  |
|-----------------|---------|----------------|----------------|----------------|----------------|
| `MIXR_16N.ASM`  | 16-bit  | nearest+linear | nearest+linear | nearest+linear | nearest+linear |
| `MIXR_32N.ASM`  | 32-bit  | nearest        | nearest        | nearest        | nearest        |
| `MIXR_32I.ASM`  | 32-bit  | linear         | linear         | linear         | linear         |

The port's `mixer_scalar.c` implements the full matrix in portable C99 and
is the **bit-exact ground truth** all other variants must match.

### Self-modifying code (DOS only)

The ASM kernels use the `argdd` macro pattern: volume, fraction increments, and
buffer addresses are patched directly into the instruction stream at setup time.
This saves a memory load per inner-loop iteration on the Pentium U/V pipeline
where one ALU slot and one AGU slot execute in parallel.

Self-modifying code is **DOS build only**. Modern OSes enforce W^X (write XOR
execute), and on out-of-order cores SMC causes pipeline flushes — it is *slower*
than a memory load. The modern x86 build replaces patch sites with `rip`-relative
loads from a per-mixer state struct.

### Memory model

PMode/W flat 32-bit — no segment tricks, pointers are flat `uint32_t`. Maps
cleanly to modern flat memory and Cortex-M bare metal.

---

## Port architectural decisions

These are settled — don't change without explicit discussion.

### Engine core is C99, not C++

Watcom's C++ is anemic; embedded toolchains (IAR, Keil, ARM GCC) are C-first;
a C core can be consumed cleanly from any C++ host. The DOS original used
Watcom C++ but only as "C with classes" — no templates, no exceptions, no STL.

### Mixer variants are build-time selectable

No runtime dispatch. An MCU build should not link code it can't execute, and
the branch cost matters in a tight mix loop.

| Variant | Macro | Hosts |
|---|---|---|
| Original P5 ASM | `MIXER_X86_P5_ASM` | DOS only |
| x86 SSE2 | `MIXER_X86_SSE2` | Linux/macOS/Windows |
| x86 AVX2 | `MIXER_X86_AVX2` | Linux/macOS/Windows |
| ARM Cortex-M DSP | `MIXER_ARM_DSP` | MCU (`__SMLAL`, `__SSAT`) |
| Scalar C (reference) | `MIXER_SCALAR_C` | all targets |

### Sample pointer width

Sample addresses use `uintptr_t`, not `uint32_t`. On 64-bit hosts the
pointer is wider than 32 bits; truncating it silently corrupts playback.

### Deferred order jumps

All three formats (MOD, S3M, 669) use the same deferred-jump pattern:
`fx_order_jump` sets `nextorder`/`nextrow`/`jump`; `goRowOrder` applies
the jump at the end of the row. **Never apply jumps immediately** — all
three `goRowOrder` functions must stay symmetric or cross-format validation
will diverge.

---

## Deterministic rendering and validation

Because the engine is sample-accurate and has no timer or thread dependency,
two renders of the same module at the same sample rate always produce
**bit-identical output**. Validation uses this property:

- Every mixer variant must match `mixer_scalar.c` exactly — this is a
  **permanent invariant** that ensures consistent cross-platform output from
  the same engine logic.
- SHA-256 CTests against reference WAVs serve as **regression baselines**.
  They catch unintended regressions. When a bug is intentionally fixed for
  tracker authenticity, the affected reference WAVs must be regenerated.
- During the initial porting phase, reference WAVs were produced by the
  original DOS build and matched exactly. That phase is complete.
- Going forward, "correct" means "authentic to the original tracker"
  (Scream Tracker 3 for S3M, ProTracker for MOD, etc.) — not byte-identical
  to the DOS binary, which had its own bugs.

See `BUILD.md` for how to run the test suite and regenerate reference WAVs.

---

## Known original bugs and their port status

The DOS binary had real bugs. Now that the porting phase is complete, these
are candidates for correction in the C99 core toward tracker-authentic behaviour.
Full list and status in `BUGS.md`.

- **Master-volume soft-clip table** (BUGS.md O-2) — uninitialized-variable quirk
  in `calcMasterVolume32`. Previously preserved for bit-exactness. Now a
  candidate for correction; needs investigation against what ST3 actually does.
- **S3M order-jump bounds** (BUGS.md O-3) — original `<=` is an off-by-one OOB
  read; the port already corrects it to strict `<`. Already fixed.
