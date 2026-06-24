# MOD Format — History and Context

## Tracker lineage

### Amiga era

| Tracker | Author(s) | Year | Key additions |
|---------|-----------|------|---------------|
| Ultimate SoundTracker 1.0 | Karsten Obarski | 1987 | First tracker; 4 ch, 15 samples, no loop, basic effects |
| SoundTracker 2.0+ | Various clones | 1987–1988 | Sample looping, more effects, 15 samples |
| NoiseTracker 1.0 | Mahoney & Kaktus | 1989 | 31 samples (format extended), pattern loop `E6x` |
| ProTracker 1.0 | Lars Hamre, Anders Hamre, Sven Vahsen, Rune Johnsrud | 1990 | `M.K.` tag, standard effect set, finetune |
| ProTracker 2.3D | above team | ~1992 | Classic "standard" version; freeware |
| ProTracker 3.62 | above team | 1996 | Final official release |
| ProTracker 4.0 Beta 2 | above team | 1997 | Never officially released |
| ProTracker 2.3E/F | Olivier "8bitbubsy" Sørensen | 2010s–ongoing | Community fork; bug fixes, modern hardware |
| ProTracker 2 Clone | Olivier Sørensen | 2010s–ongoing | Cycle-accurate PC port |
| StarTrekker | — | ~1990 | Added `FLT4`/`FLT8` 8-channel variant |
| OctaMED | Teijo Kinnunen | ~1991 | 8-channel (`OCTA`), later MIDI, eventually 128 channels |
| Oktalyzer | — (Atari ST) | ~1990 | 8-channel on Atari ST (`CD81`/`OKTA` tags) |

### PC era

| Tracker | Author(s) | Year | Notes |
|---------|-----------|------|-------|
| FastTracker (1) | Triton | ~1992 | First major PC MOD tracker; `6CHN`/`8CHN` tags |
| MultiTracker | **Renaissance** | ~1993 | Up to 32 channels on PC (`.MTM` format) |
| FastTracker 2 | Triton | 1994 | New XM format; still reads MOD |
| ScreamTracker 3 | Future Crew | 1994 | S3M format; can import MOD |
| ModPlug Tracker | Olivier Lapicque | 1997 | Windows; reads all variants |
| OpenMPT | community | 2004–ongoing | Successor to ModPlug; reference MOD player |
| Schism Tracker | — | 2003–ongoing | Cross-platform; IT focus but reads MOD |

Note: **MultiTracker was written by Renaissance** (Tran/Tomasz Pytel's group) — a different group from STIGMA (Apollo/Frank Stollar), the author of FX Player. The MTM format is not `.mod`.

## Amiga hardware background

The MOD format maps directly to the Commodore Amiga's **Paula** custom chip:
- **4 DMA channels** — dedicated per-channel sample playback in hardware with no CPU involvement during output
- **Fixed stereo:** channels 1 and 4 left; channels 2 and 3 right — cannot be changed on stock A500/A1200 without OS tricks or hardware mods
- **8-bit PCM** — Paula's DAC outputs 8-bit resolution
- **Clock-divided pitch** — pitch is set by dividing the Amiga's clock by 2 × period: `rate = clock / (2 × period)`. PAL clock = 7,093,789.2 Hz; NTSC = 7,159,090.5 Hz

Paula's simplicity made MOD files extremely portable — any Amiga could play them at full quality with zero CPU load.

The Amiga's hardware period limits (113–856) are why octaves 0 and 4 in the period table are "non-standard": they exceed what real Paula can reproduce without aliasing or timing errors.

## 15 → 31 sample transition

The original SoundTracker supported only 15 samples. Mahoney & Kaktus (NoiseTracker) extended the format to 31 samples and added the `M.K.` tag to distinguish the new format. The tag's initials are commonly (and wrongly) attributed to Mahoney & Kaktus — the aes.id.au spec credits them to Unknown/D.O.C., who actually made the change.

## The "M.K." origin

The 4-byte tag `M.K.` at offset 0x0438 is the initials of **Unknown** and **D.O.C.** (two sceners), who modified the SoundTracker format to support 31 samples. They are not Mahoney & Kaktus despite the persistent myth. This is documented in the aes.id.au format specification.

## Demoscene and cultural impact

MOD became the dominant music format of the Amiga demoscene (1988–1995) and spread to PC via FastTracker. It was used in thousands of commercial Amiga games and PC demoscene productions. The format's self-contained nature (samples and patterns in one file) made it ideal for sharing on BBSs and floppy disks.

Archive resources:
- **Amiga Music Preservation (AMP):** 177,854+ modules, 19,068+ composers archived — the largest Amiga music database in existence
- **The Mod Archive:** Active download archive with search and metadata
- **Modland FTP:** Historical document archive including the original SoundTracker format spec

## FX Player and MOD

FX Player (Apollo / STIGMA, 1996–1998) supports 4-channel and 8-channel MOD. The format was included as it was the most widespread format in the Amiga demoscene, which was the cultural context FX Player was built for.

Key implementation notes (see CLAUDE.md for authoritative list):
- MOD samples are **natively signed 8-bit PCM** — no XOR conversion on load (unlike 669)
- The deferred-jump pattern (`fx_order_jump` → `goRowOrder`) applies here as in 669/S3M
- Effect Dxy uses decimal notation (row = x×10 + y), same pattern as S3M Cxy

## Non-English language coverage

- **German:** [amiga.retroprojects.de](https://amiga.retroprojects.de/en/topics/protracker-introduction-en) is a German Amiga site with an English-language ProTracker introduction. No German-language binary format documentation found.
- **Finnish/Nordic:** No Finnish, Swedish, Danish, or Norwegian MOD format documentation found. The Amiga demoscene was strong in these countries (many notable groups: Triton (Swedish), Future Crew (Finnish)) but technical format docs were published in English.
- **Polish:** No Polish-language MOD format documentation found. Polish demoscene was active but documented in English.

## References

- [Wikipedia — Module file (format family overview)](https://en.wikipedia.org/wiki/Module_file)
- [Wikipedia — Protracker](https://en.wikipedia.org/wiki/Protracker)
- [ExoticA — Protracker](https://www.exotica.org.uk/wiki/Protracker)
- [Amiga Music Preservation (AMP)](https://amp.dascene.net/)
- [amiga.retroprojects.de — ProTracker Introduction](https://amiga.retroprojects.de/en/topics/protracker-introduction-en)
- [ProTracker 2 Clone (16-bits.org)](https://16-bits.org/pt2-clone/)
- [aes.id.au — format spec including M.K. origin](https://aes.id.au/modformat.html)
- [GitHub — tracker-history (cmatsuoka): original SoundTracker format docs](https://github.com/cmatsuoka/tracker-history/blob/master/reference/amiga/soundtracker/Ultimate_Soundtracker-format.txt)
