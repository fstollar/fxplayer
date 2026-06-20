// DMA.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __DMA_H
#define __DMA_H

char *DMAalloc( unsigned long buflen, __segment &pmsel);
void DMAfree( __segment pmseg );
void initDMA( unsigned char DMAChannel, unsigned char DMAmode, char *DMAAddress, unsigned long DMAlength);
void stopDMA( unsigned char DMAChannel );
void clearDMAmem( unsigned long value, char *DMAAddress, unsigned long DMAlength);

extern "C"
{
}
#endif
