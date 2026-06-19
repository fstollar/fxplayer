;****************************************************
;*** Soundblaster Player Routinen for F/X-Tracker ***
;**** > C-Version  v2.1

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

_TEXT  segment dword public 'CODE'

public      _InitSB      ; Initialise SB
public      _ResetSB

public      _InitPlay    ; reserve memory for player and tables
public      _ClosePlay   ; frees all memory for player

public      _StartPlay   ; programms DMA and IRQ for SB and starts interrupt
public      _StopPlay    ; stops playing SB and reset IRQ and DMA

;------------------------------

.486p
.model compact
LOCALS @@

INCLUDE Macro.asm
INCLUDE SBset.ASM        ; routines for SB DSP programming

SbBlock           db 0   ;Block0 = 0  :  Block1 = ffh
IntOld            dd ?   ;old Interrupt Vektor
irqold            db ?   ;old Bitmask of IRQ-Controller
IRQAdr            dw ?
IRQout            dw ?

PBlockLength      equ 02000h  ; blocklength of SB playing block
MBlockLength      equ 02000h  ; blocklength of channel mixing

PlayBlockSeg      dw 0   ; segement to SB PlayBlock (without DMA overflow!)
RealAdrDMA        dd ?   ; Real 32Bit Adress for PlayBuffer of DMA

ChannelsSeg       dw 0   ; segment to ChannelBlock

PatternHandle     dw 0
PatternAktOffset  dd 0

VolTableHandle    dw 0   ; Handle of FMS VolumeTable
VolTableOffset    dd 0   ; 32bit Offset of VolumeTable
VolHalveOffset    dd 0   ; 32bit/2 Offset of VolumeTable for calculations

;***********************************
;*********** InitPlayer ************
;***********************************

_InitPlay   proc far             ;!!!! ATTENTIONE !!!!
                                 ; call function only once
            pushad               ; else khrrrrrrr......
            push es
            push ds

            ;*******************************************
            ;*** make memory reservations for player ***

            ;*** seek memory for SBPlayBlock without  DMA overflow ***

            mov ax,(PBlockLength/16)  ; load blocklength
             mov cl,SB16Flag           ; if it is 16bit
             shl ax,cl                 ; then doubles
            call GetMem               ; get block 1
            jc @@Fehler               ; on error...
            mov bx,ax
            mov cx,ax
            mov dx,ax

            push cx
            mov ax,(PBlockLength/16)
             mov cl,SB16Flag
             shl ax,cl
            call GetMem               ; get block 2
            jc @@Fehler

            mov bp,1111000000000000b  ; offset in 64K steps
             mov cl,SB16Flag           ; on 16bit in 128K
             shl bp,cl
            pop cx
            and cx,bp
            sub bx,cx                 ; how many over 64K ?
            mov bp,(65536-PBlockLength)/16  ; fits block into 64K area ?
             mov cl,SB16Flag
             shl bp,cl
            cmp bx,bp
            jb  @@yaBlock1             ; if yes...

            xchg dx,ax                 ; else swap memory blocks
 @@yaBlock1:
            call FreeMem               ; and free false one
            mov PlayBlockSeg,dx        ; and save segment

          ;** clear memory block **

            mov bp,PBlockLength        ;load BlockLength
             mov cl,SB16Flag            ; on 16bit
             shl bp,cl                  ; doubles
            mov es,dx                  ;load segment
            xor di,di
            mov eax,07fff7fffh         ; twice 16bit midle value
             or cl,cl                   ; is it 16bit
             jnz @@ClearMasterBlock     ; yes then all ok...
                 mov eax,080808080h       ; on 8 bit it should be 07f7f7f7fh

 @@ClearMasterBlock:
            mov dword ptr es:[di],eax  ; = midvalue on every sample
            add di,4
            cmp di,bp
            jb @@ClearMasterBlock      ; till end of Play block

            xor eax,eax
            xor ebx,ebx
            mov ax,dx
            shl eax,4                ; mul 16
            mov RealAdrDMA,eax         ; real 32bit offset of block

       ;**************************************
       ;*** MemoryBlock for MasterVolume ***

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

        ;*************************************
        ;*** Memory for VolumeTable (128K) ***

            mov dx,128
            call AllocXMS
             or al,al
             jz @@Fehler
            mov VolTableHandle,dx
            call LockXMS
            mov VolTableOffset,eax
             ;test eax,255
             ;jnz @@Fehler

        ;***** Calculate VolumeTable *****

            mov edi,eax
            shr eax,1
            mov VolHalveOffset,eax
            xor ax,ax
            mov es,ax

            mov bl,0         ;StartSample=0
            mov bh,0         ;StartVolume=0
  @@VolumeTable2:
            mov al,bl
            sub al,128         ;negieren
            imul bh            ;mit Volume multiplizieren
            mov cx,2           ;Rundungswert
            cmp ax,0
            jns @@positiv2
               neg cx          ;negieren bei negativer Zahl
     @@positiv2:
            add ax,cx          ;und runden
            sar ax,2           ;wieder begradigen auf 8 Bit
            mov es:[edi],ax
            add edi,2          ;eins in Tabelle weiter
            inc bl             ;Sample eins weiter
            jnz @@VolumeTable2  ;erst alle 256 Sample durch
            inc bh             ;dann ein Volume weiter
            cmp bh,70          ;alle Volume fertig ?
            jb @@VolumeTable2

       ;*************************************************
       ;*** Speicher fÅr Aktuelles Pattern u nÑchstes ***

            mov dx,2
            call AllocXMS
             or al,al
             jz @@Fehler
            mov PatternHandle,dx
            call LockXMS
            mov PatternAktOffset,eax

        ;**************************************************
        ;*** Speicher fÅr 16 Channels reservieren (8 K) ***

            mov ax,(MBlockLength)/16     ;8 K da Sammel Channels
            call GetMem
            jc @@Fehler
            mov ChannelsSeg,ax

            mov es,ax
            mov cx,MBlockLength/4
            xor di,di
            mov eax,07fff7fffh
 @@ClearMixBlock:
            mov es:[di],eax
            add di,4
            dec cx
            jnz @@ClearMixBlock

            pop ds
            pop es
            popad
            mov al,0
            retf

           ;*** Fehler ***
@@Fehler:
            pop ds
            pop es
            popad
            mov al,1
            retf

_InitPlay   endp

;***********************************
;*********** ClosePlayer ***********
;***********************************

_ClosePlay   proc far

             push ax
             push dx
             push es

             mov ax,ChannelsSeg
             call FreeMem

             mov dx,PatternHandle
             call UnlockXMS
             mov dx,PatternHandle
             call FreeXMS

             mov dx,MasterVolHandle
             call UnlockXMS
             mov dx,MasterVolHandle
             call FreeXMS

             mov dx,VolTableHandle
             call UnlockXMS
             mov dx,VolTableHandle
             call FreeXMS

             mov ax,PlayBlockSeg
             call FreeMem

             pop es
             pop dx
             pop ax
             retf

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

_StartPlay  proc far

            pushad
            push es
            push ds

;*** Change Interrupts *****

            movzx ax,SbIRQ
            cmp al,7
            jna @@noIRQ8
                sub al,8
                mov SbIRQ,al
                shl ax,2
                add ax,70h*4
                mov IRQAdr,ax
                mov IRQout,0a0h
                jmp @@IRQ8
 @@noIRQ8:
            add ax,8
            shl ax,2
            mov IRQAdr,ax
            mov IRQout,020h
 @@IRQ8:

            mov ax,0
            mov es,ax
            mov si,IRQAdr
            mov eax,es:[si]  ; Int 13 = Interrupt 5 Hardware
            mov IntOld,eax
            mov ax,OFFSET InterSB
            mov es:[si],ax
            mov ax,SEG InterSB
            mov es:[si+2],ax

            mov dx,IRQout
            inc dx
            in al,dx         ; Bitmaske aus InterruptController
            mov irqold,al     ; sichern
            mov bl,1          ; und
            mov cl,SBIRQ      ; Bitmaske
            shl bl,cl         ; zurechtschieben
            xor bl,0ffh       ; und negieren
            and al,bl         ; und Bit rauslîschen
            out dx,al        ; Bit im Interruptcontroller lîschen

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
            mov eax,RealAdrDMA
             mov cl,SB16Flag
             shr eax,cl        ;16 Bit
            out dx,al       ; Lo-Adresse des DMA Buffers
            mov al,ah
            out dx,al       ; Hi-Adresse des DMA Buffers

              mov dl,DMA_Page+bx
            mov eax,RealAdrDMA
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
            mov ebx,MixSpd
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
            mov eax,MixSpd  ; Hi-Byte von Mixing speed
            call wrdsp
            mov ah,al      ; Lo-Byte von Mixing speed
            call wrdsp

            mov ah,0c6h    ; 8Bit Auto-Init mit FIFO Ausgabe
             mov al,SB16Flag ; if 16 Bit Flag set
             shl al,4        ; than  x16 = 10h
             sub ah,al       ; and sub ah to get 0b6h
            call wrdsp
            mov ah,00h     ; MONO and unsigned
             cmp SBstereoFlag,0
             jz @@noSTEREO
                 or ah,00100000b  ; or STEREO
   @@noSTEREO:
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
            popad
            retf

_StartPlay  endp

;**********************************
;*********** StopBlock ************
;**********************************

_StopPlay   proc far

            pushad
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
            mov dx,IRQout
            inc dx
            out dx,al

            mov ax,0
            mov es,ax
            mov eax,IntOld
            mov si,IRQAdr
            mov es:[si],eax

            pop es
            popad
            retf

_StopPlay   endp

;**************************************************

;***************************
;********* Interrupt *******
;***************************

InterSB     proc far             ;Interrupt for 8/16 Bit Playback
            public InterSB       ;with AUTO-INIT

            cli                  ;disable Interrupts
            push ax
            push dx
            mov dx,cs:SbIOE        ; get SBIO
             add dl,cs:SB16Flag     ; add 1 if 16bit
            in al,dx               ; and get byte to say SB, IRQ recognized

            xor cs:SbBlock,0ffh    ; Block flag switch

            test cs:IRQMixing,0    ; should mix itself ?
            jnz @@noMixer          ; if not jmp...

              call Mixer           ; call mixer routines
  @@noMixer:
            mov al,20h
            mov dx,IRQout
            out dx,al             ; EOI (End Of Interrupt)
            pop dx
            pop ax
            sti                    ;enable Interrupts
            iret                  ;return

InterSB     endp

;********************************

INCLUDE Mixer.asm         ; Mixer procedures (main programm)
INCLUDE Ticker.asm        ; Control routines for S3M
INCLUDE PLoad.asm         ; MOD, S3M, IT loader
INCLUDE PEffekt.asm       ; routines to process effects of S3M
INCLUDE MasterVol.asm     ; routine which calculate MasterVolume
INCLUDE Pattern.asm       ; routine which process the pattern notes and so on
INCLUDE PMEM.asm          ; routines to get main memory
INCLUDE FMS.asm           ; routine to install protected mode and install
                          ; Flat-Memory-System to access also above 1 Mb
_text        ends
             end
