;******************************************
;*** Utilities for the F/X Player v0.38 ***

public              _PlayFF      ;play FastForward (skip Pattern)
public              _Pause       ;pause Playback / starts Playback again
public              _Takt        ;return in dx:ax Taktcycles (only Pentium !)
public              _ChangeMode
public              _ChangeInterp
public              _CallCommand
public              _LoadS3M_Head

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

_Takt               proc far

                    mov eax,NowTimer
                    bswap eax
                    mov dx,ax
                    xchg dh,dl
                    bswap eax
                    retf

_Takt               endp

_SetGlobalV         proc far

                    mov GlobalVolume,bl
                    retf

_SetGlobalV         endp

_ChangeMode         proc far

                    push ebx
                    push ecx
                    cmp SBstereoFlag,2
                    jz @@stereo
                    cmp SBstereoFlag,1
                    jnz @@ende

                    mov SBstereoFlag,2
                    pop ecx
                    pop ebx
                    retf

            @@stereo:
                    mov SBstereoFlag,1
      @@ende:
                    pop ecx
                    pop ebx
                    retf

_ChangeMode         endp

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

HeadHandle          dw ?

_LoadS3M_Head       proc far

sloads3mH        struc
bpS3MH           dw 0
retS3MH          dd 0
FNameOffset      dw 0
FNameSegment     dw 0
BufferOffset     dw 0
BufferSegment    dw 0
sloads3mH        ends

loads3mH         equ [bp - bpS3MH]   ; set bp to structure above

                 push bp
                 mov bp,sp
                 pushad
                 push ds
                 push es
                 push fs
                 push gs

            ;*** open S3M File ***

                 mov ax, loads3mH.FNameSegment
                 mov ds,ax
                 mov dx, loads3mH.FNameOffset

                 mov ah,3dh                 ; open S3M file
                 mov al,10010000b
                 int 21h
                 jc @@notfound              ; if not found ...
                 mov HeadHandle,ax          ; save filehandle

                 mov ah,42h                 ; LSEEK
                 mov al,00                  ; begin of S3M File
                 mov bx,HeadHandle
                 xor cx,cx
                 xor dx,dx
                 int 21h                    ; now the filepointer is at start
                 jc @@notfound              ; if not found ...

            ;*** Load S3M Header ***

                 mov ax,loads3mH.BufferSegment
                 mov ds,ax                  ; load ds with buffer
                 mov dx,loads3mH.BufferOffset
                 mov cx,60h                ; and load 60h bytes block

      @@loadS3M:
                 mov bx,HeadHandle   ; mov bx with filehandle
                 mov ah,3Fh          ; load file via handle
                 int 21h             ; and energy.....

                 mov ah,3Eh
                 int 21h
                 xor bx,bx
                 mov HeadHandle,bx

                 pop gs
                 pop fs
                 pop es
                 pop ds
                 popad
                 pop bp
                 mov al,0
                 retf

@@notfound:
                 mov bx,HeadHandle
                 or bx,bx
                 jz @@notyet

                 mov ah,3Eh
                 int 21h
                 xor bx,bx
                 mov HeadHandle,bx
        @@notyet:

                 pop gs
                 pop fs
                 pop es
                 pop ds
                 popad
                 pop bp
                 mov al,1
                 retf

_LoadS3M_Head       endp
