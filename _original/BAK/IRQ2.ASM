
.386
model flat,prolog
locals

.data

InteruptNr         db ?
IRQNumber          db ?
IRQMaskAdress      dw ?
OldIRQMask         db ?
OldInteruptSel     dw ?
OldInteruptOff     dd ?

.code

extrn PlainInterupt  :near

public setNewInterupt_

setNewInterupt_    proc

                   ; bl = irq number

                   mov IRQNumber,bl
                   mov IRQMaskAdress,21h

                   cmp bl,8
                   jb @@noHighIRQ

                      add bl,70h-8
                      mov IRQMaskAdress,0A1h
       @@noHighIRQ:
                   add bl,8
                   mov InteruptNr,bl

                   mov ax,204h            ; get interupt
                                          ; bl = interupt number
                   int 31h                ; DPMI call

                   mov OldInteruptSel,cx
                   mov OldInteruptOff,edx

                   mov ax,205h
                   mov cx,SEG CardInterupt
                   mov edx,OFFSET CardInterupt

                   int 31h                ; DPMI call
                   jc @@error

                   mov dx,IRQMaskAdress
                   in al,dx
                   mov cl,IRQNumber
                   and cl,7
                   mov ah,al
                   shr ah,cl
                   and ah,1
                   mov OldIRQMask,ah

                   mov ah,1
                   shl ah,cl
                   xor ah,255
                   and al,ah
                   out dx,al

                   xor eax,eax
         @@error:
                   ret
endp


public ResetInterupt_

ResetInterupt_      proc

                    mov dx,IRQMaskAdress
                    in al,dx
                    mov ah,OldIRQMask
                    mov cl,IRQNumber
                    and cl,7
                    shl ah,cl
                    or al,ah
                    out dx,al

                    mov ax,205h            ; set interupt
                    mov bl,InteruptNr      ; bl = interupt number
                    mov cx,OldInteruptSel
                    mov edx,OldInteruptOff

                    int 31h                ; DPMI call
                    jc @@error

                    xor eax,eax
      @@error:
                    ret
endp



CardInterupt       proc

                   push es

                   push ds
                   pop es

                   pushad

                   call PlainInterupt

                  ; END OF INTERUPT

                   mov al,20h

                   cmp IRQNumber,8
                   jb @@noHighInt
                      out 0A0h,al
          @@noHighInt:
                   out 20h,al

                   popad
                   pop es
                   iret
endp

end
