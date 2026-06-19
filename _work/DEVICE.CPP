#include <stdio.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#include "fx.h"

#include "irq.h"
#include "dma.h"

#include "device.h"
#include "dev_sb.h"
#include "dev_wss.h"

//#include "format.h"
//#include "dat_wav.h"
//#include "dat_s3m.h"
extern void interrupt_WAV();
extern void interrupt_S3M();
extern void interrupt_MOD();
extern void interrupt_669();

//#include "convert.h"
//#include "mixer.h"

// *** Variables ***

static unsigned long clearvalue = 0;

#define NUL 0
#define SB  1
  #define SB_10      1
  #define SB_15      2
  #define SB_20      3
  #define SB_pro     4
  #define SB_16      5
#define WSS 2
  #define WSS_4248   1
  #define WSS_4231   2
  #define WSS_4231A  3
  #define WSS_4232   4
  #define WSS_4232A  5
#define GUS 3

// ******************* Sound Devices for the F/X Player *********************

void writeCard( )
{

switch ( CardType )
       {
        case SB  :                          // Soundblaster
                   printf("Soundblaster ");
                   switch ( SubCardType )
                          {
                           case SB_10 :
                                   printf("1.0");
                                   break;
                           case SB_15 :
                                   printf("1.5");
                                   break;
                           case SB_20 :
                                   printf("2.0");
                                   break;
                           case SB_pro :
                                   printf("PRO");
                                   break;
                           case SB_16 :
                                   printf("16/32/64/AWE");
                                   break;
                          }
                   printf(" found at Address: %x  IRQ: %u  DMA: %u\n", Address, IRQ, DMA);
                   printf("DSP Version %u.%02u", DSPVersion/256, DSPVersion%256 );
                   break;
        case WSS :                          // Windows Sound System

                   printf("Windows Sound System found at Address: %x  IRQ: %u  DMA: %u\n", Address, IRQ, DMA);
                   switch ( SubCardType )
                          {
                           case WSS_4248  :
                                            printf("CS4248/AD1848");
                                            break;
                           case WSS_4231  :
                                            printf("CS4231");
                                            break;
                           case WSS_4231A :
                                            printf("CS4231A/AD1845");
                                            break;
                           case WSS_4232  :
                                            printf("CS4232 PnP");
                                            break;
                           case WSS_4232A :
                                            printf("CS4232A PnP");
                                            break;
                          }
                   break;
        case GUS :                          // Gravis Ultrasound
                   break;

       }

 printf(" playing in %u bit %s with %u Hz\n\n", flag_16bit ? 16 : 8, flag_stereo ? "Stereo" : "Mono", MixSpeed);
}


long checkCard ( unsigned long CardType, unsigned long Address)
{
 long test;

 switch( CardType )
        {
         case SB  :                          // Soundblaster
                    test=checkSB( Address );     // get SB type
                    if (test == 0)  return 1;    // if 0 then not found
                    SubCardType=test;            //
                    DSPVersion=getDSPversion();  // get DSP version number
                    break;
         case WSS :                          // Windows Sound System
                    test=checkWSS( Address );
                    if (test == 0)  return 1;
                    SubCardType=test;
                    break;
         case GUS :                          // Gravis Ultrasound
                    break;
        }

return 0;
}


long initCard ( )
{
 unsigned long value;

 switch( CardType )
        {
         case SB  :                           // Soundblaster
                    initSB ( Address );

                    if ( (flag_16bit == 1) && (DMAhi > 3 ))
                        DMA = DMAhi;
                    else
                        DMA = DMAlo;
                    break;
         case WSS :                          // Windows Sound System
                    initWSS ( Address );
                    DMA = DMAlo;
                    break;
         case GUS :                          // Gravis Ultrasound
                    break;
        }

 value = setNewIRQ (IRQ, PlainInterupt, 0);

 if (value != 0)
    {
     puts("could not handle interrupt !! May be wrong ! Check it !\n");
     return 1;
    }

 DMApointer = (char *)DMAalloc( buffer_length << 1 , DMAselector );
 if (DMApointer == NULL)
    {
     puts("DMA memory error !! I need about 64K base memory for DMA playback !\n");
     return 2;
    }

 clearvalue = flag_16bit ? (flag_signed?0x0000:0x8000) : (flag_signed?0x0000:0x8080);

 initDMA( DMA, 8+16+64, DMApointer, buffer_length << 1 );
 clearDMAmem( clearvalue, DMApointer, buffer_length << 1 );

return 0;
}


void startCard ( unsigned long Length, unsigned long Speed)
{

 switch( CardType )
        {
         case SB  :                          // Soundblaster
                    switch( SubCardType )
                          {
                           case SB_15 :
                                   MixSpeed = setMixSB1( Speed );
                                   setLengthSB1( Length );
                                   startSB1();
                                   break;
                           case SB_20 :
                                   MixSpeed = setMixSB2( Speed );
                                   setLengthSB2( Length );
                                   startSB2();
                                   break;
                           case SB_pro :
                                   if ( flag_stereo == 1 )
                                      {
                                       MixSpeed = setMixSBpro( Speed );
                                       setLengthSBpro( Length );
                                       startSBpro();
                                      }
                                   else
                                      {
                                       MixSpeed = setMixSB2( Speed );
                                       setLengthSBpro( Length );
                                       startSBpro();
                                      }
                                   break;
                           case SB_16 :
                                   setMixSB16(Speed);
                                   startSB16( Length , flag_16bit, flag_stereo );
                                   break;
                          }
                    break;
         case WSS :                          // Windows Sound System
                    MixSpeed = setMixWSS( Speed );
                    setLengthWSS( Length );
                    startWSS();
                    break;
         case GUS :                          // Gravis Ultrasound
                    break;
        }
return;
}


void stopCard ()
{
 stopDMA( DMA );

 switch (CardType)
        {
         case NUL :                           // nothing
                    break;
         case SB  :                           // Soundblaster
                    switch ( SubCardType )
                           {
                            case SB_15 :
                                    stopSB1();
                                    break;
                            case SB_20 :
                                    stopSB2();
                                    break;
                            case SB_pro:
                                    stopSBpro();
                                    break;
                            case SB_16 :
                                    stopSB16();
                                    break;
                           }
                    break;
         case WSS :                          // Windows Sound System
                    stopWSS();
                    break;
         case GUS :                           // Gravis Ultrasound
                    break;
        }

}

void clearCard ()
{
 stopDMA( DMA );

 switch (CardType)
        {
         case NUL :                    // Silence
                    break;
         case SB  :                    // Soundblaster
                    closeSB ();
                    break;
         case WSS :                    // Windows Sound System
                    closeWSS ();
                    break;
         case GUS :                    // Gravis Ultrasound

                    break;
        }

 IRQClose();

 DMAfree(DMAselector);

return;
}

// *** This interrupt is calles everytime a IRQ from the card occours ***

void PlainInterupt ()
{
 switch (CardType)
        {
         case NUL :                    // Silence interrupt :)
                    break;
         case SB  :                    // Soundblaster interrupt
                    interrupt_SB ();
                    break;
         case WSS :                    // Windows Sound System interrupt
                    interrupt_WSS ();
                    break;
         case GUS :                    // Gravis Ultrasound interrupt
                    break;
         case 4   :
                    break;
        }

if ( Mixer_Block != 0)  Mixerpointer = Mixerpointer1;
    else                Mixerpointer = Mixerpointer2;


if ( flag_MixerReady == 1 )
   {
    if ( IRQ_counter < 3 )          // more then 2 IRQ run already, then return
       {
        if (flag_DatReady == 2)     // soundfile is at end and DMA must deleted
            clearDMAmem( clearvalue, Mixerpointer, buffer_length );

        if (flag_DatReady == 1)     // if soundfile is ready to play

        switch (DatType)
               {
                case 1:                     // RAW data type
                         break;

                case 2:                     // WAV data type
                         interrupt_WAV ();
                         break;
                case 3:                     // VOC data type
                         break;

                case 100:                   // MOD data type
                         interrupt_MOD ();
                         break;
                case 101:                   // S3M data type
                         interrupt_S3M ();
                         break;
                case 102:                   // 669 data type
                         interrupt_669 ();
                         break;
                case 103:                   // MTM data type
                         break;
                case 104:                   //
                         break;
               }

       }  // end, if IRQ_counter < 3
   }
}
