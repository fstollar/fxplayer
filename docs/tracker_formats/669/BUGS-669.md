# 669 Port — Bug Report

Findings from a systematic review of `src/core/format/m669.c` and
`src/core/effect/efc_669.c` against five independent reference implementations:

- [libxmp `src/loaders/669_load.c`](https://github.com/libxmp/libxmp)
- [OpenMPT `soundlib/Load_669.cpp`](https://github.com/OpenMPT/openmpt)
- [Open Cubic Player `playgmd/gmdl669.c`](https://github.com/mywave82/opencubicplayer)
- [Schism Tracker `fmt/669.c`](https://github.com/schismtracker/schismtracker)
- [MikMod `libmikmod/loaders/load_669.c`](https://github.com/sezero/mikmod)

There is no community "st3play equivalent" (direct port of the original Composer 669
or Unis 669 ASM source) for this format. The five players above are the primary
references. See [REFERENCE-IMPLEMENTATIONS.md](REFERENCE-IMPLEMENTATIONS.md) for
their key decisions in tabular form.

Where players disagree with each other or with the written spec, both positions are
recorded under a **SOURCE CONFLICT** callout. Conflicts with a 4-vs-1 or 5-vs-0
consensus are flagged as effectively resolved; conflicts with a 3-vs-2 or 2-vs-2
split remain open.

Deviations between `_original/DAT_669.CPP` (the author's prior DOS code) and the
current C99 port are noted where relevant; they do not override the five external
references.

---

## Severity classification

| Tag | Meaning |
|-----|---------|
| **CRITICAL** | Memory corruption, crash, or infinite hang on real or crafted inputs |
| **HIGH** | Wrong behaviour on valid but uncommon inputs; hangs on edge-case files |
| **MEDIUM** | Spec deviation or broken host feature visible on normal use |
| **LOW** | Edge-case deviation unlikely to affect tested files |
| **DOC** | Documentation error; code is correct |

---

## BUG-669-L1 — CRITICAL: Unvalidated `sample_length` causes OOB write before bounds check

**File:** `src/core/format/m669.c`, sample-loading loop inside `m669_load`
(lines 274–287).

**Code:**
```c
for (i = 0; i < M669_samples; i++) {
    uint32_t sample_length;
    s_ins[i] = ins_base + i * 0x19u;
    s_smp[i] = ins_base + smp_offset;
    sample_length = *(uint32_t *)(s_ins[i] + 13);   // ← file-controlled 32-bit read
    {
        uint8_t *d = s_smp[i];
        uint32_t j;
        for (j = 0; j < sample_length; j++) d[j] ^= 0x80u;  // ← OOB write
    }
    smp_offset += sample_length;
    if ((size_t)(ins_base + smp_offset - s_buf) > size) return -1;  // ← too late
}
```

**Root cause — three sub-issues:**

**Sub-case A — OOB write before bounds check.**
`sample_length` is a 32-bit value read directly from the file at instrument header
offset 13. It is not validated before use. The XOR loop writes `sample_length` bytes
starting at `s_smp[i]`, which points into the workspace buffer. If `sample_length`
is larger than the remaining sample data in the file (or is adversarially set to
0xFFFFFFFF), the XOR loop writes past the sample data and into the pointer arrays
(`s_ins`, `s_smp`, `s_pat`) or the master volume table, corrupting them. The bounds
check `if ((size_t)(ins_base + smp_offset - s_buf) > size) return -1` runs only
after the corruption has occurred.

**Sub-case B — `smp_offset` integer overflow.**
`smp_offset` is declared `uint32_t`. With a crafted `sample_length = 0xFFFFFFFF`,
`smp_offset += sample_length` wraps to a small value. The subsequent bounds check
then computes a plausible-looking in-range pointer and passes, allowing further
corrupted sample pointers to be built for subsequent samples.

**Sub-case C — Unaligned `uint32_t` read on ARM (UB on Cortex-M target).**
`*(uint32_t *)(s_ins[i] + 13)` reads a 32-bit value at byte 13 of a 25-byte
instrument record. The address is not 4-byte aligned. On ARM Cortex-M (one of the
four port targets), unaligned multi-byte loads via a plain pointer dereference are
undefined behaviour; they either fault or silently read garbage depending on the CPU
configuration.

**libxmp cross-check:** `669_load.c` reads the length with `hio_read32l(f)` (a
portable byte-by-byte read, never unaligned) and immediately sanity-checks:
```c
if (sih.length > MAX_SAMPLE_SIZE) return -1;
```

**OpenMPT cross-check:** `Load_669.cpp` reads via `file.ReadStruct(sampleHeader)`
(safe, portable) and rejects:
```c
if(sampleHeader.length >= 0x4000000) return false;
```

**Fix:**
1. Read `sample_length` byte-by-byte (or use `memcpy` into a local) to avoid the
   unaligned-read UB.
2. Validate before the XOR loop:
   ```c
   uint32_t remaining = (uint32_t)(size - (size_t)(ins_base + smp_offset - s_buf));
   if (sample_length > remaining) return -1;
   ```
3. Use `size_t` for `smp_offset` (or promote to `uint64_t`) so overflow cannot
   defeat the bounds check.

The same unaligned-read pattern applies to `*(uint32_t *)(ins + 17)` (loop start)
and `*(uint32_t *)(ins + 21)` (loop end) in `M669_GetNewSample`; all three should
use `memcpy`.

**Note:** The same structural issue exists in `_original/DAT_669.CPP:304-305`.
Under DOS the process owned conventional memory and a large write past the file
buffer rarely crashed; under the C99 web host it corrupts the caller-supplied
workspace and is exploitable from untrusted drag-and-drop input.

---

## BUG-669-L2 — CRITICAL: Minimum file-size check covers only the fixed header

**File:** `src/core/format/m669.c`, `m669_workspace_bytes` (line 198) and
`m669_load` (line 221).

**Code:**
```c
if (size < 0x1f1u)  return -1;   /* m669_workspace_bytes */
if (size < 0x1f1u)  return -1;   /* m669_load */
```

**Root cause:** `0x1F1 = 497` bytes covers the fixed header only (2 magic + 108
message + 1 NOS + 1 NOP + 1 loop + 128 order + 128 tempo + 128 break = 497). The
loader immediately after the check accesses:

- Instrument descriptors: `ins_base = s_buf + 0x1f1u`, then reads `M669_samples`
  records of 25 bytes each.
- Pattern data: `pat_base = ins_base + M669_samples * 0x19u`, then reads
  `M669_patterns` blocks of 1536 bytes each.

A file of exactly 497 bytes passes the check. If `M669_samples = 10` and
`M669_patterns = 20`, the sample descriptors start at byte 497 and end at
`497 + 10 * 25 = 747`; the patterns start at 747 and end at
`747 + 20 * 1536 = 31467`. A 497-byte file has none of this data. The `s_smp`
pointers point into the workspace buffer past the file image, and the BUG-669-L1
XOR loop writes into uninitialised workspace memory.

**OpenMPT cross-check:**
```cpp
static uint64 GetHeaderMinimumAdditionalSize(const _669FileHeader &fileHeader) {
    return fileHeader.samples * sizeof(_669Sample) + fileHeader.patterns * 1536u;
}
// ProbeAdditionalSize verifies the file is at least this large before proceeding.
```

**libxmp cross-check:** libxmp reads sequentially; a truncated read returns an error
via `hio_read` before any pointer is built.

**Fix:**
```c
size_t min_size = 0x1f1u
                + (size_t)nos * 25u
                + (size_t)nop * 0x600u;
if (size < min_size) return -1;
```

This check must happen after `nos` and `nop` are read but before any instrument or
pattern pointer is computed. Sample audio size is not known until instruments are
parsed, but the L1 fix (per-sample bounds check before the XOR loop) handles that.

---

## BUG-669-L3 — HIGH: `Breaklist` value not validated; values ≥ 64 cause OOB read in pattern buffer

**File:** `src/core/format/m669.c`, `M669_goRowOrder` (line 487) and
`M669_unpack_row` (line 420).

**Code:**
```c
/* M669_goRowOrder — advance when row exceeds break point */
if (M669_row >= (uint32_t)(M669_Breaklist[M669_Pattern] + 1u)) {

/* M669_unpack_row — address row inside pattern */
p = s_pat[pat_nr] + row_nr * M669_CHANNELS * 3u;   /* row_nr * 24 */
```

**Root cause:** The break list (128 bytes at file offset 0x171) stores the index of
the last active row in each pattern (0–63). Neither the loader nor `M669_goRowOrder`
validates these values. If `Breaklist[pat] = 255`, the row counter advances to 255
before the pattern advances. Each call to `M669_unpack_row` computes:
`row_nr * 24 = 255 * 24 = 6120` bytes past the start of the 1536-byte pattern buffer.
This reads far into the adjacent workspace (next pattern, pointer arrays, or master
volume table).

- Row 64: `64 * 24 = 1536` — exactly one byte past the pattern (UB, adjacent memory)
- Row 255: 4584 bytes past the end

**libxmp cross-check:**
```c
pbrk = sfh.pbrk[i];
if (pbrk >= 64) return -1;
```

**OpenMPT cross-check:**
```cpp
if(fileHeader.breaks[i] >= 64) return false;   /* in ValidateHeader */
```

**Fix:** After reading `M669_samples`, `M669_patterns`, and their lists, validate all
break values:
```c
for (i = 0; i < M669_patterns; i++) {
    if (M669_Breaklist[i] >= 64u) return -1;
}
```

---

## BUG-669-L4 — HIGH: Speed value 0 from `Speedlist` not guarded → infinite hang

**File:** `src/core/format/m669.c`, `m669_load` (line 305) and `M669_unpack_row`
(line 415).

**Code:**
```c
/* m669_load — initial speed from first pattern's speed entry */
M669_speed = M669_Speedlist[0];   /* no zero check */

/* M669_unpack_row — per-pattern speed reload */
if (pat_nr != M669_LastPattern) {
    M669_speed = M669_Speedlist[pat_nr];   /* no zero check */
}
```

**Root cause:** With `M669_speed = 0` the render loop in `m669_render_block` compares
`M669_tick == M669_speed` after every tick increment:

```c
M669_tick++;
if (M669_tick == M669_speed) {   /* 1 == 0 is always false */
    M669_tick = 0;
    M669_goRowOrder();
    ...
}
```

`M669_tick` increments without bound; the row sequencer never fires. The render
block loops indefinitely.

The effect-5 handler correctly guards:
```c
case 5u:
    if (info) M669_speed = (uint8_t)info;   /* correct: skips 0 */
    break;
```

The same protection is absent at load time and on every pattern transition.

**Cross-check:** OpenMPT rejects `tempoList[i] == 0` and `tempoList[i] > 15` in
`ValidateHeader`. MikMod rejects `tempos[i] == 0` and `tempos[i] > 32` (wider upper
bound). libxmp passes the value through a speed event whose handler silently ignores 0.

**Note on initial default speed:** `M669_speed` is initialised to 6 before the speed
list is applied. Three of the five reference players use 4 as the initial default
(OpenMPT, Schism Tracker, MikMod); only libxmp also uses 6. Since the speed list
always overwrites this on load, the default only matters when `Speedlist[0] == 0`
(the bug case here). No independent fix is needed for the default value.

**Fix:**
```c
/* at load time */
if (M669_Speedlist[0] != 0) M669_speed = M669_Speedlist[0];

/* in M669_unpack_row, per-pattern reload */
if (pat_nr != M669_LastPattern && M669_Speedlist[pat_nr] != 0)
    M669_speed = M669_Speedlist[pat_nr];
```

Optionally validate the full speed list at load time (reject 0; cap at 32 per MikMod
or 15 per OpenMPT) consistent with the L2 fix.

---

## BUG-669-L5 — MEDIUM: Order-jump bound uses `M669_OrderNum` (loop-restart byte) instead of order-list length

**Files:** `src/core/format/m669.c` line 501; `src/core/engine/fx.c` lines 67–72.

**Code:**
```c
/* fx.c — fx_order_jump computes target order */
if (target >= (int)M669_OrderNum) target = (int)M669_OrderNum - 1;
M669_nextorder = (uint32_t)target;
M669_jump = 1;

/* m669.c — M669_goRowOrder applies the jump */
if (M669_nextorder < M669_OrderNum) {
    M669_Order   = M669_nextorder;
    ...
}
```

**Root cause:** `M669_OrderNum` is loaded from file byte 0x70, which the spec defines
as "Loop order number — the order to restart at when the song ends." It is a restart
position, not an order count. Using it as a count breaks both the bound computation
and the guard.

**When `M669_OrderNum = 0`** (restart from the beginning — the common case):

1. In `fx.c`: `target = M669_Order + delta`. Suppose user presses "skip forward" from
   order 3 → `target = 4`. Then `4 >= 0` is true → `target = -1`. Cast to
   `uint32_t` gives `UINT32_MAX`. `M669_nextorder = UINT32_MAX`.
2. In `M669_goRowOrder`: `UINT32_MAX < 0` (both are uint8 and uint32 respectively;
   unsigned comparison) — the jump body never executes.

**Result:** UI order jumps (both forward and backward) are silently swallowed
whenever the song's loop-restart byte is 0.

**libxmp cross-check:** libxmp scans the order list for its length:
```c
for (i = 0; i < 128; i++) {
    if (sfh.order[i] > sfh.nop) break;
}
mod->len = i;
```

The `mod->len` value (the scanned order-list length) is what should bound jumps.

**Fix:** Track the real order count — scan the order list at load time and store it
in a new variable `M669_OrderCount`:
```c
uint32_t M669_OrderCount = 0;
for (i = 0; i < 128u; i++) {
    if (M669_Orderlist[i] >= M669_patterns) break;  /* 0xFF or OOB = sentinel */
    M669_OrderCount++;
}
```

Replace all uses of `M669_OrderNum` as a count with `M669_OrderCount`. Keep
`M669_OrderNum` for the restart-position semantics if/when song-end looping to a
non-zero position is implemented.

---

## BUG-669-E1 — LOW: Effect 6 (Extended 669 balance) formula overflows `uint8_t` for `info > 9`

**File:** `src/core/effect/efc_669.c`, `M669_RowEffect`, case 6 (line 163).

**Code:**
```c
case 6u:  /* balance / panning */
    M669_Panning[channel_index] = (uint8_t)(info * 28u);
    break;
```

**Root cause:** `info` is the 4-bit data nibble from the effect byte (values 0–15).
`info * 28u` for `info = 10` gives 280, which truncates to 24 when cast to `uint8_t`
(280 − 256 = 24). For `info = 11` through 15: 308 → 52, 336 → 80, 364 → 108,
392 → 136, 420 → 164. These produce far-left panning instead of the expected
right-side values.

Additionally, effect 6 in Extended 669 (Unis 669 `JN` format) is a balance *slide*
(`Gnx`: direction `n` = 0 left / 1 right, amount `x`), not an absolute panning
position. Setting `M669_Panning` to a fixed value ignores the direction and amount
sub-nibbles.

**OpenMPT cross-check:** only nibble values 0 and 1 are used:
```cpp
case 6:  // G - balance fine slide
    switch(m->param) {
        case 0: m->param = 0x4F; break;  // fine slide left
        case 1: m->param = 0xF4; break;  // fine slide right
        default: m->command = CMD_NONE;
    }
```

**libxmp cross-check:** effects 6 and 7 are silently dropped for all `if` files
(the effect table has only 6 entries; out-of-range entries are ignored).

**Severity note:** Effect 6 is an Extended 669 (`JN`) feature. The `m669_load`
function currently rejects `JN` files (accepts `if` magic only), so effect 6 can
only be triggered by malformed `if` files with a 6 in the effect nibble. This limits
the severity: the wrong panning is audible but not a crash.

**Fix (for when `JN` support is added):** decode the sub-nibbles correctly and apply
a per-tick panning delta rather than a fixed assignment. Until then, guard the case
against overflow:
```c
case 6u:
    /* direction = info >> 1; amount = info & 1 -- Extended 669 only */
    M669_Panning[channel_index] = (uint8_t)((info < 9u) ? info * 28u : 252u);
    break;
```

---

## BUG-669-E2 — MEDIUM: Effect carry not implemented; portamento and vibrato stop on any empty cell

**File:** `src/core/format/m669.c`, `M669_unpack_row` (lines 447–454);
`src/core/effect/efc_669.c`, `M669_GetNewEffect` (line 254).

**Code (unpack_row — empty cell handling):**
```c
if (b2 != 0xFFu) {
    effect  = (b2 & 0xF0u) >> 4;
    effinfo = b2 & 0x0Fu;
} else {
    effect  = 0xFFu;   /* ← overwrites active effect on every empty cell */
    effinfo = 0;
}
```

**Code (GetNewEffect — no-effect path):**
```c
case 0xFFu:
    M669_PeriodAdjust[channel_index] = 0;   /* ← kills vibrato on every no-effect row */
    break;
```

**Root cause:** When a pattern cell has `b2 == 0xFF` (no effect byte), the port sets
`Effect = 0xFF` (no-effect sentinel). On the next row, `M669_GetNewEffect` sees the
sentinel and zeros `PeriodAdjust`, silently cancelling any active portamento pitch
accumulation or vibrato offset.

The expected behaviour — shared by four out of five reference implementations — is
that a slide or vibrato effect begun on row N continues on row N+1 if the cell is
empty. The effect only stops when:
- a new note triggers and resets the effect state, or
- an explicit zero-data effect cancels it (see BUG-669-E3).

**Cross-check — effect carry implementation in reference players:**

| Player | Carry mechanism |
|--------|----------------|
| OCP | `commands[chan]` array; resets on `b0 < 0xFE` (new note) |
| OpenMPT | `effect[chan]` array; resets on `noteInstr < 0xFE` |
| Schism Tracker | `effect[chan]` array; resets on `b[0] < 0xFE` |
| MikMod | `lastfx`/`lastval` per channel; when `c == 0xFF`: `c = lastfx, effect = lastval` |
| libxmp | Events written once per cell; no carry (dissenting) |
| fxplayer | No carry — sets `Effect = 0xFF` on empty cells (dissenting) |

Consensus: **4 carry, 2 don't.** The `_original/DAT_669.CPP` also does not carry
(same logic at lines 532–534), so this is a shared limitation rather than a porting
error, but it is a deviation from the majority of players.

**Audible impact:** Any module using portamento up/down over multiple rows where some
cells are empty will play at the wrong pitch in fxplayer — the slide stops each time
the cell is empty and resumes from the original period on the next note-triggered cell.

**Fix:**
```c
/* In M669_unpack_row: do not clobber effect on empty b2 */
if (b2 != 0xFFu) {
    effect  = (b2 & 0xF0u) >> 4;
    effinfo = b2 & 0x0Fu;
    /* carry flag — new effect present */
} else {
    /* carry: leave M669_Effect[ch] and M669_EffectInfo[ch] unchanged */
    effect  = M669_Effect[channel_index];
    effinfo = M669_EffectInfo[channel_index];
}
```

A new note (`b0 < 0xFE`) should reset the carry:
```c
if (b0 < 0xFEu) {
    effect  = 0xFFu;   /* clear carry on new note */
    effinfo = 0;
}
```

This structural change requires care: effects that explicitly should not carry across
rows (effect 5 / set-speed) must also be cleared on carry — Schism Tracker and OCP
do this with `effect[chan] = 0xFF` after handling effect 5.

---

## BUG-669-E3 — LOW: Effect cancel on zero data not implemented

**File:** `src/core/effect/efc_669.c` — no implementation present.

**Documented rule (EFFECTS.md):**
> Setting any of the above effects (A–F) with a value of 00 cancels all active
> effects in that row and notes that follow play at their natural pitch.

**Current behaviour:** An effect with data nibble `0` runs normally with zero
parameter. For portamento (`A00`/`B00`) this is a no-change slide (audibly a no-op),
but the effect state is not cancelled — any accumulated period adjustment from a
previous row's portamento persists.

**Cross-check:**

| Player | Cancel on zero |
|--------|---------------|
| OCP | `if (!data[j]) commands[j] = 0xFF` |
| Schism Tracker | `if ((b[2] & 0x0f) == 0 && b[2] != 0x30) effect[chan] = 0xFF` (exempts effect 3 data-0) |
| OpenMPT | Not explicitly implemented |
| MikMod | Not implemented |
| libxmp | Not implemented |
| fxplayer | Not implemented |

Two of five players implement the cancel rule; it is also stated in the written spec.
The `_original/EFC_669.CPP:685-700` has a switch for this that does nothing — all
cases are empty breaks, suggesting the original author intended it but never finished.

The Schism Tracker note is interesting: it exempts `b[2] == 0x30` (effect 3, data 0)
from cancellation. This suggests the cancel rule may not apply uniformly to all
effects.

**Severity:** LOW — the audible difference only appears in modules that deliberately
use `Axx` or `Bxx` with value 0 as an explicit cancel command, which is uncommon.
The cancel would need to be implemented as part of the BUG-669-E2 carry fix anyway
(it defines when the carried effect stops).

**Fix:** In the carry implementation (see BUG-669-E2), when `b2 != 0xFF` and
`b2 & 0x0F == 0`, set `effect = 0xFFu` to cancel carry rather than latching a
zero-value effect. Adopt the Schism Tracker exemption for effect 3 if testing reveals
that to be correct.

---

## BUG-669-D1 — DOC: EFFECTS.md describes effect 3 (Dxx) as "Frequency adjust" but both code and `_original/` implement it as a finetune table index

**File:** `docs/tracker_formats/669/EFFECTS.md`, effect 3 entry.

**Current text:**
```
| 3 | `Dxx` | Frequency adjust — shift playback frequency by `xx` |
```

**Actual behaviour (code and `_original/EFC_669.CPP:249`):**
```c
case 3u:  /* finetune */
    M669_SampleFinetune[channel_index] = (uint8_t)info;
    break;
```

The 4-bit `info` value (0–15) selects one of 16 pre-computed pitch rows in
`M669_Periods[16][72]`. This is a finetune table index, not a direct frequency
offset. The EFFECTS.md description is wrong; the code is correct.

**Fix:** Update EFFECTS.md effect 3 description:
```
| 3 | `Dxx` | Set finetune — `xx` (0–15) selects the pitch table row for
  subsequent notes; corresponds to ProTracker finetune values
```

---

> **SOURCE CONFLICT — BUG-669-D1 — finetune index vs. frequency portamento (4 vs 2):**
>
> | Player | Effect 3 interpretation |
> |--------|------------------------|
> | libxmp | `FX_669_FINETUNE` — finetune table index |
> | fxplayer | `M669_SampleFinetune = info` — finetune table index |
> | OpenMPT | `CMD_PORTAMENTOUP \| 0xF0` — fine pitch portamento up; comment: "shift by (param×80) Hz per tick" |
> | OCP | `cmdRowPitchSlideUp, data<<2` — per-row pitch shift; comment: "correct? down? both?" |
> | Schism Tracker | `FX_PORTAMENTOUP \| 0xf0` — fine pitch portamento up; comment: "D - frequency adjust (??)"; effect does NOT carry |
> | MikMod | `UNI_S3MEFFECTF \| (0xF0 \| effect)` — S3M extra-fine portamento up; comment: "DMP converts this effect to S3M FF1. Why not?" |
>
> **Consensus: 4 say pitch portamento/shift; 2 say finetune index.**
>
> The code and `_original/` are in the minority position. The doc correction made above
> (changing the description to "Set finetune") is accurate for the current
> implementation, but the implementation itself may be wrong. All three source comments
> in the majority-position players show uncertainty ("correct? down? both?", "??",
> "Why not?") — suggesting this effect was reverse-engineered rather than documented.
>
> **Resolution requires:** a test 669 file using effect 3 (Dxx) with a non-zero value,
> rendered by the original Composer 669 tracker under DOSBox-X. If the output pitch
> shifts continuously each tick, the portamento interpretation is correct; if it jumps
> to a new fixed pitch immediately, the finetune-index interpretation is correct.

---

## Not a bug — `if` magic only; `JN` (Extended 669) silently rejected

`m669_load` checks `data[0] != 0x69u || data[1] != 0x66u` and returns `-1` for
`JN` magic. This is intentional for the current scope. The Effects engine contains
stubs for effects 6 (balance) and 7 (retrig) from the Extended format, but the
loader never admits `JN` files. Both libxmp and OpenMPT support `JN`; extending the
port to do so is a future milestone, not a present bug.

---

## Not a bug — song end restarts at order 0, ignoring the loop-restart byte

`M669_goRowOrder` resets `M669_Order = 0` when pattern `0xFF` is encountered. The
header byte at offset 0x70 (`M669_OrderNum`, described as "loop order number") is
stored but never used for this restart. This matches the behaviour in
`_original/DAT_669.CPP:582-584`.

Three of the five reference players (MikMod, Schism Tracker, OpenMPT) do respect
the loop-restart byte. Implementing it correctly is independent of BUG-669-L5 (which
is about the UI jump bound) and would be a new feature, not a bug fix.

---

## Not a bug — `addPeriod`/`decPeriod` clamp at 10 (port adds safety floor absent from `_original/`)

The port's `M669_addPeriod` and `M669_decPeriod` clamp the result to ≥ 10 to
prevent `Calc_669frequency` from computing `14318181 / 0`. The corresponding
functions in `_original/EFC_669.CPP` have this clamp commented out; the original
relied on the `_669_to_Mixer` check `if (_669_Periode < 10) channel deactivate`
to handle very-low periods, but an unclamped period of 0 would have caused a
division by zero in the frequency calculation under DOS too. The port's clamp is
correct defensive behaviour.

---

## Verification plan

### Open-source players to cross-check

All five reference players have been surveyed and their positions are recorded in
[REFERENCE-IMPLEMENTATIONS.md](REFERENCE-IMPLEMENTATIONS.md). For the one remaining
open conflict (effect 3 semantics), the only remaining arbiter is the original
Composer 669 tracker itself running under DOSBox-X.

### Test 669 files to build

Each file should be a minimal hand-crafted 669 (one or two instruments, fewest rows
possible) that isolates the behaviour under investigation. Where the test targets a
loader crash/hang, load only and do not render. For behavioural tests, render to WAV
in DOSBox-X (`FX.EXE -w:test.wav`) and compare output against fxplayer after fixes.

| Test file | Targets | What to check |
|-----------|---------|---------------|
| `test-breaklist-64.669` | L3 | Break value = 64 in first pattern. Confirm loader rejects file after L3 fix. |
| `test-breaklist-63.669` | L3 | Break value = 63 (max valid). Confirm all 64 rows play, no OOB. |
| `test-speed0.669` | L4 | Speed list entry 0 for first pattern. Confirm loader rejects or falls back to safe default. Load only — do not render (hangs without fix). |
| `test-speed32.669` | L4 | Speed list entry 32 (MikMod's upper limit). Confirm accepted. |
| `test-orderjump.669` | L5 | 4 patterns, `OrderNum = 0`. Trigger UI order-skip. Confirm jump works after L5 fix. |
| `test-orderjump-nonzero.669` | L5 | Same but `OrderNum = 2`. Verify before/after fix. |
| `test-sample-huge-len.669` | L1 | Sample with `length = 0xFFFFFFFF`. Confirm loader rejects instead of writing OOB. |
| `test-effect-carry.669` | E2 | Portamento up (A04) on row 0; empty cells on rows 1–3; note on row 4. With carry: pitch slides continuously rows 1–3. Without carry: pitch returns to base on row 4. Render to WAV and compare waveform against MikMod/OCP output. |
| `test-effect-cancel-zero.669` | E3 | Portamento up (A04) on row 0; `A00` on row 1. With cancel: pitch stops sliding at row 1. Without: slide continues at rate 0 (effectively same pitch). |
| `test-effect3-dxx.669` | D1 | Note at natural pitch, then `D07` (finetune 7 or slide by 7). Run in Composer 669 under DOSBox-X. If pitch jumps immediately to a different fixed tuning, finetune-index is correct. If pitch drifts continuously each tick, portamento is correct. |
| `test-effect6-panning.669` | E1 | Extended 669 (`JN`): effect G values 0–F. Compare panning direction in OpenMPT vs fxplayer. |

---

## Summary table

| ID | Severity | Location | Issue | Consensus |
|----|----------|----------|-------|-----------|
| L1 | CRITICAL | `m669.c` sample loader | `sample_length` not validated before XOR loop → OOB write; unaligned `uint32_t` reads (ARM UB); `smp_offset` uint32 overflow | 5/5 players validate length first |
| L2 | CRITICAL | `m669.c` size check | `size < 0x1f1` only covers fixed header; no guard for `nos×25 + nop×1536` | OpenMPT `GetHeaderMinimumAdditionalSize`; libxmp sequential reads |
| L3 | HIGH | `m669.c` pattern decode | `Breaklist[pat]` ≥ 64 not validated → OOB read in 1536-byte pattern buffer | 4/4 players that check reject `>= 64` |
| L4 | HIGH | `m669.c` load + `M669_unpack_row` | Speed 0 from Speedlist → infinite hang; no zero guard at load or per-pattern | OpenMPT, MikMod reject 0 at load |
| L5 | MEDIUM | `m669.c` + `fx.c` | Order-jump bound uses `M669_OrderNum` (loop-restart byte) not order-list length → UI jumps silently fail when `OrderNum = 0` | libxmp scans order-list length |
| E1 | LOW | `efc_669.c` RowEffect | Effect 6 balance: `info × 28` truncates for `info > 9`; wrong semantics (absolute vs. slide) | OCP, OpenMPT, Schism: nibble 0/1 only |
| E2 | MEDIUM | `m669.c` + `efc_669.c` | Effects do not carry across empty cells — portamento and vibrato stop on any `b2 = 0xFF` row | 4/5 players carry; only libxmp also does not |
| E3 | LOW | `efc_669.c` | Effect with data = 0 should cancel active effect; not implemented | OCP, Schism implement; spec documents it |
| D1 | DOC | `EFFECTS.md` | Effect 3 (Dxx) documented as "Frequency adjust" but implemented as finetune index | SOURCE CONFLICT: 4/5 say pitch portamento; 2/5 say finetune index — unresolved, needs DOSBox-X test |

| Conflict | Positions | Status |
|----------|-----------|--------|
| Effect 3: finetune-index vs. pitch portamento | libxmp + fxplayer = finetune; OpenMPT + OCP + Schism + MikMod = pitch slide | **Open** — 4 vs 2; resolve with DOSBox-X original tracker test |
| Effect carry on empty cells | OCP + OpenMPT + Schism + MikMod = carry; libxmp + fxplayer = no carry | **Effectively resolved** — 4 vs 2 for carry; see BUG-669-E2 |
| Effect cancel on zero data | OCP + Schism + spec = cancel; OpenMPT + MikMod + libxmp = no cancel | **Open** — 2 vs 3; Schism exempts effect 3 from the rule |
