// S3M - Routines for F/X Player v2.0
// (c) 1997-98 by Apollo / STIGMA

#include <stdio.h>
#include <stdlib.h>
//#include <dos.h>
#include <string.h>
#include <conio.h>

#include "fx.h"

//#include "irq.h"
//#include "dma.h"

//#include "device.h"
//#include "dev_sb.h"

//#include "format.h"
#include "dat_calc.h"
#include "dat_S3M.h"

//#include "convert.h"
//#include "mixer.h"


// Tables

static signed short Sinus[64] =
                         {    0,  100,  200,  297,  392,  483,  569,  650,
                            724,  792,  851,  903,  946,  980, 1004, 1019,
                           1024, 1019, 1004,  980,  946,  903,  851,  792,
                            724,  650,  569,  483,  392,  297,  200,  100,
                             -0, -100, -200, -297, -392, -483, -569, -650,
                           -724, -792, -851, -903, -946, -980,-1004,-1019,
                          -1024,-1019,-1004, -980, -946, -903, -851, -792,
                           -724, -650, -569, -483, -392, -297, -200, -100  };

static signed short Ramp[64] =
                         {-1024, -992, -960, -928, -896, -864, -832, -800,
                           -768, -736, -704, -672, -640, -608, -576, -544,
                           -512, -480, -448, -416, -384, -352, -320, -288,
                           -256, -224, -192, -160, -128,  -96,  -64,  -32,
                              0,   32,   64,   96,  128,  160,  192,  224,
                            256,  288,  320,  352,  384,  416,  448,  480,
                            512,  544,  576,  608,  640,  672,  704,  736,
                            768,  800,  832,  864,  896,  928,  960,  992  };

static signed short Square[64] =
                         {  1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                            1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                            1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                            1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                               0,    0,    0,    0,    0,    0,    0,    0,
                               0,    0,    0,    0,    0,    0,    0,    0,
                               0,    0,    0,    0,    0,    0,    0,    0,
                               0,    0,    0,    0,    0,    0,    0,    0  };

static signed short Random[64] =
                         {   235,  761, -875,  565,  677,  789, -557, -349,
                             905,  378, -378,  480,  609, -426, -140, -801,
                             730,  -48,  807, -134,   60,  932,  417, -315,
                             512,  869, -184, 1006,-1024,  551, -311,  110,
                               8, -415,  449,   62,  637, -340,  -39, -291,
                             -27, -963,   35,  -33,  447, -740,  907,  -50,
                            -149,  350,   62,  562, -430,  606,  410,  484,
                            -252,  405,  -64, -314, -272, -578, -972,  101  };

static signed short *Vibrato[S3M_Maxchannels];
static signed short *Tremolo[S3M_Maxchannels];

// *** direct effects ***

// VOLUME

static unsigned short S3M_addVolume ( short Volume, short value)
{
 Volume += value;

 if (Volume > 64)  Volume = 64;
 else
 if (Volume < 0)   Volume = 0;

 return Volume;
}

static unsigned short S3M_decVolume ( short Volume, short value)
{
 Volume -= value;

 if (Volume < 0)   Volume = 0;
 else
 if (Volume > 64)  Volume = 64;

 return Volume;
}

// TONE SLIDE

static unsigned long S3M_addPeriode ( signed long Periode, unsigned long value)
{
 Periode += value;

 if (S3M_flag_amigalimits == 1)
    {
     if ( Periode < 7232 )  Periode = 7232;
     if ( Periode > 54784 ) Periode = 54784;
    }
 else
      if ( Periode < 59 )  Periode = 59;

 return Periode;
}

static unsigned long S3M_decPeriode ( signed long Periode, unsigned long value)
{
 Periode -= value;

 if (S3M_flag_amigalimits == 1)
    {
     if ( Periode < 7232 )  Periode = 7232;
     if ( Periode > 54784 ) Periode = 54784;
    }
 else
      if ( Periode < 59 )  Periode = 59;

 return Periode;
}

// GLISSANDO

static unsigned char  GlissSpeed[S3M_Maxchannels];
static unsigned char  GlissNote[S3M_Maxchannels];
static unsigned char  GlissOctave[S3M_Maxchannels];
       unsigned long  S3M_GlissPeriode[S3M_Maxchannels];
static unsigned char  GlissFlag[S3M_Maxchannels];

// VIBRATO

static unsigned char  VibratoSpeed[S3M_Maxchannels];
static unsigned char  VibratoDepth[S3M_Maxchannels];
       unsigned short S3M_VibratoPosition[S3M_Maxchannels];

void S3M_Vibrato( unsigned long channel )
{
 if ( VibratoSpeed[channel] == 0 )  return;

 S3M_PeriodeAdjust[channel] = *(signed short *)(Vibrato[channel] + S3M_VibratoPosition[channel]) * VibratoDepth[channel] / 8;
 S3M_VibratoPosition[channel] = (S3M_VibratoPosition[channel] + VibratoSpeed[channel]) & 63;
}

void S3M_FineVibrato( unsigned long channel )
{
 if ( VibratoSpeed[channel] == 0 )  return;

 S3M_PeriodeAdjust[channel] = *(signed short *)(Vibrato[channel] + S3M_VibratoPosition[channel]) * VibratoDepth[channel] / 64;
 S3M_VibratoPosition[channel] = (S3M_VibratoPosition[channel] + VibratoSpeed[channel]) & 63;
}

// TREMOR

       unsigned short S3M_TremorVolume[S3M_Maxchannels];
static unsigned char  TremorCountOn[S3M_Maxchannels];
static unsigned char  TremorCountOff[S3M_Maxchannels];

static void S3M_Tremor( unsigned long channel, unsigned char countOn, unsigned char countOff )
{
 if ( TremorCountOn[channel] != 0 )
    {
     S3M_Volume[channel] = S3M_TremorVolume[channel];
     TremorCountOn[channel]--;
     TremorCountOff[channel] = countOff;
    }
 else
    {
     S3M_Volume[channel] = 0;
     TremorCountOff[channel]--;
     if ( TremorCountOff[channel] == 0 ) TremorCountOn[channel] = countOn;
    }
}

// RETRIG

static signed char S3M_RetrigCount[S3M_Maxchannels];

static void S3M_Retrig( unsigned long channel, unsigned char vol, signed char count )
{
 S3M_RetrigCount[channel]--;

 if (S3M_RetrigCount[channel] == 0)
    {
     S3M_RetrigCount[channel] = count;
     S3M_SamplePosition[channel] = 0;
     S3M_SampleFraction[channel] = 0;
     switch (vol)
            {
             case 0  : // +0
                      break;
             case 1  : // -1
                       S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 1);
                       break;
             case 2  : // -2
                       S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 2);
                       break;
             case 3  : // -4
                       S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 4);
                       break;
             case 4  : // -8
                       S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 8);
                       break;
             case 5  : // -16
                       S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel], 16);
                       break;
             case 6  : // *0.625
                       S3M_Volume[channel] = (uns char)((uns short)S3M_Volume[channel] * 625 / 1000);
                       break;
             case 7  : // *0.5
                       S3M_Volume[channel] = S3M_Volume[channel] >> 1;
                       break;
             case 8  : // ?
                       break;
             case 9  : // +1
                       S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 1);
                       break;
             case 10 : // +2
                       S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 2);
                       break;
             case 11 : // +4
                       S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 4);
                       break;
             case 12 : // +8
                       S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 8);
                       break;
             case 13 : // +16
                       S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], 16);
                       break;
             case 14 : // *1.5
                       S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], S3M_Volume[channel] >> 1 );
                       break;
             case 15 : // *2
                       S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel], S3M_Volume[channel]);
                       break;
        }
    }
}

// TREMOLO

void S3M_Tremolo( unsigned long channel )
{
 if ( VibratoSpeed[channel] == 0 )  return;

 S3M_Volume[channel] = S3M_addVolume( S3M_TremorVolume[channel], *(signed short *)(Tremolo[channel] + S3M_VibratoPosition[channel]) * VibratoDepth[channel] / 512 );
 S3M_VibratoPosition[channel] = (S3M_VibratoPosition[channel] + VibratoSpeed[channel]) & 63;
}

// SPECIAL

   // NOTE DELAY

   static unsigned char NoteDelayFlag[S3M_Maxchannels];      // flag
   static unsigned char NoteDelayNote[S3M_Maxchannels];
   static unsigned char NoteDelayInst[S3M_Maxchannels];
   static unsigned char NoteDelayVol[S3M_Maxchannels];

   // SBx Loop Pattern

   signed long PatLoopCount = 0;

// *** control over effects ***

void S3M_GlobalEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned short x,y;

 short jumpbreak = 0;
 short special = 0;
 short tempo = 0;
 short global = 0;

 for (channel = 0; channel < S3M_channels; channel++)
 {
 Effect = S3M_Effect[channel];
 Info = S3M_EffectInfo[channel];
 x = Info >> 4;
 y = Info & 15;

 switch (Effect)
        {
         case 255 : break; // no effect
         case  1 :  // Axx : set speed to xx (l -> r)
                   if (Info == 0)  break;
                   S3M_speed = Info;
                   break;

         case  2 :  // Bxx : jump to order xx (l <- r)
                   if (jumpbreak != 0)  break;
                   //printf("Jump!\n");
                   S3M_nextorder = Info;
                   S3M_nextrow = 0;
                   S3M_jump = 1;
                   jumpbreak = 1;
                   break;

         case  3 :  // Cxx : break pattern to row xx (l <- r) (BCD!!)
                   if (jumpbreak != 0)  break;
                   Info = (Info >> 4) * 10 + (Info & 15);
                   S3M_nextorder = S3M_Order + 1;
                   S3M_nextrow = Info;
                   S3M_jump = 1;
                   jumpbreak = 1;
                   break;

         case 19 :  // Sxy : special command x
                   switch (x)
                          {
                           case    0 :
                           case 0x01 :
                           case 0x02 :
                                       break;
                           case 0x03 : // set vibrato waveform
                                       if ( (y & 3) == 0 )  Vibrato[channel] = Sinus;
                                       if ( (y & 3) == 1 )  Vibrato[channel] = Ramp;
                                       if ( (y & 3) == 2 )  Vibrato[channel] = Square;
                                       if ( (y & 3) == 3 )  Vibrato[channel] = Random;
                                       break;
                           case 0x04 : // set tremolo waveform
                                       if ( (y & 3) == 0 )  Tremolo[channel] = Sinus;
                                       if ( (y & 3) == 1 )  Tremolo[channel] = Ramp;
                                       if ( (y & 3) == 2 )  Tremolo[channel] = Square;
                                       if ( (y & 3) == 3 )  Tremolo[channel] = Random;
                                       break;
                           case 0x08 : // set pan position
                                       S3M_Panning[channel] = y;
                                       break;
                           case 0x0a : // old panning position (from Panic)
                                       switch (y)
                                              {
                                               case 0: // L
                                                       S3M_Panning[channel] = 3;
                                                       break;
                                               case 1: // R
                                                       S3M_Panning[channel] = 12;
                                                       break;
                                               case 2: // LL
                                                       S3M_Panning[channel] = 0;
                                                       break;
                                               case 3: // RR
                                                       S3M_Panning[channel] = 15;
                                                       break;
                                               case 4: // LL-R
                                                       S3M_Panning[channel] = 5;
                                                       break;
                                               case 5: // L-RR
                                                       S3M_Panning[channel] = 10;
                                                       break;
                                               case 6: // L-R
                                                       S3M_Panning[channel] = 7;
                                                       break;
                                               case 7: // LL-RR
                                                       S3M_Panning[channel] = 8;
                                                       break;
                                              }
                                       break;

                           case 0x0b : // start (y=0) / loop (y=count)
                                       if (y == 0)
                                          {
                                           S3M_nextrow = S3M_row;
                                           break;
                                          }
                                       else
                                       if (PatLoopCount == 0)
                                          PatLoopCount = y + 1;
                                       PatLoopCount --;
                                       if (PatLoopCount != 0)
                                          {
                                           S3M_nextorder = S3M_Order;
                                           S3M_jump = 1;
                                          }
                                       else
                                          {
                                           S3M_nextrow = S3M_row + 1;
                                           S3M_jump = 0;
                                          }
                                       break;

                          }
                   break;

         case 20 :  // Txx : set tempo to xx (20h-ffh) (l <- r)
                   if (tempo != 0)  break;
                   if (Info < 0x21)  break;
                   S3M_tempo = Info;
                   S3M_TickLength = DAT_calcSpeed ( S3M_tempo, MixSpeed );
                   tempo = 1;
                   break;

         case 22 :  // Vxx : set global volume (0h-40h) (l <- r)
                   if (global != 0)  break;
                   if (Info > 0x40)  break;
                   S3M_GlobalVolume = Info;
                   global = 1;
                   break;
        }
  }
}


void S3M_beforeEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned short x,y;
 static unsigned char value;


 for (channel = 0; channel < S3M_channels; channel++)
     {
      // and the usuall stuff

      Effect = S3M_Effect[channel];
      Info = S3M_EffectInfo[channel];
      x = Info >> 4;
      y = Info & 15;

      switch (Effect)
             {
              case 255 : break; // nothing

              case  7 : // Gxx
                        value = S3M_RowBuffer[channel*5+0];  // note + octave
                        if ( (value != 255) && (value != 254) )
                           {
                            S3M_PeriodeAdjust[channel] = 0;
                            GlissNote[channel] = value & 15;
                            GlissOctave[channel] = value >> 4;
                            if ((GlissNote[channel] < 13) && (GlissOctave[channel] < 8))
                               {
                                S3M_RowBuffer[channel*5+0] = 255;
                                GlissFlag[channel] = 1;
                               }
                            else
                                 GlissFlag[channel] = 0;
                           }
                        break;

              case 12 : // Lxy
                        value = S3M_RowBuffer[channel*5+0];  // note + octave
                        if ( (value != 255) && (value != 254) )
                           {
                            S3M_PeriodeAdjust[channel] = 0;
                            S3M_RowBuffer[channel*5+0] = 255;
                           }
                        break;

              case 19 : // special commands
                        if (x == 0x0d) // note delay for y frames
                           {
                            NoteDelayFlag[channel] = 0;
                            value = S3M_RowBuffer[channel*5+0];  // note + octave
                            if ( (value != 255) && (value != 254) )
                               {
                                NoteDelayNote[channel] = value;
                                S3M_RowBuffer[channel*5+0] = 255;
                                NoteDelayFlag[channel] |= 1;
                               }
                            value = S3M_RowBuffer[channel*5+1];
                            if ( value != 0 )
                               {
                                NoteDelayInst[channel] = value;
                                S3M_RowBuffer[channel*5+1] = 0;
                                NoteDelayFlag[channel] |= 2;
                               }
                            value = S3M_RowBuffer[channel*5+2];
                            if ( value != 255 )
                               {
                                NoteDelayVol[channel] = value;
                                S3M_RowBuffer[channel*5+2] = 255;
                                NoteDelayFlag[channel] |= 4;
                               }
                           }
                        break;
             }
     }
}


void S3M_RowEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned short x,y;
 static unsigned char value;

 for (channel = 0; channel < S3M_channels; channel++)
     {
      Effect = S3M_Effect[channel];
      Info = S3M_EffectInfo[channel];
      x = Info >> 4;
      y = Info & 15;

      switch (Effect)
             {
              case 255 : break; // no effect available

              case  4 : // Dxy
                        if (x == 0)
                           {
                            if ((y == 0x0f) || (S3M_flag_st3volslides == 1))  // fine down
                               S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel],y);
                            break;
                           }
                        else
                        if (y == 0)
                           {
                            if ((x == 0x0f) || (S3M_flag_st3volslides == 1))  // fine up
                               S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel],x);
                            break;
                           }
                        if ( (x == 0x0f) && (y != 0) && (y != 0x0f) )  // fine down
                           {
                            S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel],y);
                            break;
                           }
                        if ( (y == 0x0f) && (x != 0) && (x != 0x0f) )  // fine up
                           {
                            S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel],x);
                            break;
                           }
                        if ( (y == 0x0f) && (x == 0x0f) )              // fine slide up
                           {
                            S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel],0x0f);
                            break;
                           }
                        break;

              case  5 : // Exx
                        if (x == 0x0f)
                           {
                            S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel],y * 64);
                           }
                        else
                        if (x == 0x0e)
                           {
                            S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel],y * 16);
                           }
                        break;

              case  6 : // Fxx
                        if (x == 0x0f)
                           {
                            S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel],y * 64);
                           }
                        else
                        if (x == 0x0e)
                           {
                            S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel],y * 16);
                           }
                        break;

              case  7 : // Gxx
                        if ( Info != 0 )  GlissSpeed[channel] = Info;
                        if (GlissFlag[channel] == 1)
                           {
                            unsigned long dum;
                            dum = (unsigned long)(8363*16* ( ((unsigned long)S3M_NotePeriodes[ GlissNote[channel] ] << 4) >> GlissOctave[channel] ));
                            S3M_GlissPeriode[channel] = dum / S3M_SampleC4SPD[channel];   // periode but extended by 16
                            GlissFlag[channel] = 2;
                           }
                        if ( S3M_GlissPeriode[channel] != S3M_Periode[channel] )
                           {
                            S3M_VibratoPosition[channel] = 0;
                            S3M_PeriodeAdjust[channel] = 0;
                            GlissFlag[channel] = 2;
                           }
                        break;

              case  8 : // Hxy
                        if ( x != 0 )  VibratoSpeed[channel] = x;
                        if ( y != 0 )  VibratoDepth[channel] = y;
                        break;

              case  9 : // Ixy
                        if ( TremorCountOff[channel] == 0 )  TremorCountOn[channel] = x+1;
                        S3M_Tremor( channel , x+1, y+1);
                        break;

              case 10 : // Jxy
                        S3M_PeriodeAdjust[channel] = Calc_st3periode (S3M_Note[channel],S3M_Octave[channel],S3M_SampleC4SPD[channel]) - S3M_Periode[channel];
                        break;

              case 11 : // Kxy
                        break;

              case 12 : // Lxy
                        break;

              case 15 : // Oxx
                        value = S3M_RowBuffer[channel*5+0];  // note + octave
                        if ( (value != 255) && (value != 254) )
                           {
                            Info = Info << 8;
                            S3M_SamplePosition[channel] = Info;
                            S3M_SampleFraction[channel] = 0;

                            // Comment: In the docomentation should this
                            // command work in a different way.
                            // If a unlooped sample is played, the above
                            // code is correct, but if the sample is looped
                            // and the new offset is larger than the end of
                            // loop, the new position should be calculated
                            // as if the loop was taken, but the real tracker
                            // does not use this feature. Only the demo PANIC
                            // does it as described in the documentation.
                           }
                        break;

              case 17:  // Qxy
                        if (S3M_LastEffect[channel] == 17)  S3M_Retrig( channel , x , y);
                        else
                            {
                             if (Info != 0)  S3M_RetrigCount[channel] = y;
                            }
                        break;

              case 18 : // Rxy
                        if ( x != 0 )  VibratoSpeed[channel] = x;
                        if ( y != 0 )  VibratoDepth[channel] = y;
                        break;

              case 19 : // special effect
                        switch (x)
                               {
                                case 0x0e : // pattern delay for y notes
                                            if (y != 0)
                                               {
                                                y--;
                                                S3M_RowBuffer[channel*5+4] =  0xe0 + y;
                                                S3M_flag_note = 1;
                                               }
                                            else
                                               {
                                                S3M_flag_note = 0;
                                               }
                                            break;
                               }
                        break;

              case 21 : // Uxy
                        if ( x != 0 )  VibratoSpeed[channel] = x;
                        if ( y != 0 )  VibratoDepth[channel] = y;
                        S3M_FineVibrato( channel );
                        break;
             }
     }
}


void S3M_TickEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned short x,y;

 for (channel = 0; channel < S3M_channels; channel++)
     {
      Effect = S3M_Effect[channel];
      Info = S3M_EffectInfo[channel];
      x = Info >> 4;
      y = Info & 15;

      switch (Effect)
             {
              case 255 : break; // no effect available

              case  4 : // Dxy
                        if (x == 0)
                           {
                            S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel],y);
                           }
                        else
                        if (y == 0)
                           {
                            S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel],x);
                           }
                        break;

              case  5 : // Exx
                        if ( (x != 0x0f) && (x != 0x0e) )
                           {
                            S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel],Info * 64);
                           }
                        break;

              case  6 : // Fxx
                        if ( (x != 0x0f) && (x != 0x0e) )
                           {
                            S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel],Info * 64);
                           }
                        break;

              case  7 : // Gxx
                        if ( (S3M_GlissPeriode[channel] == S3M_Periode[channel])
                           || (GlissFlag[channel] != 2) )
                              {
                               GlissFlag[channel] = 0;
                               break;
                              }
                        if (S3M_GlissPeriode[channel] > S3M_Periode[channel])
                           {
                            S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel],GlissSpeed[channel] * 64);
                            if (S3M_Periode[channel] > S3M_GlissPeriode[channel])
                               S3M_Periode[channel] = S3M_GlissPeriode[channel];
                           }
                        else
                           {
                            S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel],GlissSpeed[channel] * 64);
                            if (S3M_Periode[channel] < S3M_GlissPeriode[channel])
                               S3M_Periode[channel] = S3M_GlissPeriode[channel];
                           }
                        break;

              case  8 : // Hxy
                        S3M_Vibrato( channel );
                        break;

              case  9 : // Ixy
                        S3M_Tremor( channel, x+1, y+1);
                        break;

              case 10 : // Jxy
                        {
                         unsigned char note, oct;
                         note = S3M_Note[channel];
                         oct = S3M_Octave[channel];
                         //if ((S3M_tick % 3) == 0)
                         //   {
                         //   }
                         if ((S3M_tick % 3) == 1)
                            {
                             note += x;
                            }
                         if ((S3M_tick % 3) == 2)
                            {
                             note += y;
                            }
                        if (note > 11)
                           {
                            note -= 12;
                            oct++;
                           }
                        S3M_PeriodeAdjust[channel] = Calc_st3periode (note,oct,S3M_SampleC4SPD[channel]) - S3M_Periode[channel];
                        }
                        break;

              case 11 : // Kxy
                        if ( ( x == 0x0f ) && ( y != 0 ) )   break;
                        if ( ( y == 0x0f ) && ( x != 0 ) )   break;
                        if (x == 0)
                           {
                            S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel],y);
                           }
                        else
                        if (y == 0)
                           {
                            S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel],x);
                           }
                        S3M_Vibrato ( channel );
                        break;

              case 12 : // Lxy
                        if ( ( x == 0x0f ) && ( y != 0 ) )   break;
                        if ( ( y == 0x0f ) && ( x != 0 ) )   break;
                        if (x == 0)
                           {
                            S3M_Volume[channel] = S3M_decVolume(S3M_Volume[channel],y);
                           }
                        else
                        if (y == 0)
                           {
                            S3M_Volume[channel] = S3M_addVolume(S3M_Volume[channel],x);
                           }

                        if ( (S3M_GlissPeriode[channel] == S3M_Periode[channel])
                           || (GlissFlag[channel] != 2) )
                              {
                               GlissFlag[channel] = 0;
                               break;
                              }
                        if (S3M_GlissPeriode[channel] > S3M_Periode[channel])
                           {
                            S3M_Periode[channel] = S3M_addPeriode(S3M_Periode[channel],GlissSpeed[channel] * 64);
                            if (S3M_Periode[channel] > S3M_GlissPeriode[channel])
                               S3M_Periode[channel] = S3M_GlissPeriode[channel];
                           }
                        else
                           {
                            S3M_Periode[channel] = S3M_decPeriode(S3M_Periode[channel],GlissSpeed[channel] * 64);
                            if (S3M_Periode[channel] < S3M_GlissPeriode[channel])
                               S3M_Periode[channel] = S3M_GlissPeriode[channel];
                           }
                        break;

              case 17 : // Qxy
                        S3M_Retrig( channel, x, y );
                        break;

              case 18 : // Rxy
                        S3M_Tremolo( channel );
                        break;

              case 19 : // special effect
                        switch (x)
                               {
                                case 0x0c : // note cut in y frames
                                            if (S3M_tick == y)
                                               {
                                                S3M_ChannelActiv[channel] = 0;
                                               }
                                            break;
                                case 0x0d : // note delay for y frames
                                            if (S3M_tick == y)
                                               {
                                                if ((NoteDelayFlag[channel] & 2) == 2)
                                                   {
                                                    S3M_SampleNr[channel] = NoteDelayInst[channel];
                                                    S3M_GetNewSample( channel );
                                                   }
                                                if ((NoteDelayFlag[channel] & 4) == 4)
                                                    S3M_Volume[channel] = ( S3M_GlobalVolume * NoteDelayVol[channel] + 32 ) / 64;
                                                if ((NoteDelayFlag[channel] & 1) == 1)
                                                   {
                                                    S3M_Note[channel] = NoteDelayNote[channel] & 15;
                                                    S3M_Octave[channel] = NoteDelayNote[channel] >> 4;
                                                    S3M_GetNewNote( channel );
                                                   }
                                               }
                                            break;
                               }
                        break ;

              case 21 : // Uxy
                        S3M_FineVibrato( channel );
                        break;
             }
     }
}

void S3M_initEffects()
{
 static unsigned long channel;

 for (channel = 0; channel < 32; channel ++)
     {
      Vibrato[channel] = Sinus;
      Tremolo[channel] = Sinus;

      GlissSpeed[channel] = 0;
      GlissNote[channel] = 0;
      GlissOctave[channel] = 0;
      S3M_GlissPeriode[channel] = 0;
      GlissFlag[channel] = 0;

      VibratoSpeed[channel] = 0;
      VibratoDepth[channel] = 0;
      S3M_VibratoPosition[channel] = 0;

      S3M_TremorVolume[channel] = 0;
      TremorCountOn[channel] = 0;
      TremorCountOff[channel] = 0;

      S3M_RetrigCount[channel] = 0;

      NoteDelayFlag[channel] = 0;      // flag
      NoteDelayNote[channel] = 0;
      NoteDelayInst[channel] = 0;
      NoteDelayVol[channel] = 0;

      PatLoopCount = 0;
     }

}
