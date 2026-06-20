// Dat_669.h for F/X Player
// (C) 1998 by Apollo / STIGMA

#ifndef __DAT_669_H
#define __DAT_669_H

#define  _669_channels  8

// Variables for effects

extern unsigned long   _669_samples;
extern unsigned long   _669_pattern;

extern unsigned char   _669_OrderNum;
extern          char  *_669_Orders;

extern unsigned long _669_Periodes[16][72];


// *** 669 runtime variables ***

extern unsigned char   _669_RowBuffer[_669_channels*5];         // buffer where row is uncompressed
extern unsigned long   _669_LastPattern;    // no last pattern
extern unsigned long   _669_LastRow;        // no last row exists

extern unsigned long   _669_TickLength;         // how many samples to make one frame

extern unsigned long   _669_row;
extern unsigned long   _669_jump;
extern unsigned long   _669_tick;
extern unsigned long   _669_Order;
extern unsigned long   _669_Pattern;

extern unsigned long   _669_buffer_rest;
extern unsigned long   _669_tick_rest;

extern unsigned char   _669_tempo;            // tempo of 669
extern unsigned char   _669_speed;              // speed of 669

extern unsigned char   _669_ChannelActiv[_669_channels];       //  0 != can be active

extern unsigned char   _669_SampleNr[_669_channels];
extern unsigned long   _669_SampleAddress[_669_channels];
extern unsigned long   _669_SampleLength[_669_channels];
extern unsigned long   _669_SamplePosition[_669_channels];
extern unsigned long   _669_SampleFraction[_669_channels];
extern unsigned char   _669_SampleLoop[_669_channels];
extern unsigned long   _669_SampleLoopBegin[_669_channels];
extern unsigned long   _669_SampleLoopEnd[_669_channels];
extern unsigned long   _669_SampleFinetune[_669_channels];

extern unsigned char   _669_Volume[_669_channels];
extern unsigned char   _669_Panning[_669_channels];
extern unsigned char   _669_Note[_669_channels];
extern unsigned char   _669_Effect[_669_channels];
extern unsigned char   _669_EffectInfo[_669_channels];
extern unsigned char   _669_LastEffect[_669_channels];
extern unsigned char   _669_LastEffectInfo[_669_channels];

extern unsigned long   _669_Periode[_669_channels];
extern signed   long   _669_PeriodeAdjust[_669_channels];
extern unsigned long   _669_Frequence[_669_channels];

// Functions

long load_669 ( char * FileName );
long close_669 ();

void _669_initvariables();
void _669_unpack_row( unsigned long PatternNr, unsigned long RowNr );

unsigned long Calc_AMIGAperiode ( unsigned long Note, unsigned long Finetune );

void _669_GetNewSample ( unsigned long channel );
void _669_GetNewNote ( unsigned long channel );
void _669_read_row ();
void _669_goRowOrder();

void _669_to_Mixer ();
void Mixer_to_669 ();

void interrupt_669 ();

void FF_669 ();
void FR_669 ();

extern "C"
{
}
#endif
