// DMA.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __DMA_H
#define __DMA_H

char *DMAalloc(unsigned long buflen, __segment &pmsel);
void DMAfree(__segment pmseg);
void initDMA( unsigned char DMAChannel, unsigned char DMAmode, char *DMAadress, unsigned short DMAlength);
void stopDMA( unsigned char DMAChannel );
void clearDMAmem( unsigned short value, char *DMAadress, unsigned short DMAlength);


extern "C"
{
}
#endif
