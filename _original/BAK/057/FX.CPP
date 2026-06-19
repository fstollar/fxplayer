
// **************************************************************************
// *                           F / X - Player                               *
// *                            v 0.57 alpha                                *
// *                   (c) 1996-98 by Apollo of STIGMA                      *
// **************************************************************************                                                                        *

#define Mainversion          0
#define Subversion          57
#define Release        "alpha"

#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>

//#include "fx.h"
#include "command.h"
#include "device.h"
#include "format.h"
#include "mixer.h"


// *************************** Global variables *****************************

// *** Souncard variables

unsigned short CardType=1, SubCardType=0, DSPVersion=0;
unsigned short Address=0x220, IRQ=5, DMAlo=1, DMAhi=5, DMA=0;
unsigned char  flag_16bit=1, flag_stereo=1, flag_signed=0;
unsigned long  MixSpeed=44100;

// *** Mixing variables

unsigned char  flag_flip=0, flag_loop=0;
unsigned char  flag_interpolate=1, flag_softclip=1, flag_precalc=0;

// *** DMA buffer variables

unsigned long  block_length = 4096;      // block to play 8bit mono
unsigned long  buffer_length = 0;        // block length respective to flags
__segment      DMAselector;              // selector of DMA memory
char           *DMApointer = NULL;       // pointer to DMA memory

unsigned short MixerType = 1;            // type of mixer (1, 2 or 3)
unsigned short flag_MixerReady = 0;      // tells when mixer is ready
unsigned char  Mixer_Block = 0;          // tells which block is now played
char           *Mixerpointer;            // pointer of actual block in DMA mem
char     *Mixerpointer1, *Mixerpointer2; // pointers to both DMA blocks
char           *MixerBlock = NULL;       // memory where channels are mixed
unsigned long  Mixer_length = 0;         // how long is the mix buffer
unsigned long  MixerAddress;             // where in the buffer are we

// *** Soundfile varaibles

unsigned short DatType = 0;              // says which format is activ now
unsigned short Last_DatType = 0;         // which was loaded last time
unsigned char  flag_DatReady = 0;        // is data ready ?

char           FileName[255];            // filename of the actuall playing

// *** dummys :) ***

long test;                               // humpf ?


// ****************************** Main program ******************************

void main (int argc, char **argv)
{
 // *** The Player Headline

 printf("\n F/X - Player v%1u.%2u %s\n", Mainversion, Subversion, Release);
 printf(" (c) 1996-98 by Apollo / STIGMA\n");
 printf(" compiled on %s, %s, at Mekka'98\n", __DATE__, __TIME__);
 printf("\nType >FX -h< for help on command line switches\n\n");

 // *** read the commandline

 if ( comline(argc, argv) == -1 )  return;  // by error exit

 test=checkCard (CardType, Address);
 if (test)
    {
     puts("Soundcard not found !!");
     return;
    }

 buffer_length = block_length << (flag_16bit+flag_stereo);  // real length

 initCard ( );

 initDMAbuffer ( buffer_length );

 startCard ( block_length, MixSpeed );
 writeCard ( );

 initMixer ( MixerType );
 writeMixer ( );

  DatType = test_File ( FileName );
  if (DatType == 0)
     {
      clearCard();
      puts("This is no yet known sound format !");
      return;
     }


  test=load_Format( FileName, DatType );
  if (test!=0)
     {
      clearCard();
      switch ( test )
             {
              case 1 : puts("File not found"); break;
              case 2 : puts("Memory error"); break;
              case 3 : puts("File corrupt"); break;
             }
      return;
     }

  startMixer ( MixerType );

puts("Loaded !!");


getch();
//system("command.com");


  close_Format();

  stopCard();

// ***

  closeMixer ();

  clearCard ();

puts("You did it !!");

return;
}
