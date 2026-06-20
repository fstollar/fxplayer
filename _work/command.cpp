#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>

#include "fx.h"
#include "command.h"
#include "dev_wav.h"

//#include "irq.h"
//#include "dma.h"

//#include "device.h"
//#include "dev_sb.h"

//#include "format.h"
//#include "dat_wav.h"
//#include "dat_s3m.h"

//#include "convert.h"
//#include "mixer.h"

void strmid( char *source, char *destination, int offset, int len)
{
  int i;
  source+=offset;

  for (i=1; i<=len; i++)
  {
     *destination++ = *source++;
  }
  *destination++ = '\0';
}


short comline(int argc, char **argv)
{
 unsigned short flag_file = 0;
 long test;
 unsigned long length, i, t, count = 1;
 char string[99];
 char *point;

 if (argc<=1)  return 0;

 while (count <= (argc-1) )
       {
        if ( (argv[count][0] != '-') && (argv[count][0] != '/') )
           {
            if ( flag_file == 1 )
               {
                printf("Wrong switch ! You must use - or / infront to mark as switch !\n");
                return -1;
               }
            strcpy( FileName, argv[count] );
            flag_file = 1;
           }
        else
           {
            length = strlen(argv[count]);
            for (t = 1; t < length; t++)
                {
                 test = check_switch( argv[count][t] );
                 if ( test == 1 )   return -1;
                 switch (test)
                        {
                         case 2:
                                 if ( argv[count][t+1] == ':' )
                                    {
                                     t+=2;
                                     i = strtoul ( argv[count]+t , &point, 10 );
                                     if ( point == argv[count]+t )
                                        {
                                         printf("The switch 't:' must have a value what soundcard you like !\n");
                                         return -1;
                                        }
                                     t = point - argv[count] - 1;
                                     if ( i == 0 )
                                        {
                                         printf("Psssssh ... are you playing computer without ya parents permission ?\n");
                                         printf("Or is ya little sister asleep right the next room ?\n");
                                         printf("Or do YOU just not like that kind of music ??\n");
                                         printf("Then STOP it or ......\n");
                                         printf("        _---_\n");
                                         printf("       /     \\\n");
                                         printf("      |<--O   |\n");
                                         printf("       \\     /\n");
                                         printf("        ~-^-~\n");
                                         printf(" PUT UP THE VOLUME !! ;)\n");
                                         return -1;
                                        }
                                     if ( i < 1 || i > 3 )
                                        {
                                         printf("This soundcard Nr:%1u is not defined yet !\n",i);
                                         return -1;
                                        }
                                     CardType = i;
                                    }
                                 else
                                    {
                                     printf("The switch looks like 't:CARDTYPE' !\n");
                                     return -1;
                                    }
                                 break;
                         case 3:
                                 if ( argv[count][t+1] == ':' )
                                    {
                                     t+=2;
                                     i = strtoul ( argv[count]+t , &point, 16 );
                                     if ( point == argv[count]+t )
                                        {
                                         printf("The switch 'a:' needs the address for the soundcard (in hex) !\n");
                                         return -1;
                                        }
                                     t = point - argv[count] - 1;

                                     if ( t < (length-1) )
                                        {
                                         printf("Behind the 'a:' switch is no other switch allowed !\n");
                                         printf("You must start the next switch with a new - or / !\n");
                                         return -1;
                                        }

                                     if ( i < 0x100 )
                                        {
                                         printf("This address must be greater than 100 hex !\n");
                                         return -1;
                                        }
                                     Address = i;
                                    }
                                 else
                                    {
                                     printf("The switch looks like 'a:ADDRESS' !\n");
                                     return -1;
                                    }
                                 break;
                         case 4:
                                 if ( argv[count][t+1] == ':' )
                                    {
                                     t+=2;
                                     i = strtoul ( argv[count]+t , &point, 10 );
                                     if ( point == argv[count]+t )
                                        {
                                         printf("The switch 'i:' needs the interrupt number !\n");
                                         return -1;
                                        }
                                     t = point - argv[count] - 1;

                                     if ( i > 15 )
                                        {
                                         printf("The PC has no interrupt greater than 15 !\n");
                                         return -1;
                                        }
                                     IRQ = i;
                                    }
                                 else
                                    {
                                     printf("The switch looks like 'i:IRQ' !\n");
                                     return -1;
                                    }
                                 break;
                         case 5:
                                 if ( argv[count][t+1] == ':' )
                                    {
                                     t+=2;
                                     i = strtoul ( argv[count]+t , &point, 10 );
                                     if ( point == argv[count]+t )
                                        {
                                         printf("The switch 'd:' needs the number of the DMA channel !\n");
                                         return -1;
                                        }
                                     t = point - argv[count] - 1;

                                     if ( i > 7 )
                                        {
                                         printf("The PC has no DMA channel greater than 7 !\n");
                                         return -1;
                                        }
                                     DMA = DMAlo = DMAhi = i;
                                    }
                                 else
                                    {
                                     printf("The switch looks like 'd:DMA' !\n");
                                     return -1;
                                    }
                                 break;
                         case 6:
                                 if ( argv[count][t+1] == ':' )
                                    {
                                     t+=2;
                                     i = strtoul ( argv[count]+t , &point, 10 );
                                     if ( point == argv[count]+t )
                                        {
                                         printf("The switch 'f:' needs a speed value in Hz !\n");
                                         return -1;
                                        }
                                     t = point - argv[count] - 1;

                                     if ( i > 64000 )
                                        {
                                         printf("This frequency is not possible !\n");
                                         return -1;
                                        }
                                     MixSpeed = i;
                                    }
                                 else
                                    {
                                     printf("The switch looks like 'f:MIXSPEED' !\n");
                                     return -1;
                                    }
                                 break;
                         case 7:                         // -w:filename  (WAV output)
                                 if ( argv[count][t+1] == ':' )
                                    {
                                     t+=2;
                                     if ( argv[count][t] == '\0' )
                                        {
                                         printf("The switch 'w:' needs an output filename !\n");
                                         return -1;
                                        }
                                     strcpy( wav_outfile, argv[count]+t );
                                     CardType = 4;       // select WAV output device
                                     t = length - 1;     // filename consumed the rest of this arg
                                    }
                                 else
                                    {
                                     printf("The switch looks like 'w:FILENAME' !\n");
                                     return -1;
                                    }
                                 break;
                         case 8:                         // -n:seconds  (max render time for WAV)
                                 if ( argv[count][t+1] == ':' )
                                    {
                                     t+=2;
                                     i = strtoul ( argv[count]+t , &point, 10 );
                                     if ( point == argv[count]+t )
                                        {
                                         printf("The switch 'n:' needs a time in seconds !\n");
                                         return -1;
                                        }
                                     t = point - argv[count] - 1;
                                     wav_max_seconds = i;
                                    }
                                 else
                                    {
                                     printf("The switch looks like 'n:SECONDS' !\n");
                                     return -1;
                                    }
                                 break;
                        }
                }
           }
        count++;
       }
return 1;
}


short check_switch(char sw)
{
 switch (sw)
        {
         case 'h': case 'H':
                   helpline();
                   return 1;
         case 't': case 'T':
                   return 2;
         case 'a': case 'A':
                   return 3;
         case 'i': case 'I':
                   return 4;
         case 'd': case 'D':
                   return 5;
         case 'f': case 'F':
                   return 6;
         case 'w': case 'W':
                   return 7;
         case 'n': case 'N':      // -n:SECONDS max render time (note: 't' is already card Type)
                   return 8;
         default:
                  printf("Switch \"%c\" is not allowed !!\nTry >FX -h< for help !\n", sw );
                  return 1;
        }

return 0;
}

void waitkey()
{
 printf("\n[Press any key to continue]\n");
 getch();
 printf("\n");
}

void helpline()
{
 printf(" Usage: FX [- or / switch(es)] [file]\n\n");
 printf(" If FX is started without parameters a menu will pop up. [not yet]\n");
 printf(" With [file] you can select a module which will be loaded immediatly.\n\n");
 printf(" Switch:\n\n");
 printf("  h    : shows this help screen\n");
 printf("  t:#  : sets the type of the soundcard to number #\n");
  printf("          0 - no sound ;)\n");
  printf("          1 - Soundblaster Classic/Pro/16/32/64/AWE/PnP or compatible (default)\n");
  printf("          2 - Windows Sound System or compatible\n");
  printf("          (3 - Gravis Ultra Sound Classic/MAX/PnP or Interwave)\n");
 waitkey();
 printf("  a:#  : sets the address of the soundcard to # (in hex) (default:220)\n");
 printf("  i:#  : sets the irq of the soundcard to number # (default:5)\n");
 printf("  d:#  : sets the dma of the soundcard to number # (default:1/5)\n");
 printf("  f:#  : sets the output frequency to # Hz (default:44100)\n");
 printf("\n Following commands with a '$' can be switched 1/on or 0/off\n\n");
 printf("  (s    : sets the output of the soundcard to stereo or mono) (default:stereo)\n");
 printf("  (b    : sets the output of the soundcard to 8 bit or 16 bit) (default:16bit)\n");
 printf("  (c    : how to clip stream over-/ underrun, hard or soft)\n");
 printf("          0 = hard (linear), 1 = soft (logarithmic) (default:soft)\n");
 printf("  (p    : interpolation on or off  (default:on)\n");
 printf("  (l    : sets loop mode for playing the song continuose)\n");
 printf("  w:#  : write output to WAV file # (implies WAV output, no soundcard needed)\n");
 printf("  n:#  : max render time in seconds for WAV output (0 = until end, default)\n");
 printf("         Note: 't' is already used for card Type; use 'n' for Number of seconds\n");

 printf("\n  file : soundfile to load and start immediatly , path is allowed\n");
 printf("           Following formats are supported:\n");
 printf("             MOD - AMIGA sound modules\n");
 printf("             669 - Composer 669 modules\n");
 printf("             S3M - Scream Tracker 3 Modules\n");
 printf("             WAV - RIFF wave files\n");
}
