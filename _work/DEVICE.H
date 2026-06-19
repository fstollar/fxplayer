// Device.h for F/X Player
// (C) 1997-98 by Apollo / STIGMA

#ifndef __DEVICE_H
#define __DEVICE_H

void PlainInterupt ();

void writeCard( );
long checkCard ( unsigned long CardType, unsigned long Address );
long initCard ( );
void startCard ( unsigned long Length, unsigned long MixSpeed);
void stopCard ( );
void clearCard ( );

extern "C"
{
}
#endif
