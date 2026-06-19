#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include "m_tool.h"
//#include "tasten.h"


struct d_menu {
	 int number;
	 int aktpos;
	 char *data;
       };
struct d_select {
	 int number;
	 int aktpos;
	 char *data;
       };
struct dialogdatastruct {
	 void *lastdata;
	 int kind;
	 int active;
	 int color;
	 int x1, y1;
	 void *kinddata;
	 void *nextdata;
       };
struct dialogstruct {
	 int color;
	 int x1, y1, x2, y2;
	 struct dialogdatastruct *dialogdata;
       };

// Setzt den Dialog: *Dialogstruct; (X1/Y1) - (X2/Y2); Farbe; *Dialogdaten
struct dialogstruct *setdialog(int x1,int y1,int x2,int y2,int color,struct dialogdatastruct *dd)
{
  struct dialogstruct *d;
  d=malloc(sizeof(struct dialogstruct));
  d->x1=x1;
  d->y1=y1;
  d->x2=x2;
  d->y2=y2;
  d->color=color;
  d->dialogdata=dd;
  d->dialogdata->active=1;
  return d;
}

// Setzt Dialogpkt: Art; *Artdaten; Farbe; (X1/Y1); N„chste Dialogdaten
struct dialogdatastruct *setdialogdata(int kind,void *kinddata,int color,int x1,int y1,struct dialogdatastruct *dd)
{
  struct dialogdatastruct *dialogdata, *dialogdata2;
  dialogdata=malloc(sizeof(struct dialogdatastruct));
  dialogdata->kind=kind;
  dialogdata->active=0;
  dialogdata->color=color;
  dialogdata->x1=x1;
  dialogdata->y1=y1;
  dialogdata->kinddata=kinddata;

  dialogdata->nextdata=dd;
  if (dd!=NULL) {
    dd->lastdata=dialogdata;

    dialogdata2=dialogdata;
    while (dialogdata2->nextdata!=NULL) dialogdata2=dialogdata2->nextdata;
    dialogdata->lastdata=dialogdata2;
    dialogdata2->nextdata=dialogdata;
  }
  return dialogdata;
}

void freedialog(struct dialogstruct *d)
{
  struct dialogdatastruct *dd, *dd2, *firstdd;

  dd=d->dialogdata;
  dd2=dd->nextdata;
  firstdd=dd;

  free(d);

  free(dd->kinddata);
  free(dd);
  dd=dd2;
  while (dd!=firstdd) {
    dd2=dd->nextdata;
    free(dd->kinddata);
    free(dd);
  }
}
