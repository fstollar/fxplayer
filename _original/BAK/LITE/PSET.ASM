;*********************************************************
;*** Routinen zur allgemeinen Soundblaster Bearbeitung ***
;***         v1.1 am 01.02.96 fr F/X-Tracker         ***

SB16Flag     db ?
SBstereoFlag db ?

SBIO         dw ?
SBIOReset    dw ?
SBIOC        dw ?
SBIOE        dw ?
SBMixAdr     dw ?
SBMixDat     dw ?
SBIRQ        db ?
SBDMA        dw ?
MixSpd       dw ?

IRQAdr       dw ?
DSPVersion   dw 0
SBArt        db 0

;********************************

_InitSB     proc near

sframe      struc
bp0         dw ?
ret_adr0    dw ?        ;Rcksprungadresse
BaseAdress  dw ?        ;BasisAdresse von SB (meist 220h)
IRQAdress   dw ?        ;IRQ Nummer der SB (0-oo)
DMAChannel  dw ?        ;DMA Kanal der SB (0-3=8 Bit 4-7=16 Bit)
MixSpeed    dw ?        ;MixingSpeed in Hz (5000-44100 Hz)
sframe      ends

frame       equ [ bp - bp0 ]

            push bp
            mov bp,sp
            push bx
            push dx

            mov ax,frame.MixSpeed
            mov Mixspd,ax         ; Einstellungen von SB und Mixer
            mov ax,frame.IRQAdress
            mov SBIRQ,al
            mov ax,frame.BaseAdress
            mov SBIO,ax
            mov ax,frame.DMAChannel
            xor ah,ah
            mov SBDMA,ax
            shr al,2          ;Ist DMA 4 oder h”her
            mov SB16Flag,al   ;Dann Flag=1 sonst =0

            mov cs:SBBlock,0  ;PlayBlock auf 0 festlegen
;*******************************
            mov ax,SBIO
            add ax,04h
            mov SBMixAdr,ax
            add ax,01h
            mov SBMixDat,ax
            add ax,01h
            mov SBIOReset,ax
            add ax,06h
            mov SbIOC,ax
            add ax,02h
            mov SbIOE,ax

            call SBReset   ; SB Reset durchfhren
            cmp al,0AAh    ; keine SB wenn nicht AAh
            jnz @@noReset    ; zurck geliefert wird

            mov ah,0E1h    ; Versionsnummer DSP abfragen
            call WrDSP
            call RdDSP     ; Hauptversionsnummer
            mov ah,al
            call RdDSP     ; Unterversionsnummer
            mov DSPVersion,ax

            cmp ax,512
            jg no200     ; SB 1.5 - SB 2.0
            mov SBArt,1  ; MONO 8 Bit 4000-23000 Hz
            jmp endsp
     no200:
            cmp ax,768   ; SB 2.01 - SB 2.5
            jge no201    ; MONO 8 Bit 4000-23000 Hz
            mov SBArt,2  ; MONO HISPEED 8 Bit 23000-44100 Hz
            jmp endsp
     no201:
            cmp ax,1024  ; SB PRO 2.0
            jge no3xx    ; MONO 8 Bit 4000-23000 Hz
            mov SBArt,3  ; MONO HISPEED 8 Bit 23000-44100 Hz
            jmp endsp    ; STEREO HISPEED 8 Bit 11025-22050 Hz
     no3xx:
             mov SBArt,4 ; SB PRO 2.5
                         ; MONO 8 Bit 5000-44100 Hz
                         ; STEREO 8 Bit 5000-44100 Hz

             mov SBArt,5 ; SB 16 (ASP)
                         ; MONO 8/16 Bit 5000-44100 Hz
     endsp:              ; STEREO 8/16 Bit 5000-44100 Hz
            pop dx
            pop bx
            pop bp
            ret

@@noReset:
            pop dx
            pop bx
            pop bp
            mov ax,0ffffh
            ret

_InitSB     endp

;*** Soundblaster Reseten ***

_ResetSB    proc near

            pushad
            push es

            mov es,PlayBlockSeg

                     mov ax,PBlockLength/16
                      mov cl,SB16Flag
                      shl ax,cl
                     mov cx,ax
                     mov eax,32767*65536+32767
                     xor di,di
      @@InitAgain2:
                     mov es:[di+0],eax
                     mov es:[di+4],eax
                     mov es:[di+8],eax
                     mov es:[di+12],eax
                     add di,16
                     dec cx
                     jnz @@Initagain2

            call SBReset
            cmp al,0aah
            jnz @@noReset

            mov cs:SBBlock,0

            pop es
            popad
            mov ax,0000
            ret

@@noReset:
            pop es
            popad
            mov ax,0ffffh
            ret

_ResetSB    endp

;*** Soundblaster Reset durchfhren ***

SBReset     proc near

            push dx
            push cx

;*********** SB Reset ****

            mov dx,SbIOReset
            mov al,1
            out dx,al      ; erst 1 schreiben

            mov dx,50h
            mov ax,86h
            int 15h        ; >5 Millisek warten

            mov dx,SbIOReset
            mov al,0
            out dx,al      ; darauf 0

            mov dx,50
            mov ax,86h
            int 15h        ; Nochmal warten

            mov dx,SbIOE
            xor cx,cx
Nochnicht:
            xor al,al
            in al,dx       ; Schon fertig mit Reset
            test al,80h
            jnz ByteReady
            loop Nochnicht ; 65536 mal fragen ob fertig
            mov al,0       ; sonst keine SB vorhanden
            jmp NoDSP
ByteReady:
            sub dx,04h
            in al,dx       ; auslesen
NoDSP:                     ; keine Reset wenn al<>AAh
            pop cx
            pop dx
            ret

SbReset     endp

;*********** Routine die den DSP der Soundblaster beschreibt *********
;* IN:  ah = byte to write to Sound Blaster DSP
;* OUT: -

WrDSP       proc near

            push ax
            push dx

            mov dx,cs:SbIOC
notr:
            in al,dx
            and al,128
            jnz notr

            mov al,ah
            out dx,al

            pop dx
            pop ax
            ret

WrDSP       endp

;*********** Routine die den DSP der Soundblaster liest *********

RdDSP       proc near

            push dx

            mov dx,cs:SbIOE
notr2:
            in al,dx
            and al,128
            jz notr2

            sub dx,4
            in al,dx

            pop dx
            ret

RdDSP       endp
