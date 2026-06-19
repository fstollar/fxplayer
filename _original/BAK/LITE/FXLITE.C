# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <dir.h>
# include <dos.h>
# include <conio.h>
# include <process.h>

#define unsichtbar  0xfeff
#define strich      0x0607

unsigned int  test;
unsigned int  SBAdr=0x220, MixSpeed=22050;
unsigned char SBIRQ=5, SBDMA=1, SBStereo=0, SBInterpolation=0;
union REGS Regs;               //Interrupt Register

char far file_to_load[80];
char s3m_in_memory=0, quiet=0, shell=0;

//* Externe ASSEMBLER Routinen *

extern far unsigned char InitFMS (void);
extern far unsigned char LoadS3M_FMS (char far *FileName);
extern far void          CleanS3M_FMS (void);
extern far unsigned char InitPlay (void);
extern far void          ResetPlay(void);
extern far void          ClosePlay (void);
extern far unsigned int  InitSB (int BasisIO, char IRQ, char DMA , unsigned int MixSpeed, char StereoFlag, char Interpolation);
extern far unsigned int  ResetSB (void);
extern far void          StartPlay (void);
extern far void          StopPlay (void);
extern far void          InitMixer (void);
extern far void          StartMixer (void);
extern far void          StopMixer (void);
extern far void          CallMixer (void);
extern far void          PlayFF (void);
extern far void          Pause (void);
extern far unsigned long Takt (void);
extern far void          ChangeMode (void);
extern far void          ChangeInterp (void);
extern far unsigned char CallCommand (void);

#include "modp.c"

void cursor(unsigned int kind)
{
 asm {
     mov    cx,[kind]
     mov    ax,0x0100
     int    16
     }
}

int strmid( char *destination, char *source, int offset, int len)
{
  int i;
  source+=offset;
  for (i=1; i<=len; i++)
  {
     *destination++ = *source++;
  }
  *destination++ = '\0';
return 0;
}


//****************************************************************

void main(int argc, char *argv[] )
{
  int check, count;
  char string[10],cr1,cr2;

if (argc>1)
   {
    strcpy(file_to_load,argv[1]);
    strmid(string,argv[1],strlen(argv[1])-4,4);
    if (strcmpi(string,".S3M")!=0) strcat(file_to_load,".S3M");

    for (count=1; count<argc; count++)
        {
        if ( ((strncmpi(argv[count],"A:",2))==0) )
           {
            strmid(string,argv[count],2,1);
            SBAdr=(atoi(string))*0x100;
            strmid(string,argv[count],3,1);
            SBAdr+=(atoi(string))*0x010;
           }
        if ( ((strncmpi(argv[count],"I:",2))==0) )
           {
            strmid(string,argv[count],2,1);
            SBIRQ=atoi(string);
           }
        if ( ((strncmpi(argv[count],"D:",2))==0) )
           {
            strmid(string,argv[count],2,1);
            SBDMA=atoi(string);
           }
        if ( ((strncmpi(argv[count],"M:",2))==0) )
           {
            strmid(string,argv[count],2,5);
            MixSpeed=atoi(string);
           }
        if ( ((strncmpi(argv[count],"Mode:",5))==0) )
           {
            strmid(string,argv[count],5,6);
            if ( ((strncmpi(string,"stereo",6))==0) )
               {
                //printf("STEREO !\n");
                SBStereo=2;
               }
            else {
                  //printf("MONO !\n");
                  SBStereo=0;
                 }
           }
        if ( ((strncmpi(argv[count],"Interp:",7))==0) )
           {
            strmid(string,argv[count],7,3);
            if ( ((strncmpi(string,"on",2))==0) )
               {
                //printf("Interpolation !\n");
                SBInterpolation=1;
               }
            else {
                  //printf("no Interpolation !\n");
                  SBInterpolation=0;
                 }
           }

        if ( ((strcmpi(argv[count],"/S"))==0) )
           {
            shell=1;
           }
        if ( ((strcmpi(argv[count],"/Q"))==0) )
           {
            quiet=1;
           }
        }
   }

if (!quiet) {
   printf("\nF/X Player v0.40à *lite* by STIGMA in 1997\n");
   printf("(c) Apollo of STIGMA at 11th May 1997\n");
   }

if ( (argc==1) || ((strchr(argv[1],'?'))!=0) )
  {
   printf("\n Usage:  FXLITE Module[.S3M] [A:xxx] [I:x] [D:x] [M:xxxxx] [Mode:mono/stereo]\n");
   printf("                [Interp:on/off] [/S] [/Q]\n");
//   printf(" Module could include drive and path of the specified S3M Module.\n");
   printf("\nYour Soundcard must be at least a SoundBlaster 2.0 or full compatible !\n");
   printf("No GUS will be supported... probably next year :)\n");
   printf("\nSwitches:\n\n");
   printf(" ?       : shows this helpscreen\n");
   printf(" A:xxx   : Adress of your SoundBlaster in Hexadecimal (default: 220)\n");
   printf(" I:x     : Interrupt of your SB (default: 5)\n");
   printf(" D:x     : DMA channel of your SB which could range from 1 to 7 (default: 1)\n");
   printf("     NOTE: If the DMA is from 1 to 3 it will be played at 8bit\n");
   printf("           for DMA 4 to 7 you must have a SB16 or compatible to play in 16bit\n");
   printf(" M:xxxxx : Mixingspeed range from 5000 to 44100Hz (default: 22050)\n");
   printf(" Mode    : mono mode or stereo mode, in stereo it will panned (default: mono)\n");
   printf(" Interp  : Interpolation 1. Grade on or off (default: off)\n");
   printf(" /S      ; DOS-Shell !\n");
   printf(" /Q      : Quietmode ! No text will appeare on screen (default: off)\n");
   printf(" /Greets : hum ? :)\n");
  }

if ( ((strcmpi(argv[1],"/GREETS"))==0))
   {
   printf("\nYeaah !!! This are the greetings !!! :)\n");
   printf("\nMy very special greetings fly to...\n\n");
   printf(" ...to all STIGMA members !! Special to:\n");
   printf(" Beo      / Stigma : Without his help this player would never run...\n");
   printf(" Jakob    / Stigma : For coding one desktop after the other O:-)\n");
   printf(" Puff$tar / Stigma : For his good music he compose and to be cool !\n");
   printf(" To all QUASAR people !!\n");
   printf(" Sexton   / Quasar : Hey, hope u love this player ! Thx for to be so kewl ! :)\n");
   printf(" Int13h   / Quasar : See and get stoned ;)\n");
   printf(" To SCOPE, my second group I joined...\n");
   printf(" Xenon    / SCOPE  : Met at MEKKA'97, and now member of SCOPE :)\n");
   printf(" ... thx for let me join in !\n");
   printf(" Special greets fly out to:\n");
   printf(" Pascal / Ex.Cubic : Thx for ya invent. to Mekka and keep on coding Cubic2.0 :D\n");
   printf(" Screamager        : Please, send me some of ya muzak :-D\n");
   printf("\n And to all I met on IRC and special to them who are on #coders.ger !!\n");
   printf(" and all I forgot to mention here.... O:-)\n");
   printf("\nNow go along and make some good muzak !! :)\n");
   return;
   }

//**********************************

if (argc>1)
  {

check = FMS_Install();
      if (check == 255) return;

check = sb_check();
   if (check == 255) return;   // error occoured

   check = Play_Install();
   if (check == 255) return;

cursor(unsichtbar);

   check = run_s3m();                  // load s3m :)
   if (check!=0)
      {
      if (check==255) printf("\nS3M not found !!\n");
      if (check==1) printf("%s is not a valid S3M File !\n",file_to_load);
      if (check==2) printf("FMS allocation error !!\n");
      cursor(strich);
      return;
      }

 if (shell) check=DOS_shell();

Nothing:
;
   while (!kbhit());
   Regs.h.ah = 0x10;
   int86 ( 0x16, &Regs, &Regs);
   //cr1=Regs.h.al;
   cr2=Regs.h.ah;

   if (cr2==77) { PlayFF(); goto Nothing; }
   if (cr2==57) { Pause(); goto Nothing; }

   if (s3m_in_memory == 1)
   {
   StopMixer();
   StopPlay();
   CleanS3M_FMS();
   }

   ClosePlay();    //R„umt Playervariablen auf und gibt PlayerMem frei

   cursor(strich);
     }

return;  // AUS IS !!!
}
