#ifndef FX_EFFECT_EFC_S3M_H
#define FX_EFFECT_EFC_S3M_H

#include <stdint.h>
#include "format/s3m.h"

extern uint32_t S3M_GlissPeriod[S3M_MAXCHANNELS];
extern uint16_t S3M_VibratoPosition[S3M_MAXCHANNELS];
extern uint16_t S3M_TremorVolume[S3M_MAXCHANNELS];

/* Reset per-channel effect state (vibrato position, gliss, tremor) to defaults. */
void S3M_initEffects(void);

/* Process song-global effects (tempo change, global volume) that apply before per-channel work. */
void S3M_GlobalEffect(void);

/* Prepare per-channel state at the start of each tick, before row/tick effect processing. */
void S3M_beforeEffect(void);

/* Process row-triggered effects (note cut, delay) — called once on tick 0 of each row. */
void S3M_RowEffect(void);

/* Process continuous per-tick effects (vibrato, portamento, volume slide) — called every tick. */
void S3M_TickEffect(void);

#endif
