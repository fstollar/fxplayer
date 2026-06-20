#ifndef FX_EFFECT_EFC_MOD_H
#define FX_EFFECT_EFC_MOD_H

#include <stdint.h>
#include "format/mod.h"

extern uint32_t MOD_GlissPeriode[MOD_MAXCHANNELS];
extern uint16_t MOD_VibratoPosition[MOD_MAXCHANNELS];
extern uint16_t MOD_TremoloVolume[MOD_MAXCHANNELS];

void MOD_initEffects(void);
void MOD_GlobalEffect(void);
void MOD_beforeEffect(void);
void MOD_RowEffect(void);
void MOD_TickEffect(void);
void MOD_read_row(void);
void MOD_GetNewEffect(void);

#endif /* FX_EFFECT_EFC_MOD_H */
