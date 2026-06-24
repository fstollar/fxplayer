#ifndef FX_EFFECT_EFC_MOD_H
#define FX_EFFECT_EFC_MOD_H

#include <stdint.h>
#include "format/mod.h"

extern uint32_t MOD_GlissPeriod[MOD_MAXCHANNELS];
extern uint16_t MOD_VibratoPosition[MOD_MAXCHANNELS];
extern uint16_t MOD_TremoloVolume[MOD_MAXCHANNELS];

/* Reset per-channel effect state (vibrato position, gliss, tremolo) to defaults. */
void MOD_initEffects(void);

/* Process song-global effects that apply before per-channel work. */
void MOD_GlobalEffect(void);

/* Prepare per-channel state at the start of each tick, before row/tick effect processing. */
void MOD_beforeEffect(void);

/* Process row-triggered effects — called once on tick 0 of each row. */
void MOD_RowEffect(void);

/* Process continuous per-tick effects (vibrato, portamento, volume slide) — called every tick. */
void MOD_TickEffect(void);

/* Decode one row from the current pattern and dispatch notes/effects to all channels. */
void MOD_read_row(void);

/* Store and prepare effect parameters from the current row for all active channels. */
void MOD_GetNewEffect(void);

#endif /* FX_EFFECT_EFC_MOD_H */
