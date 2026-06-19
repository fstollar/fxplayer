# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <dir.h>
# include <dos.h>
# include <conio.h>
# include <io.h>
# include <ctype.h>

unsigned int  test, SBAdr, MixSpeed;
unsigned char SBIRQ, SBDMA, SBStereo, SBInterpolation;
union REGS Regs; //Interrupt Register

char far S3MHeader[0x62];
char far drives[26][27];
int anzlw, lfw_neu, lfw_start;
char far fname[300][27];
char far file_to_load[80];
char s3m_in_memory=0;

char far *FileName; //FileName Pointer
FILE *in;

int file_selected, eintrag_oben, eintrag_akt, max_eintrag;
int max;
int akt_drive;
char far pfad[150];
char far startpath[150],far aktpath[150];
int alength=100;
int status;
char ext[15];
char dest[25], *dest2;
int  sy_old, sy;
long far *dostime, oldtime;

// ** Daten zur Soundblastereinstellung **
char far SBI10[4][7]={"200","220","240","260"};
char far SBI20[4][7]={"1","3","5","7"};
char far SBI30[4][7]={"1","2","4","5"};
char far SBI40[4][7]={"11000","22000","36000","44100"};

//* Externe ASSEMBLER Routinen *
extern far unsigned char InitFMS (void);
extern far unsigned char LoadS3M_FMS (char far *FileName);
extern far unsigned char LoadS3M_Head(char far *FileName,void far *FileOffset);
extern far void          CleanS3M_FMS (void);
extern far unsigned char InitPlay (void);
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

extern far void putchr(int x,int y,int zeichen,char attrib);
extern far void _fputstring(int x,int y,void far *string,char attrib);

#include "fx.h"
#include "tasten.h"
#include "ini.c"
#include "screen.c"
#include "modp.c"
#include "filedsk3.c"

//****************************************************************


void main(void)
{
  int check;

lfw_start = lfw_neu = getdisk();
getcwd (startpath, alength);


check = FMS_Install();
      if (check == 255) return;

//*** VGA-TextModus 80x43/50 ***

NewScreen();
cursor(unsichtbar);

build_screen();

read_ini();

check = sb_check();
   if (check == 255) return;   // Fehler aufgetreten

   check = Play_Install();
   if (check == 255) return;

   main_loop();

   if (s3m_in_memory == 1)
   {
   StopMixer();
   StopPlay();
   CleanS3M_FMS();
   }

   ClosePlay();    //R„umt Playervariablen auf und gibt PlayerMem frei

   setdisk(lfw_start);
   chdir(startpath);
   cursor(strich);

   OldScreen();
   ClrScreen();
   printf("Thank you for using F/X Player... the new dimension of Soundblaster sound !\n\n");
   printf("You will find the newest Version under: http://www.home.pages.de/~JAKOB\n");

return;  // AUS IS !!!
}
