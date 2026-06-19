#include <stdio.h>
#include <dos.h>
#include <stdlib.h>
#include <conio.h>

#include "fx.h"

#include "keyctrl.h"
#include "format.h"

//#define cdebug

// *** Routines for handling key strokes

long getkey ()
{
 unsigned short key = 0;

 key = getch();

#ifdef cdebug
 printf("key: %u\n",key);
#endif

 switch (key)
         {
          case 0  :
                    key = getch ();
#ifdef cdebug
                    printf("skey: %u\n",key);
#endif
                    switch (key)
                           {
                            case 77 :
                                      FF_Format ();
                                      break;
                            case 75 :
                                      FR_Format ();
                                      break;
                           }
                    break;
          case 13 :
          case 27 :
          case 32 :
                    return key;
                    break;
          case 48 :
                    flag_softclip ^= 1;
                    printf("Clipping is %s\n", flag_softclip ? "soft" : "hard" );
                    break;
          case 100 :
                    system("command.com");
                    break;
          default :
                    return 0;
                    break;
         }

return 0;
}
