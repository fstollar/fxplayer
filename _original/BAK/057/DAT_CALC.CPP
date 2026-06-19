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

unsigned long DAT_calcSpeed ( unsigned short Tempo, unsigned long MixSpeed )
{
static unsigned long test;

 test = ( (MixSpeed * 10) << 2 ) / (Tempo * 4);  // MixSpeed*10 / (Tempo*4)
 test = ( test + 2 ) >> 2;                       // round

return test;
}
