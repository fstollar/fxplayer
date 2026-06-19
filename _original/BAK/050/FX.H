// F/X Player global header file
// most of it are global, public variables
// (C) 1997 by Apollo / STIGMA

#ifndef __FX_H
#define __FX_H

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#define uns unsigned
#define sig signed

// *************************** Global variables *****************************

// *** Souncard variables

extern unsigned short CardType, SubCardType, DSPVersion;
extern unsigned short Adress, IRQ, DMAlo, DMAhi, DMA;
extern unsigned short flag_16bit, flag_stereo, flag_flip, flag_loop;
extern unsigned short Option;
extern unsigned long MixSpeed;

// *** DMA buffer variables

extern unsigned long block_length, buffer_length;
extern __segment DMAselector;
extern char *DMApointer;

extern unsigned short MixerType;
extern unsigned char Mixer_Block;
extern char *Mixerpointer, *Mixerpointer1, *Mixerpointer2;
extern char *MixerBlock;
extern unsigned long MixerAdress;

// *** Soundfile varaibles

extern unsigned short DatType, Last_DatType;
extern unsigned char flag_DatReady;

extern char FileName[255];

// *** dummys :) ***

extern long dummy, test;


extern "C"
{
}
#endif
