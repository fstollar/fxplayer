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

public _putchr
public _putstring
public __fputstring

.386

;*************************
;*** Putchar NEAR-NEAR ***
;*************************

_putchr  proc near

sframec   struc
cbp0       dw ?
cret0      dw ?
cxpos      dw ?
cypos      dw ?
char       db ?
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

     mov al, framec.char
     mov ah, byte ptr framec.charattrib+1
     mov es:[di],ax

     pop es
     pop si
     pop di
     pop bp
     ret

_putchr  endp

;***************************
;*** PutString NEAR-NEAR ***
;***************************

_putstring  proc near

sframe   struc
bp0       dw ?
ret0      dw ?
xpos      dw ?
ypos      dw ?
strOffset dw ?
attrib    db ?
sframe    ends

frame     equ [bp - bp0]

  push bp
  mov bp,sp

  push di
  push si
  push es

  mov ax,0b800h
  mov es,ax

  mov di,frame.ypos
  dec di
  mov ax,di
  shl di,2
  add di,ax
  shl di,5

  add di,frame.xpos
  add di,frame.xpos
  sub di,2

  mov si,frame.strOffset
  mov ah,frame.attrib

  mov al,ds:[si]
  or al,al
  jz ende

loop1:

  mov es:[di],ax
  add di,2
  inc si
  mov al,ds:[si]
  or al,al
  jnz loop1

ende:

  pop es
  pop si
  pop di
  pop bp
  ret

_putstring  endp

;**************************
;*** PutString NEAR-FAR ***
;**************************

__fputstring  proc near

sframef   struc
fbp0       dw ?
fret0      dw ?
fxpos      dw ?
fypos      dw ?
fstrOffset dw ?
fstrSegment dw ?
fattrib    db ?
sframef    ends

frame     equ [bp - fbp0]

  push bp
  mov bp,sp

  push di
  push si
  push es
  push ds

  mov ax,0b800h
  mov es,ax

  mov di,frame.fypos
  dec di
  mov ax,di
  shl di,2
  add di,ax
  shl di,5

  add di,frame.fxpos
  add di,frame.fxpos
  sub di,2

  mov si,frame.fstrOffset
  mov ds,frame.fstrSegment
  mov ah,frame.fattrib

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
  ret

__fputstring  endp

_text      ends
           end

