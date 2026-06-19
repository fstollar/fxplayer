#include <i86.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>

#include "fx.h"

//#include "irq.h"
//#include "dma.h"

#include "device.h"
#include "dev_wss.h"

//#include "format.h"
//#include "dat_wav.h"
//#include "dat_s3m.h"

//#include "convert.h"
//#include "mixer.h"

// *** WSS programing datas

static unsigned long  WSS_io = 0,  WSS_mix = 0;
static unsigned char  WSS_irq = 0, WSS_dma = 0;
static unsigned char  oldmuteMono, oldmuteLeft, oldmuteRight;

// *** SB io Addresses

#define WSS_PORT          (WSS_io+0x4)
#define WSS_DATA          (WSS_io+0x5)
#define WSS_STATUS        (WSS_io+0x6)
#define WSS_POLLED        (WSS_io+0x7)

// *** in/out routines for the Windows Sound System ***

static unsigned char read_WSS ( unsigned long port )
{
  outp(WSS_PORT, port);
  return inp(WSS_DATA);
}

static void write_WSS ( unsigned long port, unsigned long Value )
{
  outp(WSS_PORT, port);
  outp(WSS_DATA, Value);
}

// *** check for WSS

unsigned long checkWSS ( unsigned long Address )
{
 unsigned short test;

 WSS_io = Address;

 if ( (inp ( WSS_PORT ) & 0x80) != 0x00 )
      return 0;

 test = inp ( WSS_io + 3 ) & 0x3f;
 if ( (test != 0x00) && (test != 0x04) && (test != 0x0f) )
      return 0;

 return 1;
}

void initWSS ( unsigned long Address )
{
  unsigned char ver;

  oldmuteMono = read_WSS(0x1A);
  oldmuteLeft = read_WSS(0x06);
  oldmuteRight = read_WSS(0x07);

  flag_signed = flag_16bit;

  write_WSS(0x0C, 0);
  ver=inp(WSS_PORT)&0x0F;
//  printf("ver: %i\n",ver);

  write_WSS(0x0c, 0);
  write_WSS(0x09, 0);
  write_WSS(0x48,0);

/*
  write_WSS(0x0C, 0);
  ver=inp(WSS_DATA);
  outp(WSS_DATA, 0);
  if (inp(WSS_DATA)!=ver)
    printf("Naa1\n");

  if (inp(WSS_PORT)&0x80)
    printf("Naa2\n");

  write_WSS(0x09, 0);
  outp(WSS_PORT, 0x09);

  write_WSS(0x0C, 0);
*/
  write_WSS(0x1A, oldmuteMono &~ 0x40);
  write_WSS(0x06, oldmuteLeft &~ 0x80);
  write_WSS(0x07, oldmuteRight &~ 0x80);
}

void closeWSS ()
{
  write_WSS(0x1A, oldmuteMono);
  write_WSS(0x06, oldmuteLeft);
  write_WSS(0x07, oldmuteRight);

  write_WSS ( 0x0c, 0 );
  write_WSS ( 0x09, 0 );
}


unsigned long setMixWSS ( unsigned long Speed )
{
static unsigned long rates[16]= {
    8000, 5513,16000,11025,27429,18900,32000,22050,
   54857,37800,64000,44100,48000,33075, 9600, 6615 };

// bit   0=> base frq: 384000,264600
// bit 3-1=> frq div:  48,24,14,12,7,6,8,40

unsigned long realMixSpeed, BitSpeed;
unsigned long i, Bits, diff = 65000;

 for ( i = 0; i < 16; i++ )
     {
      if ( abs(Speed - rates[i]) < diff )
         {
          diff = abs(Speed - rates[i]);
//          printf("diff: %i\n",diff);
          realMixSpeed = rates[i];
          Bits = i;
         }
     }

  BitSpeed = Bits | (flag_16bit?0x40:0x00) | (flag_stereo?0x10:0x00);

  write_WSS(0x48, BitSpeed);

  //while (inp(WSS_PORT)&0x80);

return realMixSpeed;
}


void setLengthWSS ( unsigned long length )
{
//  write_WSS(0x0C, 0);
  write_WSS(0x0F, (length-1) & 255 );
  write_WSS(0x0E, (length-1) >> 8  );
}


void startWSS ( )
{
  write_WSS(0x09, 1);
}


void stopWSS ( )
{
  write_WSS(0x0A, 0);
  write_WSS(0x09, read_WSS(0x09) &~ 0x01);
}


// *** Windows Sound System interrupt which tolds WSS, IRQ is reconized ***

//static unsigned char status;

void interrupt_WSS ()
{
// status = inp ( WSS_STATUS );
 outp ( WSS_STATUS, 0 );
 Mixer_Block = Mixer_Block ^ 0xff;    // switch mixer block
}



