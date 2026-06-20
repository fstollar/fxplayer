#ifndef FX_EFFECT_EFC_669_H
#define FX_EFFECT_EFC_669_H

#include <stdint.h>
#include "format/m669.h"

extern uint32_t M669_GlissPeriode[M669_CHANNELS];
extern uint8_t  M669_GlissFlag[M669_CHANNELS];
extern uint16_t M669_VibratoPosition[M669_CHANNELS];

void M669_initEffects(void);
void M669_GlobalEffect(void);
void M669_beforeEffect(void);
void M669_RowEffect(void);
void M669_TickEffect(void);
void M669_read_row(void);
void M669_GetNewEffect(void);

#endif /* FX_EFFECT_EFC_669_H */
