#ifndef __FX_H
#define __FX_H
// Konstanten fÅr Procedure fenster
#define einfach  "⁄ƒø≥Ÿƒ¿≥"
#define doppelt  "…Õª∫ºÕ»∫"
#define castle1  "˛ﬂ˛ﬁ˛‹˛›"
#define castle2  "˛‹˛›˛ﬂ˛ﬁ"

#define HEADLINE " F/X Player - Version 0.20‡       16 Bit + FMS                 6th October 1996"
#define HEAD_COLOR LIGHTGRAY*16+RED
#define FOOTLINE " (c) by Apollo, Pebble & Jakob                              a STIGMA Produktion"
#define FOOT_COLOR LIGHTGRAY*16+RED

// Konstanten fÅr Procedure cursor
#define block       0x000f
#define unsichtbar  0xfeff
#define strich      0x0607

// Konstanten fÅr File-Window
#define F_START_X   55
#define F_START_Y   3
#define F_DELTA_X   26
#define F_DELTA_Y   47
#define SCROLL_X    F_START_X + F_DELTA_X - 1
#define F_WIN_COLOR LIGHTGRAY*16+BLACK

#define NORMAL_COLOR  LIGHTGRAY*16+BLACK
#define SELECT_COLOR  GREEN*16+RED
#define S3M_COLOR     LIGHTGRAY*16+BROWN
#define DIR_COLOR     LIGHTGRAY*16+LIGHTBLUE
#define LFW_COLOR     LIGHTGRAY*16+MAGENTA
#define SCROLLCOLOR   LIGHTGRAY*16+DARKGRAY

// Konstanten fÅr Info-Window
#define I_START_X   1
#define I_START_Y   3
#define I_DELTA_X   54
#define I_DELTA_Y   9
#define I_WIN_COLOR BLUE*16+YELLOW

// Konstanten fÅr SB-Config-Window
#define SBI_X         2
#define SBI_Y        30
#define SBI_DX       52
#define SBI_DY       15
#define SBI_WIN_COLOR    BLUE*16+YELLOW
#define SBI_SELECT_COLOR BLUE*16+RED
#define SBICLEAR     "                                                     "
#define SBI1         "Address    ( 220 )  ( 240 )  ( 260 )"
#define SBI2         "DMA        (  1  )  (  3  )  (  5  )  (  7  )"
#define SBI3         "IRQ        (  1  )  (  2  )  (  4  )  (  5  )"
#define SBI4         "MixSpeed   ( 8000)  (20000)  (30000)  (44100)"
#define SBI5         "Self-def. Mixing-Speed: -----"

#endif
