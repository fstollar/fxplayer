// Mixer.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __MIXER_H
#define __MIXER_H


void initDMAbuffer ( unsigned long buffer_length );
void initMixer (unsigned short MixerType);  // open mixer and reserve memory tables
void startMixer (unsigned short MixerType);
void resetMixer ();           // reset runtime variables
void closeMixer ();           // close opened mixer


// Mixing rountines

void DoMixing ( unsigned long MixLength );

void clearMixer ();
void Do16Mixing ( unsigned long MixLength );

void Mix16Channel ( unsigned long channel, unsigned long length , unsigned long MixPosition );


// Master Volume routines

void calcVol8for16 ( char *VolPointer );

void calcMasterVol8for16 ( unsigned short MasterVolume, char *Pointer );
void calcMasterVol16for16 ( unsigned short MasterVolume, char *Pointer );
void MasterVol8for16 ( char *Source, char *Destin, unsigned long count);
void MasterVol16for16 ( char *Source, char *Destin, unsigned long count);

void DoMasterVolumeCalculations ( char *Source, char *Destin, unsigned long count);


// memory modifier routines

void set8M ( char *target, char value, unsigned long length );
void set16M ( char *target, unsigned short value, unsigned long length );

extern char *MasterVolumeTable;
extern char *Voltable8;

// *** global mixer variables ***

extern unsigned short ChannelSeperation;   // seperation of channels
                                           // 256 = max, 0 = mono
extern unsigned short GlobalVolume;        // global volume from 0 to 256
extern unsigned short MasterVolume;        // master volume, is realtive
                                           // volume of every channel to mix

// *** global mixerchannel variables ***

extern unsigned short ChannelUsed, ChannelLast;

extern unsigned char  ChannelActiv[256];          // 0 = quiet, 1 = activ, 2 = paused
extern unsigned char  ChannelFlag[256];           // 0 = direct, 1 = virtuell

extern unsigned short ChannelSampleNr[256];
extern unsigned short ChannelVolume[256];         // from 0 to 256
extern unsigned short ChannelPanning[256];        // from 0 to 256, 128 is middle
extern unsigned char  ChannelSampleBits[256];     // 8 or 16 bit
extern unsigned char  ChannelSampleMode[256];     // 1 = mono, 2 = stereo sample
extern unsigned long  ChannelSampleAdress[256];   // start adress of current sample
extern unsigned long  ChannelSampleLength[256];   // position length of sample INCLUSIVE
extern unsigned char  ChannelLoopMode[256];       // 0 = no, 1 = loop, 2 = pingpong
extern unsigned long  ChannelLoopBegin[256];      // position where loop starts
extern unsigned long  ChannelLoopEnd[256];        // position where loop ends
extern unsigned long  ChannelSamplePosition[256]; // relative offset in sample
extern unsigned short ChannelSampleFraction[256]; // fraction of sample position
extern unsigned long  ChannelSampleFrequence[256];// frequence to play sample

extern unsigned long  DeltaSamplePosition[256];   // delta to add on position
extern unsigned short DeltaSampleFraction[256];   // delta to add on fraction

// Assembler mixing routines

extern "C"
{
// mono output

void mix8Mto16M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8Mto16M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi]

void setup8Mto16M ( unsigned long VolAdress, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8Mto16M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

// stereo output

void mix8Mto16S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffer);
#pragma aux mix8Mto16S parm [edx] [esi] [edi] modify exact [eax ebx edx esi edi]

void setup8Mto16S ( unsigned long VolAdressLeft, unsigned long VolAdressRight, unsigned long highfraction, unsigned long lowfraction);
#pragma aux setup8Mto16S parm [eax] [edx] [ecx] [ebx] modify exact [eax ebx ecx]

void mix16Mto16S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffer);
#pragma aux mix16Mto16S parm [edx] [esi] [edi] modify exact [eax ebx edx esi edi]

void setup16Mto16S ( unsigned long VolAdressLeft, unsigned long VolAdressRight, unsigned long highfraction, unsigned long lowfraction);
#pragma aux setup16Mto16S parm [eax] [edx] [ecx] [ebx] modify exact [eax ebx ecx]

}
#endif
