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

#include "format.h"
#include "dat_wav.h"
#include "dat_s3m.h"

//#include "convert.h"
//#include "mixer.h"

// *********************

short test_File ( char *FileName )
{
 unsigned short dum=0;
 char Ext_Pointer[10];
 char *dummypointer;

 dummypointer = (char *)(FileName + strlen(FileName) - 3);

 strcpy ( Ext_Pointer, dummypointer );
 strupr ( Ext_Pointer );

 if ( !memcmp( Ext_Pointer, "RAW", 3) )  dum = 1;
 if ( !memcmp( Ext_Pointer, "WAV", 3) )  dum = 2;
 if ( !memcmp( Ext_Pointer, "VOC", 3) )  dum = 3;

 if ( !memcmp( Ext_Pointer, "MOD", 3) )  dum = 100;
 if ( !memcmp( Ext_Pointer, "S3M", 3) )  dum = 101;
 if ( !memcmp( Ext_Pointer, "669", 3) )  dum = 102;
 if ( !memcmp( Ext_Pointer, "MTM", 3) )  dum = 103;


return dum;
}


short load_Format ( char *FileName, unsigned short DatType)
{
 short test = 0;

 if (Last_DatType != 0)   close_Format ();

 switch (DatType)
        {
         case 0:
                  break;
         case 1:            // RAW

                  break;
         case 2:            // WAV

                  test = load_WAV ( FileName );
                  break;

         case 100:          // MOD

                    break;
         case 101:          // S3M

                    test = load_S3M ( FileName);
                    break;
         case 102:          // 669

                    break;
        }

 Last_DatType = DatType;

return test;
}


short close_Format ( )
{
 short test = 0;

 if (Last_DatType == 0) return 0;

 switch (DatType)
        {
         case 1:            // RAW

                  break;
         case 2:            // WAV

                  test = close_WAV ();
                  break;

         case 100:          // MOD

                    break;
         case 101:          // S3M

                    test = close_S3M ();
                    break;
         case 102:          // 669

                    break;
        }

 Last_DatType = 0;

return test;
}


