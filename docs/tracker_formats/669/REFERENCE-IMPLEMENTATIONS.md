# 669 Reference Implementations

## Primary references for format behaviour

There is no public source for the original Composer 669 or Unis 669 trackers, so
there is no "st3play equivalent" for this format. The primary references are two
independent, well-maintained reimplementations:

### libxmp (Composer 669 loader)

- **Source**: `src/loaders/669_load.c` in the
  [libxmp repository](https://github.com/libxmp/libxmp)
- **Format struct names**: `c669_file_header`, `c669_instrument_header`
- **Key decisions**:
  - Accepts both `if` and `JN` magic
  - Order list length: scans until `order[i] > sfh.nop`
  - Loop sentinel: `loopend >= 0xfffff → lpe = 0; flg = lpe ? LOOP : 0`
  - Effect 3 mapped to `FX_669_FINETUNE` (agrees with fxplayer's finetune-index interpretation)
  - Effects 6 and 7 silently dropped (`MSN(ev[2]) >= ARRAY_SIZE(fx) → continue`)
  - Break validation: `if (pbrk >= 64) return -1`
  - Initial speed: `mod->spd = 6`, tempo: `mod->bpm = 78` (matches fxplayer)

### OpenMPT (Load_669.cpp)

- **Source**: `soundlib/Load_669.cpp` in the
  [OpenMPT repository](https://github.com/OpenMPT/openmpt)
- **Key decisions**:
  - Accepts both `if` and `JN` magic
  - Header validation rejects `tempoList[i] == 0` and `tempoList[i] > 15`
  - Header validation rejects `breaks[i] >= 64`
  - Validates `restartPos >= 128` as invalid
  - Initial speed: `Order().SetDefaultSpeed(4)` (differs from fxplayer/libxmp which use 6)
  - Loop detection: `if (loopEnd > length && loopStart == 0) → no loop`
  - Effect 3 (Dxx) translated to `CMD_PORTAMENTOUP | 0xF0` — a fine pitch portamento,
    **not** a finetune table index (SOURCE CONFLICT with libxmp and fxplayer)
  - Effect 4 (Exx) vibrato param duplicated: `param |= (param << 4)`
  - Effect 6 (G): only handles sub-param 0 (slide left) and 1 (slide right)

## `_original/DAT_669.CPP` and `_original/EFC_669.CPP` — prior DOS code

`_original/` contains the author's own earlier DOS F/X Player implementation of the
669 player, written in Watcom C++. It is **not** the original Composer 669 or Unis
669 tracker, and it is **not** a ground truth for 669 format behaviour — it is
simply the predecessor to the current C99 port and shares the same author.

The C99 port (`m669.c`, `efc_669.c`) was written with reference to `_original/`,
so behavioural differences between the two are meaningful as porting notes — but
they do not override libxmp or OpenMPT when those sources disagree with `_original/`.

### Notable differences between `_original/` and the C99 port

| Behaviour | `_original/` | C99 port |
|-----------|-------------|----------|
| `addPeriode`/`decPeriode` floor | commented out (no clamp) | clamps to 10 — prevents div-by-zero in frequency calculation |
| Instrument bounds check in `unpack_row` | `inst > _669_samples` (allows equal) | `inst >= M669_samples` (stricter, correct) |
| Pattern bounds check in `unpack_row` | `PatternNr > _669_patterns` (allows equal) | `pat_nr >= M669_patterns` (stricter, correct) |
| Sample bounds check in `GetNewSample` | `SampleNr <= _669_samples` (allows OOB) | `sn >= M669_samples` early-return guard added |

### Behaviours shared by both `_original/` and the C99 port

These are consistent between the two versions; they are documented here so reviewers
know not to treat them as porting errors:

| Behaviour | `_original/` location | Port location |
|-----------|-----------------------|---------------|
| Rejects `JN` magic (accepts `if` only) | `DAT_669.CPP:276` | `m669.c:225` |
| Song end restarts at order 0 (not at `OrderNum`) | `DAT_669.CPP:582-584` | `m669.c:491-494` |
| `OrderNum` stored but never used for song-end restart | `DAT_669.CPP:286` | `m669.c:259` |
| Effects do not carry across empty cells (0xFF) | `DAT_669.CPP:532-534` | `m669.c:447-454` |
| Loop sentinel `!= 0x000fffff` (exact equality) | `DAT_669.CPP:613` | `m669.c:530` |
| Default sample volume 16 on instrument load | `DAT_669.CPP:616` | `m669.c:532` |
| Vibrato depth formula `(Info << 4) + 1` | `EFC_669.CPP:253` | `efc_669.c:159` |
| Effect 3 = set finetune table index (0–15) | `EFC_669.CPP:249` | `efc_669.c:155` |
| Speed and break lists indexed by pattern number | `DAT_669.CPP:489-490` | `m669.c:414-415` |
| Effect-cancellation-on-zero rule: present in spec, absent in code | `EFC_669.CPP:685-700` (empty switch) | `efc_669.c` (not implemented) |
