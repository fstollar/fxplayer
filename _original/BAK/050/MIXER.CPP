#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>

#include "fx.h"

#include "irq.h"
#include "dma.h"

#include "device.h"
#include "dev_sb.h"

#include "format.h"
#include "dat_wav.h"
//#include "dat_s3m.h"

#include "mixer.h"
#include "convert.h"

// *** global mixer variables ***

unsigned short ChannelSeperation = 256;    // seperation of channels
                                           // 256 = max, 0 = mono
unsigned short GlobalVolume = 256;         // global volume from 0 to 256
unsigned short MasterVolume = 0;           // master volume, is realtive
                                           // volume of every channel to mix

// *** global mixerchannel variables ***

unsigned short ChannelUsed = 0, ChannelLast = 0;

unsigned char  ChannelActiv[256];          // 0 = quiet, 1 = activ
unsigned char  ChannelFlag[256];           // 0 = available, 1 = paused

unsigned short ChannelSampleNr[256];
unsigned short ChannelVolume[256];         // from 0 to 256
unsigned short ChannelPanning[256];        // from 0 to 256, 128 is middle, +32768 = surround
unsigned char  ChannelSampleBits[256];     // 8 or 16 bit
unsigned char  ChannelSampleMode[256];     // 1 = mono, 2 = stereo sample
unsigned long  ChannelSampleAdress[256];   // start adress of current sample
unsigned long  ChannelSampleLength[256];   // position length of sample INCLUSIVE
unsigned char  ChannelLoopMode[256];       // 0 = no, 1 = loop, 2 = reverse, 3 = pingpong
unsigned long  ChannelLoopBegin[256];      // position where loop starts
unsigned long  ChannelLoopEnd[256];        // position where loop ends EXCLUSIVE
unsigned long  ChannelSamplePosition[256]; // relative offset in sample
unsigned short ChannelSampleFraction[256]; // fraction of sample position
unsigned long  ChannelSampleFrequence[256];// frequence to play sample

unsigned short ChannelVolumeLeft[256];     // from 0 to 256
unsigned short ChannelVolumeRight[256];    // from 0 to 256
unsigned long  DeltaSamplePosition[256];   // delta to add on position
unsigned short DeltaSampleFraction[256];   // delta to add on fraction

unsigned long  NextSamplePosition[256];    // sample position after mixing
unsigned short NextSampleFraction[256];    // sample fraction after mixing

unsigned char  NextChannelActiv[256];      // activ-flag after mixing

// NOTE: ALL positions given here are NOT bytes but they ARE in samples !!!

// *** some Volumetable variables ***

char *Voltable8 = NULL;

unsigned short Voltable16count = 4;
char *Voltable16 = NULL;
unsigned long  Table16Adress[256];
unsigned short Table16Volume[256];
unsigned short Table16Hits[256];

char *MasterVolumeTable = NULL;


// *** Main programm ***

void initDMAbuffer ( unsigned long buffer_length )
{
  Mixerpointer1 = DMApointer;
  Mixerpointer2 = (char *)(DMApointer+buffer_length );
  //Mixerpointer = Mixerpointer1;
}


void initMixer ( unsigned short MixerType )
{
  MixerBlock = (char *)malloc ( block_length * 8 );

  switch (MixerType)
  {
   case 1:
           Voltable8 = (char *)malloc ( 257*256*2 );  // = 128 kb
           //Voltable16 = (char *)malloc ( 65536*2*Voltable16count ); // je 128K
           MasterVolumeTable = (char *)malloc ( 131072 ); // = 128 KB

           calcVol8for16 ( Voltable8 );

           break;
   case 2:
           Voltable8 = (char *)malloc ( 257*256*4 );  // = 256 kb
           //Voltable16 = (char *)malloc ( 65536*4*Voltable16count ); // je 256K

           break;
   case 3:
           break;
  }

  resetMixer();
}


void startMixer ( unsigned short MixerType )
{
  switch (MixerType)
         {
          case 1:
                  if (flag_16bit == 0)
                      calcMasterVol8for16 ( MasterVolume, MasterVolumeTable );
                  else
                      calcMasterVol16for16 ( MasterVolume, MasterVolumeTable );
                  break;
          case 2:
                  break;
         }
}

void resetMixer ()
{
//  GlobalVolume = 256;
  ChannelUsed = 0;
  ChannelLast = 0;

  for (test=0; test<256; test++)
      {
       ChannelActiv[test] = 0;
       ChannelFlag[test]  = 0;

       ChannelSampleNr[test] = 0;
       ChannelVolume[test]   = 0;
       ChannelPanning[test]  = 128;
       ChannelSampleBits[test] = 0;
       ChannelSampleMode[test] = 0;
       ChannelSampleAdress[test] = 0;
       ChannelSamplePosition[test] = 0;
       ChannelSampleFraction[test] = 0;
       ChannelLoopMode[test]       = 0;
       ChannelLoopBegin[test]      = 0;
       ChannelLoopEnd[test]        = 0;
       ChannelVolumeLeft[test]     = 0;
       ChannelVolumeRight[test]    = 0;
       NextSamplePosition[test]    = 0;
       NextSampleFraction[test]    = 0;
      }
}

void closeMixer ( )
{
  if ( MixerBlock != NULL )  free ( MixerBlock );
  MixerBlock = NULL;

  if ( Voltable8 != NULL )  free ( Voltable8 );
  Voltable8 = NULL;

  if ( Voltable16 != NULL )  free ( Voltable16 );
  Voltable16 = NULL;

  if ( MasterVolumeTable != NULL )  free ( MasterVolumeTable );
  MasterVolumeTable = NULL;
}



// ************************* table calculations *****************************

void calcVol8for16 ( char *VolPointer )
{
 long Volume, Sample;

 for (Volume=0; Volume<=256; Volume++)
   {
    for (Sample = 0; Sample <= 127; Sample++)
      {
       *(short *)VolPointer = (short)( ( Sample * Volume + 8 ) >> 4 );
       VolPointer+=2;
      }
    for (Sample = -128; Sample <= -1; Sample++)
      {
       *(short *)VolPointer = (short)( ( Sample * Volume - 8 ) >> 4 );
       VolPointer+=2;
      }
   }
}

void calcMasterVol8for16 ( unsigned short MasterVolume , char *Pointer)
{
 // MasterVolume gives the amount of multiplication of every channel
 // for the end audiostream.
 // 32768 = Steigung von 1 (4098 steps from 0 to 255)
 // 16384 = Steigung von 0.5 (8192 steps)
 // 8192 = 0.25 .....
 // S3M-Mastervolume * 256 = MasterVolume

 unsigned long Ramp_start, Ramp_end, Ramp_delta;
 unsigned long runi;
 unsigned long value;

 if (MasterVolume < 32 )  MasterVolume = 32;

 Ramp_delta = (2 * (32768*4086) / MasterVolume + 1) >> 1;
 Ramp_start = (65536 - Ramp_delta + 1) >> 1;
 Ramp_end   = Ramp_start + Ramp_delta;

 for (runi = 0; runi < 65536; runi++)
     {
      if (runi < Ramp_start)       value = 0;
      if ( (Ramp_start <= runi) && (runi < Ramp_end) )
                                   value = ( (((runi - Ramp_start) * (uns long)MasterVolume) + 262144L) >> 19);
      if (runi >= Ramp_end)        value = 255;

      *Pointer++ = value;
     }
}


void calcMasterVol16for16 ( unsigned short MasterVolume , char *Pointer)
{
 // MasterVolume gives the amount of multiplication of every channel
 // for the end audiostream.
 // see above...

 unsigned long Ramp_start, Ramp_end, Ramp_delta;
 unsigned long runi;
 unsigned long value;

 if (MasterVolume < 32 )  MasterVolume = 32;

 Ramp_delta = (2 * (32768*4096) / MasterVolume + 1) >> 1;
 Ramp_start = (65536 - Ramp_delta + 1) >> 1;
 Ramp_end   = Ramp_start + Ramp_delta;

 for (runi = 0; runi < 65536; runi++)
     {
      if (runi < Ramp_start)       value = 0;
      if ( (Ramp_start <= runi) && (runi < Ramp_end) )
                                   value = (unsigned short)( ((runi - Ramp_start) * (uns long)MasterVolume + 1024) >> 11);
      if (runi >= Ramp_end)        value = 65535;

      *(short *)Pointer = value;
      Pointer+=2;
     }
}


void MasterVol8for16 ( char *Source, char *Destin, unsigned long length)
{
 // MasterVolume gives the amount of multiplication of every channel
 // for the end audiostream.
 // see above...

 unsigned long runi;

 for (runi = 0; runi < length; runi++)
     {
      *(uns char *)Destin= *(uns char *)(MasterVolumeTable + (uns long)(*(uns short *)Source) );

      Destin++;
      Source+=2;
     }
}


void MasterVol16for16 ( char *Source, char *Destin, unsigned long length)
{
 // MasterVolume gives the amount of multiplication of every channel
 // for the end audiostream.
 // see above...

 unsigned long runi;

 for (runi = 0; runi < length; runi++)
     {
      *(uns short *)Destin= *(uns short *)(MasterVolumeTable + ((uns long)(*(uns short *)Source) << 1) ) ;

      Destin+=2;
      Source+=2;
     }
}


void DoMasterVolumeCalculations ( char *Source, char *Destin, unsigned long length)
{
   switch (MixerType)
         {
          case 1:
                  if (flag_16bit == 0)
                      MasterVol8for16 ( Source, Destin, length << flag_stereo );
                  else
                      MasterVol16for16 ( Source, Destin, length << flag_stereo );
                  break;
          case 2:
                  break;
         }
}

// ************************** memory fill functions *************************

void set8M ( char *target, char value, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *target++ = (char) value;
     }
}

void set16M ( char *target, unsigned short value, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)(target) = (short)value;
      target+=2;
     }
}


// ************************** the real mixer ********************************

static unsigned long Division;
static unsigned long Div_Fraction;

unsigned long divide_64bit( unsigned long Position, unsigned short Fraction, unsigned long DivPosition, unsigned short DivFraction);
#pragma aux divide_64bit parm [eax] [edx] [ebx] [ecx] value [eax] modify exact [eax ebx ecx edx] = \
    "shl ebx,16"\
    "mov bx,cx"\
    "mov ecx,edx"\
    "xor edx,edx"\
    "shld edx,eax,16"\
    "shl eax,16"\
    "mov ax,cx"\
    "div ebx"\
    "mov Division,eax"\
    "mov Div_Fraction,edx";


unsigned long Calc_Scaling ( unsigned long channel )
{
 unsigned long scale;

 scale = divide_64bit (ChannelSampleFrequence[channel] << 1,0,0,MixSpeed);

return ((scale + 1) >> 1);
}


void PrepareMixing()
{
unsigned long channel;
unsigned long scaling;

signed short Pan;

  for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
     {
      // calculate scaling of every channel

      scaling = Calc_Scaling ( channel );
      if ( scaling < 16 )  ChannelActiv[channel] = 0;
      DeltaSamplePosition[channel] = scaling >> 16;
      DeltaSampleFraction[channel] = scaling & 65535;

      // calculate panning volume

      if ( flag_stereo == 1 )
         {
          Pan = ( ((long)(ChannelPanning[channel] & 511) - 128) * ChannelSeperation + 128 ) / 256;

          if ( Pan > 0 )
             ChannelVolumeLeft[channel] = ( ChannelVolume[channel] * (128 - Pan) ) >> 7;
          else  ChannelVolumeLeft[channel] = ChannelVolume[channel];

          if ( Pan < 0 )
             ChannelVolumeRight[channel] = ( ChannelVolume[channel] * (128 + Pan) ) >> 7;
          else  ChannelVolumeRight[channel] = ChannelVolume[channel];
         }
     }
}


void DoMixing ( unsigned long MixLength )
{
 unsigned long channel;

 long DeltaPosition, DeltaFraction;
 unsigned long Position, Fraction;
 unsigned long EndPosition, EndFraction;
 unsigned long LengthPosition, LengthFraction;

 PrepareMixing();

 for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
     {
      if ( ChannelActiv[channel] == 1)    // if channel is aciv
         {
          DeltaPosition = DeltaSamplePosition[channel];
          DeltaFraction = DeltaSampleFraction[channel];
          Position = ChannelSamplePosition[channel];
          Fraction = ChannelSampleFraction[channel];

          if ( Position >= ChannelLoopEnd[channel] )
              ChannelLoopMode[channel] = 0;

          if ( Position > ChannelSampleLength[channel] )
             {
              NextSamplePosition[channel] = ChannelSampleLength[channel];
              NextSampleFraction[channel] = 0;
              NextChannelActiv[channel] = 0;
             }
          else

          switch ( ChannelLoopMode[channel] )
                 {
                  case 0: // no loop

                          EndPosition = Position + (DeltaPosition * MixLength) + (((Fraction + (DeltaFraction * MixLength)) >> 16) );
                          EndFraction = (Fraction + (DeltaFraction * MixLength)) & 65535;

                          if ( EndPosition < ChannelSampleLength[channel] )
                             {
                              NextSamplePosition[channel] = EndPosition;
                              NextSampleFraction[channel] = EndFraction;

                              NextChannelActiv[channel] = 1;
                             }
                          else
                             {
                              NextSamplePosition[channel] = ChannelSampleLength[channel];
                              NextSampleFraction[channel] = 0;

                              NextChannelActiv[channel] = 0;
                             }
                          break;

                  case 1: // loop forward

                          EndPosition = Position + (DeltaPosition * MixLength) + (((Fraction + (DeltaFraction * MixLength)) >> 16) );
                          EndFraction = (Fraction + (DeltaFraction * MixLength)) & 65535;

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
                 Do16Mixing ( MixLength );
                 break;
         case 2:
                 break;
         case 3:
                 //DoC16Mixing ( MixLength );
                 break;
        }

 for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
     {
       ChannelSamplePosition[channel] = NextSamplePosition[channel];
       ChannelSampleFraction[channel] = NextSampleFraction[channel];

       ChannelActiv[channel] = NextChannelActiv[channel];
     }

}


void clearMix16 ();
#pragma aux clearMix16 modify exact [eax ecx edi] = \
            "mov ecx, buffer_length"\
            "shr ecx, 1"\
            "mov edi, MixerBlock"\
            "mov eax, 0x80008000"\
            "rep stosd";


void clearMix32 ();
#pragma aux clearMix32 modify exact [eax ecx edi] = \
            "mov ecx, buffer_length"\
            "mov edi, MixerBlock"\
            "mov eax, 0x80000000"\
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


void Do16Mixing ( unsigned long MixLength )
{
 unsigned long channel;

 unsigned long counter;

 long DeltaPosition, DeltaFraction;
 unsigned long Position, Fraction;
 unsigned long EndPosition, EndFraction;
 unsigned long LengthPosition, LengthFraction;

 unsigned long MixLength_rest, MixLoopPos;


 for (channel = 0; channel < ChannelLast; channel++)  // go through all channels
     {
      if ( ChannelActiv[channel] == 1)    // if channel is aciv
         {
          DeltaPosition = DeltaSamplePosition[channel];
          DeltaFraction = DeltaSampleFraction[channel];
          Position = ChannelSamplePosition[channel];
          Fraction = ChannelSampleFraction[channel];

          if ( Position >= ChannelLoopEnd[channel] )
              ChannelLoopMode[channel] = 0;

          if ( Position >= ChannelSampleLength[channel] )
             {
              NextSamplePosition[channel] = ChannelSampleLength[channel];
              NextSampleFraction[channel] = 0;
              NextChannelActiv[channel] = 0;
             }

          else

          switch ( ChannelLoopMode[channel] )
                 {
                  case 0: // no loop

                          LengthPosition = ChannelSampleLength[channel] - Position;
                          LengthFraction = (65536 - Fraction) & 65535;
                          if ( Fraction != 0 )   LengthPosition--;

                          counter = 1 + divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );

                          if ( counter > MixLength )   counter = MixLength;

                          Mix16Channel ( channel, counter, (MixerAdress << 1) );

                          break;

                  case 1: // loop forward

                          MixLength_rest = MixLength;
                          MixLoopPos = MixerAdress;

                          while ( MixLength_rest != 0 )
                            {
                             LengthPosition = ChannelLoopEnd[channel] - Position;
                             LengthFraction = (65536 - Fraction) & 65535;
                             if ( Fraction != 0 )
                                  LengthPosition--;

                            counter = 1 + divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );

                            //if ( counter == 0 )
                            //   {
                            //    printf("Aua!!  %u:%u\r\n",LengthPosition,DeltaPosition);
                            //    break;
                            //   }

                             if ( counter > MixLength_rest )
                                 {
                                  counter = MixLength_rest;
                                  MixLength_rest = 0;

                                  Mix16Channel ( channel, counter, (MixLoopPos << 1) );

                                  MixLoopPos+= counter;
                                 }
                              else
                                 {
                                  MixLength_rest-= counter;

                                  Mix16Channel ( channel, counter, (MixLoopPos << 1) );

                                  MixLoopPos+= counter;
                                  EndFraction = Fraction + (DeltaFraction * counter);
                                  EndPosition = Position + (DeltaPosition * counter) + (EndFraction >> 16);
                                  EndFraction = EndFraction & 65535;
                                  Position = EndPosition - ChannelLoopEnd[channel] + ChannelLoopBegin[channel];
                                  Fraction = EndFraction;
                                  ChannelSamplePosition[channel] = Position;
                                  ChannelSampleFraction[channel] = Fraction;
                                 }

                            }  // while

                          break;

                  case 2: // loop reverse

                          break;

                  case 3: // loop pingpong

                          break;
                 }
         }
     }
}



static void Mix16Channel ( unsigned long channel, unsigned long counter , unsigned long MixPosition )
{
 static uns long Volume, VolumeLeft, VolumeRight;
 static uns long VolTable, VolLeftTable, VolRightTable;
 static uns long FracCounter, SampleAdress;
 static uns long BlockAddress;

 Volume = ( ChannelVolume[channel] * GlobalVolume + 128 ) >> 8;
 VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
 VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

 if ( Volume != 0 )
    {
     if ( flag_stereo == 0 )
        {
         VolTable = (uns long)( Voltable8 + ((uns long)Volume << 9 ) );
         BlockAddress = (uns long)(MixerBlock+MixPosition+(counter << 1));

         setup8Mto16M ( VolTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );

         FracCounter = (uns long)ChannelSampleFraction[channel] << 16;
         SampleAdress = (uns long)(ChannelSampleAdress[channel] + ChannelSamplePosition[channel]);

         mix8Mto16M ( FracCounter, SampleAdress, -( counter << 1 ) );
        }
     else
        {
         VolLeftTable = (uns long)( Voltable8 + ((unsigned long)(VolumeLeft) << 9 ) );
         VolRightTable = (uns long)( Voltable8 + ((unsigned long)(VolumeRight) << 9 ) );

         FracCounter = (uns long)( ((uns long)ChannelSampleFraction[channel] << 16) + (counter & 65535) );

         if ( ChannelSampleBits[channel] == 8 )
            {
             setup8Mto16S ( VolLeftTable, VolRightTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel] );
             SampleAdress = (uns long)(ChannelSampleAdress[channel] + ChannelSamplePosition[channel]);
             mix8Mto16S ( FracCounter, SampleAdress, (uns long)((MixPosition << 1) + MixerBlock) );
            }
         else
            {
             setup16Mto16S ( VolLeftTable, VolRightTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel] );
             SampleAdress = (uns long)( (ChannelSampleAdress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16Mto16S ( FracCounter, SampleAdress, (uns long)((MixPosition << 1) + MixerBlock) );
            }
        }
    }
}


