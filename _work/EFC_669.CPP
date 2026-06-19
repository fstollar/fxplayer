// 669 - Routines for F/X Player v1.0
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
#include "dat_669.h"

//#include "convert.h"
//#include "mixer.h"


// Tables

static signed short _669_Sinus[64] =
                         {    0,  100,  200,  297,  392,  483,  569,  650,
                            724,  792,  851,  903,  946,  980, 1004, 1019,
                           1024, 1019, 1004,  980,  946,  903,  851,  792,
                            724,  650,  569,  483,  392,  297,  200,  100,
                             -0, -100, -200, -297, -392, -483, -569, -650,
                           -724, -792, -851, -903, -946, -980,-1004,-1019,
                          -1024,-1019,-1004, -980, -946, -903, -851, -792,
                           -724, -650, -569, -483, -392, -297, -200, -100  };

static signed short *_669_Vibrato[_669_channels];

// *** direct effects ***

// VOLUME

/*
static unsigned short _669_addVolume ( short Volume, short value)
{
 Volume += value;

 if (Volume > 16)  Volume = 16;
 else
 if (Volume < 0)   Volume = 0;

 return Volume;
}

static unsigned short _669_decVolume ( short Volume, short value)
{
 Volume -= value;

 if (Volume < 0)   Volume = 0;
 else
 if (Volume > 16)  Volume = 16;

 return Volume;
}
*/

// TONE SLIDE

static unsigned long _669_addPeriode ( signed long Periode, unsigned long value)
{
 Periode += value;
/*
 if (_669_flag_amigalimits == 1)
    {
     if (Periode < 113)  Periode = 113;
     if (Periode > 856 ) Periode = 856;
    }
 else
*/
//      if (Periode < 10)  Periode = 10;

 return Periode;
}

static unsigned long _669_decPeriode ( signed long Periode, unsigned long value)
{
 Periode -= value;
/*
 if (_669_flag_amigalimits == 1)
    {
     if (Periode < 113)  Periode = 113;
     if (Periode > 856 ) Periode = 856;
    }
 else
*/
//      if (Periode < 10)  Periode = 10;

 return Periode;
}


// GLISSANDO

static unsigned char  _669_GlissSpeed[_669_channels];
static unsigned char  _669_GlissNote[_669_channels];
       unsigned long  _669_GlissPeriode[_669_channels];
       unsigned char  _669_GlissFlag[_669_channels];


// VIBRATO

static unsigned char  _669_VibratoDepth[_669_channels];
       unsigned short _669_VibratoPosition[_669_channels];

void _669_MVibrato( unsigned long channel )
{
 _669_PeriodeAdjust[channel] = *(signed short *)(_669_Vibrato[channel] + _669_VibratoPosition[channel]) * _669_VibratoDepth[channel] / 64;
 _669_VibratoPosition[channel] = (_669_VibratoPosition[channel] + 1 ) & 63;
}


// RETRIG

static signed char _669_RetrigCount[_669_channels];

static void _669_Retrig( unsigned long channel, signed char count )
{
 _669_RetrigCount[channel]--;
 if ( _669_RetrigCount[channel] == 0 )
    {
     _669_RetrigCount[channel] = count;
     _669_SamplePosition[channel] = 0;
     _669_SampleFraction[channel] = 0;
    }
}


// *** control over effects ***

void _669_GlobalEffect()
{
 static unsigned long channel, Effect, Info;

 short jumpbreak = 0;
 short special = 0;

 for (channel = 0; channel < _669_channels; channel++)
 {
 Effect = _669_Effect[channel];
 Info = _669_EffectInfo[channel];

 switch (Effect)
        {
         case 255 : break; // no effect
/*
         case 0 :  // Dxx : break pattern to row xx (l <- r) (BCD!!)
                   if (jumpbreak != 0)  break;
//                   printf("Break! %i\n", Info);
                   Info = (Info >> 4) * 10 + (Info & 15);
                   _669_nextorder = _669_Order + 1;
                   _669_nextrow = Info;
                   _669_jump = 1;
                   jumpbreak = 1;
                   break;
*/
         case 5 :  // set speed to x (01-15)
                   if ( Info == 0 )  break;
                   _669_speed = Info;
                   break;
        }
  }
}


void _669_beforeEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned char value;


 for (channel = 0; channel < _669_channels; channel++)
     {
      // and the usuall stuff

      Effect = _669_Effect[channel];
      Info = _669_EffectInfo[channel];

      switch (Effect)
             {
              case 255 : break; // nothing

              case  2 : // Portamento to Note
                        value = _669_RowBuffer[channel*5+0];  // note
                        if ( (value != 0xff) && (value != 0xfe) )
                           {
                            _669_PeriodeAdjust[channel] = 0;
                            _669_GlissNote[channel] = value;
                            if ( _669_GlissNote[channel] < 64 )
                               {
                                _669_RowBuffer[channel*5+0] = 0xfe;
                                _669_GlissFlag[channel] = 1;
                               }
                            else
                                 _669_GlissFlag[channel] = 0;
                           }
                        break;
             }
     }
}


void _669_RowEffect()
{
 static unsigned long channel, Effect, Info;
 static unsigned char value;

 for (channel = 0; channel < _669_channels; channel++)
     {
      Effect = _669_Effect[channel];
      Info = _669_EffectInfo[channel];

      switch (Effect)
             {
              case 255 : break; // no effect available

              case  0 : // Tone slide up
                        break;

              case  1 : // Tone slide down
                        break;

              case  2 : // Glissando
                        if ( Info != 0 )  _669_GlissSpeed[channel] = Info;
                        if ( _669_GlissFlag[channel] == 1 )
                           {
                            _669_GlissPeriode[channel] = _669_Periodes[ _669_SampleFinetune[channel]][_669_GlissNote[channel]] << 4; // extend by 4 bits for fixkomma
                            _669_GlissFlag[channel] = 2;
                           }
                        if ( _669_GlissPeriode[channel] != _669_Periode[channel] )
                           {
                            _669_VibratoPosition[channel] = 0;
                            _669_PeriodeAdjust[channel] = 0;
                            _669_GlissFlag[channel] = 2;
                           }
                        break;

              case  3 : // Finetune
                        _669_SampleFinetune[channel] = Info;
                        break;

              case  4 : // Vibrato
                        _669_VibratoDepth[channel] = (Info << 4) + 1;
                        break;
             }
     }
}


void _669_TickEffect()
{
 static unsigned long channel, Effect, Info;

 for (channel = 0; channel < _669_channels; channel++)
     {
      Effect = _669_Effect[channel];
      Info = _669_EffectInfo[channel];

      if ( (Effect == 0xff) && (_669_GlissFlag[channel] == 2) )  Effect = 2;

      switch (Effect)
             {
              case 255 : break; // no effect available

              case  0 : // Tone slide up
                        _669_Periode[channel] = _669_decPeriode( _669_Periode[channel],Info * 16);
                        break;

              case  1 : // Tone slide down
                        _669_Periode[channel] = _669_addPeriode( _669_Periode[channel],Info * 16);
                        break;

              case  2 : // Glissando
                        if ( ( _669_GlissPeriode[channel] == _669_Periode[channel])
                           || ( _669_GlissFlag[channel] != 2) )
                              {
                               _669_GlissFlag[channel] = 0;
                               break;
                              }
                        if ( _669_GlissPeriode[channel] > _669_Periode[channel])
                           {
                            _669_Periode[channel] = _669_addPeriode( _669_Periode[channel], _669_GlissSpeed[channel] * 16);
                            if ( _669_Periode[channel] > _669_GlissPeriode[channel])
                               _669_Periode[channel] = _669_GlissPeriode[channel];
                           }
                        else
                           {
                            _669_Periode[channel] = _669_decPeriode( _669_Periode[channel], _669_GlissSpeed[channel] * 16);
//printf("GP: %u P:%u\n", _669_GlissPeriode[channel], _669_Periode[channel]);
                            if ( _669_Periode[channel] < _669_GlissPeriode[channel])
                               _669_Periode[channel] = _669_GlissPeriode[channel];
                           }
                        break;

              case  4 : // Vibrato
                        _669_MVibrato( channel );
                        break;

             }
     }
}


void _669_initEffects()
{
 static unsigned long channel;

 for (channel = 0; channel < _669_channels; channel ++)
     {
      _669_Vibrato[channel] = _669_Sinus;

      _669_GlissSpeed[channel] = 0;
      _669_GlissNote[channel] = 0;
      _669_GlissPeriode[channel] = 0;
      _669_GlissFlag[channel] = 0;

      _669_VibratoDepth[channel] = 0;
      _669_VibratoPosition[channel] = 0;

      _669_RetrigCount[channel] = 0;
     }
}
