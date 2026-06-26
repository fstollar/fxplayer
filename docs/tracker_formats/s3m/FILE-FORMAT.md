# S3M File Format — Binary Layout

Primary sources: [ModdingWiki](https://moddingwiki.shikadi.net/wiki/S3M_Format), [MultimediaWiki](https://wiki.multimedia.cx/index.php?title=Scream_Tracker_3_Module), [TECH.DOC](https://pollak.thebe.de/b/Modules/docs/TECH.txt).

**Authoritative reference implementation:** [st3play by 8bitbubsy](https://github.com/8bitbubsy/st3play) — a direct C port of the Scream Tracker 3.21 ASM/C source code. When the above docs conflict with actual ST3 behavior, st3play is the tie-breaker.

All integers are **little-endian**. Offsets are in bytes from the file start unless noted.

## Parapointers

S3M uses "parapointer" addresses inherited from the 8086 segmented memory model: each parapointer is a 16-bit value representing an offset in units of 16 bytes (one paragraph). To convert to a file byte offset: `byte_offset = parapointer * 16`.

PCM instrument sample data uses a 24-bit parapointer: the high byte is stored separately (`ptrDataH`) and the low 16 bits together (`ptrDataL`): `byte_offset = ((ptrDataH << 16) | ptrDataL) * 16`.

## Main header (0x0060 = 96 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x0000 | 28 bytes | `title` | Song title, null-terminated |
| 0x001C | 1 byte | `sig1` | Always `0x1A` |
| 0x001D | 1 byte | `type` | Always `0x10` for S3M |
| 0x001E | 2 bytes | reserved | `0x0000` |
| 0x0020 | 2 bytes | `orderCount` | Number of entries in order table (should be even) |
| 0x0022 | 2 bytes | `instrumentCount` | Number of instruments |
| 0x0024 | 2 bytes | `patternPtrCount` | Number of pattern parapointers |
| 0x0026 | 2 bytes | `flags` | See flags table below |
| 0x0028 | 2 bytes | `trackerVersion` | Upper nibble = tracker ID, lower 12 bits = version |
| 0x002A | 2 bytes | `sampleType` | `1` = signed samples (deprecated), `2` = unsigned samples |
| 0x002C | 4 bytes | `sig2` | Magic: `"SCRM"` |
| 0x0030 | 1 byte | `globalVolume` | Initial global volume |
| 0x0031 | 1 byte | `initialSpeed` | Initial speed (ticks per row) |
| 0x0032 | 1 byte | `initialTempo` | Initial tempo (rows per second × speed) |
| 0x0033 | 1 byte | `masterVolume` | Bit 7: `1`=stereo, `0`=mono; bits 6–0: mixing volume |
| 0x0034 | 1 byte | `ultraClickRemoval` | GUS click-removal channel count |
| 0x0035 | 1 byte | `defaultPan` | `0xFC` (252) = use per-channel pan values in header; else ignore |
| 0x0036 | 8 bytes | reserved | Unused; some trackers store data here |
| 0x003E | 2 bytes | `ptrSpecial` | Parapointer to extra data (valid only if flags bit 7 set) |
| 0x0040 | 32 bytes | `channelSettings` | Channel-to-physical-output map (see below) |

Immediately following the fixed header (at offset 0x0060):
- `orderCount` bytes — order list (pattern indices; `0xFE` = skip/marker, `0xFF` = end of song)
- `instrumentCount` × 2 bytes — parapointers to instrument blocks
- `patternPtrCount` × 2 bytes — parapointers to pattern blocks
- 32 bytes — per-channel pan values (only present and used if `defaultPan == 0xFC`)

### Flags

| Bit | Value | Description |
|-----|-------|-------------|
| 0 | 1 | ST2 vibrato — deprecated |
| 1 | 2 | ST2 tempo — deprecated |
| 2 | 4 | Amiga slides — deprecated |
| 3 | 8 | Zero-volume optimization: auto-silence looping notes at vol=0 for >2 rows |
| 4 | 16 | Amiga limits: ignore notes beyond Amiga hardware pitch range |
| 5 | 32 | SoundBlaster filter/SFX — deprecated |
| 6 | 64 | ST3.00 volume slides: also slide on first tick (set implicitly if `trackerVersion == 0x1300`) |
| 7 | 128 | `ptrSpecial` is valid |

### channelSettings values

| Value | Description |
|-------|-------------|
| 0–7 | PCM left channels 1–8 |
| 8–15 | PCM right channels 1–8 |
| 16–24 | OPL2 melody channels 1–9 |
| 25–29 | OPL2 percussion: bass drum, snare, tom, cymbal, hi-hat |
| 255 | Channel unused |
| 128–254 | Same as above + 128 (channel disabled/muted) |

Note: OPL percussion channels are defined in the format but were never implemented in Scream Tracker 3 itself.

### trackerVersion known values (little-endian hex)

| Value | Tracker |
|-------|---------|
| 0x1300 | ScreamTracker 3.00 |
| 0x1301 | ScreamTracker 3.01 |
| 0x1303 | ScreamTracker 3.03 |
| 0x1320 | ScreamTracker 3.20 |
| 0x2xyy | Imago Orpheus x.yy |
| 0x3xyy | Impulse Tracker x.yy |
| 0x4xyy | Schism Tracker (pre-2012); later uses timestamp encoding |
| 0x5xyy | OpenMPT x.yy |
| 0x6xyy | BeRoTracker x.yy |
| 0x7xyy | CreamTracker x.yy |
| 0xCA00 | Camoto / libgamemusic |

Many trackers write `0x1320` (disguising as ST3.20) — see [OpenMPT wiki](https://wiki.openmpt.org/Development:_Formats/S3M) for how to tell them apart.

## Timing

- `initialTempo` = rows rendered per second (BPM-like). At 50, each row = 0.02 s.
- `initialSpeed` = ticks (audio frames) per row.
- Rows per second = `initialTempo / initialSpeed`.
- Both can be changed during playback with effects `A` (speed) and `T` (tempo).

**Validation rules (confirmed by st3play `dig.c`):**
- `initialSpeed == 0` → ignored, use default (6).
- `initialSpeed == 255` → ignored, use default (6). ST3 stored 255 as a sentinel meaning "keep the previous session's speed".
- `initialTempo == 0` → ignored, use default (125). Computing tick length with tempo=0 would divide by zero.
- `initialTempo < 33` → valid for loading (no clamp on load); the Txx effect guard (`< 33` → ignore) applies only at runtime.

## Instrument block

Each instrument parapointer points to a block starting with a common header:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 1 byte | `type` | `0`=empty, `1`=PCM, `2`–`7`=OPL types |
| 1 | 12 bytes | `filename` | Original DOS 8.3 filename, not null-terminated |

### PCM instrument (type = 1), following common header:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 13 | 1 byte | `ptrDataH` | High byte of sample data parapointer |
| 14 | 2 bytes | `ptrDataL` | Low 16 bits of sample data parapointer |
| 16 | 4 bytes | `length` | Sample length in bytes (max 64000; upper 16 bits ignored) |
| 20 | 4 bytes | `loopStart` | Loop start byte offset from sample start |
| 24 | 4 bytes | `loopEnd` | Loop end byte offset (exclusive) |
| 28 | 1 byte | `volume` | Default volume 0–63 |
| 29 | 1 byte | reserved | `0x00` |
| 30 | 1 byte | `pack` | `0`=unpacked PCM; `1`=DP30ADPCM (deprecated, rarely seen) |
| 31 | 1 byte | `flags` | Bit 0: loop; Bit 1: stereo; Bit 2: 16-bit little-endian |
| 32 | 4 bytes | `c2spd` | Sample rate for middle-C (C-4); default 8363 Hz |
| 36 | 12 bytes | internal | Zero on disk; used as working state during playback |
| 48 | 28 bytes | `title` | Instrument title, null-terminated |
| 76 | 4 bytes | `sig` | `"SCRS"` |

Sample data is stored separately, typically after all pattern data. `sampleType == 2` (unsigned) is the standard; signed (`sampleType == 1`) is deprecated.

### OPL instrument (type = 2–7), following common header:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 13 | 3 bytes | reserved | `0x00 0x00 0x00` |
| 16 | 12 bytes | `oplValues` | OPL register values (see below) |
| 28 | 1 byte | `volume` | Default volume 0–63 |
| 29 | 1 byte | `dsk` | Unknown |
| 30 | 2 bytes | reserved | `0x0000` |
| 32 | 4 bytes | `c2spd` | Tuning (same semantics as PCM; default 8363) |
| 36 | 12 bytes | unused | Zero |
| 48 | 28 bytes | `title` | Instrument title, null-terminated |
| 76 | 4 bytes | `sig` | `"SCRI"` |

The `oplValues` 12-byte array maps to OPL chip registers for modulator and carrier operators:

| Byte | Operator | OPL base register |
|------|----------|-------------------|
| 0/1 | Mod/Car | 0x20 — AM, Vib, Sustain, KSR, Freq.mult |
| 2/3 | Mod/Car | 0x40 — Scale level, Output level |
| 4/5 | Mod/Car | 0x60 — Attack, Decay rates |
| 6/7 | Mod/Car | 0x80 — Sustain, Release rates |
| 8/9 | Mod/Car | 0xE0 — Wave select |
| 10 | Modulator | 0xC0 — Feedback, Connection |
| 11 | — | Unused, set to 0x00 |

Note: Scale Level bits in bytes 2–3 are swapped vs. what the ST3 TECH.DOC implies — see ModdingWiki for the corrected layout.

## Pattern block

At the offset given by the pattern parapointer:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 2 bytes | `packed_len` | Total packed data length in bytes (including this field) |
| 2 | `packed_len - 2` bytes | `packed_data` | Run-length encoded row data |

Each pattern always has exactly **64 rows**. The packed data is a series of variable-length records:

### Packed row data

Each record starts with a `what` byte:

| Bits | Field | Description |
|------|-------|-------------|
| 7 | command_present | If set, `command` and `info` bytes follow |
| 6 | volume_present | If set, `volume` byte follows |
| 5 | note_present | If set, `note` and `instrument` bytes follow |
| 4–0 | channel | Channel number (0–31) |

If `what == 0x00`, the row ends. After the 64th row, the pattern ends.

| Byte | Field | Condition | Description |
|------|-------|-----------|-------------|
| — | `what` | always | Bitfield described above |
| +1 | `note` | `what & 0x20` | Note number; `0xFE` = no note; `0xFF` = note off |
| +2 | `instrument` | `what & 0x20` | Instrument number (1-based) |
| +3 | `volume` | `what & 0x40` | Volume 0–64 (64 = max); `255` = no volume |
| +4 | `command` | `what & 0x80` | Effect letter (1=A, 2=B, … 22=V); `0` = none |
| +5 | `info` | `what & 0x80` | Effect parameter byte |

### Note encoding

The `note` byte packs octave and semitone:
- High nibble (bits 7–4): octave (0–7)
- Low nibble (bits 3–0): semitone (0=C, 1=C#, 2=D, …, 11=B)

Value `0xFE` means "no new note but keep playing"; `0xFF` means note-off / key-off.

## Detection

```c
/* Read 4 bytes at offset 0x002C */
if (buf[0x2C] == 'S' && buf[0x2D] == 'C' && buf[0x2E] == 'R' && buf[0x2F] == 'M')
    /* valid S3M file */
```
