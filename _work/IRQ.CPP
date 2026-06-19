// Interrupt routine for the F/X player in PMode
// (c) 1997-98 by Apollo / STIGMA
// got some ideas from IMS and OpenCP (thx Pascal)

#include <dos.h>
#include <i86.h>
#include <conio.h>
#include <stdio.h>

static unsigned char irqNum;
static unsigned char irqIntNum;
static unsigned char irqPort;
static void (__interrupt *irqOldInt)();
static unsigned char irqOldMask;
static void          (*irqRoutine)();
static unsigned char irqPreEOI;

volatile long IRQ_counter = 0;              // counter which counts IRQ nests

// from OpenCP

static char __far *stack;
static char __far *stackpointer;
static unsigned long stacksize = 16384;
static unsigned char stackused;
static void __far *oldssesp;

void stackcall(void *);
#pragma aux stackcall parm [eax] modify exact [eax ebx ecx edx esi edi ebp] = \
  "mov word ptr oldssesp+4,ss" \
  "mov dword ptr oldssesp+0,esp" \
  "lss esp,stack" \
  "sti" \
  "call eax" \
  "cli" \
  "lss esp,oldssesp"


void loades();
#pragma aux loades = "push ds" "pop es"

static void __interrupt irqInt()
{
  _disable();

  loades();

  IRQ_counter++;                            // normally it should be now 1

  if (irqPreEOI == 1)
  {
    outp(irqPort, inp(irqPort) | ( 1 << (irqNum&7)) );    // mask IRQ
    if (irqNum & 8)                                       // if IRQ > 7
        outp(0xA0,0x20);                                  // EOI of second IRQ controller
    outp(0x20,0x20);                                      // EOI of first IRQ controller

    if ( IRQ_counter < 4 )
       {
        if (!stackused)
           {
            stackused++;
            stackcall( irqRoutine );   // call service routine : Soundcard, Mixer etc
            stackused--;
           }
       }

    outp(irqPort, inp(irqPort) &~ ( 1 << (irqNum&7)) );   // unmask IRQ
  }
  else
  {
    if ( IRQ_counter < 4 )           // if the 4th unfinished IRQ is running
       {
        if (!stackused)
           {
            stackused++;
            stackcall( irqRoutine );   // call service routine : Soundcard, Mixer etc
            stackused--;
           }
       }
                                             // ignore this one
    if (irqNum & 8)
        outp(0xA0, 0x20);                    // EOI of second PIC
    outp(0x20, 0x20);                        // EOI of first PIC
  }

  IRQ_counter--;                            // normally it should be now 0

  _enable();
}

/*
static void *getvect(unsigned char NrInt)
{
 REGS r;

 r.w.ax=0x204;
 r.h.bl=NrInt;
 int386(0x31, &r, &r);
 return MK_FP(r.w.cx, r.x.edx);
}


static void setvect(unsigned char NrInt, void *vect)
{
 REGS r;

 r.w.ax=0x205;
 r.h.bl=NrInt;
 r.x.edx=FP_OFF(vect);
 r.w.cx=FP_SEG(vect);
 int386(0x31, &r, &r);
}
*/

long setNewIRQ (unsigned char inum, void (*routine)(), unsigned char preEOI)
{
  stackpointer = new char [stacksize];
  if ( stackpointer == NULL )   return 1;
  stack = stackpointer+stacksize;

  if ( routine == NULL )  return 1;
  irqRoutine = routine;
  irqPreEOI = preEOI;
  if (inum > 15)  return 1;
  irqNum = inum;
  irqIntNum = (inum & 8) ? (inum+0x68) : (inum+8);
  irqPort = (inum & 8) ? 0xA1 : 0x21;

//  irqOldInt = getvect(irqIntNum);
  irqOldInt = _dos_getvect( irqIntNum );

  irqOldMask = inp(irqPort) & (1 << (inum&7));

  outp(irqPort, inp(irqPort) & (1 << (inum&7)));

//  setvect(irqIntNum, irqInt);
  _dos_setvect( irqIntNum, irqInt );

  outp(irqPort, inp(irqPort) &~ (1 << (inum&7)));

return 0;
}


void IRQClose()
{
  outp(irqPort, inp(irqPort) | irqOldMask);
//setvect(irqIntNum, irqOldInt);
  _dos_setvect( irqIntNum, irqOldInt );
  delete (char near *)(stackpointer);
}


void IRQReset()
{
  outp(irqPort, inp(irqPort) &~ (1 << (irqNum&7)) );
}
