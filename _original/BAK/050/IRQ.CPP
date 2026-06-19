#include <stdlib.h>
#include <i86.h>
#include <conio.h>

static unsigned char irqNum;
static unsigned char irqIntNum;
static unsigned char irqPort;
static void far *irqOldInt;
static unsigned char irqOldMask;
static void (*irqRoutine)();
static unsigned char irqPreEOI;

unsigned short _ourss;     // big THX to KARL for his help on IRQ handling
unsigned short _ourds;
unsigned short _oures;
unsigned short _ourfs;
unsigned short _ourgs;
unsigned short _backss;
unsigned short _backds;
unsigned short _backes;
unsigned short _backfs;
unsigned short _backgs;
unsigned long  _backesp;
unsigned long  _backebp;
unsigned short stacklength=20*1024;
void *ourstack;

void backupPMstack(void);
#pragma aux backupPMstack = \
    "mov _ourss,ss" \
    "mov _ourds,ds"\
    "mov _oures,es"\
    "mov _ourfs,fs"\
    "mov _ourgs,gs"\
    modify exact [];

void restorePM(void);
#pragma aux restorePM= \
    "mov es,_oures"\
    "mov fs,_ourfs"\
    "mov gs,_ourgs"\
    "mov ebp,ourstack"\
    "mov esp,ourstack"\
    "mov ss,_ourss"\
    //"lss esp,ourstack"\
    "mov ds,_ourds"\
    modify exact [];


void backupCALLERstack(void);
#pragma aux backupCALLERstack = \
    "cli"\
    "mov _backesp,esp"\
    "mov _backss,ss" \
    "mov _backds,ds"\
    "mov _backes,es"\
    "mov _backfs,fs"\
    "mov _backgs,gs"\
    "mov _backebp,ebp"\
    modify exact [];

void restoreCALLERstack(void);
#pragma aux restoreCALLERstack = \
    "mov esp,_backesp"\
    "mov ss,_backss"\
    "mov ebp,_backebp"\
    "mov es,_backes"\
    "mov fs,_backfs"\
    "mov gs,_backgs"\
    "mov ds,_backds"\
    modify exact [];

volatile short IRQ_counter = 0;  // counter which counts IRQ nests

static void __interrupt irqInt()
{
  //backupCALLERstack();
  //restorePM();

  IRQ_counter++;                // normally it should be now 1

  if (irqPreEOI)
  {

    outp(irqPort, inp(irqPort)|(1<<(irqNum&7)));
    if (irqNum&8)
      outp(0xA0,0x20);
    outp(0x20,0x20);
    if ( IRQ_counter < 4 )  irqRoutine();
    outp(irqPort, inp(irqPort) &~ (1<<(irqNum&7)));

  }
  else
  {
    if ( IRQ_counter < 4 )  irqRoutine();   // if the 4th IRQ is running ignore this one
    if (irqNum&8)
      outp(0xA0,0x20);
    outp(0x20,0x20);
  }

  IRQ_counter--;              // normally it should be now 0

  //restoreCALLERstack();
}


static void far *getvect(unsigned char NrInt)
{
 REGS r;

 r.w.ax=0x204;
 r.h.bl=NrInt;
 int386(0x31, &r, &r);
 return MK_FP(r.w.cx, r.x.edx);
}


static void setvect(unsigned char NrInt, void far *vect)
{
 REGS r;

 r.w.ax=0x205;
 r.h.bl=NrInt;
 r.x.edx=FP_OFF(vect);
 r.w.cx=FP_SEG(vect);
 int386(0x31, &r, &r);
}


short setNewIRQ(unsigned char inum, void (*routine)(), unsigned char preEOI)
{
  backupPMstack();

  irqPreEOI=preEOI;
  if (inum > 15) return 1;
  inum &= 15;
  irqNum = inum;
  irqIntNum = (inum&8) ? (inum+0x68) : (inum+8);
  irqPort = (inum&8) ? 0xA1 : 0x21;

  irqOldInt = getvect(irqIntNum);
  irqOldMask = inp(irqPort) & (1 << (inum&7));

  irqRoutine=routine;
  setvect(irqIntNum, irqInt);
  outp(irqPort, inp(irqPort) &~ (1 << (inum&7)));

  ourstack = malloc(stacklength);

return 0;
}


void IRQClose()
{
  outp(irqPort, inp(irqPort)|irqOldMask);
  setvect(irqIntNum, irqOldInt);

  free( ourstack );
}


void IRQReset()
{
  outp(irqPort, inp(irqPort) &~ (1 << (irqNum&7)) );
}
