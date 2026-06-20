// Efc_S3M.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __EFC_S3M_H
#define __EFC_S3M_H

extern unsigned long  S3M_GlissPeriode[S3M_Maxchannels];
extern unsigned short S3M_VibratoPosition[S3M_Maxchannels];
extern unsigned short S3M_TremorVolume[S3M_Maxchannels];

void S3M_initEffects();
void S3M_GlobalEffect();
void S3M_beforeEffect();
void S3M_RowEffect();
void S3M_TickEffect();


extern "C"
{
}
#endif
