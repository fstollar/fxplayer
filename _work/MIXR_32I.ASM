;/////////////////////////////////////////////////////////////////////////////
;// F/X - Player soft mixing routines v3.0 done in pmode during 1997 - 1998 //
;//                 -- High Quality 32bit mixer interpolated --             //
;// (c) 1997-98 by Apollo of STIGMA

.486p
.model flat
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


;------------------------------ Interpolation -------------------------------

;--- MONO: 8bit mono Samples interpolated

public mix8MIto32M_
public setup8MIto32M_

; mix8MIto32M:   ecx=fraction, esi=sample adress, edi=buffercounter
; setup8MIto32M: eax=volume, ebx=lowfraction, ecx=highfraction
;                edx=bufferaddress

ALIGN 4

mix8MIto32M_       proc                ; 8bit mono in 32bit mono mixer

                   mov ebp,ecx
ALIGN 4
          _lbl_mix8MIto32M__1_:
                   mov ecx,ebp
                   add ebp,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_8MIto32M
                   mov eax, [esi-2]
                   mov ebx, [esi-3]
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_8MIto32M
                   sar eax,24
                   sar ebx,24

                   sub eax,ebx
                   shr ecx,16
                   sal ebx,16

                   imul eax,ecx
                   add ebx,eax

                   imul eax,ebx,fill32bit
                         argdd Volume_8MIto32M
                   sar eax, 8

                   add [edi+fill32bit],eax       ; add this channel to stream
                        argdd bufferaddress_8MIto32M

                   add edi,4                     ; and go to next one
                   jnz _lbl_mix8MIto32M__1_                    ; if not zero then loop again...

                   ret                           ; return
endp

ALIGN 4

setup8MIto32M_      proc

                   mov Volume_8MIto32M,eax        ; modify volume
                   shl ebx, 16                   ; shift ebx to high word
                   mov lowfrac_8MIto32M,ebx       ; modify lower fraction
                   mov highfrac_8MIto32M,ecx      ; modify higher fraction
                   mov bufferaddress_8MIto32M,edx ; end address of buffer

                   ret                           ; return
endp

;--- MONO: 16bit mono Samples interpolated

public mix16MIto32M_
public setup16MIto32M_

; mix16MIto32M:   ecx=fraction, esi=sample adress, edi=buffercounter
; setup16MIto32M: eax=volume, ebx=lowfraction, ecx=highfraction
;                 edx=bufferaddress

ALIGN 4

mix16MIto32M_      proc                ; 16bit mono in 32bit mono mixer

                   mov ebp,ecx
ALIGN 4
          _lbl_mix16MIto32M__2_:
                   mov ecx,ebp
                   add ebp,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_16MIto32M
                   mov eax,[esi+esi]
                   mov ebx,[esi+esi-2]
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_16MIto32M
                   sar eax,16
                   sar ebx,16

                   shr ecx,16
                   sub eax,ebx
                   sal ebx,8

                   imul eax,ecx
                   sar eax,8
                   add ebx,eax
                   imul eax,ebx,fill32bit
                        argdd Volume_16MIto32M
                   sar eax,8                     ; and cut down to 24 bits

                   add [edi+fill32bit],eax       ; add this channel to stream
                        argdd bufferaddress_16MIto32M

                   add edi,4                     ; and go to next one
                   jnz _lbl_mix16MIto32M__2_                    ; if not zero then loop again...

                   ret                           ; return
endp

ALIGN 4

setup16MIto32M_      proc

                   mov Volume_16MIto32M,eax       ; modify volume
                   shl ebx, 16                   ; shift ebx to high word
                   mov lowfrac_16MIto32M,ebx      ; modify lower fraction
                   mov highfrac_16MIto32M,ecx     ; modify higher fraction
                   mov bufferaddress_16MIto32M,edx ; end address of buffer

                   ret                           ; return
endp


;--- STEREO: 8bit mono Samples interpolated

public mix8MIto32S_
public setup8MIto32S_

; mix8MIto32S:   ecx=fraction, esi=sampleaddress, edi=buffercounter
; setup8MIto32S: eax=VolumeLeft, ebx=VolumeRight,
;                ecx=lowfraction, edx=highfraction, esi=bufferaddress

ALIGN 4

mix8MIto32S_       proc                ; 8bit mono in 32bit stereo mixer

                   mov ebp,ecx
ALIGN 4
          _lbl_mix8MIto32S__3_:
                   mov ecx,ebp
                   add ebp,fill32bit
                        argdd lowfrac_8MIto32S
                   mov eax, [esi-2]
                   mov ebx, [esi-3]
                   adc esi,fill32bit
                        argdd highfrac_8MIto32S
                   sar eax,24
                   sar ebx,24
                   shr ecx,16
                   sub eax,ebx
                   sal ebx,16

                   imul eax,ecx
                   add ebx,eax

                   imul eax,ebx,fill32bit
                         argdd VolumeLeft_8MIto32S
                   imul ebx,fill32bit
                         argdd VolumeRight_8MIto32S
                   sar eax, 8
                   sar ebx, 8

                   add [edi+fill32bit],eax
                        argdd bufferaddress1_8MIto32S
                   add [edi+fill32bit],ebx
                        argdd bufferaddress2_8MIto32S

                   add edi,8
                   jnz _lbl_mix8MIto32S__3_

                   ret                 ; return
endp

ALIGN 4

setup8MIto32S_     proc

                   mov VolumeLeft_8MIto32S,eax   ; modify volume left eg. panning
                   mov VolumeRight_8MIto32S,ebx  ; modify volume right eg. panning
                   shl ecx, 16                   ; shift ebx to high word
                   mov lowfrac_8MIto32S,ecx      ; modify lower fraction
                   mov highfrac_8MIto32S,edx     ; modify higher fraction
                   mov bufferaddress1_8MIto32S,esi ; write buffer end address
                   add esi,4
                   mov bufferaddress2_8MIto32S,esi ; write buffer end address

                   ret                           ; return
endp

;--- STEREO: 16bit mono Samples interpolated

public mix16MIto32S_
public setup16MIto32S_

; mix16MIto32S:   ecx=fraction, esi=sampleaddress/2, edi=buffercounter
; setup16MIto32S: eax=VolumeLeft, ebx=VolumeRight,
;                 ecx=lowfraction, edx=highfraction, esi=bufferaddress

ALIGN 4

mix16MIto32S_       proc                ; 16bit mono in 32bit stereo mixer

                    mov ebp,ecx
ALIGN 4
          _lbl_mix16MIto32S__4_:
                   mov ecx,ebp
                   add ebp,fill32bit
                        argdd lowfrac_16MIto32S
                   mov eax,[esi+esi]
                   mov ebx,[esi+esi-2]
                   adc esi,fill32bit
                        argdd highfrac_16MIto32S
                   sar eax,16
                   sar ebx,16
                   shr ecx,16
                   sub eax,ebx

                   imul eax,ecx
                   sal ebx,8
                   sar eax,8
                   add ebx,eax

                   imul eax,ebx,fill32bit
                        argdd VolumeLeft_16MIto32S
                   imul ebx,fill32bit
                        argdd VolumeRight_16MIto32S
                   sar eax,8
                   sar ebx,8

                   add [edi+fill32bit],eax
                        argdd bufferaddress1_16MIto32S
                   add [edi+fill32bit],ebx
                        argdd bufferaddress2_16MIto32S

                   add edi,8
                   jnz _lbl_mix16MIto32S__4_

                   ret                           ; return
endp

ALIGN 4

setup16MIto32S_     proc

                   mov VolumeLeft_16MIto32S,eax    ; modify volume left eg. panning
                   mov VolumeRight_16MIto32S,ebx   ; modify volume right eg. panning
                   shl ecx, 16                     ; shift ecx to high word
                   mov lowfrac_16MIto32S,ecx       ; modify lower fraction
                   mov highfrac_16MIto32S,edx      ; modify higher fraction
                   mov bufferaddress1_16MIto32S,esi ; write buffer end address
                   add esi,4
                   mov bufferaddress2_16MIto32S,esi ; write buffer end address

                   ret                             ; return
endp

end
