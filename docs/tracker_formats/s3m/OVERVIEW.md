# S3M — Scream Tracker 3 Module Format Overview

S3M is the native format of Scream Tracker 3, created by Sami Tammilehto (Psi) of the Finnish demogroup Future Crew. Final version 3.21 was released in 1994.

## Key facts

| Property | Value |
|----------|-------|
| Magic bytes | `SCRM` at offset 0x002C |
| Byte order | Little-endian (DOS/x86) |
| Max channels | 32 virtual (25 physical: 16 PCM + 9 OPL) |
| Max instruments | 99 |
| Max patterns | 65,536 (practical: 100 in ST3 UI) |
| Max order positions | 256 |
| PCM sample depth | 8-bit (signed or unsigned; see header flag) |
| PCM stereo/16-bit | Supported via instrument flags |
| OPL support | OPL2/OPL3 FM instruments (Adlib/SoundBlaster) |
| Pointer format | Parapointers (×16 bytes = byte offset) |
| Extension | `.s3m` (compressed: `.s3z`, coined by ModPlug) |

## What S3M added over MOD

- Up to 32 channels (vs. 4–8 in MOD)
- Up to 99 instruments (vs. 31)
- Octaves 0–7 (wider range than Amiga hardware limits)
- Volume column (per-note volume, separate from effect column)
- Per-channel stereo panning (GUS hardware; 16 positions)
- 16-bit and stereo PCM samples
- Higher sample rates
- FM/OPL2 synthesis channels alongside PCM channels
- Richer effect set (A–V plus Sxx sub-effects)
- Per-pattern `initialSpeed` / `initialTempo` (BPM-based timing)

## Format relationship

S3M succeeded STM (Scream Tracker 2 Module) and influenced IT (Impulse Tracker) and OpenMPT. Its effect naming convention (A–V letters) differs from ProTracker MOD but effects can be mapped back and forth between the two.

## Non-English sources found

- **German:** [de.wikipedia.org/wiki/S3M](https://de.wikipedia.org/wiki/S3M) — brief overview (Dateiformat, 4→32 Spuren, 31→99 Instrumente)
- **Finnish:** [fi.wikipedia.org/wiki/Scream_Tracker](https://fi.wikipedia.org/wiki/Scream_Tracker) — historical context; notes ST3 popularized >16-channel tracking on PC ("yleisti yli 16-kanavaisen tracker-musiikin PC:n puolella")

## Documents in this directory

- [FILE-FORMAT.md](FILE-FORMAT.md) — Binary layout: header, parapointers, PCM/Adlib instruments, packed pattern data
- [EFFECTS.md](EFFECTS.md) — All effect commands A–V and Sxx sub-effects with quirks
- [HISTORY.md](HISTORY.md) — Historical context, tracker lineage, hardware, demoscene background

## External references

- [ModdingWiki — S3M Format](https://moddingwiki.shikadi.net/wiki/S3M_Format) — header layout, instrument and pattern tables, timing
- [MultimediaWiki — Scream Tracker 3 Module](https://wiki.multimedia.cx/index.php?title=Scream_Tracker_3_Module) — most complete effects reference with all quirks
- [OpenMPT Wiki — Development:Formats/S3M](https://wiki.openmpt.org/Development:_Formats/S3M) — tracker ID table, compatibility detection, per-tracker quirks
- [pollak.thebe.de — The S3M Format](https://pollak.thebe.de/b/the-s3m-format/) — well-written narrative explanation of the format and OPL support
- [Kaitai Struct — s3m](https://formats.kaitai.io/s3m/) — machine-readable format spec
- [VGMPF Wiki — S3M](https://vgmpf.com/Wiki/index.php?title=S3M) — players, editors, converters, game usage
- [Wikipedia — Scream Tracker](https://en.wikipedia.org/wiki/Scream_Tracker) — history, versions, hardware support
- [Wikipedia — S3M](https://en.wikipedia.org/wiki/S3M) — format disambiguation
- [Wikipedia (DE) — S3M](https://de.wikipedia.org/wiki/S3M) — German language overview
- [Wikipedia (FI) — Scream Tracker](https://fi.wikipedia.org/wiki/Scream_Tracker) — Finnish language overview
- [TECH.DOC (original ST3 documentation)](https://pollak.thebe.de/b/Modules/docs/TECH.txt) — authoritative spec distributed with ScreamTracker 3
- [FS3MDOC.TXT — S3M Player Tutorial](http://schismtracker.org/wiki/FS3MDOC.TXT) — player implementation guide
