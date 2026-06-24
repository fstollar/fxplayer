# 669 Effect Commands

Effects are encoded in the low byte of each note cell (nibble `c` = command, nibble `d` = value).
The command encoding partially maps to Protracker MOD notation but with a different numbering:

| 669 command `c` nibble | MOD letter equivalent |
|------------------------|----------------------|
| 0 | a |
| 1 | b |
| 2 | c |
| 3 | d |
| 4 | e |
| 5 | f |

## Vanilla 669 effects (Composer 669)

| Command | Notation | Description |
|---------|----------|-------------|
| 0 | `Axx` | Portamento up — raise pitch by `xx` each tick |
| 1 | `Bxx` | Portamento down — lower pitch by `xx` each tick |
| 2 | `Cxx` | Portamento to note — slide toward trigger note at rate `xx` |
| 3 | `Dxx` | Frequency adjust — shift playback frequency by `xx` |
| 4 | `Exx` | Frequency vibrato — oscillate pitch at depth/speed `xx` |
| 5 | `Fxx` | Set tempo — `xx` = new tempo value |

**Effect cancellation rule:** Setting any of the above effects (`A`–`F`) with a value of `00` cancels all active effects in that row and notes that follow play at their natural pitch.

## Extended 669 additional effects (Unis 669, 1994)

The Extended 669 format (`JN` magic) adds three more commands:

| Command | Notation | Description |
|---------|----------|-------------|
| 5 | `F0x` | Super fast tempo — subcommand `x` selects an ultra-high tempo mode not achievable with plain `Fxx` |
| 6 | `Gnx` | Balance fine slide — `n` = direction (0 = slide left, 1 = slide right), `x` = slide amount |
| 7 | `Hxx` | Slot retrigger — retrigger the current note `xx` times per row |

Notes:
- `F0x` uses the same command slot as `F` but the `0` subcommand selects the super-fast mode; vanilla `Fxx` with `x > 0` is the normal tempo command.
- `G` (balance slide) is the only stereo effect in the 669 family; Composer 669 itself only supported mono output (SoundBlaster) or stereo (SoundBlaster Pro) but had no per-channel panning control.

## Notes on FX Player implementation

In `src/core/mod669.c` the effect dispatch follows the 0–5 numeric mapping from the file format, not the A–F letters shown above. The letters are the display convention used in Composer 669's UI and in most documentation.

Effect `E` (frequency vibrato) uses the same fixed sine table as the S3M vibrato but with a different depth scaling — do not share the S3M vibrato path without adjusting the depth factor.

## References

- [BotB — 669 Format](https://battleofthebits.com/lyceum/View/669+Format)
- [BotB — Extended 669 Format](https://battleofthebits.com/lyceum/View/Extended+669+Format)
- [Modland format spec](ftp://ftp.modland.com/pub/documents/format_documentation/Composer%20669,%20Unis%20669%20(.669).txt)
