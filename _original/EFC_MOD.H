// Efc_MOD.h for F/X Player
// (C) 1998 by Apollo / STIGMA

#ifndef __EFC_MOD_H
#define __EFC_MOD_H

extern unsigned long  MOD_GlissPeriode[MODchannel];
extern unsigned short MOD_VibratoPosition[MODchannel];
extern unsigned short MOD_TremoloVolume[MODchannel];

void MOD_initEffects();
void MOD_GlobalEffect();
void MOD_beforeEffect();
void MOD_RowEffect();
void MOD_TickEffect();

extern "C"
{
}
#endif
