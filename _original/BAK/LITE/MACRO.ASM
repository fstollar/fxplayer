;ALIGN 4
LastTimer           dd 0
NowTimer            dd 0

INCLUDE             print.asm

TaktStart           MACRO

                    ;ALIGN 4
                    push edx
                    push eax
                    db 0fh,31h
                    mov NowTimer,eax
                    pop eax
                    pop edx   ;16h=leer = 17 oder 27

                    ENDM

TaktEnd             MACRO

                    ;ALIGN 4
                    push edx
                    push eax
                    db 0fh,31h
                    sub eax,NowTimer
                    mov NowTimer,eax
                    pop eax
                    pop edx

                    ENDM
