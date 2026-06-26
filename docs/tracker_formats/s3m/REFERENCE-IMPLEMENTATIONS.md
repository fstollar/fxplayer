# S3M Reference Implementations

## Primary reference: st3play (`8bitbubsy/st3play`)

[st3play repository](https://github.com/8bitbubsy/st3play)

st3play is a direct C port of the original Scream Tracker 3.21 ASM/C source code by
8bitbubsy. It is the ground truth for S3M playback — the only reference that maps
one-to-one with the actual ST3.21 binary. When any other source (MultimediaWiki,
OpenMPT, libxmp) disagrees with st3play, st3play wins unless there is evidence that
st3play itself deviates intentionally from the original.

### Source file map

| st3play file | Role | Equivalent in fxplayer |
|---|---|---|
| `dig.c` | Engine init, global effects, `checkheader()`, BPM table | `s3m.c` (load), `efc_s3m.c` (global) |
| `digcmd.c` | All effect handlers (`s_volslide`, `s_jmpto`, `s_break`, `s_setgvol`, `docmd1`, ...) | `efc_s3m.c` |
| `load.c` | Module loader, `checkins()`, sample conversion | `s3m.c` |
| `digread.c` | Pattern decoder, `donewnote()` | `s3m.c` (row reader) |

### Key decisions (excerpted from source)

**`checkheader()` in `dig.c` — init_speed / init_tempo validation:**
```c
if (song.header.inittempo != 0) settempo(song.header.inittempo);
else                             settempo(125);

if (song.header.initspeed != 255) setspeed(song.header.initspeed);
else                               setspeed(6);
```
`setspeed()` also guards against zero: `if (val > 0) song.musicmax = val;`
This is the ground truth for BUG-S3.

**`checkins()` in `load.c` — sample / instrument validation:**
```c
if (ins->vol > 64) ins->vol = 64;         // instrument vol capped at 64, not 63
if (ins->c2spd > 65535) ins->c2spd = 65535;
if (ins->length > 64000) ins->length = 64000;
if (ins->lend == ins->lbeg) ins->flags &= 0xFE;  // disable loop if end == begin
if (ins->lend < ins->lbeg) ins->lend = ins->lbeg + 1;
```
8-bit unsigned samples are XOR'd with `0x80` byte-by-byte in `load.c`; there is no
16-bit sample conversion path — ST3.21 never supported 16-bit samples natively.
This is the ground truth for BUG-S1 and the BUG-S7 SOURCE CONFLICT (instrument vol
allowed up to 64; only the running `avol` is capped at 63).

**`digcmd.c` — `s_volslide` (Dxy effect):**
```c
else if (song.fastvolslide || song.musiccount > 0)
{
    if (infolo == 0) ch->avol += infohi;    // slide up
    else             ch->avol -= infolo;    // slide down (fires when BOTH nibbles non-zero)
}
```
When both `infohi` and `infolo` are non-zero: `avol -= infolo`. This is the ground
truth for BUG-S4 (fxplayer misses this case).

**`digcmd.c` — `docmd1` / retrigger counter (Qxy):**
```c
// cmd == 0 (no effect):
ch->atrigcnt = 0;

// cmd == 4 (volume slide D):
ch->atrigcnt = 0;   // comment: "fix trigger D"
```
Counter only resets for no-effect and volume-slide, not for arbitrary other effects.
Ground truth for BUG-S5 and SC-2.

**`digcmd.c` — jump tables (Vxx global volume):**
```c
soncejmp[22]  = s_ret;       // tick=0: Vxx is a no-op
sotherjmp[22] = s_setgvol;   // tick>0: Vxx fires
```
Ground truth for BUG-S6.

**`digcmd.c` — `s_jmpto` (Bxx pattern jump):**
```c
if (ch->info == 0xFF)
    song.breakpat = 255;   // end-of-song sentinel
else
    { song.breakpat = 1; song.jmptoord = ch->info; }
```
Ground truth for BUG-S8 (B0xFF is end-of-song, not a jump to order 255).

**`digread.c` — `s_break` (Cxy pattern break, BCD decoding):**
```c
const uint8_t hi = ch->info >> 4;
const uint8_t lo = ch->info & 0x0F;
if (hi <= 9 && lo <= 9)
{
    song.startrow = (hi * 10) + lo;
    song.breakpat = 1;
}
```
Both nibbles must be valid decimal digits (0–9); hex nibbles cause the command to be
silently ignored. Ground truth for SC-3.

**`digcmd.c` — volume clamp throughout:**
```c
ch->avol = CLAMP(ch->avol, 0, 63);
setvol(ch);
```
Active channel volume (`avol`) is always clamped to 63, never 64. Ground truth for
BUG-S7 (fxplayer uses 64 as the ceiling).

**`digcmd.c` — tremor (Ixy) counter:**
```c
// When cmd != Ixy:
ch->atremor = 0;
ch->atreon  = true;
```
Resets to the start of the on-phase whenever the channel has any non-tremor command.
Ground truth for SC-1 (contradicts MultimediaWiki "counters never reset").

---

## Format specification: MultimediaWiki

[MultimediaWiki — Scream Tracker 3 Module](https://wiki.multimedia.cx/index.php?title=Scream_Tracker_3_Module)

The most complete written description of the S3M binary format. Used for structure
offsets, field sizes, and effect semantics. Where MultimediaWiki and st3play disagree,
st3play is the tie-breaker (it is the executable, not documentation). Disagreements
are recorded as SOURCE CONFLICT entries in BUGS-S3M.md.

**Known MultimediaWiki vs st3play conflicts (documented in BUGS-S3M.md):**

| Topic | MultimediaWiki | st3play |
|-------|---------------|---------|
| `init_tempo < 33` at load | Ignored, previous value used | Only `== 0` is ignored; 1–32 applied |
| Qxy counter reset condition | Resets on any non-Qxy row | Resets only on cmd==0 or cmd==4 (Dxy) |
| Vxx timing | "Processed on tick 1" | Fires on every tick > 0 (idempotent) |
| Volume cap | "Volumes peak at 63" (general) | Only `avol` capped at 63; instrument vol allowed 64 |
| Cxy hex nibbles | Only row-range check (`< 64`) | Also validates BCD digits (both nibbles 0–9) |
| Tremor counter | "Never reset except in tremor procedure" | Resets to on-phase on any non-Ixy command |

---

## Secondary cross-check players

These are used to confirm or challenge st3play findings. They are independent
reimplementations; none is the ground truth.

### OpenMPT (`soundlib/Load_s3m.cpp`, `soundlib/Snd_fx.cpp`)

[OpenMPT repository](https://github.com/OpenMPT/openmpt)

The most thoroughly documented modern S3M-compatible tracker. Has explicit
compatibility flags per tracker version (`SONG_ITOLDEFFECTS`, etc.) and is often
the most conservative in following spec nuances. Useful for the volume-cap question
(confirms `CLAMP(avol, 0, 63)` throughout) and for cases where st3play may have
modernisation fixes.

### libxmp (`src/loaders/s3m_load.c`, `src/effects.c`)

[libxmp repository](https://github.com/libxmp/libxmp)

Well-tested multi-platform library. Conservative and widely deployed; good for
verifying that fixes don't break common modules.

### Schism Tracker

[Schism Tracker repository](https://github.com/schismtracker/schismtracker)

Derived from the same codebase family as st3play; useful as a second data point for
effect semantics where st3play's translation may have introduced subtle differences.

### ft2play (`8bitbubsy/ft2play`)

[ft2play repository](https://github.com/8bitbubsy/ft2play)

Same author as st3play; useful for comparing handling philosophy and any shared
patterns in the Kxy/Lxx volume-slide+effect combinations.

---

## Consensus table for BUGS-S3M.md findings

| Bug | st3play | MultimediaWiki | Notes |
|-----|---------|----------------|-------|
| S1: conv_u16m sample-count vs byte-count | 8-bit XOR only; no 16-bit conversion | — | st3play is definitive; 16-bit path is fxplayer extension |
| S2: min file-size check | Full header struct read; `meof()` abort | — | st3play reads full header atomically |
| S3: init_speed 0 / 255 / init_tempo 0 | 0 → setspeed(6); 255 → skip; tempo 0 → 125 | tempo < 33 ignored | st3play is definitive for speed; tempo conflict documented |
| S4: Dxy both nibbles | `avol -= infolo` | `D0y` behavior | st3play definitive |
| S5: Qxy counter reset | cmd==0 or cmd==4 only | Any non-Qxy row | st3play definitive; SC-2 is an artefact of original ASM |
| S6: Vxx timing | `sotherjmp[22]` — fires tick>0 | "Processed on tick 1" | Functionally identical; st3play fires on ALL tick>0 |
| S7: volume cap 63 vs 64 | `avol` capped 63; instrument vol 64 | "volumes peak at 63" | st3play draws narrower boundary |
| S8: B0xFF | `breakpat = 255` (end-of-song) | — | st3play definitive |
