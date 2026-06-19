

.386
model flat,prolog
locals

.data

.code

public allocDOSmem

allocDOSmem        proc

                   ; bx = paragraphs to alloc

                   mov ax,0100h

                   int 31h
                   jc @@error

                   ; ax = real mode segment
                   ; dx = protected mode selector

                   ret

        @@error:
                   mov ax,0ffffh
                   ret

endp

public freeDOSmem

freeDOSmem         proc

                   ; dx = protected mode selector

                   mov ax,0101h
                   int 31h

                   ; carry flag = error

                   ret

endp

end
