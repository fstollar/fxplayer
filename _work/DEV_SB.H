// Device_SB.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __DEV_SB_H
#define __DEV_SB_H

unsigned char reset_SB (unsigned long SB_Address);
unsigned long getDSPversion();
unsigned long checkSB (unsigned long SB_Address);
void initSB (unsigned long SB_Address);
void closeSB ();

void interrupt_SB ();  // interupt which is called everytime

// SB 1.0

unsigned long setMixSB1 (unsigned long SB_MixSpeed);
void setLengthSB1 (unsigned long SB_blocklength);
void startSB1 ();
void stopSB1 ();

// SB 2.01

unsigned long setMixSB2 (unsigned long SB_MixSpeed);
void setLengthSB2 (unsigned long SB_blocklength);
void startSB2 ();
void stopSB2 ();

// SB pro

unsigned long setMixSBpro (unsigned long SB_MixSpeed);
void setLengthSBpro (unsigned long SB_blocklength);
void startSBpro ();
void stopSBpro ();

// SB 16

void setMixSB16 (unsigned long SB_MixSpeed);
void startSB16 (unsigned long SB_blocklength, unsigned char flag_16bit, unsigned char flag_stereo );
void stopSB16 ();

extern "C"
{
}
#endif
