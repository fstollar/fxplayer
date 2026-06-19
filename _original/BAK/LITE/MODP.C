
//*** Allgemeiner Check von SB etc. ***

int sb_check(void)
{
test=InitSB (SBAdr,SBIRQ,SBDMA,MixSpeed,SBStereo,SBInterpolation);
if (test==0xffff)
   {
    printf("\nNo Soundblaster found !\n");
    return 255;
   }

if (!quiet)
{
printf("\n Soundblaster found .... Version: %01u.%02u\n",(test/256),(test%256));
printf(" Adr: %03x  IRQ: %01u  DMA: %01u  MixSpeed: %05u\n",SBAdr,SBIRQ,SBDMA,MixSpeed);
}

return 0;
}

//*** FMS Manager installieren ***

int FMS_Install(void)
{
test=InitFMS ();
if (test!=0) {
   printf("\n");
   if (test==1) { printf("Processor already in Protected Mode !!\n\n");
		  printf("Please restart your PC without EMM,Qemm or any other driver like this.\n");
		  printf("This programm will also not run if you start it in OS/2 or WINDOWS !!\n");
		  return 255; }
   if (test==2) { printf("No XMM found !!\n\n");
		  printf("Please restart your PC with HIMEM or a similar driver witch install XMS.\n");
		  return 255; }
   if (test==3) { printf("XMM too old !!\n\n");
		  printf("Your himem driver is MUCH too old !! Ask your friends for a newer one.\n");
		  return 255; }
  }
return 0;
}

//*** Playervariablen reservieren ***

int Play_Install(void)
{
if (InitPlay()!=0)      //Reserviere Speicher fÅr Player
   {
   printf("\nInternal Memory error !!\nNot enough Basememory !\n");
   return 255;
   }
return 0;
}

//********************************
//*** S3M laden und dekodieren ***
//********************************

int s3m_load_and_decode(void)
{
test=LoadS3M_FMS(file_to_load);  //Lade S3M Modul und reserviere Speicher
return test;
}

int run_s3m(void)
{
   int check;

   if (s3m_in_memory == 1)
   {
	 StopMixer();

	 StopPlay();     //Stoppe SB

	 //*** Free all Memory also FMS !!! ***

	 CleanS3M_FMS();     //RÑumt S3M Variablen auf und gibt Speicher frei
    s3m_in_memory = 0;
   }

   ResetSB();

   //ResetPlay();

   check = s3m_load_and_decode();
   if (check != 0) { return check; }   // Fehler aufgetreten

   InitMixer();

   //*************************
   //*** Modplayer starten ***
   //*************************

   StartPlay();    //Starte SB Interrupt

   StartMixer();   //Beginne mit Mischen

   s3m_in_memory = 1;

   return 0;
}

char DOS_shell(void)
{
int check;

if (!quiet)
   {
    clrscr();
    printf("\nReturn to F/X Player by typing: EXIT + [RETURN]\n");
    printf("Have a nice DOS :)\n");
   }

check=CallCommand();
if (check!=0)
     {
      printf("\nCOMMAND.COM not found !!\n");
     }

return check;
}
