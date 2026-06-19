// Device_WSS.h for F/X Player
// (C) 1998 by Apollo / STIGMA

#ifndef __DEV_WSS_H
#define __DEV_WSS_H

unsigned short checkWSS (unsigned short WSS_Address);
void initWSS ( unsigned short WSS_Address );
void closeWSS ( );

unsigned long setMixWSS ( unsigned long WSS_Speed );
void setLengthWSS ( unsigned long WSS_length );
void startWSS ( );
void stopWSS ( );

void interrupt_WSS ();  // interupt which is called everytime

extern "C"
{
}
#endif
