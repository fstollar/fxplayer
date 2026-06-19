// MOD - Routines for F/X Player v1.0
// (c) 1998 by Apollo / STIGMA

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
//#include "dat_wav.h"
#include "dat_MOD.h"

//#include "convert.h"
//#include "mixer.h"


// Tables

static signed short MOD_Sinus[64] =
                         {    0,  100,  200,  297,  392,  483,  569,  650,
                            724,  792,  851,  903,  946,  980, 1004, 1019,
                           1024, 1019, 1004,  980,  946,  903,  851,  792,
                            724,  650,  569,  483,  392,  297,  200,  100,
                             -0, -100, -200, -297, -392, -483, -569, -650,
                           -724, -792, -851, -903, -946, -980,-1004,-1019,
                          -1024,-1019,-1004, -980, -946, -903, -851, -792,
                           -724, -650, -569, -483, -392, -297, -200, -100  };

static signed short MOD_Ramp[64] =
                         {-1024, -992, -960, -928, -896, -864, -832, -800,
                           -768, -736, -704, -672, -640, -608, -576, -544,
                           -512, -480, -448, -416, -384, -352, -320, -288,
                           -256, -224, -192, -160, -128,  -96,  -64,  -32,
                              0,   32,   64,   96,  128,  160,  192,  224,
                            256,  288,  320,  352,  384,  416,  448,  480,
                            512,  544,  576,  608,  640,  672,  704,  736,
                            768,  800,  832,  864,  896,  928,  960,  992  };

static signed short MOD_Square[64] =
                         {  1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                            1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                            1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                            1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
                               0,    0,    0,    0,    0,    0,    0,    0,
                               0,    0,    0,    0,    0,    0,    0,    0,
                               0,    0,    0,    0,    0,    0,    0,    0,
                               0,    0,    0,    0,    0,    0,    0,    0  };

static signed short MOD_Random[64] =
                         {   235,  761, -875,  565,  677,  789, -557, -349,
                             905,  378, -378,  480,  609, -426, -140, -801,
                             730,  -48,  807, -134,   60,  932,  417, -315,
                             512,  869, -184, 1006,-1024,  551, -311,  110,
                               8, -415,  449,   62,  637, -340,  -39, -291,
                             -27, -963,   35,  -33,  447, -740,  907,  -50,
                            -149,  350,   62,  562, -430,  606,  410,  484,
                            -252,  405,  -64, -314, -272, -578, -972,  101  };

static signed short *MOD_Vibrato[MODchannel];
static signed short *MOD_Tremolo[MODchannel];

// *** direct effects ***

// VOLUME

static unsigned short MOD_addVolume ( short Volume, short value)
{
 Volume += value;

 if (Volume > 64)  Volume = 64;
 else
 if (Volume < 0)   Volume = 0;

 return Volume;
}

static unsigned short MOD_decVolume ( short Volume, short value)
{
 Volume -= value;

 if (Volume < 0)   Volume = 0;
 else
 if (Volume > 64)  Volume = 64;

 return Volume;
}

// TONE SLIDE

static unsigned long MOD_addPeriode ( signed long Periode, unsigned long value)
{
 Periode += value;

 if (MOD_flag_amigalimits == 1)
    {
     if (Periode < 113)  Periode = 113;
     if (Periode > 856 ) Periode = 856;
    }
 else
      if (Periode < 10)  Periode = 10;

 return Periode;
}

static unsigned long MOD_decPeriode ( signed long Periode, unsigned long value)
{
 Periode -= value;

 if (MOD_flag_amigalimits == 1)
    {
     if (Periode < 113)  Periode = 113;
     if (Periode > 856 ) Periode = 856;
    }
 else
      if (Periode < 10)  Periode = 10;

 return Periode;
}

// GLISSANDO

static unsigned char  MOD_GlissSpeed[MODchannel];
static unsigned char  MOD_GlissNote[MODchannel];
       unsigned long  MOD_GlissPeriode[MODchannel];
static unsigned char  MOD_GlissFlag[MODchannel];

// VIBRATO

static unsigned char  MOD_VibratoSpeed[MODchannel];
static unsigned char  MOD_VibratoDepth[MODchannel];
       unsigned short MOD_VibratoPosition[MODchannel];

void MOD_MVibrato( unsigned long channel )
{
 if ( MOD_VibratoSpeed[channel] == 0 )  return;

 MOD_PeriodeAdjust[channel] = *(signed short *)(MOD_Vibrato[channel] + MOD_VibratoPosition[channel]) * MOD_VibratoDepth[channel] / 64;
 MOD_VibratoPosition[channel] = (MOD_VibratoPosition[channel] + MOD_VibratoSpeed[channel]) & 63;
}

// RETRIG

static signed char MOD_RetrigCount[MODchannel];

static void MOD_Retrig( unsigned long channel, signed char count )
{
 MOD_RetrigCount[channel]--;
 if ( MOD_RetrigCount[channel] == 0 )
    {
     MOD_RetrigCount[channel] = count;
     MOD_SamplePosition[channel] = 0;
     MOD_SampleFraction[channel] = 0;
    }
}

// TREMOLO

       unsigned short MOD_TremoloVolume[MODchannel];

void MOD_MTremolo( unsigned long channel )
{
 if ( MOD_VibratoSpeed[channel] == 0 )  return;

 MOD_Volume[channel] = MOD_addVolume( MOD_TremoloVolume[channel], *(signed short *)(MOD_Tremolo[channel] + MOD_VibratoPosition[channel]) * MOD_VibratoDepth[channel] / 512 );
 MOD_VibratoPosition[channel] = (MOD_VibratoPosition[channel] + MOD_VibratoSpeed[channel]) & 63;
}

// SPECIAL

   // NOTE DELAY

   static unsigned char MOD_NoteDelayFlag[MODchannel];      // flag
   static unsigned char MOD_NoteDelayNote[MODchannel];
   static unsigned char MOD_NoteDelayInst[MODchannel];

   // E6x Loop Pattern

   static signed short MOD_PatLoopCount = 0;


// *** control over effects ***

void MOD_GlobalEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned short x,y;

 short jumpbreak = 0;
 short special = 0;

 for (channel = 0; channel < MOD_channels; channel++)
 {
 Effect = MOD_Effect[channel];
 Info = MOD_EffectInfo[channel];
 x = Info >> 4;
 y = Info & 15;

 switch (Effect)
        {
         case 255 : break; // no effect

         case 11 :  // Bxx : jump to order xx (l <- r)
                   if (jumpbreak != 0)  break;
//                   printf("Jump!\n");
                   MOD_nextorder = Info;
                   MOD_nextrow = 0;
                   MOD_jump = 1;
                   jumpbreak = 1;
                   break;

         case 13 :  // Dxx : break pattern to row xx (l <- r) (BCD!!)
                   if (jumpbreak != 0)  break;
//                   printf("Break! %i\n", Info);
                   Info = (Info >> 4) * 10 + (Info & 15);
                   MOD_nextorder = MOD_Order + 1;
                   MOD_nextrow = Info;
                   MOD_jump = 1;
                   jumpbreak = 1;
                   break;

         case 14 :  // Exy : special command x
                   switch (x)
                          {
                           case 0x03 :
                                       break;
                           case 0x04 : // set vibrato waveform
                                       if ( (y & 3) == 0 )  MOD_Vibrato[channel] = MOD_Sinus;
                                       if ( (y & 3) == 1 )  MOD_Vibrato[channel] = MOD_Ramp;
                                       if ( (y & 3) == 2 )  MOD_Vibrato[channel] = MOD_Square;
                                       if ( (y & 3) == 3 )  MOD_Vibrato[channel] = MOD_Random;
                                       break;
                           case 0x07 : // set tremolo waveform
                                       if ( (y & 3) == 0 )  MOD_Tremolo[channel] = MOD_Sinus;
                                       if ( (y & 3) == 1 )  MOD_Tremolo[channel] = MOD_Ramp;
                                       if ( (y & 3) == 2 )  MOD_Tremolo[channel] = MOD_Square;
                                       if ( (y & 3) == 3 )  MOD_Tremolo[channel] = MOD_Random;
                                       break;
                           case 0x08 : // set pan position
                                       MOD_Panning[channel] = y;
                                       break;

                           case 0x06 : // start (y=0) / loop (y=count)
                                       if (y == 0)
                                          {
                                           MOD_nextrow = MOD_row;
                                           break;
                                          }
                                       else
                                       if (MOD_PatLoopCount == 0)
                                          MOD_PatLoopCount = y + 1;
                                       MOD_PatLoopCount --;
                                       if (MOD_PatLoopCount != 0)
                                          {
                                           MOD_nextorder = MOD_Order;
                                           MOD_jump = 1;
                                          }
                                       else
                                          {
                                           MOD_nextrow = MOD_row + 1;
                                           MOD_jump = 0;
                                          }
                                       break;

                          }
                   break;

         case 15 :  // Fxx : set tempo to xx (00h-1fh,20h-ffh) (l <- r)
                   if (Info < 0x20)
                      {
                       if ( Info == 0 )  break;
                       MOD_speed = Info;
                      }
                   else
                      {
                       MOD_tempo = Info;
                       MOD_TickLength = DAT_calcSpeed ( MOD_tempo, MixSpeed );
                      }
                   break;

        }
  }
}


void MOD_beforeEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned short x,y;
 static unsigned char value;


 for (channel = 0; channel < MOD_channels; channel++)
     {
      // and the usuall stuff

      Effect = MOD_Effect[channel];
      Info = MOD_EffectInfo[channel];
      x = Info >> 4;
      y = Info & 15;

      switch (Effect)
             {
              case 255 : break; // nothing

              case  3 : // Gxx
                        value = MOD_RowBuffer[channel*4+0];  // note + octave
                        if ( value != 255 )
                           {
                            MOD_PeriodeAdjust[channel] = 0;
                            MOD_GlissNote[channel] = value;
                            if ( MOD_GlissNote[channel] < 84 )
                               {
                                MOD_RowBuffer[channel*4+0] = 255;
                                MOD_GlissFlag[channel] = 1;
                               }
                            else
                                 MOD_GlissFlag[channel] = 0;
                           }
                        break;

              case 5 : // Lxy
                        value = MOD_RowBuffer[channel*4+0];  // note + octave
                        if ( value != 255 )
                           {
                            MOD_PeriodeAdjust[channel] = 0;
                            MOD_RowBuffer[channel*4+0] = 255;
                           }
                        break;

              case 14 : // special commands
                        if (x == 0x0d) // note delay for y frames
                           {
                            MOD_NoteDelayFlag[channel] = 0;
                            value = MOD_RowBuffer[channel*4+0];  // note + octave
                            if ( value != 255 )
                               {
                                MOD_NoteDelayNote[channel] = value;
                                MOD_RowBuffer[channel*4+0] = 255;
                                MOD_NoteDelayFlag[channel] |= 1;
                               }
                            value = MOD_RowBuffer[channel*4+1];
                            if ( value != 0 )
                               {
                                MOD_NoteDelayInst[channel] = value;
                                MOD_RowBuffer[channel*4+1] = 0;
                                MOD_NoteDelayFlag[channel] |= 2;
                               }
                           }
                        break;
             }
     }
}


void MOD_RowEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned short x,y;
 static unsigned char value;

 for (channel = 0; channel < MOD_channels; channel++)
     {
      Effect = MOD_Effect[channel];
      Info = MOD_EffectInfo[channel];
      x = Info >> 4;
      y = Info & 15;

      switch (Effect)
             {
              case 255 : break; // no effect available

              case  0 : // Arpeggio
                         if ( Info == 0 )  break;
                         MOD_PeriodeAdjust[channel] = Calc_AMIGAperiode (MOD_Note[channel],MOD_SampleFinetune[channel]) - MOD_Periode[channel];
                         break;

              case  1 : // Tone slide up
                        break;

              case  2 : // Tone slide down
                        break;

              case  3 : // Glissando
                        if ( Info != 0 )  MOD_GlissSpeed[channel] = Info;
                        if ( MOD_GlissFlag[channel] == 1 )
                           {
                            MOD_GlissPeriode[channel] = MOD_Periodes[MOD_SampleFinetune[channel]][MOD_GlissNote[channel]] << 4; // extend by 4 bits for fixkomma
                            MOD_GlissFlag[channel] = 2;
                           }
                        if ( MOD_GlissPeriode[channel] != MOD_Periode[channel] )
                           {
                            MOD_VibratoPosition[channel] = 0;
                            MOD_PeriodeAdjust[channel] = 0;
                            MOD_GlissFlag[channel] = 2;
                           }
                        break;

              case  4 : // Vibrato
                        if ( x != 0 )  MOD_VibratoSpeed[channel] = x;
                        if ( y != 0 )  MOD_VibratoDepth[channel] = y;
                        break;

              case  5 : // Gliss + Volume slide
                        if ( MOD_GlissFlag[channel] == 1 )
                           {
                            MOD_GlissPeriode[channel] = MOD_Periodes[MOD_SampleFinetune[channel]][MOD_GlissNote[channel]] << 4; // extend by 4 bits for fixkomma
                            MOD_GlissFlag[channel] = 2;
                           }
                        if ( MOD_GlissPeriode[channel] != MOD_Periode[channel] )
                           {
                            MOD_VibratoPosition[channel] = 0;
                            MOD_PeriodeAdjust[channel] = 0;
                            MOD_GlissFlag[channel] = 2;
                           }
                        MOD_Volume[channel] = MOD_decVolume(MOD_Volume[channel],y);
                        MOD_Volume[channel] = MOD_addVolume(MOD_Volume[channel],x);
                        break;

              case  6 : // Vibrato + Volume slide
                        MOD_Volume[channel] = MOD_decVolume(MOD_Volume[channel],y);
                        MOD_Volume[channel] = MOD_addVolume(MOD_Volume[channel],x);
                        break;

              case  7 : // Tremolo
                        if ( x != 0 )  MOD_VibratoSpeed[channel] = x;
                        if ( y != 0 )  MOD_VibratoDepth[channel] = y;
                        break;

              case  8 : // not used
                        break;

              case  9 : // Sample Offset
                        Info = Info << 8;
                        if ( Info > MOD_SampleLength[channel] )  break;
                        MOD_SamplePosition[channel] = Info;
                        MOD_SampleFraction[channel] = 0;
                        MOD_ChannelActiv[channel] = 1;
                        break;

              case 10 : // Volume Slide
                        MOD_Volume[channel] = MOD_decVolume(MOD_Volume[channel],y);
                        MOD_Volume[channel] = MOD_addVolume(MOD_Volume[channel],x);
                        break;

              case 12 : // set volume
                        if ( Info > 64 )  Info = 64;
                        MOD_TremoloVolume[channel] = MOD_Volume[channel] = Info;
                        break;

              case 14 : // special effect
                        switch (x)
                               {
                                case 0x00 : // Filter on/off
                                            break;
                                case 0x01 : // Fineslide up
                                            MOD_Periode[channel] = MOD_decPeriode(MOD_Periode[channel],Info);
                                            break;
                                case 0x02 : // Fineslide down
                                            MOD_Periode[channel] = MOD_addPeriode(MOD_Periode[channel],Info);
                                            break;
                                case 0x05 : // set finetune
                                            MOD_SampleFinetune[channel] = y;
                                            break;
                                case 0x08 : // unused / panning
                                            break;
                                case 0x09 : // Retrig
                                            if ( (MOD_LastEffect[channel] == 14) && ((MOD_LastEffectInfo[channel] >> 4) == 9) )
                                                MOD_Retrig( channel , y);
                                            else
                                               {
                                                if (Info == 0)  break;
                                                MOD_RetrigCount[channel] = y;
                                               }
                                            break;
                                case 0x0a : // FineVol up
                                            MOD_Volume[channel] = MOD_addVolume(MOD_Volume[channel],y);
                                            break;
                                case 0x0b : // FineVol down
                                            MOD_Volume[channel] = MOD_decVolume(MOD_Volume[channel],y);
                                            break;
                                case 0x0e : // pattern delay for y notes
                                            if (y != 0)
                                               {
                                                y--;
                                                MOD_RowBuffer[channel*4+3] =  0xe0 + y;
                                                MOD_flag_note = 1;
                                               }
                                            else
                                               {
                                                MOD_flag_note = 0;
                                               }
                                            break;
                               }
                        break;
             }
     }
}


void MOD_TickEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned short x,y;

 for (channel = 0; channel < MOD_channels; channel++)
     {
      Effect = MOD_Effect[channel];
      Info = MOD_EffectInfo[channel];
      x = Info >> 4;
      y = Info & 15;

      switch (Effect)
             {
              case 255 : break; // no effect available

              case  0 : // Arpeggio
                        if ( Info != 0 )
                        {
                         unsigned char note;
                         note = MOD_Note[channel];
                         if ((MOD_tick % 3) == 1)
                            {
                             note += x;
                            }
                         if ((MOD_tick % 3) == 2)
                            {
                             note += y;
                            }
                        MOD_PeriodeAdjust[channel] = Calc_AMIGAperiode (note,MOD_SampleFinetune[channel]) - MOD_Periode[channel];
                        }
                        break;

              case  1 : // Tone slide up
                        MOD_Periode[channel] = MOD_decPeriode(MOD_Periode[channel],Info * 16);
                        break;

              case  2 : // Tone slide down
                        MOD_Periode[channel] = MOD_addPeriode(MOD_Periode[channel],Info * 16);
                        break;

              case  3 : // Glissando
                        if ( (MOD_GlissPeriode[channel] == MOD_Periode[channel])
                           || (MOD_GlissFlag[channel] != 2) )
                              {
                               MOD_GlissFlag[channel] = 0;
                               break;
                              }
                        if (MOD_GlissPeriode[channel] > MOD_Periode[channel])
                           {
                            MOD_Periode[channel] = MOD_addPeriode(MOD_Periode[channel],MOD_GlissSpeed[channel] * 16);
                            if (MOD_Periode[channel] > MOD_GlissPeriode[channel])
                               MOD_Periode[channel] = MOD_GlissPeriode[channel];
                           }
                        else
                           {
                            MOD_Periode[channel] = MOD_decPeriode(MOD_Periode[channel],MOD_GlissSpeed[channel] * 16);
                            if (MOD_Periode[channel] < MOD_GlissPeriode[channel])
                               MOD_Periode[channel] = MOD_GlissPeriode[channel];
                           }
                        break;

              case  4 : // Vibrato
                        MOD_MVibrato( channel );
                        break;

              case  5 : // Glissando + Volume slide
                        MOD_Volume[channel] = MOD_decVolume(MOD_Volume[channel],y);
                        MOD_Volume[channel] = MOD_addVolume(MOD_Volume[channel],x);

                        if ( (MOD_GlissPeriode[channel] == MOD_Periode[channel])
                           || (MOD_GlissFlag[channel] != 2) )
                              {
                               MOD_GlissFlag[channel] = 0;
                               break;
                              }
                        if (MOD_GlissPeriode[channel] > MOD_Periode[channel])
                           {
                            MOD_Periode[channel] = MOD_addPeriode(MOD_Periode[channel],MOD_GlissSpeed[channel] * 16);
                            if (MOD_Periode[channel] > MOD_GlissPeriode[channel])
                               MOD_Periode[channel] = MOD_GlissPeriode[channel];
                           }
                        else
                           {
                            MOD_Periode[channel] = MOD_decPeriode(MOD_Periode[channel],MOD_GlissSpeed[channel] * 16);
                            if (MOD_Periode[channel] < MOD_GlissPeriode[channel])
                               MOD_Periode[channel] = MOD_GlissPeriode[channel];
                           }
                        break;

              case  6 : // Vibrato + Volume slide
                        MOD_Volume[channel] = MOD_decVolume(MOD_Volume[channel],y);
                        MOD_Volume[channel] = MOD_addVolume(MOD_Volume[channel],x);
                        MOD_MVibrato ( channel );
                        break;

              case  7 : // Tremolo
                        MOD_MTremolo( channel );
                        break;

              case 10 : // Volume slide
                        MOD_Volume[channel] = MOD_decVolume(MOD_Volume[channel],y);
                        MOD_Volume[channel] = MOD_addVolume(MOD_Volume[channel],x);
                        break;

              case 14 : // special effect
                        switch (x)
                               {
                                case 0x09 : // Retrig
                                            MOD_Retrig( channel, y );
                                            break;
                                case 0x0c : // note cut in y frames
                                            if (MOD_tick == y)
                                               {
                                                MOD_Volume[channel] = 0;
                                               }
                                            break;
                                case 0x0d : // note delay for y frames
                                            if (MOD_tick == y)
                                               {
                                                if ((MOD_NoteDelayFlag[channel] & 2) == 2)
                                                   {
                                                    MOD_SampleNr[channel] = MOD_NoteDelayInst[channel];
                                                    MOD_GetNewSample( channel );
                                                   }
                                                if ((MOD_NoteDelayFlag[channel] & 1) == 1)
                                                   {
                                                    MOD_Note[channel] = MOD_NoteDelayNote[channel];
                                                    MOD_GetNewNote( channel );
                                                   }
                                               }
                                            break;
                               }
                        break ;

             }
     }
}


void MOD_initEffects()
{
 static unsigned long channel;

 for (channel = 0; channel < MODchannel; channel ++)
     {
      MOD_Vibrato[channel] = MOD_Sinus;
      MOD_Tremolo[channel] = MOD_Sinus;

      MOD_GlissSpeed[channel] = 0;
      MOD_GlissNote[channel] = 0;
      MOD_GlissPeriode[channel] = 0;
      MOD_GlissFlag[channel] = 0;

      MOD_VibratoSpeed[channel] = 0;
      MOD_VibratoDepth[channel] = 0;
      MOD_VibratoPosition[channel] = 0;

      MOD_TremoloVolume[channel] = 0;

      MOD_RetrigCount[channel] = 0;

      MOD_NoteDelayFlag[channel] = 0;      // flag
      MOD_NoteDelayNote[channel] = 0;
      MOD_NoteDelayInst[channel] = 0;

      MOD_PatLoopCount = 0;
     }
}
