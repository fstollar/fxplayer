
#include <i86.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>

#include "fx.h"

//#include "irq.h"
//#include "dma.h"

//#include "device.h"
#include "dev_sb.h"

//#include "format.h"
//#include "dat_wav.h"
//#include "dat_s3m.h"

//#include "convert.h"
//#include "mixer.h"

// *** SB programing datas

static unsigned long SB_io = 0;
static unsigned char SB_irq = 0, SB_dma = 0;

// *** SB io Addresses

#define MIXER_ADDRESS     (SB_io+0x4)
#define MIXER_DATA        (SB_io+0x5)
#define DSP_RESET         (SB_io+0x6)
#define DSP_READ_DATA     (SB_io+0xa)
#define DSP_WRITE_DATA    (SB_io+0xc)
#define DSP_WRITE_STATUS  (SB_io+0xc)
#define DSP_DATA_AVAIL    (SB_io+0xe)

// *** in/out routines for the Soundblaster ***

static unsigned char read_DSP ()
{
 unsigned long counter=65536;

 while ( (!(inp(DSP_DATA_AVAIL)&128)) && (counter!=0) )  counter--;
 return inp(DSP_READ_DATA);
}

static void write_DSP ( unsigned char Value )
{
 unsigned long counter=65536;

 while ( (inp(DSP_WRITE_STATUS)&128) && (counter!=0) )  counter--;
 outp(DSP_WRITE_DATA, Value);
}


static unsigned char read_Mixer ( unsigned long Address )
{
 outp(MIXER_ADDRESS, Address);
 delay( 2 );
 return inp(MIXER_DATA);
}

static void write_Mixer ( unsigned long Address, unsigned char Value )
{
 outp(MIXER_ADDRESS, Address);
 delay ( 2 );
 outp(MIXER_DATA, Value);
}


// *** Reset Soundblaster ***


unsigned char reset_SB (unsigned long Address)
{
 unsigned long counter=65536;

 SB_io=Address;

 outp (DSP_RESET, 1);
 delay ( 10 );         // better too much then too less ;)
 outp (DSP_RESET, 0);
 delay ( 10 );

 while ( (!(inp(DSP_DATA_AVAIL)&128)) && (counter!=0) )  counter--;

 return inp(DSP_READ_DATA);
}


unsigned long getDSPversion ()
{
 write_DSP ( 0xE1 );                      // say DSP, I want know his version
 return ( read_DSP() *256 + read_DSP() ); // and read hi- / low-byte
}


unsigned long checkSB (unsigned long Address)
{
 unsigned short DSPNr, DSPver = 0;

 SB_io=Address;

 if (reset_SB(Address)==0xaa)         // check if DSP resets
    {
     DSPNr = getDSPversion();        // get DSP version number

     if (DSPNr <= 4.5*256) { DSPver=5; }                 // SB 16 and above
     if (DSPNr <  4.0*256) { DSPver=4; flag_signed=0; }  // SB pro
     if (DSPNr <= 2.5*256) { DSPver=3; flag_signed=0; }  // SB 2.01
     if (DSPNr <= 2.0*256) { DSPver=2; flag_signed=0; }  // SB 1.5
     if (DSPNr <  1.5*256) { DSPver=1; flag_signed=0; }  // SB 1.0
    }

 return DSPver;
}


void initSB ( unsigned long Address)
{
 SB_io=Address;         // set Address
 write_DSP( 0xd1 );    // turn on speaker
}


void closeSB ()
{
 write_DSP ( 0xd3 );   // turn speaker off
 reset_SB ( Address );
}

// ********************* SB 1.0 - 2.0 setups ********************

unsigned long setMixSB1( unsigned long MixSpeed )
{
 unsigned long realMix;
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

void setLengthSB1( unsigned long length )
{
 write_DSP( 0x48 );            // set block length
 write_DSP( (length-1) & 255 );  // length-1 low byte
 write_DSP( ((length-1) >> 8) & 255 );   // length-1 high byte
}

void startSB1()
{
 write_DSP( 0x1c );    // start 8 bit auto init
}

void stopSB1()
{
 write_DSP ( 0xda );
 write_DSP ( 0xd0 );
}

// ********************* SB 2.01 - 2.5 setups ********************

unsigned long setMixSB2( unsigned long MixSpeed )
{
 unsigned long realMix;
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

void setLengthSB2( unsigned long length )
{
 write_DSP( 0x48 );            // set block length
 write_DSP( (length-1) & 255 );  // length-1 low byte
 write_DSP( ((length-1) >> 8) & 255 );   // length-1 high byte
}

void startSB2()
{
 write_DSP( 0x90 );    // 8 bit HIGH-SPEED with auto init
}

void stopSB2()
{
 write_DSP ( 0xda );
 write_DSP ( 0xd0 );
}

// *********************** SB pro setups **********************

unsigned long setMixSBpro( unsigned long MixSpeed )
{
 unsigned long realMix;
 unsigned long MixValue;

 write_DSP( 0x40 );                                // set sampling rate

// Samplingrate : Normal 256 - (1000000 DIV Freq)
//                High 65536 - (256000000 Div Freq)
//                HIGH BYTE only !

 MixValue = ( 65536 - (256000000 / (MixSpeed << 1) ) ) >> 8;  // calculate value
                                                        // for mixing speed
 write_DSP( MixValue );                            // write into DSP

 realMix = (256000000 / (65536 - (MixValue << 8))) >> 1;  // what is the real freq.

 return realMix;
}

void setLengthSBpro( unsigned long length )
{
 length <<= flag_stereo;
 write_Mixer( 0x0e, read_Mixer( 0x0e ) &~ 0x02 | (flag_stereo?0x22:0x20) );
 write_DSP( 0x48 );                      // set block length
 write_DSP( (length-1) & 255 );          // length-1 low byte
 write_DSP( ((length-1) >> 8) & 255 );   // length-1 high byte
}

void startSBpro()
{
 // here is the code to make the pro play stereo (hopefully :)
 write_Mixer( 0x0e, read_Mixer( 0x0e ) &~ 0x02 | (flag_stereo?0x22:0x20) );
 write_DSP( 0x10 );
 write_DSP( 0x80 );
 write_DSP( 0x90 );    // start 8 bit auto init
 write_Mixer( 0x0e, read_Mixer( 0x0e ) &~ 0x02 | (flag_stereo?0x22:0x20) );
}

void stopSBpro()
{
 write_DSP ( 0xda );
 write_DSP ( 0xd0 );
}

// ****************** SB16/AWE setups ****************

void setMixSB16 ( unsigned long MixSpeed )
{
 write_DSP( 0x41 );                          // set mixrate
 write_DSP( (MixSpeed >> 8) & 255 );         // high byte
 write_DSP( MixSpeed & 255 );                // low byte
}

void startSB16 ( unsigned long length , unsigned char flag_16bit, unsigned char flag_stereo )
{
 length <<= flag_stereo;
 write_DSP( flag_16bit ? 0xb6 : 0xc6 );    // 8bit Auto-init with FIFO
                                           // 16bit Auto-init with FIFO
 write_DSP( (flag_stereo << 5) + (flag_signed << 4) );  // mono/stereo and un-/signed

 write_DSP( (length-1) & 255 );               // length-1 low byte
 write_DSP( ((length-1) >> 8) & 255 );        // length-1 high byte
}

void stopSB16 ()
{
 write_DSP ( flag_16bit ? 0xda : 0xd9 );   // stop 8bit transfer
                                           // stop 16bit trans
 write_DSP ( flag_16bit ? 0xd0 : 0xd5 );   // stop output
}


// *** Soundblaster interrupt which tolds SB, IRQ is reconized ***

void interrupt_SB ()
{
 inp( DSP_DATA_AVAIL + flag_16bit );   // say, IRQ is ok
 Mixer_Block = Mixer_Block ^ 0xff;    // switch mixer block
}



