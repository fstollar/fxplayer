// Dat_S3M.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __DAT_S3M_H
#define __DAT_S3M_H

// Variables for effects

extern unsigned short  S3M_NotePeriodes[12];

extern unsigned short  S3M_OrderNum, S3M_InstrumentNum, S3M_PatternNum;

extern unsigned short  S3M_channels;          // how many channels are used
extern unsigned char   S3M_RowBuffer[32*5];

extern unsigned short  S3M_row;
extern unsigned short  S3M_jump;
extern unsigned short  S3M_nextrow;
extern unsigned short  S3M_nextorder;
extern unsigned short  S3M_tick;
extern unsigned short  S3M_Order;
extern unsigned short  S3M_Pattern;

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

extern unsigned char   S3M_ChannelActiv[32];       //  0 != can be active

extern unsigned char   S3M_SampleNr[32];
extern unsigned long   S3M_SampleAddress[32];
extern unsigned long   S3M_SampleLength[32];
extern unsigned long   S3M_SamplePosition[32];
extern unsigned long   S3M_SampleFraction[32];
extern unsigned long   S3M_SampleLoopBegin[32];
extern unsigned long   S3M_SampleLoopEnd[32];
extern unsigned long   S3M_SampleC4SPD[32];
extern unsigned char   S3M_SampleFlags[32];

extern unsigned char   S3M_Volume[32];
extern unsigned char   S3M_Panning[32];
extern unsigned char   S3M_Note[32];
extern unsigned char   S3M_Octave[32];
extern unsigned char   S3M_Effect[32];
extern unsigned char   S3M_EffectInfo[32];
extern unsigned char   S3M_LastEffect[32];
extern unsigned char   S3M_LastEffectInfo[32];

extern unsigned long   S3M_Periode[32];
extern signed   long   S3M_PeriodeAdjust[32];
extern unsigned long   S3M_Frequence[32];

// Functions

short load_S3M ( char * FileName );
short close_S3M ();

void unpack_firstRow ( unsigned long PatternAddress );
void S3M_unpack_row( unsigned short PatternNr, unsigned short RowNr );

unsigned long Calc_st3periode ( uns char Note, uns char Octave, unsigned long C4SPD );

void GetNewSample ( unsigned short channel );
void GetNewNote ( unsigned short channel );
void S3M_read_row ();
void S3M_goRowOrder();
void S3M_initvariables();

void S3M_to_Mixer ();
void Mixer_to_S3M ();

void interrupt_S3M ();


extern "C"
{
}
#endif
