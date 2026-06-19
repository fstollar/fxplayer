#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>

#include "fx.h"

//#include "irq.h"
//#include "dma.h"

//#include "device.h"
//#include "dev_sb.h"

//#include "format.h"
#include "dat_calc.h"
//#include "dat_wav.h"
//#include "dat_s3m.h"

//#include "convert.h"
//#include "mixer.h"

// ********************* Calculations of different formats ******************


unsigned long DAT_calcSpeed ( unsigned long Tempo, unsigned long MixSpeed )
{
static unsigned long test;

 test = ( (MixSpeed * 10) << 2 ) / (Tempo * 4);  // MixSpeed*10 / (Tempo*4)
 test = ( test + 2 ) >> 2;                       // round

return test;
}


volatile unsigned long Division;
volatile unsigned long Div_Fraction;

/*
//unsigned long divide_64bit( unsigned long Position, unsigned long Fraction, unsigned long DivPosition, unsigned long DivFraction);
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

*/
