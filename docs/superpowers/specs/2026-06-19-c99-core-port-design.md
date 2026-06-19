# C99 Core Port — Design Spec

**Date:** 2026-06-19  
**Status:** Approved  
**Phase:** 1 of N — S3M bit-exact render

---

## Goal

Port the S3M playback path of F/X Player from the DOS C++ source (`_work/`) to a
C99 engine library (`core/`) that produces **bit-identical** WAV output to the
reference renders in `tests/reference_renders/`. Validation is a CTest sha256
comparison. No miniaudio, no real-time playback in this phase.

---

## Directory layout

New files created, existing files extended:

```
core/
  include/fx/
    fx.h              ← extend: fx_workspace_size, fx_load, fx_render_frames,
                                fx_close, fx_err, soft_clip in fx_config
  engine/
    fx.c              ← extend: implement dispatch for load/render/close
    channel.h         ← new: shared channel-state type aliases (uint32_t arrays)
  format/
    s3m.c             ← new: dat_s3m.cpp → C99
    s3m.h             ← new: internal header
  effect/
    efc_s3m.c         ← new: efc_s3m.cpp → C99
    efc_s3m.h         ← new: internal header
  mixer/
    mixer_scalar.c    ← implement: mixer.cpp scalar path → C99 (was placeholder)
    mixer_scalar.h    ← new: internal header
  util/
    calc.c            ← new: dat_calc.cpp → C99 (DAT_calcSpeed, divide_64bit)
    calc.h            ← new: internal header
tests/
  render_s3m/
    main.c            ← new: render TEST.S3M → WAV, exit 0 on success
    CMakeLists.txt    ← new: add_executable + CTest sha256 compare
cmake/
  check_sha256.cmake  ← new: ~10-line CMake script for sha256 assertion
```

---

## C99 translation rules

The DOS source uses Watcom C++ with DOS-specific constructs. Systematic replacements:

| Original | C99 replacement |
|---|---|
| `#include <dos.h>`, `<conio.h>` | Drop — no OS headers in core |
| `malloc` / `free` | Removed — caller supplies workspace buffer |
| `unsigned long` (32-bit on Watcom) | `uint32_t` |
| `signed long` | `int32_t` |
| `unsigned short` | `uint16_t` |
| `unsigned char` | `uint8_t` |
| `char *` buffer pointers | `uint8_t *` |
| `FILE *` / `fopen` / `fread` | Caller passes `const uint8_t *data, size_t size` |
| `#pragma aux divide_64bit` (Watcom inline ASM) | Plain C: `(uint64_t)` arithmetic |
| `extern "C" {}` guards | Stripped — C99, no C++ |
| `printf` / `cprintf` debug output | Stripped — no stdio in core |
| `exit()` on error | Return `fx_err` code to caller |

**Load buffer strategy:** `s3m_load()` takes `const uint8_t *data, size_t size,
uint8_t *workspace, size_t ws_size`. Pattern data, sample data, order list, and
instrument pointers are all carved from the caller-supplied workspace. The engine
stores raw pointers into this workspace; the host owns the allocation.

**Global state:** Internal variables (`ChannelVolume[256]`, etc.) remain
file-scope globals in each `.c` file — same layout as the DOS source. Struct
encapsulation comes *after* a passing test suite.

---

## Public API additions to `core/include/fx/fx.h`

```c
/* Error codes */
typedef enum {
    FX_OK            =  0,
    FX_ERR_FORMAT    = -1,   /* unrecognised or corrupt module */
    FX_ERR_WORKSPACE = -2,   /* workspace buffer too small     */
    FX_ERR_STATE     = -3,   /* engine not loaded              */
} fx_err;

/* Updated fx_config — adds soft_clip field */
typedef struct {
    uint32_t          sample_rate;
    uint8_t           channels;
    fx_output_format  output_format;
    uint8_t           interpolate;   /* 0 = nearest, 1 = linear */
    uint8_t           soft_clip;     /* 0 = hard clip, 1 = soft clip */
} fx_config;

/*
 * Parse the module header and report the workspace bytes required.
 * Call this before fx_load to size the workspace buffer.
 * On embedded targets: call at load time, assert(ws <= MY_STATIC_SIZE).
 */
fx_err fx_workspace_size(const void *data, size_t size, size_t *out_bytes);

/*
 * Load a module from a memory buffer.
 * workspace/ws_size: caller-owned scratch carved for pattern+sample data.
 * Returns FX_OK or an error code.
 */
fx_err fx_load(const void *data, size_t size,
               void *workspace, size_t ws_size);

/*
 * Render `frame_count` interleaved frames into `out`.
 * out must hold frame_count * channels * sizeof(sample) bytes.
 * Returns frames actually rendered (< frame_count at end of song).
 */
size_t fx_render_frames(void *out, size_t frame_count, const fx_config *cfg);

/* Release engine state. Safe to call even if fx_load failed. */
void fx_close(void);
```

---

## Test binary (`tests/render_s3m/main.c`)

1. Read `argv[1]` (path to `TEST.S3M`) into a heap buffer
2. Call `fx_workspace_size()` → `malloc()` workspace → `fx_load()`
3. Render to a PCM buffer in 1024-frame chunks until `fx_render_frames()` returns < 1024
4. Write a minimal RIFF WAV header + PCM to `argv[2]`
5. Exit 0 on success, non-zero on any error

Render config used (must match reference renders exactly):

```c
fx_config cfg = {
    .sample_rate   = 48000,
    .channels      = 2,
    .output_format = FX_OUTPUT_S16,
    .interpolate   = 1,
    .soft_clip     = 1,
};
```

---

## CTest wiring (`tests/render_s3m/CMakeLists.txt`)

```cmake
add_executable(render_s3m main.c)
target_link_libraries(render_s3m PRIVATE fx::core)

add_test(NAME render_s3m COMMAND render_s3m
    ${CMAKE_SOURCE_DIR}/tests/reference_renders/TEST.S3M
    ${CMAKE_CURRENT_BINARY_DIR}/out.wav)

add_test(NAME compare_s3m
    COMMAND ${CMAKE_COMMAND}
        -DFILE=${CMAKE_CURRENT_BINARY_DIR}/out.wav
        -DEXPECTED_SHA256=5edd49d54cd161dfac8c88ffbec30bf6fcadea4a2e3cd053db3dbb1c68c30834
        -P ${CMAKE_SOURCE_DIR}/cmake/check_sha256.cmake)

set_tests_properties(compare_s3m PROPERTIES DEPENDS render_s3m)
```

`cmake/check_sha256.cmake`: compute sha256 of `FILE`, compare to
`EXPECTED_SHA256`, `message(FATAL_ERROR ...)` on mismatch.

---

## Build system changes

- **`core/CMakeLists.txt`**: add `format/s3m.c`, `effect/efc_s3m.c`,
  `util/calc.c` to `FXCORE_SOURCES`; `mixer/mixer_scalar.c` was already listed
- **Root `CMakeLists.txt`**: no change needed — `FX_BUILD_TESTS` gate already
  calls `add_subdirectory(tests)`
- **`tests/CMakeLists.txt`**: new, adds `render_s3m` subdirectory
- **`cmake/check_sha256.cmake`**: new helper script

---

## Success criterion

```
cmake -B build -DFX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

All tests pass. `compare_s3m` exits 0, meaning the rendered WAV sha256-matches
`tests/reference_renders/TEST.wav` (sha256: `5edd49d54cd161dfac8c88ffbec30bf6fcadea4a2e3cd053db3dbb1c68c30834`).
