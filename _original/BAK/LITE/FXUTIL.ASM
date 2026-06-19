;******************************************
;*** Utilities for the F/X Player v0.38 ***

public              _PlayFF      ;play FastForward (skip Pattern)
public              _Pause       ;pause Playback / starts Playback again
;public              _Takt        ;return in dx:ax Taktcycles (only Pentium !)
;public              _ChangeMode
public              _ChangeInterp
public              _CallCommand
;public              _LoadS3M_Head

_PlayFF             proc far

                    cli
                    push eax

                    mov NewRowPos,0       ; save new row to start
                    mov al,RowPosNow      ; and end this pattern
                    inc al                ; in this row + 1
                    mov PatternEndRow,al  ; and do it

                    pop eax
                    sti
                    retf

_PlayFF             endp

PauseFlag           db 0

_Pause              proc far

                    push ax
                    cmp SB16flag,0
                    jz @@8bit

                    mov ah,0D5h       ; D5h = pause 16bit
                    add ah,PauseFlag  ; D6h = start 16bit
                    call WrDSP
                    xor PauseFlag,1
                    pop ax
                    retf
             @@8bit:
                    mov ah,0D0h
                    mov al,PauseFlag
                    shl al,2
                    add ah,al
                    call WrDSP
                    xor PauseFlag,1
                    pop ax
                    retf

_Pause              endp

_ChangeInterp       proc far

                    xor SBinterpolation,1
                    retf

_ChangeInterp       endp

Command         db "c:\command.com",0
parameter       dw 0, ?,?, 0ffh,0ffh, 0ffh,0ffh
Kommandozeile   db 1," ",13
saveSS          dw ?
saveESP         dd ?

_CallCommand        proc far

                    pushad
                    push ds
                    push es
                    push fs
                    push gs

                    cli
                    mov ax,cs
                    mov es,ax
                    mov ds,ax
                    mov bx,offset parameter
                    mov dx,offset command

                    mov parameter+4,ax
                    mov ax,offset Kommandozeile
                    mov parameter+2,ax

                    mov ax,ss
                    mov saveSS,ax
                    mov saveESP,esp

                    mov ax,4b00h
                    int 21h
                    mov esp,saveESP
                    mov ss,saveSS
                    jc @@error

                    sti
                    pop gs
                    pop fs
                    pop es
                    pop ds
                    popad
                    mov al,0
                    retf

@@error:
                    sti
                    pop gs
                    pop fs
                    pop es
                    pop ds
                    popad
                    mov al,1
                    retf

_CallCommand        endp


