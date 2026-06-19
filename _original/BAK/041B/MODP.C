void read_ini(void)
{
FILE *INIFile;
char drive[3], dummy[70];
char *endptr;

  if ((INIFile=fopen("fx.ini","rt"))!=NULL)
  {
    SBDMA=atoi(getdata(INIFile,"DMA"));
    SBIRQ=atoi(getdata(INIFile,"IRQ"));
    SBAdr=strtol(getdata(INIFile,"Adress"),&endptr,16);
    MixSpeed=atoi(getdata(INIFile,"MixingSpeed"));
    strcpy(dummy,getdata(INIFile,"StereoMode"));
      if ((strcmpi(dummy,"Mono"))==0)       SBStereo=0;
      if ((strcmpi(dummy,"WideMono"))==0)   SBStereo=1;
      if ((strcmpi(dummy,"Stereo"))==0)     SBStereo=2;
      if ((strcmpi(dummy,"WideStereo"))==0) SBStereo=3;
      if ((strcmpi(dummy,"Auto"))==0)       SBStereo=128;
    strcpy(dummy,getdata(INIFile,"Interpolation"));
      if ((strcmpi(dummy,"off"))==0) SBInterpolation=0;
      if ((strcmpi(dummy,"on"))==0)  SBInterpolation=1;
      if ((strcmpi(dummy,"0"))==0)   SBInterpolation=0;
      if ((strcmpi(dummy,"1"))==0)   SBInterpolation=1;
    strcpy(startpath,getdata(INIFile,"StartUpDirectory"));
    strupr(startpath);
    chdir(startpath);
    fnsplit(startpath, drive, dummy, dummy, dummy);
    setdisk(drive[0]-'A');

  }  else {
    SBDMA=1;
    SBAdr=0x220;
    SBIRQ=5;
    MixSpeed=44100;
    SBStereo=0;
    SBInterpolation=0;
    setdisk(2);   strcpy(startpath,"C:\\");
  }
 _fputstring( 2,2, startpath, HEAD_COLOR);
 fclose(INIFile);

 return;
}


//*** Allgemeiner Check von SB etc. ***

int sb_check(void)
{
test=InitSB (SBAdr,SBIRQ,SBDMA,MixSpeed,SBStereo,SBInterpolation);
if (test==0xffff)
   {
    printf("No Soundblaster found !\nPlease check your INI file !\n");
    return 255;
   }
gotoxy(1,14);
printf("Soundblaster found .... Version: %01u.%02u\n",(test/256),(test%256));
printf("Adr: %03x  IRQ: %01u  DMA: %01u  MixSpeed: %05u\n",SBAdr,SBIRQ,SBDMA,MixSpeed);


return 0;
}

//*** FMS Manager installieren ***

int FMS_Install(void)
{
test=InitFMS ();
if (test!=0) {
   OldScreen();
   clrscr();
   gotoxy(1,1);
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
   printf("Internal Memory error !!\nNot enough Basememory !\n");
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
if (test!=0)
    {
    //OldScreen();
    if (test==255) printf("%s not found !\n",file_to_load);
    if (test==1) printf("%s is not a valid S3M File !\n",file_to_load);
    if (test==2) printf("FMS allocation error !!\n");
    //gotoxy(1,24);
    return test;
    }
return 0;
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

   ResetPlay();

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

