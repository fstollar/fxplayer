#if !defined(__M_TOOL_H)
#define __M_TOOL_H

#include <conio.h>
#include <stdio.h>

extern void putchr(int x,int y,int zeichen,char attrib);
extern void putstring(int x,int y,void *string,char attrib);
extern void _fputstring(int x,int y,void far *string,char attrib);

// Konstanten fr Procedure fenster
#define einfach  "ÚÄ¿³ÙÄÀ³"
#define doppelt  "ÉÍ»º¼ÍÈº"
#define castle1  "şßşŞşÜşİ"
#define castle2  "şÜşİşßşŞ"
// Konstanten fr Procedure cursor
#define block       0x000f
#define unsichtbar  0xfeff
#define strich      0x0607

void fenster(int x1,int y1,int x2,int y2,char rahmen[8],int farbe)
{
 unsigned int i, t;
 /* Waagrecht oben*/
 putchr(x1,y1,rahmen[0],farbe);
 for (i=1; i<x2-x1; i++) putchr(x1+i,y1,rahmen[1],farbe);
 putchr(x2,y1,rahmen[2],farbe);
 /*Senkrecht*/
 for (i=1; i<y2-y1; i++)
 {
   putchr(x1,y1+i,rahmen[7],farbe);
   for (t=x1+1; t<x2; t++) putchr(t,y1+i,' ',farbe);
   putchr(x2,y1+i,rahmen[3],farbe);
 }
 /* Waagrecht unten*/
 putchr(x1,y2,rahmen[6],farbe);
 for (i=1; i<x2-x1; i++) putchr(x1+i,y2,rahmen[5],farbe);
 putchr(x2,y2,rahmen[4],farbe);
}

void w_open(int x1,int y1,int x2,int y2,char rahmen[8],int farbe)
{
  fenster(x1,y1,x2,y2,rahmen,farbe);
}

void  w_close(void)
{
}

void cursor(unsigned int kind)
{
 asm {
     mov    cx,[kind]
     mov    ax,0x0100
     int    16
     }
}
#endif