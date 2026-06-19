// Convert.h for F/X Player
// (C) 1997 by Apollo / STIGMA

#ifndef __CONVERT_H
#define __CONVERT_H

void ConvU8MtoS8M ( char *source, unsigned long length );
void ConvU16MtoS16M ( char *source, unsigned long length );

void Conv8Mto8M ( char *source, char *target, unsigned long length );
void Conv8Mto8S ( char *source, char *target, unsigned long length );
void Conv8Mto16M ( char *source, char *target, unsigned long length );
void Conv8Mto16S ( char *source, char *target, unsigned long length );

void Conv8Sto8M ( char *source, char *target, unsigned long length );
void Conv8Sto8S ( char *source, char *target, unsigned long length );
void Conv8Sto16M ( char *source, char *target, unsigned long length );
void Conv8Sto16S ( char *source, char *target, unsigned long length );

void Conv16Mto8M ( char *source, char *target, unsigned long length );
void Conv16Mto8S ( char *source, char *target, unsigned long length );
void Conv16Mto16M ( char *source, char *target, unsigned long length );
void Conv16Mto16S ( char *source, char *target, unsigned long length );

void Conv16Sto8M ( char *source, char *target, unsigned long length );
void Conv16Sto8S ( char *source, char *target, unsigned long length );
void Conv16Sto16M ( char *source, char *target, unsigned long length );
void Conv16Sto16S ( char *source, char *target, unsigned long length );

extern "C"
{
}
#endif
