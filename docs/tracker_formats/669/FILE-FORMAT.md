# 669 File Format — Binary Layout

Source: [fileformat.info (Corion mirror)](https://www.fileformat.info/format/669/corion.htm)
and the [authoritative Modland spec](ftp://ftp.modland.com/pub/documents/format_documentation/Composer%20669,%20Unis%20669%20(.669).txt).

## Header (0x01F1 bytes)

| Offset | Size | Description |
|--------|------|-------------|
| 0x0000 | 2 bytes | Magic: `if` (0x69 0x66) for vanilla 669; `JN` (0x4A 0x4E) for Extended 669 |
| 0x0002 | 108 bytes | ASCII song message (not null-terminated) |
| 0x006E | 1 byte | NOS — number of samples (0–0x40 = 0–64) |
| 0x006F | 1 byte | NOP — number of patterns (0–0x80 = 0–128) |
| 0x0070 | 1 byte | Loop order number (order to restart at when song ends) |
| 0x0071 | 128 bytes | Order list — sequence of pattern indices |
| 0x00F1 | 128 bytes | Tempo list — initial tempo for each pattern slot |
| 0x0171 | 128 bytes | Break location list — last active row (0–63) for each pattern slot |

Order list, tempo list, and break location list are all 128 entries even if NOP is less than 128; unused slots are ignored.

## Sample descriptors (0x01F1 + NOS × 25 bytes)

Immediately after the fixed header, NOS sample descriptors follow, each 25 bytes:

| Offset within record | Size | Description |
|----------------------|------|-------------|
| 0 | 13 bytes | ASCIIZ sample filename (null-terminated, used as label) |
| 13 | 4 bytes | Sample length in bytes |
| 17 | 4 bytes | Loop start offset (relative to sample start) |
| 21 | 4 bytes | Loop end offset (exclusive; loop disabled if start == end) |

## Pattern data (NOP × 0x600 bytes)

After the sample descriptors, NOP patterns follow. Each pattern is exactly **0x600 = 1536 bytes**:

```
64 rows × 8 channels × 3 bytes per note = 1536 bytes
```

Row order is row-major: all 8 channel notes for row 0, then all 8 for row 1, etc.

### Note encoding (3 bytes)

```
Byte[0]   Byte[1]   Byte[2]
nnnnnnii  iiiivvvv  ccccdddd
```

| Field | Bits | Description |
|-------|------|-------------|
| `n` | 6 bits (Byte[0] bits 7–2) | Note value (0 = C-0 … see note table below) |
| `i` | 6 bits (Byte[0] bits 1–0, Byte[1] bits 7–4) | Instrument number (1-based) |
| `v` | 4 bits (Byte[1] bits 3–0) | Volume (0–15, where 15 = full) |
| `c` | 4 bits (Byte[2] bits 7–4) | Effect command (see [EFFECTS.md](EFFECTS.md)) |
| `d` | 4 bits (Byte[2] bits 3–0) | Effect value / data |

### Special note values

| Byte[0] | Byte[2] | Meaning |
|---------|---------|---------|
| `0xFE` | any | No new note; apply volume change only |
| `0xFF` | `0xFF` | Empty cell — no note, no command |
| `0xFF` | other | No new note; apply command only |

### Note value table

Note values are semitone offsets starting at C in octave 0. The exact frequency mapping follows Composer 669's internal table (identical to the lookup used in FX Player's `src/core/mod669.c`).

## Sample data

Immediately after all pattern data, raw sample audio follows — NOS samples concatenated in the order they appear in the sample descriptors.

- **Encoding:** unsigned 8-bit PCM (values 0–255, silence = 128)
- **FX Player note:** samples are XOR'd with 0x80 on load to convert to signed 8-bit before the mixer sees them. This is format-specific — MOD samples are natively signed; only 669 requires the XOR step.
- **Sample rate:** ≤ 22050 Hz (vanilla 669) / ≤ 44100 Hz (Extended 669)

## Variant detection

Read the first two bytes to select the parser:

```c
if (buf[0] == 'i' && buf[1] == 'f')      /* vanilla Composer 669 */
else if (buf[0] == 'J' && buf[1] == 'N') /* Extended 669 / Unis 669 */
```

Both variants share the same layout above; the Extended variant adds new effect commands (see [EFFECTS.md](EFFECTS.md)).

## MultimediaWiki

The MultimediaWiki page [Composer 669](https://wiki.multimedia.cx/index.php/Composer_669) exists but contains only the file extension stub (as of 2007). No structural information is documented there.
