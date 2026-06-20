// dev_wav.h for F/X Player
// WAV file output device (card type 4)
// (C) 1998 by Apollo / STIGMA

#ifndef __DEV_WAV_H
#define __DEV_WAV_H

extern char           wav_outfile[255];
extern unsigned long  wav_max_seconds;
extern unsigned long  wav_data_bytes;

long  initWAV  (unsigned long buf_len);
void  closeWAV ();
void  writeWAV ();          // write current Mixerpointer block to file

extern "C"
{
}
#endif
