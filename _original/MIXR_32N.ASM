;/////////////////////////////////////////////////////////////////////////////
;// F/X - Player soft mixing routines v3.0 done in pmode during 1997 - 1998 //
;//               -- High Quality 32bit mixer not interpolated --           //
;// (c) 1997-98 by Apollo of STIGMA

.486p
model flat,prolog
locals

.data

.code

;// some helpfull definitions...

low16bit           equ 8000h           ; value of 16th bit on
high16bit          equ 80000000h       ; value of 32th bit on

fill32bit          equ 7fffffffh       ; value to fill 32bit paramter
fill16bit          equ 7fffh           ; value to fill 16bit paramter

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


;**************************** 32 BIT MIXER (2) ******************************

;--- MONO: 8bit mono Samples not interpolated

public mix8Mto32M_
public setup8Mto32M_

; mix8Mto32M:   ecx=fraction, esi=sample adress, edi=buffercounter
; setup8Mto32M: eax=tabeladress+volume, ebx=lowfraction, ecx=highfraction
;               edx=bufferaddress

ALIGN 4

mix8Mto32M_        proc                ; 8bit mono in 32bit mono mixer

                   xor ebx,ebx                   ; ebx = 0
ALIGN 4
          @@loop:
                   mov bl,[esi]                  ; load sample byte into ebx
                   add ecx,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_8Mto32M
                   mov eax, [4*ebx+fill32bit]    ; fetch sample*Volume
                        argdd Volume_8Mto32M
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_8Mto32M
                   add [edi+fill32bit],eax       ; add this channel to stream
                        argdd bufferaddress_8Mto32M

                   add edi,4                     ; and go to next one
                   jnz @@loop                    ; if not zero then loop again...

                   ret                           ; return
endp

ALIGN 4

setup8Mto32M_      proc

                   mov Volume_8Mto32M,eax        ; modify volume
                   shl ebx, 16                   ; shift ebx to high word
                   mov lowfrac_8Mto32M,ebx       ; modify lower fraction
                   mov highfrac_8Mto32M,ecx      ; modify higher fraction
                   mov bufferaddress_8Mto32M,edx ; end address of buffer

                   ret                           ; return
endp

;--- MONO: 16bit mono Samples not interpolated

public mix16Mto32M_
public setup16Mto32M_

; mix16Mto32M:   ecx=fraction, esi=sample adress, edi=buffercounter
; setup16Mto32M: eax=volume, ebx=lowfraction, ecx=highfraction
;                edx=bufferaddress

ALIGN 4

mix16Mto32M_        proc                ; 16bit mono in 32bit mono mixer

ALIGN 4
          @@loop:
                   add ecx,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_16Mto32M
                   mov eax,[esi+esi-2]           ; load sample byte into eax
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_16Mto32M
                   sar eax,16                    ; signextend ax to eax
                   imul eax,fill32bit            ; multiply sample with volume
                         argdd Volume_16Mto32M
                   add [edi+fill32bit],eax       ; add this channel to stream
                        argdd bufferaddress_16Mto32M

                   add edi,4                     ; and go to next one
                   jnz @@loop                    ; if not zero then loop again...

                   ret                           ; return
endp

ALIGN 4

setup16Mto32M_      proc

                   mov Volume_16Mto32M,eax       ; modify volume
                   shl ebx, 16                   ; shift ebx to high word
                   mov lowfrac_16Mto32M,ebx      ; modify lower fraction
                   mov highfrac_16Mto32M,ecx     ; modify higher fraction
                   mov bufferaddress_16Mto32M,edx ; end address of buffer

                   ret                           ; return
endp


;--- STEREO: 8bit mono Samples not interpolated

public mix8Mto32S_
public setup8Mto32S_

; mix8Mto32S:   ecx=fraction, esi=sampleaddress, edi=buffercounter
; setup8Mto32S: eax=tabeladdress+VolumeLeft, ebx=tableaddress+VolumeRight,
;               ecx=lowfraction, edx=highfraction, esi=bufferaddress

ALIGN 4

mix8Mto32S_         proc                ; 8bit mono in 32bit stereo mixer

                   xor ebx,ebx
ALIGN 4
          @@loop:
                   mov bl,[esi]
                   add ecx,fill32bit
                        argdd lowfrac_8Mto32S
                   mov eax, [ebx*4+fill32bit]
                        argdd VolumeLeft_8Mto32S
                   adc esi,fill32bit
                        argdd highfrac_8Mto32S
                   mov edx, [ebx*4+fill32bit]
                        argdd VolumeRight_8Mto32S
                   add [edi+fill32bit],eax
                        argdd bufferaddress1_8Mto32S
                   add [edi+fill32bit],edx
                        argdd bufferaddress2_8Mto32S

                   add edi,8
                   jnz @@loop

                   ret                 ; return
endp

ALIGN 4

setup8Mto32S_      proc

                   mov VolumeLeft_8Mto32S,eax    ; modify volume left eg. panning
                   mov VolumeRight_8Mto32S,ebx   ; modify volume right eg. panning
                   shl ecx, 16                   ; shift ebx to high word
                   mov lowfrac_8Mto32S,ecx       ; modify lower fraction
                   mov highfrac_8Mto32S,edx      ; modify higher fraction
                   mov bufferaddress1_8Mto32S,esi ; write buffer end address
                   add esi,4
                   mov bufferaddress2_8Mto32S,esi ; write buffer end address

                   ret                           ; return
endp

;--- STEREO: 16bit mono Samples not interpolated

public mix16Mto32S_
public setup16Mto32S_

; mix16Mto32S:   ecx=fraction, esi=sampleaddress/2, edi=buffercounter
; setup16Mto32S: eax=VolumeLeft, ebx=VolumeRight,
;                ecx=lowfraction, edx=highfraction, esi=bufferaddress

ALIGN 4

mix16Mto32S_       proc                ; 16bit mono in 32bit stereo mixer

ALIGN 4
          @@loop:
                   add ecx,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_16Mto32S
                   mov eax,[esi+esi-2]           ; load sample byte into eax
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_16Mto32S
                   sar eax,16                    ; signextend ax to eax
                   imul ebx,eax,fill32bit        ; multiply sample with volume
                        argdd VolumeLeft_16Mto32S
                   imul eax,fill32bit            ; multiply sample with volume
                        argdd VolumeRight_16Mto32S
                   add [edi+fill32bit],ebx
                        argdd bufferaddress1_16Mto32S
                   add [edi+fill32bit],eax
                        argdd bufferaddress2_16Mto32S

                   add edi,8
                   jnz @@loop

                   ret                           ; return
endp

ALIGN 4

setup16Mto32S_     proc

                   mov VolumeLeft_16Mto32S,eax   ; modify volume left eg. panning
                   mov VolumeRight_16Mto32S,ebx  ; modify volume right eg. panning
                   shl ecx, 16                   ; shift ebx to high word
                   mov lowfrac_16Mto32S,ecx      ; modify lower fraction
                   mov highfrac_16Mto32S,edx     ; modify higher fraction
                   mov bufferaddress1_16Mto32S,esi ; write buffer end address
                   add esi,4
                   mov bufferaddress2_16Mto32S,esi ; write buffer end address

                   ret                           ; return
endp

end
