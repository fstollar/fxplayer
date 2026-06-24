#ifndef FX_EFFECT_EFC_669_H
#define FX_EFFECT_EFC_669_H

#include <stdint.h>
#include "format/m669.h"

extern uint32_t M669_GlissPeriod[M669_CHANNELS];
extern uint8_t  M669_GlissFlag[M669_CHANNELS];
extern uint16_t M669_VibratoPosition[M669_CHANNELS];

/* Reset per-channel effect state (vibrato position, gliss) to defaults. */
void M669_initEffects(void);

/* Process song-global effects that apply before per-channel work. */
void M669_GlobalEffect(void);

/* Prepare per-channel state at the start of each tick, before row/tick effect processing. */
void M669_beforeEffect(void);

/* Process row-triggered effects — called once on tick 0 of each row. */
void M669_RowEffect(void);

/* Process continuous per-tick effects (vibrato, portamento) — called every tick. */
void M669_TickEffect(void);

/* Decode one row from the current pattern and dispatch notes/effects to all channels. */
void M669_read_row(void);

/* Store and prepare effect parameters from the current row for all active channels. */
void M669_GetNewEffect(void);

#endif /* FX_EFFECT_EFC_669_H */
