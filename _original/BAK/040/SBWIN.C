char far SBI1[] = {"Address    ( 220 )  ( 240 )  ( 260 )"};

void far SB_Window(void)
{

	int i, loop;
	int SBAdr_ii, SBDMA_ii, SBIRQ_ii, SBMix_ii;

	fenster(SBI_X,SBI_Y,SBI_X+SBI_DX,SBI_Y+SBI_DY,doppelt,SBI_WIN_COLOR);

	_fputstring(SBI_X+2,SBI_Y+2,"Soundblaster Configuration",SBI_WIN_COLOR);
	_fputstring(SBI_X+4,SBI_Y+4,SBI1, SBI_WIN_COLOR);
	_fputstring(SBI_X+4,SBI_Y+6,"DMA        (  1  )  (  3  )  (  5  )  (  7  )" , SBI_WIN_COLOR);
	_fputstring(SBI_X+4,SBI_Y+8,"IRQ        (  1  )  (  2  )  (  4  )  (  5  )" , SBI_WIN_COLOR);
	_fputstring(SBI_X+4,SBI_Y+10,"MixSpeed   ( 8000)  (20000)  (30000)  (44100)" , SBI_WIN_COLOR);
	_fputstring(SBI_X+4,SBI_Y+12, "Self-def. Mixing-Speed: -----" , SBI_WIN_COLOR);

	_fputstring(SBI_X+1,SBI_Y+4, ">>" , SBI_SELECT_COLOR);

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

