// Mixer control v2.3 for the F/X Player
// (c) 1996-98 by Apollo of STIGMA

//#define cdebug

#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>

#include "fx.h"

#include "irq.h"
//#include "dma.h"

//#include "device.h"
//#include "dev_sb.h"

//#include "format.h"
#include "dat_calc.h"
//#include "dat_wav.h"
//#include "dat_s3m.h"

//#include "convert.h"
#include "mixer.h"
#include "amptable.h"


#define MAXCHANNELS      256          // How many channels can be managed
#define MC               MAXCHANNELS  // only the shortcut

// *** global mixer variables ***

unsigned long  ChannelSeperation = 256;    // seperation of channels
                                           // 256 = max, 0 = mono
unsigned long  GlobalVolume = 256;         // global volume from 0 to 256
unsigned long  MasterVolume = 0;           // master volume, is realtive
                                           // volume of every channel to mix

// *** global mixerchannel variables ***

unsigned long  ChannelUsed = 0, ChannelLast = 0;

unsigned long  ChannelActiv[MC];          // 0 = quiet, 1 = activ
unsigned long  ChannelFlag[MC];           // 0 = available, 1 = paused

unsigned long  ChannelSampleNr[MC];       // Nr. of Sample playing (not really needed)
unsigned long  ChannelVolume[MC];         // from 0 (quiet) to 256 (loud)
unsigned long  ChannelPanning[MC];        // from 0 (left) to 256 (right), 128 is middle, +32768 = surround
unsigned long  ChannelSampleBits[MC];     // 8 or 16 bit
unsigned long  ChannelSampleMode[MC];     // 0 = mono, 1 = stereo sample
unsigned long  ChannelSampleAddress[MC];  // start address of current sample INCLUSIVE
unsigned long  ChannelSampleLength[MC];   // position length of sample EXCLUSIVE
unsigned long  ChannelLoopMode[MC];       // 0 = no, 1 = forward, 2 = reverse, 3 = pingpong
unsigned long  ChannelLoopBegin[MC];      // position where loop starts INCLUSIVE
unsigned long  ChannelLoopEnd[MC];        // position where loop ends EXCLUSIVE
unsigned long  ChannelSamplePosition[MC]; // actuall position in sample
unsigned long  ChannelSampleFraction[MC]; // actuall fraction of sample position
unsigned long  ChannelSampleFrequence[MC];// actuall frequence to play sample
  signed long  ChannelLastValueLeft[MC];  // last left-value written into mixerbuffer
  signed long  ChannelLastValueRight[MC]; // last right-value written into mixerbuffer

unsigned long  ChannelVolumeLeft[MC];     // from 0 to 256
unsigned long  ChannelVolumeRight[MC];    // from 0 to 256
  signed long  DeltaSamplePosition[MC];   // delta to add on position
  signed long  DeltaSampleFraction[MC];   // delta to add on fraction

unsigned long  NextSamplePosition[MC];    // sample position after mixing
unsigned long  NextSampleFraction[MC];    // sample fraction after mixing

unsigned long  NextChannelActiv[MC];      // activ-flag after mixing

// NOTE: ALL positions given here are NOT bytes but they ARE in samples !!!


// **************************************************************************

void (*MixRealChannel) ( unsigned long channel, unsigned long counter, unsigned long MixPosition );
void (*MixRealIChannel) ( unsigned long channel, unsigned long counter, unsigned long MixPosition );
void (*MixRealICarry) ( unsigned long channel, unsigned long Position1, unsigned long Position2, unsigned long Fraction, unsigned long MixPosition);


// *** some Volumetable variables ***

extern char *Voltable8;
extern char *MasterVolumeTable;


// *** Mixer initalizations ***

void writeMixer( )
{
  printf("Running %2ibit Mixer with", (MixerType-1) ? 32 : 16 );

  printf("%s interpolation and %s clipping\n\n", flag_interpolate ? "" : " no", flag_softclip ? "soft" : "hard");
}


void initDMAbuffer ( unsigned long length )
{
  Mixerpointer1 = DMApointer;
  Mixerpointer2 = (char *)( DMApointer + length );
}


void initMixer ( unsigned long MixerType )
{
  flag_MixerReady = 0;

  Mixer_length = block_length << flag_stereo;
  if ( MixerType == 1 )  Mixer_length <<= 1;
  if ( MixerType == 2 )  Mixer_length <<= 2;

  MixerBlock = (char *)malloc ( Mixer_length );

  switch (MixerType)
  {
   case 1:
           Voltable8 = (char *)malloc ( 258*256*2 );  // = 128 kb
           calcVol8for16 ( Voltable8 );
           MasterVolumeTable = (char *)malloc ( 132000 ); // = 128 KB
           break;
   case 2:
           Voltable8 = (char *)malloc ( 258*256*4 );  // = 256 kb
           calcVol8for32 ( Voltable8 );
           MasterVolumeTable = (char *)malloc ( 16390*4 ); // = 64 kb
           break;
   case 3:
           break;
  }

  resetMixer();
}


void startMixer ( unsigned long MixerType )
{
  switch (MixerType)
         {
          case 1:
                  if (flag_16bit == 0)
                      calcMasterVol8for16 ( MasterVolume, MasterVolumeTable );
                  else
                      calcMasterVol16for16 ( MasterVolume, MasterVolumeTable );
                  MixRealChannel = Mix16Channel;
                  MixRealIChannel = Mix16IChannel;
                  MixRealICarry = Mix16ICarry;
                  break;
          case 2:
                  calcMasterVolume32 ( MasterVolume, MasterVolumeTable );
                  MixRealChannel = Mix32Channel;
                  MixRealIChannel = Mix32IChannel;
                  MixRealICarry = Mix32ICarry;
                  break;
         }

 flag_MixerReady = 1;
}

void resetMixer ()
{
 register unsigned long channel;

 flag_MixerReady = 0;

  ChannelUsed = 0;
  ChannelLast = 0;

  for (channel = 0; channel < 256; channel++)
      {
       ChannelActiv[channel] = 0;
       ChannelFlag[channel]  = 0;

       ChannelSampleNr[channel]        = 0;
       ChannelVolume[channel]          = 0;
       ChannelPanning[channel]         = 128;
       ChannelSampleNr[channel]        = 0;
       ChannelSampleBits[channel]      = 0;
       ChannelSampleMode[channel]      = 0;
       ChannelSampleAddress[channel]   = 0;
       ChannelSamplePosition[channel]  = 0;
       ChannelSampleFraction[channel]  = 0;
       ChannelSampleFrequence[channel] = 0;
       ChannelLoopMode[channel]        = 0;
       ChannelLoopBegin[channel]       = 0;
       ChannelLoopEnd[channel]         = 0;
       ChannelVolumeLeft[channel]      = 0;
       ChannelVolumeRight[channel]     = 0;
       DeltaSamplePosition[channel]    = 0;
       DeltaSampleFraction[channel]    = 0;
       NextSamplePosition[channel]     = 0;
       NextSampleFraction[channel]     = 0;
       NextChannelActiv[channel]       = 0;
      }
}

void closeMixer ( )
{
  if ( MixerBlock != NULL )  free ( MixerBlock );
  MixerBlock = NULL;

  if ( Voltable8 != NULL )  free ( Voltable8 );
  Voltable8 = NULL;

//  if ( Voltable16 != NULL )  free ( Voltable16 );
//  Voltable16 = NULL;

  if ( MasterVolumeTable != NULL )  free ( MasterVolumeTable );
  MasterVolumeTable = NULL;

  flag_MixerReady = 0;
}


// ************************** the real mixer ********************************

unsigned long Calc_Scaling ( unsigned long channel )
{
 register unsigned long scale;

 scale = divide_64bit (ChannelSampleFrequence[channel] << 1,0,0,MixSpeed);

return ((scale + 1) >> 1);
}


void PrepareMixing()
{
 register unsigned long channel;
 static unsigned long scaling;
 static   signed long Pan;

  for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
     {
      // calculate scaling of every channel

      scaling = Calc_Scaling ( channel );
      if ( scaling < 16 )  ChannelActiv[channel] = 0;
      DeltaSamplePosition[channel] = scaling >> 16;
      DeltaSampleFraction[channel] = scaling & 65535;

      // calculate panning volume

      if ( ChannelVolume[channel] > 256 )  ChannelVolume[channel] = 256;

      if ( flag_stereo == 1 )
         {
          Pan = ( ((signed long)(ChannelPanning[channel] & 511) - 128) * (signed long)ChannelSeperation + 128 ) / 256;

          if ( Pan > 0 )
             ChannelVolumeLeft[channel] = ( ChannelVolume[channel] * (128 - Pan) ) >> 7;
          else  ChannelVolumeLeft[channel] = ChannelVolume[channel];

          if ( Pan < 0 )
             ChannelVolumeRight[channel] = ( ChannelVolume[channel] * (128 + Pan) ) >> 7;
          else  ChannelVolumeRight[channel] = ChannelVolume[channel];
         }
     }
}


void DoPreMixing ( unsigned long MixLength )
{
 register unsigned long channel;

 static   signed long DeltaPosition, DeltaFraction;
 static unsigned long Position, Fraction;
 static unsigned long EndPosition, EndFraction;
 static unsigned long LengthPosition, LengthFraction;

 PrepareMixing();

 for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
     {

      Position = ChannelSamplePosition[channel];
      Fraction = ChannelSampleFraction[channel];

      if ( Position >= ChannelLoopEnd[channel] )
          ChannelLoopMode[channel] = 0;

      if ( Position >= ChannelSampleLength[channel] )
         {
          Position = ChannelSamplePosition[channel] = NextSamplePosition[channel] = 0;
          Fraction = ChannelSampleFraction[channel] = NextSampleFraction[channel] = 0;
          ChannelActiv[channel] = NextChannelActiv[channel] = 0;
         }

      if ( ChannelActiv[channel] == 1)    // if channel is aciv
         {
          DeltaPosition = DeltaSamplePosition[channel];
          DeltaFraction = DeltaSampleFraction[channel];

          switch ( ChannelLoopMode[channel] )
                 {
                  case 0: // no loop

                          EndFraction = Fraction + (DeltaFraction * MixLength);
                          EndPosition = Position + (DeltaPosition * MixLength) + (EndFraction >> 16);
                          EndFraction = EndFraction & 65535;

                          if ( EndPosition < ChannelSampleLength[channel] )
                             {
                              NextChannelActiv[channel] = 1;
                             }
                          else
                             {
                              EndPosition = 0;
                              EndFraction = 0;
                              NextChannelActiv[channel] = 0;
                             }

                          NextSamplePosition[channel] = EndPosition;
                          NextSampleFraction[channel] = EndFraction; 

                          break;

                  case 1: // loop forward

                          EndFraction = Fraction + (DeltaFraction * MixLength);
                          EndPosition = Position + (DeltaPosition * MixLength) + ( EndFraction >> 16 );
                          EndFraction = EndFraction & 65535;

                          while ( EndPosition >= ChannelLoopEnd[channel] )
                            {
                             EndPosition = EndPosition - ChannelLoopEnd[channel] + ChannelLoopBegin[channel];
                            }

                          NextSamplePosition[channel] = EndPosition;
                          NextSampleFraction[channel] = EndFraction;

                          NextChannelActiv[channel] = 1;
                          break;

                  case 2: // loop reverse

                          break;

                  case 3: // loop pingpong
                          break;

                 }
         }
      }

 if ( IRQ_counter < 2 )

 switch ( MixerType )
        {
         case 1:
                 DoMixing ( MixLength );
                 break;
         case 2:
                 DoMixing ( MixLength );
                 break;
         case 3:
                 //DoC16Mixing ( MixLength );
                 break;
        }

 for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
     {
       if ( ChannelActiv[channel] == 1 )
          {
#ifdef cdebug
           if ( ChannelSamplePosition[channel] != NextSamplePosition[channel] )
              { 
               printf("\nChannel: %u\n", channel);
               printf("Calc NextPos wrong !, %i\n", (signed long)NextSamplePosition[channel]-(signed long)ChannelSamplePosition[channel]);
               printf("ChPos: %u\n",ChannelSamplePosition[channel]);
              }
#endif
           ChannelSamplePosition[channel] = NextSamplePosition[channel];
           ChannelSampleFraction[channel] = NextSampleFraction[channel];

           ChannelActiv[channel] = NextChannelActiv[channel];
          }
     }

}


void clearMix16 ();
#pragma aux clearMix16 modify exact [eax ecx edi] = \
            "mov ecx, Mixer_length"\
            "mov edi, MixerBlock"\
            "mov eax, 0x80008000"\
            "shr ecx,2"\
            "rep stosd";


void clearMix32 ();
#pragma aux clearMix32 modify exact [eax ecx edi] = \
            "mov ecx, Mixer_length"\
            "mov edi, MixerBlock"\
            "mov eax, 0x00000000"\
            "shr ecx,2"\
            "rep stosd";


void clearMixer ()
{
 switch ( MixerType )
        {
         case 1:
                 clearMix16();
                 break;
         case 2:
                 clearMix32();
                 break;
         case 3:
                 clearMix16();
                 break;
        }

}


void DoMixing ( unsigned long MixLength )
{
 register unsigned long channel;

 static unsigned long counter, counter1, counter2;

 static   signed long DeltaPosition, DeltaFraction;
 static unsigned long Position, Fraction;
 static unsigned long Length, LoopBegin, LoopEnd;

 static unsigned long EndPosition, EndFraction;
 static unsigned long LengthPosition, LengthFraction;

 static unsigned long MixLengthRest, MixPosition;


 if ( flag_interpolate == 0 )  // not interpolated
    {

     for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
         {

          if ( ChannelActiv[channel] == 1)    // if channel is aciv
             {
              DeltaPosition = DeltaSamplePosition[channel];
              DeltaFraction = DeltaSampleFraction[channel];
              Position = ChannelSamplePosition[channel];
              Fraction = ChannelSampleFraction[channel];

              switch ( ChannelLoopMode[channel] )
                     {
                      case 0: // no loop

                              Length = ChannelSampleLength[channel];

                              LengthPosition = Length - Position;
                              LengthFraction = (65536L - Fraction) & 65535;

                              if ( LengthFraction != 0 )
                                   LengthPosition--;

                              counter = divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );
                              if (Div_Fraction != 0)  counter++;

                              if ( counter != 0 )
                              if ( counter > MixLength )
                                 {
                                  counter = MixLength;
                                  MixRealChannel ( channel, counter, MixerAddress );
                                 }
                              else
                                  MixRealChannel ( channel, counter, MixerAddress );
#ifdef cdebug
                              Fraction = Fraction + (DeltaFraction * counter);
                              Position = Position + (DeltaPosition * counter) + (Fraction >> 16);
                              Fraction = Fraction & 65535;
                              if ( Position >= Length )
                                 {
                                  Position = 0;
                                  Fraction = 0;
                                 }
                              ChannelSamplePosition[channel] = Position;
                              ChannelSampleFraction[channel] = Fraction;
#endif
                              break;

                      case 1: // loop forward

                              LoopBegin = ChannelLoopBegin[channel];
                              LoopEnd = ChannelLoopEnd[channel];

                              MixLengthRest = MixLength;
                              MixPosition = MixerAddress;

                              while ( MixLengthRest != 0 )
                                    {
                                     LengthPosition = LoopEnd - Position;
                                     LengthFraction = (65536L - Fraction) & 65535;

                                     if ( LengthFraction != 0 )
                                          LengthPosition--;

                                     counter = divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );
                                     if (Div_Fraction != 0)  counter++;

                                     if ( counter != 0 )
                                     if ( counter > MixLengthRest )
                                        {
                                         counter = MixLengthRest;
                                         MixRealChannel ( channel, counter, MixPosition );
                                         MixPosition += counter;
                                         MixLengthRest = 0;
                                        }
                                     else
                                        {
                                         MixRealChannel ( channel, counter, MixPosition );
                                         MixPosition += counter;
                                         MixLengthRest -= counter;
                                        }
                                     Fraction = Fraction + (DeltaFraction * counter);
                                     Position = Position + (DeltaPosition * counter) + (Fraction >> 16);
                                     Fraction = Fraction & 65535;
                                     if ( Position >= LoopEnd )  Position = Position - LoopEnd + LoopBegin;
                                     ChannelSamplePosition[channel] = Position;
                                     ChannelSampleFraction[channel] = Fraction;
                                    }  // end while loop
                              break;

                     case 2: // loop reverse

                             break;

                     case 3: // loop pingpong

                             break;
                    }
            }  // end if activ
        }  // end channel loop
    }
 else  // ***************** interpolation is on ***********************
    {
     for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
         {

          if ( ChannelActiv[channel] == 1)    // if channel is aciv
             {
              DeltaPosition = DeltaSamplePosition[channel];
              DeltaFraction = DeltaSampleFraction[channel];
              Position = ChannelSamplePosition[channel];
              Fraction = ChannelSampleFraction[channel];

              switch ( ChannelLoopMode[channel] )
                     {
                      case 0: // no loop

                              Length = ChannelSampleLength[channel];

                              MixLengthRest = MixLength;
                              MixPosition = MixerAddress;

                              LengthPosition = Length - Position;
                              LengthFraction = (65536L - Fraction) & 65535;

#ifdef cdebug
                              if ( LengthPosition == 0 ) printf("Holla!\n");
#endif
                              if ( LengthFraction != 0 )
                                  LengthPosition--;

                              counter = divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );
                              if ( Div_Fraction != 0 )  counter++;

                              if ( LengthPosition >= 1 )
                                 {
                                  counter1 = divide_64bit( LengthPosition-1, LengthFraction, DeltaPosition, DeltaFraction );
                                  if ( Div_Fraction != 0 )  counter1++;
                                 }
                              else
                                   counter1 = 0;

                              counter2 = counter - counter1;

                              if ( counter1 != 0 ) // if no illegal value
                                 {
                                  if ( counter1 > MixLengthRest )
                                     {
                                      counter = MixLengthRest;
                                      MixRealIChannel ( channel, counter, MixPosition );
                                      MixPosition += counter;
                                      MixLengthRest = 0;
                                     }
                                  else
                                     {
                                      counter = counter1;
                                      MixRealIChannel ( channel, counter, MixPosition );
                                      MixPosition += counter;
                                      MixLengthRest -= counter;
                                     }
                                  Fraction = Fraction + (DeltaFraction * counter);
                                  Position = Position + (DeltaPosition * counter) + (Fraction >> 16);
                                  Fraction = Fraction & 65535;
                                  ChannelSamplePosition[channel] = Position;
                                  ChannelSampleFraction[channel] = Fraction;
                                 }

                              if ( MixLengthRest == 0 )  break;

                              if ( counter2 != 0 )  // if no illegal value
                              if ( counter2 > MixLengthRest )
                                 {
                                  counter = MixLengthRest;
                                  while ( counter > 0 )
                                        {
                                         EndPosition = Position + 1;
                                         if ( EndPosition >= Length )  EndPosition = Length-1;
                                         MixRealICarry ( channel, Position, EndPosition, Fraction, MixPosition );
                                         Fraction = Fraction + DeltaFraction;
                                         Position = Position + DeltaPosition + (Fraction >> 16);
                                         Fraction = Fraction & 65535;
                                         MixPosition++;
                                         counter--;
#ifdef cdebug
                                         if ( Position >= Length )
                                            {
                                             printf("Shit1! counter: %u\n", counter);
                                             //Position = Length-1;
                                            }
#endif
                                        }
                                  MixLengthRest = 0;
                                 }
                              else
                                 {
                                  counter = counter2;
                                  while ( counter > 0 )
                                        {
                                         EndPosition = Position + 1;
                                         if ( EndPosition >= Length )  EndPosition = Length-1;
                                         MixRealICarry ( channel, Position, EndPosition, Fraction, MixPosition );
                                         Fraction = Fraction + DeltaFraction;
                                         Position = Position + DeltaPosition + (Fraction >> 16);
                                         Fraction = Fraction & 65535;
                                         MixPosition++;
                                         counter--;
#ifdef cdebug
                                         if ( (Position-DeltaPosition) > Length ) // weil Pos immer eins groesser
                                            {
                                             printf("\nShit2! %i  counter: %u\n", Length-Position, counter);
                                             printf("counter1: %u  counter2: %u\n", counter1, counter2);
                                             printf("DeltaPosition: %u  DeltaFraction: %u\n", DeltaPosition, DeltaFraction);
                                             printf("CPosition: %u  CFraction: %u\n",ChannelSamplePosition[channel], ChannelSampleFraction[channel]);
                                             printf("Length: %u\n",Length);
                                             printf("LengthPos: %u  LengthFrac: %u\n", LengthPosition, LengthFraction);
                                             printf("MixLengthRest: %u\n", MixLengthRest);
                                             //Position = Length-1;
                                            }
#endif
                                        }
                                  MixLengthRest -= counter2;
                                 }
#ifdef cdebug
                              if ( Position >= Length )
                                 {
                                  Position = 0;
                                  Fraction = 0;
                                 }              
                              ChannelSamplePosition[channel] = Position;
                              ChannelSampleFraction[channel] = Fraction;
#endif
                              break;

                      case 1: // loop forward

                              LoopBegin = ChannelLoopBegin[channel];
                              LoopEnd = ChannelLoopEnd[channel];

                              MixLengthRest = MixLength;
                              MixPosition = MixerAddress;

                              while ( MixLengthRest > 0 )
                                    {
                                     LengthPosition = LoopEnd - Position;
                                     LengthFraction = (65536L - Fraction) & 65535;

                                     if ( LengthFraction > 0 )
                                          LengthPosition--;

                                     counter = divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );
                                     if ( Div_Fraction != 0 )  counter++;

                                     if ( LengthPosition >= 1 )
                                        {
                                         counter1 = divide_64bit( LengthPosition-1, LengthFraction, DeltaPosition, DeltaFraction );
                                         if ( Div_Fraction != 0 )  counter1++;
                                        }
                                     else
                                          counter1 = 0;

                                     counter2 = counter - counter1;

                                     if ( counter1 != 0 )  // if no illegal value
                                        {
                                         if ( counter1 > MixLengthRest )
                                            {
                                             counter = MixLengthRest;
                                             MixRealIChannel ( channel, counter, MixPosition );
                                             MixPosition += counter;
                                             MixLengthRest = 0;
                                            }
                                         else
                                            {
                                             counter = counter1;
                                             MixLengthRest -= counter;
                                             MixRealIChannel ( channel, counter, MixPosition );
                                             MixPosition += counter;
                                            }
                                         Fraction = Fraction + (DeltaFraction * counter);
                                         Position = Position + (DeltaPosition * counter) + (Fraction >> 16);
                                         Fraction = Fraction & 65535;
                                         if ( Position >= LoopEnd )  Position = Position - LoopEnd + LoopBegin;
                                         ChannelSamplePosition[channel] = Position;
                                         ChannelSampleFraction[channel] = Fraction;
                                        }

                                     if ( MixLengthRest == 0 )  break;

                                     if ( counter2 != 0 )  // if no illegal value
                                     if ( counter2 > MixLengthRest )
                                        {
                                         counter = MixLengthRest;
                                         while ( counter > 0 )
                                               {
                                                EndPosition = Position + 1;
                                                if ( EndPosition >= LoopEnd )  EndPosition = EndPosition - LoopEnd + LoopBegin;
                                                MixRealICarry ( channel, Position, EndPosition, Fraction, MixPosition );
                                                Fraction = Fraction + DeltaFraction;
                                                Position = Position + DeltaPosition + (Fraction >> 16);
                                                Fraction = Fraction & 65535;
                                                MixPosition++;
                                                counter--;
                                               }
                                         MixLengthRest = 0;
                                        }
                                     else
                                        {
                                         counter = counter2;
                                         while ( counter > 0 )
                                               {
                                                EndPosition = Position + 1;
                                                if ( EndPosition >= LoopEnd )  EndPosition = EndPosition - LoopEnd + LoopBegin;
                                                MixRealICarry ( channel, Position, EndPosition, Fraction, MixPosition );
                                                Fraction = Fraction + DeltaFraction;
                                                Position = Position + DeltaPosition + (Fraction >> 16);
                                                Fraction = Fraction & 65535;
                                                MixPosition++;
                                                counter--;
                                               }
                                         MixLengthRest -= counter2;
#ifdef cdebug
                                         if ( Position != LoopEnd )
                                            {
                                             printf("LoopPos: %i\n",LoopEnd-Position);
                                            }
#endif
                                         if ( Position >= LoopEnd )  Position = Position - LoopEnd + LoopBegin;
                                        }
#ifdef cdebug
                                     if ( Position >= LoopEnd)
                                        printf("Fault2 in loop!\n");
#endif
                                     ChannelSamplePosition[channel] = Position;
                                     ChannelSampleFraction[channel] = Fraction;

                                    }  // end while loop
                              break;

                     case 2: // loop reverse

                             break;

                     case 3: // loop pingpong

                             break;
                    }
            }  // end if activ
        }  // end channel loop
    }
}



static void Mix16Channel ( unsigned long channel, unsigned long counter, unsigned long MixPosition )
{
 static unsigned long Volume, VolumeLeft, VolumeRight;
 static unsigned long VolTable, VolLeftTable, VolRightTable;
 static unsigned long FracCounter, SampleAddress;
 static unsigned long BlockAddress;

 if ( counter == 0 )  return;

 Volume = ( ChannelVolume[channel] * GlobalVolume + 128 ) >> 8;

 if ( Volume != 0 )
    {
     if ( flag_stereo == 0 )    // mono sample
        {
         BlockAddress = (unsigned long)(MixerBlock+(MixPosition << 1)+(counter << 1));

         FracCounter = (unsigned long)ChannelSampleFraction[channel] << 16;

         if ( ChannelSampleBits[channel] == 8 )
            {
             VolTable = (unsigned long)( Voltable8 + ((unsigned long)Volume << 9 ) );
             setup8Mto16M ( VolTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8Mto16M ( FracCounter, SampleAddress, -(counter << 1) );
            }
         else    // 16bit sample
            {
             setup16Mto16M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16Mto16M ( FracCounter, SampleAddress, -(counter << 1) );
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (unsigned long)(MixerBlock+(MixPosition << 2)+(counter << 2) );

         FracCounter = (unsigned long)ChannelSampleFraction[channel] << 16;

         if ( ChannelSampleBits[channel] == 8 )
            {
             VolLeftTable = (unsigned long)( Voltable8 + ((unsigned long)VolumeLeft << 9 ) );
             VolRightTable = (unsigned long)( Voltable8 + ((unsigned long)VolumeRight << 9 ) );
             setup8Mto16S ( VolLeftTable, VolRightTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8Mto16S ( FracCounter, SampleAddress, -(counter << 2) );
            }
         else   // 16bit sample
            {
             setup16Mto16S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16Mto16S ( FracCounter, SampleAddress, -(counter << 2) );
            }
        }  // end stereo
    }  // end if silence
}


static void Mix16IChannel ( unsigned long channel, unsigned long counter, unsigned long MixPosition )
{
 static unsigned long Volume, VolumeLeft, VolumeRight;
 static unsigned long VolTable;
 static unsigned long FracCounter, SampleAddress;
 static unsigned long BlockAddress;

 if ( counter == 0 )  return;

 Volume = ( ChannelVolume[channel] * GlobalVolume + 128 ) >> 8;

 if ( Volume != 0 )
    {
     if ( flag_stereo == 0 )    // mono sample
        {
         BlockAddress = (unsigned long)(MixerBlock+(MixPosition << 1)+(counter << 1));

         FracCounter = (unsigned long)ChannelSampleFraction[channel] << 16;

         if ( ChannelSampleBits[channel] == 8 )
            {
             setup8MIto16M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8MIto16M ( FracCounter, SampleAddress, -(counter << 1) );
            }
         else    // 16bit sample
            {
             setup16MIto16M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16MIto16M ( FracCounter, SampleAddress, -(counter << 1) );
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (unsigned long)(MixerBlock+(MixPosition << 2)+(counter << 2) );

         FracCounter = (unsigned long)ChannelSampleFraction[channel] << 16;

         if ( ChannelSampleBits[channel] == 8 )  // 8bit sample
            {
             setup8MIto16S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8MIto16S ( FracCounter, SampleAddress, -(counter << 2) );
            }
         else   // 16bit sample
            {
             setup16MIto16S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16MIto16S ( FracCounter, SampleAddress, -(counter << 2) );
            }
        } // end stereo
    }  // end if silence
}

static void Mix16ICarry ( unsigned long channel, unsigned long Position1, unsigned long Position2, unsigned long Fraction, unsigned long MixPosition)
{
 static unsigned long Volume, VolumeLeft, VolumeRight;
 static unsigned long SampleAddress1, SampleAddress2;
 static   signed long IntSample, Sample1, Sample2;
 static unsigned long BlockAddress;

 Volume = ( ChannelVolume[channel] * GlobalVolume + 128 ) >> 8;

 if ( Volume != 0 )
    {
     SampleAddress1 = SampleAddress2 = ChannelSampleAddress[channel];

     if ( flag_stereo == 0 )    // mono sample
        {
         BlockAddress = (uns long)( MixerBlock + (MixPosition << 1) );

         if ( ChannelSampleBits[channel] == 8 )
            {
             SampleAddress1 += Position1;
             SampleAddress2 += Position2;
             Sample1 = (signed long) (*(signed char *)SampleAddress1);
             Sample2 = (signed long) (*(signed char *)SampleAddress2);
             IntSample = (Sample2 - Sample1) * (signed long)Fraction + (Sample1 << 16);

             (*(signed short *)(BlockAddress)) += (signed short)( (__int64)IntSample * (__int64)Volume / 16777216);
            }
         else    // 16bit sample
            {
             SampleAddress1 += (Position1 << 1);
             SampleAddress2 += (Position2 << 1);
             Sample1 = (signed long) (*(signed short *)SampleAddress1);
             Sample2 = (signed long) (*(signed short *)SampleAddress2);
             IntSample = (( (__int64)(Sample2 - Sample1) * (__int64)Fraction) / 256) + (Sample1 << 8);

             (*(signed short *)(BlockAddress)) += (signed short)( (__int64)IntSample * (__int64)Volume / 16777216);
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (unsigned long)( MixerBlock + (MixPosition << 2) );

         if ( ChannelSampleBits[channel] == 8 )  // 8bit sample
            {
             SampleAddress1 += Position1;
             SampleAddress2 += Position2;

             Sample1 = (signed long) (*(signed char *)SampleAddress1);
             Sample2 = (signed long) (*(signed char *)SampleAddress2);
             IntSample = (Sample2 - Sample1) * (signed long)Fraction + (Sample1 << 16);

             (*(signed short *)(BlockAddress)) += (signed short)( (__int64)IntSample * (__int64)VolumeLeft / 16777216);
             (*(signed short *)(BlockAddress+2)) += (signed short)( (__int64)IntSample * (__int64)VolumeRight / 16777216);
            }
         else   // 16bit sample
            {
             SampleAddress1 += (Position1 << 1);
             SampleAddress2 += (Position2 << 1);

             Sample1 = (signed long) (*(signed short *)SampleAddress1);
             Sample2 = (signed long) (*(signed short *)SampleAddress2);
             IntSample = (( (__int64)(Sample2 - Sample1) * (__int64)Fraction) / 256) + (Sample1 << 8);

             (*(signed short *)(BlockAddress)) += (signed short)( (__int64)IntSample * (__int64)VolumeLeft / 16777216);
             (*(signed short *)(BlockAddress+2)) += (signed short)( (__int64)IntSample * (__int64)VolumeRight / 16777216);
            }
        } // end stereo
    }  // end if silence
}



static void Mix32Channel ( unsigned long channel, unsigned long counter, unsigned long MixPosition )
{
 static unsigned long Volume, VolumeLeft, VolumeRight;
 static unsigned long VolTable, VolLeftTable, VolRightTable;
 static unsigned long FracCounter, SampleAddress;
 static unsigned long BlockAddress;

 if ( counter == 0 )  return;

 Volume = ( ChannelVolume[channel] * GlobalVolume + 128 ) >> 8;

 if ( Volume != 0 )
    {
     if ( flag_stereo == 0 )    // mono mixer
        {
         BlockAddress = (unsigned long)MixerBlock + (MixPosition << 2) + (counter << 2);

         FracCounter = (unsigned long)ChannelSampleFraction[channel] * 65536;

         if ( ChannelSampleBits[channel] == 8 )
            {
             VolTable = (unsigned long)( Voltable8 + ((unsigned long)Volume << 10 ) );
             setup8Mto32M ( VolTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8Mto32M ( FracCounter, SampleAddress, -(counter << 2) );
            }
         else    // 16bit sample
            {
             setup16Mto32M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16Mto32M ( FracCounter, SampleAddress, -(counter << 2) );
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (unsigned long)MixerBlock + (MixPosition << 3) + (counter << 3);

         FracCounter = (unsigned long)ChannelSampleFraction[channel] * 65536;

         if ( ChannelSampleBits[channel] == 8 )
            {
             VolLeftTable = (unsigned long)( Voltable8 + ((unsigned long)VolumeLeft << 10 ) );
             VolRightTable = (unsigned long)( Voltable8 + ((unsigned long)VolumeRight << 10 ) );
             setup8Mto32S ( VolLeftTable, VolRightTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8Mto32S ( FracCounter, SampleAddress, -(counter << 3) );
            }
         else   // 16bit sample
            {
             setup16Mto32S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16Mto32S ( FracCounter, SampleAddress, -(counter << 3) );
            }
        }  // end stereo
    }  // end if silence
}


static void Mix32IChannel ( unsigned long channel, unsigned long counter, unsigned long MixPosition )
{
 static unsigned long Volume, VolumeLeft, VolumeRight;
 static unsigned long VolTable;
 static unsigned long FracCounter, SampleAddress;
 static unsigned long BlockAddress;

 if ( counter == 0 )  return;

 Volume = ( ChannelVolume[channel] * GlobalVolume + 128 ) >> 8;

 if ( Volume != 0 )
    {
     if ( flag_stereo == 0 )    // mono sample
        {
         BlockAddress = (unsigned long)MixerBlock + (MixPosition << 2) + (counter << 2);

         FracCounter = (unsigned long)ChannelSampleFraction[channel] * 65536;

         if ( ChannelSampleBits[channel] == 8 )
            {
             setup8MIto32M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8MIto32M ( FracCounter, SampleAddress, -(counter << 2) );
            }
         else    // 16bit sample
            {
             setup16MIto32M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16MIto32M ( FracCounter, SampleAddress, -(counter << 2) );
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (unsigned long)MixerBlock + (MixPosition << 3) + (counter << 3);

         FracCounter = (unsigned long)ChannelSampleFraction[channel] * 65536;

         if ( ChannelSampleBits[channel] == 8 )  // 8bit sample
            {
             setup8MIto32S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8MIto32S ( FracCounter, SampleAddress, -(counter << 3) );
            }
         else   // 16bit sample
            {
             setup16MIto32S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (unsigned long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16MIto32S ( FracCounter, SampleAddress, -(counter << 3) );
            }
        } // end stereo
    }  // end if silence
}


static void Mix32ICarry ( unsigned long channel, unsigned long Position1, unsigned long Position2, unsigned long Fraction, unsigned long MixPosition)
{
 static unsigned long Volume, VolumeLeft, VolumeRight;
 static unsigned long SampleAddress1, SampleAddress2;
 static   signed long IntSample, Sample1, Sample2;
 static unsigned long BlockAddress;

// return;
 Volume = ( ChannelVolume[channel] * GlobalVolume + 128 ) / 256;

 if ( Volume != 0 )
    {
     SampleAddress1 = ChannelSampleAddress[channel];
     SampleAddress2 = ChannelSampleAddress[channel];

     if ( flag_stereo == 0 )    // mono sample
        {
         BlockAddress = (unsigned long)MixerBlock + (MixPosition << 2);

         if ( ChannelSampleBits[channel] == 8 )
            {
             SampleAddress1 += Position1;
             SampleAddress2 += Position2;
             Sample1 = (signed long) (*(signed char *)SampleAddress1);
             Sample2 = (signed long) (*(signed char *)SampleAddress2);
             IntSample = (Sample2 - Sample1) * (signed long)Fraction + (Sample1 << 16);

             (*(signed long *)(BlockAddress)) = (*(signed long *)(BlockAddress)) + (signed long)( (__int64)IntSample * (__int64)Volume / 256);
            }
         else    // 16bit sample
            {
             SampleAddress1 += (Position1 << 1);
             SampleAddress2 += (Position2 << 1);
             Sample1 = (signed long) (*(signed short *)SampleAddress1);
             Sample2 = (signed long) (*(signed short *)SampleAddress2);
             IntSample = (( (__int64)(Sample2 - Sample1) * (__int64)Fraction) / 256) + (Sample1 << 8);

             (*(signed long *)(BlockAddress)) = (*(signed long *)(BlockAddress)) + (signed long)( (__int64)IntSample * (__int64)Volume / 256);
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (unsigned long)MixerBlock + (MixPosition << 3);

         if ( ChannelSampleBits[channel] == 8 )  // 8bit sample
            {
             SampleAddress1 += Position1;
//             printf("SampleAddress1: %u %u\n",SampleAddress1, Position1);
             SampleAddress2 += Position2;

             Sample1 = (signed long) (*(signed char *)SampleAddress1);
             Sample2 = (signed long) (*(signed char *)SampleAddress2);
             IntSample = (Sample2 - Sample1) * (signed long)Fraction + (Sample1 << 16);

             (*(signed long *)(BlockAddress+0)) = (*(signed long *)(BlockAddress+0)) + (signed long)( (__int64)IntSample * (__int64)VolumeLeft / 256);
             (*(signed long *)(BlockAddress+4)) = (*(signed long *)(BlockAddress+4)) + (signed long)( (__int64)IntSample * (__int64)VolumeRight / 256);
            }
         else   // 16bit sample
            {
             //printf("Uuups ! 16bit !\n");
             SampleAddress1 += (Position1 * 2);
             SampleAddress2 += (Position2 * 2);

             Sample1 = (signed long) (*(signed short *)SampleAddress1);
             Sample2 = (signed long) (*(signed short *)SampleAddress2);
             IntSample = ( ( (__int64)(Sample2 - Sample1) * (__int64)Fraction) / 256) + (Sample1 << 8);

             (*(signed long *)(BlockAddress+0)) = (*(signed long *)(BlockAddress+0)) + (signed long)( (__int64)IntSample * (__int64)VolumeLeft / 256);
             (*(signed long *)(BlockAddress+4)) = (*(signed long *)(BlockAddress+4)) + (signed long)( (__int64)IntSample * (__int64)VolumeRight / 256);
            }

        } // end stereo

    }  // end if silence
}



