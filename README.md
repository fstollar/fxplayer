# F/X Player — Cross-platform port

Porting **F/X Player** — a 1998 DOS MOD/S3M tracker module player by Apollo of STIGMA —
to a modern, portable codebase while keeping the original DOS build alive.

**Four build targets from one C99 engine core:**

1. **Modern CLI player** (Linux / macOS / Windows) — C++ host, audio out via miniaudio.
2. **Original DOS build** — OpenWatcom + TASM + PMode/W, SoundBlaster / WSS unchanged.
3. **Bare-metal embedded** — 32-bit ARM Cortex-M with I2S DMA out.
4. **Web / GitHub Pages** — C99 core compiled to Wasm32; AudioWorklet host.

See **[BUILD.md](BUILD.md)** for toolchain requirements and step-by-step build instructions.

---

## Repo layout

```
fxplayer/
├── README.md                  # this file
├── BUILD.md                   # build instructions for all targets
├── ARCHITECTURE.md            # engine design: original + port decisions
├── ROADMAP.md                 # planned work / next milestones
├── CHANGELOG.md               # per-change history
├── BUGS.md                    # known bugs and quirks
├── CLAUDE.md                  # AI assistant instructions
├── build-cli.sh               # shortcut: cmake --build build/ (CLI target)
├── build-web.sh               # compile C99 core to Wasm32 + deploy to gh-pages
├── serve-web.sh               # build + serve web host locally for development
├── CMakeLists.txt             # CLI + tests build (CMake)
├── _original/                 # 1998 source — NEVER modified
├── _work/                     # working DOS copy (bug fixes applied here)
├── _watcom2/                  # OpenWatcom V2 toolchain (gitignored; see README.txt)
├── src/
│   ├── core/                  # C99 engine: no deps, no I/O, no alloc
│   │   ├── include/fx/        # public C API headers
│   │   ├── engine/            # state, channel mgmt, mix dispatch
│   │   ├── format/            # mod.c, s3m.c, m669.c, wav.c
│   │   ├── effect/            # efc_mod.c, efc_s3m.c, efc_669.c
│   │   └── mixer/
│   │       ├── mixer_scalar.c     # portable reference (bit-exact ground truth)
│   │       ├── mixer_x86_sse2.c
│   │       ├── mixer_x86_avx2.c
│   │       ├── mixer_armcm_dsp.c
│   │       └── mixer_x86_p5.asm   # original ASM (DOS only)
│   └── host/
│       ├── cli/               # C++ CLI player (miniaudio)
│       ├── dos/               # DOS host (DEV_SB / DEV_WSS / DMA / IRQ)
│       ├── web/               # Wasm32 shim + AudioWorklet + HTML UI
│       └── mcu_example/       # bare-metal STM32 I2S DMA example
├── tests/
│   ├── reference_renders/     # bit-exact WAV ground truths (sha256-validated)
│   └── render/                # CTest render harness
├── build/                     # CMake build output (gitignored)
├── build-web/                 # Wasm build output (gitignored)
├── release/                   # release binaries (gitignored)
├── cmake/                     # CMake helper scripts (CPM)
└── docs/                      # reference documentation
```

## Supported formats

| Format | Channels | Notes |
|---|---|---|
| S3M | up to 32 | Scream Tracker 3 |
| MOD | 4 or 8 | Amiga / ProTracker |
| 669 | up to 8 | Composer 669 / UNIS 669 |

## Validation

Output is fully deterministic. Every port variant must produce **bit-identical** WAV
output to the scalar C reference, which itself matches the original DOS render.
Comparison is `sha256(wav)` — no tolerance, no fuzzy matching.
