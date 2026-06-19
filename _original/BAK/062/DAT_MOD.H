// Dat_MOD.h for F/X Player
// (C) 1998 by Apollo / STIGMA

#ifndef __DAT_MOD_H
#define __DAT_MOD_H

#define  MODchannel  8

// Variables for effects

extern unsigned long   MOD_channels;
extern unsigned long   MOD_samples;
extern unsigned long   MOD_pattern;

extern unsigned char   MOD_OrderNum;
extern          char  *MOD_Orders;

extern unsigned long MOD_Periodes[16][84];


// *** MOD runtime variables ***

extern unsigned char   MOD_RowBuffer[MODchannel*5];         // buffer where row is uncompressed
extern unsigned long   MOD_LastPattern;    // no last pattern
extern unsigned long   MOD_LastRow;        // no last row exists

extern unsigned long   MOD_TickLength;         // how many samples to make one frame

extern unsigned long   MOD_row;
extern unsigned long   MOD_jump;
extern unsigned long   MOD_nextrow;
extern unsigned long   MOD_nextorder;
extern unsigned long   MOD_tick;
extern unsigned long   MOD_Order;
extern unsigned long   MOD_Pattern;

extern unsigned long   MOD_buffer_rest;
extern unsigned long   MOD_tick_rest;

extern unsigned char   MOD_flag_amigalimits;   // 0 = no amiga limits
extern unsigned char   MOD_flag_note;

extern unsigned char   MOD_tempo;            // tempo of MOD
extern unsigned char   MOD_speed;              // speed of MOD

extern unsigned char   MOD_ChannelActiv[MODchannel];       //  0 != can be active

extern unsigned char   MOD_SampleNr[MODchannel];
extern unsigned long   MOD_SampleAddress[MODchannel];
extern unsigned long   MOD_SampleLength[MODchannel];
extern unsigned long   MOD_SamplePosition[MODchannel];
extern unsigned long   MOD_SampleFraction[MODchannel];
extern unsigned char   MOD_SampleLoop[MODchannel];
extern unsigned long   MOD_SampleLoopBegin[MODchannel];
extern unsigned long   MOD_SampleLoopEnd[MODchannel];
extern unsigned long   MOD_SampleFinetune[MODchannel];

extern unsigned char   MOD_Volume[MODchannel];
extern unsigned char   MOD_Panning[MODchannel];
extern unsigned char   MOD_Note[MODchannel];
extern unsigned char   MOD_Effect[MODchannel];
extern unsigned char   MOD_EffectInfo[MODchannel];
extern unsigned char   MOD_LastEffect[MODchannel];
extern unsigned char   MOD_LastEffectInfo[MODchannel];

extern unsigned long   MOD_Periode[MODchannel];
extern signed   long   MOD_PeriodeAdjust[MODchannel];
extern unsigned long   MOD_Frequence[MODchannel];

// Functions

long load_MOD ( char * FileName );
long close_MOD ();

void MOD_initvariables();
void MOD_unpack_row( unsigned long PatternNr, unsigned long RowNr );

unsigned long Calc_AMIGAperiode ( unsigned char Note, unsigned long Finetune );

void MOD_GetNewSample ( unsigned long channel );
void MOD_GetNewNote ( unsigned long channel );
void MOD_read_row ();
void MOD_goRowOrder();

void MOD_to_Mixer ();
void Mixer_to_MOD ();

void interrupt_MOD ();

extern "C"
{
}
#endif
