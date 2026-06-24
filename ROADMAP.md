# Roadmap — Next milestones

Per-change history lives in `CHANGELOG.md`; known bugs/quirks in `BUGS.md`.
This file tracks work that is planned but not yet started.

---

## Mixer SIMD variants

Implement and validate the remaining mixer variants, each bit-exact against
`mixer_scalar.c`:

- `mixer_x86_sse2.c`
- `mixer_x86_avx2.c`
- `mixer_armcm_dsp.c` (Cortex-M4F `__SMLAL` / `__SSAT` intrinsics)

## Engine state encapsulation

Encapsulate the global-array state into a context struct. Prerequisite: CTest
suite must be green (it now is). This unblocks multi-instance use and the MCU
port where a global is unacceptable.

## XM / IT formats

Planned in the original F/X Player but never implemented. Low priority until
the existing three formats are fully hardened.

## Web host — GitHub Pages

Live at **https://fstollar.github.io/fxplayer** (repo is public; `gh-pages` branch).

To redeploy after changes: run `build-web.sh` — it rebuilds and force-pushes `gh-pages`.

See `BUILD.md` for build/serve/deploy commands.
See `docs/superpowers/specs/2026-06-23-web-host-design.md` for design notes.

## Web host — AudioWorklet option 1: Worker + SharedArrayBuffer (if needed)

The current web host uses **option 2**: a Web Worker pre-renders audio and
transfers `Float32Array` chunks via `postMessage` to the AudioWorklet.
`process()` reads from a queue — zero WASM on the audio thread.

If this still proves insufficient on very low-end devices, **option 1** is the
canonical solution described in the Chrome "Audio Worklet Design Pattern" article:

- Worker renders WASM → writes into a **SharedArrayBuffer** ring buffer
- AudioWorklet reads from the SAB directly — no message overhead, truly lock-free
- Synchronisation via `Atomics.waitAsync` (Worker) + `Atomics.notify` (Worklet)
- **Prerequisite:** Cross-Origin Isolation (COOP + COEP response headers)
  — required for `SharedArrayBuffer` in browsers.
- **GitHub Pages workaround:** add `coi-serviceworker.min.js` (a ~1 KB Service
  Worker that injects the headers client-side). Many WASM projects on Pages use
  this exact pattern.
- Reference: https://developer.chrome.com/blog/audio-worklet-design-pattern/
- Reference: https://github.com/nicowillis/coi-serviceworker

---

## Bug audit

Systematically work through `BUGS.md` and classify each quirk as:

- **(a)** faithful reproduction required for bit-exactness — leave alone
- **(b)** fixable in the C99 core without breaking bit-exactness — fix
- **(c)** fixable only in the DOS original — decide whether/how to fix

Document the decision for each entry.

## Remove floating-point from `src/core/`

`mixer_scalar.c` uses `double` for the master-volume table init
(`build_vol_table`, lines ~604–631). Replace with fixed-point arithmetic.
Until done, `-ffast-math` is kept on `fxcore` (noted in `src/core/CMakeLists.txt`).
