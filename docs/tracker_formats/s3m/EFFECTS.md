# S3M Effect Commands

Primary source: [MultimediaWiki — Scream Tracker 3 Module](https://wiki.multimedia.cx/index.php?title=Scream_Tracker_3_Module). Effect names are letters A–V plus Sxx sub-effects. The numeric encoding in the file is `command = letter_index` (A=1, B=2, …, V=22); `0` means no effect.

**Authoritative reference implementation:** [st3play by 8bitbubsy](https://github.com/8bitbubsy/st3play) — a direct C port of the ST3.21 ASM/C source. When documentation and behavior diverge, st3play is the ground truth.

Effects marked **(%)** use **the latest nonzero effect parameter** in that channel if the current parameter is 0 (i.e. they have "effect memory"). Effects marked **(\*)** have their own independent memory.

## Standard effects

| Letter | Code | Notation | Description |
|--------|------|----------|-------------|
| A | 0x01 | `Axx` | **Set speed.** Sets ticks-per-row. `xx=0` → effect ignored. |
| B | 0x02 | `Bxx` | **Order jump.** Jump to order `xx` after current row. |
| C | 0x03 | `Cxy` | **Jump to row.** `xy` is **decimal-as-hex** (BCD): row = `(x * 10) + y`. Jump to first row of next pattern in sequence at that row. Row ≥ 64 → ignored. |
| D | 0x04 | `Dxy` (%) | **Volume slide.** See detailed rules below. |
| E | 0x05 | `Exx` (%) | **Slide pitch down** by `xx` units per tick. |
| F | 0x06 | `Fxx` (%) | **Slide pitch up** by `xx` units per tick. |
| G | 0x07 | `Gxx` (*) | **Slide to note** (portamento). See quirks below. |
| H | 0x08 | `Hxy` (*) | **Vibrato.** Shares memory with `Uxy`. `x`=speed, `y`=depth. |
| I | 0x09 | `Ixy` (%) | **Tremor.** On for `x+1` ticks, off for `y+1` ticks. Counters never reset between rows; only reset at song start. |
| J | 0x0A | `Jxy` (%) | **Arpeggio.** |
| K | 0x0B | `Kxy` (%) | **H00 + Dxy** (vibrato continue + volume slide). First tick is a no-op; fine slides do not work. |
| L | 0x0C | `Lxy` (%) | **G00 + Dxy** (portamento continue + volume slide). Same no-op quirk as `K`. |
| O | 0x0F | `Oxx` (*) | **Set sample offset.** High byte of offset; full offset = `Oxx * 256`. |
| Q | 0x11 | `Qxy` (%) | **Retrigger note** every `y` ticks with volume modifier `x`. `y=0` → ignored. Processed every tick including tick 0. Counter not reset on new note with effect. |
| R | 0x12 | `Rxy` (%) | **Tremolo.** Speed = `x*4`, depth = `y*4`. Applied from tick 1 onward. Speed=1 → no effect. Depth does not affect notes at vol 0. |
| S | 0x13 | `Sxy` (%) | **Miscellaneous.** See Sxx sub-effects below. |
| T | 0x14 | `Txx` | **Set tempo (BPM).** `xx < 33` → ignored. |
| U | 0x15 | `Uxy` (*) | **Fine vibrato.** Shares memory with `Hxy`. |
| V | 0x16 | `Vxx` | **Set global volume.** `xx > 0x40` → ignored. Processed on tick 1 (not tick 0), so speed=1 means no effect. |

## Dxy (volume slide) — detailed rules

- `D0x` (1 ≤ x ≤ 0xE): slide down by x on all non-zero ticks (also tick 0 if fast slides enabled)
- `Dx0` (1 ≤ x ≤ 0xE): slide up by x on all non-zero ticks
- `DFx` (1 ≤ x ≤ 0xE): fine slide down — only on tick 0
- `DxF` (1 ≤ x ≤ 0xE): fine slide up — only on tick 0
- `DFF`: fine slide up by 15, tick 0 only
- `D0F`: slide down by 15 on **all** ticks, unaffected by fast slides flag
- `DF0`: slide up by 15 on **all** ticks, unaffected by fast slides flag
- `Dxy` (both nibbles non-zero and non-F): ST3 treats as `D0y` (slide down); Impulse Tracker does nothing
- `D00`: uses last nonzero parameter in channel (effect memory)
- Volume slide modifies the **active** (current) volume, not the stored volume

## Gxx quirks

- If channel has no current note, the destination is **the last note that appeared in the channel**, even without Gxx.
- Gxx **does not clear** the target note on arrival — any future `G00` keeps sliding back to that note.

## Sxx sub-effects

| Sub-effect | Description |
|------------|-------------|
| `S00` | Repeat previous Sxy (effect memory); if repeating a note delay (SDx), note triggers on tick 0 AND tick x |
| `S1x` | Set glissando control |
| `S2x` | Set finetune |
| `S3x` | Set vibrato waveform (bits 1–0: 0=sine, 1=ramp-down, 2=square, 3=random; bit 2: don't reset on new note; bit 3: ignored) |
| `S4x` | Set tremolo waveform (same bit encoding as S3x) |
| `S8x` | Set panning position (GUS hardware only in ST3; 16 positions) |
| `SAx` | Old stereo control (SoundBlaster only; SA0/SA2=normal, SA1/SA3=reversed, SA4–SA7=center, SA8–SAF=no-op). Mostly removed in ST3.01+; players convert to S8x |
| `SBx` | Pattern loop. Loop info is **global** (not per-channel) — different from ImpulseTracker. |
| `SCx` | Note cut after x ticks. `x=0` → ignored. Does not set volume to 0; **freezes** playback (may resume with E/F/G/H/J/K/L/U effects). |
| `SDx` | Note delay: trigger note on tick x instead of tick 0 |
| `SEx` | Pattern delay: extra x rows of delay before advancing |
| `SFx` | FunkRepeat — not implemented in ST3 |
| `S0x` | Set filter — not implemented in ST3; behaves as S00 (effect memory) |

## Effect memory (%) rule

When the parameter byte is 0x00 for an effect marked %, the effect uses the last nonzero parameter seen in that channel for that effect. This is independent per-channel and per-effect; `D00` recalls the last D parameter, not the last S parameter.

## Qxy volume modifier table

The `x` nibble of `Qxy` applies a volume modifier on each retrigger:

| x | Modifier |
|---|----------|
| 0 | +0 |
| 1 | −1 |
| 2 | −2 |
| 3 | −4 |
| 4 | −8 |
| 5 | −16 |
| 6 | ×2/3 (table lookup, not exact) |
| 7 | ×1/2 |
| 8 | +0 |
| 9 | +1 |
| A | +2 |
| B | +4 |
| C | +8 |
| D | +16 |
| E | ×3/2 |
| F | ×2 |

## Notes on FX Player implementation

In `src/core/format/s3m.c` / `src/core/effect/efc_s3m.c` the Cxy jump command stores the row in decimal-as-hex (BCD). The order-jump bounds use strict `<` against `S3M_OrderNum` — the `<=` in the original DOS source is an off-by-one OOB read (see BUGS.md O-3). The deferred-jump pattern (`S3M_jump` → `S3M_goRowOrder`) is used here the same as in MOD and 669.

**Known deviations from st3play confirmed behavior:**

- **Vxx (set global volume)**: fxplayer processes this on tick 0 via `S3M_GlobalEffect`. st3play only fires it on tick > 0 (`sotherjmp`). Consequence: at `speed == 1`, fxplayer incorrectly applies the new global volume (st3play leaves it unchanged because there are no non-zero ticks).
- **Dxy both-nibbles case**: fxplayer's `S3M_TickEffect` does nothing when both x and y are non-zero and neither is 0xF. st3play's `s_volslide` slides down by y in that case (treating it as equivalent to D0y).
- **Qxy counter not reset on absent row**: fxplayer never resets `S3M_RetrigCount` when the effect is absent. st3play resets `atrigcnt = 0` whenever `cmd == 0` (no effect on channel).
- **Volume clamp at 64 vs 63**: `S3M_addVolume`/`S3M_decVolume` clamp to 64; st3play uses `CLAMP(avol, 0, 63)`. Active volume in effects should peak at 63.
- **Bxx with info=0xFF**: st3play treats `B00FF` as "end of song" (`breakpat = 255`); fxplayer sets `nextorder = 0xFF` which could OOB-index into the order table if it has fewer than 256 entries.

## References

- [st3play (8bitbubsy)](https://github.com/8bitbubsy/st3play) — direct C port of ST3.21 ASM/C source — **primary reference**
- [MultimediaWiki — effects with all quirks](https://wiki.multimedia.cx/index.php?title=Scream_Tracker_3_Module#Effects)
- [ModdingWiki — effects overview](https://moddingwiki.shikadi.net/wiki/S3M_Format#Effects)
- [TECH.DOC — original ST3 effect documentation](https://pollak.thebe.de/b/Modules/docs/TECH.txt)
