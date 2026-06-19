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

#include "format.h"
#include "dat_wav.h"
#include "dat_s3m.h"

//#include "convert.h"
//#include "mixer.h"

// *** Variables ***

unsigned short clearvalue = 0;

// ******************* Sound Devices for the F/X Player *********************

short checkCard ( unsigned short CardType, unsigned short Adress)
{

 switch( CardType )
        {
         case 1 :                          // Soundblaster
                  test=checkSB(Adress);
                  if (test==0)  return 1;
                  SubCardType=test;
                  DSPVersion=getDSPversion();
                  break;
         case 2 :                          // Gravis Ultrasound

                  break;
        }

return 0;
}


short initCard ( unsigned short CardType, unsigned short Adress, unsigned short IRQ, unsigned short DMAlo, unsigned short DMAhi, unsigned short flag_16bit, unsigned short flag_stereo, unsigned long buffer_length)
{
 unsigned long realDMAmem;

 switch( CardType )
        {
         case 1 :                           // Soundblaster

                  initSB(Adress);

                  test=setNewIRQ(IRQ, PlainInterupt, 0);
                  if (test)
                     {
                      puts("Interupt Fehler !!\n");
                      return 1;
                     }

                  realDMAmem = buffer_length << 2;
                  DMApointer=(char *)DMAalloc( realDMAmem , DMAselector );
                  if (DMApointer==NULL)
                      {
                       puts("DMA Fehler !!\n");
                       return 2;
                      }

                  if (flag_16bit==0)
                     {
                      DMA=DMAlo;
                      clearvalue = 128*256+128;
                     }
                  else
                     {
                      DMA=DMAhi;
                      clearvalue = 32768;
                     }

                  initDMA( DMA, 8+16+64, DMApointer, (buffer_length << 1) >> flag_16bit );
                  clearDMAmem( clearvalue, DMApointer, buffer_length << 1 );

                  break;

         case 2 :                          // Gravis Ultrasound

                  break;
        }


return 0;
}

void startCard ( unsigned short CardType, unsigned short SubCardType, unsigned short flag_16bit, unsigned short flag_stereo, unsigned long Length, unsigned long MixSpeed)
{

 switch( CardType )
        {
         case 1 :                          // Soundblaster
                  switch( SubCardType )
                        {
                         case 1:
                                 MixSpeed=setMixSB1( MixSpeed );
                                 setLengthSB1( Length );
                                 startSB1();
                                 break;
                         case 2:
                                 MixSpeed=setMixSB2( MixSpeed );
                                 setLengthSB2( Length );
                                 startSB2();
                                 break;
                         case 3:
                         case 4:
                                 MixSpeed=setMixSBpro( MixSpeed );
                                 setLengthSBpro( Length );
                                 startSBpro();
                                 break;
                         case 5:
                                 setMixSB16(MixSpeed);
                                 startSB16( (Length >> flag_16bit) , flag_16bit, flag_stereo );
                                 break;
                        }
                  break;
         case 2 :                          // Gravis Ultrasound

                  break;
        }


return;
}

void stopCard ()
{
 stopDMA( DMA );

 switch (CardType)
        {
         case 0:
                 break;
         case 1:                    // Soundblaster
                 switch ( SubCardType )
                        {
                         case 1:
                                 stopSB1();
                                 break;
                         case 2:
                                 stopSB2();
                                 break;
                         case 3:
                         case 4:
                                 stopSBpro();
                                 break;
                         case 5:
                                 stopSB16();
                                 break;
                        }
                 break;

         case 2:                    // Gravis Ultrasound
                 break;

         case 3:                    // Windows Sound System
                 break;

         case 4:
                 break;
        }

}

void clearCard ()
{
 stopDMA( DMA );

 switch (CardType)
        {
         case 0:                    // Silence
                 break;
         case 1:                    // Soundblaster
                 closeSB ();
                 //reset_SB ( Adress );
                 break;

         case 2:                    // Gravis Ultrasound
                 break;

         case 3:                    // Windows Sound System
                 break;

         case 4:
                 break;
        }

 IRQClose();

 DMAfree(DMAselector);

return;
}

// *** This interrupt is calles everytime a IRQ from the card occours ***

void PlainInterupt ()
{

 if ( IRQ_counter < 4 )             // if more than 3 IRQ running, return

 switch (CardType)
        {
         case 0:                    // Silence interrupt :)
                 break;
         case 1:                    // Soundblaster interrupt
                 interupt_SB ();
                 break;

         case 2:                    // Gravis Ultrasound interrupt
                 break;

         case 3:                    // Windows Sound System interrupt
                 break;

         case 4:
                 break;
        }

if (Mixer_Block != 0)  Mixerpointer = Mixerpointer1;
   else                Mixerpointer = Mixerpointer2;

if ( IRQ_counter < 3 )        // more then 2 IRQ run already, then return
   {

    if (flag_DatReady==2)   clearDMAmem(clearvalue, DMApointer, buffer_length*2 );

    if (flag_DatReady==1)   // if data type is ready to play

    switch (DatType)
           {
            case 1:                    // RAW data type
                     break;

            case 2:                    // WAV data type
                     interupt_WAV();
                     break;

            case 3:                    // VOC data type
                     break;

            case 100:                   // MOD data type
                     break;

            case 101:                   // S3M data type
                     interupt_S3M();
                     break;

            case 102:                   // 669 data type
                     break;

            case 103:                   // MTM data type
                     break;

            case 104:                   //
                     break;
           }

   }  // end, if IRQ_counter < 3
}
