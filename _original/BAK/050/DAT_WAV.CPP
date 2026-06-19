// WAV - Routines for F/X Player v1.0
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
#include "dat_wav.h"
//#include "dat_s3m.h"

#include "convert.h"
#include "mixer.h"


// Main programm

FILE *WAVhandle = NULL;

char *WAV_Buffer = NULL;
char *WAV_pointer = NULL;

unsigned short WAV_16bit = 0, WAV_stereo = 0;
unsigned long WAV_start = 0, WAV_length = 0, WAV_rate = 0;
unsigned long WAV_playlength = 0, WAV_played = 0;

unsigned long WAV_buffer_length = 0; // buffer length to equ dma size

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

short open_file_WAV ( char *FileName )
{

 if ( (WAVhandle=fopen(FileName,"rb") )==NULL )
      {
       return 1;
      }

return 0;
}

short close_file_WAV (void)
{

 if (WAVhandle == NULL)
    return 1;

 fclose ( WAVhandle );

 WAVhandle = NULL;

return 0;
}


short load_WAV( char *FileName )
{

 test = open_file_WAV ( FileName );
 if (test != 0) return 1;

 // load RIFF header

 if ( (fread( &R_header , 1, 8, WAVhandle ))!= sizeof(R_header) )
     return 3;

    if (memcmp(R_header.Riff, "RIFF", 4))
        return 3;

 // load chunk header

 if ( (fread( &C_header , 1, sizeof(C_header), WAVhandle ))!= sizeof(C_header) )
     return 3;

    if (memcmp(C_header.Wave, "WAVE", 4))
        return 3;
    if (memcmp(C_header.fmt, "fmt", 3))
        return 3;

 // load data chunk

 if ( (fread( &D_chunk , 1, sizeof(D_chunk), WAVhandle ))!= sizeof(D_chunk) )
     return 3;

    if (memcmp(D_chunk.data, "data", 4))
        {
         char *datapointer;
         char tempBuffer[1024];
         int i;

         fseek ( WAVhandle , sizeof(R_header)+sizeof(C_header), SEEK_SET );
         fread ( tempBuffer, 1, 512, WAVhandle );

         datapointer=(char *)memchr( tempBuffer,'d', 512 );
         i = datapointer-(char *)tempBuffer;

         if (memcmp(datapointer, "data" , 4))
            return 3;
         fseek ( WAVhandle, i+sizeof(R_header)+sizeof(C_header), SEEK_SET );
         fread( &D_chunk , 1, sizeof(D_chunk), WAVhandle );
        }

 WAV_start = ftell ( WAVhandle );

 WAV_stereo = (C_header.Channels) - 1;
 WAV_16bit =  C_header.BitsPerSample >> 4;

 WAV_length = D_chunk.Data_length;
 WAV_rate = C_header.Samplerate;

//printf("%d %d %d %d\r\n",WAV_stereo, WAV_16bit, WAV_length, WAV_rate);

 WAV_buffer_length = buffer_length;

 if (WAV_stereo!=flag_stereo)
    {
     if (WAV_stereo>flag_stereo) WAV_buffer_length <<= 1;
     if (WAV_stereo<flag_stereo) WAV_buffer_length >>= 1;
    }
 if (WAV_16bit!=flag_16bit)
    {
     if (WAV_16bit>flag_16bit) WAV_buffer_length <<= 1;
     if (WAV_16bit<flag_16bit) WAV_buffer_length >>= 1;
    }

//printf("%d\r\n",WAV_buffer_length);

 WAV_playlength = WAV_length >> (WAV_16bit+WAV_stereo);

 // get memory to load WAV *completly* into memory

 WAV_Buffer = (char *)malloc ( WAV_length );
 if ( WAV_Buffer == NULL )
    {
       close_file_WAV ();
       return 2;
    }

 // load WAV complete

 WAV_pointer = WAV_Buffer;  // set memory pointer to begin of data block

  fread( WAV_Buffer, 1, WAV_length, WAVhandle );

  flag_DatReady = 1;

return 0;
}

short close_WAV (void)
{

 close_file_WAV ();

 if ( WAV_Buffer == NULL )  return 1;
 free ( WAV_Buffer );
 WAV_Buffer = NULL;

return 0;
}

void play8M ( unsigned long toplay )
{
 if ( (flag_16bit == 0) && (flag_stereo == 0) )
    {
     Conv8Mto8M ( WAV_pointer, Mixerpointer, toplay  );
     WAV_pointer += toplay;
     Mixerpointer += toplay;
     WAV_played += toplay;
    }

 if ( (flag_16bit == 0) && (flag_stereo == 1) )
    {
     Conv8Mto8S ( WAV_pointer, Mixerpointer, toplay  );
     WAV_pointer += toplay;
     Mixerpointer += toplay*2;
     WAV_played += toplay;
    }

 if ( (flag_16bit == 1) && (flag_stereo == 0) )
    {
     Conv8Mto16M ( WAV_pointer, Mixerpointer, toplay  );
     WAV_pointer += toplay;
     Mixerpointer += toplay*2;
     WAV_played += toplay;
    }

 if ( (flag_16bit == 1) && (flag_stereo == 1) )
    {
     Conv8Mto16S ( WAV_pointer, Mixerpointer, toplay  );
     WAV_pointer += toplay;
     Mixerpointer += toplay*4;
     WAV_played += toplay;
    }
}

void play8S ( unsigned long toplay )
{
 if ( (flag_16bit == 0) && (flag_stereo == 0) )
    {
     Conv8Sto8M ( WAV_pointer, Mixerpointer, toplay  );
     WAV_pointer += toplay*2;
     Mixerpointer += toplay;
     WAV_played += toplay;
    }

 if ( (flag_16bit == 0) && (flag_stereo == 1) )
    {
     Conv8Sto8S ( WAV_pointer, Mixerpointer, toplay  );
     WAV_pointer += toplay*2;
     Mixerpointer += toplay*2;
     WAV_played += toplay;
    }

 if ( (flag_16bit == 1) && (flag_stereo == 0) )
    {
     Conv8Sto16M ( WAV_pointer, Mixerpointer, toplay  );
     WAV_pointer += toplay*2;
     Mixerpointer += toplay*2;
     WAV_played += toplay;
    }

 if ( (flag_16bit == 1) && (flag_stereo == 1) )
    {
     Conv8Sto16S ( WAV_pointer, Mixerpointer, toplay  );
     WAV_pointer += toplay*2;
     Mixerpointer += toplay*4;
     WAV_played += toplay;
    }
}

void play16M ( unsigned long toplay )
{

   if ( (flag_16bit == 0) && (flag_stereo == 0) )
      {
       Conv16Mto8M ( WAV_pointer, Mixerpointer, toplay  );
       WAV_pointer += toplay*2;
       Mixerpointer += toplay;
       WAV_played += toplay;
      }

   if ( (flag_16bit == 0) && (flag_stereo == 1) )
      {
       Conv16Mto8S ( WAV_pointer, Mixerpointer, toplay  );
       WAV_pointer += toplay*2;
       Mixerpointer += toplay*2;
       WAV_played += toplay;
      }

   if ( (flag_16bit == 1) && (flag_stereo == 0) )
      {
       Conv16Mto16M ( WAV_pointer, Mixerpointer, toplay  );
       WAV_pointer += toplay*2;
       Mixerpointer += toplay*2;
       WAV_played += toplay;
      }

   if ( (flag_16bit == 1) && (flag_stereo == 1) )
      {
       Conv16Mto16S ( WAV_pointer, Mixerpointer, toplay  );
       WAV_pointer += toplay*2;
       Mixerpointer += toplay*4;
       WAV_played += toplay;
      }
}

void play16S ( unsigned long toplay )
{

   if ( (flag_16bit == 0) && (flag_stereo == 0) )
      {
       Conv16Sto8M ( WAV_pointer, Mixerpointer, toplay  );
       WAV_pointer += toplay*4;
       Mixerpointer += toplay;
       WAV_played += toplay;
      }

   if ( (flag_16bit == 0) && (flag_stereo == 1) )
      {
       Conv16Sto8S ( WAV_pointer, Mixerpointer, toplay  );
       WAV_pointer += toplay*4;
       Mixerpointer += toplay*2;
       WAV_played += toplay;
      }

   if ( (flag_16bit == 1) && (flag_stereo == 0) )
      {
       Conv16Sto16M ( WAV_pointer, Mixerpointer, toplay  );
       WAV_pointer += toplay*4;
       Mixerpointer += toplay*2;
       WAV_played += toplay;
      }

   if ( (flag_16bit == 1) && (flag_stereo == 1) )
      {
       Conv16Sto16S ( WAV_pointer, Mixerpointer, toplay  );
       WAV_pointer += toplay*4;
       Mixerpointer += toplay*4;
       WAV_played += toplay;
      }
}


// *** Interrupt which is called every soundcard frame ***

void interupt_WAV ( )
{
 unsigned long toplay = 0;


if ( (WAV_16bit == 0) && (WAV_stereo == 0) )
   {
    if ( WAV_played >= WAV_playlength)
       {
        //if ( flag_16bit == 0 ) set8M ( Mixerpointer, 128, block_length << flag_stereo);
        //if ( flag_16bit == 1 ) set16M ( Mixerpointer, 32768, block_length << flag_stereo);
       }
    else
       {
        if ( (WAV_playlength - WAV_played) > block_length )
           {
            toplay = block_length;
            play8M ( toplay );
           }
        else
           {
            toplay = (WAV_playlength - WAV_played);
            if ( toplay > 0 )  play8M ( toplay );
            toplay = block_length - toplay;

            if ( flag_loop == 0)
               {
                if ( flag_16bit == 0 ) set8M ( Mixerpointer, 128, toplay << flag_stereo);
                if ( flag_16bit == 1 ) set16M ( Mixerpointer, 32768, toplay << flag_stereo);
                flag_DatReady = 2;
               }
            else
               {
                WAV_pointer = WAV_Buffer;
                WAV_played = 0;
                play8M ( toplay );
               }
           }
       }
   }


if ( (WAV_16bit == 0) && (WAV_stereo == 1) )
   {
    if ( WAV_played >= WAV_playlength)
       {
        //if ( flag_16bit == 0 ) set8M ( Mixerpointer, 128, block_length << flag_stereo);
        //if ( flag_16bit == 1 ) set16M ( Mixerpointer, 32768, block_length << flag_stereo);
       }
    else
       {
        if ( (WAV_playlength - WAV_played) > block_length )
           {
            toplay = block_length;
            play8S ( toplay );
           }
        else
           {
            toplay = (WAV_playlength - WAV_played);
            if ( toplay > 0 )  play8S ( toplay );
            toplay = block_length - toplay;
            if ( flag_loop == 0)
               {
                if ( flag_16bit == 0 ) set8M ( Mixerpointer, 128, toplay << flag_stereo);
                if ( flag_16bit == 1 ) set16M ( Mixerpointer, 32768, toplay << flag_stereo);
                flag_DatReady = 2;
               }
            else
               {
                WAV_pointer = WAV_Buffer;
                WAV_played = 0;
                play8S ( toplay );
               }
           }
       }
   }


if ( (WAV_16bit == 1) && (WAV_stereo == 0) )
   {
    if ( WAV_played >= WAV_playlength)
       {
        //if ( flag_16bit == 0 ) set8M ( Mixerpointer, 128, block_length << flag_stereo);
        //if ( flag_16bit == 1 ) set16M ( Mixerpointer, 32768, block_length << flag_stereo);
       }
    else
       {
        if ( (WAV_playlength - WAV_played) > block_length )
           {
            toplay = block_length;
            play16M ( toplay );
           }
        else
           {
            toplay = (WAV_playlength - WAV_played);
            if ( toplay > 0 )  play16M ( toplay );
            toplay = block_length - toplay;
            if ( flag_loop == 0)
               {
                if ( flag_16bit == 0 ) set8M ( Mixerpointer, 128, toplay << flag_stereo);
                if ( flag_16bit == 1 ) set16M ( Mixerpointer, 32768, toplay << flag_stereo);
                flag_DatReady = 2;
               }
            else
               {
                WAV_pointer = WAV_Buffer;
                WAV_played = 0;
                play16M ( toplay );
               }
           }
       }
   }


if ( (WAV_16bit == 1) && (WAV_stereo == 1) )
   {
    if ( WAV_played >= WAV_playlength)
       {
        //if ( flag_16bit == 0 ) set8M ( Mixerpointer, 128, block_length << flag_stereo);
        //if ( flag_16bit == 1 ) set16M ( Mixerpointer, 32768, block_length << flag_stereo);
       }
    else
       {
        if ( (WAV_playlength - WAV_played) > block_length )
           {
            toplay = block_length;
            play16S ( toplay );
           }
        else
           {
            toplay = (WAV_playlength - WAV_played);
            if ( toplay > 0 )  play16S ( toplay );
            toplay = block_length - toplay;
            if ( flag_loop == 0)
               {
                if ( flag_16bit == 0 ) set8M ( Mixerpointer, 128, toplay << flag_stereo);
                if ( flag_16bit == 1 ) set16M ( Mixerpointer, 32768, toplay << flag_stereo);
                flag_DatReady = 2;
               }
            else
               {
                WAV_pointer = WAV_Buffer;
                WAV_played = 0;
                play16S ( toplay );
               }
           }
       }
   }

}


