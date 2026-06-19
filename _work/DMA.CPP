#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>

#include "fx.h"


static void *dosmalloc(unsigned long len, __segment &pmsel)
{
 REGS r;

 len=(len+15) >> 4;               // round length up to next paragraph
 r.w.ax=0x100;                    // DPMI func. 0x100 = allocate DOS memory
 r.w.bx=len;                      // bx = length in paragraphs
 int386(0x31, &r, &r);            // call DPMI int 0x31

 if (r.x.cflag)  return 0;        // if carry then error

 pmsel=r.w.dx;                    // dx has selector needed later for free up
 return (void *)((unsigned long)r.w.ax<<4);       // return 32bit pointer
}


static void dosfree(__segment pmsel)
{
 REGS r;

 r.w.ax=0x101;               // DPMI function 0x101 = free DOS memory
 r.w.dx=pmsel;               // dx = selector to free up
 int386(0x31, &r, &r);       // call int 0x31
}


// *** DMA memory routines ***

char *DMAalloc(unsigned long buflen, __segment &pmsel)
{
 char *ptr;
 unsigned long a;

 if (buflen > 0x20000)  buflen=0x20000;

 ptr=(char *)dosmalloc(buflen << 1, pmsel);
 if (ptr == NULL)  return 0;

 a = 0x10000 - ( (unsigned long)ptr & 0xFFFF );

 if ( a < buflen )
    {
      return (char *)( ptr+a );    // gotcha!
    }

 return ((char *)ptr);
}


void DMAfree(__segment pmseg)
{
  dosfree(pmseg);
}


// *** DMA programming datas ***

uns short DMA_Address[] = { 0x00, 0x02, 0x04, 0x06,  0xc0, 0xc4, 0xc8, 0xcc };
uns short DMA_Count[]   = { 0x01, 0x03, 0x05, 0x07,  0xc2, 0xc6, 0xca, 0xce };
uns short DMA_Page[]    = { 0x87, 0x83, 0x81, 0x82,  0x88, 0x8b, 0x89, 0x8a };
uns short DMA_UPage[]   = { 0x487,0x483,0x481,0x482, 0x488,0x48b,0x489,0x48a };

uns char DMA_Status[]   = { 0x08, 0xd0 };
uns char DMA_Command[]  = { 0x08, 0xd0 };
uns char DMA_Request[]  = { 0x09, 0xd2 };
uns char DMA_ChMask[]   = { 0x0a, 0xd4 };
uns char DMA_Mode[]     = { 0x0b, 0xd6 };
uns char DMA_FlipFlop[] = { 0x0c, 0xd8 };
uns char DMA_CLR[]      = { 0x0d, 0xda };
uns char DMA_MaskCLR[]  = { 0x0e, 0xdc };
uns char DMA_Mask[]     = { 0x0f, 0xde };


//uns char DMAmode=8+16+64;   // = 010110xx
                            // bit 0+1 : channel Nr
                            // bit 2+3 : 00 = verify
                            //           01 = writing (into mem)
                            //           10 = reading (from mem)
                            //           11 = not valid
                            // bit 4   : 0 = Autoinit off
                            //           1 = Autoinit on
                            // bit 5   : 0 = increment Address
                            //           1 = decrement Address
                            // bit 6+7 : 00 = Demand mode
                            //           01 = Single mode
                            //           10 = Block mode
                            //           11 = kaskade mode


void initDMA( unsigned char DMAChannel, unsigned char DMAmode, char *DMAAddress, unsigned long DMAlength)
{
 unsigned char DMAhigh;         // 0 = DMA 0-3, 1 = DMA 4-7

 DMAhigh = DMAChannel >> 2;
 if ( DMAhigh == 1 )  DMAlength >>= 1;

 outp( DMA_ChMask[DMAhigh], DMAChannel | 4 );
                                               // mask channel out
 outp( DMA_Mode[DMAhigh], DMAChannel & 3 | DMAmode );
                                               // select DMA channel and
                                               // write mode
 outp( DMA_FlipFlop[DMAhigh], 0 );
                                               // clear flipflop
 outp( DMA_Address[DMAChannel], ( ((uns long)DMAAddress) >> DMAhigh) & 255 );
 outp( DMA_Address[DMAChannel], (( ((uns long)DMAAddress) >> DMAhigh) >> 8) & 255 );

 outp( DMA_Page[DMAChannel], ( ((uns long)DMAAddress) >> 16) & 255 );
// outp( DMA_UPage[DMAChannel], ( ((uns long)DMAAddress) >> 24) & 255 );
                                      // Upper Page Register
                                      // some older motherboards have problems
                                      // with this port... :(

 outp( DMA_Count[DMAChannel], (DMAlength-1) & 255 );
 outp( DMA_Count[DMAChannel], ((DMAlength-1) >> 8) & 255 );
                                     // load DMA counter with lo-byte length-1
                                     // load DMA counter with hi-byte length-1

 outp( DMA_ChMask[DMAhigh], DMAChannel & 3 );
                                     // mask channel in again :)
}


void stopDMA( unsigned char DMAChannel )
{
 outp( DMA_FlipFlop[DMAChannel >> 2], 0 );
 outp( DMA_ChMask[DMAChannel >> 2], DMAChannel | 4 );
}


void clearDMAmem( unsigned long value, char *DMAAddress, unsigned long DMAlength)
{
 static unsigned long counter;

 for (counter = 0; counter < DMAlength; counter += 2)
      *(unsigned short *)(DMAAddress+counter) = value;
}

