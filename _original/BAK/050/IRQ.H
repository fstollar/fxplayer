// IRQ.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __IRQ_H
#define __IRQ_H

extern volatile short IRQ_counter;

short setNewIRQ(unsigned char IRQnumber, void (*routine)(), unsigned char preEOI);
void IRQClose ();
void IRQReset ();

extern "C"
{
}
#endif
