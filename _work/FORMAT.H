// FORMAT.h for F/X Player
// (C) 1997/98 by Apollo / STIGMA

#ifndef __FORMAT_H
#define __FORMAT_H

long test_File ( char *FileName );
long load_Format ( char *FileName, unsigned long DatType);
long close_Format ();

long FF_Format ();
long FR_Format ();

extern "C"
{
}
#endif
