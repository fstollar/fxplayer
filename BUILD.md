# FX Player — Build Guide

Four build targets share a single C99 engine core in `src/core/`. Each host has
its own toolchain and entry point; the core itself has no host dependencies.

---

## 1. DOS build — OpenWatcom V2 + TASM

**Source tree:** `_work/` (the working copy; `_original/` is never touched).

### Getting OpenWatcom V2

`_watcom2/` is **not** part of the repo (gitignored). Download the toolchain from the
Open Watcom V2 GitHub Releases page:

```
https://github.com/open-watcom/open-watcom-v2/releases
```

This build uses the **2026-06-19** `Current-build` snapshot
(`wpp386` reports `Version 2.0 beta Jun 19 2026`). Any snapshot from around that date
or later should work; avoid anything older than early 2026 — earlier snapshots have
issues with the Linux 64-bit host tools.

Download the Linux tarball (`ow-snapshot.tar.xz`), then unpack it so the toolchain
lands at `_watcom2/ow2/`:

```sh
mkdir -p _watcom2
cd _watcom2
tar -xf /path/to/ow-snapshot.tar.xz   # must produce ow2/ here
```

The directory structure inside the tarball already has an `ow2/` top-level, so the
`tar` command should give you `_watcom2/ow2/binl64/`, `_watcom2/ow2/h/`, etc.
Verify with `ls _watcom2/ow2/binl64/wpp386`.

**Toolchain:** once unpacked, the build script sets `WATCOM` and `PATH` itself —
no shell environment setup is needed.

**Build** (Linux host only — uses the `binl64/` OpenWatcom binaries):
```sh
cd _work
./build-dos.linux.sh
```
Output: `_work/FX.EXE` (~156 K, PMode/W 32-bit DOS extender).

**Run in DOSBox-X:**
```sh
# DOSBox-X config and drive mappings live in _dosbox/
dosbox-x -conf _dosbox/dosbox-x.conf
```
Inside DOSBox the `C:` drive maps to `_dosbox/C/`.
Copy `FX.EXE` and module files to `_dosbox/C/` before running.
**8.3-safe filenames are required** on the `C:` mount (DOS can't see long names).

**WAV render (for reference / validation):**
```
FX.EXE -w:OUT.WAV -n:TEST.S3M
```
Or use `tests/render-dosbox.sh --native` to batch-render the reference WAVs.

**Reference docs:**
- `docs/open-watcom-v2.md` — compiler/linker flags, OW V2 C99 notes
- `docs/openwatcom-v2-bugs.md` — known OW V2 bugs and workarounds

---

## 2. Linux / macOS CLI build — CMake + miniaudio

**Requirements:**
- CMake ≥ 3.16
- C99 + C++17 compiler (GCC or Clang)
- Internet access on first configure (CPM downloads deps to `build/_deps/`)

**Configure and build:**
```sh
cmake -B build
cmake --build build -j$(nproc)
# or just:
./build-cli.sh
```

**With tests enabled:**
```sh
cmake -B build -DFX_BUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Output binary: `build/fxplayer`

**CMake options:**

| Option | Default | Description |
|---|---|---|
| `FX_BUILD_CLI` | `ON` | Build the C++ CLI host |
| `FX_BUILD_TESTS` | `OFF` | Build CTest regression suite |
| `VALGRIND_BUILD` | `OFF` | Drop AVX-512 so valgrind/callgrind can instrument |

**CPM dependency cache** (avoids re-downloading across builds):
```sh
export CPM_SOURCE_CACHE=~/.cache/cpm
```
Managed deps: miniaudio 0.11.21, cxxopts 3.2.0. Both download to `build/_deps/`.

**Usage:**
```
fxplayer <module.s3m|.mod|.669> [options]

  -r <hz>           Sample rate (default 44100)
  -c <channels>     Output channels (1=mono, 2=stereo)
  -l <n>            Loop count: 0=no loop, N=N+1 plays, -1=infinite (default)
  -v <0-255>        Initial volume
  --no-interpolation  Disable sample interpolation
  --no-softclip       Disable soft-clip limiter
```

**Interactive controls (keyboard):**

| Key | Action |
|---|---|
| Space | Pause / resume |
| ← / → | Jump order back / forward |
| ↑ / ↓ | Volume up / down |
| Q / Esc | Quit |

---

## 3. Web host — Wasm32 + AudioWorklet

**Requirements:**
- `clang` with `wasm32-unknown-unknown` target support
- `lld-23` (the linker is invoked via clang's `-fuse-ld` path; version 23 required)
- Python 3 (for the dev server)
- `openssl` (for the LAN HTTPS server)

**Build only (no deploy):**
```sh
./build-web.sh --no-deploy
```
Output: `fxcore.wasm` in the repo root.

**Dev server — localhost:**
```sh
./serve-web.sh
# serves at http://localhost:8765
```

**Dev server — LAN (HTTPS, for mobile / other devices):**
```sh
./serve-web.sh --network [port]
# serves at https://<lan-ip>:<port>
# AudioWorklet requires a secure context on non-localhost origins;
# a self-signed cert is generated automatically — click through the
# browser warning once (Firefox: Advanced → Accept the Risk,
# Chrome/Edge: Advanced → Proceed anyway).
```

**Deploy to GitHub Pages:**
```sh
./build-web.sh          # compiles + pushes to gh-pages branch
```
This commits to the `gh-pages` branch and pushes to `origin`.

**Enable GitHub Pages** (one-time, manual):
1. Make the repo public (currently private).
2. Settings → Pages → Source: `gh-pages` branch / root.

---

## 4. Validation / regression tests

The mixer output is fully deterministic. Validation is sha256-exact — no tolerance.

**Run the CTest suite (Linux host):**
```sh
cmake -B build -DFX_BUILD_TESTS=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```
6 tests: `compare_s3m`, `compare_mod`, `compare_669` (2 variants each).

**Regenerate DOS reference WAVs** (when the DOS build changes):
```sh
tests/render-dosbox.sh --native
```
Reference WAVs live in `tests/reference_renders/`.
After regenerating, update the sha256 hashes in `tests/CMakeLists.txt`.
