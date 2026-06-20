# Known bugs & quirks

This file documents bugs — both in the **original 1998 DOS source** (`_original/`,
preserved verbatim) and discovered during the C99 port — plus how the port
handles them.

The C99 core is validated by **bit-exact SHA-256** comparison against WAV
renders produced by the original DOS build (`FX.EXE` / `FX2.EXE`) running in
DOSBox-X. See `tests/render-dosbox.sh` and `tests/reference_renders/`.

---

## Original DOS player bugs

### O-1: S3M out-of-bounds note-period reads (latent, can crash)

**Where:** `_original/EFC_S3M.CPP` arpeggio (`Jxy`, effect 10) and
`_original/DAT_S3M.CPP` `Calc_st3periode`.

**What:** `Calc_st3periode(note, octave, c4spd)` indexes
`S3M_NotePeriodes[note]` — an array of only **12** entries. Several paths feed
out-of-range `note` values:

- Uninitialised channels start at `S3M_Note = 0xFF` (255). An arpeggio effect
  on a channel that never received a note reads `S3M_NotePeriodes[255]`.
- `S3M_Note = value & 15` (note assignment) permits notes 12–15.
- In `S3M_RowEffect` the arpeggio case has **no** `note -= 12` octave-wrap
  guard (unlike the tick-effect case), so even valid arpeggios can exceed 11.
- `c4spd` can be 0 for an empty/未-loaded sample, giving a divide-by-zero.

**Why it "works" on DOS:** real-mode DOS has no memory protection, so the
out-of-bounds read returns adjacent garbage instead of faulting, and the result
only feeds `S3M_PeriodeAdjust` for channels that are usually inactive — so it
rarely affects audible output. A zero divisor *would* fault (INT 0), but the
affected modules never hit that path with `c4spd == 0` on the original.

**Trigger:** `stars.s3m` (in `tests/_test_mods/`). The original `FX.EXE`
**crashes/hangs** on it — the DOS WAV render is left with an **unpatched RIFF
header** (`data_size == 0`), proving `closeWAV()` never ran (DOSBox had to be
killed). Renders of 30 s and 60 s caps produce byte-identical truncated output,
so the failure is deterministic, not a timeout.

**Port handling:** the C99 core (`core/format/s3m.c`, `core/effect/efc_s3m.c`)
adds bounds/zero guards so it never reads out of range or divides by zero.
Where the original would read garbage, the port returns a safe period — this is
*more* robust than the original. Up to the frame where the DOS player died,
the C99 output of `stars.s3m` is **bit-identical** to the DOS capture.

---

### O-2: `calcMasterVolume32` soft-clip ramp leaves `test` uninitialised past `2*val`

**Where:** `_original/AMPTABLE.CPP` `calcMasterVolume32`.

**What:** the soft-clip ramp loop is:

```c
for (counter = val; counter < 8192; counter++) {
    if (counter < (2*val))
        test = (4*val - counter) * (MasterVolume / (3*val));
    if (test < 128) test = 128;          // note: no else above
    table[counter]   = test;
    table[counter-1] = test;
}
```

There is **no `else`** on the inner `if`. Once `counter >= 2*val`, `test`
retains its last computed value (≈ ⅔·MasterVolume, floored at 128) for the rest
of the table — it is *not* reset to 128. This is almost certainly unintended,
but it defines the reference output, so the port must reproduce it exactly.

**Symptom when mis-ported:** an early C99 version added `else test = 128;`,
zeroing the high end of the soft-clip table. Quiet/mono S3M modules never index
that far into the table, so they stayed bit-exact (masking the bug). Loud
modules that hit large mix magnitudes diverged — `purple.669`
(MasterVolume 12288, 8 channels) first differed at frame 436857, where the DOS
output saturates to 32767 but the mis-ported table produced small values.

**Port handling:** `core/mixer/mixer_scalar.c` `mixer_calc_master_vol32`
reproduces the original exactly — `test` persists across iterations, no `else`.

---

## Port quirks (not bugs, but easy to trip over)

### P-1: DOS 8.3 filename truncation in the render harness

`tests/render-dosbox.sh` mounts a host dir and runs `FX.EXE <module>` inside
DOSBox. DOS truncates long names to 8.3, so a module called
`spacedel_short.mod` is invisible to `fopen` inside DOS → `load_MOD` fails and
the WAV comes out as a bare 44-byte header. Use 8.3-safe names
(`debris.mod`, `hul.mod`, `purple.669`) for reference renders.

### P-2: Misaligned 16-bit reads in the S3M loader

`s3m_load` builds instrument/sample/pattern pointer tables from little-endian
parapointers that follow an **odd-length** order list (`ord_num` can be odd).
The original cast straight to `uint16_t*` (fine on x86). On stricter targets
this is a misaligned access (UBSAN flagged it); the port now reads those
fields byte-wise. Behaviour is unchanged on x86, correct everywhere.

### P-3: Master-volume soft-clip table symmetry

`mixer_calc_master_vol32` (port of `calcMasterVolume32` in
`_original/AMPTABLE.CPP`) fills the soft-clip ramp at `table[counter]` **and**
`table[counter-1]` (one slot below, staying on the positive-index side) — *not*
`table[-counter]`. An early port mistakenly mirrored to the negative side,
which only showed up on **stereo, hard-panned** modules (mono/centre modules
were unaffected, masking the bug). Fixed to match the original exactly.

---

## Status of test modules

| Module            | Format | Result                                              |
|-------------------|--------|-----------------------------------------------------|
| TEST.S3M          | S3M    | bit-exact ✓                                         |
| 64mania.s3m       | S3M    | bit-exact ✓                                         |
| rising.s3m        | S3M    | bit-exact ✓                                         |
| star.s3m          | S3M    | bit-exact ✓                                         |
| starship.s3m      | S3M    | bit-exact ✓                                         |
| trip.s3m          | S3M    | bit-exact ✓                                         |
| stars.s3m         | S3M    | bit-exact up to DOS crash point (see O-1)           |
| debris.mod        | MOD 4ch| bit-exact ✓                                         |
| hul.mod           | MOD 8ch| bit-exact ✓                                         |
| purple.669        | 669    | bit-exact ✓ (after O-2 fix)                          |
