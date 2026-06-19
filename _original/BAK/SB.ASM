
.386
model flat,prolog
locals

.data

SB16bit            db 0
SBstereo           db 0
SBflip             db 0

;*** SB programing datas

SB_IO              dw 0
SB_IRQ             db 0
SB_DMA             db 0
SB_MixSpeed        dw 0

SB_blocklength     dw 0
SB_DSP             dw 0
SB_Ver             db 0


;*** SB io adresses

SBioReset          dw ?
SBioC              dw ?
SBioE              dw ?

SBioMix            dw ?
SBioMixDat         dw ?

.code

extrn Delay :near

extrn Mixer_Block : byte

public SBInterupt_

SBInterupt_        proc

                   push ax
                   push dx

                   mov dx,SBioE        ; get SBIO
                   add dl,SB16bit      ; add 1 if 16bit
                   in al,dx               ; and get byte to say SB, IRQ recognized

                   xor Mixer_Block,0ffh      ; Block flag switch

                   pop dx
                   pop ax
                   ret
endp


public checkSB_

checkSB_           proc

                   ; ax = SB io adress

                   mov SB_IO,ax
                   add ax,4
                   mov SBioMix,ax
                   inc ax
                   mov SBioMixDat,ax
                   inc ax
                   mov SBioReset,ax
                   add ax,6
                   mov SBioC,ax
                   add ax,2
                   mov SBioE,ax

                   call SBReset_   ; SB Reset durchfhren
                   cmp al,0h
                   jz @@timeout
                   cmp al,0AAh      ; keine SB wenn nicht AAh
                   jnz @@noReset    ; zurck geliefert wird

                   mov ah,0E1h    ; Versionsnummer DSP abfragen
                   call wrDSP
                   call rdDSP     ; Hauptversionsnummer
                   mov ah,al
                   call rdDSP     ; Unterversionsnummer
                   mov SB_DSP,ax

                   cmp ax,512
                   jg @@no200    ; SB 1.5 - SB 2.0
                   mov SB_Ver,1  ; MONO 8 Bit 4000-23000 Hz
                   jmp @@endsp
     @@no200:
                   cmp ax,768    ; SB 2.01 - SB 2.5
                   jge @@no201   ; MONO 8 Bit 4000-23000 Hz
                   mov SB_Ver,2  ; MONO HISPEED 8 Bit 23000-44100 Hz
                   jmp @@endsp
     @@no201:
                   cmp ax,1024   ; SB PRO 2.0
                   jge @@no3xx   ; MONO 8 Bit 4000-23000 Hz
                   mov SB_Ver,3  ; MONO HISPEED 8 Bit 23000-44100 Hz
                   jmp @@endsp   ; STEREO HISPEED 8 Bit 11025-22050 Hz
     @@no3xx:
                   mov SB_Ver,4  ; SB PRO 2.5
                                 ; MONO 8 Bit 5000-44100 Hz
                                 ; STEREO 8 Bit 5000-44100 Hz

                   mov SB_Ver,5  ; SB 16 (ASP)
                                 ; MONO 8/16 Bit 5000-44100 Hz
                                 ; STEREO 8/16 Bit 5000-44100 Hz
     @@endsp:
                   movzx ax,SB_Ver
                   ret

     @@timeout:
     @@noReset:
                    mov ax,0
                    ret
endp

public getSB_DSP_

getSB_DSP_         proc

                   mov ax,SB_DSP
                   ret
endp


public initSB_

initSB_            proc

                   ; ax = SB io adress
                   ; bx = SB 16 bit flag
                   ; cx = SB stereo flag

                   mov SB_IO,ax
                   add ax,4
                   mov SBioMix,ax
                   inc ax
                   mov SBioMixDat,ax
                   inc ax
                   mov SBioReset,ax
                   add ax,6
                   mov SBioC,ax
                   add ax,2
                   mov SBioE,ax

                   mov SB16bit,bl
                   mov SBstereo,cl

                   mov ah,0d1h
                   call wrDSP    ; Lautsprecher an

                   ret
endp

;********************* SB 1.0 - 1.5 setups ********************

public setMixSB_
public setLengthSB_
public startSB_

setMixSB_          proc

                   mov SB_MixSpeed,ax

                   mov ah,40h
                   call wrDSP  ; Samplingrate : Normal 256 - (1000000 DIV Freq)
                               ;                High 65536 - (256000000 Div Freq)
                               ;                HIGH BYTE only !
                   mov bx,SB_MixSpeed
                   mov dx,3906*2   ;256000000/65536
                   mov ax,16384*2  ;256000000 mod 65536
                   div bx
                   inc ax
                   shr ax,1
                   neg ax
                               ;Speed 233 = 43478 Hz entspricht 44100 Hz
                   call wrDSP  ;maxspeed = 235 = 47619 Hz

                   ret
endp

setLengthSB_       proc

                   mov SB_blocklength,ax

                   mov ah,48h
                   call wrDSP  ; Blockgr”áe = Length-1

                   mov ax,SB_blocklength
                   dec ax
                   xchg ah,al
                   call wrDSP  ; Lo-Byte Length
                   mov ah,al
                   call wrDSP  ; Hi-Byte Length

                   ret
endp

startSB_           proc

                   mov ah,90h
                   call wrDSP  ; High-Speed 8bit AUTO-INIT Ausgabe starten

                   ret
endp

;****************** SB16 setups ****************

public setMixSB16_
public setLengthSB16_
public startSB16_

setMixSB16_        proc

                   mov SB_MixSpeed,ax

                   mov ah,041h         ; MixRate einstellen
                   call wrDSP
                   mov ax,SB_MixSpeed  ; Hi-Byte von Mixing speed
                   call wrDSP
                   mov ah,al           ; Lo-Byte von Mixing speed
                   call wrDSP

                   ret
endp

setLengthSB16_     proc

                   mov SB_blocklength,ax

                   mov ah,0c6h     ; 8Bit Auto-Init mit FIFO Ausgabe
                   mov al,SB16bit  ; if 16 Bit Flag set
                   shl al,4        ; than  x16 = 10h
                   sub ah,al       ; and sub ah to get 0b6h
                   call wrDSP
                   mov ah,00h     ; MONO and unsigned
                   cmp SBstereo,0
                   jz @@noSTEREO
                      or ah,00100000b  ; or STEREO
   @@noSTEREO:
                   call wrDSP

                   ret
endp

startSB16_         proc

                   mov ax,SB_blocklength
                   dec ax
                   xchg ah,al     ; Lo-Byte Length
                   call wrDSP
                   mov ah,al      ; Hi-Byte Length
                   call wrDSP

                   ret
endp



;*** Soundblaster Reset durchfhren ***

public SBReset_

SBReset_           proc

                   push dx
                   push cx

         ;*********** SB Reset ****

                   mov dx,SBioReset
                   mov al,1
                   out dx,al         ; erst 1 schreiben

                   mov cx,8196
                   call Delay        ; >5 Millisek warten

                   mov dx,SBioReset
                   mov al,0
                   out dx,al         ; darauf 0

                   mov cx,8196
                   call Delay        ; Nochmal warten

                   mov dx,SBioE
                   xor cx,cx
@@notyet:
                   xor al,al
                   in al,dx          ; Schon fertig mit Reset
                   test al,80h
                   jnz @@ByteReady

                   loop @@notyet     ; 65536 mal fragen ob fertig

                   xor al,al         ; sonst keine SB vorhanden
                   jmp @@NoDSP
@@ByteReady:
                   sub dx,04h
                   in al,dx          ; auslesen
@@NoDSP:                             ; keine Reset wenn al<>AAh
                   pop cx
                   pop dx
                   ret
endp

;*********** Routine die den DSP der Soundblaster beschreibt *********
;* IN:  ah = byte to write to Sound Blaster DSP
;* OUT: -

wrDSP              proc

                   push ax
                   push dx

                   mov dx,SBioC
@@notready:
                   in al,dx
                   and al,128
                   jnz @@notready

                   mov al,ah
                   out dx,al

                   pop dx
                   pop ax
                   ret
endp

;*********** Routine die den DSP der Soundblaster liest *********

rdDSP              proc

                   push dx

                   mov dx,cs:SBioE
@@notready:
                   in al,dx
                   and al,128
                   jz @@notready

                   sub dx,4
                   in al,dx

                   pop dx
                   ret
endp

end
