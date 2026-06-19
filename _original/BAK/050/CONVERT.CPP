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

//#include "format.h"
//#include "dat_wav.h"
//#include "dat_s3m.h"

#include "convert.h"
#include "mixer.h"

// Main programm


// *** convert unsigned samples into signed samples ***

void ConvU8MtoS8M ( char *source, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     *source++-=128;
}

void ConvU16MtoS16M ( char *source, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     *(short *)source++-=32768;
}

// *** convert 8M audio to 8M, 8S, 16M and 16S ***

void Conv8Mto8M ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
      *target++ = *source++;
}

void Conv8Mto8S ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(target + 1) = *target = *source++;

      target+=2;
     }
}

void Conv8Mto16M ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)target = (unsigned short)(*source) << 8;

      source++;
      target+=2;
     }
}

void Conv8Mto16S ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)(target + 2) = *(short *)target = (unsigned short)(*source) << 8;

      source++;
      target+=4;
     }
}


// *** convert 8S audio to 8M, 8S, 16M and 16S ***

void Conv8Sto8M ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *target++ = ( (*source) + (*(source + 1) ) ) / 2;
      source+=2;
     }
}

void Conv8Sto8S ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)target = *(short *)source;

      source+=2;
      target+=2;
     }
}

void Conv8Sto16M ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)target = (unsigned short)(( (*source) + (*(source + 1) ) ) / 2) << 8;

      source+=2;
      target+=2;
     }
}

void Conv8Sto16S ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)target = (unsigned short)(*source) << 8;
      *(short *)(target + 2) = (unsigned short)(*(source + 1)) << 8;

      source+=2;
      target+=4;
     }
}


// *** convert 16M audio to 8M, 8S, 16M and 16S ***

void Conv16Mto8M ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *target = (unsigned char)( ( (32768 + *(short *)source) + 128 ) >> 8);

      source+=2;
      target++;
     }
}

void Conv16Mto8S ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(target + 1) = *target = (unsigned char)( ((32768 + *(short *)source) + 128) >> 8);

      source+=2;
      target+=2;
     }
}

void Conv16Mto16M ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)target =  (32768 + *(short *)source);

      source+=2;
      target+=2;
     }
}

void Conv16Mto16S ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)(target + 2) = *(short *)target = 32768 + *(short *)source;

      source+=2;
      target+=4;
     }
}


// *** convert 16S audio to 8M, 8S, 16M and 16S ***

void Conv16Sto8M ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *target = (unsigned char)(( ( (32768 + *(short *)source) + (32768 + *(short *)(source + 2)) ) / 2 + 128 ) >> 8);

      source+=4;
      target++;
     }
}

void Conv16Sto8S ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *target = (unsigned char)( ((32768 + *(short *)source) + 128) >> 8);
      *(target + 1) = (unsigned char)( ((32768 + *(short *)(source + 2)) + 128) >> 8);

      source+=4;
      target+=2;
     }
}

void Conv16Sto16M ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)target = (unsigned short)( ( (32768 + *(short *)source) + (32768 + *(short *)(source + 2)) ) / 2 ) ;

      source+=4;
      target+=2;
     }
}

void Conv16Sto16S ( char *source, char *target, unsigned long length )
{
 unsigned long i;

 for (i=0; i<length; i++)
     {
      *(short *)target = 32768 + *(short *)source;
      *(short *)(target + 2) = 32768 + *(short *)(source + 2);

      source+=4;
      target+=4;
     }
}
