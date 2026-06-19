// Device_SB.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __DEV_SB_H
#define __DEV_SB_H

unsigned char reset_SB (unsigned short SB_Adress);
unsigned short getDSPversion();
unsigned short checkSB (unsigned short SB_Adress);

void initSB (unsigned short SB_Adress);
void closeSB ();

void interupt_SB ();  // interupt which is called everytime

// SB 1.0

unsigned long setMixSB1 (unsigned long SB_MixSpeed);
void setLengthSB1 (unsigned short SB_blocklength);
void startSB1 ();
void stopSB1 ();

// SB 2.01

unsigned long setMixSB2 (unsigned long SB_MixSpeed);
void setLengthSB2 (unsigned short SB_blocklength);
void startSB2 ();
void stopSB2 ();

// SB pro

unsigned long setMixSBpro (unsigned long SB_MixSpeed);
void setLengthSBpro (unsigned short SB_blocklength);
void startSBpro ();
void stopSBpro ();

// SB 16

void setMixSB16 (unsigned long SB_MixSpeed);
void startSB16 (unsigned short SB_blocklength, unsigned short flag_16bit, unsigned short flag_stereo );
void stopSB16 ();

extern "C"
{
}
#endif
