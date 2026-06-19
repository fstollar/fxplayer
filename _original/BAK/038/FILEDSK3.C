//***************************************************************************
//******************* OberflÑche fÅr F/X Player v0.20‡ **********************
//*** made by STIGMA in 1996 in a great desire of laziness............... ***

int sortfkt ( const void *a, const void *b);
void far show_files (int start, int max, int select);
int strmid ( char *dest, char *source, int offset, int len);
void far showinfo (void);
void far SB_Window (void);
void build_screen (void);
int far directory (void);
int getdrives (void);

// *** MAIN LOOP ***

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

// **************************************************************************
// *** SECOND MAIN

int far directory(void)
{
  struct ffblk ffblk;
  int done,ii,i;
  char string[25], str[10], ch, cr1, cr2, *d;

  getcwd (aktpath, alength);
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
    sy = (int) ( ( (float)(eintrag_akt) / (max_eintrag) ) * (F_DELTA_Y - 5) );
    _fputstring(SCROLL_X, sy_old + F_START_Y + 2,"±",SCROLLCOLOR);
    _fputstring(SCROLL_X, sy + F_START_Y + 2,"˛",SCROLLCOLOR);

    oldtime=*dostime;

    while (!kbhit()) {
//				   gotoxy(38,2); printf("%010lu",(Takt()-17) );
                       if (((*dostime)-(oldtime))==8) showinfo();
                     }

    Regs.h.ah = 0x10;
    int86 ( 0x16, &Regs, &Regs);
    cr1=Regs.h.al; cr2=Regs.h.ah;

    switch (cr1)
	  {
	case alt  :  switch (cr2)
			{
			 case chr_x  : strcpy(file_to_load,"\0"); file_selected=255;
				    break;
    case chr_m  : ChangeMode(); break;
    case chr_i  : ChangeInterp(); break;
    case chr_s  : OldScreen();
                  ClrScreen();
                  printf("Type 'EXIT' to return to F/X Player v0.38‡\n");
                  test=CallCommand(); if (test!=0) {gotoxy(1,10); printf("ERROR\n");}
                  NewScreen();
                  cursor(unsichtbar);
                  build_screen();
                  show_files(eintrag_oben, max , 0);
                  getcwd( aktpath, alength);
                  for (i=1; i<=80; i++)
                  {
                     putchr(i,2,' ', HEAD_COLOR);
                  }
                  _fputstring( 2,2, aktpath, HEAD_COLOR);
                  file_selected = 1;
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

     case right    : PlayFF(); break;

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
     if (strlen(aktpath)<3) strcat ( aktpath, "\\");
				}
			  else
			  {
			   if (strlen(aktpath) > 3) strcat( aktpath, "\\");
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
			 if (strlen(aktpath) > 3) strcat( file_to_load , "\\");
			 strmid( dest, fname[eintrag_akt] , 1, 13);
			 strcat( file_to_load, dest);
			 showinfo();
			 // file_selected = 10;
       run_s3m();
			 }
    break;
//	case f10:  SB_Window();  break;
	 case tab : Pause(); break;
	   }
 }
return file_selected;
}




void far show_files(int start, int max, int select)
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
return;
}


// *************************************************************************
// ***                   Statusinfofenster fÅllen                        ***

void far showinfo(void)
{
  unsigned int flags;
  char modulname[35], version[35], s[5], *d;
  char dumm[50], dumm2[50], nowpath[80];

 strcpy(modulname," N O   S 3 M - F i l e !   ");
 strcpy(version,  "                           ");

 strmid( dumm, fname[eintrag_akt] , 1, 13);
 d=strchr(dumm,'.');
 strmid( ext, d, 1, 3);
 if (!(strcmp(ext,"S3M")))     //Ist es Åberhaupt ein S3M
 {
  getcwd( nowpath, alength);
  strcpy( dumm , nowpath);
  if (strlen(nowpath) > 4) strcat( dumm , "\\");
  strmid( dumm2, fname[eintrag_akt] , 1, 13);
  strcat( dumm, dumm2);

  if ((LoadS3M_Head(dumm,S3MHeader))==NULL)
  {
    _fputstring(14,5,"Reading...                 ",BLACK+16*BLUE);
    _fputstring(14,7,version,BLACK+16*BLUE);

    if ((S3MHeader[0x1c]==0x1a)|(S3MHeader[0x1d]==0x10)|(strncmp(&S3MHeader[0x2c],"SCRM",4)))
    {
    // S3M-File
	strncpy(modulname,S3MHeader,28);
	strncat(modulname,"                         ",30-strlen(S3MHeader));

	flags=(S3MHeader[0x29]>>4);
	switch (flags) {
	    case 1 : strcpy(version,"ScreamTracker "); break;
	    case 2 : strcpy(version,"F/X Tracker "); break;
     case 3 : strcpy(version,"Impuls Tracker "); break;
	    default: strcpy(version,"UNKOWN TRACKER "); break;
		    }
	ultoa((S3MHeader[0x29] & 0x0f),s,16);
	strcat(s,".");
	strcat(version,s);
	ultoa(S3MHeader[0x28],s,16);
	if (strlen(s)==1) strcat(version,"0");
	strcat(version,s); strcat(version,"   ");
    }

  }
 }
// Bildschirmausgabe
_fputstring(14,5,modulname,I_WIN_COLOR);
_fputstring(14,7,version,I_WIN_COLOR);

return;
}

// **************************************************************************
// ***                         Bildschirmaufbau                           ***

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


  fenster(I_START_X,I_START_Y,I_START_X+I_DELTA_X,I_START_Y+I_DELTA_Y,castle1,I_WIN_COLOR);
  _fputstring(I_START_X+2,I_START_Y+2,"Modulname:",I_WIN_COLOR);
  _fputstring(I_START_X+2,I_START_Y+4,"Tracker:",I_WIN_COLOR);

  fenster(F_START_X,F_START_Y,F_START_X+F_DELTA_X,F_START_Y+F_DELTA_Y,doppelt,F_WIN_COLOR);
  _fputstring(SCROLL_X,F_START_Y+1,"",SCROLLCOLOR);
  for (i=F_START_Y+2; i<=F_START_Y+F_DELTA_Y-3; i++) _fputstring(SCROLL_X,i,"±",SCROLLCOLOR);
  _fputstring(SCROLL_X,F_START_Y+F_DELTA_Y-2,"",SCROLLCOLOR);

  anzlw = getdrives();
  return;
}
 

// **************************************************************************
// ***                     kleine Routinen                                ***

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


int sortfkt(const void *a, const void *b)
{
   return( strcmp((char *)a , (char *)b) );
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


//#include"sbwin.c";

