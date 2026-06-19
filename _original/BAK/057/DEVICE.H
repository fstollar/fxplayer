// Device.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __DEVICE_H
#define __DEVICE_H

void far PlainInterupt ();

void writeCard( );
short checkCard ( unsigned short CardType, unsigned short Address );
short initCard ( );
void startCard ( unsigned long Length, unsigned long MixSpeed);
void stopCard ( );
void clearCard ( );

extern "C"
{
}
#endif
