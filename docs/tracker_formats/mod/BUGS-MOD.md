# MOD Port — Bug Report

Findings from a systematic review of `src/core/format/mod.c` and
`src/core/effect/efc_mod.c` against:

- `docs/tracker_formats/mod/FILE-FORMAT.md` and `EFFECTS.md` (format reference)
- `_original/DAT_MOD.CPP` and `_original/EFC_MOD.CPP` — the original DOS source
  (Apollo/STIGMA, 1996–1998). Used as a reference for what FX Player originally did.
- [pt2-clone (`8bitbubsy/pt2-clone`)](https://github.com/8bitbubsy/pt2-clone) —
  a near bit-exact C reimplementation of ProTracker 2.3D. Used as the ProTracker
  ground truth for timing and behavior questions.

See [REFERENCE-IMPLEMENTATIONS.md](REFERENCE-IMPLEMENTATIONS.md) for the full pt2-clone
source map, key excerpts, and a summary of all original-vs-pt2-clone conflicts.

## Validation phase

The porting phase is complete: the C99 core was validated bit-exactly against the
DOS binary. The focus is now **tracker authenticity** — ProTracker 2.3D is the
correctness reference for MOD playback, not the DOS binary.

Bugs are classified as:
- **Port bugs** — the port diverges from both the original and ProTracker; fix immediately.
- **Source conflicts (SC)** — the original FX Player and ProTracker disagree. These
  are now open questions: the author decides whether to follow ProTracker (more
  authentic) or keep the original behavior. Each SC entry documents both behaviors.

When a fix intentionally changes output, regenerate the affected reference WAVs and
update the CTest hashes.

---

## Severity classification

| Tag | Meaning |
|-----|---------|
| **CRITICAL** | Crash, hang, or memory corruption on real inputs |
| **HIGH** | Wrong behaviour on valid but uncommon inputs |
| **MEDIUM** | Spec deviation audible in some real modules |
| **LOW** | Edge-case deviation unlikely to affect tested files |

---

## BUG-M1 — CRITICAL: `EEx` pattern delay never expires → permanent hang

**File:** `src/core/effect/efc_mod.c`, `MOD_GlobalEffect` case 14/sub-14;
and `src/core/format/mod.c`, `MOD_goRowOrder`.

**Root cause:** The original DOS code implemented pattern delay using a
**self-modification** trick: `EFC_MOD.CPP`'s `MOD_RowEffect` decremented the
delay counter directly inside `MOD_RowBuffer[channel*4+3]` on each row-held call,
eventually writing `EE0` which reset `MOD_flag_note = 0`. This worked because the
original played from the same buffer without re-unpacking.

The port re-unpacks each row via `MOD_unpack_row` (reads fresh from pattern data)
before calling `MOD_read_row`. The buffer modification is immediately overwritten.
The countdown never happens.

Additionally, the port moved EEx handling from `MOD_RowEffect` (after note
triggers) to `MOD_GlobalEffect` (before note triggers), so on the first row with
EEx, notes are incorrectly suppressed.

**Code (port — `efc_mod.c`):**
```c
case 14u:  /* pattern delay */
    MOD_flag_note = (uint8_t)y;  // set once, never decremented
    break;
```

**`MOD_goRowOrder` (`mod.c`):**
```c
if ( MOD_flag_note == 0 )
    MOD_row++;
// flag never reaches 0 → row never advances → hang
```

**pt2-clone cross-check:** `src/pt2_replayer.c` uses a two-variable approach:
```c
// patternDelay (called at tick 0 only if no delay already running):
if (song->tick == 0 && pattDelTime2 == 0)
    pattDelTime = (ch->n_cmd & 0xF) + 1;

// in row-advance logic:
if (pattDelTime > 0) { pattDelTime2 = pattDelTime; pattDelTime = 0; }
if (pattDelTime2 > 0) { pattDelTime2--; if (pattDelTime2 > 0) song->row--; }
```

**Original DOS code (`_original/EFC_MOD.CPP`, `MOD_RowEffect`):**
```cpp
case 0x0e : // pattern delay for y notes
    if (y != 0) {
        y--;
        MOD_RowBuffer[channel*4+3] = 0xe0 + y;  // self-decrement in buffer
        MOD_flag_note = 1;
    } else {
        MOD_flag_note = 0;   // expires when buffer reaches EE0
    }
    break;
```

**Fix:**

1. In `MOD_GlobalEffect` (`efc_mod.c`): guard against overwriting an active
   countdown, and keep the EEx handling there (do NOT move back to RowEffect —
   note suppression during delay rows is correct original behavior):
   ```c
   case 14u:  /* EEx — pattern delay */
       if (y != 0 && MOD_flag_note == 0)
           MOD_flag_note = (uint8_t)y;   // only start if not already counting
       break;
   ```

2. In `MOD_goRowOrder` (`mod.c`): decrement the countdown:
   ```c
   if ( MOD_flag_note == 0 )
       MOD_row++;
   else
       MOD_flag_note--;   // count down; row advances when this reaches 0
   ```

**Verification:** construct `test-eex-delay3.mod` — row 0: EE3, note on channel 1.
Row 1: different note. With the fix, channel 1's note at row 0 fires once (tick 0),
then the row is held for 3 additional row-lengths without retriggering, and row 1's
note plays on the 5th row-length. A sha256 comparison against DOSBox-X rendering
of `FX.EXE -w:test.wav` confirms correctness.

---

## BUG-M2 — CRITICAL: `F00` sets `MOD_speed = 0`, row never advances

**File:** `src/core/effect/efc_mod.c`, `MOD_GlobalEffect`, case 15.

**Code:**
```c
case 15u:  /* F - set speed/tempo */
    if (info < 32u) {
        MOD_speed = (uint8_t)info;   // info == 0 → speed = 0 → hang
    } else { ... }
    break;
```

In `mod_render_block`:
```c
MOD_tick++;
if ( MOD_tick == MOD_speed ) {   // never true when speed == 0
    MOD_tick = 0;
    ...advance row...
}
```

With `MOD_speed = 0`: `MOD_tick` increments to 1, 2, 3 … and the condition
`MOD_tick == 0` is never satisfied again. The row sequencer freezes.

**pt2-clone cross-check:** `src/pt2_replayer.c` `setSpeed`:
```c
if ((ch->n_cmd & 0xFF) > 0) { modSetSpeed(...); }
else { doStopSong = true; }  // F00 = explicit stop
```
ProTracker treats F00 as "stop playback." fxplayer has no stop mechanism, so
the appropriate equivalent is to ignore it (no speed change).

**Original DOS cross-check:** `_original/EFC_MOD.CPP` does not contain an F-effect
handler for MOD (the F command in the original's `MOD_GlobalEffect` appears to set
speed unconditionally). The hang was latent in the original too — no real MOD file
uses F00.

**Fix:**
```c
case 15u:  /* F - set speed/tempo */
    if (info == 0u) break;            // F00: ignore (don't hang)
    if (info < 32u) {
        MOD_speed = (uint8_t)info;
    } else {
        MOD_tempo = (uint8_t)info;
        MOD_TickLength = s3m_calc_speed( MOD_tempo, g_MixSpeed );
    }
    break;
```

**Verification:** load `test-f00.mod` (single row with F00). Confirm playback
continues past row 0. This should not match original DOS behavior on F00 (which
would also hang), so no sha256 comparison is meaningful here — just confirm no
freeze.

---

## BUG-M3 — LOW: `MOD_OrderNum` not clamped to ≤ 128

**File:** `src/core/format/mod.c`, `mod_load`.

**Code:**
```c
MOD_OrderNum = s_buf[470];   // 15-sample path
// or
MOD_OrderNum = s_buf[950];   // 31-sample path
```

`MOD_OrderNum` is a raw `uint8_t` from the file, valid range 1–128 per spec. If a
malformed module sets byte 470 (or 950) to a value > 128, the order list access
in `MOD_goRowOrder` can walk past the 128-byte order list:

```c
MOD_Pattern = s_order[MOD_Order];   // s_order points to a 128-byte array
// if MOD_OrderNum > 128 and MOD_Order reaches 128..255 → OOB read
```

The original DOS code also does no clamping, but it uses `malloc`'d buffers and the
read would access heap memory past the order list.

**Fix:**
```c
if ( MOD_OrderNum == 0 || MOD_OrderNum > 128u )
    MOD_OrderNum = 128u;
```

**Verification:** fuzz test with `MOD_OrderNum = 0xFF` in a truncated module.
No crash/sanitizer hit after fix.

---

## Source conflicts (SC) — original FX Player vs ProTracker 2.3D

These are places where pt2-clone (ProTracker 2.3D) and the original FX Player
disagree. The port currently matches the original in all cases — a legacy of the
porting phase. Now that ProTracker authenticity is the goal, each SC is an open
decision point. The author decides which behavior to follow for each entry.

---

### SC-1 — Default tempo: 128 (original) vs 125 BPM (ProTracker)

**Original FX Player (`_original/DAT_MOD.CPP` line 79):**
```cpp
unsigned char MOD_tempo = 128;   // tempo of MOD
```

**pt2-clone `src/pt2_replayer.c`:**
```c
song->currSpeed = 6;
song->currBPM = 125;
```

**fxplayer port (`mod.c` + `efc_mod.c`):**
```c
MOD_tempo = 128;   // faithful copy of original
MOD_speed = 6;
```

Practical impact: MOD files that do not contain an explicit `F7D` (set BPM 125) or
`F80` (set BPM 128) command will play at 128 BPM in fxplayer and in DOSBox-X with
the original `FX.EXE`, but at 125 BPM in ProTracker and pt2-clone. The difference
is about 2.4 % (≈ one semitone of tempo). Most demoscene MOD files do set the
tempo explicitly on row 0, so this rarely audible in practice.

**Status (authenticity phase):** Original FX Player starts at 128 BPM;
ProTracker 2.3D starts at 125 BPM. Most demoscene MODs set BPM explicitly on
row 0, so impact is limited. Decision: keep 128 BPM (original behavior) or
switch to 125 BPM (ProTracker authentic). Requires author decision.

---

### SC-2 — Volume slide (Axy) fires on tick 0

**Original FX Player (`_original/EFC_MOD.CPP`, `MOD_RowEffect`):**
```cpp
case 10 : // Volume Slide
    MOD_Volume[channel] = MOD_decVolume(MOD_Volume[channel], y);
    MOD_Volume[channel] = MOD_addVolume(MOD_Volume[channel], x);
    break;
```

**pt2-clone:** `volumeSlide` is only reachable from `chkefx2` (tick > 0 path); it
is absent from `checkMoreEffects` (tick 0 path). Volume slide fires on ticks 1 to
speed−1 only. At speed 1, Axy does nothing.

**fxplayer port (`efc_mod.c`, `MOD_RowEffect`):** volume slide appears in
`MOD_RowEffect` (tick-0 path) AND `MOD_TickEffect` (tick > 0 path). Faithful copy
of original.

Practical impact: at speed 6, fxplayer applies 6 volume steps per row; pt2-clone
applies 5. Effects 5 (portamento + volslide) and 6 (vibrato + volslide) are
affected the same way.

**Status (authenticity phase):** Diverges from ProTracker (tick-0 vs tick >0).
Original FX Player fires on tick 0; ProTracker does not. Requires author decision.

---

### SC-3 — Tremolo shares vibrato speed, depth, and position counters

**Original FX Player (`_original/EFC_MOD.CPP`, `MOD_RowEffect`):**
```cpp
case  7 : // Tremolo
    if ( x != 0 ) MOD_VibratoSpeed[channel] = x;
    if ( y != 0 ) MOD_VibratoDepth[channel] = y;
    break;
```

Same variables (`MOD_VibratoSpeed`, `MOD_VibratoDepth`, `MOD_VibratoPosition`) are
written by both effect 4 (vibrato) and effect 7 (tremolo).

**pt2-clone:** uses entirely separate per-channel fields:
- vibrato: `n_vibratocmd`, `n_vibratopos`, `n_wavecontrol` low nibble
- tremolo: `n_tremolocmd`, `n_tremolopos`, `n_wavecontrol` high nibble

Note: a known ProTracker 2.3D bug causes the tremolo ramp waveform to check
`ch->n_vibratopos` instead of `ch->n_tremolopos` — pt2-clone faithfully preserves
this quirk with a comment.

**fxplayer port:** faithful copy of original. Writing vibrato parameters
overwrites tremolo parameters and vice versa; `MOD_MTremolo` reads
`MOD_VibratoSpeed[ch]` for the "is tremolo active" guard, so tremolo silently
stops when vibrato is used on the same channel.

**Status (authenticity phase):** Diverges from ProTracker, which keeps fully
separate vibrato/tremolo state. Rare in demoscene work. Requires author decision.

---

### SC-4 — E1x/E2x fine portamento uses full info byte instead of low nibble

**Original FX Player (`_original/EFC_MOD.CPP`, `MOD_RowEffect`):**
```cpp
case 0x01 : // Fineslide up
    MOD_Periode[channel] = MOD_decPeriode(MOD_Periode[channel], Info);
    break;
case 0x02 : // Fineslide down
    MOD_Periode[channel] = MOD_addPeriode(MOD_Periode[channel], Info);
    break;
```

`Info` for E1y is the full byte `0x1y` (e.g., E15 → Info = 0x15 = 21). The
period is stored as `actual_period × 16` (4-bit fixed-point for smooth slides).
For a slide by `y` actual period units, the correct delta is `y << 4`. The
original subtracts `0x1y` ≈ 16 + y, which equals ≈ 1 + y/16 actual period units.

**pt2-clone `finePortaUp`:**
```c
lowMask = 0xF;
portaUp(ch);   // ch->n_period -= (cmd & 0xFF) & 0xF = y only
```
Period is unscaled in pt2-clone; subtracts exactly `y` period units.

**fxplayer port (`efc_mod.c`, `MOD_RowEffect`):**
```c
case 1u:  /* fine porta up — slide amount is full Info byte */
    MOD_Period[channel_index] = MOD_decPeriod(..., info);
    break;
```

Faithful copy of original. For E15 (slide by 5): subtracts 0x15 = 21 from the
stored (×16) period, equivalent to ≈ 1.3 actual period units instead of 5.

**Status (authenticity phase):** Audibly wrong relative to ProTracker on any
module using E1x/E2x. The original applies roughly 1/16th the intended slide
amount. This is a candidate for correction to ProTracker behavior. Requires
author decision.

---

### SC-5 — Order list scan checks 127 entries, spec requires 128

**Original FX Player (`_original/DAT_MOD.CPP` line 356):**
```cpp
for (test = 0; test < 127; test++)   // scans indices 0..126 only
```

**fxplayer port (`mod.c`):**
```c
for ( i = 0; i < 127u; i++ )   // faithful copy
```

Spec (FILE-FORMAT.md, aes.id.au): scan all 128 entries to find the highest pattern
index. Skipping entry 127 means if the highest pattern index is only referenced
from order position 127, `MOD_patterns` will be too low. Sample-data pointers are
computed from `MOD_patterns × channels × 64 × 4`; undercount shifts all sample
pointers by `(missed_patterns × channels × 256)` bytes, potentially reading
sample audio from wrong file offsets.

In well-formed demoscene MOD files the 128th order entry is typically 0 or unused
(song_positions ≤ 127), so this is latent. A file with exactly 128 valid entries
where entry 127 has the highest pattern index would trigger it.

**Status (authenticity phase):** Both the original and the port scan 127
entries; ProTracker scans 128. Trivial to fix (`127u` → `128u`). Low risk,
candidate for correction.

---

### SC-6 — E3x glissando control unimplemented

**Original FX Player (`_original/EFC_MOD.CPP`):** no case for E3x — it is silently
ignored.

**fxplayer port (`efc_mod.c`, `MOD_RowEffect`):**
```c
case 3u:  /* set gliss control */
    break;
```

**pt2-clone:** `setGlissControl(ch)` stores the flag in `n_glissfunk` low nibble;
`tonePortNoChange` checks it to slide in semitone steps vs smoothly.

Effect 3 (portamento to note) always slides smoothly in fxplayer and the original.
E3x is extremely rare in demoscene MOD files.

**Status (authenticity phase):** Both original and port ignore E3x. ProTracker
implements it. Low priority; E3x is rare in demoscene MOD files.

---

## Not a bug — FILE-FORMAT.md header offsets

The offset table in `FILE-FORMAT.md` lists wrong hex values for the 15-sample and
31-sample header layouts:

| Field | FILE-FORMAT.md (wrong) | Correct (code + original) |
|-------|------------------------|---------------------------|
| 15-sample order count | 0x01C6 = 454 | 0x01D6 = 470 (20 + 15×30) |
| 15-sample restart byte | 0x01C7 = 455 | 0x01D7 = 471 |
| 15-sample order list | 0x01C8 = 456 | 0x01D8 = 472 |
| 31-sample order count | 0x0196 = 406 | 0x03B6 = 950 (20 + 31×30) |
| 31-sample restart byte | 0x0197 = 407 | 0x03B7 = 951 |
| 31-sample order list | 0x0198 = 408 | 0x03B8 = 952 |

Both the fxplayer port and the original DOS code use the correct offsets (470/472
and 950/952). The FILE-FORMAT.md documentation was written with arithmetic errors.
These offsets have been corrected in FILE-FORMAT.md directly; no code change needed.

---

## Verification plan

### Cross-check against other open-source players

For each SOURCE CONFLICT, use at least two of the following to determine whether
the original's behavior is unique to FX Player or shared among Amiga-era MOD
players. See REFERENCE-IMPLEMENTATIONS.md for full list.

| Player | Reason to include |
|--------|-------------------|
| [pt2-clone](https://github.com/8bitbubsy/pt2-clone) | ProTracker 2.3D ground truth |
| [OpenMPT](https://github.com/OpenMPT/openmpt) | Highest-coverage compatible tracker |
| [libxmp](https://github.com/libxmp/libxmp) | Well-tested conservative reference |

---

### Test MOD files to build

All test files: one instrument, two channels (or one), minimal rows. For port
bugs (M1–M3), compare against ProTracker behavior as ground truth. For SC
entries, compare output between fxplayer and pt2-clone to characterize the
difference. DOS/DOSBox renders are still useful for regression checking but are
no longer the correctness reference.

| Test file | Targets | What to verify |
|-----------|---------|----------------|
| `test-m1-eex3.mod` | BUG-M1 | Row 0: EE3 + note on ch1. Row 1: different note on ch1. Confirm ch1 note at row 0 fires once, is held 3 extra row-lengths, then row-1 note fires. |
| `test-m1-eex0.mod` | BUG-M1 | Row 0: EE0 (no delay). Confirm playback is identical to same file without EE0. |
| `test-m2-f00.mod` | BUG-M2 | Row 0: F00. Row 1: note on ch1. Confirm playback doesn't freeze and row 1 sounds. |
| `test-m3-ordernum.mod` | BUG-M3 | MOD_OrderNum byte set to 200. Load only (don't play in DOSBox with real hardware). Confirm no crash/sanitizer hit in fxplayer after fix. |
| `test-sc1-notempo.mod` | SC-1 | No F command anywhere. Play in fxplayer (128 BPM) and pt2-clone (125 BPM). Measure waveform length ratio ≈ 125/128 ≈ 0.977. |
| `test-sc2-volslide.mod` | SC-2 | Speed 6. Axy on ch1 all rows. Count volume steps per row: fxplayer = 6×; pt2-clone = 5×. |
| `test-sc4-e1x.mod` | SC-4 | E15 (fine porta up by 5). Compare pitch after one row in fxplayer vs pt2-clone. |
| `test-sc5-order127.mod` | SC-5 | song_positions = 128; highest pattern index only at order[127]. Confirm correct pattern count and non-corrupted sample data. |

---

## Summary table

| ID | Severity | Location | Issue | Status |
|----|----------|----------|-------|--------|
| M1 | CRITICAL | `efc_mod.c` `MOD_GlobalEffect`, `mod.c` `MOD_goRowOrder` | EEx sets `MOD_flag_note` but never decrements it → hang | port bug, fix needed |
| M2 | CRITICAL | `efc_mod.c` `MOD_GlobalEffect` case 15 | F00 sets `MOD_speed = 0` → row never advances | port bug, fix needed |
| M3 | LOW | `mod.c` `mod_load` | `MOD_OrderNum` not clamped to ≤ 128 → potential OOB order table read | port bug, fix needed |

| Conflict | Original / fxplayer | pt2-clone (ProTracker 2.3D) |
|----------|--------------------|-----------------------------|
| SC-1 | Default tempo 128 BPM | 125 BPM |
| SC-2 | Volume slide fires on tick 0 | Tick > 0 only; Axy does nothing at speed 1 |
| SC-3 | Tremolo shares vibrato speed/depth/position | Fully separate tremolo state |
| SC-4 | E1x/E2x uses full info byte `0x1y` as slide amount | Uses low nibble `y` only |
| SC-5 | Order scan 127 entries | n/a (pt2-clone: 128 entries) |
| SC-6 | E3x glissando ignored | Implemented (`n_glissfunk`) |
