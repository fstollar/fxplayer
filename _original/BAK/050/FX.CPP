// **************************************************************************
// *                           F / X - Player                               *
// *                            v 0.50 beta                                 *
// *                   (c) 1996-98 by Apollo of STIGMA                      *
// **************************************************************************                                                                        *

#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>

#include "fx.h"
#include "command.h"

#include "irq.h"
#include "dma.h"

#include "device.h"
#include "dev_sb.h"

#include "format.h"
//#include "dat_wav.h"
//#include "dat_s3m.h"

#include "convert.h"
#include "mixer.h"


// *************************** Global variables *****************************

// *** Souncard variables

unsigned short CardType=1, SubCardType=0, DSPVersion=0;
unsigned short Adress=0x220, IRQ=5, DMAlo=1, DMAhi=5, DMA=0;
unsigned short flag_16bit=1, flag_stereo=1, flag_flip=0, flag_loop=0;
unsigned short Option = 0;
unsigned long MixSpeed=44100;

// *** DMA buffer variables

unsigned long block_length = 4096;      // block to play 8bit mono
unsigned long buffer_length = 0;        // block length respective to flags
__segment DMAselector;                  // selector of DMA memory
char *DMApointer = NULL;                // pointer to DMA memory

unsigned short MixerType = 1;           // type of mixer (1, 2 or 3)
unsigned char Mixer_Block = 0;          // tells which block is now played
char *Mixerpointer;                     // pointer of actual block in DMA mem
char *Mixerpointer1, *Mixerpointer2;    // pointers to both DMA blocks
char *MixerBlock = NULL;                // memory where channels are mixed
unsigned long MixerAdress;

// *** Soundfile varaibles

unsigned short DatType = 0;             // says which format is activ now
unsigned short Last_DatType = 0;        // which was loaded last time
unsigned char flag_DatReady = 0;        // is data ready ?

char FileName[255];

// *** dummys :) ***

long dummy, test;


// ****************************** Main program ******************************

void main (int argc, char **argv)
{

printf("\n F/X - Player v0.50 alpha\n");
printf(" (c) 1996-98 by Apollo / STIGMA\n");
printf(" compiled on %s, %s, happy birthday to me !\n", __DATE__, __TIME__);
printf("\nType >FX -h< for help on command line switches\n\n");

  if ( comline(argc, argv) == -1 )  return;  // by error exit

  test=checkCard (CardType, Adress);
  if (test) { puts("Soundcard not found !!"); return; }

  buffer_length = block_length << (flag_16bit+flag_stereo);  // real length


  initCard ( CardType, Adress, IRQ, DMAlo, DMAhi, flag_16bit, flag_stereo, buffer_length);


switch( SubCardType )
      {
       case 1:
              printf("Soundblaster 1.5 found at Address: %x  IRQ: %i  DMA: %i\n", Adress, IRQ, DMA);
              break;
       case 2:
              printf("Soundblaster 2.0 found at Address: %x  IRQ: %i  DMA: %i\n", Adress, IRQ, DMA);
              break;
       case 3:
              printf("Soundblaster PRO found at Address: %x  IRQ: %i  DMA: %i\n", Adress, IRQ, DMA);
              break;
       case 4:
              break;
       case 5:
              printf("Soundblaster 16/32/64 found at Address: %x  IRQ: %i  DMA: %i\n", Adress, IRQ, DMA);
              break;
      }

printf("DSP Version %i.%02i playing in", DSPVersion/256, DSPVersion%256 );
if ( flag_16bit == 0 )   printf(" 8 bit");  else  printf(" 16 bit");
if ( flag_stereo == 0 )  printf(" Mono");   else  printf(" Stereo");
printf(" with %u Hz\n\n", MixSpeed );


  initDMAbuffer ( buffer_length );

  startCard ( CardType, SubCardType, flag_16bit, flag_stereo, buffer_length, MixSpeed );


  initMixer ( MixerType );

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
              case 1 : cputs("Not found\r\n"); break;
              case 2 : cputs("Memory error\r\n"); break;
              case 3 : cputs("File corrupt\r\n"); break;
             }
      return;
     }

  startMixer ( MixerType );

puts("Loaded !!");


getch();
system("command.com");


  close_Format();

  stopCard();

// ***

  closeMixer ();

  clearCard ();

puts("You did it !!");

return;
}
