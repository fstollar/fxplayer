union REGS Regs; //Interrupt Register

void OldScreen(void)
{
 //* Alte Aufl”sung 80x25 *

 Regs.x.ax=0x1114;
 Regs.x.bx=0x0000;
 int86 ( 0x10 , &Regs,&Regs);
 return;
}

void NewScreen(void)
{
  //*** VGA-TextModus 80x43/50 ***

 Regs.x.ax=0x0003;
 int86 ( 0x10 , &Regs,&Regs);
 Regs.x.ax=0x1112;
 Regs.x.bx=0x0000;
 int86 ( 0x10 , &Regs,&Regs);
 //Regs.x.ax=0x1003;
 //Regs.h.bh=0;
 //int86 ( 0x10 , &Regs,&Regs);
 return;
}

void far fenster(int x1,int y1,int x2,int y2,char rahmen[8],int farbe)
{
 unsigned int i, t;

 y2--; x2--;
 /* Waagrecht oben*/
 putchr(x1,y1,rahmen[0],farbe);
 for (i=1; i<x2-x1; i++) putchr(x1+i,y1,rahmen[1],farbe);
 putchr(x2,y1,rahmen[2],farbe);
 /*Senkrecht*/
 for (i=1; i<y2-y1; i++)
 {
   putchr(x1,y1+i,rahmen[7],farbe);
   for (t=x1+1; t<x2; t++) putchr(t,y1+i,' ',farbe);
   putchr(x2,y1+i,rahmen[3],farbe);
 }
 /* Waagrecht unten*/
 putchr(x1,y2,rahmen[6],farbe);
 for (i=1; i<x2-x1; i++) putchr(x1+i,y2,rahmen[5],farbe);
 putchr(x2,y2,rahmen[4],farbe);

return;
}



void cursor(unsigned int kind)
{
 asm {
     mov    cx,[kind]
     mov    ax,0x0100
     int    16
     }
return;
}
