# MOD — ProTracker/SoundTracker Module Format Overview

MOD is the original tracker module format, born on the Commodore Amiga in 1987. It has the longest history and the most sub-variants of any tracker format. All variants share the `.mod` extension; the variant is distinguished by a 4-byte tag at offset 0x0438 (or its absence).

## Format family tree

| Variant | Magic tag | Channels | Samples | Year | Tracker |
|---------|-----------|----------|---------|------|---------|
| SoundTracker (original) | *(no tag)* | 4 | 15 | 1987 | Ultimate SoundTracker (Karsten Obarski) |
| NoiseTracker / early PT | *(no tag)* | 4 | 31 | 1989 | NoiseTracker (Mahoney & Kaktus) |
| ProTracker standard | `M.K.` | 4 | 31 | 1990 | ProTracker (≤64 patterns) |
| ProTracker extended | `M!K!` | 4 | 31 | ~1992 | ProTracker (>64 patterns) |
| ProTracker rare | `M&K!` | 4 | 31 | ? | Unknown ("fleg's module train-er"); only one known file |
| StarTrekker 4ch | `FLT4` | 4 | 31 | ~1990 | StarTrekker |
| StarTrekker 8ch | `FLT8` | 8 | 31 | ~1990 | StarTrekker (interleaved pattern pairs) |
| FastTracker 2ch | `2CHN` | 2 | 31 | ~1992 | FastTracker (PC) |
| FastTracker 6ch | `6CHN` | 6 | 31 | ~1992 | FastTracker (PC) |
| FastTracker 8ch | `8CHN` | 8 | 31 | ~1992 | FastTracker (PC) |
| FastTracker 10–32ch | `xxCH` | 10–32 | 31 | ~1992 | FastTracker (even channels only) |
| TakeTracker odd-ch | `xxCN` | varies | 31 | ~1993 | TakeTracker |
| TakeTracker 1/2/3ch | `TDZ1`/`TDZ2`/`TDZ3` | 1/2/3 | 31 | ~1993 | TakeTracker |
| TakeTracker 5/7/9ch | `5CHN`/`7CHN`/`9CHN` | 5/7/9 | 31 | ~1993 | TakeTracker |
| Oktalyzer 8ch | `CD81`/`OKTA` | 8 | 31 | ~1990 | Oktalyzer (Atari ST) |
| OctaMED 8ch | `OCTA` | 8 | 31 | ~1991 | OctaMED |

## Key properties (all variants)

| Property | Value |
|----------|-------|
| Byte order | **Big-endian** (Amiga convention) |
| Sample encoding | Signed 8-bit PCM |
| Rows per pattern | Always 64 |
| Max patterns | 64 (M.K.) / 128 (M!K! and others) |
| Max order positions | 128 |
| Max sample size | 65535 words = 131070 bytes |
| Volume range | 0–64 |
| Pitch encoding | Period (Amiga DMA clock divisor), not frequency |

## Identifying 15 vs 31 sample modules

Seek to offset 0x0438 and check whether the 4 bytes are printable ASCII (0x20–0x7E). If not, it is a 15-sample module with no tag. If the bytes are a known tag from the table above, it is 31-sample. This is more reliable than other heuristics.

## Amiga hardware basis

The MOD format maps directly to the Amiga's **Paula** sound chip:
- 4 DMA channels, hardware stereo: channels 1 and 4 left; channels 2 and 3 right
- 8-bit unsigned PCM output (converted to signed by the DMA hardware)
- Clock: PAL = 7,093,789.2 Hz; NTSC = 7,159,090.5 Hz
- Sample rate for note = clock / (2 × period)
- At C-2 (period 428, PAL): ≈ 8287 samples/sec

## Documents in this directory

- [FILE-FORMAT.md](FILE-FORMAT.md) — Binary layout: header, 15 vs 31 sample structure, sample descriptors, pattern cell encoding, period table
- [EFFECTS.md](EFFECTS.md) — All standard effects 0–F and the full Exx sub-effect table, with ProTracker quirks
- [VARIANTS.md](VARIANTS.md) — All known magic tags, tracker lineage, PC extensions, platform variants
- [HISTORY.md](HISTORY.md) — Historical context from SoundTracker 1987 through ProTracker 3.x and the PC variants

## External references

### English
- [aes.id.au — Noisetracker/Soundtracker/Protracker Module Format](https://aes.id.au/modformat.html) — classic community spec; the reference most implementations are built from
- [felloff.net mirror of aes.id.au spec](https://felloff.net/doc/comp/spec/modformat.html) — alternate mirror of same document
- [greg-kennedy.com — Protracker Module Format](https://greg-kennedy.com/tracker/modformat.html) — another annotated copy
- [MultimediaWiki — Protracker Module](https://wiki.multimedia.cx/index.php/Protracker_Module) — all magic tags, effects with quirks, playback notes
- [OpenMPT Wiki — Module formats](https://wiki.openmpt.org/Manual:_Module_formats) — broad format family overview including MOD
- [Wikipedia — Module file](https://en.wikipedia.org/wiki/Module_file) — format family history and sub-format descriptions
- [Wikipedia — Protracker](https://en.wikipedia.org/wiki/Protracker) — tracker history, versions, authors
- [ExoticA — Protracker](https://www.exotica.org.uk/wiki/Protracker) — Amiga demoscene context
- [Amiga Music Preservation (AMP)](https://amp.dascene.net/) — largest Amiga module and composer database (177,854+ modules, 19,068+ composers)
- [The Mod Archive](https://modarchive.org/) — active MOD download archive
- [GitHub — tracker-history (cmatsuoka)](https://github.com/cmatsuoka/tracker-history) — historical format docs including Ultimate SoundTracker original format

### German
- [amiga.retroprojects.de — ProTracker Introduction (EN)](https://amiga.retroprojects.de/en/topics/protracker-introduction-en) — German Amiga site; intro article in English; ProTracker 2.3D/E/F, 2-clone noted

### Non-English coverage
No non-English technical documentation for the MOD binary format was found — similar to 669 and S3M, all deep technical documentation is in English.
