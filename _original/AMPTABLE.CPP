
#include <stdio.h>
#include <math.h>

#include "fx.h"

#include "mixer.h"

// *** some Volumetable variables ***

char *Voltable8 = NULL;
char *MasterVolumeTable = NULL;

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


void calcVol8for32 ( char *VolPointer )
{
 long Volume, Sample;

 for (Volume = 0; Volume <= 256; Volume++)
   {
    for (Sample = 0; Sample <= 127; Sample++)
      {
       *(signed long *)VolPointer = (signed long)( Sample * Volume * 256);
       VolPointer+=4;
      }
    for (Sample = -128; Sample <= -1; Sample++)
      {
       *(signed long *)VolPointer = (signed long)( Sample * Volume * 256);
       VolPointer+=4;
      }
   }
}

// ******************** Master Volume table calculations ********************

void calcMasterVol8for16 ( unsigned long MasterVolume , char *Pointer)
{
 // MasterVolume gives the amount of multiplication of every channel
 // for the end audiostream.
 // 32768 = Steigung von 0.0625 (in 4096 von 0 nach 255)
 // 16384 =
 // 8192  =
 // 2048  = Steigung von 0.00390625 (in 65536 steps von 0 nach 255)
 // S3M-Mastervolume * 256 = MasterVolume

 unsigned long Ramp_start, Ramp_end, Ramp_delta;
 unsigned long counter;
 unsigned long value;
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


void calcMasterVol16for16 ( unsigned long MasterVolume , char *Pointer)
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
     unsigned long Draw;
     unsigned long  Pos, Pos1, Pos2;
     unsigned long Val, Val1, Val2;
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


void calcMasterVolume32 ( unsigned long MasterVolume , char *Pointer)
{
 signed long counter;

 Pointer += 32768;

 if (MasterVolume < 128)  MasterVolume = 128;

 for (counter = -8192; counter < 8191; counter++)
     {
      *(unsigned long *)(Pointer+counter*4) = MasterVolume;
     }

 if (MasterVolume == 32768)  return;

 double value;
 signed long val;
 signed long test;

 value = 32768.0 / (double)MasterVolume * 32.0;
 value = value * 0.80;
 val = (signed long)value;

 for ( counter = val; counter < 8192; counter++)
     {
      if ( counter < (2*val) )
        test = (signed long)( (double)(4.0*val-counter) * ((double)MasterVolume / (double)(3.0*val)) );
      if ( test < 128 )  test = 128;
      *(unsigned long *)(Pointer+counter*4) = test;
      *(unsigned long *)(Pointer+counter*4-4) = test;
     }
}

//********************** Master Volume calculations *************************

// 16 bit mixer

void MasterVol8for16 ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol8for16 parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebp, MasterVolumeTable"\
            "shr ecx, 1"\
            "C16M8loop:"\
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
            "jnz C16M8loop";


void MasterVol16for16 ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol16for16 parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebp, MasterVolumeTable"\
            "shr ecx,1"\
            "C16M16loop:"\
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
            "jnz C16M16loop";


// 32 bit mixer hard clipping

void MasterVol8for32uh ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol8for32uh parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebx,0x00800000"\
            "mov ebp,MasterVolume"\
           "C32M8loop:"\
            "mov eax,[esi]"\
            "imul ebp"\
            "idiv ebx"\
            "add eax,0x00008000"\
            "test eax,0xffff0000"\
            "jz C32M8noOverflow"\
            "or eax,eax"\
            "jns C32M8noUnderflow"\
            "xor eax,eax"\
            "jmp C32M8noOverflow"\
           "C32M8noUnderflow:"\
            "mov eax,0x0000ffff"\
           "C32M8noOverflow:"\
            "mov [edi],ah"\
            "add edi,1"\
            "add esi,4"\
            "dec ecx"\
            "jnz C32M8loop";

void MasterVol8for32sh ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol8for32sh parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebx,0x00800000"\
            "mov ebp,MasterVolume"\
           "C32M8loop:"\
            "mov eax,[esi]"\
            "imul ebp"\
            "idiv ebx"\
            "add eax,0x00008000"\
            "test eax,0xffff0000"\
            "jz C32M8noOverflow"\
            "or eax,eax"\
            "jns C32M8noUnderflow"\
            "xor eax,eax"\
            "jmp C32M8noOverflow"\
           "C32M8noUnderflow:"\
            "mov eax,0x0000ffff"\
           "C32M8noOverflow:"\
            "sub ah,128"\
            "mov [edi],ah"\
            "add edi,1"\
            "add esi,4"\
            "dec ecx"\
            "jnz C32M8loop";

void MasterVol16for32uh ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol16for32uh parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebx,0x00800000"\
            "mov ebp,MasterVolume"\
           "C32M16loop:"\
            "mov eax,[esi]"\
            "imul ebp"\
            "idiv ebx"\
            "add eax,0x00008000"\
            "test eax,0xffff0000"\
            "jz C32M16noOverflow"\
            "or eax,eax"\
            "jns C32M16noUnderflow"\
            "xor eax,eax"\
            "jmp C32M16noOverflow"\
           "C32M16noUnderflow:"\
            "mov eax,0x0000ffff"\
           "C32M16noOverflow:"\
            "mov [edi],ax"\
            "add edi,2"\
            "add esi,4"\
            "dec ecx"\
            "jnz C32M16loop";

void MasterVol16for32sh ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol16for32sh parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebx,0x00800000"\
            "mov ebp,MasterVolume"\
           "C32M16loop:"\
            "mov eax,[esi]"\
            "imul ebp"\
            "idiv ebx"\
            "add eax,0x00008000"\
            "test eax,0xffff0000"\
            "jz C32M16noOverflow"\
            "or eax,eax"\
            "jns C32M16noUnderflow"\
            "xor eax,eax"\
            "jmp C32M16noOverflow"\
           "C32M16noUnderflow:"\
            "mov eax,0x0000ffff"\
           "C32M16noOverflow:"\
            "sub eax,32768"\
            "mov [edi],ax"\
            "add edi,2"\
            "add esi,4"\
            "dec ecx"\
            "jnz C32M16loop";


// 32 bit mixer soft clipping

void MasterVol8for32us ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol8for32us parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebx,0x00800000"\
            "mov ebp,MasterVolumeTable"\
            "add ebp, 32768"\
           "C32M8loop:"\
            "mov eax,[esi]"\
            "mov edx,eax"\
            "sar edx,18"\
            "imul dword ptr [ebp+edx*4]"\
            "idiv ebx"\
            "add eax,0x00008000"\
            "test eax,0xffff0000"\
            "jz C32M8noOverflow"\
            "or eax,eax"\
            "jns C32M8noUnderflow"\
            "xor eax,eax"\
            "jmp C32M8noOverflow"\
           "C32M8noUnderflow:"\
            "mov eax,0x0000ffff"\
           "C32M8noOverflow:"\
            "mov [edi],ah"\
            "add edi,1"\
            "add esi,4"\
            "dec ecx"\
            "jnz C32M8loop";

void MasterVol8for32ss ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol8for32ss parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebx, 0x00800000"\
            "mov ebp, MasterVolumeTable"\
            "add ebp, 32768"\
           "C32M8loop:"\
            "mov eax,[esi]"\
            "mov edx,eax"\
            "sar edx,18"\
            "imul dword ptr [ebp+edx*4]"\
            "idiv ebx"\
            "add eax,0x00008000"\
            "test eax,0xffff0000"\
            "jz C32M8noOverflow"\
            "or eax,eax"\
            "jns C32M8noUnderflow"\
            "xor eax,eax"\
            "jmp C32M8noOverflow"\
           "C32M8noUnderflow:"\
            "mov eax,0x0000ffff"\
           "C32M8noOverflow:"\
            "sub ah,128"\
            "mov [edi],ah"\
            "add edi,1"\
            "add esi,4"\
            "dec ecx"\
            "jnz C32M8loop";

void MasterVol16for32us ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol16for32us parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebx, 0x00800000"\
            "mov ebp, MasterVolumeTable"\
            "add ebp, 32768"\
           "C32M16loop:"\
            "mov eax,[esi]"\
            "mov edx,eax"\
            "sar edx,18"\
            "imul dword ptr [ebp+edx*4]"\
            "idiv ebx"\
            "add eax,0x00008000"\
            "test eax,0xffff0000"\
            "jz C32M16noOverflow"\
            "or eax,eax"\
            "jns C32M16noUnderflow"\
            "xor eax,eax"\
            "jmp C32M16noOverflow"\
           "C32M16noUnderflow:"\
            "mov eax,0x0000ffff"\
           "C32M16noOverflow:"\
            "mov [edi],ax"\
            "add edi,2"\
            "add esi,4"\
            "dec ecx"\
            "jnz C32M16loop";

void MasterVol16for32ss ( char *Source, char *Destin, unsigned long length);
#pragma aux MasterVol16for32ss parm [esi] [edi] [ecx] modify exact [eax ebx ecx edx esi edi ebp] = \
            "mov ebx, 0x00800000"\
            "mov ebp, MasterVolumeTable"\
            "add ebp, 32768"\
           "C32M16loop:"\
            "mov eax,[esi]"\
            "mov edx,eax"\
            "sar edx,18"\
            "imul dword ptr [ebp+edx*4]"\
            "idiv ebx"\
            "add eax,0x00008000"\
            "test eax,0xffff0000"\
            "jz C32M16noOverflow"\
            "or eax,eax"\
            "jns C32M16noUnderflow"\
            "xor eax,eax"\
            "jmp C32M16noOverflow"\
           "C32M16noUnderflow:"\
            "mov eax,0x0000ffff"\
           "C32M16noOverflow:"\
            "sub eax,32768"\
            "mov [edi],ax"\
            "add edi,2"\
            "add esi,4"\
            "dec ecx"\
            "jnz C32M16loop";


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
                  if (flag_softclip == 0)
                     {
                      if (flag_16bit == 0)
                         if (flag_signed == 0)
                             MasterVol8for32uh ( Source, Destin, length << flag_stereo );
                          else
                              MasterVol8for32sh ( Source, Destin, length << flag_stereo );
                      else
                          if (flag_signed == 0)
                              MasterVol16for32uh ( Source, Destin, length << flag_stereo );
                          else
                              MasterVol16for32sh ( Source, Destin, length << flag_stereo );
                     }
                  else
                     {
                      if (flag_16bit == 0)
                         if (flag_signed == 0)
                             MasterVol8for32us ( Source, Destin, length << flag_stereo );
                          else
                              MasterVol8for32ss ( Source, Destin, length << flag_stereo );
                      else
                          if (flag_signed == 0)
                              MasterVol16for32us ( Source, Destin, length << flag_stereo );
                          else
                              MasterVol16for32ss ( Source, Destin, length << flag_stereo );
                     }
                  break;
         }
}

