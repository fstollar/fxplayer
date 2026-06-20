# F/X Player — Change Log

## _work/ — Working DOS build (OpenWatcom + TASM, runs in DOSBox-X)

All changes are in `_work/`. The `_original/` source is never touched.

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
