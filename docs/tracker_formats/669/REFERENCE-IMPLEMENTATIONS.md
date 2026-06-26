# 669 Reference Implementations

## Primary references for format behaviour

There is no public source for the original Composer 669 or Unis 669 trackers, so
there is no "st3play equivalent" for this format. Five independent, well-maintained
reimplementations were surveyed; their consensus positions are used as the reference
for fxplayer bug classification.

---

### libxmp (`src/loaders/669_load.c`)

[libxmp repository](https://github.com/libxmp/libxmp)

- Accepts both `if` and `JN` magic
- Order list length: scans until `order[i] > sfh.nop`
- Loop sentinel: `loopend >= 0xfffff → lpe = 0; loop = (lpe != 0)`
- Break validation: `if (pbrk >= 64) return -1`
- Effect 3 → `FX_669_FINETUNE` — **finetune table index** (minority position)
- Effects 6 and 7 silently dropped
- Effect carry: not implemented (events are written once per cell; no carry state)
- Effect cancel on zero: not implemented
- Initial speed/BPM: `spd = 6, bpm = 78`

---

### OpenMPT (`soundlib/Load_669.cpp`)

[OpenMPT repository](https://github.com/OpenMPT/openmpt)

- Accepts both `if` and `JN` magic
- Header validation: rejects `tempoList == 0`, `tempoList > 15`, `breaks >= 64`, `restartPos >= 128`
- Loop detection: `if (loopEnd > length && loopStart == 0) → no loop`
- Effect 3 → `CMD_PORTAMENTOUP | 0xF0` — **fine pitch portamento up** (majority position)
- Effect 4 vibrato: `param |= (param << 4)` (both nibbles set from single data nibble)
- Effect 6: only sub-params 0 (slide left → `0x4F`) and 1 (right → `0xF4`) handled
- Effect carry: implemented via `effect[chan]` array; resets on new note
- Effect cancel on zero: not explicitly implemented
- Respects `restartPos` for loop restart
- Initial speed/BPM: `speed = 4, bpm = 78`

---

### Open Cubic Player (`playgmd/gmdl669.c`)

[OCP repository](https://github.com/mywave82/opencubicplayer)
Original author: Niklas Beisert (1994), maintained by Stian Skjelstad

- Accepts both `if` and `JN` magic
- Loop detection: `(loopend <= length) ? LOOP : 0` — sentinel handled implicitly
- Effect 3 → `cmdRowPitchSlideUp, data<<2` — **row-level pitch shift** (majority position);
  source comment reads `/* correct? down? both? */`
- Effect 4 vibrato: `(data<<4)|1` — same scaling as fxplayer
- Effect 6: only sub-params 0 and 1 handled
- **Effect carry**: implemented — `commands[chan]` array persists across empty cells;
  resets on new note (`b0 < 0xFE`)
- **Effect cancel on zero**: implemented — `if (!data[j]) commands[j] = 0xFF`
- Initial speed/BPM: `cmdSpeed=78` (once at song start), `cmdTempo=patspeed[pat]` per pattern

---

### Schism Tracker (`fmt/669.c`)

[Schism Tracker repository](https://github.com/schismtracker/schismtracker)
Source comment: "This is better than IT's and MPT's 669 loaders"

- Accepts both `if` and `JN` magic
- Header validation: `restartpos > 127 → reject`; `breaks[i] > 0x3f → reject`
- Loop detection: `if (tmplong > sample.length) tmplong = 0; else CHN_LOOP`
- Effect 3 → `FX_PORTAMENTOUP | 0xf0` — **fine pitch portamento up** (majority position);
  source comment `/* D - frequency adjust (??) */`; additionally `effect[chan] = 0xFF`
  after handling (effect 3 does **not** carry across subsequent empty cells)
- Effect 4 vibrato → `FX_ARPEGGIO` with `param |= (param<<4)` (arpeggio approximation)
- Effect 5 (speed): `effect[chan] = 0xFF` after handling — speed does not carry
- Effect 6: sub-params 0 (`0x4F`) and 1 (`0xF4`) only
- **Effect carry**: implemented — `effect[chan]` array; resets on new note
- **Effect cancel on zero**: implemented — `if ((b[2] & 0x0f) == 0 && b[2] != 0x30)`
  cancels all effects with zero data, *except* effect 3 data-0 (`0x30`)
- Respects `restartpos` for loop restart (`csf_insert_restart_pos`)
- Initial speed/BPM: `initial_speed = 4, initial_tempo = 78`

---

### MikMod (`libmikmod/loaders/load_669.c`)

[MikMod/sezero repository](https://github.com/sezero/mikmod)

- Accepts both `if` and `JN` magic
- Header validation: `tempos[i] == 0 || tempos[i] > 32 → reject`; `breaks[i] > 0x3f → reject`
- Loop detection: `loopend >= 0xfffff → loopend = 0`; then `loopstart < loopend → LOOP`
- Effect 3 → `UNI_S3MEFFECTF | (0xF0 | effect)` — **extra-fine portamento up** (majority
  position); source comment `/* DMP converts this effect to S3M FF1. Why not? */`
- Effect 4 vibrato → ProTracker vibrato (`UniPTEffect(0x4, effect)`)
- **Effect carry**: implemented — `lastfx`/`lastval` per channel; when `c == 0xFF`:
  `c = lastfx, effect = lastval`; resets on new note (`a < 0xFE`)
- Effect cancel on zero: not implemented (zero-data effects still apply)
- Speed validation: rejects `tempo == 0` or `tempo > 32` (wider range than OpenMPT's 15)
- Respects `looporder` for restart position: `reppos = looporder < numpos ? looporder : 0`
- Initial speed/BPM: `initspeed = 4, inittempo = 78`

---

## Consensus positions across all five references

| Question | libxmp | OpenMPT | OCP | Schism | MikMod | fxplayer |
|----------|--------|---------|-----|--------|--------|----------|
| Effect 3 semantics | finetune index | pitch portamento | row pitch shift | pitch portamento | pitch portamento | **finetune index** |
| Effect carry on empty cells | no | yes | yes | yes | yes | **no** |
| Effect cancel on zero data | no | no | yes | yes | no | **no** |
| Break validation (`>= 64` → reject) | yes | yes | — | yes | yes | **no** (BUG-L3) |
| Speed 0 rejected at load | no | yes | — | — | yes | **no** (BUG-L4) |
| Speed range upper limit | — | 1–15 | — | — | 1–32 | **none** |
| Restart position field used | — | yes | — | yes | yes | **no** |
| Default ticks per row | 6 | 4 | — | 4 | 4 | **6** |

---

## `_original/DAT_669.CPP` and `_original/EFC_669.CPP` — prior DOS code

`_original/` contains the author's own earlier DOS F/X Player implementation of the
669 player. It is **not** the original Composer 669 or Unis 669 tracker; it is simply
the predecessor to the current C99 port and shares the same author.

The C99 port was written with reference to `_original/`, so differences between
the two are porting notes. They do not override the five external references above.

### Notable differences between `_original/` and the C99 port

| Behaviour | `_original/` | C99 port |
|-----------|-------------|----------|
| `addPeriode`/`decPeriode` floor | commented out (no clamp) | clamps to 10 — prevents div-by-zero |
| Instrument bounds in `unpack_row` | `inst > nos` (allows equal) | `inst >= nos` (stricter) |
| Pattern bounds in `unpack_row` | `PatternNr > nop` (allows equal) | `pat_nr >= nop` (stricter) |
| Sample bounds in `GetNewSample` | `SampleNr <= nos` without secondary guard | `sn >= nos` early-return added |

### Behaviours shared by both `_original/` and the C99 port

| Behaviour | `_original/` location | Port location |
|-----------|-----------------------|---------------|
| Rejects `JN` magic | `DAT_669.CPP:276` | `m669.c:225` |
| Song end restarts at order 0 (loop-restart byte unused) | `DAT_669.CPP:582-584` | `m669.c:491-494` |
| Effects do not carry across empty cells | `DAT_669.CPP:532-534` | `m669.c:447-454` |
| Loop sentinel `!= 0x000fffff` | `DAT_669.CPP:613` | `m669.c:530` |
| Default volume 16 on instrument load | `DAT_669.CPP:616` | `m669.c:532` |
| Vibrato depth `(Info << 4) + 1` | `EFC_669.CPP:253` | `efc_669.c:159` |
| Effect 3 = finetune table index | `EFC_669.CPP:249` | `efc_669.c:155` |
| Speed/break lists indexed by pattern number | `DAT_669.CPP:489-490` | `m669.c:414-415` |
| Effect cancel on zero: documented, not implemented | `EFC_669.CPP:685-700` | `efc_669.c` |
