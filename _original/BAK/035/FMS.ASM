;*** FMS - Flat Memory System procedures v1.1
; (c) by Apollo / STIGMA 1996/97
;
; NOTE: It runs only on PC with a 386 and above and no EMM should be loaded
;       else an error appear. Also no Windoze is allowed or any system where
;       Protected Mode or V86 Mode is already used.
;       You need a XMS manager loaded or no mem above 1meg can be accessed.
;
; A newer release will be in which EMM will be allowed and also WIN95
; users will be happy ! :)

.386p
LOCALS @@

public      _MemFreeXMS        ; returns how many KB XMS is free
public      _InitFMS           ; init FMS mode and returns success/errorcode

.code

;*** unlock XMS Block again ***

UnlockXMS   proc near

            push bx
            ;DX=Handle of Block
            mov ah,0Dh
            call dword ptr [XMMOffset]
            mov ah,bl
            pop bx
            ret

UnlockXMS   endp

;*** Lock XMS mem against its movement ***

LockXMS     proc near

            push dx
            push bx
            mov ah,0Ch
            ;DX=Handle ofn Block
            call dword ptr [XMMOffset]
            or ax,ax
            jz @@noLock   ;error

            mov ax,dx
            shl eax,16
            mov ax,bx
            ;EAX=32-Bit Offset of XMS
            pop bx
            pop dx
            ret

@@noLock:
            mov ah,bl
            pop bx
            pop dx
            ret

LockXMS     endp

;*** frees allocated XMS  ***

FreeXMS     proc near

            push bx
            ;DX=Handle of XMS mem
            mov ah,0Ah
            call dword ptr [XMMOffset]
            mov ah,bl
            pop bx
            ret

FreeXMS     endp

;*** allocate free XMS ***

AllocXMS    proc near

            push bx
            mov ah,09h
            ;DX=XMS memory in KB
            call dword ptr [XMMOffset]
            or ax,ax        ;if AX > 0 then error
            jz @@noAlloc

            ;DX=Handle of XMS memory
            pop bx
            ret

@@noAlloc:
            mov ah,bl       ;in AH errorcode
            pop bx
            ret

AllocXMS    endp

;*** how much free XMS ***

_MemFreeXMS proc near

            push dx

            mov ah,08h
            call dword ptr [XMMOffset]

            ;AX=amount of free XMS
            pop dx
            ret

_MemFreeXMS endp

;*** Initialise FMS ***

XMMOffset   dw ?
XMMSegment  dw ?
FMSCheck    db 0

_InitFMS    proc far

            pushad
            push es
            push ds
            push gs
            push fs

      ;*** Prozessor already in PMode ? ***

            mov eax,cr0
            test eax,1
            jnz @@PModeActive    ;if yes, EMM or something already loaded...

      ;*** Is there an XMS driver installed ? ***

            mov ax,4300h
            int 2Fh
            cmp al,80h
            jnz @@noXMS          ;If not go to end with error...

      ;*** get FAR-CALL XMM ***

            mov ax,4310h
            int 2Fh
            mov XMMSegment,es
            mov XMMOffset,bx     ;gets FAR call adress of XMS manager

      ;*** VersionNr from XMM ***

            mov ah,00
            call dword ptr [XMMOffset]
            cmp ax,0200h      ;if version XMM < 2.00
            jb @@noNewXMM     ;then too old...

    ;*** switch to the FLAT-MEMORY Mode ***

            mov ax,cs
            push ds
            mov ds,ax
            mov word ptr cs:aktseg,ax
            movzx eax,ax
            shl eax,4

            mov ebx,eax
            mov word ptr cs:segmente+0ah,ax
            mov word ptr cs:segmente+12h,ax
            ror eax,16
            mov byte ptr cs:segmente+0ch,al
            mov byte ptr cs:segmente+14h,al
            xor eax,eax
            mov ax,offset cs:segmente
            add ebx,eax
            mov word ptr cs:GDTPtr,GDTLen
            mov dword ptr cs:GDTPtr+2,ebx
            pushf

            cli
            in al,70h
            mov ah,al
            or al,80h
            out 70h,al                  ;no int allowed anymore
            and ah,80h
            mov ch,ah
            lgdt fword ptr cs:GDTPtr
            mov bx,ds
            mov dx,ss
            mov eax,cr0
            or al,1
            mov cr0,eax                 ;welcome to pmode
            db 0eah
            dw (OFFSET pmode)
            dw 8
            ;JMP FAR CS:PMODE

pmode:      mov ax,10h
            mov ss,ax
            mov ax,18h
            mov ds,ax
            mov es,ax
            mov fs,ax
            mov gs,ax
            mov eax,cr0
            and eax,not 1
            mov cr0,eax                 ;and back to INTEL reality
            db 0eah
            dw (offset rmode)
aktseg      dw ?
            ;JMP FAR CS:RMODE

rmode:      mov ss,dx
            mov ds,bx
            xor ax,ax
            mov es,ax
            mov gs,ax
            mov fs,ax
            in al,70h
            and al,not 80h
            or al,ch
            out 70h,al
            popf
            pop ds

            pop fs
            pop gs
            pop ds
            pop es
            popad
            mov al,00    ;all is gone
            mov FMSCheck,al
            retf

@@PModeActive:
            pop fs
            pop gs
            pop ds
            pop es
            popad
            mov al,01    ;Protected Mode activ
            mov FMSCheck,al
            retf

@@noXMS:
            pop fs
            pop gs
            pop ds
            pop es
            popad
            mov al,02    ;no XMS friver found
            mov FMSCheck,al
            retf

@@noNewXMM:
            pop fs
            pop gs
            pop ds
            pop es
            popad
            mov al,03    ;XMS driver found but too old !
            mov FMSCheck,al
            retf

_InitFMS    endp

;**********************************

GDTPtr      dq ?
segmente    dw 0,0,0,0
            ;GDT
            dw 0ffffh
            dw 0000
            ;CS,SS
            db 0
            db 9ah
            dw 0000
            dw 0ffffh
            dw 0000
            ;DS,ES,GS,FS
            db 0
            db 92h
            dw 0
            dw 0ffffh
            dw 0000
            db 0
            db 92h
            dw 0ffffh
seg_end label word

GDTLen      equ (offset seg_end-segmente-1)

