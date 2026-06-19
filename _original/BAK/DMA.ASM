
.386
model flat,prolog
locals

.data

public DMAadress

;*** DMA memory datas

DMAadress          dd 0

DMAsegment         dw 0
DMAselector        dw 0

DMAlength          dw 0
DMAparagraph       dw 0

;*** DMA programming datas

DMA_Adress         db  00h, 02h, 04h, 06h, 0c0h,0c4h,0c8h,0cch
DMA_Count          db  01h, 03h, 05h, 07h, 0c2h,0c6h,0cah,0ceh
DMA_Page           db  87h, 83h, 81h, 82h, 088h,08bh,089h,08ah
DMA_UPage          dw 487h,483h,481h,482h, 488h,48bh,489h,48ah

DMA_Status         db 08h, 0d0h
DMA_Command        db 08h, 0d0h
DMA_Request        db 09h, 0d2h
DMA_ChMask         db 0ah, 0d4h
DMA_Mode           db 0bh, 0d6h
DMA_FlipFlop       db 0ch, 0d8h
DMA_CLR            db 0dh, 0dah
DMA_MaskCLR        db 0eh, 0dch
DMA_Mask           db 0fh, 0deh

DMAChannel         db ?                     ; from 0 to 7

DMAmode            db 8+16+64               ; = 010110xx
                   ; bit 0+1 : channel Nr
                   ; bit 2+3 : 00 = verify
                   ;           01 = writing (into mem)
                   ;           10 = reading (from mem)
                   ;           11 = not valid
                   ; bit 4   : 0 = Autoinit off
                   ;           1 = Autoinit on
                   ; bit 5   : 0 = increment adress
                   ;           1 = decrement adress
                   ; bit 6+7 : 00 = Demand mode
                   ;           01 = Single mode
                   ;           10 = Block mode
                   ;           11 = kaskade mode

.code

extrn allocDOSmem  :near
extrn freeDOSmem   :near

public initDMA_

initDMA_           proc

                   mov DMAChannel,al
                   mov DMAmode, bl
                   mov DMAlength,cx

      ;*** DMA*********************

                   xor dh,dh
                   movzx ebx,DMAChannel
                   shr ebx,2
                   mov dl,DMA_ChMask+ebx
                   mov al,DMAChannel      ; load channel number
                   or al,4                ; mask channel bit
                   out dx,al              ; mask this channel out

                   mov dl,DMA_Mode+ebx
                   mov al,DMAChannel
                   and al,3             ; first 2 bits
                   or al,DMAmode        ; writing mode bits
                   out dx,al            ; write into DMA

                   mov dl,DMA_FlipFlop+ebx
                   mov al,0h
                   out dx,al            ; reset Flip-Flops

                   movzx ebx,DMAChannel
                   mov dl,DMA_Adress+ebx

                   mov eax,DMAadress
                   mov cl,DMAChannel
                   shr cl,2             ; if high DMA it is 16 bit
                   shr eax,cl           ; divide / 2 if 16 bit
                   out dx,al            ; lo-adress of DMA buffers
                   mov al,ah
                   out dx,al            ; hi-adress of DMA buffers

                   mov dl,DMA_Page+ebx
                   mov eax,DMAadress
                   shr eax,16           ; high bits of adress
                   out dx,al            ; Lower Page Register of DMA
                   mov al,ah
                   mov dx,DMA_UPage+[ebx*2]
                   ;out dx,al      ; Upper Page Register
                                   ; some older motherboards have problems
                                   ; with this port... :(
                   xor dh,dh
                   mov dl,DMA_Count+ebx
                   mov ax,DMAlength
                   dec ax               ; -1
                   out dx,al            ; load DMA counter with lo-byte length
                   mov al,ah
                   out dx,al            ; load DMA counter with hi-byte length

                   shr ebx,2
                   mov dl,DMA_ChMask+ebx
                   mov al,DMAChannel
                   and al,3
                   out dx,al            ; mask channel in again :)

                   ret
endp


public allocDMAmem_

allocDMAmem_       proc

                   ; bx = bytes to alloc

                   mov DMAlength,bx

                   add bx,8
                   shr bx,4   ; round up to next paragraph

                   mov DMAparagraph,bx

                   call allocDOSmem
                   jc @@error

       @@tryagain:

                   mov DMAsegment,ax
                   mov DMAselector,dx
                   mov bx,ax
                   and bx,0f000h
                   add ax,DMAparagraph
                   and ax,0f000h
                   cmp ax,bx
                   jz @@DMAblockfound

                       mov bx,DMAparagraph

                       call allocDOSmem
                       jc @@error

                       mov DMAsegment,ax
                       xchg DMAselector,dx

                       call freeDOSmem
                       mov dx,DMAselector
                       jmp @@tryagain
      @@DMAblockfound:

                   mov ax,DMAsegment
                   mov dx,DMAselector

                   movzx ebx,ax
                   shl ebx,4
                   mov DMAadress,ebx

                   ; ax = real mode segment
                   ; dx = segment selector

                   ret

           @@error:
                   xor ax,ax
                   xor dx,dx
                   ret
endp


public freeDMAmem_

freeDMAmem_        proc

                   ; dx = protected mode selector

                   call freeDOSmem
                   jc @@error

                   ; carry flag = error

                   xor eax,eax
                   ret

         @@error:
                   mov ax,0ffffh
                   ret
endp

public clearDMAmem_

clearDMAmem_       proc

                   ; ax = value to write into DMA buffer
                   ; cx = length of DMA buffer

                   ;push ds
                   ;pop es

                   and ecx,0ffffh
                   shr ecx,1
                   mov edi,DMAadress
                   ;rep stosw

                   ret
endp

end
