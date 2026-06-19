
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
//#include "dat_wav.h"
//#include "dat_s3m.h"

#include "mixer.h"
//#include "convert.h"

// *** global mixer variables ***

unsigned short ChannelSeperation = 256;    // seperation of channels
                                           // 256 = max, 0 = mono
unsigned short GlobalVolume = 256;         // global volume from 0 to 256
unsigned short MasterVolume = 0;           // master volume, is realtive
                                           // volume of every channel to mix

// *** global mixerchannel variables ***

unsigned long  ChannelUsed = 0, ChannelLast = 0;

unsigned char  ChannelActiv[256];          // 0 = quiet, 1 = activ
unsigned char  ChannelFlag[256];           // 0 = available, 1 = paused

unsigned short ChannelSampleNr[256];       // Nr. of Sample playing (not needed really)
unsigned short ChannelVolume[256];         // from 0 (quiet) to 256 (loud)
unsigned short ChannelPanning[256];        // from 0 (left) to 256 (right), 128 is middle, +32768 = surround
unsigned char  ChannelSampleBits[256];     // 8 or 16 bit
unsigned char  ChannelSampleMode[256];     // 1 = mono, 2 = stereo sample
unsigned long  ChannelSampleAddress[256];  // start address of current sample INCLUSIVE
unsigned long  ChannelSampleLength[256];   // position length of sample EXCLUSIVE
unsigned char  ChannelLoopMode[256];       // 0 = no, 1 = forward, 2 = reverse, 3 = pingpong
unsigned long  ChannelLoopBegin[256];      // position where loop starts INCLUSIVE
unsigned long  ChannelLoopEnd[256];        // position where loop ends EXCLUSIVE
unsigned long  ChannelSamplePosition[256]; // relative offset in sample
unsigned long  ChannelSampleFraction[256]; // fraction of sample position
unsigned long  ChannelSampleFrequence[256];// frequence to play sample

unsigned short ChannelVolumeLeft[256];     // from 0 to 256
unsigned short ChannelVolumeRight[256];    // from 0 to 256
  signed long  DeltaSamplePosition[256];   // delta to add on position
  signed long  DeltaSampleFraction[256];   // delta to add on fraction

unsigned long  NextSamplePosition[256];    // sample position after mixing
unsigned long  NextSampleFraction[256];    // sample fraction after mixing

unsigned char  NextChannelActiv[256];      // activ-flag after mixing

// NOTE: ALL positions given here are NOT bytes but they ARE in samples !!!



// *** some Volumetable variables ***

static char *Voltable8 = NULL;
static char *MasterVolumeTable = NULL;



// *** Main programm ***

void writeMixer( )
{
  switch (MixerType)
         {
          case 1:
                  printf("Running 16bit Mixer with");
                  break;
          case 2:
                  printf("Running 32bit Mixer with");
                  break;
         }

  if ( flag_interpolate == 0 )    printf(" no");
  printf(" interpolation and");
  if ( flag_softclip == 0 )
       printf(" hard");
  else
       printf(" soft");
  printf(" clipping\n\n");
}


void initDMAbuffer ( unsigned long length )
{
  Mixerpointer1 = DMApointer;
  Mixerpointer2 = (char *)( DMApointer + length );
}


void initMixer ( unsigned short MixerType )
{
  flag_MixerReady = 0;

  Mixer_length = block_length << flag_stereo;
  if ( MixerType == 1 )  Mixer_length <<= 1;
  if ( MixerType == 2 )  Mixer_length <<= 2;

  MixerBlock = (char *)malloc ( Mixer_length );

  switch (MixerType)
  {
   case 1:
           Voltable8 = (char *)malloc ( 258*256*2 );  // = 256 kb
           calcVol8for16 ( Voltable8 );

           MasterVolumeTable = (char *)malloc ( 132000 ); // = 128 KB
           break;
   case 2:
           Voltable8 = (char *)malloc ( 258*256*4 );  // = 256 kb
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

 flag_MixerReady = 1;
}

void resetMixer ()
{
 static unsigned long channel;

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



// ************************* table calculations *****************************

void calcVol8for16 ( char *VolPointer )
{
 long Volume, Sample;

 for (Volume = 0; Volume <= 256; Volume++)
   {
    for (Sample = 0; Sample <= 127; Sample++)
      {
       *(signed short *)VolPointer = (signed short)( ( Sample * Volume + 8 ) / 16 );
       VolPointer+=2;
      }
    for (Sample = -128; Sample <= -1; Sample++)
      {
       *(signed short *)VolPointer = (signed short)( ( Sample * Volume - 8 ) / 16 );
       VolPointer+=2;
      }
   }
}


void calcMasterVol8for16 ( unsigned short MasterVolume , char *Pointer)
{
 // MasterVolume gives the amount of multiplication of every channel
 // for the end audiostream.
 // 32768 = Steigung von 1 (in 65536 von 0 nach 255)
 // 16384 =
 // 8192  =
 // 2048  = Steigung von 0.065 (in 4096 steps von 0 nach 255)
 // S3M-Mastervolume * 256 = MasterVolume

 unsigned long Ramp_start, Ramp_end, Ramp_delta;
 unsigned long counter;
 unsigned short value;
 unsigned char *MasterTable;

 MasterTable = (unsigned char *)Pointer;

 if (MasterVolume < 2048 )  MasterVolume = 2048;

 Ramp_delta = (2 * (32768*4086) / MasterVolume + 1) >> 1;
 Ramp_start = (65536 - Ramp_delta + 1) >> 1;
 Ramp_end   = Ramp_start + Ramp_delta;

 for (counter = 0; counter < 65536; counter++)
     {
      if (counter < Ramp_start)
          value = 0;
      if ( (Ramp_start <= counter) && (counter < Ramp_end) )
          value = ( (((counter - Ramp_start) * (unsigned long)MasterVolume) + 262144L) >> 19);
      if (counter >= Ramp_end)
          value = 255;

      *(MasterTable+counter) = value - (flag_signed?128:0);
     }

 if ( flag_softclip != 0 )
    {
     unsigned short Draw;
     unsigned long  Pos, Pos1, Pos2;
     unsigned short Val, Val1, Val2;
     double y1,y2;
     unsigned long x,counter;
     double a,b,c,d;

     Draw = (MasterVolume / 2 + 2000) / 256;

     counter = 0;
     while ( *(unsigned char *)(MasterTable+counter) < Draw )
           {
            counter++;
           }

     Pos = counter;
     Val = *(unsigned char *)(MasterTable+Pos) << 8;
     Pos1 = (unsigned long) ( (double)Pos*0.90 );
     Val1 = (unsigned short) ( (double)Val*0.30 );
     Pos2 = Pos - Pos1;
     Val2 = Val - Val1;
     y1 = (double)Val / (double)Pos;
     y2 = (double)MasterVolume / 2048.0;

     a = ((y1*(double)Pos1) - (2.0*(double)Val1)) / ( (double)Pos1*(double)Pos1*(double)Pos1 );
     b = ( y1 - (3.0*a*(double)Pos1*(double)Pos1) ) / (2.0*(double)Pos1);

     for ( x=0; x < Pos1; x++ )
         {
          value = (unsigned short) ( a*(double)x*(double)x*(double)x + b*(double)x*(double)x + 0.5 );
          *(MasterTable+x) = (value >> 8) - (flag_signed?128:0);
          *(MasterTable+65535-x)= ((65535 - value) >> 8) - (flag_signed?128:0);
         }

     d = (double)Val1;
     c = y1;
     a = (y2+c) / ((double)Pos2*(double)Pos2) - (2.0*(c+(double)Val2)) / ((double)Pos2*(double)Pos2*(double)Pos2);
     b = (y2 - (3.0*a*(double)Pos2*(double)Pos2) - c) / (2.0*(double)Pos2);

     for ( x=0; x < Pos2; x++ )
         {
          value = (unsigned short) ( a*(double)x*(double)x*(double)x + b*(double)x*(double)x + c*(double)x + d + 0.5 );
          *(MasterTable+Pos1+x) = (value >> 8) - (flag_signed?128:0);
          *(MasterTable+65535-x-Pos1) = ((65535 - value) >> 8) - (flag_signed?128:0);
         }
    }
}


void calcMasterVol16for16 ( unsigned short MasterVolume , char *Pointer)
{
 // MasterVolume gives the amount of multiplication of every channel
 // for the end audiostream.
 // see above...

 unsigned long Ramp_start, Ramp_end, Ramp_delta;
 unsigned long counter;
 unsigned long value;
 unsigned short *MasterTable;

 MasterTable = (unsigned short *)Pointer;

 if (MasterVolume < 2048 )  MasterVolume = 2048;

 Ramp_delta = (2 * (32768*4096) / MasterVolume + 1) >> 1;
 Ramp_start = (65536 - Ramp_delta + 1) >> 1;
 Ramp_end   = Ramp_start + Ramp_delta;

 for (counter = 0; counter < 65536; counter++)
     {
      if (counter < Ramp_start)
          value = 0;
      if ( (Ramp_start <= counter) && (counter < Ramp_end) )
          value = (unsigned short)( ((counter - Ramp_start) * (unsigned long)MasterVolume + 1024) >> 11);
      if (counter >= Ramp_end)
          value = 65535;

      *(MasterTable+counter) = value - (flag_signed?32768:0);
     }

 if ( flag_softclip != 0 )
    {
     unsigned short Draw;
     unsigned long  Pos, Pos1, Pos2;
     unsigned short Val, Val1, Val2;
     double y1,y2;
     unsigned long x,counter;
     double a,b,c,d;

     Draw = MasterVolume / 2 + 2000;

     counter = 0;
     while ( *(MasterTable+counter) < Draw )
           {
            counter++;
           }

     Pos = counter;
     Val = *(MasterTable+Pos);
     Pos1 = (unsigned long) ( (double)Pos*0.90 );
     Val1 = (unsigned short) ( (double)Val*0.30 );
     Pos2 = Pos - Pos1;
     Val2 = Val - Val1;
     y1 = (double)Val / (double)Pos;
     y2 = (double)MasterVolume / 2048.0;

     a = ((y1*(double)Pos1) - (2.0*(double)Val1)) / ( (double)Pos1*(double)Pos1*(double)Pos1 );
     b = ( y1 - (3.0*a*(double)Pos1*(double)Pos1) ) / (2.0*(double)Pos1);

     for ( x=0; x < Pos1; x++ )
         {
          value = (unsigned short) ( a*(double)x*(double)x*(double)x + b*(double)x*(double)x + 0.5 ) - (flag_signed?32768:0);
          *(MasterTable+x) = value;
          *(MasterTable+65535-x)= 65535 - value;
         }

     d = (double)Val1;
     c = y1;
     a = (y2+c) / ((double)Pos2*(double)Pos2) - (2.0*(c+(double)Val2)) / ((double)Pos2*(double)Pos2*(double)Pos2);
     b = (y2 - (3.0*a*(double)Pos2*(double)Pos2) - c) / (2.0*(double)Pos2);

     for ( x=0; x < Pos2; x++ )
         {
          value = (unsigned short) ( a*(double)x*(double)x*(double)x + b*(double)x*(double)x + c*(double)x + d + 0.5 ) - (flag_signed?32768:0);
          *(MasterTable+Pos1+x) = value;
          *(MasterTable+65535-x-Pos1) = 65535 - value;
         }

    }
}


void MasterVol8for16 ( char *Source, char *Destin, unsigned long length);
/*
{
 // see above...

 static unsigned long counter;

 for (counter = 0; counter < length; counter++)
     {
      *(unsigned char *)Destin= *(unsigned char *)(MasterVolumeTable + (unsigned long)(*(unsigned short *)Source) );

      Destin++;
      Source+=2;
     }
}
*/
#pragma aux MasterVol8for16 parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebp, MasterVolumeTable"\
            "shr ecx, 1"\
            "M8loop:"\
            "mov eax, [esi]"\
            "mov ebx, [esi+2]"\
            "and eax, 65535"\
            "and ebx, 65535"\
            "mov al, [ebp+eax]"\
            "mov bl, [ebp+ebx]"\
            "mov [edi], al"\
            "mov [edi+1], bl"\
            "add edi,2"\
            "add esi,4"\
            "dec ecx"\
            "jnz M8loop";

void MasterVol16for16 ( char *Source, char *Destin, unsigned long length);
/*
{
 // see above...

 static unsigned long counter;

 for (counter = 0; counter < length; counter++)
     {
      *(unsigned short *)Destin= *(unsigned short *)(MasterVolumeTable + ((unsigned long)(*(unsigned short *)Source) << 1) );

      Destin+=2;
      Source+=2;
     }
}
*/
#pragma aux MasterVol16for16 parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebp, MasterVolumeTable"\
            "shr ecx,1"\
            "M16loop:"\
            "mov eax, [esi]"\
            "mov ebx, [esi+2]"\
            "and eax, 65535"\
            "and ebx, 65535"\
            "mov eax, [ebp+eax*2]"\
            "mov ebx, [ebp+ebx*2]"\
            "mov [edi], eax"\
            "mov [edi+2], ebx"\
            "add edi,4"\
            "add esi,4"\
            "dec ecx"\
            "jnz M16loop";


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
 static unsigned long scale;

 scale = divide_64bit (ChannelSampleFrequence[channel] << 1,0,0,MixSpeed);

return ((scale + 1) >> 1);
}


void PrepareMixing()
{
static unsigned long channel;
static unsigned long scaling;

static signed long Pan;

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
 static unsigned long channel;

 static   signed long DeltaPosition, DeltaFraction;
 static unsigned long Position, Fraction;
 static unsigned long EndPosition, EndFraction;
 static unsigned long LengthPosition, LengthFraction;

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

          if ( Position >= ChannelSampleLength[channel] )
             {
              ChannelSamplePosition[channel] = NextSamplePosition[channel] = 0;
              ChannelSampleFraction[channel] = NextSampleFraction[channel] = 0;
              ChannelActiv[channel] = NextChannelActiv[channel] = 0;
             }
          else

          switch ( ChannelLoopMode[channel] )
                 {
                  case 0: // no loop

                          EndFraction = Fraction + (DeltaFraction * MixLength);
                          EndPosition = Position + (DeltaPosition * MixLength) + (EndFraction >> 16);
                          EndFraction = EndFraction & 65535;

                          if ( EndPosition < ChannelSampleLength[channel] )
                             {
                              NextSamplePosition[channel] = EndPosition;
                              NextSampleFraction[channel] = EndFraction;

                              NextChannelActiv[channel] = 1;
                             }

                          else
                             {
                              NextSamplePosition[channel] = 0;
                              NextSampleFraction[channel] = 0;

                              NextChannelActiv[channel] = 0;
                             }
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
            "mov ecx, Mixer_length"\
            "mov edi, MixerBlock"\
            "mov eax, 0x80008000"\
            "shr ecx,2"\
            "rep stosd";


void clearMix32 ();
#pragma aux clearMix32 modify exact [eax ecx edi] = \
            "mov ecx, Mixer_length"\
            "mov edi, MixerBlock"\
            "mov eax, 0x80000000"\
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


void Do16Mixing ( unsigned long MixLength )
{
 static unsigned long channel;

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
                              LengthFraction = (65536 - Fraction) & 65535;

                              if ( LengthFraction != 0 )
                                   LengthPosition--;

                              counter = 1 + divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );

                              if ( counter > MixLength )
                                 {
                                  counter = MixLength;
                                  Mix16Channel ( channel, counter, MixerAddress );
                                 }
                              else
                                  Mix16Channel ( channel, counter, MixerAddress );

                              break;

                      case 1: // loop forward

                              LoopBegin = ChannelLoopBegin[channel];
                              LoopEnd = ChannelLoopEnd[channel];

                              MixLengthRest = MixLength;
                              MixPosition = MixerAddress;

                              while ( MixLengthRest != 0 )
                                    {
                                     LengthPosition = LoopEnd - Position;
                                     LengthFraction = (65536 - Fraction) & 65535;

                                     if ( LengthFraction != 0 )
                                          LengthPosition--;

                                     counter = 1 + divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );

                                     if ( counter > MixLengthRest )
                                        {
                                         counter = MixLengthRest;
                                         MixLengthRest = 0;
                                         Mix16Channel ( channel, counter, MixPosition );
                                         MixPosition += counter;
                                        }
                                     else
                                        {
                                         MixLengthRest -= counter;
                                         Mix16Channel ( channel, counter, MixPosition );
                                         MixPosition += counter;
                                         Fraction = Fraction + (DeltaFraction * counter);
                                         Position = Position + (DeltaPosition * counter) + (Fraction >> 16);
                                         Fraction = Fraction & 65535;
                                         ChannelSamplePosition[channel] = Position = Position - LoopEnd + LoopBegin;
                                         ChannelSampleFraction[channel] = Fraction;
                                        }
                                     //if ( Position >= LoopEnd)
                                     //   printf("Fault no interpolation!\n");
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
                              LengthFraction = (65536 - Fraction) & 65535;

                              if ( LengthFraction != 0 )
                                   LengthPosition--;

                              counter = 1 + divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );

                              if ( (LengthPosition > 1) || ( (LengthPosition == 1) && (LengthFraction != 0) ) )
                                   counter1 = divide_64bit( LengthPosition-1, LengthFraction, DeltaPosition, DeltaFraction );
                              else
                                   counter1 = 0;

                              counter2 = counter - counter1;

                              if ( counter1 != 0 )  // if no illegal value
                              if ( counter1 > MixLengthRest )
                                 {
                                  counter = MixLengthRest;
                                  MixLengthRest = 0;
                                  Mix16IChannel ( channel, counter, MixPosition );
                                  MixPosition += counter;
                                 }
                              else
                                 {
                                  counter = counter1;
                                  MixLengthRest -= counter;
                                  Mix16IChannel ( channel, counter, MixPosition );
                                  MixPosition += counter;
                                  Fraction = Fraction + (DeltaFraction * counter);
                                  Position = Position + (DeltaPosition * counter) + (Fraction >> 16);
                                  Fraction = Fraction & 65535;
                                  ChannelSamplePosition[channel] = Position;
                                  ChannelSampleFraction[channel] = Fraction;
                                 }

                              if ( MixLengthRest == 0 )  break;

                              if ( counter2 != 0 )  // if no illegal value
                              if ( counter2 >= MixLengthRest )
                                 {
                                  counter = MixLengthRest;
                                  MixLengthRest = 0;
                                  while ( counter != 0 )
                                        {
                                         EndPosition = Position + 1;
                                         if ( EndPosition >= Length )  EndPosition = Length-1;
                                         Mix16ICarry ( channel, Position, EndPosition, Fraction, MixPosition );
                                         Fraction = Fraction + DeltaFraction;
                                         Position = Position + DeltaPosition + (Fraction >> 16);
                                         Fraction = Fraction & 65535;
                                         MixPosition++;
                                         counter--;
                                        }
                                 }
                              else
                                 {
                                  counter = counter2;
                                  MixLengthRest -= counter;
                                  while ( counter != 0 )
                                        {
                                         EndPosition = Position + 1;
                                         if ( EndPosition >= Length )  EndPosition = Length-1;
                                         Mix16ICarry ( channel, Position, EndPosition, Fraction, MixPosition );
                                         Fraction = Fraction + DeltaFraction;
                                         Position = Position + DeltaPosition + (Fraction >> 16);
                                         Fraction = Fraction & 65535;
                                         MixPosition++;
                                         counter--;
                                        }
                                  //ChannelSamplePosition[channel] = Position;
                                  //ChannelSampleFraction[channel] = Fraction;
                                 }
                              break;

                      case 1: // loop forward

                              LoopBegin = ChannelLoopBegin[channel];
                              LoopEnd = ChannelLoopEnd[channel];

                              MixLengthRest = MixLength;
                              MixPosition = MixerAddress;

                              while ( MixLengthRest != 0 )
                                    {
                                     LengthPosition = LoopEnd - Position;
                                     LengthFraction = (65536 - Fraction) & 65535;

                                     if ( LengthFraction != 0 )
                                          LengthPosition--;

                                     counter = 1 + divide_64bit( LengthPosition, LengthFraction, DeltaPosition, DeltaFraction );

                                     if ( (LengthPosition > 1) || ( (LengthPosition == 1) && (LengthFraction != 0) ) )
                                          counter1 = divide_64bit( LengthPosition-1, LengthFraction, DeltaPosition, DeltaFraction );
                                     else
                                          counter1 = 0;

                                     counter2 = counter - counter1;

                                     //if ( counter2 == 0 )
                                     //     printf("GOTCHA2 %lu\n",DeltaPosition);

                                     if ( counter1 != 0 )  // if no illegal value
                                     if ( counter1 > MixLengthRest )
                                        {
                                         counter = MixLengthRest;
                                         MixLengthRest = 0;
                                         Mix16IChannel ( channel, counter, MixPosition );
                                         MixPosition += counter;
                                        }
                                     else
                                        {
                                         counter = counter1;
                                         MixLengthRest -= counter;
                                         Mix16IChannel ( channel, counter, MixPosition );
                                         MixPosition += counter;
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
                                         MixLengthRest = 0;
                                         while ( counter != 0 )
                                               {
                                                EndPosition = Position + 1;
                                                if ( EndPosition >= LoopEnd )  EndPosition = EndPosition - LoopEnd + LoopBegin;
                                                Mix16ICarry ( channel, Position, EndPosition, Fraction, MixPosition );
                                                Fraction = Fraction + DeltaFraction;
                                                Position = Position + DeltaPosition + (Fraction >> 16);
                                                Fraction = Fraction & 65535;
                                                MixPosition++;
                                                counter--;
                                               }
                                        }
                                     else
                                        {
                                         counter = counter2;
                                         MixLengthRest -= counter;
                                         while ( counter != 0 )
                                               {
                                                EndPosition = Position + 1;
                                                if ( EndPosition >= LoopEnd )  EndPosition = EndPosition - LoopEnd + LoopBegin;
                                                Mix16ICarry ( channel, Position, EndPosition, Fraction, MixPosition );
                                                Fraction = Fraction + DeltaFraction;
                                                Position = Position + DeltaPosition + (Fraction >> 16);
                                                Fraction = Fraction & 65535;
                                                MixPosition++;
                                                counter--;
                                               }
                                         if ( Position >= LoopEnd )  Position = Position - LoopEnd + LoopBegin;
                                         ChannelSamplePosition[channel] = Position;
                                         ChannelSampleFraction[channel] = Fraction;
                                        }

                                     //if ( Position >= LoopEnd)
                                     //   printf("Fault2!\n");

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
         BlockAddress = (uns long)(MixerBlock+(MixPosition << 1)+(counter << 1));

         FracCounter = (uns long)ChannelSampleFraction[channel] << 16;

         if ( ChannelSampleBits[channel] == 8 )
            {
             VolTable = (uns long)( Voltable8 + ((uns long)Volume << 9 ) );
             setup8Mto16M ( VolTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (uns long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8Mto16M ( FracCounter, SampleAddress, -(counter << 1) );
            }
         else    // 16bit sample
            {
             setup16Mto16M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (uns long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16Mto16M ( FracCounter, SampleAddress, -(counter << 1) );
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (uns long)(MixerBlock+(MixPosition << 2)+(counter << 2) );

         FracCounter = (uns long)ChannelSampleFraction[channel] << 16;

         if ( ChannelSampleBits[channel] == 8 )
            {
             VolLeftTable = (uns long)( Voltable8 + ((uns long)VolumeLeft << 9 ) );
             VolRightTable = (uns long)( Voltable8 + ((uns long)VolumeRight << 9 ) );
             setup8Mto16S ( VolLeftTable, VolRightTable, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (uns long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8Mto16S ( FracCounter, SampleAddress, -(counter << 2) );
            }
         else   // 16bit sample
            {
             setup16Mto16S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (uns long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16Mto16S ( FracCounter, SampleAddress, -(counter << 2) );
            }
        }  // end stereo
    }  // end if silence
}


static void Mix16IChannel ( unsigned long channel, unsigned long counter, unsigned long MixPosition )
{
 static uns long Volume, VolumeLeft, VolumeRight;
 static uns long VolTable;
 static uns long FracCounter, SampleAddress;
 static uns long BlockAddress;

 if ( counter == 0 )  return;

 Volume = ( ChannelVolume[channel] * GlobalVolume + 128 ) >> 8;

 if ( Volume != 0 )
    {
     if ( flag_stereo == 0 )    // mono sample
        {
         BlockAddress = (uns long)(MixerBlock+(MixPosition << 1)+(counter << 1));

         FracCounter = (uns long)ChannelSampleFraction[channel] << 16;

         if ( ChannelSampleBits[channel] == 8 )
            {
             setup8MIto16M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (uns long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8MIto16M ( FracCounter, SampleAddress, -(counter << 1) );
            }
         else    // 16bit sample
            {
             setup16MIto16M ( Volume, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (uns long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
             mix16MIto16M ( FracCounter, SampleAddress, -(counter << 1) );
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (uns long)(MixerBlock+(MixPosition << 2)+(counter << 2) );

         FracCounter = (uns long)ChannelSampleFraction[channel] << 16;

         if ( ChannelSampleBits[channel] == 8 )  // 8bit sample
            {
             setup8MIto16S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (uns long)(ChannelSampleAddress[channel] + ChannelSamplePosition[channel]);
             mix8MIto16S ( FracCounter, SampleAddress, -(counter << 2) );
            }
         else   // 16bit sample
            {
             setup16MIto16S ( VolumeLeft, VolumeRight, DeltaSamplePosition[channel], DeltaSampleFraction[channel], BlockAddress );
             SampleAddress = (uns long)( (ChannelSampleAddress[channel] >> 1) + ChannelSamplePosition[channel]);
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
             Sample1 = (signed long)*(signed char *)(SampleAddress1);
             Sample2 = (signed long)*(signed char *)(SampleAddress2);
             IntSample = (Sample2 - Sample1) * (signed long)Fraction + (Sample1 << 16);

             *(signed short *)(BlockAddress) += (signed short)( IntSample * (signed long)Volume >> 20);
            }
         else    // 16bit sample
            {
             SampleAddress1 += (Position1 << 1);
             SampleAddress2 += (Position2 << 1);
             Sample1 = (signed long)*(signed short *)(SampleAddress1);
             Sample2 = (signed long)*(signed short *)(SampleAddress2);
             IntSample = (((Sample2 - Sample1) * (signed long)Fraction) / 256) + (Sample1 << 8);

             *(signed short *)(BlockAddress) += (signed short)( IntSample * (signed long)Volume >> 20);
            }
        }
     else   // stereo
        {
         VolumeLeft = ( ChannelVolumeLeft[channel] * GlobalVolume + 128 ) >> 8;
         VolumeRight = ( ChannelVolumeRight[channel] * GlobalVolume + 128 ) >> 8;

         BlockAddress = (uns long)( MixerBlock + (MixPosition << 2) );

         if ( ChannelSampleBits[channel] == 8 )  // 8bit sample
            {
             SampleAddress1 += Position1;
             SampleAddress2 += Position2;

             Sample1 = (signed long)*(signed char *)(SampleAddress1);
             Sample2 = (signed long)*(signed char *)(SampleAddress2);
             IntSample = (Sample2 - Sample1) * (signed long)Fraction + (Sample1 << 16);

             *(signed short *)(BlockAddress) += (signed short)( IntSample * (signed long)VolumeLeft >> 20);
             *(signed short *)(BlockAddress+2) += (signed short)( IntSample * (signed long)VolumeRight >> 20);
            }
         else   // 16bit sample
            {
             SampleAddress1 += (Position1 << 1);
             SampleAddress2 += (Position2 << 1);

             Sample1 = (signed long)*(signed short *)(SampleAddress1);
             Sample2 = (signed long)*(signed short *)(SampleAddress2);
             IntSample = (((Sample2 - Sample1) * (signed long)Fraction) / 256) + (Sample1 << 8);

             *(signed short *)(BlockAddress) += (signed short)( IntSample * (signed long)VolumeLeft >> 20);
             *(signed short *)(BlockAddress+2) += (signed short)( IntSample * (signed long)VolumeRight >> 20);
            }
        } // end stereo
    }  // end if silence
}
