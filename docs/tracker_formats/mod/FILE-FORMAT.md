# MOD File Format — Binary Layout

All integers are **big-endian** (Amiga convention). "Word" = 16-bit (2 bytes). Lengths in sample headers are in words; multiply by 2 for bytes.

Primary sources: [aes.id.au spec](https://aes.id.au/modformat.html), [MultimediaWiki](https://wiki.multimedia.cx/index.php/Protracker_Module).

## Detecting 15 vs 31 sample format

Before loading anything, seek to offset **0x0438** and read 4 bytes. If all four bytes are printable ASCII (0x20–0x7E inclusive), it is a **31-sample** module and the tag tells you the channel count and tracker. If any byte falls outside that range, it is a **15-sample** SoundTracker module with no tag field.

## 15-sample layout (original SoundTracker)

| Offset | Size | Description |
|--------|------|-------------|
| 0x0000 | 20 bytes | Song title, null-padded |
| 0x0014 | 15 × 30 = 450 bytes | Sample descriptors (see below) |
| 0x01C6 | 1 byte | Number of song positions (order list length), 1–128 |
| 0x01C7 | 1 byte | Restart byte (traditionally `0x78`) |
| 0x01C8 | 128 bytes | Order list (pattern indices 0–63) |
| 0x0248 | NOP × 1024 bytes | Pattern data |
| — | variable | Sample data (concatenated, signed 8-bit) |

`NOP` (number of patterns) = highest pattern index in the full 128-entry order list + 1.

## 31-sample layout (NoiseTracker / ProTracker)

| Offset | Size | Description |
|--------|------|-------------|
| 0x0000 | 20 bytes | Song title, null-padded |
| 0x0014 | 31 × 30 = 930 bytes | Sample descriptors |
| 0x0196 | 1 byte | Number of song positions, 1–128 |
| 0x0197 | 1 byte | Restart byte (`0x7F` in ProTracker, `0x78` in SoundTracker, restart position in NoiseTracker/FastTracker) |
| 0x0198 | 128 bytes | Order list (pattern indices) |
| 0x0218 | — | — |
| **0x0438** | **4 bytes** | **Magic tag** (see [VARIANTS.md](VARIANTS.md)) |
| 0x043C | NOP × channels × 64 × 4 bytes | Pattern data |
| — | variable | Sample data |

The 31-sample fixed header is exactly **1084 bytes** (0x043C), then pattern data begins.

## Sample descriptor (30 bytes each)

| Offset | Size | Description |
|--------|------|-------------|
| 0 | 22 bytes | Sample name, null-padded. A name starting with `#` is typically a text message, not an instrument. |
| 22 | 2 bytes | Sample length in **words** (×2 = bytes). A length of 1 = empty sample (first word overwritten by tracker). |
| 24 | 1 byte | Finetune: signed nibble −8..+7 stored as 0x00–0x0F (0x00–0x07 = +0..+7, 0x08–0x0F = −8..−1). Each step = 1/8 semitone. |
| 25 | 1 byte | Default volume: 0–64. |
| 26 | 2 bytes | Loop start offset in **words** from sample start. |
| 28 | 2 bytes | Loop length in **words**. Loop active if > 1. A value of 0 may crash real Amiga hardware. |

Periods for the 8 finetune values (+0..+7, −8..−1) are stored in separate lookup tables — see [EFFECTS.md](EFFECTS.md) for the period table.

## Order list

- 128 bytes always stored; only the first `song_positions` entries are played.
- Scan the **entire** 128 entries (regardless of `song_positions`) to find the maximum pattern index — that determines how many patterns exist in the file.
- Values: 0–63 (ProTracker); up to 127 in `M!K!` modules.
- Order value `0xFE` = "marker" (skip in some players); `0xFF` = end of song (rare in MOD).

## Pattern cell encoding (4 bytes per cell, big-endian)

Read each 4-byte cell as a big-endian 32-bit value, then extract fields:

```
Byte 0   Byte 1   Byte 2   Byte 3
SSSS pppp pppppppp ssss CCCC XXXX XXXX
```

| Field | Bits | Description |
|-------|------|-------------|
| `S` (high) | Byte 0 bits 7–4 | Upper 4 bits of sample number |
| `period` | Byte 0 bits 3–0, Byte 1 bits 7–0 | 12-bit Amiga period value |
| `s` (low) | Byte 2 bits 7–4 | Lower 4 bits of sample number |
| `C` | Byte 2 bits 3–0 | Effect command (0–F) |
| `X` | Byte 3 bits 7–0 | Effect parameter (two nibbles: x and y) |

**Sample number** = `(S << 4) | s`, giving 0–31 (0 = no new sample; same as previous).
**Period** = 12-bit big-endian: the DMA clock divisor. See period table below.
**Effect** `C` and parameter `XX` together define the effect (see [EFFECTS.md](EFFECTS.md)).

### Period table (finetune 0)

```
        C    C#   D    D#   E    F    F#   G    G#   A    A#   B
Oct 1: 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453
Oct 2: 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226
Oct 3: 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113
```

Octaves 0 and 4 are non-standard extensions (used by PC trackers):
```
Oct 0:1712,1616,1525,1440,1357,1281,1209,1141,1077,1017, 961, 907
Oct 4: 107, 101,  95,  90,  85,  80,  76,  71,  67,  64,  60,  57
```

Amiga hardware limits: **113 ≤ period ≤ 856**. Period 0 means "keep previous period". PC trackers commonly extend this range.

### Sample rate calculation

```
sample_rate = clock / (2 × period)
```

- PAL Amiga: clock = 7,093,789.2 Hz → C-2 (period 428): ≈ 8287.1 Hz
- NTSC Amiga: clock = 7,159,090.5 Hz → C-2 (period 428): ≈ 8363.4 Hz

The 8363 Hz figure (NTSC approximation) became the de-facto standard used in most PC players and is the `c2spd` default in S3M.

## Sample data

Immediately after pattern data, all sample audio data follows in sequence (same order as sample descriptors). Each sample is:
- **Signed 8-bit PCM** (unlike 669 which stores unsigned and requires XOR 0x80)
- The first word (2 bytes) of any looping sample is overwritten by the tracker with loop pointer data — treat it as silence/zero
- Maximum size: 65535 words = 131,070 bytes per sample

## StarTrekker FLT8 special case

`FLT8` modules interleave two 4-channel patterns per order position. Order entry N plays pattern N (channels 1–4) and pattern N+1 (channels 5–8) simultaneously. Players must merge them.

## Channel stereo assignment (Amiga hardware)

Paula hardware fixed panning (non-adjustable on original Amiga without software mixing):
- Channels 1 and 4: **left**
- Channels 2 and 3: **right**

For PC multi-channel MODs (6, 8, …), trackers typically alternate L/R: channels 1,4,5,8,… left; 2,3,6,7,… right. FX Player uses this pattern for its 8-channel support.
