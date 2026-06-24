# S3M Format — History and Context

## Scream Tracker lineage

| Version | Format | Year | Notes |
|---------|--------|------|-------|
| Scream Tracker 1.0 | (none) | ~1990 | 4-bit PC speaker / Covox / SoundBlaster 1.x output |
| Scream Tracker 2.2 | STM | 1990 | First popular version; 4 channels, limited effects |
| Scream Tracker 3.00 | S3M | 1994 | First S3M release; 32 channels, OPL support |
| Scream Tracker 3.20 | S3M | 1994 | Most common version; widely imitated tracker ID |
| Scream Tracker 3.21 | S3M | 1994 | Final release; last version distributed by Future Crew |

## Authors and group

**Author:** Sami Tammilehto (handle: Psi)  
**Group:** Future Crew — a Finnish demoscene group, one of the most influential of the 1990s PC demoscene.  
**Language:** Written in C and x86 assembly.

The format is Finnish in origin — Future Crew (and Scream Tracker) were based in Finland. This is why the Finnish Wikipedia article notes that ST3 "popularized more-than-16-channel tracking on the PC" (yleisti yli 16-kanavaisen tracker-musiikin PC:n puolella).

## What made S3M significant

1. **First widely-used tracker to support 32 channels on PC.** MOD was limited to 4–8; Composer 669 to 8.
2. **Mixed PCM + OPL synthesis simultaneously.** No other tracker of the era supported both at once.
3. **BPM-based timing** (`initialTempo` + `initialSpeed`) replaced the cruder period-based tempo of MOD.
4. **Influenced successors directly.** Impulse Tracker's UI is explicitly based on ST3's. FastTracker 2 competed with it. OpenMPT traces lineage through all of them.

## Hardware context

| Hardware | PCM channels | OPL channels | Panning |
|----------|-------------|--------------|---------|
| SoundBlaster 1.x | 8 (mono) | — | Fixed center |
| SoundBlaster Pro | 16 (stereo L/R fixed) | 9 | Fixed L/R per channel |
| Gravis Ultrasound | 16 | — | 16-position free panning (S8x effect) |
| Adlib / OPL-only | — | 9 (melody) + 5 (rhythm, unused in ST3) | Mono |

The `masterVolume` field in the header affects PCM channels only; OPL channels are unaffected by it.

## S3M in the demoscene

S3M was the dominant PC tracker format from 1994 until XM (FastTracker 2) and IT (Impulse Tracker) took over in 1995–1996. FX Player (Apollo / STIGMA, 1996–1998) included S3M playback as one of its three main formats alongside MOD and 669.

Game usage: Unreal (1998) shipped S3M music; ModPlug coined the `.s3z` extension for zlib-compressed S3M files.

## Compatible trackers and players

| Tool | Platform | Notes |
|------|----------|-------|
| Scream Tracker 3.21 | DOS | Reference implementation; freeware |
| Impulse Tracker | DOS | S3M compatible; some playback differences |
| FastTracker 2 | DOS | Can convert S3M to XM |
| OpenMPT | Windows | Full S3M support; extensive compatibility heuristics |
| Schism Tracker | Windows/Linux | Modern ST3 remake; uses MAME OPL emulator |
| libxmp | cross-platform | Open-source S3M playback library |
| ModPlug / foobar2000 | Windows | Playback plugin |

Many trackers disguise themselves as ST3.20 in the `trackerVersion` header field — see [OpenMPT wiki](https://wiki.openmpt.org/Development:_Formats/S3M) for how to detect the true creator.

## Non-English language coverage

- **German (DE):** [de.wikipedia.org/wiki/S3M](https://de.wikipedia.org/wiki/S3M) — brief but accurate; notes the STM→S3M successor relationship, 4→32 tracks, 31→99 instruments.
- **Finnish (FI):** [fi.wikipedia.org/wiki/Scream_Tracker](https://fi.wikipedia.org/wiki/Scream_Tracker) — the format's home country; brief article emphasizing ST3's role in popularizing multi-channel PC tracking.
- No Polish, Danish, Swedish, or other language documentation found.

## References

- [Wikipedia — Scream Tracker](https://en.wikipedia.org/wiki/Scream_Tracker)
- [Wikipedia (DE) — S3M](https://de.wikipedia.org/wiki/S3M)
- [Wikipedia (FI) — Scream Tracker](https://fi.wikipedia.org/wiki/Scream_Tracker)
- [pollak.thebe.de — The S3M Format](https://pollak.thebe.de/b/the-s3m-format/) — detailed narrative on format design, hardware context, and OPL
- [VGMPF Wiki — S3M](https://vgmpf.com/Wiki/index.php?title=S3M) — players, editors, game usage list
- [TECH.DOC (original ST3 documentation)](https://pollak.thebe.de/b/Modules/docs/TECH.txt) — distributed with ScreamTracker 3
