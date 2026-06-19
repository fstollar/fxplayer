;/////////////////////////////////////////////////////////////////////////////
;// F/X - Player soft mixing routines v3.0 done in pmode during 1997 - 1998 //
;//       -- High Speed 16bit mixer and High Quality 32bit mixer --         //
;// (c) 1997 by Apollo of STIGMA

.386
model flat,prolog
locals

.data

.code

;// some helpfull definitions...

low16bit           equ 32768           ; value of 16th bit on
high16bit          equ 32768*65536     ; value of 32th bit on

fill32bit          equ 1234      ; value to set 32bit on 1


;// ...and some helpfull macros for selfmodifing code

argdb              macro name          ; for selfmodifing byte

                   org $-1             ; actuall adress-1
                   name db 0           ; is a databyte
endm


argdw              macro name          ; for selfmodifing word

                   org $-2             ; actuall adress-2
                   name dw 0           ; is a dataword
endm


argdd              macro name          ; for selfmodifing dword

                   org $-4             ; actuall adress-4
                   name dd 0           ; is a data dword
endm


;**************************** 16 BIT MIXER (1) ******************************

ALIGN 4

public mix8Mto16M_
public setup8Mto16M_

; mix8Mto16M:   ebx=0, ebp=fraction, esi=sample adress, edi=buffer counter
; setup8Mto16M: eax=tabeladress+volume, ebx=lowfraction, ecx=highfraction
;               edx=bufferaddress

mix8Mto16M_        proc                ; 8bit mono in 16bit mono mixer

                   xor ebx,ebx                ; ebx = 0
ALIGN 4
          @@loop:
                   mov bl,[esi]               ; load sample byte into ebx
                   add ecx,fill32bit          ; increase highword of ebp
                        argdd lowfrac_8Mto16M
                   mov eax, [2*ebx+fill32bit]  ; fetch sample*Volume
                        argdd Volume_8Mto16M
                   adc esi,fill32bit          ; increase sample pointer
                        argdd highfrac_8Mto16M
                   add [fill32bit-edi],ax     ; add this channel to stream
                        argdd bufferaddress_8Mto16M
                   add edi,2                  ; and go to next one

                   jnz @@loop                 ; if not zero then loop again...

                   ret                        ; return
endp

ALIGN 4

setup8Mto16M_      proc

                   mov Volume_8Mto16M,eax     ; modify volume
                   shl ebx, 16                ; shift ebx to high word
                   mov lowfrac_8Mto16M,ebx    ; modify lower fraction
                   mov highfrac_8Mto16M,ecx   ; modify higher fraction
                   mov bufferaddress_8Mto16M,edx ; modify address of buffer

                   ret                        ; return
endp



public mix8Mto16S_
public setup8Mto16S_

; mix8Mto16S: eax=ebx=0, edx=fraction+counter, esi=sample adress, edi=buffer
; setup8Mto16S: eax=VolumeLeft+tabeladress, edx=VolumeRight+tableadress, bx=lowfraction, ecx=highfraction

ALIGN 4

mix8Mto16S_         proc                ; 8bit mono in 16bit stereo mixer

                   xor ebx,ebx
ALIGN 4
          @@loop:
                   mov bl,[esi]
                   add edx,fill32bit
                        argdd lowfrac_8Mto16S
                   mov eax, [2*ebx+fill32bit]
                        argdd VolumeRight_8Mto16S
                   adc esi,fill32bit
                        argdd highfrac_8Mto16S
                   mov ax, [2*ebx+fill32bit]
                        argdd VolumeLeft_8Mto16S
                   add [edi],eax
                   add edi,4

                   dec dx
                   jnz @@loop

                   ret                 ; return
endp

ALIGN 4

setup8Mto16S_      proc

                   mov VolumeLeft_8Mto16S,eax   ; modify volume left eg. panning
                   sub edx,2
                   mov VolumeRight_8Mto16S,edx  ; modify volume right eg. panning
                   shl ebx, 16                  ; shift ebx to high word
                   mov lowfrac_8Mto16S,ebx      ; modify lower fraction
                   mov highfrac_8Mto16S,ecx     ; modify higher fraction

                   ret                          ; return
endp



public mix16Mto16S_
public setup16Mto16S_

; mix16Mto16S: eax=ebx=0, edx=fraction+counter, esi=sample adress/2, edi=buffer
; setup16Mto16S: eax=VolumeLeft+tabeladress, edx=VolumeRight+tableadress, bx=lowfraction, ecx=highfraction

ALIGN 4

mix16Mto16S_         proc                ; 8bit mono in 16bit stereo mixer

                   xor ebx,ebx
ALIGN 4
          @@loop:
                   mov bl,[2*esi+1]
                   add edx,fill32bit
                                      argdd lowfrac_16Mto16S
                   mov eax, [2*ebx+fill32bit]
                                      argdd VolumeRight_16Mto16S
                   adc esi,fill32bit
                                      argdd highfrac_16Mto16S
                   mov ax, [2*ebx+fill32bit]
                                      argdd VolumeLeft_16Mto16S
                   add [edi],eax
                   add edi,4

                   dec dx
                   jnz @@loop

                   ret                 ; return
endp

ALIGN 4

setup16Mto16S_      proc

                   mov VolumeLeft_16Mto16S,eax   ; modify volume left eg. panning
                   sub edx,2
                   mov VolumeRight_16Mto16S,edx  ; modify volume right eg. panning
                   shl ebx, 16                  ; shift ebx to high word
                   mov lowfrac_16Mto16S,ebx      ; modify lower fraction
                   mov highfrac_16Mto16S,ecx     ; modify higher fraction

                   ret                          ; return
endp

ALIGN 4

;**************************** 32 BIT MIXER (2) ******************************


end
