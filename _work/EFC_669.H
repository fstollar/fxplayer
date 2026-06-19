// Efc_669.h for F/X Player
// (C) 1998 by Apollo / STIGMA

#ifndef __EFC_669_H
#define __EFC_669_H

extern unsigned long  _669_GlissPeriode[_669_channels];
extern unsigned char  _669_GlissFlag[_669_channels];
extern unsigned short _669_VibratoPosition[_669_channels];

void _669_initEffects();
void _669_GlobalEffect();
void _669_beforeEffect();
void _669_RowEffect();
void _669_TickEffect();

extern "C"
{
}
#endif
