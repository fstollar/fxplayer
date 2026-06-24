# Open Watcom V2 — Reference Notes

> Source: <https://github.com/open-watcom/open-watcom-v2>  
> Scraped: 2026-06-19

## What It Is

Open Watcom V2 is a community fork of the original Open Watcom 1.9 compiler suite,
hosted on GitHub under the `open-watcom` organization. The primary goal is forward
development on **64-bit support** (both tools that are 64-bit executables and code
that targets 64-bit). All other improvements are also accepted.

The original OpenWatcom.org site (Bugzilla, Wiki, News, Perforce) has been dead for
years. V2 on GitHub is the only actively maintained branch.

**Stats (June 2026):** 1.2k stars, 198 forks, 56 contributors, ~19,500 commits.
Led primarily by Jiří Malák (`jmalak`).

---

## Release Cadence

| Track | Tag | Frequency |
|---|---|---|
| CI build | `Last-CI-build` | After every push |
| Current build | `Current-build` | Daily (if any change) |
| Monthly archive | `YYYY-MM-DD-Build` | Monthly |

All builds are labeled **pre-release** — there is no formal "2.0 release" yet.
The compiler itself reports `Version 2.0 beta` in its banner.

Downloads: GitHub Releases page. Older installers also on SourceForge.

Community: GitHub Discussions, Reddit `r/OpenWatcom`, Discord (invite in README).

---

## Supported Targets

OW V2 can cross-compile to a wide range of targets from any host:

| Target | Notes |
|---|---|
| DOS 16-bit | Real-mode, small/large/huge memory models |
| DOS 32-bit | Flat model via DOS extenders (PMode/W, DOS/4GW, Causeway) |
| OS/2 (16 and 32-bit) | |
| Windows 3.x / 9x / NT | Win16, Win32 |
| Linux x86 / x86-64 | |
| MIPS (WinNT) | Cross-compile only, runtime library incomplete |
| PowerPC (Linux) | Experimental, runtime library has gaps |
| Alpha | |
| ARM | Limited |

Build hosts: Windows, Linux, FreeBSD/NetBSD/OpenBSD (BSD support added 2025).

---

## Tools

| Tool | Purpose |
|---|---|
| `wcc` / `wcc386` | C compiler — 16-bit / 32-bit x86 |
| `wpp` / `wpp386` | C++ compiler — 16-bit / 32-bit x86 |
| `wcl` / `wcl386` | Compile-and-link frontend (wraps wcc + wlink) |
| `wlink` | Linker |
| `wasm` | Assembler (TASM-compatible syntax for 32-bit flat) |
| `wmake` | Make utility |
| `wdis` | Disassembler |
| `wd` | Source-level debugger |

The POSIX-style driver names (`owcc`, `bowcc`) are also provided and accept
GCC-style flags — useful for CMake integration.

---

## C99 Compliance (`-zastd=c99`)

C99 is **partial and undocumented**. Enable available extensions with `-zastd=c99`.

### Implemented

- `long long int` (with library support)
- Designated initializers
- `// comments`
- `inline` functions
- `_Bool` (with `-zastd=c99`)
- `__func__` predefined identifier
- Variadic macros (`...` in `#define`)
- Mixed declarations and code
- Flexible array members
- Trailing comma in enum
- `_Pragma` operator
- Hexadecimal floating-point constants
- `<stdint.h>`, `<inttypes.h>`, `<stdbool.h>`, `<fenv.h>`
- `va_copy`

### Not Implemented

- Variable length arrays (VLAs)
- Compound literals
- Universal character names (`\u`, `\U`)
- `static` and type qualifiers in array parameter declarators
- `<tgmath.h>`
- `%a`/`%A` in `scanf` (only in `printf`)

### Notes

- `-zastd=c99` also enables `intmax_t`/`uintmax_t` in preprocessor arithmetic
- `-aa` enables relaxed aggregate/union initialization (matches C99)
- `SIZE_MAX` is **not** available from `<stddef.h>` — use `(size_t)-1`
- `stricmp` is the native Watcom function for case-insensitive compare;
  `clibext.h` maps it to `strcasecmp` on POSIX hosts

---

## C++ Status

C++ support is based on C++03 with incremental C++11 work ongoing. The wiki
tracks progress in `C++ Language`, `C++ Library`, and `C++ Library Status` pages.
Not suitable as a conforming C++11 compiler yet.

---

## Assembler (wasm) — Relevance to FX Player

`wasm` is the assembler included with OW V2. It supports TASM-compatible syntax
for the 32-bit flat model, which is what the original FX Player `.ASM` files use
(`MIXR_32N.ASM`, `MIXR_32I.ASM`, `MIXR_16N.ASM`).

Key points:
- TASM-style segments and macros work in `wasm`
- `argdd` self-modifying code pattern uses `dword ptr` and direct ES: references —
  these are legal in `wasm` 32-bit flat mode
- The original `MAKEFILE` uses `wasm` directly — no changes needed for OW V2

---

## Building DOS 32-bit Targets

Typical invocation for a DOS/32-bit flat binary with PMode/W:

```
wcl386 -bt=dos -l=pmodew -3s -ox foo.cpp
```

| Flag | Meaning |
|---|---|
| `-bt=dos` | Build target: DOS |
| `-l=pmodew` | Link system: PMode/W extender |
| `-3s` | Pentium-compatible, stack-based calling |
| `-mf` | Flat memory model (default for 386) |
| `-ox` | Optimize for speed |
| `-zastd=c99` | Enable C99 extensions |

For the FX Player original build, `wcl386` with the flags in `MAKEFILE` should
work unchanged under OW V2.

---

## CMake Integration

OW V2 ships CMake toolchain files. Basic usage:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(myapp C)
```

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/watcom.cmake \
      -DCMAKE_SYSTEM_NAME=DOS \
      -B build
```

The wiki page `OW tools usage with CMake` has the full toolchain file and
variable reference. VS Code integration is documented in `OW tools usage with VSCode`.

---

## Key Gotchas vs. OW 1.9

| Topic | Detail |
|---|---|
| `SIZE_MAX` | Not in `<stddef.h>`; use `(size_t)-1` |
| `stricmp` | Native; `strcasecmp` available via `clibext.h` on POSIX |
| Environment vars | Renamed: `RELROOT`→`OWRELROOT`, `OBJDIR`→`OWOBJDIR`, etc. |
| BSD support | Added 2025; FreeBSD/NetBSD/OpenBSD each need separate native build |
| C++ linking | Fixed June 2026 (`ca307b7`) — older builds may have link issues |

---

## Original F/X Player architecture (relevant to the port)

These notes describe the 1998 DOS source in `_original/` — the ground truth the
port must reproduce bit-exactly.

- **State is global parallel arrays** sized `[256]` (`ChannelVolume[256]`,
  `ChannelSamplePosition[256]`, etc.). Designed for up to 255 channels in
  preparation for XM/IT.
- **Format separation is clean:** `DAT_*` files load+parse, `EFC_*` files
  interpret per-row effects, `MIXER.CPP` orchestrates, `MIXR_*.ASM` are the
  inner kernels.
- **Sample-accurate timing** (no PIT/timer use). Tempo is converted to
  "samples per tick" at the chosen mix rate → fully deterministic and reproducible.
- **Mix buffer is 32-bit signed integers, 16.8 fixed-point per channel, with
  8 bits of headroom** for accumulating up to 256 channels without saturation.
- **16 ASM mixer kernel variants** declared in `MIXER.H`:
  `{8,16}-bit input × {mono,stereo} output × {16,32}-bit mix buffer × {nearest,linear-interp}`.
  Three of the four `.ASM` files are populated (`MIXR_16N`, `MIXR_32N`, `MIXR_32I`);
  the 16-bit interpolated set is declared but unimplemented.
- **The ASM uses self-modifying code** (the `argdd` macro pattern) to patch
  volume, fraction increments, and buffer addresses directly into the instruction
  stream — saves a memory load per inner-loop iteration on Pentium U/V pipelines.
  Self-modifying code stays in the DOS build only; modern OSes enforce W^X and
  on OoO cores SMC is slower than a memory load.
- **PMode/W flat 32-bit memory model** — no segment tricks, pointers are flat.
  Maps cleanly to modern flat memory and Cortex-M.

---

## Links

- Repo: <https://github.com/open-watcom/open-watcom-v2>
- Wiki: <https://github.com/open-watcom/open-watcom-v2/wiki>
- Docs (generated): <https://open-watcom.github.io/open-watcom-v2-wikidocs/>
- C99 status: <https://github.com/open-watcom/open-watcom-v2/wiki/C99-Compliance>
- CMake usage: <https://github.com/open-watcom/open-watcom-v2/wiki/OW-tools-usage-with-CMake>
- Releases: <https://github.com/open-watcom/open-watcom-v2/releases>
