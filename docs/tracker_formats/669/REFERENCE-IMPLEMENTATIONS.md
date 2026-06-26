# 669 Reference Implementations

## Primary reference: `_original/DAT_669.CPP` + `_original/EFC_669.CPP`

The original F/X Player DOS source (Apollo / STIGMA, 1998) is the ground truth for
the port. Both files are in `_original/` and must not be modified.

`m669.c` and `efc_669.c` are mechanical translations of these files. When the port
and any external reference diverge, the original DOS source is the tie-breaker.

## Secondary references (cross-check material)

### libxmp (Composer 669 loader)

- **Source**: `src/loaders/669_load.c` in the
  [libxmp repository](https://github.com/libxmp/libxmp)
- **Format struct names**: `c669_file_header`, `c669_instrument_header`
- **Key decisions**:
  - Accepts both `if` and `JN` magic (fxplayer and original accept `if` only)
  - Order list length: scans until `order[i] > sfh.nop`
  - Loop sentinel: `loopend >= 0xfffff → lpe = 0; flg = lpe ? LOOP : 0`
  - Effect 3 mapped to `FX_669_FINETUNE` (confirms port's finetune-index interpretation)
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

## Original DOS source findings (key cross-checks)

The following behaviors in the original `_original/` DOS source are faithfully
reproduced in the port — they should **not** be changed without confirming against
the original:

| Behavior | Original location | Port location |
|----------|-------------------|---------------|
| Rejects `JN` magic (only accepts `if`) | `DAT_669.CPP:276` `*(short*)Pointer != 0x6669` | `m669.c:225` |
| Song end restarts at order 0 (not at `_669_OrderNum`) | `DAT_669.CPP:582-584` | `m669.c:491-494` |
| `_669_OrderNum` is stored but never used for restart | `DAT_669.CPP:286` | `m669.c:259` |
| Effects do NOT carry across empty cells (0xFF byte 2) | `DAT_669.CPP:532-534` — effect set to 0xFF | `m669.c:447-454` |
| Loop sentinel `!= 0x000fffff` (exact value) | `DAT_669.CPP:613` | `m669.c:530` |
| Default sample volume 16 (above the 4-bit note-cell max) | `DAT_669.CPP:616` | `m669.c:532` |
| Vibrato depth formula `(Info << 4) + 1` | `EFC_669.CPP:253` | `efc_669.c:159` |
| Effect 3 = set finetune table index | `EFC_669.CPP:249` | `efc_669.c:155` |
| Speed and break lists indexed by pattern number | `DAT_669.CPP:489-490` | `m669.c:414-415` |
| `addPeriode`/`decPeriode` minimum clamp at 10 (commented out in original) | `EFC_669.CPP:82,98` — commented | `efc_669.c:44,51` — active |

The clamp-at-10 deviation is intentional: the original has no clamp (code commented
out), relying on a post-hoc check in `_to_Mixer` to deactivate the channel. The port
adds an explicit clamp to prevent division by zero in the frequency calculation. This
changes the behaviour slightly (channel stays active at period=10 vs. deactivated), but
avoids undefined behaviour that would crash on modern hardware.
