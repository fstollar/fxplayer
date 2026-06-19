;****************************************************
;*** Soundblaster Player Routinen fÅr F/X-Tracker ***
;**** > C-Version  v1.1

IGROUP group _text
DGROUP group cons, _bss, _data
       assume CS:IGROUP, ds:DGROUP, es:DGROUP, ss:DGROUP

CONS   segment word public 'CONS'
CONS   ends

_BSS   segment word public 'BSS'
_BSS   ends

_DATA  segment word public 'DATA'

_DATA  ends

;********

_DATA segment word public 'DATA'

_DATA       ends

;********************************

_TEXT  segment byte public 'CODE'

public      _InitSB
public      _InitPlay
public      _ClosePlay
public      _StartPlay
public      _StopPlay

;------------------------------

.486
.model small

INCLUDE Pset.ASM

PBlockLength   equ 02000h
MBlockLength   equ 02000h

PlayBlockSeg   dw 0   ;*Segment wo SB PlayBlock liegt (ohne DMA Åberlauf)
ChannelsSeg    dw 0   ;*Segment wo ChannelBlock=8 KB liegt

PatternAktSeg  dw 0
PatternNexSeg  dw 0

;***********************************
;*********** InitPlayer ************
;***********************************

_InitPlay   proc near             ;!!!! ATTENTIONE !!!!
                                  ; call function only once
            pusha                 ; else khrrrrrrr......
            push es
            push ds

            ;******************************
            ;*** Alte Variablen lîschen ***

            mov PatternChannelFlag,1
            mov SampleLengthing,0
            mov SampleLoopEnding,0
            mov Scaling,0

            mov ebp,0
            mov es,bp
            mov dword ptr es:[12],0f000fff0h
            mov dword ptr es:[04],0f000fff0h   ; little anti-debug
KillVariable:
            mov PatternStart+bp,0
            mov PatternRow+bp,64
            mov PatternTempoBytes+ebp*2,0
            mov PatternTempoRow+bp,0
            mov PatternSpeed+bp,0
            mov PatternSpeedRow+bp,0

            mov PlayingFlag+bp,0
            mov CountTick+ebp*2,0
            mov TickNow+bp,0
            mov SpeedNow+bp,0
            mov RowPos+bp,0

            mov ChannelSampleNr+bp,0
            mov ChannelNote+bp,0
            mov ChannelOctave+bp,0
            mov ChannelPeriode+ebp*4,0
            mov ChannelVolume+bp,0
            mov ChannelGVolume+bp,0
            mov ChannelEffekt+bp,0FFh
            mov ChannelEffektInfo+bp,0
            mov GlissandoPeriode+ebp*4,0
            mov GlissandoSpeed+bp,0

            mov SampleFlag+bp,0
            mov SampleLength+ebp*2,0
            mov SampleLoopBegin+ebp*2,0
            mov SampleLoopEnd+ebp*2,0
            mov SampleVolume+bp,0
            mov SampleFinetune+ebp*2,0

            mov SampleOffset+ebp*4,0
            mov SamplePosition+ebp*2,0
            mov SamplePosScale+ebp*4,0
            mov SampleScale+ebp*4,0
            mov SamplePeriode+ebp*2,0
            mov SampleFreq+ebp*4,0

            inc bp
            cmp bp,16
            jb KillVariable

            ;*******************************************
            ;*** Speicher fÅr den Player vorbereiten ***

            ;*** Speicher fÅr 8K SBPlayBlock finden ***

            mov ax,(PBlockLength/16)
             mov cl,SB16Flag
             shl ax,cl
            call GetMem             ;Block 1 reservieren
            jc @@Fehler
            mov bx,ax
            mov cx,ax
            mov dx,ax

            push cx
            mov ax,(PBlockLength/16)
             mov cl,SB16Flag
             shl ax,cl
            call GetMem             ;Block 2 reservieren
            jc @@Fehler

            mov bp,1111000000000000b  ;Nur in 64K schritten
             mov cl,SB16Flag
             shl bp,cl
            pop cx
            and cx,bp
            sub bx,cx                 ;wieviel Åber letztem 64K Block
            mov bp,(65536-PBlockLength)/16  ;pa·t Block noch in 64K Block
             mov cl,SB16Flag
             shl bp,cl
            cmp bx,bp
            jb  yaBlock1               ; wenn ja ...

            xchg dx,ax                ;sonst tausche Register
   yaBlock1:
            call FreeMem
            mov PlayBlockSeg,dx

          ;** Speicherblock lîschen **

            mov bp,PBlockLength
             mov cl,SB16Flag
             shl bp,cl
            mov es,dx
            xor di,di
  ClearMasterBlock:
            mov dword ptr es:[di],80808080h ;=128 auf jedem Byte
            add di,4
            cmp di,bp
            jb ClearMasterBlock

              xor eax,eax
              xor ebx,ebx
              mov ax,dx
              shl eax,4
              mov RealAdr1,eax         ; Reelle Adresse im Speicher berechnen

       ;**************************************
       ;*** SpeicherBlock fÅr MasterVolume ***

            cmp FMSCheck,0
            jnz @@Fehler
            mov dx,64
             mov cl,SB16Flag
             shl dx,cl
            call AllocXMS
            or al,al
            jz @@Fehler
            mov MasterVolHandle,dx
            call LockXMS
            mov MasterVolOffset,eax

       ;*** Speicher fÅr Aktuelles Pattern u nÑchstes ***

            mov ax,5*16*64 /16    ;!! Achtung, nur Pattern Daten !!
            call GetMem           ;!!  Geschw. und PatternBreak
            jc @@Fehler
            mov PatternAktSeg,ax  ;!!    noch nicht integriert   !!

            mov ax,5*16*64 /16
            call GetMem
            jc @@Fehler
            mov PatternNexSeg,ax  ;!! Undefinierter Inhalt

        ;*** Speicher fÅr 16 Channels reservieren (8 K)

            mov ax,(MBlockLength+80*256*2)/16     ;8 K da Sammel Channels
            call GetMem                      ;+16K fÅr VolumeTable
            jc @@Fehler
            mov ChannelsSeg,ax

            mov es,ax
            mov di,MBlockLength

    ;***** VolumeTable berechnen *****

            mov bl,0         ;StartSample=0
            mov bh,0         ;StartVolume=0
  VolumeTable:
            mov al,bl
            sub al,128         ;negieren
            imul bh            ;mit Volume multiplizieren
            mov cx,2           ;Rundungswert
            cmp ax,0
            jns positiv
               neg cx          ;negieren bei negativer Zahl
     positiv:
            add ax,cx          ;und runden
            sar ax,2           ;wieder begradigen auf 8 Bit
            mov es:[di],ax
            add di,2           ;eins in Tabelle weiter
            inc bl             ;Sample eins weiter
            jnz VolumeTable    ;erst alle 256 Sample durch
            inc bh             ;dann ein Volume weiter
            cmp bh,75          ;alle Volume fertig ?
            jb VolumeTable

            pop ds
            pop es
            popa
            mov al,0
            ret

           ;*** Fehler ***
@@Fehler:
            pop ds
            pop es
            popa
            mov al,1
            ret

_InitPlay   endp

;***********************************
;*********** ClosePlayer ***********
;***********************************

_ClosePlay   proc near

             push ax
             push es

             mov ax,ChannelsSeg
             call FreeMem

             mov ax,PatternNexSeg
             call FreeMem
             mov ax,PatternAktSeg
             call FreeMem

             mov dx,MasterVolHandle
             call UnlockXMS
             call FreeXMS
             mov ax,PlayBlockSeg
             call FreeMem

             pop es
             pop ax
             ret

_ClosePlay   endp

;***********************************
;************* PlayBLock ***********
;***********************************

DMA_Adress   db 00h,02h,04h,06h, 0c0h,0c4h,0c8h,0cch
DMA_Count    db 01h,03h,05h,07h, 0c2h,0c6h,0cah,0ceh
DMA_Page     db 87h,83h,81h,82h, 088h,08bh,089h,08ah
DMA_UPage    dw 487h,483h,481h,482h, 488h,48bh,489h,48ah

DMA_Status   db 08h, 0d0h
DMA_Command  db 08h, 0d0h
DMA_Request  db 09h, 0d2h
DMA_ChMask   db 0ah, 0d4h
DMA_Mode     db 0bh, 0d6h
DMA_FlipFlop db 0ch, 0d8h
DMA_CLR      db 0dh, 0dah
DMA_MaskCLR  db 0eh, 0dch
DMA_Mask     db 0fh, 0deh

_StartPlay  proc near

            pusha
            push es
            push ds

;*** Change Interrupts *****

            mov ax,0
            mov es,ax
            mov al,SbIRQ
            add ax,8
            shl ax,2
            mov si,ax
            mov IRQAdr,si
            mov eax,es:[si]  ; Int 13 = Interrupt 5 Hardware
            mov IntOld,eax
            mov ax,OFFSET Inter5
            mov es:[si],ax
            mov ax,SEG Inter5
            mov es:[si+2],ax

            in al,21h         ; Bitmaske aus InterruptController
            mov irqold,al     ; sichern
            mov bl,1          ; und
            mov cl,SBIRQ      ; Bitmaske
            shl bl,cl         ; zurechtschieben
            xor bl,0ffh       ; und negieren
            and al,bl         ; und Bit rauslîschen
            out 21h,al        ; Bit im Interruptcontroller lîschen

;*** DMA*********************

              xor dh,dh
              mov bx,SBDMA
              shr bx,2
              mov dl,DMA_ChMask+bx
             mov ax,SBDMA
             or al,4        ;jeweiligen Kanal maskieren
            out dx,al            ; KanÑle Maskieren

              mov dl,DMA_Mode+bx
             mov ax,SBDMA       ; mit Autoinit
             and al,3
             or al,64+16+8
            out dx,al            ; Art der öbertragung
              mov dl,DMA_FlipFlop+bx
            mov al,0h
            out dx,al            ; Lîschen des Flip-Flops

              mov bx,SBDMA
              mov dl,DMA_Adress+bx
            mov eax,RealAdr1
             mov cl,SB16Flag
             shr eax,cl        ;16 Bit
            out dx,al       ; Lo-Adresse des DMA Buffers
            mov al,ah
            out dx,al       ; Hi-Adresse des DMA Buffers

              mov dl,DMA_Page+bx
            mov eax,RealAdr1
            shr eax,16
            out dx,al     ; Lower Page Register des DMA
            mov al,ah
              add bx,bx
              mov dx,DMA_UPage+bx
            ;out dx,al      ; Upper Page Register
                            ; some older motherboards have problems with this
                            ; port... :(
              xor dh,dh
              mov bx,SBDMA
              mov dl,DMA_Count+bx
            mov ax,PBlockLength-1
            out dx,al     ; Count Register des DMA mit Lo-LÑnge laden
            mov al,ah
            out dx,al     ; Count Register mi Hi-LÑnge laden

              shr bx,2
              mov dl,DMA_ChMask+bx
             mov ax,SBDMA
             and al,3
            out dx,al     ; Maskierung wieder lîschen

; **** SB *****************

            mov ah,0d1h
            call wrdsp  ; Lautsprecher an

            cmp SBArt,5   ;Ist SB eine SB16 ?
            jz StartSB16

            mov ah,40h
            call wrdsp  ; Samplingrate : Normal 256 - (1000000 DIV Freq)
                        ;                High 65536 - (256000000 Div Freq)
                        ;                HIGH BYTE only !
            mov bx,MixSpd
            mov dx,3906*2   ;256000000/65536
            mov ax,16384*2  ;256000000 mod 65536
            div bx
             inc ax
             shr ax,1
            neg ax
                        ;Speed 233 = 43478 Hz entspricht 44100 Hz
            call wrdsp  ;maxspeed = 235 = 47619 Hz

            mov ah,48h
            call wrdsp  ; Blockgrî·e = Length-1

            mov ax,(PBlockLength/2)-1
            xchg ah,al
            call wrdsp  ; Lo-Byte Length
            mov ah,al
            call wrdsp  ; Hi-Byte Length

            mov ah,90h
            call wrdsp  ; High-Speed 8bit AUTO-INIT Ausgabe starten
            jmp endplay

      ;*** Wenn eine SB16 gefunden dann...

StartSB16:
            mov ah,041h    ; MixRate einstellen
            call wrdsp
            mov ax,MixSpd  ; Hi-Byte von Mixing speed
            call wrdsp
            mov ah,al      ; Lo-Byte von Mixing speed
            call wrdsp

            mov ah,0c6h    ; 8Bit Auto-Init mit FIFO Ausgabe
             mov al,SB16Flag ; if 16 Bit Flag set
             shl al,4        ; than  x16 = 10h
             sub ah,al       ; and sub ah to get 0b6h
            call wrdsp
            mov ah,00h     ; MONO and unsigned
            call wrdsp
            mov ax,(PBlockLength/2)-1
            xchg ah,al     ; Lo-Byte Length
            call wrdsp
            mov ah,al      ; Hi-Byte Length
            call wrdsp

endplay:
;********** END OF Start Playing ***

            pop ds
            pop es
            popa
            ret

_StartPlay  endp

;**********************************
;*********** StopBlock ************
;**********************************

_StopPlay   proc near

            pusha
            push es
ende:
              mov bx,SBDMA
              shr bx,2
              xor dh,dh
              mov dl,DMA_ChMask+bx
             mov ax,SBDMA
             or al,4
            out dx,al
            mov ah,0d0h
            call wrdsp        ; DMA aus
            mov ah,0d3h
            call wrdsp        ; Lautsprecher aus
 nextReset:
            call SbReset
            cmp al,0AAh
            jnz nextReset

            mov al,irqold
            out 21h,al

            mov ax,0
            mov es,ax
            mov eax,IntOld
            mov si,IRQAdr
            mov es:[si],eax

            pop es
            popa
            ret

_StopPlay   endp

;**************************************************

;***************************
;********* Interrupt *******
;***************************

Inter5      proc far             ;Interrupt fÅr 8/16 Bit öbertragung
            public Inter5        ;mit AUTO-INIT

            cli
            pusha
            mov dx,cs:SbIOE        ; get SBIO
            add dl,cs:SB16Flag     ; add 1 if 16bit
            in al,dx               ; and get byte to say SB, IRQ recognized

            xor cs:SbBlock,0ffh    ; Block flag switch

            mov al,020h
            out 20h,al             ; EOI (End Of Interrupt)
            popa
            sti
            iret

Inter5      endp

;********************************

INCLUDE PLoad.asm         ; S3M loader
INCLUDE Mixer.asm         ; Mixer procedures (main programm)
INCLUDE PEffekt.asm       ; routines to process S3M effects
INCLUDE MasterVol.asm     ; routine which calculate MasterVolume
INCLUDE Pattern.asm       ; routine which process the pattern notes and so on
INCLUDE PMEM.asm          ; routines to get main memory
INCLUDE FMS.asm           ; routine to install protected mode and install
                          ; Flat-Memory-System to access also above 1 Mb


RealAdr1     dd ?   ;Real 32Bit Adresse von PlayBuffer fÅr DMA

SbBlock      db 0   ;Block0 = 0  :  Block1 = ff
IntOld       DD ?   ;Alter Interrupt Vektor
irqold       db ?   ;alte Bitmaske von IRQ-Controller

_text        ends
             end
