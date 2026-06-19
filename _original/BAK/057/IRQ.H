// IRQ.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __IRQ_H
#define __IRQ_H

extern volatile long IRQ_counter;

long setNewIRQ(unsigned char IRQnumber, void far (*routine)(), unsigned char preEOI);
void IRQClose ();
void IRQReset ();

extern "C"
{
}
#endif
