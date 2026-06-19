.386p
LOCALS @@

public      _MemFreeXMS
public      _InitFMS

.code

;*** Entsperrt XMS BLock wieder ***

UnlockXMS   proc near

            push bx
            ;DX=Handle des zu entsperrenden Blocks
            mov ah,0Dh
            call dword ptr [XMMOffset]
            mov ah,bl
            pop bx
            ret

UnlockXMS   endp

;*** Sperrt XMS Speicher gegen seine Verschiebung ***

LockXMS     proc near

            push dx
            push bx
            mov ah,0Ch
            ;DX=Handle des zu sperrenden Blocks
            call dword ptr [XMMOffset]
            or ax,ax
            jz @@noLock   ;Bei Fehler

            mov ax,dx
            shl eax,16
            mov ax,bx
            ;EAX=32-Bit Offset des XMS
            pop bx
            pop dx
            ret

@@noLock:
            mov ah,bl
            pop bx
            pop dx
            ret

LockXMS     endp

;*** Gibt Allokierten XMS wieder frei ***

FreeXMS     proc near

            push bx
            ;DX=Handle des XMS Speichers
            mov ah,0Ah
            call dword ptr [XMMOffset]
            mov ah,bl
            pop bx
            ret

FreeXMS     endp

;*** Allokiert freien XMS ***

AllocXMS    proc near

            push bx
            mov ah,09h
            ;DX=Grî·e in KB
            call dword ptr [XMMOffset]
            or ax,ax        ;wenn AX > 0 dann Fehler
            jz @@noAlloc

            ;DX=Handle des XMS Speichers
            pop bx
            ret

@@noAlloc:
            mov ah,bl       ;dann in AH Fehlercode
            pop bx
            ret

AllocXMS    endp

;*** Abfrage wieviel freier XMS ***

_MemFreeXMS proc near

            push dx

            mov ah,08h
            call dword ptr [XMMOffset]

            ;AX=Grî·e des freien XMS
            pop dx
            ret

_MemFreeXMS endp

;**********************************************
;********** Initialisiert FMS,   **************

XMMOffset   dw ?
XMMSegment  dw ?
FMSCheck    db 0

_InitFMS    proc near

            pushad
            push es
            push ds
            push gs
            push fs

      ;*** Prozessor bereits im PMode ? ***

            mov eax,cr0
            test eax,1
            jnz @@PModeActive    ;Wenn ja, dann tschÅss

      ;*** Ist ein XMS Treiber installiert ? ***

            mov ax,4300h
            int 2Fh
            cmp al,80h
            jnz @@noXMS

      ;*** FAR-CALL von XMM holen ***

            mov ax,4310h
            int 2Fh
            mov XMMSegment,es
            mov XMMOffset,bx

      ;*** VersionsNr von XMM holen ***

            mov ah,00
            call dword ptr [XMMOffset]
            cmp ax,0200h      ;Wenn XMM < 2.00
            jb @@noNewXMM     ;dann zu alt...

    ;*** Dann in den FLAT-MEMORY Modus umschalten ***

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
            out 70h,al
            and ah,80h
            mov ch,ah
            lgdt fword ptr cs:GDTPtr
            mov bx,ds
            mov dx,ss
            mov eax,cr0
            or al,1
            mov cr0,eax
            db 0eah
            dw (OFFSET pmode)
            dw 8
            ;JMP FAR CS:PMODE

pmode:      mov ax,10h
            mov ss,ax
            mov ax,18h
            mov ds,ax
            mov es,ax
            mov gs,ax
            mov fs,ax
            mov eax,cr0
            and eax,not 1
            mov cr0,eax
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
            mov al,00    ;Alles in Ordnung
            mov FMSCheck,al
            ret

@@PModeActive:
            pop fs
            pop gs
            pop ds
            pop es
            popad
            mov al,01    ;Protected Mode aktiv
            mov FMSCheck,al
            ret

@@noXMS:
            pop fs
            pop gs
            pop ds
            pop es
            popad
            mov al,02    ;Kein XMS Treiber gefunden
            mov FMSCheck,al
            ret

@@noNewXMM:
            pop fs
            pop gs
            pop ds
            pop es
            popad
            mov al,03    ;XMS Treiber gefunden aber zu alt !
            mov FMSCheck,al
            ret

_InitFMS    endp

;**********************************

GDTPtr      dq ?
segmente    dw 0,0,0,0
            ;GDT
            dw 0ffffh
            dw 0000
            ;CS
            db 0
            db 9ah
            dw 0000
            dw 0ffffh
            dw 0000
            ;DS
            db 0
            db 92h
            dw 0
            dw 0ffffh
            dw 0000
            ;GS
            db 0
            db 92h
            dw 0ffffh
seg_end label word

GDTLen      equ (offset seg_end-segmente-1)
