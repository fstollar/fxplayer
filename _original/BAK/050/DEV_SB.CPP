#include <i86.h>
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

//#include "convert.h"
//#include "mixer.h"

// *** SB programing datas

static unsigned short SB_io = 0, SB_mix = 0;
static unsigned char SB_irq = 0, SB_dma = 0;

// *** SB io adresses

#define MIXER_ADRESS      (SB_io+0x4)
#define MIXER_DATA        (SB_io+0x5)
#define DSP_RESET         (SB_io+0x6)
#define DSP_READ_DATA     (SB_io+0xa)
#define DSP_WRITE_DATA    (SB_io+0xc)
#define DSP_WRITE_STATUS  (SB_io+0xc)
#define DSP_DATA_AVAIL    (SB_io+0xe)


// *** in/out routines for the Soundblaster ***


static unsigned char read_DSP ()
{
  while ( !(inp(DSP_DATA_AVAIL)&128) );
  return inp(DSP_READ_DATA);
}

static void write_DSP ( unsigned char Value )
{
  while ( (inp(DSP_WRITE_STATUS)&128) );
  outp(DSP_WRITE_DATA, Value);
}


static unsigned char read_Mixer ( unsigned short Adress )
{
 outp(MIXER_ADRESS, Adress);
 return inp(MIXER_DATA);
}

static void write_Mixer ( unsigned short Adress, unsigned char Value )
{
 outp(MIXER_ADRESS, Adress);
 outp(MIXER_DATA, Value);
}


// *** Reset Soundblaster ***


unsigned char reset_SB (unsigned short Adress)
{
 unsigned long counter=65536;

 SB_io=Adress;

 outp (DSP_RESET, 1);
 delay ( 10 );         // better too much then too less ;)
 outp (DSP_RESET, 0);
 delay ( 10 );

 while ( (!(inp(DSP_DATA_AVAIL)&128)) || (counter!=0) )  counter--;

 return inp(DSP_READ_DATA);
}


unsigned short getDSPversion ()
{
 write_DSP ( 0xE1 );                      // say DSP, I want know his version
 return ( read_DSP() *256 + read_DSP() ); // and read hi- / low-byte
}


unsigned short checkSB (unsigned short Adress)
{
 unsigned short DSPNr, DSPver = 0;

 SB_io=Adress;

 if (reset_SB(Adress)==0xaa)         // check if DSP resets
    {
     DSPNr = getDSPversion();        // get DSP version number

     if (DSPNr <= 4.5*256) DSPver=5;            // SB 16 and above
     if (DSPNr <  4.0*256) { DSPver=3; DSPver=4; } // SB pro
     if (DSPNr <= 2.5*256) DSPver=2;            // SB 2.01
     if (DSPNr <= 2.0*256) DSPver=1;            // SB 1.0
    }

 return DSPver;
}


void initSB ( unsigned short Adress)
{
 SB_io=Adress;         // set adress

 write_DSP( 0xd1 );    // turn on speaker
}


void closeSB ()
{
 write_DSP ( 0xd3 );   // turn speaker off
 reset_SB ( Adress );
}

// ********************* SB 1.0 - 2.0 setups ********************

unsigned long setMixSB1( unsigned long MixSpeed )
{
 unsigned long realMix=0;
 unsigned short MixValue;

 write_DSP( 0x40 );                                // set sampling rate

// Samplingrate : Normal 256 - (1000000 DIV Freq)
//                High 65536 - (256000000 Div Freq)
//                HIGH BYTE only !

 MixValue = (( 65536 - (256000000 / MixSpeed) ) >> 8);  // calculate value
                                                        // for mixing speed
 write_DSP( MixValue );                            // write into DSP

 realMix = 256000000 / (65536 - (MixValue << 8));  // what is the real freq.

 return realMix;
}

void setLengthSB1( unsigned short length )
{
 write_DSP( 0x48 );            // set block length
 write_DSP( (length-1)&255 );  // length-1 low byte
 write_DSP( (length-1)>>8 );   // length-1 high byte
}

void startSB1()
{
 write_DSP( 0x1c );    // start 8 bit auto init
}

void stopSB1()
{
 write_DSP ( 0xd0 );
}

// ********************* SB 2.01 - 2.5 setups ********************

unsigned long setMixSB2( unsigned long MixSpeed )
{
 unsigned long realMix=0;
 unsigned short MixValue;

 write_DSP( 0x40 );                                // set sampling rate

// Samplingrate : Normal 256 - (1000000 DIV Freq)
//                High 65536 - (256000000 Div Freq)
//                HIGH BYTE only !

 MixValue = (( 65536 - (256000000 / MixSpeed) ) >> 8);  // calculate value
                                                        // for mixing speed
 write_DSP( MixValue );                            // write into DSP

 realMix = 256000000 / (65536 - (MixValue << 8));  // what is the real freq.

 return realMix;
}

void setLengthSB2( unsigned short length )
{
 write_DSP( 0x48 );            // set block length
 write_DSP( (length-1)&255 );  // length-1 low byte
 write_DSP( (length-1)>>8 );   // length-1 high byte
}

void startSB2()
{
 write_DSP( 0x90 );    // 8 bit HIGH-SPEED with auto init
}

void stopSB2()
{
 write_DSP ( 0xd0 );
}

// *********************** SB pro setups **********************

unsigned long setMixSBpro( unsigned long MixSpeed )
{
 unsigned long realMix=0;
 unsigned short MixValue;

 write_DSP( 0x40 );                                // set sampling rate

// Samplingrate : Normal 256 - (1000000 DIV Freq)
//                High 65536 - (256000000 Div Freq)
//                HIGH BYTE only !

 MixValue = (( 65536 - (256000000 / (MixSpeed << flag_stereo) ) ) >> 8);  // calculate value
                                                        // for mixing speed
 write_DSP( MixValue );                            // write into DSP

 realMix = (256000000 / (65536 - (MixValue << 8)) ) >> flag_stereo ;  // what is the real freq.

 return realMix;
}

void setLengthSBpro( unsigned short length )
{
 write_DSP( 0x48 );            // set block length
 write_DSP( (length-1)&255 );  // length-1 low byte
 write_DSP( (length-1)>>8 );   // length-1 high byte
}

void startSBpro()
{
 write_DSP( 0x90 );    // start 8 bit auto init

 if ( flag_stereo == 1 )
     {
      // here is the code to make the pro play stereo (hopefully :)
      write_Mixer( 0x0e, read_Mixer( 0x0e ) | 2 );
     }
 else
     {
      write_Mixer( 0x0e, read_Mixer( 0x0e ) & ~2 );
     }
}

void stopSBpro()
{
 write_DSP ( 0xd0 );
}

// ****************** SB16/AWE setups ****************

void setMixSB16 ( unsigned long MixSpeed )
{
 write_DSP( 0x41 );             // set mixrate
 write_DSP( (MixSpeed >> 8) );    // high byte
 write_DSP( (MixSpeed & 255) );   // low byte
}

void startSB16 ( unsigned short length , unsigned short flag_16bit, unsigned short flag_stereo )
{
 if (flag_16bit==0)  write_DSP( 0xc6 );     // 8bit Auto-init with FIFO
 else  write_DSP( 0xb6 );                   // 16bit Auto-init with FIFO
 write_DSP( (flag_stereo << 5) );             // mono/stereo and unsigned

 write_DSP( (length-1) & 255 );  // length-1 low byte
 write_DSP( (length-1) >> 8 );   // length-1 high byte
}

void stopSB16 ()
{
 if ( flag_16bit==0 ) write_DSP ( 0xda );  // stop 8bit transfer
 else write_DSP ( 0xd9 );                  // stop 16bit trans
}


// *** Soundblaster interrupt which tolds SB, IRQ is reconized ***

void interupt_SB ()
{
 inp( DSP_DATA_AVAIL + flag_16bit );   // say, IRQ is ok
 Mixer_Block = (Mixer_Block ^ 255);    // switch mixer block
}



