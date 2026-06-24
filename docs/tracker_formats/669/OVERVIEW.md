# 669 Tracker Format — Overview

The `.669` format is an 8-channel DOS tracker module format with two variants:

| Variant | Magic bytes | Creator | Year |
|---------|-------------|---------|------|
| 669 (vanilla) | `if` (0x69 0x66) | Tran / Tomasz Pytel, Renaissance | 1992 |
| Extended 669 | `JN` (0x4A 0x4E) | Jason Nunn (Unis 669) | 1994 |

Both variants share the same `.669` file extension. Players detect the variant by reading the first two bytes.

## Key constraints

- **Byte order: Little-endian** (DOS/x86)
- Up to **8 channels** (fixed)
- Up to **64 instruments** per song
- Up to **128 patterns**, each exactly **64 rows**
- Samples: unsigned 8-bit PCM (must be XOR'd 0x80 before use — see [FILE-FORMAT.md](FILE-FORMAT.md))
- Sample rate: ≤ 22 kHz (vanilla) / ≤ 44 kHz (extended)
- Max sample size: 1 MByte per instrument
- Max sample memory: 1408 KBytes total

## Documents in this directory

- [FILE-FORMAT.md](FILE-FORMAT.md) — Binary layout: header, sample descriptors, pattern data, note encoding
- [EFFECTS.md](EFFECTS.md) — Effect commands for vanilla 669 and Extended 669 (Unis 669)
- [HISTORY.md](HISTORY.md) — Historical context, trackers, demoscene background

## Relationship to other formats

669 predates S3M and MOD on the PC (Composer 669 was the **first 8-channel tracker for IBM PC**).
The effect numbering partially maps to Protracker MOD convention (see [EFFECTS.md](EFFECTS.md)).

## External references

- [fileformat.info — 669 format (Corion mirror)](https://www.fileformat.info/format/669/corion.htm) — original binary layout documentation
- [ftp.modland.com — official Composer/Unis 669 format spec](ftp://ftp.modland.com/pub/documents/format_documentation/Composer%20669,%20Unis%20669%20(.669).txt) — authoritative text from the authors
- [Battle of the Bits — 669 Format](https://battleofthebits.com/lyceum/View/669+Format) — effects reference, community notes
- [Battle of the Bits — Extended 669 Format](https://battleofthebits.com/lyceum/View/Extended+669+Format) — extra effects in Unis 669
- [NostalgicPlayer — Unis 669](https://nostalgicplayer.dk/modules/format/unis669/1) — sample modules, player notes
- [pouët.net — Composer 669 NFO](https://www.pouet.net/prod_nfo.php?which=63357&font=3) — original release NFO by Renaissance
- [Wikipedia — Module file](https://en.wikipedia.org/wiki/Module_file) — broader tracker format family context
- [Demozoo — Composer 669 by Renaissance](https://demozoo.org/productions/111803/) — production entry with release metadata
- [Internet Archive — Composer 669 v1.3](https://archive.org/details/compsd13_zip) — downloadable original binary
- [MultimediaWiki — Composer 669](https://wiki.multimedia.cx/index.php/Composer_669) — stub (extension only, no structural info)
