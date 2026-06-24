# MOD Effect Commands

Effects are encoded as nibble `C` (command 0x0–0xF) + byte `XX` (parameter, two nibbles `x` and `y`) in the 4-byte pattern cell. See [FILE-FORMAT.md](FILE-FORMAT.md) for cell encoding.

Timing: each row is divided into `ticks` (set by effect `F`). Default ticks/row = 6. "Non-zero tick" = all ticks after the first (ticks 1, 2, …). Pitch is encoded as Amiga period — lower period = higher pitch.

## Standard effects

| Cmd | Notation | Description |
|-----|----------|-------------|
| `0` | `0xy` | **Arpeggio.** Cycle through note, note+x semitones, note+y semitones, one step per tick (using tick % 3). Used to simulate chords. `000` = no effect. |
| `1` | `1xx` | **Slide up (pitch up).** Decrease period by `xx` on each non-zero tick. Cannot slide above period 113 (Amiga limit). |
| `2` | `2xx` | **Slide down (pitch down).** Increase period by `xx` on each non-zero tick. Cannot slide below period 856 (Amiga limit). |
| `3` | `3xx` | **Slide to note (portamento).** Move period toward target period by `xx` per non-zero tick; stop exactly at target. Any note in the cell sets the target but is not played. `300` = continue previous slide rate. |
| `4` | `4xy` | **Vibrato.** Oscillate period at speed `x`, depth `y/16` semitones. `x` controls how many 64ths of a cycle occur per tick; `y` is amplitude. Default waveform: sine. `4xy` with `xy=0` continues previous values. |
| `5` | `5xy` | **Portamento + volume slide.** Continue effect `3` (use `300`) while simultaneously sliding volume: `x > 0` → slide up by `x`; `y > 0` → slide down by `y`. Illegal to have both x and y non-zero. |
| `6` | `6xy` | **Vibrato + volume slide.** Continue effect `4` (use `400`) while simultaneously sliding volume. Same x/y rules as `5xy`. |
| `7` | `7xy` | **Tremolo.** Oscillate volume at speed `x`, depth `y`. Similar to vibrato but affects volume. Waveform set by `E7x`. |
| `8` | `8xx` | **Set panning (PC/DMP extension).** `00`=hard left, `40`=center, `80`=hard right, `A4`=surround. Unused in standard ProTracker. FastTracker uses 0x00–0xFF range. |
| `9` | `9xx` | **Set sample offset.** Start playing sample at offset `xx * 256` words from the beginning. If no new sample given but one is playing, retrigger it at the new offset. Old ProTracker (<3.15) doubled this offset — standard players do not. |
| `A` | `Axy` | **Volume slide.** Every non-zero tick: if `x > 0`, slide volume up by `x`; if `y > 0`, slide down by `y`. If both non-zero, `y` is ignored. `A00` = no effect (no memory). Clamp to 0–64. |
| `B` | `Bxx` | **Position jump.** After current row, jump to order position `xx` (decimal). Restarts song if `xx` exceeds length. |
| `C` | `Cxx` | **Set volume.** Set channel volume to `xx`. Values > 64 clamp to 64. |
| `D` | `Dxy` | **Pattern break.** After current row, advance to next pattern and start at row `x*10 + y` (decimal, **not hex**). This is a common implementation mistake. |
| `E` | `Exy` | **Extended effects** — see Exx table below. |
| `F` | `Fxx` | **Set speed / tempo.** If `xx == 0`, treated as `xx = 1`. If `xx ≤ 32` (0x20): set ticks-per-row. If `xx > 32`: set BPM (beats per minute). Default: 6 ticks/row, 125 BPM. |

## Exx extended effects

| Cmd | Notation | Description |
|-----|----------|-------------|
| `E0` | `E0x` | **Set Amiga filter.** `x=0` filter ON; `x=1` filter OFF. Hardware-specific 1-pole low-pass (≈11,500 Hz) on A500/A2000/A1200. Ignored on PC players. |
| `E1` | `E1x` | **Fine slide up.** Decrease period by `x` on tick 0 only (immediate, no sliding). No effect memory. Cannot go below period 113. |
| `E2` | `E2x` | **Fine slide down.** Increase period by `x` on tick 0 only. No effect memory. Cannot go above period 856. |
| `E3` | `E3x` | **Set glissando.** `x=1` ON; `x=0` OFF. When ON, effect `3` (portamento) slides in semitone steps instead of smoothly. |
| `E4` | `E4x` | **Set vibrato waveform.** Low 2 bits (x & 3): 0=sine, 1=ramp-down, 2=square, 3=random. Bit 2 (x & 4): if set, do not retrigger waveform on new note. |
| `E5` | `E5x` | **Set finetune.** Set the finetune value of the current sample to signed nibble `x` (0–15 → 0..7, −8..−1). Timing of when this takes effect is tracker-dependent. |
| `E6` | `E6x` | **Loop pattern.** `x=0`: set loop start to current row. `x>0`: jump back to loop start and repeat `x` more times. Loop point resets on Bxx, Dxx, or pattern transition. Loops do not nest. |
| `E7` | `E7x` | **Set tremolo waveform.** Same encoding as `E4x`, but affects tremolo (effect `7`). |
| `E8` | `E8x` | **Set stereo (rare extension).** `0`=hard left, `F`=hard right. Rarely implemented. |
| `E9` | `E9x` | **Retrigger sample.** Trigger current sample every `x` ticks. `x=0` → no retrigger. |
| `EA` | `EAx` | **Fine volume slide up.** Increase volume by `x` on tick 0 only. Clamp to 64. |
| `EB` | `EBx` | **Fine volume slide down.** Decrease volume by `x` on tick 0 only. Clamp to 0. |
| `EC` | `ECx` | **Note cut.** Set volume to 0 at tick `x`. If `x=0` you will not hear the note at all. |
| `ED` | `EDx` | **Note delay.** Do not start this row's note until tick `x`. `x=0` effectively still has a very small delay. |
| `EE` | `EEx` | **Row (pattern) delay.** After this row, wait the equivalent of `x` additional rows before advancing. Effects and notes continue during delay. |
| `EF` | `EFx` | **Funk repeat / Invert loop.** Periodically inverts (bitwise NOT) successive bytes in the current sample's loop. `x=0` resets counter. `x>0` sets speed from table. Only works if sample has a loop. Rare; mostly used in chiptunes. Overwrites sample data in memory. |

### EFx speed table (from ProTracker 1.2 source)

```
x:     0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
incr:  0   5   6   7   8  10  11  13  16  19  22  26  32  43  64 128
ticks: ∞  26  22  19  16  13  12  10   8   7   6   5   4   3   2   1
```

## Timing and speed

Default state: **6 ticks/row**, **125 BPM**.

- BPM controls how many rows per second: `rows/sec = BPM / (ticks/row * 2.5)`. At 125 BPM, 6 ticks: 125 / 15 = 8.333 rows/sec.
- BPM range accepted by `Fxx`: 33–255 (values ≤ 32 set ticks/row instead).
- Effect `F00` → implementation-defined; many players treat it as `F01` (1 tick/row) rather than stopping.

## ProTracker quirks worth noting

- **Effect 0 (arpeggio) with `000`:** Many ProTracker versions treat `000` as "no arpeggio" and continue playing the base note. Some treat it as arpeggio with 0-semitone shifts (audibly the same). Playing `000` after a previous arpeggio effect does not continue the arpeggio.
- **Effect A (volume slide) has no memory:** Unlike S3M's Dxx, `A00` does nothing (does not recall last value).
- **Effect D is decimal:** `D13` = break to row 13, not row 0x13 (19). This is the same BCD convention as S3M's `Cxy` and causes widespread bugs in re-implementations.
- **Effect 9 (sample offset) in old ProTracker:** Versions before approximately 3.15 doubled the offset, giving a range of 0–510 words × 512 rather than 0–255 × 256. Modern players do not apply this doubling.
- **Loop length 0:** Allegedly crashes real Amiga hardware. Use loop length = 1 to disable looping (length > 1 activates loop).
- **Loop length 1 = no loop:** Do not loop if `loop_length <= 1`. The aes.id.au spec says "loop if repeat length > 1."
- **Restart byte (offset 0x0197):** ProTracker writes `0x7F` here and ignores the field. NoiseTracker and FastTracker use it as the restart order position. SoundTracker wrote `0x78` (120). This byte is useful for rough tracker detection.

## Finetune period tables

The standard period table (finetune 0) is given in [FILE-FORMAT.md](FILE-FORMAT.md). The 16 finetune tables (one per value −8..+7) are precalculated — each step shifts pitch by 1/8 of a semitone. Player implementations typically store all 16 × 36 period values in a compile-time table.

## Notes on FX Player implementation

In `src/core/modmod.c` the cell encoding uses the formula:
```c
period    = ((cell[0] & 0x0F) << 8) | cell[1];
sample    = (cell[0] & 0xF0) | (cell[2] >> 4);
effect    = cell[2] & 0x0F;
param     = cell[3];
```
The Dxy pattern-break row is decimal (`x*10 + y`), consistent with the S3M Cxy BCD convention.  
Samples are signed 8-bit PCM — no XOR conversion needed (unlike 669).  
FX Player supports 4- and 8-channel MOD files; the 8-channel path uses the standard L/R channel alternation.

## References

- [aes.id.au — full effects reference](https://aes.id.au/modformat.html)
- [MultimediaWiki — effects with quirks](https://wiki.multimedia.cx/index.php/Protracker_Module#Effects)
