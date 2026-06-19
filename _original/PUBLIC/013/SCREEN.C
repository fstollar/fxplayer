// Screen function for FX-Player

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
 Regs.x.ax=0x1003;
 Regs.h.bh=0;
 int86 ( 0x10 , &Regs,&Regs);
 return;
}
