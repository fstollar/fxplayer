// MOD - Routines for F/X Player v1.0
// (c) 1998 by Apollo / STIGMA

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <conio.h>

#include "fx.h"

//#include "irq.h"
//#include "dma.h"

//#include "device.h"
//#include "dev_sb.h"

//#include "format.h"
#include "dat_calc.h"
//#include "dat_wav.h"
#include "dat_mod.h"
#include "efc_mod.h"

#include "convert.h"
#include "mixer.h"
#include "amptable.h"


// *** Variables for MOD's and headers ***

FILE                 *MOD_handle = NULL;
static char          *MOD_Buffer = NULL;
static unsigned long  MOD_length = 0;

static unsigned long *InstrumentPointer = NULL, *SamplePointer = NULL, *PatternPointer = NULL;

static char MOD_id[8][5] = {
                           "M.K.", "FLT4", "FLT6", "FLT8", "4CHN", "6CHN",
                           "8CHN", "M!K!"
                         };
static long MOD_sampleNr[8] = {
                              31 ,    31 ,    31 ,    31 ,    31 ,    31 ,
                              31 ,    31
                            };
static long MOD_channelNr[8] = {
                               4 ,     4 ,     6 ,     8 ,     4 ,     6 ,
                               8 ,     4
                            };

unsigned long   MOD_channels = 4;
unsigned long   MOD_samples  = 15;
unsigned long   MOD_patterns = 0;

unsigned char   MOD_OrderNum;
         char  *MOD_Orders;

// *** MOD runtime variables ***

unsigned char   MOD_RowBuffer[MODchannel*5];         // buffer where row is uncompressed
unsigned long   MOD_LastPattern = 65535;    // no last pattern
unsigned long   MOD_LastRow = 65535;        // no last row exists

unsigned long   MOD_TickLength = 0;         // how many samples to make one frame

unsigned long   MOD_row = 0;
unsigned long   MOD_jump = 0;
unsigned long   MOD_nextrow = 0;
unsigned long   MOD_nextorder = 0;
unsigned long   MOD_tick = 0;
unsigned long   MOD_Order = 0;
unsigned long   MOD_Pattern = 0;

unsigned long   MOD_buffer_rest = 0;
unsigned long   MOD_tick_rest = 0;

unsigned char   MOD_flag_amigalimits = 0;   // 0 = no amiga limits
unsigned char   MOD_flag_note = 0;

unsigned char   MOD_tempo = 128;            // tempo of MOD
unsigned char   MOD_speed = 6;              // speed of MOD

unsigned char   MOD_ChannelActiv[MODchannel];       //  0 != can be active

unsigned char   MOD_SampleNr[MODchannel];
unsigned long   MOD_SampleAddress[MODchannel];
unsigned long   MOD_SampleLength[MODchannel];
unsigned long   MOD_SamplePosition[MODchannel];
unsigned long   MOD_SampleFraction[MODchannel];
unsigned char   MOD_SampleLoop[MODchannel];
unsigned long   MOD_SampleLoopBegin[MODchannel];
unsigned long   MOD_SampleLoopEnd[MODchannel];
unsigned long   MOD_SampleFinetune[MODchannel];

unsigned char   MOD_Volume[MODchannel];
unsigned char   MOD_Panning[MODchannel];
unsigned char   MOD_Note[MODchannel];
unsigned char   MOD_Effect[MODchannel];
unsigned char   MOD_EffectInfo[MODchannel];
unsigned char   MOD_LastEffect[MODchannel];
unsigned char   MOD_LastEffectInfo[MODchannel];

unsigned long   MOD_Periode[MODchannel];
signed   long   MOD_PeriodeAdjust[MODchannel];
unsigned long   MOD_Frequence[MODchannel];

// ********************** Periodes of MOD ************************

unsigned long MOD_Periodes[16][84] = {
// Tuning 0, Normal
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      856,808,762,720,678,640,604,570,538,508,480,453, // C-1 to B-1
      428,404,381,360,339,320,302,285,269,254,240,226, // C-2 to B-2
      214,202,190,180,170,160,151,143,135,127,120,113, // C-3 to B-3
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 1
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      850,802,757,715,674,637,601,567,535,505,477,450, // same as above
      425,401,379,357,337,318,300,284,268,253,239,225, // but with
      213,201,189,179,169,159,150,142,134,126,119,113, // finetune +1
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 2
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      844,796,752,709,670,632,597,563,532,502,474,447, // etc,
      422,398,376,355,335,316,298,282,266,251,237,224, // finetune +2
      211,199,188,177,167,158,149,141,133,125,118,112,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 3
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      838,791,746,704,665,628,592,559,528,498,470,444,
      419,395,373,352,332,314,296,280,264,249,235,222,
      209,198,187,176,166,157,148,140,132,125,118,111,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 4
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      832,785,741,699,660,623,588,555,524,495,467,441,
      416,392,370,350,330,312,294,278,262,247,233,220,
      208,196,185,175,165,156,147,139,131,124,117,110,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 5
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      826,779,736,694,655,619,584,551,520,491,463,437,
      413,390,368,347,328,309,292,276,260,245,232,219,
      206,195,184,174,164,155,146,138,130,123,116,109,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 6
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      820,774,730,689,651,614,580,547,516,487,460,434,
      410,387,365,345,325,307,290,274,258,244,230,217,
      205,193,183,172,163,154,145,137,129,122,115,109,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 7
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      814,768,725,684,646,610,575,543,513,484,457,431,
      407,384,363,342,323,305,288,272,256,242,228,216,
      204,192,181,171,161,152,144,136,128,121,114,108,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -8
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      907,856,808,762,720,678,640,604,570,538,508,480,
      453,428,404,381,360,339,320,302,285,269,254,240,
      226,214,202,190,180,170,160,151,143,135,127,120,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -7
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      900,850,802,757,715,675,636,601,567,535,505,477,
      450,425,401,379,357,337,318,300,284,268,253,238,
      225,212,200,189,179,169,159,150,142,134,126,119,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -6
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      894,844,796,752,709,670,632,597,563,532,502,474,
      447,422,398,376,355,335,316,298,282,266,251,237,
      223,211,199,188,177,167,158,149,141,133,125,118,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -5
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      887,838,791,746,704,665,628,592,559,528,498,470,
      444,419,395,373,352,332,314,296,280,264,249,235,
      222,209,198,187,176,166,157,148,140,132,125,118,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -4
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      881,832,785,741,699,660,623,588,555,524,494,467,
      441,416,392,370,350,330,312,294,278,262,247,233,
      220,208,196,185,175,165,156,147,139,131,123,117,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -3
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      875,826,779,736,694,655,619,584,551,520,491,463,
      437,413,390,368,347,328,309,292,276,260,245,232,
      219,206,195,184,174,164,155,146,138,130,123,116,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -2
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      868,820,774,730,689,651,614,580,547,516,487,460,
      434,410,387,365,345,325,307,290,274,258,244,230,
      217,205,193,183,172,163,154,145,137,129,122,115,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -1
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      862,814,768,725,684,646,610,575,543,513,484,457,
      431,407,384,363,342,323,305,288,272,256,242,228,
      216,203,192,181,171,161,152,144,136,128,121,114,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
     };

/*
struct MOD_struct
 {
  char Song_name[20];                     // Offset 0, length 20
 };                                       // together = ?? bytes

struct MOD_Instrument                  // 15 or 31 instrument one after
 {                                     // the other
  char            Inst_name[22];          // Offset 20, length 22
  unsigned short  Inst_length;            // Offset 42, length 2
  char            Inst_finetune;          // Offset 44, length 1
  char            Inst_volume;            // Offset 45, length 1
  unsigned short  Inst_loopstart;         // Offset 46, length 2
  unsigned short  Inst_loopend;           // Offset 48, length 2
 };                                    // next intrument at Offset 50 ...
*/

// *** Functions for loading and preparing MOD ***

static unsigned short amiga_to_x86 ( unsigned short value)
{
return ( ((value & 0x00ff) << 8) + (value >> 8) );
}


static long open_file_MOD ( char *FileName )
{
 if ( (MOD_handle=fopen(FileName,"rb") )==NULL )
      {
       return 1;
      }

return 0;
}


static long close_file_MOD (void)
{
 if (MOD_handle == NULL)
    return 1;

 fclose ( MOD_handle );
 MOD_handle = NULL;

return 0;
}


long load_MOD ( char *FileName )
{
 static unsigned long channel;
 static char *Pointer;
 static long counter, test, value;

 flag_DatReady = 0;

 test = open_file_MOD ( FileName );     // open MOD file
 if (test != 0)  return 1;              // could not open file

// *** load MOD complete into memory ***

 fseek ( MOD_handle, 0L, SEEK_END );    // go to end of MOD
 MOD_length = ftell ( MOD_handle );     // read file pointer eg. length of MOD
 fseek ( MOD_handle, 0L, SEEK_SET );    // and go back to start

 MOD_Buffer = (char *) malloc ( MOD_length+0x10 );   // alloc memory for MOD
 if ( MOD_Buffer == NULL )                      // if not enough memory free
    {
     close_file_MOD ();
     return 2;                          // not enough memory to load MOD
    }

 if ( (fread( MOD_Buffer , 1, MOD_length, MOD_handle )) != MOD_length )   // read MOD
     return 3;                          // corrupt MOD

// ****************** some precalc to make it playable **********************

 *(char *)(MOD_Buffer+19) = 0;  // terminate song string in all cases !

 for (counter = 0; counter < 16; counter++)
     for (test = 0; test < 12; test++)
         {
          MOD_Periodes[counter][test] = MOD_Periodes[counter][test+24] * 4;
          MOD_Periodes[counter][test+12] = MOD_Periodes[counter][test+24] * 2;
          MOD_Periodes[counter][test+60] = (MOD_Periodes[counter][test+48] + 1) / 2;
          MOD_Periodes[counter][test+72] = (MOD_Periodes[counter][test+48] + 2) / 4;
         }

// ************************** Test id of MOD ********************************

 Pointer = (char *)(MOD_Buffer + 1080);
 MOD_channels = 4; MOD_samples = 15;

 for (test = 0; test < 8; test++)
     {
      if ( !memcmp(Pointer , MOD_id[test] , 4) )  // which MOD type is it ?
         {
          printf("ID: %s\n", MOD_id[test]);
          MOD_channels = MOD_channelNr[test];   // load channel number
          MOD_samples  = MOD_sampleNr[test];    // load sample number
         }
     }

// * Set pointers in MOD for Orders etc. *

 if ( MOD_samples == 15 )
    {
     MOD_OrderNum = *(unsigned char *)(MOD_Buffer + 470);
     MOD_Orders = (char *)(MOD_Buffer + 472);            // Pattern Order list
    }
 else
    {
     MOD_OrderNum = *(unsigned char *)(MOD_Buffer + 950);
     MOD_Orders = (char *)(MOD_Buffer + 952);            // Pattern Order list
    }

 value = 0;
 for (test = 0; test < 127; test++)
     {
      if ( MOD_Orders[test] > value )  value = MOD_Orders[test];
     }
 MOD_patterns = value + 1;

 printf("patterns: %u\n",MOD_patterns);

// * Get memory for pointers *

 InstrumentPointer = (unsigned long *) malloc ( MOD_samples * 4 + 4);
 SamplePointer = (unsigned long *) malloc ( MOD_samples * 4 + 4);

 Pointer = (char *)(MOD_Buffer + 20);

 if ( MOD_samples == 15 )  value = 600;
 else  value = 1084;
 value += MOD_patterns*MOD_channels*64*4;

 for (test = 0; test < MOD_samples; test++)    // set Ins & Smp pointers
     {
      *(char *)(Pointer + 21 + (test*30) ) = 0;  // terminate sample string in all cases !
      *(unsigned short *)(Pointer + 22 + (test*30) ) = amiga_to_x86 ( *(unsigned short *)(Pointer + 22 + (test*30) ) );
      *(unsigned short *)(Pointer + 26 + (test*30) ) = amiga_to_x86 ( *(unsigned short *)(Pointer + 26 + (test*30) ) );
      *(unsigned short *)(Pointer + 28 + (test*30) ) = amiga_to_x86 ( *(unsigned short *)(Pointer + 28 + (test*30) ) );
      InstrumentPointer[test] = (unsigned long)(Pointer + (test*30));
      SamplePointer[test] = (unsigned long)(MOD_Buffer + value );
      value += (unsigned long)( *(unsigned short *)(InstrumentPointer[test] + 22) ) * 2;
     }

 PatternPointer = (unsigned long *) malloc ( MOD_patterns * 4 + 4);

 if ( MOD_samples == 15 )  Pointer = (char *)(MOD_Buffer + 600);
 else  Pointer = (char * )(MOD_Buffer + 1084);

 for (test = 0; test < MOD_patterns; test++)    // set Pattern pointers
     {
      PatternPointer[test] = (unsigned long)(Pointer + (test*4*64*MOD_channels) );
     }

 // *** prepare MOD variables ***

 MOD_initvariables();

 MasterVolume = (0x50 - (MOD_channels*0x04)) * 256;  // 4 ch = 0x40 * 256
                                                     // 6 ch = 0x38 * 256
                                                     // 8 ch = 0x30 * 256

 MOD_flag_amigalimits = 0;

 MOD_tempo = 128;
 MOD_speed = 6;

 MOD_Panning[0] = 28;
 MOD_Panning[1] = 100;
 MOD_Panning[2] = 100;
 MOD_Panning[3] = 28;
 MOD_Panning[4] = 28;
 MOD_Panning[5] = 100;
 MOD_Panning[6] = 100;
 MOD_Panning[7] = 28;

 // *** calculating and preparing MOD variables like speed etc. ***

 MOD_TickLength = DAT_calcSpeed ( MOD_tempo, MixSpeed );

 MOD_LastPattern = 65535;         // no pattern last worked on
 MOD_LastRow = 65535;             // the same as above for row

 MOD_row = 0;
 MOD_jump = 0;
 MOD_tick = 0;
 MOD_tick_rest = MOD_TickLength;
 MOD_Order = 0;
 MOD_Pattern = MOD_Orders[MOD_Order];

 MOD_unpack_row ( MOD_Pattern, MOD_row );   // unpack first Orderpattern, first row
 MOD_read_row();

 MOD_initEffects();

 // some test code...

 printf("MOD file %s loaded !\n",FileName);
 printf("Name: %s\n",MOD_Buffer);
 printf("MOD:  %u  %x\n", MOD_Buffer, MOD_Buffer[0]);
 printf("Ins1: %u  %x  %s\n", InstrumentPointer[0], *(unsigned char *)InstrumentPointer[0], (char *)(InstrumentPointer[0]));
 printf("Smp1: %u  %x\n", SamplePointer[0], *(char *)SamplePointer[0] );
 printf("Ins2: %u  %x  %s\n", InstrumentPointer[1], *(unsigned char *)InstrumentPointer[1], (char *)(InstrumentPointer[1]));
 printf("Smp2: %u  %x\n", SamplePointer[1], *(char *)SamplePointer[1] );
 printf("Pattern1: %u\n", PatternPointer[0] );
 printf("Channels: %u\n",MOD_channels);
 printf("MasterVolume %u\n",MasterVolume);

 close_file_MOD ();  // close MOD file coz it is ready for playing

 flag_DatReady = 1;

return 0;
}


long close_MOD (void)
{
 close_file_MOD ();

 if ( InstrumentPointer != NULL )
    {
     free ( InstrumentPointer );
     InstrumentPointer = NULL;
    }
 if ( SamplePointer != NULL )
    {
     free ( SamplePointer );
     SamplePointer = NULL;
    }
 if ( PatternPointer == NULL )
    {
     free ( PatternPointer );
     PatternPointer = NULL;
    }

 if ( MOD_Buffer == NULL )  return 1;

 free ( MOD_Buffer );
 MOD_Buffer = NULL;

return 0;
}

// ****************************** Initializations ***************************

void MOD_initvariables()
{
 unsigned long channel;

  MOD_LastPattern = 65535;        // no last pattern
  MOD_LastRow = 65535;            // no last row exists

  MOD_TickLength = 0;         // how many samples to make one frame

  MOD_row = 0;
  MOD_jump = 0;
  MOD_nextrow = 0;
  MOD_nextorder = 0;
  MOD_tick = 0;
  MOD_Order = 0;
  MOD_Pattern = 0;

  MOD_buffer_rest = 0;
  MOD_tick_rest = 0;

  MOD_flag_amigalimits = 0;   // 0 = no amiga limits
  MOD_flag_note = 0;

  MOD_tempo = 128;            // tempo of MOD
  MOD_speed = 6;              // speed of MOD

 for (channel = 0; channel < MOD_channels; channel++ )
     {
      MOD_ChannelActiv[channel] = 0;       //  0 != can be active

      MOD_SampleNr[channel] = 0;
      MOD_SampleAddress[channel] = 0;
      MOD_SampleLength[channel] = 0;
      MOD_SamplePosition[channel] = 0;
      MOD_SampleFraction[channel] = 0;
      MOD_SampleLoop[channel] = 0;
      MOD_SampleLoopBegin[channel] = 0;
      MOD_SampleLoopEnd[channel] = 0;
      MOD_SampleFinetune[channel] = 0;

      MOD_Volume[channel] = 0;
      MOD_Panning[channel] = 0x40;
      MOD_Note[channel] = 0xff;
      MOD_Effect[channel] = 0xff;
      MOD_EffectInfo[channel] = 0;
      MOD_LastEffect[channel] = 0xff;
      MOD_LastEffectInfo[channel] = 0;

      MOD_Periode[channel] = 0;
      MOD_PeriodeAdjust[channel] = 0;
      MOD_Frequence[channel] = 0;
     }
}


// **************************** Pattern functions ***************************

static void clear_Row ()             // clear row buffer with default values
{
 static unsigned long counter;

 for ( counter = 0; counter < 32; counter += 4)
     {
      *(unsigned long *)(MOD_RowBuffer + counter) = 0x00ff00ff;
     }
}


static void MOD_unpack_row( unsigned long PatternNr, unsigned long RowNr )
{
 static unsigned long  counter;
 static signed long  value, periode, note;
 static char          *point;


 if ( PatternNr > MOD_patterns )
    {
     clear_Row ();
     MOD_LastPattern = PatternNr;
     MOD_LastRow = RowNr;
     return;
    }

 if ( (PatternNr == MOD_LastPattern) && ( RowNr == MOD_LastRow ) )  return;  // the same line again ;)

 clear_Row ();

 // begin of decrunching

 point = (char *)(PatternPointer[PatternNr] + (RowNr * MOD_channels * 4) );

 for ( counter = 0; counter < MOD_channels; counter++)
     {
      periode = ((((unsigned long)(*point)) & 0x0f) << 8) + (unsigned long)(*(char *)(point+1));
      note = 0xff;
      if ( periode != 0 )
         {
      for ( value = 0; value < 84; value++ )
          {
           if ( abs(periode - MOD_Periodes[0][value]) <= (periode / 100 + 1) )
               note = value;
          }
        if ( note == 0xff )  printf("Note seems wrong! Periode: %u\n", periode);
       }
      MOD_RowBuffer[counter*4+0] = note;

        value = ((*point) & 0xf0) + (*(char *)(point+2) >> 4);
        if ( value > MOD_samples )  value = 0;
      MOD_RowBuffer[counter*4+1] = value;
        value = *(char *)(point+2) & 0x0f;
      MOD_RowBuffer[counter*4+2] = value;
        value = *(char *)(point+3);
      MOD_RowBuffer[counter*4+3] = value;
      if ( (MOD_RowBuffer[counter*4+2] == 0) && (MOD_RowBuffer[counter*4+3] == 0) )
           MOD_RowBuffer[counter*4+2] = 0xff;
      point += 4;
     }

 // end of decrunching

 MOD_LastPattern = PatternNr;
 MOD_LastRow = RowNr;

}



unsigned long Calc_AMIGAperiode ( unsigned char Note, unsigned long Finetune )
{
static unsigned long noteperiode;

 noteperiode = MOD_Periodes[Finetune][Note] << 4; // extend by 4 bits for fixkomma

return noteperiode;   // extended by 16 = 4bit fixpoint
}


static unsigned long Calc_AMIGAfrequence ( unsigned long channel )
{
// frequence = 7159090.5 / (periode*2)
// but we must remeber that here periode is extended by 4 bits
// and we will let it extended by 2 bits

return  ( (14318181L << 4) / ( MOD_Periode[channel] + MOD_PeriodeAdjust[channel] ));
}


void MOD_goRowOrder()
{
 if ( flag_DatReady != 1 )  return;

 if (MOD_jump == 0)
    {
     if (MOD_flag_note == 0)  MOD_row++;

     if ( MOD_row == 64 )
        {
         MOD_Order++;
         MOD_row = 0;
         MOD_Pattern = MOD_Orders[ MOD_Order ];

         if ( MOD_Order == MOD_OrderNum )
            {
             MOD_Order = 0;
             MOD_Pattern = MOD_Orders[ MOD_Order ];
             if ( flag_loop != 1 )
                {
                 //toplay = MOD_buffer_rest;  // to set MOD_buffer_rest = 0
                 flag_DatReady = 2;
                }
            }
        }
    }
else
    {
     MOD_jump = 0;
     if (MOD_nextrow > 63)  return;
     if (MOD_nextorder > MOD_OrderNum)  return;
     MOD_row = MOD_nextrow;
     MOD_nextrow = 0;
     MOD_Order = MOD_nextorder;
     MOD_nextorder = 0;

     MOD_Pattern = MOD_Orders[ MOD_Order ];

     if ( MOD_Order == MOD_OrderNum )
        {
         MOD_Order = 0;
         MOD_Pattern = MOD_Orders[ MOD_Order ];
         if ( flag_loop != 1 )
            {
             //toplay = MOD_buffer_rest;  // to set MOD_buffer_rest = 0
             flag_DatReady = 2;
            }
        }
    }
}



// **************************** MOD tick routines ***************************

void MOD_GetNewSample ( unsigned long channel )
{
 static unsigned long SampleNr;

 if ( MOD_SampleNr[ channel ] == 0 )  return;

 SampleNr = MOD_SampleNr[ channel ] - 1 ;

 if ( SampleNr <= MOD_samples )
 if ( (*(unsigned short *)(InstrumentPointer[SampleNr] + 22)) > 0 )
 {
  MOD_SampleAddress[channel] = SamplePointer[SampleNr];
  MOD_SampleLength[channel] = (unsigned long)(*(unsigned short *)( InstrumentPointer[SampleNr] + 22 )) * 2;
  MOD_SampleLoopBegin[channel] = (unsigned long)(*(unsigned short *)( InstrumentPointer[SampleNr] + 26 )) * 2;
  MOD_SampleLoopEnd[channel] = (unsigned long)(*(unsigned short *)( InstrumentPointer[SampleNr] + 28 )) * 2;
  if (MOD_SampleLoopEnd[channel] > 2)  MOD_SampleLoop[channel] = 1;
     else  MOD_SampleLoop[channel] = 0;
  MOD_SampleLoopEnd[channel] += MOD_SampleLoopBegin[channel];
  MOD_SampleFinetune[channel] = (unsigned long)*(unsigned char *)( InstrumentPointer[SampleNr] + 24 );
  MOD_TremoloVolume[channel] = MOD_Volume[channel] = *(unsigned char *)( InstrumentPointer[SampleNr] + 25 );
 }
 else   MOD_SampleNr[ channel ] = 0;
 else   MOD_SampleNr[ channel ] = 0;
}


void MOD_GetNewNote ( unsigned long channel )
{
 if ( MOD_SampleNr[channel] != 0 )
    {
     if ( (MOD_Note[channel] >= 84 ) )
         {
          MOD_Periode[channel] = 0;
          MOD_ChannelActiv[channel] = 0;
         }
     else
         {
          MOD_Periode[channel] = MOD_GlissPeriode[channel] = Calc_AMIGAperiode ( MOD_Note[channel], MOD_SampleFinetune[channel] );
          MOD_SamplePosition[channel] = 0;
          MOD_SampleFraction[channel] = 0;
          MOD_ChannelActiv[channel] = 1;
         }
    }
 else   MOD_ChannelActiv[channel] = 0;
}


void MOD_GetNewEffect ()
{
 static unsigned long channel;
 static unsigned short Effect, Info;

 for (channel = 0; channel < MOD_channels; channel++)
     {
      Effect = MOD_Effect[channel];
      Info = MOD_EffectInfo[channel];

      // reset of variables depending on commands

      switch (Effect)
             {
              case 255 : // no effect
                         MOD_PeriodeAdjust[channel] = 0;
                         break;
              case  0 : // None/Arpeggio
                        MOD_PeriodeAdjust[channel] = 0;
                        MOD_VibratoPosition[channel] = 0;
                        break;
              case  1 : // Axx
              case  2 : // Bxx
                        MOD_Periode[channel] += MOD_PeriodeAdjust[channel];
                        MOD_PeriodeAdjust[channel] = 0;
                        MOD_VibratoPosition[channel] = 0;
                        break;
              case  3 : // Cxx
                        MOD_VibratoPosition[channel] = 0;
                        break;
              case  4 : // Dxy
                        break;
              case  5 : // Exx
                        MOD_VibratoPosition[channel] = 0;
                        break;
              case  6 : // Fxx
                        break;
              case  7 : // Gxx
                        break;
              case  8 : // Hxy
                        break;
              case  9 : // Ixy
                        MOD_VibratoPosition[channel] = 0;
                        break;
              case 10 : // Jxy
                        MOD_PeriodeAdjust[channel] = 0;
                        break;
              case 11 : // Kxy
                        break;
              case 12 : // Lxy
                        break;
              case 13 : // M--
                        break;
              case 14 : // N--
                        break;
              case 15 : // Oxx
                        break;
             }

      // reloading last effect info if info is zero

      if ( Info == 0 )

      switch (Effect)
             {
              case 255 : // no effect
                         break;
              case  0 :
              case  1 : // Axx
              case  2 : // Bxx
              case  3 : // Cxx
              case  4 : // Dxy
              case  5 : // Exx
              case  6 : // Fxx
                        break;
              case  7 : // Gxx
              case  8 : // Hxy
              case  9 : // Ixy
              case 10 : // Jxy
                        //MOD_EffectInfo[channel] = MOD_LastEffectInfo[channel];
                        break;
              case 11 : // Kxy
              case 12 : // Lxy
              case 13 : // M--
              case 14 : // N--
              case 15 : // Oxx
                        break;
             }
     }

 MOD_GlobalEffect();
 MOD_beforeEffect();
}



void MOD_read_row ()
{
 static unsigned long channel;
 static long value;

 // MOD effect

 for (channel = 0; channel < MOD_channels; channel++)
      {
       value = MOD_RowBuffer[channel*4+3];  // effect info
       if ( MOD_EffectInfo[channel] != 0 )
           MOD_LastEffectInfo[channel] = MOD_EffectInfo[channel];
       //if ( MOD_LastEffectInfo[channel] == 0)
       //    MOD_LastEffectInfo[channel] = value;
       MOD_EffectInfo[channel] = value;

       value = MOD_RowBuffer[channel*4+2];  // effect
       if ( MOD_Effect[channel] != 255 )
           MOD_LastEffect[channel] = MOD_Effect[channel];
       //if ( MOD_LastEffect[channel] == 255 )
       //    MOD_LastEffect[channel] = value;
       MOD_Effect[channel] = value;
      }

 MOD_GetNewEffect();


 // MOD note+instrument+volume

 if ( MOD_flag_note == 0 )    // then read note, volume and instrument
    {
     for (channel = 0; channel < MOD_channels; channel++)
         {
          value = MOD_RowBuffer[channel*4+1];  // instrument
          if ( (value != 0) && (value <= MOD_samples) )
             {
//              printf("SampleNr: %u\n",value);
              MOD_SampleNr[channel] = value;
              MOD_GetNewSample ( channel );
             }

          value = MOD_RowBuffer[channel*4+0];  // note + octave
          if ( value != 255 )
             {
              MOD_Note[channel] = value;
              MOD_GetNewNote ( channel );
              MOD_VibratoPosition[channel] = 0;   // reset Vibrato
              MOD_PeriodeAdjust[channel] = 0;
             }
         }     // for channel
    }  // flag test MOD_noteflag

 MOD_RowEffect();
}


// ****************************** MOD interrupt *****************************

void MOD_to_Mixer ()
{
 static unsigned long channel;

 ChannelLast = MOD_channels;

 for ( channel = 0; channel < MOD_channels; channel++ )
  {
   if ( MOD_Periode[channel] < 10 )  MOD_ChannelActiv[channel] = 0;
   ChannelActiv[channel] = MOD_ChannelActiv[channel];

   ChannelVolume[channel] = (unsigned long)(MOD_Volume[channel]) << 2;
   ChannelPanning[channel] = (unsigned long)(MOD_Panning[channel]) * 2;

   if ( MOD_ChannelActiv[channel] != 0 )
      {
//       printf("YES ! ");
       ChannelSamplePosition[channel] = MOD_SamplePosition[channel];
       ChannelSampleFraction[channel] = MOD_SampleFraction[channel];
       MOD_Frequence[channel] = Calc_AMIGAfrequence( channel );
       ChannelSampleFrequence[channel] = (MOD_Frequence[channel] + 2) >> 2;

       if ( MOD_SampleNr[channel] != 0 )
       if ( ChannelSampleNr[channel] != MOD_SampleNr[channel] )
          {
           ChannelSampleNr[channel] = MOD_SampleNr[channel];
           ChannelSampleBits[channel] = 8;   // 8 bit sample only
           ChannelSampleMode[channel] = 0;   // sample always mono
           ChannelSampleAddress[channel] = MOD_SampleAddress[channel];
           ChannelSampleLength[channel] = MOD_SampleLength[channel];
           ChannelLoopMode[channel] = MOD_SampleLoop[channel];
           ChannelLoopBegin[channel] = MOD_SampleLoopBegin[channel];
           ChannelLoopEnd[channel] = MOD_SampleLoopEnd[channel];
          }
      }
  }
}


void Mixer_to_MOD ()
{
 static unsigned long channel;

 for ( channel = 0; channel < MOD_channels; channel++ )
  {
   if ( MOD_ChannelActiv[channel] != 0 )
      {
       MOD_SamplePosition[channel] = ChannelSamplePosition[channel];
       MOD_SampleFraction[channel] = ChannelSampleFraction[channel];
       MOD_ChannelActiv[channel] = ChannelActiv[channel];
      }
  }
}



void interrupt_MOD ()
{
 static unsigned long MOD_toplay = 0;

 MOD_buffer_rest = block_length;
 MixerAddress = 0;
 clearMixer();

while ( MOD_buffer_rest > 0 )
      {
       if ( MOD_tick_rest <= MOD_buffer_rest )
          {
           MOD_toplay = MOD_tick_rest;

           MOD_to_Mixer();
           DoPreMixing ( MOD_toplay );
           Mixer_to_MOD();
           MixerAddress += MOD_toplay;
           //MOD_tick_rest = 0;

           MOD_tick++;

           if ( MOD_tick == MOD_speed )
              {
               MOD_tick=0;

               MOD_goRowOrder();

               MOD_unpack_row ( MOD_Pattern, MOD_row );
               if ( flag_DatReady == 1 )   MOD_read_row();
              }
           else
                MOD_TickEffect();

           MOD_buffer_rest -= MOD_toplay;
           MOD_tick_rest = MOD_TickLength;
          }
       else
          {
           MOD_toplay = MOD_buffer_rest;

           MOD_to_Mixer();
           DoPreMixing ( MOD_toplay );
           Mixer_to_MOD();

           MixerAddress += MOD_toplay;
           MOD_tick_rest -= MOD_toplay;
           MOD_buffer_rest = 0;
          }

      }  // end while

 DoMasterVolumeCalculations( MixerBlock, Mixerpointer, block_length );
}

