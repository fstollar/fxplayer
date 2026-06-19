// S3M - Routines for F/X Player v2.0
// (c) 1997 by Apollo / STIGMA

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

#include "format.h"
#include "dat_calc.h"
//#include "dat_wav.h"
#include "dat_s3m.h"
#include "efc_s3m.h"

#include "convert.h"
#include "mixer.h"


// *** Variables for S3M's and headers ***

FILE *S3M_handle = NULL;

char *S3M_Buffer = NULL;
char *S3M_pointer = NULL;

unsigned long S3M_length = 0;

struct S3M_struct
 {
  char Song_name[28];                     // Offset 0, length 28
  char value_1ah;                         // Offset 28, 1 byte
  char Type;                              // Offset 29, 1 byte
  short dummy;                            // Offset 30, 2 byte
  unsigned short OrdNum;                  // Offset 32, 1 word
  unsigned short InsNum;                  // Offset 34, 1 word
  unsigned short PatNum;                  // Offset 36, 1 word
  unsigned short Flags;                   // Offset 38, 1 word
  unsigned short Tracker_version;         // Offset 40, 1 word
  unsigned short File_Format_Info;        // Offset 42, 1 word
  char SCRM[4];                           // Offset 44, 4 bytes
  unsigned char Global_Volume;            // Offset 48, 1 byte
  unsigned char Init_Speed;               // Offset 49, 1 byte
  unsigned char Init_Tempo;               // Offset 50, 1 byte
  unsigned char Master_Volume;            // Offset 51, 1 byte
  unsigned char Ultra_Click;              // Offset 52, 1 byte
  unsigned char Default_Panning;          // Offset 53, 1 byte
  char dummy2[8];                         // Offset 54, 8 bytes
  unsigned short Special_Pointer;         // Offset 62, 1 word
 } *S3M_header;                           // together = 64 bytes

unsigned short S3M_OrderNum, S3M_InstrumentNum, S3M_PatternNum;
//unsigned short S3M_Flags, Tracker_version, File_Format_Info;

char *ChannelSets, *Orders, *DefaultPan;
unsigned long *InstrumentPointer=NULL, *SamplePointer=NULL, *PatternPointer=NULL;


// *** S3M runtime variables ***

unsigned short  S3M_channels = 32;          // how many channels are used

unsigned char   S3M_RowBuffer[32*5];        // buffer where row is uncompressed
unsigned short  LastPattern = 65535;        // no last pattern
unsigned short  LastRow = 65535;            // no last row exists

unsigned short  S3M_TickLength = 0;         // how many samples to make one frame

unsigned short  S3M_row = 0;
unsigned short  S3M_jump = 0;
unsigned short  S3M_nextrow = 0;
unsigned short  S3M_nextorder = 0;
unsigned short  S3M_tick = 0;
unsigned short  S3M_Order = 0;
unsigned short  S3M_Pattern = 0;

unsigned short  S3M_buffer_rest = 0;
unsigned short  S3M_tick_rest = 0;

unsigned char   S3M_flag_stereo = 0;        // 0 = mono, 1 = stereo S3M
unsigned char   S3M_flag_amigalimits = 0;   // 0 = no amiga limits
unsigned char   S3M_flag_st3volslides = 0;  // 1 = volume slide on EVERY frame
unsigned char   S3M_flag_note = 0;

unsigned char   S3M_tempo = 128;            // tempo of S3M
unsigned char   S3M_speed = 6;              // speed of S3M
unsigned char   S3M_GlobalVolume = 64;      // global volume of S3M, 64 = max

unsigned char   S3M_ChannelActiv[32];       //  0 != can be active

unsigned char   S3M_SampleNr[32];
unsigned long   S3M_SampleAdress[32];
unsigned long   S3M_SampleLength[32];
unsigned long   S3M_SamplePosition[32];
unsigned short  S3M_SampleFraction[32];
unsigned long   S3M_SampleLoopBegin[32];
unsigned long   S3M_SampleLoopEnd[32];
unsigned long   S3M_SampleC4SPD[32];
unsigned char   S3M_SampleFlags[32];

unsigned char   S3M_Volume[32];
unsigned char   S3M_Panning[32];
unsigned char   S3M_Note[32];
unsigned char   S3M_Octave[32];
unsigned char   S3M_Effect[32];
unsigned char   S3M_EffectInfo[32];
unsigned char   S3M_LastEffect[32];
unsigned char   S3M_LastEffectInfo[32];

unsigned long   S3M_Periode[32];
signed   long   S3M_PeriodeAdjust[32];
unsigned long   S3M_Frequence[32];

unsigned short  S3M_NotePeriodes[12] =
{ 1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 907 }; // C to B

unsigned short S3M_PanningTable [16] =
{ 0, 18, 37, 55, 73, 91, 110, 128, 144, 160, 176, 192, 208, 224, 240, 256 };


// *** Functions for loading and preparing S3M ***

short open_file_S3M ( char *FileName )
{

 if ( (S3M_handle=fopen(FileName,"rb") )==NULL )
      {
       return 1;
      }

return 0;
}

short close_file_S3M (void)
{

 if (S3M_handle == NULL)
    return 1;

 fclose ( S3M_handle );

 S3M_handle = NULL;

return 0;
}


short load_S3M ( char *FileName )
{
 unsigned long channel;
 long test;

 test = open_file_S3M ( FileName );     // open S3M file
 if (test != 0)  return 1;              // could not open file

 // load S3M complete into memory

 fseek ( S3M_handle, 0L, SEEK_END );    // go to end of S3M
 S3M_length = ftell ( S3M_handle );     // read file pointer eg. length of S3M
 fseek ( S3M_handle, 0L, SEEK_SET );    // and go back to start

 S3M_Buffer = (char *) malloc ( S3M_length );   // alloc memory for S3M
 if ( S3M_Buffer == NULL )                      // if not enough memory free
    {
     close_file_S3M ();
     return 2;                          // not enough memory to load s3m
    }

 if ( (fread( S3M_Buffer , 1, S3M_length, S3M_handle ))!= S3M_length )   // read S3M
     return 3;                          // corrupt S3M

 S3M_header = (struct S3M_struct *)S3M_Buffer;   // set header into structure

S3M_OrderNum = S3M_header->OrdNum;
S3M_InstrumentNum = S3M_header->InsNum;
S3M_PatternNum = S3M_header->PatNum;
// S3M_Flags = S3M_header->Flags;
// Tracker_Version = S3M_header->Tracker_Version;
// File_Format_Info = S3M_header->File_Info;

// * Test id of S3M *

 if ( memcmp(S3M_header->SCRM , "SCRM" , 4) )  return 3;  // is "SCRM"
 if ( S3M_header->value_1ah != 0x1a )  return 3;          // is 0x1a
 if ( S3M_header->Type      != 16   )  return 3;          // is 16 = module
 if ( S3M_header->OrdNum < 2 )   return 3;                // is OrderNum > 2
 if ( S3M_header->InsNum == 0 )  return 3;                // is InsNum > 0
 if ( S3M_header->PatNum == 0 )  return 3;                // is PatNum > 0

// * Set pointers in S3M for Orders etc. *

 ChannelSets = (char *)(S3M_Buffer + 0x40);       // Channel settings
 Orders = (char *)(S3M_Buffer + 0x60);            // Pattern Order list
 DefaultPan = (char *)(S3M_Buffer + 0x60 + S3M_header->OrdNum + (S3M_header->InsNum)*2 + (S3M_header->PatNum)*2 );
                                                  // Default Panning table

// * Get memory for pointers *

 InstrumentPointer = (unsigned long *) malloc ( (S3M_header->InsNum) * 4 );
 SamplePointer = (unsigned long *) malloc ( (S3M_header->InsNum) * 4 );
 PatternPointer = (unsigned long *) malloc ( (S3M_header->PatNum) * 4 );

 unsigned short *TestPointer = (unsigned short *)(S3M_Buffer + 0x60 + S3M_header->OrdNum);

 for (test=0; test < S3M_header->InsNum; test++)    // set Ins & Smp pointers
     {
      InstrumentPointer[test] = (unsigned long)(S3M_Buffer + ((unsigned long)TestPointer[test] << 4) );
      SamplePointer[test] = (unsigned long)(S3M_Buffer + ( ((unsigned long)*(unsigned short *)(InstrumentPointer[test] + 0x0e)  +  ((unsigned long)(*(unsigned char *)(InstrumentPointer[test] + 0x0d)) << 16) << 4) ) );
      *(unsigned char *)(InstrumentPointer[test] + 0x0d) = 0;  // to terminate sample name
     }

 TestPointer = (unsigned short *)( (S3M_Buffer + 0x60 + S3M_header->OrdNum) + (S3M_header->InsNum * 2) );

 for (test=0; test < S3M_PatternNum; test++)    // set Pattern pointers
      if ( TestPointer[test] != NULL )
          PatternPointer[test] = (unsigned long)(S3M_Buffer + ((unsigned long)TestPointer[test] << 4) );
      else
          PatternPointer[test] = NULL;


 // *** convert unsigned samples into signed ones ***

 if ( S3M_header->File_Format_Info == 2)     // if samples are unsigned

    for (test=0; test < S3M_header->InsNum; test++)  // convert all
        {
         if ( (*(unsigned char *)(InstrumentPointer[test] + 0)) == 1 )
            if ( (*(unsigned char *)(InstrumentPointer[test] + 0x1F) &4 ) == 0 )
                if ( (*(unsigned char *)(InstrumentPointer[test] + 0x1F) &2 ) == 0 )
                    ConvU8MtoS8M ( (char *)SamplePointer[test], *(unsigned long *)(InstrumentPointer[test] + 0x10) );
                else
                    ConvU8MtoS8M ( (char *)SamplePointer[test], *(unsigned long *)(InstrumentPointer[test] + 0x10)*2 );
            else
                if ( (*(unsigned char *)(InstrumentPointer[test] + 0x1F) &2 ) == 0 )
                    ConvU16MtoS16M ( (char *)SamplePointer[test], *(unsigned long *)(InstrumentPointer[test] + 0x10) );
                else
                    ConvU16MtoS16M ( (char *)SamplePointer[test], *(unsigned long *)(InstrumentPointer[test] + 0x10)*2 );
        }

 // *** prepare S3M variables ***

 S3M_initvariables();

 S3M_GlobalVolume = S3M_header->Global_Volume;
 MasterVolume = (S3M_header->Master_Volume & 127) * 256;
 S3M_flag_stereo = S3M_header->Master_Volume >> 7;

 S3M_flag_amigalimits = (S3M_header->Flags & 16) >> 4;
 S3M_flag_st3volslides = (S3M_header->Flags & 64) >> 6;
 if ( S3M_header->Tracker_version == 0x1300 )  S3M_flag_st3volslides = 1;

 S3M_tempo = S3M_header->Init_Tempo;
 S3M_speed = S3M_header->Init_Speed;

 S3M_channels = 0;

 for ( channel = 0; channel < 32; channel++)
      if ((ChannelSets[channel] & 127) < 16)  S3M_channels++;  // calculate used channels

 if ( (S3M_header->Default_Panning == 0xFC) )  // check if default pan available
    {                                         // 0xFC seems to be a Future Crew joke :)
     for ( channel = 0; channel < 32; channel++ )
         {
          if (S3M_flag_stereo == 1)   S3M_Panning[channel] = 0x03 + ((ChannelSets[channel] & 15) >> 3) * 0x09;

          if ((DefaultPan[channel] & 32) == 32)
             {
              S3M_Panning[channel] = DefaultPan[channel] & 15;
             }
        }
    }

 // *** calculating and preparing S3M variables like speed etc. ***

 S3M_TickLength = DAT_calcSpeed ( S3M_tempo, MixSpeed );

 LastPattern = 65535;         // no pattern last worked on
 LastRow = 65535;             // the same as above for row

 S3M_row = 0;
 S3M_jump = 0;
 S3M_tick = 0;
 S3M_tick_rest = S3M_TickLength;
 S3M_Order = 0;
 S3M_Pattern = Orders[S3M_Order];

 while ( S3M_Pattern > 99 )
       {
        S3M_Order++;
        S3M_Pattern = Orders[S3M_Order];
        if ( S3M_Pattern == 255 )  return 3;
       }

 S3M_unpack_row ( S3M_Pattern, S3M_row );   // unpack first Orderpattern, first row
 S3M_read_row();

 S3M_initEffects();

 // some test code...

 printf("Name: %s\n",S3M_header->Song_name);
 printf("S3M:  %u  %x\n", S3M_Buffer, S3M_Buffer[0]);
 //printf("Ins1: %u  %x  %s\n", InstrumentPointer[0], *(unsigned char *)InstrumentPointer[0], (char *)(InstrumentPointer[0] + 1));
 //printf("Smp1: %u  %x\n", SamplePointer[0], *(char *)SamplePointer[0] );
 //printf("Ins2: %u  %x  %s\n", InstrumentPointer[1], *(unsigned char *)InstrumentPointer[1], (char *)(InstrumentPointer[1] + 1));
 //printf("Smp2: %u  %x\n", SamplePointer[1], *(char *)SamplePointer[1] );
 //printf("Pattern1: %u\n", PatternPointer[0] );
 printf("Channels: %u\n",S3M_channels);
 printf("Pan1: %u  Pan2: %u\n",S3M_Panning[0], S3M_Panning[1]);

 close_file_S3M ();  // close S3M file coz it is ready for playing

 flag_DatReady = 1;

return 0;
}


short close_S3M (void)
{

 close_file_S3M ();

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

 if ( S3M_Buffer == NULL )  return 1;

 free ( S3M_Buffer );
 S3M_Buffer = NULL;

return 0;
}


// **************************** Pattern functions ***************************

static void clear_Row ()             // clear row buffer with default values
{
 unsigned long counter;

 for ( counter = 0; counter < 160; counter+= 20)
     {
      *(long *)(S3M_RowBuffer +counter+0)  = 0xffff00ff;
      *(long *)(S3M_RowBuffer +counter+4)  = 0xff00ff00;
      *(long *)(S3M_RowBuffer +counter+8)  = 0x00ff00ff;
      *(long *)(S3M_RowBuffer +counter+12) = 0xff00ffff;
      *(long *)(S3M_RowBuffer +counter+16) = 0x00ffff00;
     }
}

static char *LastRowPointer = NULL;

static void unpack_firstRow ( unsigned long PatternAdress )
{
 char *Pointer;
 char Value;
 unsigned long ChannelNr;

 S3M_nextrow = 0;  // reset jumppoint for SBx

 clear_Row ();

 Pointer = (char *)PatternAdress;

 if ( PatternAdress == NULL )
     {
      LastRowPointer = NULL;
      return;  // empty pattern
     }

 Pointer+=2;

 while ( *Pointer != 0 )
  {
   Value = *Pointer++;

   if ( Value != 0 )
      {
       ChannelNr = Value & 31;

       if ( (Value & 32) == 32 )
          {
           S3M_RowBuffer[ChannelNr*5+0] = *Pointer++;
           S3M_RowBuffer[ChannelNr*5+1] = *Pointer++;
          }
       if ( (Value & 64) == 64 )
          {
           S3M_RowBuffer[ChannelNr*5+2] = *Pointer++;
          }
       if ( (Value & 128) == 128 )
          {
           S3M_RowBuffer[ChannelNr*5+3] = *Pointer++;
           S3M_RowBuffer[ChannelNr*5+4] = *Pointer++;
          }

      }
  }

  Pointer++;
  LastRowPointer = Pointer;

}

static void unpack_nextRow ( )
{
 char *Pointer;
 char Value;
 unsigned long ChannelNr;

 clear_Row ();

 Pointer = LastRowPointer;

 if ( LastRowPointer == NULL )  return;  // empty pattern

 while ( *Pointer != 0 )
  {
   Value = *Pointer++;

   if ( Value != 0 )
      {
       ChannelNr = Value & 31;

       if ( (Value & 32) == 32 )
          {
           S3M_RowBuffer[ChannelNr*5+0] = *Pointer++;
           S3M_RowBuffer[ChannelNr*5+1] = *Pointer++;
          }
       if ( (Value & 64) == 64 )
          {
           S3M_RowBuffer[ChannelNr*5+2] = *Pointer++;
          }
       if ( (Value & 128) == 128 )
          {
           S3M_RowBuffer[ChannelNr*5+3] = *Pointer++;
           S3M_RowBuffer[ChannelNr*5+4] = *Pointer++;
          }

      }
  }

  Pointer++;
  LastRowPointer = Pointer;

}


static void S3M_unpack_row( unsigned short PatternNr, unsigned short RowNr )
{
 unsigned long counter;

 if ( PatternNr >= S3M_PatternNum )
    {
     clear_Row ();
     LastPattern = PatternNr;
     LastRow = RowNr;
     return;
    }

 if ( PatternNr == LastPattern )
    {
     if ( RowNr == LastRow )  return;  // the same line again ;)
     if ( RowNr == (LastRow+1) )  {  unpack_nextRow ();  }
     else
         {
          if ( RowNr < LastRow )
             {
              unpack_firstRow ( PatternPointer[PatternNr] );
              if ( RowNr != 0 )  for (counter=0; counter < RowNr; counter++)  unpack_nextRow ();
             }
          else  for (counter=0; counter < (RowNr-LastRow); counter++)  unpack_nextRow ();
         }
    }
 else
    {
     unpack_firstRow ( PatternPointer[PatternNr] );
     if ( RowNr != 0 )
          {
           for (counter=0; counter < RowNr; counter++)  unpack_nextRow ();
          }
    }

 LastPattern = PatternNr;
 LastRow = RowNr;

}


unsigned long Calc_st3periode ( uns char Note, uns char Octave, unsigned long C4SPD )
{
unsigned long noteperiode, st3periode, dum;

//               8363 * 16 * ( periode(NOTE) >> octave(NOTE) )
// st3_periode = ---------------------------------------------
//                       C4SPDfinetune(SAMPLE)

 noteperiode = (uns long)S3M_NotePeriodes[ Note ] << 4; // extend by 4 bits for fixkomma
 dum = (uns long)(8363*16* ( noteperiode >> Octave ));
 st3periode = dum / C4SPD;   // periode but extended by 16

return st3periode;   // extended by 16 = 4bit fixpoint
}


static unsigned long Calc_st3frequence ( unsigned short channel )
{
// st3frequence = 14317456 / st3periode
// but we must remeber that here st3periode is extended by 4 bits
// and we will let it extended by 2 bits

static unsigned long dum;

if ( S3M_Periode[channel] > 1000 )  dum = S3M_Periode[channel];
else dum = 1000;

return  ( (14317456L << 6) / ( dum + S3M_PeriodeAdjust[channel] ));
}


void S3M_goRowOrder()
{
 if (S3M_jump == 0)
    {
     if (S3M_flag_note == 0)  S3M_row++;

     if ( S3M_row == 64 )
        {
         S3M_Order++;
         S3M_row = 0;
         S3M_Pattern = Orders[ S3M_Order ];

         while ( S3M_Pattern == 254 )
               {
                S3M_Order++;
                S3M_Pattern = Orders[ S3M_Order ];
               }

         if ( S3M_Pattern == 255 )
            {
             S3M_Order = 0;
             S3M_Pattern = Orders[ S3M_Order ];
             if ( flag_loop != 1 )
                {
                 //toplay = S3M_buffer_rest;  // to set S3M_buffer_rest = 0
                 flag_DatReady = 2;
                }
            }
        }
    }
else
    {
     S3M_jump = 0;
     if (S3M_nextrow > 63)  return;
     if (S3M_nextorder > S3M_OrderNum)  return;
     S3M_row = S3M_nextrow;
     S3M_nextrow = 0;
     S3M_Order = S3M_nextorder;
     S3M_nextorder = 0;

     S3M_Pattern = Orders[ S3M_Order ];

     while ( S3M_Pattern == 254 )
           {
            S3M_Order++;
            S3M_Pattern = Orders[ S3M_Order ];
           }

     if ( S3M_Pattern == 255 )
        {
         S3M_Order = 0;
         S3M_Pattern = Orders[ S3M_Order ];
         if ( flag_loop != 1 )
            {
             //toplay = S3M_buffer_rest;  // to set S3M_buffer_rest = 0
             flag_DatReady = 2;
            }
        }
    }
}

// **************************** S3M tick routines ***************************

void GetNewSample ( unsigned short channel )
{
 unsigned short SampleNr;
 unsigned char SampleBits, SampleStereo;

 SampleNr = S3M_SampleNr[ channel ] - 1 ;

 if ( SampleNr <= S3M_header->InsNum )
 if ( *(char *)InstrumentPointer[SampleNr] == 1 )
 {
  S3M_SampleAdress[channel] = SamplePointer[SampleNr];
  S3M_SampleFlags[channel] = *(unsigned char *)( InstrumentPointer[SampleNr] + 0x1f );
  SampleBits = (S3M_SampleFlags[channel] & 4) >> 2;
  SampleStereo = (S3M_SampleFlags[channel] & 2) >> 1;
  S3M_SampleLength[channel] = (*(unsigned long *)( InstrumentPointer[SampleNr] + 0x10 )) >> (SampleBits+SampleStereo);
  S3M_SampleLoopBegin[channel] = (*(unsigned long *)( InstrumentPointer[SampleNr] + 0x14 )) >> (SampleBits+SampleStereo);
  S3M_SampleLoopEnd[channel] = (*(unsigned long *)( InstrumentPointer[SampleNr] + 0x18 )) >> (SampleBits+SampleStereo);
  S3M_SampleC4SPD[channel] = *(unsigned long *)( InstrumentPointer[SampleNr] + 0x20 );
  TremorVolume[channel] = S3M_Volume[channel] = ( S3M_GlobalVolume * *(unsigned char *)( InstrumentPointer[SampleNr] + 0x1c ) + 32 ) / 64;

 }
 else   S3M_SampleNr[ channel ] = 0;
 else   S3M_SampleNr[ channel ] = 0;
}

void GetNewNote ( unsigned short channel )
{
 if ( S3M_SampleNr[channel] != 0 )
    {
     if ( (S3M_Note[channel] > 11) )
         {
          S3M_Periode[channel] = 0;
          S3M_ChannelActiv[channel] = 0;
         }
     else
         {
          S3M_Periode[channel] = GlissPeriode[channel] = Calc_st3periode ( S3M_Note[channel], S3M_Octave[channel], S3M_SampleC4SPD[channel] );
          S3M_SamplePosition[channel] = 0;
          S3M_SampleFraction[channel] = 0;
          S3M_ChannelActiv[channel] = 1;
         }
    }
 else   S3M_ChannelActiv[channel] = 0;
}


void GetNewEffect ()
{
 unsigned long channel;
 unsigned short Effect, Info;

 for (channel = 0; channel < S3M_channels; channel++)
     {
      Effect = S3M_Effect[channel];
      Info = S3M_EffectInfo[channel];

      // reset of variables depending on commands

      switch (Effect)
             {
              case 255 : // no effect
                         S3M_PeriodeAdjust[channel] = 0;
                         break;
              case  1 : // Axx
              case  2 : // Bxx
              case  3 : // Cxx
                        VibratoPosition[channel] = 0;
                        break;
              case  4 : // Dxy
                        S3M_PeriodeAdjust[channel] = 0;
                        break;
              case  5 : // Exx
              case  6 : // Fxx
                        S3M_Periode[channel] += S3M_PeriodeAdjust[channel];
                        S3M_PeriodeAdjust[channel] = 0;
                        VibratoPosition[channel] = 0;
                        break;
              case  7 : // Gxx
                        VibratoPosition[channel] = 0;
                        break;
              case  8 : // Hxy
                        break;
              case  9 : // Ixy
              case 10 : // Jxy
                        VibratoPosition[channel] = 0;
                        break;
              case 11 : // Kxy
                        break;
              case 12 : // Lxy
              case 13 : // M--
              case 14 : // N--
              case 15 : // Oxx
              case 16 : // P--
              case 17 : // Qxy
                        VibratoPosition[channel] = 0;
                        break;
              case 18 : // Rxy
                        break;
              case 19 : // Sxy
              case 20 : // Txx
                        VibratoPosition[channel] = 0;
                        break;
              case 21 : // Uxy
                        break;
              case 22 : // Vxx
              case 23 : // W--
              case 24 : // Xxx
              case 25 : // Y--
              case 26 : // Z--
                        VibratoPosition[channel] = 0;
                        break;
             }

      // reloading last effect info if info is zero

      if ( Info == 0 )

      switch (Effect)
             {
              case 255 : // no effect
                         break;
              case  1 : // Axx
              case  2 : // Bxx
              case  3 : // Cxx
                        break;
              case  4 : // Dxy
              case  5 : // Exx
              case  6 : // Fxx
                        S3M_EffectInfo[channel] = S3M_LastEffectInfo[channel];
                        break;
              case  7 : // Gxx
              case  8 : // Hxy
                        break;
              case  9 : // Ixy
              case 10 : // Jxy
              case 11 : // Kxy
              case 12 : // Lxy
              case 13 : // M--
              case 14 : // N--
              case 15 : // Oxx
              case 16 : // P--
              case 17 : // Qxy
                        S3M_EffectInfo[channel] = S3M_LastEffectInfo[channel];
                        break;
              case 18 : // Rxy
              case 19 : // Sxy
              case 20 : // Txx
              case 21 : // Uxy
              case 22 : // Vxx
              case 23 : // W--
              case 24 : // Xxx
              case 25 : // Y--
              case 26 : // Z--
                        break;
             }
     }

 S3M_GlobalEffect();
 S3M_beforeEffect();
}


void S3M_read_row ()
{
 unsigned long channel;
 char value;

 // S3M effect

 for (channel = 0; channel < S3M_channels; channel++)
      {

       value = S3M_RowBuffer[channel*5+4];  // effect info
       if ( S3M_EffectInfo[channel] != 0 )
          {
           S3M_LastEffectInfo[channel] = S3M_EffectInfo[channel];
           if (S3M_LastEffectInfo[channel] == 0)  S3M_LastEffectInfo[channel] = value;
          }
       S3M_EffectInfo[channel] = value;

       value = S3M_RowBuffer[channel*5+3];  // effect
       if ( value != 255 )
          {
           S3M_LastEffect[channel] = S3M_Effect[channel];
           if (S3M_LastEffect[channel] == 0)  S3M_LastEffect[channel] = value;
          }
      S3M_Effect[channel] = value;
      }

 GetNewEffect();


 // S3M note+instrument+volume

 if ( S3M_flag_note == 0 )    // then read note, volume and instrument
    {
     for (channel = 0; channel < S3M_channels; channel++)
         {

          value = S3M_RowBuffer[channel*5+1];  // instrument
          if ( value != 0 )
             {
              S3M_SampleNr[channel] = value;
              GetNewSample ( channel );
             }

          value = S3M_RowBuffer[channel*5+2];  // volume
          if ( value != 255 )
             {
              S3M_Volume[channel] = ( S3M_GlobalVolume * value + 32 ) / 64;
              TremorVolume[channel] = value;
             }

          value = S3M_RowBuffer[channel*5+0];  // note + octave
          if ( value != 255 )
             {
              if ( value == 254 )  // key off
                 S3M_ChannelActiv[channel] = 0;    // stop smp
              else
                 {
                  S3M_Note[channel] = value & 15;
                  S3M_Octave[channel] = value >> 4;
                  GetNewNote ( channel );
                  VibratoPosition[channel] = 0;   // reset Vibrato
                  S3M_PeriodeAdjust[channel] = 0;
                 }
             }
         }     // for channel
    }  // flag test S3M_noteflag

 S3M_RowEffect();

}


void S3M_initvariables()
{
 unsigned long channel;

  LastPattern = 65535;        // no last pattern
  LastRow = 65535;            // no last row exists

  S3M_TickLength = 0;         // how many samples to make one frame

  S3M_row = 0;
  S3M_jump = 0;
  S3M_nextrow = 0;
  S3M_nextorder = 0;
  S3M_tick = 0;
  S3M_Order = 0;
  S3M_Pattern = 0;

  S3M_buffer_rest = 0;
  S3M_tick_rest = 0;

  S3M_flag_stereo = 0;        // 0 = mono, 1 = stereo S3M
  S3M_flag_amigalimits = 0;   // 0 = no amiga limits
  S3M_flag_st3volslides = 0;  // 1 = volume slide on EVERY frame
  S3M_flag_note = 0;

  S3M_tempo = 128;            // tempo of S3M
  S3M_speed = 6;              // speed of S3M
  S3M_GlobalVolume = 64;      // global volume of S3M, 64 = max

 for (channel = 0; channel < 32; channel++ )
     {
      S3M_RowBuffer[channel*5] = 0;        // buffer where row is uncompressed

      S3M_ChannelActiv[channel] = 0;       //  0 != can be active

      S3M_SampleNr[channel] = 0;
      S3M_SampleAdress[channel] = 0;
      S3M_SampleLength[channel] = 0;
      S3M_SamplePosition[channel] = 0;
      S3M_SampleFraction[channel] = 0;
      S3M_SampleLoopBegin[channel] = 0;
      S3M_SampleLoopEnd[channel] = 0;
      S3M_SampleC4SPD[channel] = 0;
      S3M_SampleFlags[channel] = 0;

      S3M_Volume[channel] = 0;
      S3M_Panning[channel] = 7;
      S3M_Note[channel] = 0;
      S3M_Octave[channel] = 0;
      S3M_Effect[channel] = 0;
      S3M_EffectInfo[channel] = 0;
      S3M_LastEffect[channel] = 0;
      S3M_LastEffectInfo[channel] = 0;

      S3M_Periode[channel] = 0;
      S3M_PeriodeAdjust[channel] = 0;
      S3M_Frequence[channel] = 0;
     }

}

// ****************************** S3M interrupt *****************************

void S3M_to_Mixer ()
{
unsigned long channel;

 ChannelLast=S3M_channels;

 for ( channel = 0; channel < S3M_channels; channel++ )
  {
   if ( S3M_Periode[channel] < 60 )  S3M_ChannelActiv[channel] = 0;
   ChannelActiv[channel] = S3M_ChannelActiv[channel];

   ChannelVolume[channel] = S3M_Volume[channel] << 2;
   ChannelPanning[channel] = S3M_PanningTable[ S3M_Panning[channel] ];

   if ( S3M_ChannelActiv[channel] != 0 )
      {
       ChannelSamplePosition[channel] = S3M_SamplePosition[channel];
       ChannelSampleFraction[channel] = S3M_SampleFraction[channel];
       S3M_Frequence[channel] = Calc_st3frequence( channel );
       ChannelSampleFrequence[channel] = (S3M_Frequence[channel] + 2) >> 2;

       if ( S3M_SampleNr[channel] != 0 )
       if ( ChannelSampleNr[channel] != S3M_SampleNr[channel] )
          {
           ChannelSampleNr[channel] = S3M_SampleNr[channel];
           ChannelSampleBits[channel] = ((S3M_SampleFlags[channel] & 4) << 1) + 8;
           //if (ChannelSampleBits[channel] == 16 ) printf("16 bit !!\n");
           ChannelSampleMode[channel] = (S3M_SampleFlags[channel] & 2) >> 1;
           ChannelSampleAdress[channel] = S3M_SampleAdress[channel];
           ChannelSampleLength[channel] = S3M_SampleLength[channel];
           ChannelLoopMode[channel] = S3M_SampleFlags[channel] & 1;
           ChannelLoopBegin[channel] = S3M_SampleLoopBegin[channel];
           ChannelLoopEnd[channel] = S3M_SampleLoopEnd[channel];
          }
      }
  }
}


void Mixer_to_S3M ()
{
unsigned long channel;

 for ( channel = 0; channel < S3M_channels; channel++ )
  {

   if ( S3M_ChannelActiv[channel] != 0 )
      {
       S3M_SamplePosition[channel] = ChannelSamplePosition[channel];
       S3M_SampleFraction[channel] = ChannelSampleFraction[channel];
       S3M_ChannelActiv[channel] = ChannelActiv[channel];
      }

  }
}



void interupt_S3M ()
{
 static unsigned long toplay = 0;

 S3M_buffer_rest = block_length;
 MixerAdress = 0;
 clearMixer();

while ( S3M_buffer_rest > 0 )
      {

       if ( S3M_tick_rest <= S3M_buffer_rest )
          {
          toplay = S3M_tick_rest;

          S3M_to_Mixer();
          DoMixing ( toplay );
          MixerAdress += toplay;
          Mixer_to_S3M();

          //S3M_tick_rest-= toplay;

          S3M_tick++;

          if ( S3M_tick == S3M_speed )
             {
              S3M_tick=0;

              S3M_goRowOrder();

              S3M_unpack_row ( S3M_Pattern, S3M_row );
              S3M_read_row();
             }
          else
               S3M_TickEffect();

          S3M_buffer_rest-= toplay;
          S3M_tick_rest = S3M_TickLength;

         }
      else
         {
          toplay = S3M_buffer_rest;

          S3M_to_Mixer();
          DoMixing ( toplay );
          MixerAdress += toplay;
          Mixer_to_S3M();

          S3M_tick_rest-= toplay;
          S3M_buffer_rest = 0;
         }

      }

 DoMasterVolumeCalculations( MixerBlock, Mixerpointer, block_length );
}
