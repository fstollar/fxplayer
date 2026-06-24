# 669 Format — History and Context

## Composer 669 (vanilla .669)

**Author:** Tran (Tomasz Pytel) of Renaissance
**Released:** 1992 (versions 1.1, 1.2 as shareware; v1.3 final as freeware)
**Platform:** MS-DOS

Composer 669 was the **first 8-channel digital audio tracker for the IBM PC** and the first to support samples larger than 64 KByte. Earlier PC trackers (like Fast Tracker 1) were limited to 4 channels and smaller samples. The format is named after the number of the program's first release or the hex magic bytes — the exact origin of "669" is not documented definitively.

### Technical constraints (Composer 669)

- 8 channels, fixed
- Up to 64 instruments, 128 patterns, 64 rows per pattern
- Sample rate output: 22 kHz
- Instruments stored as `.VOC` files (Creative Voice), loadable/saveable from the tracker
- Display: VGA 80×50 text mode
- Hardware: 386+, 2 MB RAM, VGA, SoundBlaster (mono) or SoundBlaster Pro (stereo)
- OS: DOS 3.0+

### Accuracy note

Composer 669 itself (v1.3) is the reference for maximum format accuracy in playback.
OpenMPT can play .669 files but may sound different — it is not the authoritative player.

## Unis 669 / Extended 669

**Author:** Jason Nunn
**Released:** 1994
**Magic bytes:** `JN` (Jason Nunn's initials)
**Platform:** MS-DOS

Unis 669 improved on Composer 669 by:
- Raising the sample rate cap from 22 kHz to 44 kHz
- Adding three new effects: super fast tempo (`F0x`), stereo balance slide (`Gnx`), slot retrigger (`Hxx`)
- Keeping full backward compatibility with vanilla .669 files (same extension, different magic)

See [EFFECTS.md](EFFECTS.md) for the extended effect commands.

## Demoscene context

Both trackers were used in the early-1990s PC demoscene. The 669 format competed with MOD (Amiga-origin) and preceded S3M (Scream Tracker 3, 1994). By the mid-1990s S3M and later XM/IT had largely superseded 669, but the format remains historically significant as the first native PC 8-channel tracker format.

Module archives with .669 files can be found at [The Mod Archive](https://modarchive.org/) and [Modland](ftp://ftp.modland.com/).

## FX Player and 669

FX Player (Apollo / STIGMA, 1996–1998) includes native 669 playback. The format was added alongside MOD and S3M because all three were common in the demoscene in that era.

Key implementation facts (see CLAUDE.md for the authoritative list):
- 669 samples are unsigned 8-bit PCM; they are XOR'd with 0x80 on load to convert to signed before mixing. MOD samples are natively signed — this XOR step is 669-specific.
- The deferred-jump pattern (`fx_order_jump` → `goRowOrder`) is used by 669 just as by MOD and S3M; all three must stay symmetric.

## Tools

| Tool | Author | Year | Notes |
|------|--------|------|-------|
| Composer 669 v1.3 | Tran / Renaissance | 1992 | Original tracker; freeware; reference player |
| Unis 669 | Jason Nunn | 1994 | Extended format; adds effects and 44 kHz support |
| OpenMPT | various | ongoing | Can read .669; not bit-accurate vs. Composer 669 |
| foobar2000 | Peter Pawlowski | ongoing | Playback via plugin |
| Droidsound-E | various | ongoing | Android playback |

## References

- [pouet.net — Composer 669 NFO](https://www.pouet.net/prod_nfo.php?which=63357&font=3) — original release text
- [pouet.net — Multitracker Module Editor by Renaissance](https://www.pouet.net/prod.php?which=13362)
- [BotB — Composer 669](https://battleofthebits.com/lyceum/View/Composer+669)
- [BotB — Unis 669](https://battleofthebits.com/lyceum/View/Unis+669)
- [NostalgicPlayer — Unis 669 sample modules](https://nostalgicplayer.dk/modules/format/unis669/1)
- [Modland format documentation](ftp://ftp.modland.com/pub/documents/format_documentation/Composer%20669,%20Unis%20669%20(.669).txt)
- [MultimediaWiki — Composer 669](https://wiki.multimedia.cx/index.php/Composer_669) (stub only)
