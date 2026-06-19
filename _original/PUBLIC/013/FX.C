#include <stdlib.h>
#include <dos.h>
#include "screen.c"
#include "fopen.h"

//* Externe ASSEMBLER Routinen *
extern unsigned char InitFMS (void);
extern unsigned char LoadS3MF (char far *FileName);
extern void          CleanS3M(void);
extern unsigned char InitPlay(void);
extern void          ClosePlay(void);
extern unsigned int  InitSB (int BasisIO, char IRQ, char DMA , unsigned int MixSpeed);
extern void          StartPlay(void);
extern void          StopPlay(void);
extern void          StartMixer(void);

//* Allgemeine wichtige Variablen *
unsigned int  test, SBAdr, MixSpeed;
unsigned char testS3M, SBIRQ, SBDMA;
union REGS Regs; //Interrupt Register

//****************************************************************

int main(void)
{
char far *FileName; //FileName Pointer
FILE *INIFile;

struct menustruct m;

  setsize(&m,80,50,RED+16*LIGHTGRAY);
  setpos(&m,56,3,49);
  setcolor(&m,16*LIGHTGRAY,DARKGRAY+16*GREEN,16*LIGHTGRAY,DARKGRAY+16*LIGHTGRAY);

  if ((INIFile=openfile("fxtrack.ini"))!=NULL)
  {
    SBDMA=atoi(getdata(INIFile,"DMA"));
    SBIRQ=atoi(getdata(INIFile,"IRQ"));
    SBAdr=stol(getdata(INIFile,"Address"),16);
    MixSpeed=atoi(getdata(INIFile,"Mixing-Speed"));
    setdirectory(&m,getdata(INIFile,"StartUpDirectory"));
  }  else {
    SBDMA=1;
    SBAdr=0x220;
    SBIRQ=5;
    MixSpeed=44100;
    setdirectory(&m,"F:\\digi");
  }

//*** VGA-TextModus 80x43/50 ***

NewScreen();

//*** Allgemeiner Check von SB etc. ***

Again:
;
test=InitSB (SBAdr,SBIRQ,SBDMA,MixSpeed);
if (test==0xffff)
   {
    printf("No Soundblaster found !\n");
    return 0;
   }
gotoxy(1,14);
printf("Soundblaster found .... Version: %01u.%02u\n",(test/256),(test%256));
printf("Adr: %03x  IRQ: %01u  DMA: %01u  MixSpeed: %05u\n",SBAdr,SBIRQ,SBDMA,MixSpeed);

test=InitFMS ();
if (test!=0) {
   OldScreen();
   clrscr();
   gotoxy(1,1);
   if (test==1) { printf("Processor already in Protected Mode !!\n\n");
                  printf("Please restart your PC without EMM,Qemm or any other driver like this.\n");
                  printf("This programm will also not run if you start it in OS/2 or WINDOWS !!\n");
                  return 0; }
   if (test==2) { printf("No XMM found !!\n\n");
                  printf("Please restart your PC with HIMEM or a similar driver witch install XMS.\n");
                  return 0; }
   if (test==3) { printf("XMM too old !!\n\n");
                  printf("Your himem driver is MUCH too old !! Ask your friends for a newer one.\n");
                  return 0; }
  }
//********************************
//*** S3M laden und dekodieren ***
//********************************

FileName=load(m);

if (InitPlay()!=0)      //Reserviere Speicher fÅr Player
   {
   printf("Internal Memory error !!\n");
   return 0;
   }

gotoxy(1,17);
printf("Loading ...\n");

testS3M=LoadS3MF(FileName);  //Lade S3M Modul und reserviere Speicher
if (testS3M!=0)
    {
    OldScreen();
    ClosePlay();
    //if (testS3M==255) printf("%s not found !\n",FileName);
    if (testS3M==1) printf("%s is not a usual S3M File !\n",FileName);
    if (testS3M==2) printf("FMS allocation error !!\n");
    gotoxy(1,24);
    return 0;
    }

//*************************
//*** Modplayer starten ***
//*************************

printf("Start Playing ...\n");
printf("\nChannels:   Pattern:  /  (  ) RowPos:   \n");
printf("\nSpeed:  /\n");
printf("\nTimeCounter:       \n");

StartPlay();    //Starte SB Interrupt

StartMixer();   //Beginne mit Mischen (ESC = Return)

StopPlay();     //Stoppe SB

printf("\nPlaying Stopped ...\n");

//*** Free all Memory also FMS !!! ***

CleanS3M();     //RÑumt S3M Variablen auf und gibt Speicher frei
ClosePlay();    //RÑumt Playervariablen auf und gibt PlayerMem frei

NewScreen();

goto Again;

//return 0;  // AUS IS !!!
}

