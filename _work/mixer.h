
// Mixer.h for F/X Player
// (C) 1997/98 by Apollo / STIGMA

#ifndef __MIXER_H
#define __MIXER_H

void writeMixer( );
void initDMAbuffer ( unsigned long buffer_length );
void initMixer (unsigned long MixerType);  // open mixer and reserve memory tables
void startMixer (unsigned long MixerType); // init mixer values given form just loadad file
void resetMixer ();           // reset runtime variables
void closeMixer ();           // close opened mixer

// Mixing rountines

void clearMixer ();

void DoPreMixing ( unsigned long MixLength );

void DoMixing ( unsigned long MixLength );
void Mix16Channel ( unsigned long channel, unsigned long counter , unsigned long MixPosition );
void Mix16IChannel ( unsigned long channel, unsigned long counter , unsigned long MixPosition );
void Mix16ICarry ( unsigned long channel, unsigned long Pos1, unsigned long Pos2, unsigned long Fraction, unsigned long MixPosition );

void Mix32Channel ( unsigned long channel, unsigned long counter , unsigned long MixPosition );
void Mix32IChannel ( unsigned long channel, unsigned long counter , unsigned long MixPosition );
void Mix32ICarry ( unsigned long channel, unsigned long Pos1, unsigned long Pos2, unsigned long Fraction, unsigned long MixPosition );

// *** global mixer variables ***

extern unsigned long  ChannelSeperation;   // seperation of channels
                                           // 256 = max, 0 = mono
extern unsigned long  GlobalVolume;        // global volume from 0 to 256
extern unsigned long  MasterVolume;        // master volume, is realtive
                                           // volume of every channel to mix

// *** global mixerchannel variables ***

extern unsigned long  ChannelUsed, ChannelLast;

extern unsigned long  ChannelActiv[256];          // 0 = quiet, 1 = activ, 2 = paused
extern unsigned long  ChannelFlag[256];           // 0 = direct, 1 = virtuell

extern unsigned long  ChannelSampleNr[256];
extern unsigned long  ChannelVolume[256];         // from 0 to 256
extern unsigned long  ChannelPanning[256];        // from 0 to 256, 128 is middle
extern unsigned long  ChannelSampleBits[256];     // 8 or 16 bit
extern unsigned long  ChannelSampleMode[256];     // 0 = mono, 2 = stereo sample
extern unsigned long  ChannelSampleAddress[256];   // start Address of current sample
extern unsigned long  ChannelSampleLength[256];   // position length of sample INCLUSIVE
extern unsigned long  ChannelLoopMode[256];       // 0 = no, 1 = forward, 2 = reverse
extern unsigned long  ChannelLoopBegin[256];      // position where loop starts
extern unsigned long  ChannelLoopEnd[256];        // position where loop ends
extern unsigned long  ChannelSamplePosition[256]; // relative offset in sample
extern unsigned long  ChannelSampleFraction[256]; // fraction of sample position
extern unsigned long  ChannelSampleFrequence[256];// frequence to play sample

extern   signed long  DeltaSamplePosition[256];   // delta to add on position
extern   signed long  DeltaSampleFraction[256];   // delta to add on fraction

// Assembler mixing routines

extern "C"
{
// ***** 16bit mixer *****

// mono not interpolated

void mix8Mto16M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8Mto16M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi]

void setup8Mto16M ( unsigned long VolAddress, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8Mto16M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

void mix16Mto16M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix16Mto16M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi]

void setup16Mto16M ( unsigned long Volume, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup16Mto16M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

// stereo not interpolated

void mix8Mto16S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8Mto16S parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi]

void setup8Mto16S ( unsigned long VolAddressLeft, unsigned long VolAddressRight, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8Mto16S parm [eax] [ebx] [edx] [ecx] [esi] modify exact [eax ebx ecx edx esi]

void mix16Mto16S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix16Mto16S parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi]

void setup16Mto16S ( unsigned long VolumeLeft, unsigned long VolumeRight, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup16Mto16S parm [eax] [ebx] [edx] [ecx] [esi] modify exact [eax ebx ecx edx esi]

// mono interpolated

void mix8MIto16M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8MIto16M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi ebp]

void setup8MIto16M ( unsigned long VolAddress, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8MIto16M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

void mix16MIto16M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix16MIto16M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi ebp]

void setup16MIto16M ( unsigned long Volume, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup16MIto16M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

// stereo interpolated

void mix8MIto16S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8MIto16S parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi ebp]

void setup8MIto16S ( unsigned long VolumeLeft, unsigned long VolumeRight, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8MIto16S parm [eax] [ebx] [edx] [ecx] [esi] modify exact [eax ebx ecx edx esi]

void mix16MIto16S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix16MIto16S parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi ebp]

void setup16MIto16S ( unsigned long VolumeLeft, unsigned long VolumeRight, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup16MIto16S parm [eax] [ebx] [edx] [ecx] [esi] modify exact [eax ebx ecx edx esi]

// ***** 32bit mixer *****

// mono not interpolated

void mix8Mto32M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8Mto32M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi]

void setup8Mto32M ( unsigned long VolAddress, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8Mto32M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

void mix16Mto32M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix16Mto32M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi]

void setup16Mto32M ( unsigned long Volume, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup16Mto32M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

// stereo not interpolated

void mix8Mto32S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8Mto32S parm [ecx] [esi] [edi] modify exact [eax ebx ecx edx esi edi]

void setup8Mto32S ( unsigned long VolAddressLeft, unsigned long VolAddressRight, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8Mto32S parm [eax] [ebx] [edx] [ecx] [esi] modify exact [eax ebx ecx edx esi]

void mix16Mto32S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix16Mto32S parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi]

void setup16Mto32S ( unsigned long VolumeLeft, unsigned long VolumeRight, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup16Mto32S parm [eax] [ebx] [edx] [ecx] [esi] modify exact [eax ebx ecx edx esi]

// mono interpolated

void mix8MIto32M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8MIto32M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi ebp]

void setup8MIto32M ( unsigned long VolAddress, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8MIto32M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

void mix16MIto32M ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix16MIto32M parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi ebp]

void setup16MIto32M ( unsigned long Volume, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup16MIto32M parm [eax] [ecx] [ebx] [edx] modify exact [eax ebx ecx edx]

// stereo interpolated

void mix8MIto32S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix8MIto32S parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi ebp]

void setup8MIto32S ( unsigned long VolumeLeft, unsigned long VolumeRight, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup8MIto32S parm [eax] [ebx] [edx] [ecx] [esi] modify exact [eax ebx ecx edx esi]

void mix16MIto32S ( unsigned long frac_counter, unsigned long Position, unsigned long Buffercounter);
#pragma aux mix16MIto32S parm [ecx] [esi] [edi] modify exact [eax ebx ecx esi edi ebp]

void setup16MIto32S ( unsigned long VolumeLeft, unsigned long VolumeRight, unsigned long highfraction, unsigned long lowfraction, unsigned long Bufferaddress);
#pragma aux setup16MIto32S parm [eax] [ebx] [edx] [ecx] [esi] modify exact [eax ebx ecx edx esi]

}
#endif
