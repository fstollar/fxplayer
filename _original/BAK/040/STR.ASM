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

public _putchr
public __fputstring

.386

;*************************
;*** Putchar NEAR-NEAR ***
;*************************

_putchr  proc far

sframec   struc
cbp0       dw ?
cret0      dd ?
cxpos      dw ?
cypos      dw ?
char       dw ?
charattrib dw ?
sframec    ends

framec     equ [bp - cbp0]

     push bp
     mov bp,sp
     push di
     push si
     push es

     mov ax,0b800h
     mov es,ax

     mov di,framec.cypos
     dec di
     mov ax,di
     shl di,2
     add di,ax
     shl di,5

     add di,framec.cxpos
     add di,framec.cxpos
     sub di,2

     mov al, byte ptr framec.char
     mov ah, byte ptr framec.charattrib
     mov es:[di],ax

     pop es
     pop si
     pop di
     pop bp
     retf

_putchr  endp

;**************************
;*** PutString NEAR-FAR ***
;**************************

__fputstring  proc far

sframef   struc
fbp0       dw ?
fret0      dd ?
fxpos      dw ?
fypos      dw ?
fstrOffset dw ?
fstrSegment dw ?
fattrib    dw ?
sframef    ends

framef     equ [bp - fbp0]

  push bp
  mov bp,sp

  push di
  push si
  push es
  push ds

  mov ax,0b800h
  mov es,ax

  mov di,framef.fypos
  dec di
  mov ax,di
  shl di,2
  add di,ax
  shl di,5

  add di,framef.fxpos
  add di,framef.fxpos
  sub di,2

  mov si,framef.fstrOffset
  mov ds,framef.fstrSegment
  mov ah,byte ptr framef.fattrib

  mov al,ds:[si]
  or al,al
  jz fende

floop1:

  mov es:[di],ax
  add di,2
  inc si
  mov al,ds:[si]
  or al,al
  jnz floop1

fende:

  pop ds
  pop es
  pop si
  pop di
  pop bp
  retf

__fputstring  endp

_TEXT      ends
           end

