// DAT_Calculations.h for F/X Player
// (C) 1997-98 by Apollo / STIGMA

#ifndef __DAT_CALC_H
#define __DAT_CALC_H

extern unsigned long DAT_calcSpeed ( unsigned long Tempo, unsigned long MixSpeed );

extern volatile unsigned long Division;
extern volatile unsigned long Div_Fraction;

extern unsigned long divide_64bit( unsigned long Position, unsigned long Fraction, unsigned long DivPosition, unsigned long DivFraction);
#pragma aux divide_64bit parm [eax] [edx] [ebx] [ecx] value [eax] modify exact [eax ebx ecx edx] = \
    "shl ebx,16"\
    "mov bx,cx"\
    "mov ecx,edx"\
    "xor edx,edx"\
    "shld edx,eax,16"\
    "shl eax,16"\
    "mov ax,cx"\
    "div ebx"\
    "mov Division,eax"\
    "mov Div_Fraction,edx";

extern "C"
{
}
#endif
