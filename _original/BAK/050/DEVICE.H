// Device.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __DEVICE_H
#define __DEVICE_H

void PlainInterupt ();

short checkCard ( unsigned short CardType, unsigned short Adress );
short initCard ( unsigned short CardType, unsigned short Adress, unsigned short IRQ, unsigned short DMAlo, unsigned short DMAhi, unsigned short flag_16bit, unsigned short flag_stereo, unsigned long buffer_length);
void startCard ( unsigned short CardType, unsigned short SubCardType, unsigned short flag_16bit, unsigned short flag_stereo, unsigned long Length, unsigned long MixSpeed);
void stopCard ( void );
void clearCard ( void );


extern "C"
{
}
#endif
