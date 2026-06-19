;/////////////////////////////////////////////////////////////////////////////
;// F/X - Player soft mixing routines v3.0 done in pmode during 1997 - 1998 //
;//       -- High Speed 16bit mixer and High Quality 32bit mixer --         //
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


;**************************** 16 BIT MIXER (1) ******************************

;--- MONO: 8bit mono Samples not interpolated

public mix8Mto16M_
public setup8Mto16M_

; mix8Mto16M:   ecx=fraction, esi=sample adress, edi=buffercounter
; setup8Mto16M: eax=tabeladress+volume, ebx=lowfraction, ecx=highfraction
;               edx=bufferaddress

ALIGN 4

mix8Mto16M_        proc                ; 8bit mono in 16bit mono mixer

                   xor ebx,ebx                   ; ebx = 0
ALIGN 4
          _lbl_mix8Mto16M__1_:
                   mov bl,[esi]                  ; load sample byte into ebx
                   add ecx,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_8Mto16M
                   mov eax, [2*ebx+fill32bit]    ; fetch sample*Volume
                        argdd Volume_8Mto16M
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_8Mto16M
                   add [edi+fill32bit],ax        ; add this channel to stream
                        argdd bufferaddress_8Mto16M

                   add edi,2                     ; and go to next one
                   jnz _lbl_mix8Mto16M__1_                    ; if not zero then loop again...

                   ret                           ; return
endp

ALIGN 4

setup8Mto16M_      proc

                   mov Volume_8Mto16M,eax        ; modify volume
                   shl ebx, 16                   ; shift ebx to high word
                   mov lowfrac_8Mto16M,ebx       ; modify lower fraction
                   mov highfrac_8Mto16M,ecx      ; modify higher fraction
                   mov bufferaddress_8Mto16M,edx ; end address of buffer

                   ret                           ; return
endp

;--- MONO: 16bit mono Samples not interpolated

public mix16Mto16M_
public setup16Mto16M_

; mix16Mto16M:   ecx=fraction, esi=sample adress, edi=buffercounter
; setup16Mto16M: eax=volume, ebx=lowfraction, ecx=highfraction
;                edx=bufferaddress

ALIGN 4

mix16Mto16M_        proc                ; 8bit mono in 16bit mono mixer

ALIGN 4
          _lbl_mix16Mto16M__2_:
                   add ecx,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_16Mto16M
                   mov eax,[esi+esi-2]           ; load sample byte into eax
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_16Mto16M
                   sar eax,16                    ; signextend ax to eax
                   imul eax,fill32bit            ; multiply sample with volume
                         argdd Volume_16Mto16M
                   sar eax,12                    ; and cut down to 12 bits
                   add [edi+fill32bit],ax        ; add this channel to stream
                        argdd bufferaddress_16Mto16M

                   add edi,2                     ; and go to next one
                   jnz _lbl_mix16Mto16M__2_                    ; if not zero then loop again...

                   ret                           ; return
endp

ALIGN 4

setup16Mto16M_      proc

                   mov Volume_16Mto16M,eax       ; modify volume
                   shl ebx, 16                   ; shift ebx to high word
                   mov lowfrac_16Mto16M,ebx      ; modify lower fraction
                   mov highfrac_16Mto16M,ecx     ; modify higher fraction
                   mov bufferaddress_16Mto16M,edx ; end address of buffer

                   ret                           ; return
endp


;--- STEREO: 8bit mono Samples not interpolated

public mix8Mto16S_
public setup8Mto16S_

; mix8Mto16S:   ecx=fraction, esi=sampleaddress, edi=buffercounter
; setup8Mto16S: eax=tabeladdress+VolumeLeft, ebx=tableaddress+VolumeRight,
;               ecx=lowfraction, edx=highfraction, esi=bufferaddress

ALIGN 4

mix8Mto16S_         proc                ; 8bit mono in 16bit stereo mixer

                   xor ebx,ebx
ALIGN 4
          _lbl_mix8Mto16S__3_:
                   mov bl,[esi]
                   add ecx,fill32bit
                        argdd lowfrac_8Mto16S
                   mov eax, [ebx+ebx+fill32bit]
                        argdd VolumeRight_8Mto16S
                   adc esi,fill32bit
                        argdd highfrac_8Mto16S
                   mov ax, [ebx+ebx+fill32bit]
                        argdd VolumeLeft_8Mto16S
                   add [edi+fill32bit],eax
                        argdd bufferaddress_8Mto16S

                   add edi,4
                   jnz _lbl_mix8Mto16S__3_

                   ret                 ; return
endp

ALIGN 4

setup8Mto16S_      proc

                   mov VolumeLeft_8Mto16S,eax    ; modify volume left eg. panning
                   sub ebx,2
                   mov VolumeRight_8Mto16S,ebx   ; modify volume right eg. panning
                   shl ecx, 16                   ; shift ebx to high word
                   mov lowfrac_8Mto16S,ecx       ; modify lower fraction
                   mov highfrac_8Mto16S,edx      ; modify higher fraction
                   mov bufferaddress_8Mto16S,esi ; write buffer end address

                   ret                           ; return
endp

;--- STEREO: 16bit mono Samples not interpolated

public mix16Mto16S_
public setup16Mto16S_

; mix16Mto16S:   ecx=fraction, esi=sampleaddress/2, edi=buffercounter
; setup16Mto16S: eax=VolumeLeft, ebx=VolumeRight,
;                ecx=lowfraction, edx=highfraction, esi=bufferaddress

ALIGN 4

mix16Mto16S_       proc                ; 16bit mono in 16bit stereo mixer

ALIGN 4
          _lbl_mix16Mto16S__4_:
                   add ecx,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_16Mto16S
                   mov eax,[esi+esi-2]           ; load sample byte into eax
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_16Mto16S
                   sar eax,16                    ; signextend ax to eax
                   imul ebx,eax,fill32bit        ; multiply sample with volume
                        argdd VolumeRight_16Mto16S
                   sar ebx,12                    ;
                   imul eax,fill32bit            ; multiply sample with volume
                        argdd VolumeLeft_16Mto16S
                   shrd eax,ebx,16
                   add [edi+fill32bit],eax
                        argdd bufferaddress_16Mto16S

                   add edi,4
                   jnz _lbl_mix16Mto16S__4_

                   ret                           ; return
endp

ALIGN 4

setup16Mto16S_     proc

                   shl eax,4                     ; expand Volume to 12 bit to
                                                 ; gain 28bit after multiplikation
                   mov VolumeLeft_16Mto16S,eax   ; modify volume left eg. panning
                   mov VolumeRight_16Mto16S,ebx  ; modify volume right eg. panning
                   shl ecx, 16                   ; shift ebx to high word
                   mov lowfrac_16Mto16S,ecx      ; modify lower fraction
                   mov highfrac_16Mto16S,edx     ; modify higher fraction
                   mov bufferaddress_16Mto16S,esi ; write buffer end address

                   ret                           ; return
endp


;------------------------------ Interpolation -------------------------------

;--- MONO: 8bit mono Samples interpolated

public mix8MIto16M_
public setup8MIto16M_

; mix8MIto16M:   ecx=fraction, esi=sample adress, edi=buffercounter
; setup8MIto16M: eax=volume, ebx=lowfraction, ecx=highfraction
;                edx=bufferaddress

ALIGN 4

mix8MIto16M_       proc                ; 8bit mono in 16bit mono mixer

                   mov ebp,ecx
ALIGN 4
          _lbl_mix8MIto16M__5_:
                   mov ecx,ebp
                   add ebp,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_8MIto16M
                   mov eax, [esi-2]
                   mov ebx, [esi-3]
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_8MIto16M
                   sar eax,24
                   sar ebx,24

                   sub eax,ebx
                   shr ecx,16
                   sal ebx,16

                   imul eax,ecx
                   add ebx,eax

                   imul eax,ebx,fill32bit
                         argdd Volume_8MIto16M
                   sar eax, 20

                   add [edi+fill32bit],ax        ; add this channel to stream
                        argdd bufferaddress_8MIto16M

                   add edi,2                     ; and go to next one
                   jnz _lbl_mix8MIto16M__5_                    ; if not zero then loop again...

                   ret                           ; return
endp

ALIGN 4

setup8MIto16M_      proc

                   mov Volume_8MIto16M,eax        ; modify volume
                   shl ebx, 16                   ; shift ebx to high word
                   mov lowfrac_8MIto16M,ebx       ; modify lower fraction
                   mov highfrac_8MIto16M,ecx      ; modify higher fraction
                   mov bufferaddress_8MIto16M,edx ; end address of buffer

                   ret                           ; return
endp

;--- MONO: 16bit mono Samples interpolated

public mix16MIto16M_
public setup16MIto16M_

; mix16MIto16M:   ecx=fraction, esi=sample adress, edi=buffercounter
; setup16MIto16M: eax=volume, ebx=lowfraction, ecx=highfraction
;                 edx=bufferaddress

ALIGN 4

mix16MIto16M_      proc                ; 8bit mono in 16bit mono mixer

                   mov ebp,ecx
ALIGN 4
          _lbl_mix16MIto16M__6_:
                   mov ecx,ebp
                   add ebp,fill32bit             ; increase fraction of samplepointer
                        argdd lowfrac_16MIto16M
                   mov eax,[esi+esi]
                   mov ebx,[esi+esi-2]
                   adc esi,fill32bit             ; increase sample pointer
                        argdd highfrac_16MIto16M
                   sar eax,16
                   sar ebx,16

                   shr ecx,16
                   sub eax,ebx
                   sal ebx,8

                   imul eax,ecx
                   sar eax,8
                   add ebx,eax
                   imul eax,ebx,fill32bit
                        argdd Volume_16MIto16M
                   sar eax,20                    ; and cut down to 12 bits

                   add [edi+fill32bit],ax        ; add this channel to stream
                        argdd bufferaddress_16MIto16M

                   add edi,2                     ; and go to next one
                   jnz _lbl_mix16MIto16M__6_                    ; if not zero then loop again...

                   ret                           ; return
endp

ALIGN 4

setup16MIto16M_      proc

                   mov Volume_16MIto16M,eax       ; modify volume
                   shl ebx, 16                   ; shift ebx to high word
                   mov lowfrac_16MIto16M,ebx      ; modify lower fraction
                   mov highfrac_16MIto16M,ecx     ; modify higher fraction
                   mov bufferaddress_16MIto16M,edx ; end address of buffer

                   ret                           ; return
endp


;--- STEREO: 8bit mono Samples interpolated

public mix8MIto16S_
public setup8MIto16S_

; mix8MIto16S:   ecx=fraction, esi=sampleaddress, edi=buffercounter
; setup8MIto16S: eax=VolumeLeft, ebx=VolumeRight,
;                ecx=lowfraction, edx=highfraction, esi=bufferaddress

ALIGN 4

mix8MIto16S_         proc                ; 8bit mono in 16bit stereo mixer

                   mov ebp,ecx
ALIGN 4
          _lbl_mix8MIto16S__7_:
                   mov ecx,ebp
                   add ebp,fill32bit
                        argdd lowfrac_8MIto16S
                   mov eax, [esi-2]
                   mov ebx, [esi-3]
                   adc esi,fill32bit
                        argdd highfrac_8MIto16S
                   sar eax,24
                   sar ebx,24
                   shr ecx,16
                   sub eax,ebx
                   sal ebx,16

                   imul eax,ecx
                   add ebx,eax

                   imul eax,ebx,fill32bit
                         argdd VolumeRight_8MIto16S
                   imul ebx,fill32bit
                         argdd VolumeLeft_8MIto16S
                   sar eax, 20
                   sar ebx, 4
                   shld eax,ebx,16

                   add [edi+fill32bit],eax
                        argdd bufferaddress_8MIto16S

                   add edi,4
                   jnz _lbl_mix8MIto16S__7_

                   ret                 ; return
endp

ALIGN 4

setup8MIto16S_      proc

                   mov VolumeLeft_8MIto16S,eax   ; modify volume left eg. panning
                   mov VolumeRight_8MIto16S,ebx  ; modify volume right eg. panning
                   shl ecx, 16                   ; shift ebx to high word
                   mov lowfrac_8MIto16S,ecx      ; modify lower fraction
                   mov highfrac_8MIto16S,edx     ; modify higher fraction
                   mov bufferaddress_8MIto16S,esi ; write buffer end address

                   ret                           ; return
endp

;--- STEREO: 16bit mono Samples interpolated

public mix16MIto16S_
public setup16MIto16S_

; mix16MIto16S:   ecx=fraction, esi=sampleaddress/2, edi=buffercounter
; setup16MIto16S: eax=VolumeLeft, ebx=VolumeRight,
;                 ecx=lowfraction, edx=highfraction, esi=bufferaddress

ALIGN 4

mix16MIto16S_       proc                ; 16bit mono in 16bit stereo mixer

                   mov ebp,ecx
ALIGN 4
          _lbl_mix16MIto16S__8_:
                   mov ecx,ebp
                   add ebp,fill32bit
                        argdd lowfrac_16MIto16S
                   mov eax,[esi+esi]
                   mov ebx,[esi+esi-2]
                   adc esi,fill32bit
                        argdd highfrac_16MIto16S
                   sar eax,16
                   sar ebx,16
                   shr ecx,16
                   sub eax,ebx

                   imul eax,ecx
                   sal ebx,8
                   sar eax,8
                   add ebx,eax

                   imul eax,ebx,fill32bit
                        argdd VolumeRight_16MIto16S
                   imul ebx,fill32bit
                        argdd VolumeLeft_16MIto16S
                   sar eax,20
                   sar ebx,4
                   shld eax,ebx,16

                   add [edi+fill32bit],eax
                        argdd bufferaddress_16MIto16S

                   add edi,4
                   jnz _lbl_mix16MIto16S__8_

                   ret                           ; return
endp

ALIGN 4

setup16MIto16S_     proc

                   mov VolumeLeft_16MIto16S,eax    ; modify volume left eg. panning
                   mov VolumeRight_16MIto16S,ebx   ; modify volume right eg. panning
                   shl ecx, 16                     ; shift ecx to high word
                   mov lowfrac_16MIto16S,ecx       ; modify lower fraction
                   mov highfrac_16MIto16S,edx      ; modify higher fraction
                   mov bufferaddress_16MIto16S,esi ; write buffer end address

                   ret                             ; return
endp


end
