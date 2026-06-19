//***************************************************************************
//******************* OberflÑche fÅr F/X Player v0.20‡ **********************
//*** made by STIGMA in 1996 in a great desire of laziness............... ***

int sortfkt( const void *a, const void *b);
void show_files(int start, int max, int select);
int strmid( char *dest, char *source, int offset, int len);
void showinfo(void);
void SB_Window(void);


int sortfkt(const void *a, const void *b)
{
   return( strcmp((char *)a , (char *)b) );
}

int directory(void)
{
  struct ffblk ffblk;
  int done,ii,i;
  char string[25], str[10], ch, cr1, cr2, *d;


  i = 0;
  done = findfirst("*.*", &ffblk, FA_DIREC+FA_ARCH+FA_HIDDEN+FA_RDONLY);
  done = findnext(&ffblk);
  while (!done)
  {
	if (ffblk.ff_attrib &= FA_DIREC)
	{
	  if (strcmp(string, "."))
	  {
	 strcpy(string , " ");
	 strcat(string , ffblk.ff_name);
	 strcat(string , "              ");
	 strncpy(fname[i], string, 15);
	 fname[i][15] = '\0';
	 strcat(fname[i] , " < DIR > ");
	 i++;
	  }
	}
	else
	{
	  strcpy(fname[i]," ");
	  strcat(fname[i] , ffblk.ff_name);
	  strcat(fname[i],"                      ");
	  ltoa((ffblk.ff_fsize + 1023)>> 10,str,10);
	  strncpy(string, fname[i], 22-strlen(str));
	  string[22-strlen(str)] = '\0';
	  strcat(string,str);
	  strcat(string,"K ");
	  strcpy(fname[i] , string);

	  i++;
	}
	done = findnext(&ffblk);
  }

  // Verzeichnis sortieren
  qsort( (void *)fname, i, sizeof(fname[0]), sortfkt);

  // Laufwerksbuchstaben anhÑngen
  for (ii=0; ii<anzlw; ii++)
  {
	 strcpy(fname[i] , drives[ii]);
	 i++;
  }

  max = i;


// ********************

  file_selected = -1;
  eintrag_oben  = 0;
  eintrag_akt   = 0;
  max_eintrag   = i-1;

  // Files im akt. Fenster anzeigen
  show_files(eintrag_oben, max , 0);

  dostime=MK_FP(0x40,0x6c);

  // #####  MenÅ  #####

  while (file_selected==-1)
  {
    sy_old = sy;
    sy = (int) (( (float)(eintrag_akt) / (float)(max_eintrag) ) * (float)(F_DELTA_Y - 5) );
    _fputstring(SCROLL_X, sy_old + F_START_Y + 2,"±",SCROLLCOLOR);
    _fputstring(SCROLL_X, sy + F_START_Y + 2,"˛",SCROLLCOLOR);

    oldtime=*dostime;

    while (!kbhit())  if (((*dostime)-(oldtime))==8) showinfo();

    Regs.h.ah = 0x10;
    int86 ( 0x16, &Regs, &Regs);
    cr1=Regs.h.al; cr2=Regs.h.ah;
    //gotoxy(1,1);
    //printf("%i %i",cr1,cr2);
    switch (cr1)
	  {
	case alt  :  switch (cr2)
			{
			 case chr_x  : strcpy(file_to_load,"\0"); file_selected=255;
				    break;
			}
			break;
	case ctrl  : break;
	case shft  : break;
	case escape: file_selected=255;
			   strcpy(file_to_load,"\0");
			   break;
	default:
			for (i=1; i<max_eintrag; i++)
			  {
			 if (fname[i][1]==toupper(cr1))
			 {
			    eintrag_akt=i;
			    if (eintrag_akt < eintrag_oben)
				eintrag_oben = eintrag_akt-((F_DELTA_Y-2)/2);
			    if (eintrag_oben < 0)
				eintrag_oben = 0;
			    if (eintrag_akt >= eintrag_oben+F_DELTA_Y-2)
				eintrag_oben = eintrag_akt-((F_DELTA_Y-2)/2);
			    if (eintrag_oben > abs(max_eintrag- F_DELTA_Y + 2))
				eintrag_oben = (max_eintrag - F_DELTA_Y + 3);
			    show_files(eintrag_oben, max, eintrag_akt);

			    break;
			 }
			  }
			break;

	  }

    switch (cr2)  // scan-code
	  {
	case down   : if ( eintrag_akt < max_eintrag )  eintrag_akt++;
			 if (( eintrag_akt % (F_DELTA_Y+eintrag_oben-2) )==0)
			  eintrag_oben++;
			 show_files(eintrag_oben, max, eintrag_akt);
			 break;
	case up     : if ( eintrag_akt > 0 )  eintrag_akt--;
			 if (( eintrag_akt < eintrag_oben ))
			  eintrag_oben--;
			 show_files(eintrag_oben, max, eintrag_akt);
			 break;

	case pgup   : if (eintrag_oben ==0)
			  eintrag_akt=0;
			 else eintrag_oben-= (F_DELTA_Y - 2)-1;
			 if (eintrag_oben < 0) eintrag_oben=0;
			 if (eintrag_akt >= eintrag_oben+F_DELTA_Y -2)
			  eintrag_akt = eintrag_oben+F_DELTA_Y-2-1;
			 show_files(eintrag_oben, max, eintrag_akt);
			 break;

	case pgdn   : if (eintrag_oben <= max_eintrag- F_DELTA_Y + 2)
			  eintrag_oben += (F_DELTA_Y -2)-1;
			 if (eintrag_oben > abs(max_eintrag- F_DELTA_Y + 2))
			  eintrag_oben = (max_eintrag - F_DELTA_Y + 3);
			 if (eintrag_akt < (max_eintrag - F_DELTA_Y + 2) )
			  eintrag_akt = eintrag_oben;
			 else eintrag_akt = max_eintrag;
			 show_files(eintrag_oben, max, eintrag_akt);
			 break;

	case ctrl_pgup:
	case home   : eintrag_akt  = 0;
			 eintrag_oben = 0;
			 show_files(eintrag_oben, max, eintrag_akt);
			 break;
	case ctrl_pgdn:
	case end    : if (max_eintrag > (F_DELTA_Y - 2) )
			 eintrag_oben = max_eintrag - (F_DELTA_Y - 2) + 1;
			 eintrag_akt = max_eintrag;
			 show_files(eintrag_oben, max, eintrag_akt);
			 break;
	case enter  : ch=fname[eintrag_akt][1];
			 // *** Verzeichnis wechseln ***
			 if (ch == '')
			 {
			  strmid( dest, fname[eintrag_akt] , 2, 13);
			  getcwd( aktpath, alength);
			  if (!(strcmp(dest, "..           ")))
				{
				 d=strrchr(aktpath,'\\');
				 strncpy(aktpath,aktpath, (d-aktpath));
				 aktpath[d-aktpath]='\0';
				}
			  else
			  {
			   if (strlen(aktpath) > 4) strcat( aktpath, "\\");
			   strcat( aktpath, dest );
			  }
			  status = chdir(aktpath);
			  for (i=1; i<=80; i++)
			  {
			  putchr(i,2,' ', HEAD_COLOR);
			  }
			  getcwd (aktpath, alength);
			  _fputstring( 2,2, aktpath, HEAD_COLOR);
			  file_selected = 1;
			  break;
			 }
			 // *** Laufwerk wechseln ***
			 if (ch == ' ')
			 {
			 lfw_neu = fname[eintrag_akt][6] - 65;
			 setdisk(lfw_neu);
			 getcwd( aktpath, alength);
			 for (i=1; i<=80; i++)
			 {
			    putchr(i,2,' ', HEAD_COLOR);
			 }
			 _fputstring( 2,2, aktpath, HEAD_COLOR);
			 file_selected = 2;
			 break;
			 }
			 strmid( dest, fname[eintrag_akt] , 1, 12);
			 dest2=strchr(dest,'.');
			 strmid( ext, dest2, 1, 3);

			 if (!strcmp(ext,"S3M"))
			 {
			 getcwd( aktpath, alength);
			 strcpy( file_to_load , aktpath);
			 if (strlen(aktpath) > 4) strcat( file_to_load , "\\");
			 strmid( dest, fname[eintrag_akt] , 1, 13);
			 strcat( file_to_load, dest);
			 showinfo();
			 // file_selected = 10;
     run_s3m();
			 }
    break;
	case f10:  SB_Window();
			 break;
	   }
 }
return file_selected;
}




void show_files(int start, int max, int select)
{
  int ii;
  char dummy[30], ch;
  char *string;

  for (ii=start; ii<max; ii++)
  {
    if (ii==start+F_DELTA_Y-2) break;
    string=strchr(fname[ii],'.');
    ch = fname[ii][1];

    if (ii==select)
	 _fputstring(F_START_X+1,F_START_Y+1+ii-start, fname[ii],SELECT_COLOR);
    else
	{
	 if (!strncmp(string+1, "S3M",3))
      _fputstring(F_START_X+1,F_START_Y+1+ii-start, fname[ii],S3M_COLOR);
      else
      if (ch == '')
      _fputstring(F_START_X+1,F_START_Y+1+ii-start, fname[ii],DIR_COLOR);
      else
      if (ch == ' ')
      _fputstring(F_START_X+1,F_START_Y+1+ii-start, fname[ii],LFW_COLOR);
      else
      _fputstring(F_START_X+1,F_START_Y+1+ii-start, fname[ii],NORMAL_COLOR);
     }
  }
  if ((max-start) < (F_DELTA_Y - 2))
  {
    for (ii=max; ii<(F_DELTA_Y - 2 - (max-start) + max); ii++)
    _fputstring(F_START_X+1,F_START_Y+1+ii-start, "                        " ,NORMAL_COLOR);
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




int getdrives(void)
{
  int g,t,i;

  // Laufwerke ermitteln
  g=0;
  i=getdisk();
  for (t=0; t<26; t++)
  {
    setdisk(t);
    if (t==getdisk())
    {
	 strcpy(drives[g],"    [  : ]              ");
	 drives[g][6]=t+65;
	 g++;
    }
  }
  setdisk(i);
  return g;
}

// Statusinfofenster fÅllen
void showinfo(void)
{
  unsigned int flags, insnum, patnum, ordnum;
  char S3MHeader[0x62];
  char modulname[35], version[35], s[5], *d;
  char dummy[50], nowpath[80];
  FILE *file;

 strcpy(modulname," N O   S 3 M - F i l e !   ");
 strcpy(version,  "                           ");

 strmid( dummy, fname[eintrag_akt] , 1, 13);
 d=strchr(dummy,'.');
 strmid( ext, d, 1, 3);
 if (!(strcmp(ext,"S3M")))     //Ist es Åberhaupt ein S3M
 {
  getcwd( nowpath, alength);
  strcpy( file_to_load , nowpath);
  if (strlen(nowpath) > 4) strcat( file_to_load , "\\");
  strmid( dummy, fname[eintrag_akt] , 1, 13);
  strcat( file_to_load, dummy);

  if ((file=fopen(file_to_load,"rb"))!=NULL)
  {
    _fputstring(14,5,"Reading...                 ",BLACK+16*BLUE);
    _fputstring(14,7,version,BLACK+16*BLUE);
    fread(S3MHeader,0x60,1,file);

    if ((S3MHeader[0x1c]==0x1a)|(S3MHeader[0x1d]==0x10)|(strncmp(&S3MHeader[0x2c],"SCRM",4)))
    {
    // S3M-File
	strncpy(modulname,S3MHeader,28);
	strncat(modulname,"                         ",30-strlen(S3MHeader));

	flags=(S3MHeader[0x29]>>4);
	switch (flags) {
	    case 1 : strcpy(version,"ScreamTracker "); break;
	    case 2 : strcpy(version,"F/X Tracker "); break;
	    default: strcpy(version,"UNKOWN TRACKER "); break;
		    }
	ultoa((S3MHeader[0x29] & 0x0f),s,16);
	strcat(s,".");
	strcat(version,s);
	ultoa(S3MHeader[0x28],s,16);
	if (strlen(s)==1) strcat(version,"0");
	strcat(version,s); strcat(version,"   ");
    }
   fclose(file);
  }
 }
// Bildschirmausgabe
_fputstring(14,5,modulname,I_WIN_COLOR);
_fputstring(14,7,version,I_WIN_COLOR);

return;
}

void SB_Window(void)
{
	int i, loop;
	int SBAdr_ii, SBDMA_ii, SBIRQ_ii, SBMix_ii;

	fenster(SBI_X,SBI_Y,SBI_X+SBI_DX,SBI_Y+SBI_DY,doppelt,SBI_WIN_COLOR);

	_fputstring(SBI_X+2,SBI_Y+2,"Soundblaster Configuration",SBI_WIN_COLOR);
	_fputstring(SBI_X+4,SBI_Y+4, SBI1 , SBI_WIN_COLOR);
	_fputstring(SBI_X+4,SBI_Y+6, SBI2 , SBI_WIN_COLOR);
	_fputstring(SBI_X+4,SBI_Y+8, SBI3 , SBI_WIN_COLOR);
   //	_fputstring(SBI_X+4,SBI_Y+10, SBI4 , SBI_WIN_COLOR);
   //	_fputstring(SBI_X+4,SBI_Y+12, SBI5 , SBI_WIN_COLOR);
   //_fputstring(5,5,SBI4,14);

   //	_fputstring(SBI_X+1,SBI_Y+4, ">>" , SBI_SELECT_COLOR);
   /*
	switch(SBAdr)
	{
	case 0x220: SBAdr_ii=0; break;
	case 0x240: SBAdr_ii=1; break;
	case 0x260: SBAdr_ii=2; break;
	}
	_fputstring(SBI_X+17+SBAdr_ii*9,SBI_Y+4,SBI10[SBAdr_ii],SBI_SELECT_COLOR);

	switch(SBDMA)
	{
	case 1:     SBDMA_ii=0; break;
	case 3:     SBDMA_ii=1; break;
	case 4:     SBDMA_ii=2; break;
	case 5:     SBDMA_ii=3; break;
	}
	_fputstring(SBI_X+18+SBDMA_ii*9,SBI_Y+6,SBI20[SBDMA_ii],SBI_SELECT_COLOR);

	switch(SBIRQ)
	{
	case 1:     SBIRQ_ii=0; break;
	case 2:     SBIRQ_ii=1; break;
	case 5:     SBIRQ_ii=2; break;
	case 7:     SBIRQ_ii=3; break;
	}
	_fputstring(SBI_X+18+SBIRQ_ii*9,SBI_Y+8,SBI30[SBIRQ_ii],SBI_SELECT_COLOR);

	switch(MixSpeed)
	{
	case 8000 :     SBMix_ii=0; break;
	case 20000:     SBMix_ii=1; break;
	case 30000:     SBMix_ii=2; break;
	case 44100:     SBMix_ii=3; break;
	}
	_fputstring(SBI_X+16+SBMix_ii*9,SBI_Y+10,SBI40[SBMix_ii],SBI_SELECT_COLOR);

	loop=1;
	while (loop==1)
	{
  while (!kbhit());
		Regs.h.ah = 0x10; 	int86 ( 0x16, &Regs, &Regs);
		//cr2=Regs.h.ah;
		switch(Regs.h.ah)
		{
		case tab: SBAdr_ii++; if (SBAdr_ii==3)  SBAdr_ii=0;
				SBAdr=stol(SBI10[SBAdr_ii],16);
				_fputstring(SBI_X+4,SBI_Y+4, SBI1 , SBI_WIN_COLOR);
				_fputstring(SBI_X+17+SBAdr_ii*9,SBI_Y+4,SBI10[SBAdr_ii],SBI_SELECT_COLOR);
				break;
		case enter:loop=255;
				break;
		}
	}

	_fputstring(SBI_X+1,SBI_Y+4, "  " , SBI_SELECT_COLOR);
	_fputstring(SBI_X+1,SBI_Y+6, ">>" , SBI_SELECT_COLOR);

	loop=1;
	while (loop==1)
	{
  while (!kbhit());
		Regs.h.ah = 0x10; 	int86 ( 0x16, &Regs, &Regs);
		//cr2=Regs.h.ah;
		switch(Regs.h.ah)
		{
		case tab: SBDMA_ii++; if (SBDMA_ii==4)  SBDMA_ii=0;
				SBDMA=atoi(SBI20[SBDMA_ii]);
				_fputstring(SBI_X+4,SBI_Y+6, SBI2 , SBI_WIN_COLOR);
				_fputstring(SBI_X+18+SBDMA_ii*9,SBI_Y+6,SBI20[SBDMA_ii],SBI_SELECT_COLOR);
				break;
		case enter:loop=255;
				break;
		}
	}

	_fputstring(SBI_X+1,SBI_Y+6, "  " , SBI_SELECT_COLOR);
	_fputstring(SBI_X+1,SBI_Y+8, ">>" , SBI_SELECT_COLOR);
	loop=1;
	while (loop==1)
	{
  while (!kbhit());
		Regs.h.ah = 0x10; 	int86 ( 0x16, &Regs, &Regs);
		//cr2=Regs.h.ah;
		switch(Regs.h.ah)
		{
		case tab: SBIRQ_ii++; if (SBIRQ_ii==4)  SBIRQ_ii=0;
				SBIRQ=atoi(SBI30[SBIRQ_ii]);
				_fputstring(SBI_X+4,SBI_Y+8, SBI3 , SBI_WIN_COLOR);
				_fputstring(SBI_X+18+SBIRQ_ii*9,SBI_Y+8,SBI30[SBIRQ_ii],SBI_SELECT_COLOR);
				break;
		case enter:loop=255;
				break;
		}
	}

	_fputstring(SBI_X+1,SBI_Y+8, "  " , SBI_SELECT_COLOR);
	_fputstring(SBI_X+1,SBI_Y+10, ">>" , SBI_SELECT_COLOR);
	loop=1;
	while (loop==1)
	{
  while (!kbhit());
		Regs.h.ah = 0x10; 	int86 ( 0x16, &Regs, &Regs);
		//cr2=Regs.h.ah;
		switch(Regs.h.ah)
		{
		case tab: SBMix_ii++; if (SBMix_ii==4)  SBMix_ii=0;
				MixSpeed=atoi(SBI40[SBMix_ii]);
				_fputstring(SBI_X+4,SBI_Y+10, SBI4 , SBI_WIN_COLOR);
				_fputstring(SBI_X+16+SBMix_ii*9,SBI_Y+10,SBI40[SBMix_ii],SBI_SELECT_COLOR);
				break;
		case enter:loop=255;
				break;
		}
	}

	for (i=SBI_Y; i<=SBI_Y+SBI_DY; i++)
	_fputstring(SBI_X,i,SBICLEAR,BLACK*16+BLACK);
	*/
return;
}


void main_loop(void)
{
  int eintrag;

  eintrag = 0;

  while (eintrag != 255)
  {
	eintrag = directory();
  }

  return;
}



void build_screen(void)
{
  int i;
  char string[100];
  sy_old = 0;  sy = 0;

  for (i=1; i<=80; i++)
  {
    putchr(i,1,' ', HEAD_COLOR);
    putchr(i,2,' ', HEAD_COLOR);
    putchr(i,50,' ', FOOT_COLOR);
  }
  _fputstring(1,1, HEADLINE, HEAD_COLOR);
  _fputstring(1,50, FOOTLINE, FOOT_COLOR);

  getcwd (startpath, alength);


  fenster(I_START_X,I_START_Y,I_START_X+I_DELTA_X,I_START_Y+I_DELTA_Y,castle1,I_WIN_COLOR);
  _fputstring(I_START_X+2,I_START_Y+2,"Modulname:",I_WIN_COLOR);
  _fputstring(I_START_X+2,I_START_Y+4,"Tracker:",I_WIN_COLOR);

  fenster(F_START_X,F_START_Y,F_START_X+F_DELTA_X,F_START_Y+F_DELTA_Y,doppelt,F_WIN_COLOR);
  _fputstring(SCROLL_X,F_START_Y+1,"",SCROLLCOLOR);
  for (i=F_START_Y+2; i<=F_START_Y+F_DELTA_Y-3; i++) _fputstring(SCROLL_X,i,"±",SCROLLCOLOR);
  _fputstring(SCROLL_X,F_START_Y+F_DELTA_Y-2,"",SCROLLCOLOR);

  anzlw = getdrives();
  lfw_start = lfw_neu = getdisk();
  return;
}

