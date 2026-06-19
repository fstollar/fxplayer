#ifndef FX_EFFECT_EFC_S3M_H
#define FX_EFFECT_EFC_S3M_H

#include <stdint.h>
#include "format/s3m.h"

extern uint32_t S3M_GlissPeriode[S3M_MAXCHANNELS];
extern uint16_t S3M_VibratoPosition[S3M_MAXCHANNELS];
extern uint16_t S3M_TremorVolume[S3M_MAXCHANNELS];

void S3M_initEffects(void);
void S3M_GlobalEffect(void);
void S3M_beforeEffect(void);
void S3M_RowEffect(void);
void S3M_TickEffect(void);

#endif
