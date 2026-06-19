
.386
model flat,prolog
locals

.data

.code

public _Delay

_Delay             proc

                   push ax
                   push bx

                   ; cx = cycles to wait = cx/1190000 sec

                   in al,40h
                   mov ah,al
                   in al,40h
                   mov bx,ax
    @@self:
                   in al,40h
                   mov ah,al
                   in al,40h
                   sub ax,bx
                   cmp ax,cx
                   jb @@self

                   pop bx
                   pop ax

                   ret
endp

end

