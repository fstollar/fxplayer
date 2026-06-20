// dev_wav.cpp for F/X Player
// WAV file output device (card type 4)
// (C) 1998 by Apollo / STIGMA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fx.h"
#include "dev_wav.h"

char          wav_outfile[255]  = "OUTPUT.WAV";
unsigned long wav_max_seconds   = 0;
unsigned long wav_data_bytes    = 0;

static FILE *wav_fp = NULL;

static void write_u16le(FILE *f, unsigned short v)
{
    fputc( v        & 0xFF, f);
    fputc((v >> 8)  & 0xFF, f);
}

static void write_u32le(FILE *f, unsigned long v)
{
    fputc( v        & 0xFF, f);
    fputc((v >>  8) & 0xFF, f);
    fputc((v >> 16) & 0xFF, f);
    fputc((v >> 24) & 0xFF, f);
}

long initWAV(unsigned long buf_len)
{
    unsigned short channels    = (unsigned short)(flag_stereo ? 2 : 1);
    unsigned short bits        = (unsigned short)(flag_16bit  ? 16 : 8);
    unsigned short block_align = channels * (bits / 8);
    unsigned long  byte_rate   = MixSpeed * block_align;

    wav_fp = fopen(wav_outfile, "wb");
    if (!wav_fp) return 1;

    // Plain heap buffer — no DMA needed for WAV output
    DMApointer = (char *)malloc(buf_len);
    if (!DMApointer) { fclose(wav_fp); wav_fp = NULL; return 2; }

    // Write placeholder 44-byte RIFF/WAV header; sizes patched in closeWAV()
    fwrite("RIFF", 1, 4, wav_fp);
    write_u32le(wav_fp, 0);            // total file size - 8 (patched)
    fwrite("WAVE", 1, 4, wav_fp);
    fwrite("fmt ", 1, 4, wav_fp);
    write_u32le(wav_fp, 16);           // PCM fmt chunk is always 16 bytes
    write_u16le(wav_fp, 1);            // audio format: PCM
    write_u16le(wav_fp, channels);
    write_u32le(wav_fp, MixSpeed);
    write_u32le(wav_fp, byte_rate);
    write_u16le(wav_fp, block_align);
    write_u16le(wav_fp, bits);
    fwrite("data", 1, 4, wav_fp);
    write_u32le(wav_fp, 0);            // data chunk size (patched)

    wav_data_bytes = 0;
    return 0;
}

void closeWAV()
{
    if (!wav_fp) return;

    // Patch RIFF chunk size (offset 4) and data chunk size (offset 40)
    fseek(wav_fp, 4L, SEEK_SET);
    write_u32le(wav_fp, 36 + wav_data_bytes);
    fseek(wav_fp, 40L, SEEK_SET);
    write_u32le(wav_fp, wav_data_bytes);

    fclose(wav_fp);
    wav_fp = NULL;

    free(DMApointer);
    DMApointer = NULL;
}

void writeWAV()
{
    if (!wav_fp || !Mixerpointer) return;
    fwrite(Mixerpointer, 1, buffer_length, wav_fp);
    wav_data_bytes += buffer_length;
}
