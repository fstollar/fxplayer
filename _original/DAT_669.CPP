// 669 - Routines for F/X Player v1.0
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
#include "dat_669.h"
#include "efc_669.h"

#include "convert.h"
#include "mixer.h"
#include "amptable.h"


// *** Variables for 669's and headers ***

FILE                 *_669_handle = NULL;
static char          *_669_Buffer = NULL;
static unsigned long  _669_length = 0;

static unsigned long *InstrumentPointer = NULL, *SamplePointer = NULL, *PatternPointer = NULL;

unsigned long   _669_samples  = 0;
unsigned long   _669_patterns = 0;

unsigned char   _669_OrderNum;
unsigned char  *_669_Orderlist;
unsigned char  *_669_Speedlist;
unsigned char  *_669_Breaklist;

// *** 669 runtime variables ***

unsigned char   _669_RowBuffer[_669_channels*5];         // buffer where row is uncompressed
unsigned long   _669_LastPattern = 65535;    // no last pattern
unsigned long   _669_LastRow = 65535;        // no last row exists

unsigned long   _669_TickLength = 0;         // how many samples to make one frame

unsigned long   _669_row = 0;
unsigned long   _669_jump = 0;
unsigned long   _669_tick = 0;
unsigned long   _669_Order = 0;
unsigned long   _669_Pattern = 0;

unsigned long   _669_buffer_rest = 0;
unsigned long   _669_tick_rest = 0;

unsigned char   _669_tempo = 78;            // tempo of 669
unsigned char   _669_speed = 6;              // speed of 669

unsigned char   _669_ChannelActiv[_669_channels];       //  0 != can be active

unsigned char   _669_SampleNr[_669_channels];
unsigned long   _669_SampleAddress[_669_channels];
unsigned long   _669_SampleLength[_669_channels];
unsigned long   _669_SamplePosition[_669_channels];
unsigned long   _669_SampleFraction[_669_channels];
unsigned char   _669_SampleLoop[_669_channels];
unsigned long   _669_SampleLoopBegin[_669_channels];
unsigned long   _669_SampleLoopEnd[_669_channels];
unsigned long   _669_SampleFinetune[_669_channels];

unsigned char   _669_Volume[_669_channels];
unsigned char   _669_Panning[_669_channels];
unsigned char   _669_Note[_669_channels];
unsigned char   _669_Effect[_669_channels];
unsigned char   _669_EffectInfo[_669_channels];
unsigned char   _669_LastEffect[_669_channels];
unsigned char   _669_LastEffectInfo[_669_channels];

unsigned long   _669_Periode[_669_channels];
signed   long   _669_PeriodeAdjust[_669_channels];
unsigned long   _669_Frequence[_669_channels];

// ********************** Periodes of 669 ************************

unsigned long _669_Periodes[16][72] = {
// Tuning 0, Normal
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      856,808,762,720,678,640,604,570,538,508,480,453, // C-1 to B-1
      428,404,381,360,339,320,302,285,269,254,240,226, // C-2 to B-2
      214,202,190,180,170,160,151,143,135,127,120,113, // C-3 to B-3
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 1
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      850,802,757,715,674,637,601,567,535,505,477,450, // same as above
      425,401,379,357,337,318,300,284,268,253,239,225, // but with
      213,201,189,179,169,159,150,142,134,126,119,113, // finetune +1
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 2
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      844,796,752,709,670,632,597,563,532,502,474,447, // etc,
      422,398,376,355,335,316,298,282,266,251,237,224, // finetune +2
      211,199,188,177,167,158,149,141,133,125,118,112,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 3
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      838,791,746,704,665,628,592,559,528,498,470,444,
      419,395,373,352,332,314,296,280,264,249,235,222,
      209,198,187,176,166,157,148,140,132,125,118,111,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 4
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      832,785,741,699,660,623,588,555,524,495,467,441,
      416,392,370,350,330,312,294,278,262,247,233,220,
      208,196,185,175,165,156,147,139,131,124,117,110,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 5
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      826,779,736,694,655,619,584,551,520,491,463,437,
      413,390,368,347,328,309,292,276,260,245,232,219,
      206,195,184,174,164,155,146,138,130,123,116,109,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 6
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      820,774,730,689,651,614,580,547,516,487,460,434,
      410,387,365,345,325,307,290,274,258,244,230,217,
      205,193,183,172,163,154,145,137,129,122,115,109,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning 7
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      814,768,725,684,646,610,575,543,513,484,457,431,
      407,384,363,342,323,305,288,272,256,242,228,216,
      204,192,181,171,161,152,144,136,128,121,114,108,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -8
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      907,856,808,762,720,678,640,604,570,538,508,480,
      453,428,404,381,360,339,320,302,285,269,254,240,
      226,214,202,190,180,170,160,151,143,135,127,120,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -7
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      900,850,802,757,715,675,636,601,567,535,505,477,
      450,425,401,379,357,337,318,300,284,268,253,238,
      225,212,200,189,179,169,159,150,142,134,126,119,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -6
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      894,844,796,752,709,670,632,597,563,532,502,474,
      447,422,398,376,355,335,316,298,282,266,251,237,
      223,211,199,188,177,167,158,149,141,133,125,118,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -5
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      887,838,791,746,704,665,628,592,559,528,498,470,
      444,419,395,373,352,332,314,296,280,264,249,235,
      222,209,198,187,176,166,157,148,140,132,125,118,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -4
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      881,832,785,741,699,660,623,588,555,524,494,467,
      441,416,392,370,350,330,312,294,278,262,247,233,
      220,208,196,185,175,165,156,147,139,131,123,117,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -3
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      875,826,779,736,694,655,619,584,551,520,491,463,
      437,413,390,368,347,328,309,292,276,260,245,232,
      219,206,195,184,174,164,155,146,138,130,123,116,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -2
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      868,820,774,730,689,651,614,580,547,516,487,460,
      434,410,387,365,345,325,307,290,274,258,244,230,
      217,205,193,183,172,163,154,145,137,129,122,115,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
// Tuning -1
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      862,814,768,725,684,646,610,575,543,513,484,457,
      431,407,384,363,342,323,305,288,272,256,242,228,
      216,203,192,181,171,161,152,144,136,128,121,114,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
     };


// *** Functions for loading and preparing 669 ***


static long open_file_669 ( char *FileName )
{
 if ( (_669_handle=fopen(FileName,"rb") )==NULL )
      {
       return 1;
      }

return 0;
}


static long close_file_669 (void)
{
 if (_669_handle == NULL)
    return 1;

 fclose ( _669_handle );
 _669_handle = NULL;

return 0;
}


long load_669 ( char *FileName )
{
 static unsigned long channel;
 static char *Pointer;
 static long counter, test, value;

 flag_DatReady = 0;

 test = open_file_669 ( FileName );     // open 669 file
 if (test != 0)  return 1;              // could not open file

// *** load 669 complete into memory ***

 fseek ( _669_handle, 0L, SEEK_END );    // go to end of 669
 _669_length = ftell ( _669_handle );     // read file pointer eg. length of 669
 fseek ( _669_handle, 0L, SEEK_SET );    // and go back to start

 _669_Buffer = (char *) malloc ( _669_length+0x10 );   // alloc memory for 669
 if ( _669_Buffer == NULL )                      // if not enough memory free
    {
     close_file_669 ();
     return 2;                          // not enough memory to load 669
    }

 if ( (fread( _669_Buffer , 1, _669_length, _669_handle )) != _669_length )   // read 669
     return 3;                          // corrupt 669

// ****************** some precalc to make it playable **********************

 *(char *)(_669_Buffer + 109) = 0;

 for (counter = 0; counter < 16; counter++)
     for (test = 0; test < 12; test++)
         {
          _669_Periodes[counter][test] = _669_Periodes[counter][test+12] * 2;
          _669_Periodes[counter][test+48] = (_669_Periodes[counter][test+36] + 1) / 2;
          _669_Periodes[counter][test+60] = (_669_Periodes[counter][test+36] + 2) / 4;
         }

// ************************** Test id of 669 ********************************

 Pointer = _669_Buffer;

 if ( *(short *)Pointer != 0x6669 )  // is it a 669 type ?
    {
     close_669();
     return 3;
    }

// * Set pointers in 669 for Orders etc. *

 _669_samples = *(char *)(Pointer + 0x6e);
 _669_patterns = *(char *)(Pointer + 0x6f);
 _669_OrderNum = *(char *)(Pointer + 0x70);

 _669_Orderlist = Pointer + 0x71;
 _669_Speedlist = Pointer + 0xf1;
 _669_Breaklist = Pointer + 0x171;

// * Get memory for pointers *

 InstrumentPointer = (unsigned long *) malloc ( _669_samples * 4 + 4);
 SamplePointer = (unsigned long *) malloc ( _669_samples * 4 + 4);

 Pointer = (char *)(_669_Buffer + 0x1f1);
 value = (_669_samples * 0x19) + (_669_patterns * 0x600);

 for (test = 0; test < _669_samples; test++)    // set Ins & Smp pointers
     {
      InstrumentPointer[test] = (unsigned long)( Pointer + (test * 0x19) );
      SamplePointer[test] = (unsigned long)( Pointer + value );
      value += *(unsigned long *)( InstrumentPointer[test] + 13 );
      ConvU8MtoS8M ( (char *)SamplePointer[test], *(unsigned long *)(InstrumentPointer[test] + 13) );
     }

 PatternPointer = (unsigned long *) malloc ( _669_patterns * 4 + 4);
 Pointer = (char *)( _669_Buffer + 0x1f1 + _669_samples * 0x19 );

 for (test = 0; test < _669_patterns; test++)    // set Pattern pointers
     {
      PatternPointer[test] = (unsigned long)(Pointer + (test * 0x0600) );
     }

 // *** prepare 669 variables ***

 _669_initvariables();
 _669_initEffects();
 
 MasterVolume = 12288;

 _669_tempo = 78;
 _669_speed = _669_Speedlist[ 0 ];

 _669_Panning[0] = 56;
 _669_Panning[1] = 200;
 _669_Panning[2] = 56;
 _669_Panning[3] = 200;
 _669_Panning[4] = 56;
 _669_Panning[5] = 200;
 _669_Panning[6] = 56;
 _669_Panning[7] = 200;

 // *** calculating and preparing 669 variables like speed etc. ***

 _669_TickLength = DAT_calcSpeed ( _669_tempo, MixSpeed );

 _669_LastPattern = 65535;         // no pattern last worked on
 _669_LastRow = 65535;             // the same as above for row

 _669_row = 0;
 _669_jump = 0;
 _669_tick = 0;
 _669_tick_rest = _669_TickLength;
 _669_Order = 0;
 _669_Pattern = _669_Orderlist[ _669_Order ];

 _669_unpack_row ( _669_Pattern, _669_row );   // unpack first Orderpattern, first row
 _669_read_row();


 // some test code...

 printf("669 file %s loaded !\n",FileName);
 printf("Name: %s\n",(_669_Buffer + 2));
/*
 printf("669:  %u  %x\n", _669_Buffer, _669_Buffer[0]);
 printf("Ins1: %u  %x  %s\n", InstrumentPointer[0], *(unsigned char *)InstrumentPointer[0], (char *)(InstrumentPointer[0]));
 printf("Smp1: %u  %x\n", SamplePointer[0], *(char *)SamplePointer[0] );
 printf("Ins2: %u  %x  %s\n", InstrumentPointer[1], *(unsigned char *)InstrumentPointer[1], (char *)(InstrumentPointer[1]));
 printf("Smp2: %u  %x\n", SamplePointer[1], *(char *)SamplePointer[1] );
*/
 printf("Samples: %u, Patterns: %u\n", _669_samples, _669_patterns);
// printf("Pattern1: %u\n", PatternPointer[0] );
 printf("MasterVolume %u\n",MasterVolume);

 close_file_669 ();  // close 669 file coz it is ready for playing

 flag_DatReady = 1;

return 0;
}


long close_669 (void)
{
 close_file_669 ();

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

 if ( _669_Buffer == NULL )  return 1;

 free ( _669_Buffer );
 _669_Buffer = NULL;

return 0;
}

// ****************************** Initializations ***************************

void _669_initvariables()
{
 unsigned long channel;

  _669_LastPattern = 65535;        // no last pattern
  _669_LastRow = 65535;            // no last row exists

  _669_TickLength = 0;         // how many samples to make one frame

  _669_row = 0;
  _669_jump = 0;
  _669_tick = 0;
  _669_Order = 0;
  _669_Pattern = 0;

  _669_buffer_rest = 0;
  _669_tick_rest = 0;

  _669_tempo = 78;            // tempo of 669
  _669_speed = 6;              // speed of 669

 for (channel = 0; channel < _669_channels; channel++ )
     {
      _669_ChannelActiv[channel] = 0;       //  0 != can be active

      _669_SampleNr[channel] = 0;
      _669_SampleAddress[channel] = 0;
      _669_SampleLength[channel] = 0;
      _669_SamplePosition[channel] = 0;
      _669_SampleFraction[channel] = 0;
      _669_SampleLoop[channel] = 0;
      _669_SampleLoopBegin[channel] = 0;
      _669_SampleLoopEnd[channel] = 0;
      _669_SampleFinetune[channel] = 0;

      _669_Volume[channel] = 0;
      _669_Panning[channel] = 128;
      _669_Note[channel] = 0xff;
      _669_Effect[channel] = 0xff;
      _669_EffectInfo[channel] = 0;
      _669_LastEffect[channel] = 0xff;
      _669_LastEffectInfo[channel] = 0;

      _669_Periode[channel] = 0;
      _669_PeriodeAdjust[channel] = 0;
      _669_Frequence[channel] = 0;
     }
}


// **************************** Pattern functions ***************************

static void clear_Row ()             // clear row buffer with default values
{
 static unsigned long counter;

 for ( counter = 0; counter < _669_channels*5; counter += 5)
     {
      *(unsigned long *)(_669_RowBuffer + counter) = 0xffffffff;
      *(unsigned char *)(_669_RowBuffer + counter + 4) = 0x00;
     }
}


static void _669_unpack_row( unsigned long PatternNr, unsigned long RowNr )
{
 static unsigned long  counter;
 static   signed long  value, note, inst, vol;
 static char          *point;


 if ( PatternNr > _669_patterns )
    {
     clear_Row ();
     _669_LastPattern = PatternNr;
     _669_LastRow = RowNr;
     return;
    }

 if ( (PatternNr == _669_LastPattern) && ( RowNr == _669_LastRow ) )
    return;   // the same line again ;)

 if ( PatternNr != _669_LastPattern )
    {
     _669_speed = _669_Speedlist[ PatternNr ];
    }

 clear_Row ();

 // begin of decrunching

 point = (char *)(PatternPointer[PatternNr] + (RowNr * _669_channels * 3) );

 for ( counter = 0; counter < _669_channels; counter++)
     {
      note = *point;
      if ( note != 0xff )
         {
          if ( note != 0xfe )
             {
              note >>= 2;
              inst = (((*point) & 0x03) << 4) | ((*(point+1) & 0xf0) >> 4);
              if ( inst > _669_samples )
                 {
                  inst = 0xff;
                  note = 0xfe;
                 }
//printf("Note %i\n", note);
             }
          else
             {
              note = 0xfe;
              inst = 0xff;
             }
          vol = *(point+1) & 0x0f;
         }
      else
         {
          note = 0xff;
          inst = 0xff;
          vol = 0xff;
         }
      _669_RowBuffer[counter*5+0] = note;
      _669_RowBuffer[counter*5+1] = inst;
      _669_RowBuffer[counter*5+2] = vol;

      value = *(point+2);
      if ( value != 0xff )   value = (*(point+2) & 0xf0) >> 4;
      _669_RowBuffer[counter*5+3] = value;
      value = (*(point+2)) & 0x0f;
      _669_RowBuffer[counter*5+4] = value;
      point += 3;
     }

 // end of decrunching

 _669_LastPattern = PatternNr;
 _669_LastRow = RowNr;

}



unsigned long Calc_669periode ( unsigned long Note, unsigned long Finetune )
{
static unsigned long noteperiode;

 noteperiode = _669_Periodes[Finetune][Note] << 4; // extend by 4 bits for fixkomma

return noteperiode;   // extended by 16 = 4bit fixpoint
}


static unsigned long Calc_669frequence ( unsigned long channel )
{
// frequence = 7159090.5 / (periode*2)
// but we must remeber that here periode is extended by 4 bits
// and we will let it extended by 2 bits

return  ( (14318181L << 4) / ( _669_Periode[channel] + _669_PeriodeAdjust[channel] ));
}


void _669_goRowOrder()
{
 if ( flag_DatReady != 1 )  return;

     _669_row++;

     if ( _669_row >= (_669_Breaklist[ _669_Pattern ]+1) )
        {
         _669_Order++;
         _669_row = 0;
         _669_Pattern = _669_Orderlist[ _669_Order ];

         if ( _669_Pattern == 0xff )
            {
             _669_Order = 0;
             _669_Pattern = _669_Orderlist[ _669_Order ];
             if ( flag_loop != 1 )
                {
                 //toplay = 669_buffer_rest;  // to set 669_buffer_rest = 0
                 flag_DatReady = 2;
                }
            }
        }
}



// **************************** 669 tick routines ***************************

void _669_GetNewSample ( unsigned long channel )
{
 static unsigned long SampleNr;

 if ( _669_SampleNr[ channel ] == 0 )  return;

 SampleNr = _669_SampleNr[ channel ] - 1 ;

 if ( SampleNr <= _669_samples )
 if ( (*(unsigned long *)(InstrumentPointer[SampleNr] + 13)) > 0 )
 {
  _669_SampleAddress[channel] = SamplePointer[SampleNr];
  _669_SampleLength[channel] = *(unsigned long *)( InstrumentPointer[SampleNr] + 13 );
  _669_SampleLoopBegin[channel] = *(unsigned long *)( InstrumentPointer[SampleNr] + 17 );
  _669_SampleLoopEnd[channel] = *(unsigned long *)( InstrumentPointer[SampleNr] + 21 );
  if ( _669_SampleLoopEnd[channel] != 0x000fffff)  _669_SampleLoop[channel] = 1;
     else  _669_SampleLoop[channel] = 0;
  _669_SampleFinetune[channel] = 0;
  _669_Volume[channel] = 16;
 }
 else   _669_SampleNr[ channel ] = 0;
 else   _669_SampleNr[ channel ] = 0;
}


void _669_GetNewNote ( unsigned long channel )
{
 if ( _669_SampleNr[channel] != 0 )
    {
     if ( ( _669_Note[channel] >= 64 ) )
         {
          _669_Periode[channel] = 0;
          _669_ChannelActiv[channel] = 0;
         }
     else
         {
          _669_Periode[channel] = _669_GlissPeriode[channel] = Calc_669periode ( _669_Note[channel], _669_SampleFinetune[channel] );
          _669_GlissFlag[channel] = 0;
          _669_SamplePosition[channel] = 0;
          _669_SampleFraction[channel] = 0;
          _669_ChannelActiv[channel] = 1;
         }
    }
 else   _669_ChannelActiv[channel] = 0;
}


void _669_GetNewEffect ()
{
 static unsigned long channel;
 static unsigned short Effect, Info;

 for (channel = 0; channel < _669_channels; channel++)
     {
      Effect = _669_Effect[channel];
      Info = _669_EffectInfo[channel];

      // reset of variables depending on commands

      switch (Effect)
             {
              case 255 : // no effect
                         _669_PeriodeAdjust[channel] = 0;
                         break;
              case  0 : // Portamento up
              case  1 : // Portamento down
                        _669_Periode[channel] += _669_PeriodeAdjust[channel];
                        _669_PeriodeAdjust[channel] = 0;
                        _669_VibratoPosition[channel] = 0;
                        break;
              case  2 : // Portamento to Note (Glissando)
                        _669_VibratoPosition[channel] = 0;
                        break;
              case  3 : // Finetune
                        break;
              case  4 : // Vibrato
                        break;
              case  5 : // Set speed
                        break;
              case  6 : // Blance shift
                        break;
              case  7 : // Retrig
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
              case  7 : // Gxx
                        break;
             }
     }

 _669_GlobalEffect();
 _669_beforeEffect();
}



void _669_read_row ()
{
 static unsigned long channel;
 static long value;

 // 669 effect

 for (channel = 0; channel < _669_channels; channel++)
      {
       value = _669_RowBuffer[channel*5+4];  // effect info
       if ( _669_EffectInfo[channel] != 0 )
           _669_LastEffectInfo[channel] = _669_EffectInfo[channel];
       //if ( 669_LastEffectInfo[channel] == 0)
       //    669_LastEffectInfo[channel] = value;
       _669_EffectInfo[channel] = value;

       value = _669_RowBuffer[channel*5+3];  // effect
       if ( _669_Effect[channel] != 255 )
           _669_LastEffect[channel] = _669_Effect[channel];
       //if ( 669_LastEffect[channel] == 255 )
       //    669_LastEffect[channel] = value;
       _669_Effect[channel] = value;
      }

 _669_GetNewEffect();


 // 669 note+instrument+volume

     for (channel = 0; channel < _669_channels; channel++)
         {
           value = _669_RowBuffer[channel*5+1];  // instrument
           if ( ( value <= _669_samples ) && ( value != 0xff ) )
              {
               value++;
               _669_SampleNr[channel] = value;
               _669_GetNewSample ( channel );
              }

          value = _669_RowBuffer[channel*5+0];  // note + octave
          if ( ( value != 0xff ) && ( value != 0xfe ) )
             {
              _669_Note[channel] = value;
              _669_GetNewNote ( channel );
              _669_VibratoPosition[channel] = 0;   // reset Vibrato
              _669_PeriodeAdjust[channel] = 0;
             }

          value = _669_RowBuffer[channel*5+2];  // volume
          if ( _669_RowBuffer[channel*5+0] != 0xff )
             {
              _669_Volume[channel] = value;
             }

         }     // for channel

 _669_RowEffect();
}


// ****************************** 669 interrupt *****************************

void _669_to_Mixer ()
{
 static unsigned long channel;

 ChannelLast = _669_channels;

 for ( channel = 0; channel < _669_channels; channel++ )
  {
   if ( _669_Periode[channel] < 10 )  _669_ChannelActiv[channel] = 0;
   ChannelActiv[channel] = _669_ChannelActiv[channel];

   ChannelVolume[channel] = (unsigned long)(_669_Volume[channel]) << 4;
   ChannelPanning[channel] = (unsigned long)(_669_Panning[channel]);

   if ( _669_ChannelActiv[channel] != 0 )
      {
//printf("YES ! ");
       ChannelSamplePosition[channel] = _669_SamplePosition[channel];
       ChannelSampleFraction[channel] = _669_SampleFraction[channel];
       _669_Frequence[channel] = Calc_669frequence( channel );
       ChannelSampleFrequence[channel] = (_669_Frequence[channel] + 2) >> 2;

       if ( _669_SampleNr[channel] != 0 )
       if ( ChannelSampleNr[channel] != _669_SampleNr[channel] )
          {
           ChannelSampleNr[channel] = _669_SampleNr[channel];
           ChannelSampleBits[channel] = 8;   // 8 bit sample only
           ChannelSampleMode[channel] = 0;   // sample always mono
           ChannelSampleAddress[channel] = _669_SampleAddress[channel];
           ChannelSampleLength[channel] = _669_SampleLength[channel];
           ChannelLoopMode[channel] = _669_SampleLoop[channel];
           ChannelLoopBegin[channel] = _669_SampleLoopBegin[channel];
           ChannelLoopEnd[channel] = _669_SampleLoopEnd[channel];
          }
      }
  }
}


void Mixer_to_669 ()
{
 static unsigned long channel;

 for ( channel = 0; channel < _669_channels; channel++ )
  {
   if ( _669_ChannelActiv[channel] != 0 )
      {
       _669_SamplePosition[channel] = ChannelSamplePosition[channel];
       _669_SampleFraction[channel] = ChannelSampleFraction[channel];
       _669_ChannelActiv[channel] = ChannelActiv[channel];
      }
  }
}



void interrupt_669 ()
{
 static unsigned long _669_toplay = 0;

 _669_buffer_rest = block_length;
 MixerAddress = 0;
 clearMixer();

while ( _669_buffer_rest > 0 )
      {
       if ( _669_tick_rest <= _669_buffer_rest )
          {
           _669_toplay = _669_tick_rest;

           _669_to_Mixer();
           DoPreMixing ( _669_toplay );
           Mixer_to_669();

           MixerAddress += _669_toplay;
           //669_tick_rest = 0;

           _669_tick++;

           if ( _669_tick == _669_speed )
              {
               _669_tick=0;
//printf("Row: %i\n", _669_row);
               _669_goRowOrder();

               _669_unpack_row ( _669_Pattern, _669_row );
               if ( flag_DatReady == 1 )   _669_read_row();
              }
           else
                _669_TickEffect();

           _669_buffer_rest -= _669_toplay;
           _669_tick_rest = _669_TickLength;
          }
       else
          {
           _669_toplay = _669_buffer_rest;

           _669_to_Mixer();
           DoPreMixing ( _669_toplay );
           Mixer_to_669();

           MixerAddress += _669_toplay;
           _669_tick_rest -= _669_toplay;
           _669_buffer_rest = 0;
          }

      }  // end while

 DoMasterVolumeCalculations( MixerBlock, Mixerpointer, block_length );
}


void FF_669 ()
{
 printf("Skip one 669 pattern\n");

 //printf("Order: %u\n", _669_Order);

 _669_row = _669_Breaklist[ _669_Pattern ];

}

void FR_669 ()
{
 printf("Rewind one 669 pattern\n");

 if ( _669_Order > 0)
    {
     _669_Order--;
     _669_row = -1;
     _669_Pattern = _669_Orderlist[ _669_Order ];
    }

}
