// WAV - Routines for F/X Player v1.0
// (c) 1997-98 by Apollo / STIGMA

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <conio.h>
#include <i86.h>

#include "fx.h"

//#include "irq.h"
//#include "dma.h"

//#include "device.h"
//#include "dev_sb.h"

//#include "format.h"
#include "dat_calc.h"
#include "dat_wav.h"
//#include "dat_s3m.h"

#include "convert.h"
#include "mixer.h"
#include "amptable.h"


static void quickload(char *name, unsigned long address, unsigned long length)
{
 REGS r;

 r.w.ax=0x3d90;
 r.x.edx=(unsigned long)name;
 int386(0x21, &r, &r);
 r.x.ebx = r.x.eax;

 r.h.ah=0x3f;
 r.x.edx=address;
 r.x.ecx=length;
 int386(0x21, &r, &r);

 printf("%d\n",r.x.eax);
}

// Main programm

FILE *WAV_handle = NULL;

static char *WAV_Buffer = NULL;
static char *WAV_pointer = NULL;

static unsigned long WAV_16bit = 0, WAV_stereo = 0;
static unsigned long WAV_start = 0, WAV_length = 0, WAV_rate = 0;
static unsigned long WAV_playlength = 0, WAV_played = 0;

static unsigned long WAV_Position = 0, WAV_Fraction = 0;
static unsigned long WAV_PosAdd = 0, WAV_FracAdd = 0;

static unsigned long WAV_buffer_rest = 0;

struct Riff_header
 {
  char Riff[4];                     // Offset 0, length 4
  unsigned long File_length;        // Offset 4, length 4
 } R_header;

struct Chunk_header
 {
  char Wave[4];                     // Offset 8, length 4
  char fmt[4];                      // Offset 12, length 4
  unsigned long SubChunk_length;    // Offset 16, length 4
  unsigned short FormatTag;         // Offset 20, length 2
  unsigned short Channels;          // Offset 22, length 2
  unsigned long Samplerate;         // Offset 24, length 4
  unsigned long BytesPerSec;        // Offset 28, length 4
  unsigned short BytesPerSample;    // Offset 32, length 2
  unsigned short BitsPerSample;     // Offset 34, length 2
 } C_header;

struct Data_chunk
 {
  char data[4];                     // Offset 36/38, length 4
  unsigned long Data_length;        // Offset 40/42, length 4
 } D_chunk;


// ******************* WAV Functions *********************

long open_file_WAV ( char *FileName )
{
 if ( (WAV_handle=fopen(FileName,"rb") )==NULL )
      {
       return 1;
      }

return 0;
}

long close_file_WAV (void)
{
 if (WAV_handle == NULL)
    return 1;

 fclose ( WAV_handle );

 WAV_handle = NULL;

return 0;
}


long load_WAV( char *FileName )
{
 static long test;

 test = open_file_WAV ( FileName );
 if (test != 0) return 1;

 // load RIFF header

 if ( (fread( &R_header , 1, 8, WAV_handle ))!= sizeof(R_header) )
     return 3;

    if (memcmp(R_header.Riff, "RIFF", 4))
        return 3;

 // load chunk header

 if ( (fread( &C_header , 1, sizeof(C_header), WAV_handle ))!= sizeof(C_header) )
     return 3;

    if (memcmp(C_header.Wave, "WAVE", 4))
        return 3;
    if (memcmp(C_header.fmt, "fmt", 3))
        return 3;

 // load data chunk

 if ( (fread( &D_chunk , 1, sizeof(D_chunk), WAV_handle ))!= sizeof(D_chunk) )
     return 3;

    if (memcmp(D_chunk.data, "data", 4))
        {
         char *datapointer;
         char tempBuffer[1024];
         int i;

         fseek ( WAV_handle , sizeof(R_header)+sizeof(C_header), SEEK_SET );
         fread ( tempBuffer, 1, 512, WAV_handle );

         datapointer=(char *)memchr( tempBuffer,'d', 512 );
         i = datapointer-(char *)tempBuffer;

         if (memcmp(datapointer, "data" , 4))
            return 3;
         fseek ( WAV_handle, i+sizeof(R_header)+sizeof(C_header), SEEK_SET );
         fread( &D_chunk , 1, sizeof(D_chunk), WAV_handle );
        }

 WAV_start = ftell ( WAV_handle );

 WAV_stereo = (C_header.Channels) - 1;
 WAV_16bit =  C_header.BitsPerSample >> 4;

 fseek ( WAV_handle, 0L, SEEK_END );             // go to end of WAV
 WAV_length = ftell ( WAV_handle ) - WAV_start;  // read file pointer eg. length of WAV
 fseek ( WAV_handle, WAV_start, SEEK_SET );      // and go back to start

 if ( D_chunk.Data_length < WAV_length )   WAV_length = D_chunk.Data_length;

 WAV_rate = C_header.Samplerate;

 printf("Loading %s\n",FileName);
 printf("Rate: %u  Length: %ld  stereo: %s  16bit: %s\n",WAV_rate, WAV_length, WAV_stereo ? "yes" : "no", WAV_16bit ? "yes" : "no");

 WAV_playlength = WAV_length >> (WAV_stereo+WAV_16bit);
 WAV_Position = WAV_Fraction = 0;

 test = divide_64bit (WAV_rate,0,0,MixSpeed);
 WAV_PosAdd = test >> 16;
 WAV_FracAdd = test & 65535;

 MasterVolume = 32768;

 // get memory to load WAV *completly* into memory

 WAV_Buffer = (char *)malloc ( WAV_length + 0x10 );
 if ( WAV_Buffer == NULL )
    {
       close_file_WAV ();
       return 2;
    }

 // load WAV complete

 WAV_pointer = WAV_Buffer;  // set memory pointer to begin of data block

 fread( WAV_Buffer, 1, WAV_length, WAV_handle );
// quickload( FileName, (unsigned long)WAV_Buffer, WAV_length);

 flag_DatReady = 1;

return 0;
}

long close_WAV (void)
{
 close_file_WAV ();

 if ( WAV_Buffer == NULL )  return 1;
 free ( WAV_Buffer );
 WAV_Buffer = NULL;

return 0;
}


void WAV_MixI16m( char *Position1, char *Position2, unsigned long Frac, unsigned long BlockAddress)
{
 static signed long Value, Value1, Value2;

 if ( WAV_stereo == 0 )
    {
     if ( WAV_16bit == 0 )
        {
         Value = (long)(*(char *)(Position1)-128);
         Value1 = Value2 = (Value * 65536) + ( ( (long)(*(char *)(Position2)-128) - Value) * Frac);
         Value1 = Value1 / 4096;
        }
     else
        {
         Value = (long)(*(short *)(Position1));
         Value1 = Value2 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2)) - Value ) * (__int64)Frac) / 256 );
         Value1 = Value1 / 4096;
        }
    }
 else
    {
     if ( WAV_16bit == 0 )
        {
         Value = (long)(*(char *)(Position1)-128);
         Value1 = (Value * 65536) + ( ( (long)(*(char *)(Position2)-128) - Value) * Frac);
         Value = (long)(*(char *)(Position1+1)-128);
         Value2 = (Value * 65536) + ( ( (long)(*(char *)(Position2+1)-128) - Value) * Frac);
         Value1 = ( Value1 / 4096 + Value2 / 4096 ) / 2;
        }
     else
        {
         Value = (long)(*(short *)(Position1));
         Value1 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2)) - Value ) * (__int64)Frac) / 256 );
         Value = (long)(*(short *)(Position1+2));
         Value2 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2+2)) - Value ) * (__int64)Frac) / 256 );
         Value1 = ( Value1 / 4096 + Value2 / 4096 ) / 2;
        }
    }

 (*(short *)(BlockAddress+0)) += (short)Value1;
}


void WAV_MixI16s( char *Position1, char *Position2, unsigned long Frac, unsigned long BlockAddress)
{
 static signed long Value, Value1, Value2;

 if ( WAV_stereo == 0 )
    {
     if ( WAV_16bit == 0 )
        {
         Value = (long)(*(char *)(Position1)-128);
         Value1 = Value2 = (Value * 65536) + ( ( (long)(*(char *)(Position2)-128) - Value) * Frac);
         Value1 = Value1 / 4096;
        }
     else
        {
         Value = (long)(*(short *)(Position1));
         Value1 = Value2 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2)) - Value ) * (__int64)Frac) / 256 );
         Value1 = Value1 / 4096;
        }
    }
 else
    {
     if ( WAV_16bit == 0 )
        {
         Value = (long)(*(char *)(Position1)-128);
         Value1 = ( (Value * 65536) + ( ( (long)(*(char *)(Position2)-128) - Value) * Frac) ) / 4096;
         Value = (long)(*(char *)(Position1+1)-128);
         Value2 = ( (Value * 65536) + ( ( (long)(*(char *)(Position2+1)-128) - Value) * Frac) ) / 4096;
        }
     else
        {
         Value = (long)(*(short *)(Position1));
         Value1 =  ( ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2)) - Value ) * (__int64)Frac) / 256 ) ) / 4096;
         Value = (long)(*(short *)(Position1+2));
         Value2 = ( ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2+2)) - Value ) * (__int64)Frac) / 256 ) ) / 4096;
        }
    }

 (*(short *)(BlockAddress+0)) += (short)Value1;
 (*(short *)(BlockAddress+2)) += (short)Value2;
}


void WAV_MixI32m( char *Position1, char *Position2, unsigned long Frac, unsigned long BlockAddress)
{
 static signed long Value, Value1, Value2;

 if ( WAV_stereo == 0 )
    {
     if ( WAV_16bit == 0 )
        {
         Value = (long)(*(char *)(Position1)-128);
         Value1 = Value2 = (Value * 65536) + ( ( (long)(*(char *)(Position2)-128) - Value) * Frac);
        }
     else
        {
         Value = (long)(*(short *)(Position1));
         Value1 = Value2 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2)) - Value ) * (__int64)Frac) / 256 );
        }
    }
 else
    {
     if ( WAV_16bit == 0 )
        {
         Value = (long)(*(char *)(Position1)-128);
         Value1 = (Value * 65536) + ( ( (long)(*(char *)(Position2)-128) - Value) * Frac);
         Value = (long)(*(char *)(Position1+1)-128);
         Value2 = (Value * 65536) + ( ( (long)(*(char *)(Position2+1)-128) - Value) * Frac);
         Value1 = Value1 + Value2 / 2;
        }
     else
        {
         Value = (long)(*(short *)(Position1));
         Value1 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2)) - Value ) * (__int64)Frac) / 256 );
         Value = (long)(*(short *)(Position1+2));
         Value2 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2+2)) - Value ) * (__int64)Frac) / 256 );
         Value1 = Value1 + Value2 / 2;
        }
    }

 (*(long *)(BlockAddress+0)) += Value1;
}


void WAV_MixI32s( char *Position1, char *Position2, unsigned long Frac, unsigned long BlockAddress)
{
 static signed long Value, Value1, Value2;

 if ( WAV_stereo == 0 )
    {
     if ( WAV_16bit == 0 )
        {
         Value = (long)(*(char *)(Position1)-128);
         Value1 = Value2 = (Value * 65536) + ( ( (long)(*(char *)(Position2)-128) - Value) * Frac);
        }
     else
        {
         Value = (long)(*(short *)(Position1));
         Value1 = Value2 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2)) - Value ) * (__int64)Frac) / 256 );
        }
    }
 else
    {
     if ( WAV_16bit == 0 )
        {
         Value = (long)(*(char *)(Position1)-128);
         Value1 = (Value * 65536) + ( ( (long)(*(char *)(Position2)-128) - Value) * Frac);
         Value = (long)(*(char *)(Position1+1)-128);
         Value2 = (Value * 65536) + ( ( (long)(*(char *)(Position2+1)-128) - Value) * Frac);
        }
     else
        {
         Value = (long)(*(short *)(Position1));
         Value1 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2)) - Value ) * (__int64)Frac) / 256 );
         Value = (long)(*(short *)(Position1+2));
         Value2 = ( (Value * 256) + ( (__int64)( (long)(*(short *)(Position2+2)) - Value ) * (__int64)Frac) / 256 );
        }
    }

 (*(long *)(BlockAddress+0)) += Value1;
 (*(long *)(BlockAddress+4)) += Value2;
}


void WAV_MixI( unsigned long Pos, unsigned long Frac, unsigned long Address)
{
 static char *Position1, *Position2;

 Position1 = (char *)(WAV_pointer + (Pos << (WAV_stereo+WAV_16bit)) );
 Position2 = (char *)(WAV_pointer + ((Pos+1) << (WAV_stereo+WAV_16bit)) );

 if ( (MixerType == 1) && (flag_stereo == 0) )  WAV_MixI16m(Position1, Position2, Frac, (unsigned long)MixerBlock + (Address<<1) );
 if ( (MixerType == 1) && (flag_stereo == 1) )  WAV_MixI16s(Position1, Position2, Frac, (unsigned long)MixerBlock + (Address<<2) );
 if ( (MixerType == 2) && (flag_stereo == 0) )  WAV_MixI32m(Position1, Position2, Frac, (unsigned long)MixerBlock + (Address<<2) );
 if ( (MixerType == 2) && (flag_stereo == 1) )  WAV_MixI32s(Position1, Position2, Frac, (unsigned long)MixerBlock + (Address<<3) );
}

// *** Interrupt which is called every soundcard frame ***

void interrupt_WAV ( )
{
 static unsigned long WAV_toplay = 0;

 WAV_buffer_rest = block_length;
 MixerAddress = 0;
 clearMixer();

while ( WAV_buffer_rest > 0 )
      {
        WAV_MixI ( WAV_Position, WAV_Fraction, MixerAddress );
       WAV_Fraction += WAV_FracAdd;
       WAV_Position += WAV_PosAdd + (WAV_Fraction >> 16);
       WAV_Fraction = WAV_Fraction & 65535;

       WAV_buffer_rest--;
       MixerAddress++;
       if (WAV_Position >= WAV_playlength )
          {
           if ( flag_loop == 0 )
              {
               flag_DatReady = 2;
               WAV_buffer_rest = 0;
              }
           else
              {
               WAV_Position = 0;
              }
          }
      }  // end while

 DoMasterVolumeCalculations( MixerBlock, Mixerpointer, block_length );
}


