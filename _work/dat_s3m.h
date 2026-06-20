// Dat_S3M.h for F/X Player
// (C) 1997-98 by Apollo / STIGMA

#ifndef __DAT_S3M_H
#define __DAT_S3M_H

#define  S3M_Maxchannels  32

// Variables for effects

extern unsigned long  S3M_NotePeriodes[12];

extern unsigned long  S3M_OrderNum, S3M_InstrumentNum, S3M_PatternNum;

extern unsigned long   S3M_channels;          // how many channels are used
extern unsigned char   S3M_RowBuffer[S3M_Maxchannels*5];

extern unsigned long   S3M_row;
extern unsigned long   S3M_jump;
extern unsigned long   S3M_nextrow;
extern unsigned long   S3M_nextorder;
extern unsigned long   S3M_tick;
extern unsigned long   S3M_Order;
extern unsigned long   S3M_Pattern;

extern unsigned long   S3M_TickLength;        // how many samples to make one frame
//extern unsigned long   S3M_buffer_rest;
//extern unsigned long   S3M_tick_rest;

extern unsigned char   S3M_flag_stereo;        // 0 = mono, 1 = stereo S3M
extern unsigned char   S3M_flag_amigalimits;   // 0 = no amiga limits
extern unsigned char   S3M_flag_st3volslides;  // 1 = volume slide on EVERY frame
extern unsigned char   S3M_flag_note;


extern unsigned char   S3M_tempo;            // tempo of S3M
extern unsigned char   S3M_speed;              // speed of S3M
extern unsigned char   S3M_GlobalVolume;      // global volume of S3M, 64 = max

extern unsigned char   S3M_ChannelActiv[S3M_Maxchannels];       //  0 != can be active

extern unsigned char   S3M_SampleNr[S3M_Maxchannels];
extern unsigned long   S3M_SampleAddress[S3M_Maxchannels];
extern unsigned long   S3M_SampleLength[S3M_Maxchannels];
extern unsigned long   S3M_SamplePosition[S3M_Maxchannels];
extern unsigned long   S3M_SampleFraction[S3M_Maxchannels];
extern unsigned long   S3M_SampleLoopBegin[S3M_Maxchannels];
extern unsigned long   S3M_SampleLoopEnd[S3M_Maxchannels];
extern unsigned long   S3M_SampleC4SPD[S3M_Maxchannels];
extern unsigned char   S3M_SampleFlags[S3M_Maxchannels];

extern unsigned char   S3M_Volume[S3M_Maxchannels];
extern unsigned char   S3M_Panning[S3M_Maxchannels];
extern unsigned char   S3M_Note[S3M_Maxchannels];
extern unsigned char   S3M_Octave[S3M_Maxchannels];
extern unsigned char   S3M_Effect[S3M_Maxchannels];
extern unsigned char   S3M_EffectInfo[S3M_Maxchannels];
extern unsigned char   S3M_LastEffect[S3M_Maxchannels];
extern unsigned char   S3M_LastEffectInfo[S3M_Maxchannels];

extern unsigned long   S3M_Periode[S3M_Maxchannels];
extern signed   long   S3M_PeriodeAdjust[S3M_Maxchannels];
extern unsigned long   S3M_Frequence[S3M_Maxchannels];

// Functions

long load_S3M ( char * FileName );
long close_S3M ();

void unpack_firstRow ( unsigned long PatternAddress );
void S3M_unpack_row( unsigned long PatternNr, unsigned long RowNr );

unsigned long Calc_st3periode ( unsigned long Note, unsigned long Octave, unsigned long C4SPD );

void S3M_GetNewSample ( unsigned long channel );
void S3M_GetNewNote ( unsigned long channel );
void S3M_read_row ();
void S3M_goRowOrder();
void S3M_initvariables();

void S3M_to_Mixer ();
void Mixer_to_S3M ();

void interrupt_S3M ();

void FF_S3M ();
void FR_S3M ();

extern "C"
{
}
#endif
