# S3M Port — Bug Report

Findings from a systematic review of `src/core/format/s3m.c` and
`src/core/effect/efc_s3m.c` against:

- `docs/tracker_formats/s3m/FILE-FORMAT.md` and `EFFECTS.md` (format reference)
- [MultimediaWiki — Scream Tracker 3 Module](https://wiki.multimedia.cx/index.php?title=Scream_Tracker_3_Module)
- [st3play by 8bitbubsy](https://github.com/8bitbubsy/st3play) — direct C port of the
  ST3.21 ASM/C source code (the definitive reference implementation)

When docs and MultimediaWiki conflict with actual ST3 behaviour, **st3play is the
tie-breaker** — it is a direct machine translation of the original ASM. All st3play
cross-references point to specific functions in `dig.c`, `digcmd.c`, `load.c`, and
`digread.c`.

Where st3play and MultimediaWiki disagree, both positions are recorded under a
**SOURCE CONFLICT** callout. These entries should be revisited: st3play itself could
have bugs or intentional deviations from the original hardware behaviour.

---

## Severity classification

| Tag | Meaning |
|-----|---------|
| **CRITICAL** | Memory corruption, crash, or infinite hang on real inputs |
| **HIGH** | Wrong behaviour on valid but uncommon inputs |
| **MEDIUM** | Spec deviation audible in some real modules |
| **LOW** | Edge-case deviation unlikely to affect tested files |

---

## BUG-S1 — CRITICAL: `conv_u16m_to_s16m` called with byte count, not sample count

**File:** `src/core/format/s3m.c`, inside `s3m_load`, the
`hdr->file_format_info == 2` branch.

**Code:**
```c
} else {
    if ((flags & 2) == 0) conv_u16m_to_s16m(s_smp[i], len);
}
```

**Root cause:** `len` is read from the instrument header field at offset 0x10, which
is "sample length in bytes" (confirmed by FILE-FORMAT.md and MultimediaWiki). The
function `conv_u16m_to_s16m` iterates over `uint16_t` elements and treats its second
argument as a **count of 16-bit samples**, not bytes:

```c
static void conv_u16m_to_s16m(uint8_t *data, uint32_t length)
{
    uint16_t *s = (uint16_t *)data;
    for (uint32_t i = 0; i < length; i++) s[i] ^= 0x8000u;
}
```

For a 1000-byte 16-bit sample, `len = 1000`. The call XORs 1000 `uint16_t` entries
= **2000 bytes**, writing 1000 bytes past the end of the sample into adjacent
workspace memory. This is silent heap corruption.

**st3play cross-check:** st3play (`load.c` line 231–235) only ever converts 8-bit
samples — it byte-XORs with `0x80` unconditionally regardless of the 16-bit flag.
ST3.21 never supported 16-bit samples natively; the flag was added by later trackers.
This code path never fires for authentic ST3 files, but fires for 16-bit samples from
any tracker that sets bit 2 of the sample flags.

**Fix:**
```c
if ((flags & 2) == 0) conv_u16m_to_s16m(s_smp[i], len >> 1);
```

The matching read path in `S3M_GetNewSample` already does this correctly:
```c
S3M_SampleLength[channel] = *(uint32_t *)(s_ins[sample_nr] + 0x10)
                             >> (sample_bits + sample_stereo);
```
where `sample_bits = 1` for 16-bit → right-shifts by 1 = divides bytes by 2.

---

## BUG-S2 — CRITICAL: Minimum file-size check is 64 bytes; header needs 96

**File:** `src/core/format/s3m.c`, `s3m_workspace_bytes` and `s3m_load`.

**Code:**
```c
if (size < 64) return 0;   // s3m_workspace_bytes
if (size < 64) return -1;  // s3m_load
```

**Root cause:** The packed header struct `s3m_header_t` covers bytes 0x00–0x3F
(64 bytes, ending at `special_pointer`). Immediately after the check, the loader
accesses:

```c
s_chansets = s_buf + 0x40;   // bytes 0x40–0x5F: channelSettings (32 bytes)
s_order    = s_buf + 0x60;   // bytes 0x60+: order list
```

A file with exactly 64–95 bytes passes the check but produces an out-of-bounds read
on `s_chansets`. A file with 96 bytes but `ord_num > 0` will OOB-read the order
list. The bare minimum to safely parse the header is **96 bytes** plus at least
`ord_num` order bytes.

**st3play cross-check:** st3play (`load.c`) does
`mread(&song.header, sizeof(song.header), 1, f)` where `song.header` includes
`channel[32]` (channelSettings). If the read is short, `meof(f)` is true immediately
after and the loader aborts. The fxplayer struct is smaller and the protection is
absent.

**Fix:**
```c
if (size < 96) return 0;   // s3m_workspace_bytes
if (size < 96) return -1;  // s3m_load
```

A stricter check would also verify `size >= 96 + hdr->ord_num` before accessing
`s_order`, but `ord_num < 2` is already rejected so 96 bytes catches all practical
truncated inputs.

---

## BUG-S3 — CRITICAL: `init_speed` and `init_tempo` not validated

**File:** `src/core/format/s3m.c` in `s3m_load`.

**Code:**
```c
S3M_tempo = hdr->init_tempo;
S3M_speed = hdr->init_speed;
```

Both assigned unconditionally, no validation.

### Sub-case A — `init_speed == 0`: infinite hang

In `s3m_render_block`:
```c
S3M_tick++;
if (S3M_tick == S3M_speed) { S3M_tick = 0; /* advance row */ }
else                        { S3M_TickEffect(); }
```

With `S3M_speed = 0`: `S3M_tick` starts at 0, increments to 1, 2, 3 … and the
condition `S3M_tick == 0` is never true again. **The row sequencer never advances.**

### Sub-case B — `init_speed == 255`: 255 ticks per row instead of default 6

ST3 stored 255 in `initspeed` as a sentinel meaning "keep the previous session's
speed." A fresh load should fall back to the default. Setting `S3M_speed = 255`
causes extremely slow playback (255 ticks between row advances).

### Sub-case C — `init_tempo == 0`: division by zero crash

`s3m_calc_speed(S3M_tempo, g_MixSpeed)` computes:
```c
((mix_speed * 10u) << 2) / (tempo * 4u)
```
With `tempo = 0` this is integer division by zero — undefined behaviour, typically
SIGFPE on x86.

Note: st3play is safe with `settempo(0)` because its BPM lookup table uses
`bpm = (i == 0) ? 1 : i` — index 0 maps to bpm=1 internally. The fxplayer formula
divides directly by `tempo * 4`, so 0 is a hard crash.

**st3play cross-check:** `dig.c` lines 82–90:
```c
if (song.header.inittempo != 0)  settempo(song.header.inittempo);
else                             settempo(125);    // default when header says 0

if (song.header.initspeed != 255) setspeed(song.header.initspeed);
else                              setspeed(6);     // default when header says 255

// setspeed() also guards:
void setspeed(uint8_t val) { if (val > 0) song.musicmax = val; }  // silently ignores 0
```

**Fix:**
```c
if (hdr->init_speed != 0 && hdr->init_speed != 255)
    S3M_speed = hdr->init_speed;
if (hdr->init_tempo != 0)
    S3M_tempo = hdr->init_tempo;
```

---

> **SOURCE CONFLICT — BUG-S3 — init_tempo range:**
>
> MultimediaWiki says: "Initial tempo — if **less than 33**, it is ignored and the
> previous value used when you loaded the song is used instead."
>
> st3play (`dig.c`) only ignores `inittempo == 0` and applies any value 1–255
> otherwise. Values 1–32 would produce a very slow tick rate (not ignored). The
> `< 33` guard exists only for the **Txx runtime effect** in SB Pro mode
> (`settempo`: `if (soundcardtype == SBPRO && bpm <= 0x20) return`), not at load
> time and not for GUS mode.
>
> The fxplayer's existing `if (info < 0x21) break` guard in `S3M_GlobalEffect`
> (Txx runtime) applies unconditionally regardless of sound card type, which matches
> MultimediaWiki but diverges from st3play's GUS-mode behaviour.
>
> **Neither fxplayer nor st3play enforces the `< 33` floor at load time.** If the
> MultimediaWiki is correct for the initial-tempo field, the fix in sub-case C should
> be extended to: `if (hdr->init_tempo >= 33) S3M_tempo = hdr->init_tempo;`

---

## BUG-S4 — MEDIUM: `Dxy` with both nibbles 1–E not handled in `S3M_TickEffect`

**File:** `src/core/effect/efc_s3m.c`, `S3M_TickEffect`, case 4.

**Code:**
```c
case 4:  /* Dxy: volume slide */
    if (x == 0)
        S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], y);
    else if (y == 0)
        S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], x);
    break;
```

When both `x` and `y` are 1–E neither branch is taken and volume is unchanged on
every non-zero tick.

**Correct behaviour (MultimediaWiki + st3play confirmed):** `Dxy` with both nibbles
non-zero is treated as `D0y` — slide down by `y` on every non-zero tick.

**st3play cross-check:** `digcmd.c` `s_volslide` (handles both tick 0 and tick > 0
via `song.musiccount`):
```c
else if (song.fastvolslide || song.musiccount > 0)
{
    if (infolo == 0)  ch->avol += infohi;
    else              ch->avol -= infolo;   // fires when both nibbles non-zero
}
else { return; }  // tick-0 without fast slides: illegal, do nothing
```

On tick > 0 with both nibbles non-zero: `avol -= infolo`. fxplayer is missing this
case.

**Fix:**
```c
case 4:
    if (x == 0)
        S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], y);
    else if (y == 0)
        S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], x);
    else
        S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], y);
    break;
```

The `S3M_RowEffect` handler (tick 0) is already correct for this case — it does
nothing, matching the "illegal slide" return in st3play.

---

## BUG-S5 — MEDIUM: `Qxy` retrigger counter never reset when effect is absent

**File:** `src/core/effect/efc_s3m.c`, effect 17 handling.

`S3M_RetrigCount[channel]` is only reset in `S3M_initEffects` (called once at song
load). When a row has no effect the counter is never touched. A leftover non-zero
count from a previous Qxy row keeps decrementing on subsequent ticks, potentially
firing spurious retriggers.

**st3play cross-check:** `digcmd.c` `docmd1` — when `ch->cmd == 0` (no effect on
channel):
```c
else  // cmd == 0
{
    ch->atrigcnt = 0;   // counter reset
    ...
}
```

Additionally, when `ch->cmd == 4` (volume slide D, which is `'D'-64`):
```c
if (ch->cmd == 'D'-64)
{
    ch->atrigcnt = 0;   // also reset here, comment says "fix trigger D"
    ...
}
```

**Fix:** reset `S3M_RetrigCount[channel] = 0` when `S3M_Effect[channel] != 17`
in `S3M_GetNewEffect` or at the start of `S3M_read_row` after effects are latched.
To match st3play exactly, also reset it when `S3M_Effect[channel] == 4` (volume
slide), and do NOT reset it for other non-Qxy effects.

---

> **SOURCE CONFLICT — BUG-S5 — Qxy reset condition:**
>
> MultimediaWiki says: "The counter is reset (without retriggering) when a row
> **without the Qxx effect** is encountered in the channel." This implies reset on
> any row where the effect is not Qxy — including rows with other effects like Exx,
> Gxx, Hxx, etc.
>
> st3play resets `atrigcnt` only in two cases:
> 1. `cmd == 0` (no effect at all)
> 2. `cmd == 4` (volume slide D) — labelled "fix trigger D" in the source,
>    probably an artefact of the original ASM code structure
>
> For rows where a channel has, say, Fxx (slide up) after a row with Qxy, st3play
> does **not** reset the counter. MultimediaWiki says it should. The fix above
> follows st3play behaviour because it is the ground-truth implementation; the
> MultimediaWiki may be wrong or may be describing an ideal that ST3.21 didn't
> actually implement.

---

## BUG-S6 — MEDIUM: `Vxx` fires on tick 0; should fire on tick > 0 only

**File:** `src/core/effect/efc_s3m.c`, `S3M_GlobalEffect`, case 22.

`S3M_GlobalEffect()` is called from `S3M_GetNewEffect()` → `S3M_read_row()`, which
runs at the start of each row when `S3M_tick` has just reset to 0. Vxx therefore
fires on **tick 0**.

**st3play cross-check:** `digcmd.c` jump tables:
```c
soncejmp[22]  = s_ret;       // tick=0: Vxx is a no-op
sotherjmp[22] = s_setgvol;   // tick>0: Vxx fires
```

**Practical impact:**

1. At speed > 1: the global volume change fires one tick early — the first
   tick-length of audio uses the new volume instead of the old one.
2. At speed == 1: fxplayer incorrectly applies Vxx (fires on tick 0 = the only
   tick). In st3play there are no tick > 0 events at speed=1, so Vxx does nothing.
   MultimediaWiki: "The effect doesn't do anything if the current speed is 1."

**Fix:** move `case 22` out of `S3M_GlobalEffect` (tick-0 path) and into
`S3M_TickEffect` (tick > 0 path). Since `s_setgvol` sets a fixed value,
idempotent repeated application on every tick > 0 is harmless.

---

> **SOURCE CONFLICT — BUG-S6 — "tick 1" vs "all tick > 0":**
>
> MultimediaWiki says: "This effect **is actually processed on tick 1** (that is the
> second tick) of the row."
>
> st3play fires Vxx on **every tick > 0** (`sotherjmp` is called for every non-zero
> tick, not just tick 1). Because setting global volume to a fixed value is
> idempotent, the audible result is identical whether it fires on tick 1 only or on
> all ticks > 0. MultimediaWiki's "tick 1" description is functionally accurate but
> technically imprecise — it describes the first observable moment, not that later
> ticks are excluded. Both agree on the speed=1 no-op behaviour.

---

## BUG-S7 — LOW: Active volume clamps at 64 instead of 63

**File:** `src/core/effect/efc_s3m.c`, `S3M_addVolume` and `S3M_decVolume`.

```c
static uint8_t S3M_addVolume(int16_t vol, int16_t val)
{
    vol += val;
    if (vol > 64) vol = 64;  // BUG: should be 63
    ...
}
```

**st3play cross-check:** all effect handlers use `CLAMP(ch->avol, 0, 63)`. Pattern
column volume is capped in `digread.c` `donewnote`:
```c
if (ch->vol != 255 && ch->vol > 63) ch->vol = 63;
```

**Fix:**
```c
if (vol > 63) vol = 63;   // in S3M_addVolume and S3M_decVolume
```

Also: in `S3M_read_row`, after global-volume scaling the pattern column volume can
reach 64: `(64 * 64 + 32) / 64 == 64`. Clamp `S3M_Volume[channel]` to 63 after
that assignment.

---

> **SOURCE CONFLICT — BUG-S7 — scope of the 63 cap:**
>
> MultimediaWiki says: "**Volumes actually peak at 63**, and not 64. Setting the
> volume to 64 will actually make it go to 63."  This is stated generally, implying
> it applies to all volume values everywhere.
>
> st3play draws a narrower line:
> - `avol` (active/effect-modified channel volume) — capped at 63 throughout
>   (`CLAMP(avol, 0, 63)`)
> - Instrument default volume (`ins->vol`) — allowed up to 64:
>   `if (ins->vol > 64) ins->vol = 64;` in `load.c checkins`
> - Global volume (`Vxx`, `setglobalvol`) — allowed up to 64:
>   `if ((uint8_t)vol > 64) vol = 64;` in `dig.c`
>
> So st3play enforces 63 only for the running channel volume that effects modify,
> not for the stored baseline values. The MultimediaWiki quote likely refers to the
> same active-volume context. The fix above is consistent with st3play. If the wider
> interpretation is correct, the caps on instrument vol and global vol should also be
> lowered to 63, which would change mixing output.

---

## BUG-S8 — LOW: `Bxx` with `info == 0xFF` can OOB-read the order table

**File:** `src/core/effect/efc_s3m.c`, `S3M_GlobalEffect`, case 2.

```c
case 2:  /* Bxx: jump to order */
    ...
    S3M_nextorder = info;   // info == 0xFF → nextorder = 255
```

In `S3M_goRowOrder`: `s_order[255]` is accessed. The order table has at most 255
entries (indices 0–254), so index 255 is one past the end — an OOB read.

**st3play cross-check:** `digcmd.c` `s_jmpto`:
```c
if (ch->info == 0xFF)
    song.breakpat = 255;   // end-of-song sentinel, np_ord is not touched
else { song.breakpat = 1; song.jmptoord = ch->info; }
```

`B00FF` is treated as an end-of-song marker.

**Fix:**
```c
case 2:
    if (jumpbreak) break;
    if (info == 0xFF) {
        s3m_mark_looped();
        s_dat_ready = 2;
        break;
    }
    if ((uint32_t)info <= S3M_Order) s3m_mark_looped();
    S3M_nextorder = info;
    S3M_nextrow   = 0;
    S3M_jump      = 1;
    jumpbreak     = 1;
    break;
```

---

## Not a bug — `SCx` with `x == 0` is already correctly ignored

`S3M_TickEffect` is only entered when `S3M_tick >= 1` (the render loop increments
`S3M_tick` before the comparison). The condition `if (S3M_tick == y)` with `y == 0`
therefore never fires. SC0 is silently ignored, which matches st3play:

```c
// st3play s_notecut (tick=0): sets anotecutcnt to info & 0xF = 0 for SC0
// st3play s_notecutb (tick>0): if (anotecutcnt > 0) → false for 0 → nothing
```

Both players treat SC0 as a no-op.

---

## Source conflicts not tied to current fxplayer bugs

These are places where MultimediaWiki and st3play disagree but fxplayer does not have
a specific bug related to the conflict yet. They are recorded here so they can be
revisited if st3play is found to have errors.

---

### SC-1 — Tremor (Ixy) counter persistence between rows

**MultimediaWiki:** "The 'on' and 'off' counters are **never reset**, except in the
tremor update procedure described above. Scream Tracker doesn't even reset them on
playback start. **Only on tracker startup** are they reset."

**st3play** (`digcmd.c` `docmd1`): when any command **other than Ixy** is present
on a channel:
```c
if (ch->cmd != 'I'-64)   // 'I'-64 = 9 = tremor effect
{
    ch->atremor = 0;
    ch->atreon  = true;
}
```

`atremor` (the countdown within the current on/off phase) and `atreon` (the on/off
phase flag) are reset to their initial state whenever the channel has any non-tremor
command. Setting `atreon = true; atremor = 0` restarts the tremor cycle from the
beginning of the on-phase.

**Conflict:** MultimediaWiki says these counters persist row-to-row; st3play resets
them on any non-Ixy row. If a module uses tremor on row N, then has a note or pitch
effect on row N+1, then returns to tremor on row N+2, the two implementations will
disagree on where the tremor cycle resumes.

**fxplayer status:** `TremorCountOn` and `TremorCountOff` are only reset in
`S3M_initEffects` (song load). When Ixy is absent on a row, neither counter is
touched — fxplayer follows the MultimediaWiki interpretation, which contradicts
st3play.

---

### SC-2 — Qxy counter also reset on volume slide D (st3play only)

Already noted in BUG-S5, extracted here for visibility.

**st3play** resets `atrigcnt = 0` not only when `cmd == 0` (no effect) but also
when `cmd == 4` (volume slide D). This is labelled "fix trigger D" in the source
and is probably an artefact of the original ASM code where a register cleared by the
D handler happened to be the same one used by Qxy.

**MultimediaWiki** does not mention this — it only says the counter resets when
"a row without the Qxx effect is encountered."

Neither fxplayer nor the MultimediaWiki specification accounts for this D-effect
reset. Implementing it as-is from st3play is recommended for accuracy; understanding
why requires digging into the original ASM.

---

### SC-3 — Cxy: BCD nibble validation vs row-range validation

**MultimediaWiki:** "Jump to row x×10 + y. The value provided is in **decimal**. If
the row number specified **is 64 or higher**, the effect is ignored."  Only a
row-range check is specified.

**st3play** (`digcmd.c` `s_break`):
```c
const uint8_t hi = ch->info >> 4;
const uint8_t lo = ch->info & 0x0F;
if (hi <= 9 && lo <= 9)
{
    song.startrow = (hi * 10) + lo;
    song.breakpat = 1;
}
```

st3play validates that **both nibbles are valid decimal digits (0–9)**. If either
nibble is A–F the entire command is ignored, regardless of the resulting row number.

**Conflict:** for `C3A` (hi=3, lo=0xA), MultimediaWiki computes row = 40 (valid,
< 64 → apply), st3play ignores (lo > 9). For `C6A` (row = 70 ≥ 64) both agree the
command is ignored — but for different reasons.

**fxplayer** (`S3M_GlobalEffect` case 3) computes the BCD row and stores it without
nibble validation. The later `S3M_nextrow <= 63` guard in `S3M_goRowOrder` catches
the row-range case, but does not reject non-decimal nibbles. fxplayer follows
MultimediaWiki, not st3play, for inputs with hex nibbles.

In practice, valid ST3 tracker output never produces non-decimal nibbles in Cxy, so
this conflict only affects malformed or adversarially crafted files.

---

## Verification plan

### Cross-check against other open-source players

st3play is the primary reference but is not infallible — it could carry over bugs
from the original ST3 ASM, or contain 8bitbubsy's own fixes that deviate from the
true original. Every finding marked with a SOURCE CONFLICT callout should be
cross-checked against at least one other player that has accessible source code
before any conflicting fix is committed.

**Players with available source to check:**

| Player | Language | Notes |
|--------|----------|-------|
| [OpenMPT](https://github.com/OpenMPT/openmpt) | C++ | The most thoroughly documented compatible tracker; `soundlib/Load_s3m.cpp`, `soundlib/Snd_fx.cpp`. Has explicit compat flags per tracker version. |
| [libxmp](https://github.com/libxmp/libxmp) | C | `src/loaders/s3m_load.c`, `src/effects.c`. Conservative, well-tested against many edge cases. |
| [Schism Tracker](https://github.com/schismtracker/schismtracker) | C | Derived from the same ST3 codebase family as st3play; useful second data point for effects. |
| [ft2play](https://github.com/8bitbubsy/ft2play) | C | Same author as st3play; compare handling philosophy between the two replayers. |
| [MilkyTracker](https://github.com/milkytracker/MilkyTracker) | C++ | Has a separate S3M replay path (`src/tracker/s3m/`). |

For each SOURCE CONFLICT, the goal is to find at least two independent players that
agree — that consensus is then more reliable than any single source alone.

---

### Test S3M files to build

Each of the following should be a minimal hand-crafted S3M (one instrument, one or
two channels, as few rows as needed) that isolates exactly the behaviour under
investigation. The file should produce a clearly audible or measurable difference
between the two interpretations, so it can be loaded into **DOSBox-X running the
original Scream Tracker 3.21** and the output compared against fxplayer and st3play.

Where the issue is about a WAV render difference (not a crash/hang), the test
methodology is: render the file in DOSBox-X via `FX.EXE -w:test.wav`, render with
fxplayer, render with st3play, `sha256sum` all three and note which players agree.

| Test file | Targets | What to listen/check for |
|-----------|---------|--------------------------|
| `test-dxy-both.s3m` | BUG-S4 | Two channels: one with `D34` (both nibbles non-zero), one with `D04` (reference). Confirm the `D34` channel slides down on non-zero ticks like `D04`. |
| `test-qxy-noeffect.s3m` | BUG-S5 | Row 0: `Q22`. Row 1: no effect (channel silent). Row 2: `Q22`. Measure whether the retrigger phase is reset at row 2 or continues from where it left off at row 0. |
| `test-qxy-deffect.s3m` | SC-2 | Row 0: `Q22`. Row 1: `D04` (volume slide). Row 2: `Q22`. Verify whether the Dxy row resets the retrigger counter as st3play does. |
| `test-qxy-othereffect.s3m` | BUG-S5 / SC-2 | Row 0: `Q22`. Row 1: `F04` (slide up — neither no-effect nor Dxy). Row 2: `Q22`. Resolves whether the MultimediaWiki "any row without Q" or st3play's "cmd==0 or D only" is correct. |
| `test-vxx-speed1.s3m` | BUG-S6 | Speed 1. Row 0: note at volume 64. Row 1: `V00` (global vol 0). Row 2: same note. Expected: if Vxx fires on tick 0, row 2 is silent; if Vxx doesn't fire at speed 1, row 2 is at vol 64. |
| `test-vxx-timing.s3m` | BUG-S6 | Speed 6. `V00` on a row that also has a note. Compare first tick of the row — is the note played at the old or new global volume? |
| `test-tremor-interrupt.s3m` | SC-1 | Row 0: note + `I24` (tremor). Row 1: same note, no effect. Row 2: same note + `I24`. Verify whether the tremor cycle at row 2 restarts (st3play) or continues mid-cycle (MultimediaWiki). |
| `test-cxy-hexnibble.s3m` | SC-3 | Pattern order: rows end with `C3A`. Does the song jump to row 40 (MultimediaWiki) or ignore the effect (st3play)? |
| `test-bxx-ff.s3m` | BUG-S8 | Single-order song with a `BFF` at row 0. Verify whether ST3 treats it as end-of-song or attempts to jump to order 255. |
| `test-volume63.s3m` | BUG-S7 | Instrument at default volume 64 with `D0F` (fine slide down) on the first row. After the slide, does the starting volume read as 63 or 64? |
| `test-init-speed0.s3m` | BUG-S3 | `initspeed = 0` in the header. Should be ignored and default speed used; used to verify fxplayer fix doesn't hang. Load only — do not render in DOSBox (unknown original behaviour; original may also hang). |
| `test-init-tempo-low.s3m` | BUG-S3 / SC (init_tempo) | `inittempo = 20` (below 33). Verify what ST3 actually does with a sub-33 initial tempo: ignores it (MultimediaWiki) or uses it (st3play)? |

Human-readable output is preferred for test files where possible (e.g. a note that
pulses at a known rate so the on/off timing can be counted by ear or by waveform
inspection). For cursor-only effects like Qxy retrigger count, rendering to WAV and
comparing peak positions is more reliable than listening.

---

## Summary table

| ID | Severity | Location | Issue | st3play ref |
|----|----------|----------|-------|-------------|
| S1 | CRITICAL | `s3m.c` sample conv | `conv_u16m_to_s16m(smp, len)` — byte count, corrupts memory past sample | `load.c` 8-bit XOR only |
| S2 | CRITICAL | `s3m.c` size check | `size < 64` insufficient; `s_chansets`+`s_order` need 96 bytes minimum | `meof()` after full header read |
| S3 | CRITICAL | `s3m.c` init | `init_speed==0` → hang; `==255` → wrong default; `init_tempo==0` → div-by-zero | `dig.c` lines 82–90 |
| S4 | MEDIUM | `efc_s3m.c` TickEffect | `Dxy` both nibbles 1–E: no slide on non-zero ticks | `s_volslide` else branch |
| S5 | MEDIUM | `efc_s3m.c` retrig | Qxy counter not reset when effect absent | `docmd1` `atrigcnt=0` on `cmd==0` and `cmd==4` |
| S6 | MEDIUM | `efc_s3m.c` Vxx | Vxx fires on tick 0; should only fire on tick > 0 | `soncejmp[22]=s_ret`, `sotherjmp[22]=s_setgvol` |
| S7 | LOW | `efc_s3m.c` volume | Active volume clamps at 64, should be 63 | `CLAMP(avol, 0, 63)` throughout |
| S8 | LOW | `efc_s3m.c` Bxx | `Bxx` with `info=0xFF` OOB-reads order table | `s_jmpto` `breakpat=255` sentinel |

| Conflict | Sources | Status |
|----------|---------|--------|
| SC-1 | Tremor counter persistence: MultimediaWiki says never reset; st3play resets on non-Ixy cmd | fxplayer follows MultimediaWiki (counters persist) |
| SC-2 | Qxy also resets on volume slide D in st3play only | fxplayer doesn't do this; MultimediaWiki doesn't mention it |
| SC-3 | Cxy: BCD nibble validation (st3play) vs row-range only (MultimediaWiki) | fxplayer follows MultimediaWiki |
| BUG-S3 extra | init_tempo < 33 ignored (MultimediaWiki) vs only 0 ignored (st3play) | neither player enforces < 33 at load time |
| BUG-S5 extra | Qxy resets on any non-Qxy row (MultimediaWiki) vs cmd==0 or D only (st3play) | fxplayer resets on neither |
| BUG-S6 extra | Vxx on "tick 1" (MultimediaWiki) vs "all tick>0" (st3play) | functionally identical; both agree on speed=1 no-op |
| BUG-S7 extra | All volumes cap at 63 (MultimediaWiki) vs only avol caps (st3play) | fix follows st3play (avol only) |
