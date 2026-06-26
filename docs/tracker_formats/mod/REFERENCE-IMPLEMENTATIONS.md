# MOD Reference Implementations

## Primary reference: pt2-clone (`8bitbubsy/pt2-clone`)

[pt2-clone repository](https://github.com/8bitbubsy/pt2-clone)

pt2-clone is a near bit-exact reimplementation of **ProTracker 2.3D** for modern
platforms by 8bitbubsy — the same author as st3play (the S3M reference). The
replayer (`src/pt2_replayer.c`) translates the original Amiga assembly code to C,
preserving quirks (like the shared `n_vibratopos` reference in the tremolo ramp
check). When fxplayer deviates from pt2-clone, one of three things is true:
1. fxplayer has a port bug (fixable)
2. fxplayer faithfully reproduces the original FX Player DOS code (source conflict,
   user decides)
3. pt2-clone itself deviates intentionally from the original

The original FX Player (`_original/`) is the second ground truth for this port and
takes precedence over pt2-clone for bit-exactness decisions.

### Source file map

| pt2-clone file | Role | Equivalent in fxplayer |
|---|---|---|
| `src/pt2_replayer.c` | Engine init, all effects, row advance, pattern delay | `mod.c` (load + row engine) + `efc_mod.c` |
| `src/pt2_tables.h` | Period tables for all 16 finetunes × 37 notes | `mod.c` `MOD_Periods[16][84]` |
| `src/pt2_audio.c` | CIA timer / BPM to tick-length conversion | `calc.c` `s3m_calc_speed` |

### Dispatch structure: tick 0 vs tick > 0

`checkMoreEffects(ch)` — called **once per row at tick 0** (new-note path). Handles
effects B (positionJump), D (patternBreak), E (extended), F (setSpeed), C
(volumeChange), 9 (sampleOffset). Volume slide (A) is **absent** — it does NOT fire
on tick 0.

`chkefx2(ch)` / `checkEffects(ch)` — called **every tick > 0**. Handles effects
0 (arpeggio), 1 (portaUp), 2 (portaDown), 3 (tonePortamento), 4 (vibrato), 5
(tonePlusVolSlide), 6 (vibratoPlusVolSlide), 7 (tremolo), A (volumeSlide), E.

This dispatch structure is the ground truth for all tick-0 vs tick > 0 questions.

### Key decisions (excerpted from source)

**Default BPM and speed:**
```c
// start-of-playback initialization:
song->currSpeed = 6;
song->currBPM = 125;
```

**F00 — stop song:**
```c
static void setSpeed(moduleChannel_t *ch) {
    if ((ch->n_cmd & 0xFF) > 0) {
        if (...) modSetSpeed(ch->n_cmd & 0xFF);
        else ciaSetBPM = ch->n_cmd & 0xFF;   // F20+ = set BPM
    } else {
        // F00 - stop song
        doStopSong = true;
    }
}
```

**Pattern delay (EEx) — two-variable countdown:**
```c
static void patternDelay(moduleChannel_t *ch) {
    // only starts if no delay is already running (pattDelTime2 == 0)
    if (song->tick == 0 && pattDelTime2 == 0)
        pattDelTime = (ch->n_cmd & 0xF) + 1;  // +1: includes the current row
}
// in row-advance logic:
if (pattDelTime > 0) { pattDelTime2 = pattDelTime; pattDelTime = 0; }
if (pattDelTime2 > 0) { pattDelTime2--; if (pattDelTime2 > 0) song->row--; }
```
EEx with y=3 → `pattDelTime = 4` → row held for 4 total plays (1 + 3 extra).

**Fine portamento up (E1x) — lower nibble only:**
```c
static void finePortaUp(moduleChannel_t *ch) {
    if (song->tick == 0) {
        lowMask = 0xF;
        portaUp(ch);   // ch->n_period -= (cmd & 0xFF) & 0xF = y
    }
}
```
pt2-clone subtracts only `y` (the low nibble). The period is stored as the actual
Amiga period (not scaled). See BUGS-MOD.md for comparison with original FX Player.

**Volume slide (Axy) — tick > 0 only:**
```c
static void volumeSlide(moduleChannel_t *ch) {
    // no tick check — caller (chkefx2) is only called for tick > 0
    uint8_t param = ch->n_cmd & 0xFF;
    if ((param & 0xF0) == 0) { ch->n_volume -= param & 0x0F; ... }
    else                     { ch->n_volume += param >> 4;   ... }
}
```
`volumeSlide` is unreachable from `checkMoreEffects` (tick-0 path) — confirmed by
source structure.

**Tremolo — separate state from vibrato:**
```c
// vibrato:  n_vibratocmd  (speed in high nibble, depth in low),  n_vibratopos
// tremolo:  n_tremolocmd  (speed in high nibble, depth in low),  n_tremolopos
// waveforms: n_wavecontrol: low nibble = vibrato type, high nibble = tremolo type
```
Vibrato and tremolo have completely separate speed/depth/position state. Note: a
known ProTracker 2.3D bug makes the tremolo ramp check use `ch->n_vibratopos`
instead of `ch->n_tremolopos` — pt2-clone preserves this with a comment.

---

## Other open-source players to cross-check

| Player | Language | Relevant files | Notes |
|--------|----------|----------------|-------|
| [OpenMPT](https://github.com/OpenMPT/openmpt) | C++ | `soundlib/Load_mod.cpp`, `soundlib/Snd_fx.cpp` | Comprehensive; handles many MOD variants. Has explicit compat flags per tracker. |
| [libxmp](https://github.com/libxmp/libxmp) | C | `src/loaders/mod_load.c`, `src/effects.c` | Conservative; tested against many edge cases. |
| [Schism Tracker](https://github.com/schismtracker/schismtracker) | C | `player/effects.c` | Derived from the same codebase family; useful for effect timing. |
| [ft2play](https://github.com/8bitbubsy/ft2play) | C | `ft2_replayer.c` | Same author as pt2-clone; useful for comparing philosophy. |
| [MilkyTracker](https://github.com/milkytracker/MilkyTracker) | C++ | `src/tracker/mod/` | Has a dedicated MOD replay path. |
| [mikmod](https://mikmod.sourceforge.net/) | C | `libmikmod/drivers/`, loaders | Classic reference player; widely tested. |

For any SOURCE CONFLICT finding (see BUGS-MOD.md), the goal is at least two
independent players that agree — that consensus is more reliable than any single
source.

---

## Source conflicts between pt2-clone and original FX Player (`_original/`)

These are places where pt2-clone (ProTracker 2.3D behavior) differs from the
original FX Player DOS code. In all cases, fxplayer currently preserves the
original's behavior (bit-exact requirement). See BUGS-MOD.md for full analysis.

| Topic | pt2-clone (ProTracker 2.3D) | Original FX Player / fxplayer port |
|-------|----------------------------|--------------------------------------|
| Default BPM | 125 | 128 |
| Volume slide (Axy) timing | tick > 0 only | also fires on tick 0 |
| Tremolo state | separate speed/depth/position | shares vibrato speed/depth/position |
| E1x/E2x slide amount | low nibble `y` (period units) | full info byte `0x1y` (off by ~16×) |
| EEx pattern delay | two-variable countdown, notes fire on delay row | buffer self-modification (broken in port, see BUGS-MOD.md M1) |
| E3x glissando control | sets `n_glissfunk` low nibble | ignored (no-op) |
| Order list scan | 128 entries | 127 entries (faithful to original) |

These conflicts are documented in BUGS-MOD.md with SOURCE CONFLICT callouts. They
should be revisited before any public compatibility work that targets ProTracker
files from other authors.
