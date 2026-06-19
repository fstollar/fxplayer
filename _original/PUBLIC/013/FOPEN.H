// Library fÅr Dateieinleseoperationen
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <dir.h>
#include "tasten.h"
#include "m_tool.h"
#include "dialog.h"
#include "ini.h"

// Farbendefinitionen
#define SCROLLCOLOR m.scrollcolor
#define MENUX       m.x
#define MENUSY      m.y_start
#define MENUEY      m.y_end
#define DELTAY      (MENUEY-MENUSY-2)
#define SCROLLX     24+MENUX
#define WINDOWCOLOR m.windowcolor
#define MENUNORMAL  files->color
#define MENUSELECT  m.selected

// Stellt den Scrollpunkt neu ein
#define SCROLLY    (DELTAY-2)
#define SHOWSCROLL _fputstring(SCROLLX,(char)(((float)((oa+ob)/(float)i))*SCROLLY)+MENUSY+2,"±",SCROLLCOLOR); _fputstring(SCROLLX,(char)(((float)((a+b)/(float)i))*SCROLLY)+MENUSY+2,"˛",SCROLLCOLOR)

// Stellt den MenÅpunkt dar
#define SHOWNORMAL(string,nummer) _fputstring(MENUX+1,MENUSY+1+nummer,string,MENUNORMAL)
#define SHOWSELECT(string,nummer) _fputstring(MENUX+1,MENUSY+1+nummer,string,MENUSELECT)

struct filestruct {
	 void *lastfile;
	 unsigned long number;
	 int color;
	 char name[40];
	 void *nextfile;
		 };

struct wildcatstruct {
	 int number;
	 char *wildcat;
	  };

struct menustruct {
	 int x_max, y_max, statcolor;
	 int x, y_start, y_end;
	 int windowcolor, scrollcolor, selected, normal;
	 char directory[100];
	 struct wildcatstruct wildcat;
	  };

struct filestruct *files, *firstfile;
char drives[25][41];
int g, counter=0;

// Funktionen zum Initialisieren
void setsize(struct menustruct *m,const int xm,const int ym,const int stcol)
{
  m->x_max=xm;
  m->y_max=ym;
  m->statcolor=stcol;
}
// öbergabe: X, Y1, Y2
void setpos(struct menustruct *m,const int x,const int y_start,const int y_end)
{
  m->x=x; m->y_start=y_start; m->y_end=y_end;
  m->wildcat.wildcat="*.*"; m->wildcat.number=1;
}

// öbergabe: Fenster-, Selected-, Unselected-, Scrollcolor
void setcolor(struct menustruct *m, int wc, int sel, int nor, int sc)
{
  m->windowcolor=wc; m->scrollcolor=sc; m->selected=sel; m->normal=nor;
}

// öbergabe: Verzeichnis
void setdirectory(struct menustruct *m, char *dir)
{
  strcpy(m->directory,dir);
}

// Wechselt Laufwerk mit ALT-L
int changedrive(void)
{
 return 0;
}

// Stezt Filepointer
struct filestruct *setmenupkt(unsigned long num)
{
  if (num==files->number) return files;
  if (num<files->number) {
    while ((files->number) > num) files=files->lastfile;
    return files;
  }
  while (files->number<num) files=files->nextfile;
  return files;
}

// Sortiert die eingelesenen Dateien
void sort(int anz)
{
  int t, i;
  char string[40];
  for (i=0; i<anz; i++)
    for (t=0; t<(anz-i); t++)
	 if (strcmp(setmenupkt(t)->name,setmenupkt(t+1)->name)>0) {
	strcpy(string,setmenupkt(t)->name);
	strcpy(setmenupkt(t)->name,setmenupkt(t+1)->name);
	strcpy(setmenupkt(t+1)->name,string);
	 }
}

// Schreibt Grî·e, Datum und Zeit der Datei in einen String
char *setdt(struct ffblk file)
{
  char string[40], str[13];

  if (file.ff_attrib &= FA_DIREC) strcpy(string," <DIR>    ");
  else {
   strcpy(string,"          ");
   ltoa((file.ff_fsize + 512)>> 10,str,10);
   string[9-strlen(str)]='\0';
   strcat(string,str);
   strcat(string,"K");
  }

  return string;
}

// Liest aktuelles Verzeichnis aus
int readdir(char *filetoload, char *wildcat)
{
  int i, done, t;
  struct ffblk file;
  struct filestruct *filepointer;
  char *string;

// Falls keine Daten neuen Pointer anlegen, sonst alles freigeben
  if (files!=NULL) {
    i=0;
    files=firstfile;
    while (files!=NULL) {
      i++;
      filepointer=files->nextfile;
      free(files);
      files=filepointer;
    }
  }
  files=(struct filestruct *) malloc(sizeof(struct filestruct));
  firstfile=files;
  if (files!=NULL)
      files->lastfile=NULL;
  files->nextfile=NULL;

// Dateien lesen und in files[] speichern // i=Anzahl der gefundenen Dateien
  done=findfirst(filetoload,&file,FA_DIREC+FA_ARCH+FA_RDONLY+FA_HIDDEN);
  i=0;
  while (!done)
  {
    if (i!=0) {
      files->nextfile=(struct filestruct *) malloc(sizeof(struct filestruct));
      filepointer=files;
      files=files->nextfile;
      if (files!=NULL)
         files->lastfile=filepointer;
      files->nextfile=NULL;
    }
    if (file.ff_attrib &= FA_DIREC)
    {
// Behandlung der Datei als Verzeichnis
      strupr(file.ff_name);
      if (strcmp(file.ff_name,"."))
      {
	*(files->name)=16;
	*(files->name+1)='\0';
	strncat(files->name,file.ff_name,strlen(file.ff_name));
	strncat(files->name,"             ",13-strlen(file.ff_name)-1);
	files->number=i;
	files->color=LIGHTBLUE+16*LIGHTGRAY;
	i++;
      }
    } else {
// Behandlung der Datei als File
      strcpy(files->name,file.ff_name);
      strncat(files->name,"             ",13-strlen(file.ff_name));
      files->number=i;
	 string=strchr(file.ff_name,'.');
      if (!strcmp(string+1,"S3M")) files->color=BROWN+16*LIGHTGRAY;
      else files->color=BLACK+16*LIGHTGRAY;
      i++;
    }
// Datum und Zeit an den String hÑngen
    if (strcmp(file.ff_name,".")) {
      i--;
      string=setdt(file);
      strcat(files->name,string);
      i++;
    }
    done=findnext(&file);
  }
  i--;
// Dateien sortieren
//  sort(i-1);
  for (t=0; t<=g; t++) {
    files->nextfile=(struct filestruct *) malloc(sizeof(struct filestruct));
    filepointer=files;
    files=files->nextfile;
    if (files!=NULL)
       files->lastfile=filepointer;
    files->nextfile=NULL;
    files->number=i+t+1;
    files->color=RED+16*LIGHTGRAY;
    strcpy(files->name,drives[t]);
  }
  i=i+g+1;
  wildcat++;
  return (i);
}

// Setzt aktuelles Verzeichnis bei Verzeichnis-, Laufwerkswechsel
int setdir(char *dir, char *verzeichnis)
{
  if (!((dir[0]==' ') | (dir[0]==16))) return (0);
  else {
// Verzeichnis zurÅck
    if (!strncmp(dir+1,"..",2)) {
      *(verzeichnis+strlen(verzeichnis)-1)='\0';
      *(strrchr(verzeichnis,'\\')+1)='\0';
      return (-1);
    }
// Anderes Laufwerk
    else if (!strncmp(dir,"   [ ",5)) {
      verzeichnis[0]=dir[5];
      verzeichnis[1]='\0';
      strcat(verzeichnis,":\\");
      return(1);
    } else {
// Verzeichnisebene tiefer
      strncat(verzeichnis,dir+1,strchr(dir,' ')-(dir+1));
      strcat(verzeichnis,"\\");
      return (-2);
    }
  }
}

// Statusinfofenster fÅllen
void showinfo(char *filename)
{
  unsigned int flags, insnum, patnum, ordnum;
  char S3MHeader[0x60];
  char modulname[30], version[20], s[10];
  FILE *file;

  if ((file=fopen(filename,"rb"))==NULL)
  {
// Verzeichnis oder Laufwerk
    modulname[0]='\0';
    strcpy(modulname,"                              ");
    version[0]='\0';
    strcpy(version,"                   ");
  } else {
   _fputstring(14,5,"Reading...                    ",BLACK+16*RED);
   fread(S3MHeader,0x60,1,file);

    if ((S3MHeader[0x1c]!=0x1a)|(S3MHeader[0x1d]!=0x10)|(strncmp(&S3MHeader[0x2c],"SCRM",4)))
    {
// Kein S3M-File
	 modulname[0]='\0';
	 strcpy(modulname," N O   S 3 M - F i l e !      ");
	 version[0]='\0';
	 strcpy(version,"                   ");
    } else {
// S3M-File
	 modulname[0]='\0';
	 strncat(modulname,S3MHeader,28);
	 strncat(modulname,"                              ",30-strlen(S3MHeader));
	 modulname[30]='\0';
	 version[0]='\0';
//	 ultoa(S3MHeader[0x28]+S3MHeader[0x29]*256,version,10);
  if ((S3MHeader[0x29]>4)==1) strcpy(version,"ScreamTracker ");
  ultoa((S3MHeader[0x29] & 0x0f),s,16);
  strcat(s,".");
  strcat(version,s);
  s[0]='\0';
  ultoa(S3MHeader[0x28],s,16);
  if (strlen(s)==1) strcat(version,"0");
  strcat(version,s);
    }
  }

// Bildschirmausgabe
  _fputstring(14,5,modulname,LIGHTRED+16*RED);
  _fputstring(14,7,version,LIGHTRED+16*RED);

  fclose(file);
}

// ************ HauptFunktion ***********************

char far *load(struct menustruct m)
{
// Variablendefinition
  int a, b, oa, ob;
  int stop, i, t;
  char verzeichnis[500], filetoload[513], filename[513];
  struct filestruct *filepointer;
  long *dostime, oldtime;

// Initialisierung
// Screen und Variablen initialisieren
  cursor(unsichtbar);

  a=0; b=0;
  oa=0; ob=0;
  stop=0;

// Strings initialisieren
  strcpy(verzeichnis,m.directory);
  strupr(verzeichnis);
  if (!(verzeichnis[(strrchr(verzeichnis,'\\')-verzeichnis)+1]=='\0')) strcat(verzeichnis,"\\");

  if (!strcmp(verzeichnis,"")) {
    verzeichnis[0]=(getdisk()+'A');
    verzeichnis[1]='\0';
    strcat(verzeichnis,":\\");
  }
  strcpy(filetoload,verzeichnis);

  strcat(filetoload,"*.*");

// Laufwerke ermitteln
  g=0;
  i=getdisk();
  for (t=0; t<26; t++) {
    setdisk(t);
    if (t==getdisk()) {
	 strcpy(drives[g],"   [  : ]              ");
	 drives[g][5]=t+65;
	 g+=1;
    }
  }
  g--;
  setdisk(i);

// Verzeichnis auslesen
  m.wildcat.number++;
  m.wildcat.number--;
  strupr(m.wildcat.wildcat);
  i=readdir(filetoload,m.wildcat.wildcat);

  strcpy(filename,verzeichnis);
  strcat(filename,firstfile->name);
  strchr(filename,' ')[0]='\0';

// Bildschirmaufbau
// Infoline oben und unten
  for (t=1; t<=m.x_max; t++) {
    putchr(t,1,' ',m.statcolor);
    putchr(t,2,' ',m.statcolor);
    putchr(t,m.y_max,' ',m.statcolor);
  }

  _fputstring(1,1," F/X Player - Version 0.13‡   FMS + 16 Bit",m.statcolor);
  _fputstring(2,2,verzeichnis,m.statcolor);
  _fputstring(1,m.y_max," (C) 1996 by Apollo & Jakl, the Great ",m.statcolor);

// Statusinfofenster
  fenster(1,3,55,12,castle1,LIGHTRED+16*RED);

  _fputstring(3,5,"Modulname:",LIGHTRED+16*RED);
  _fputstring(3,7,"Tracker:",LIGHTRED+16*RED);

// SelectmenÅ
  fenster(MENUX,MENUSY,SCROLLX,MENUEY,doppelt,WINDOWCOLOR);

  _fputstring(SCROLLX,MENUSY+1,"",SCROLLCOLOR);
  for (t=MENUSY+2; t<=MENUEY-2; t++) _fputstring(SCROLLX,t,"±",SCROLLCOLOR);
  _fputstring(SCROLLX,MENUEY-1,"",SCROLLCOLOR);
  SHOWSCROLL;

  SHOWSELECT(setmenupkt(0)->name,0);
  if (i>0) for (t=1; (t<=i) & (t<=DELTAY); t++) {
    setmenupkt(t);
    SHOWNORMAL(files->name,t);
  }

// Ticks initialisieren
  dostime=MK_FP(0x40,0x6c);

  oldtime=*dostime;

// MenÅ
  while (!stop)
  {
    oa=a; ob=b;
    while (!kbhit()) if (((*dostime)-(oldtime))==7) showinfo(filename);
    switch (getch()) {
	case down   : if (a>=DELTAY) {
			if (a+b<i) b++;
			 } else if (a+b<i) a++; break;
	case up     : if (a<1) {
			if (b!=0) b--;
			 } else a--; break;
	case enter  : if (!setdir(setmenupkt(a+b)->name,verzeichnis)) {
			stop=a+b+1;
   setdirectory(&m,verzeichnis);
			strncat(verzeichnis,setmenupkt(a+b)->name,strchr(setmenupkt(a+b)->name,' ')-setmenupkt(a+b)->name);
			strcpy(filetoload,verzeichnis);
			strupr(filetoload);
   showinfo(filetoload);
			 } else {
			strcpy(filetoload,verzeichnis);
			strcat(filetoload,"*.*");
			a=0; b=0;
			SHOWSCROLL;
			i=readdir(filetoload,m.wildcat.wildcat);
			for (t=2; t<=m.x_max; t++) putchr(t,2,' ',m.statcolor);
			_fputstring(2,2,verzeichnis,m.statcolor);
			oa=0; ob=0;
			for (t=1; (t<=i) & (t<=DELTAY); t++)
			{
			  setmenupkt(t);
			  SHOWNORMAL(files->name,t);
			}
			for (a=t; a<=DELTAY; a++) SHOWNORMAL("                       ",a);
			a=0;
			SHOWSELECT(setmenupkt(a+b)->name,a);
			 } break;
	case alt_x  :
	case escape : {
			stop=-1;
			filetoload[0]='\0';
			 } break;
	case ctrl_pgdn :
	case end    : if (i<=DELTAY) a=i;
			 else { a=DELTAY; b=i-DELTAY; } break;
	case ctrl_pgup :
	case home   : { a=0; b=0; } break;
	case pgup   : if (b==0) a=0; else {
			if (b>DELTAY) b=b-DELTAY; else b=0;
			 } break;
	case pgdn   : if (b==(i-DELTAY)) a=DELTAY; else {
			if ((b<(i-(DELTAY+1)*2)) && ((i-(DELTAY+1)*2)>0)) b=b+DELTAY; else b=i-DELTAY;
			if (b<0) {
			  b=0;
			  a=i;
			}
			 } break;
	case ctrl_home : a=0; break;
	case ctrl_end : if (i>DELTAY) a=DELTAY; else a=i; break;
	case alt_l  : if (!changedrive()) {}
    }

// MenÅpunkte auf Bildschirm ausgeben
    if ((oa!=a) && (ob==b))
    {
	 setmenupkt(oa+ob);
	 SHOWNORMAL(setmenupkt(oa+ob)->name,oa);
	 SHOWSELECT(setmenupkt(a+b)->name,a);
    }

    if (ob!=b)
    {
	 for (t=0; t<=DELTAY; t++) {
	setmenupkt(b+t);
	SHOWNORMAL(files->name,t);
	 }
	 SHOWSELECT(setmenupkt(a+b)->name,a);
    }
    SHOWSCROLL;

    strcpy(filename,verzeichnis);
    strcat(filename,files->name);
    strchr(filename,' ')[0]='\0';
    oldtime=*dostime;
  }

// Programmende
  files=firstfile;
  while (files!=NULL) {
    filepointer=files->nextfile;
    free(files);
    files=filepointer;
  }

  cursor(strich);
  return filetoload;
}
