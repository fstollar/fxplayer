LastTimer           dd 0
NowTimer            dd 0

TaktStart           MACRO

                    push edx
                    push eax
                    db 0fh,31h
                    mov NowTimer,eax
                    pop eax
                    pop edx   ;16h=leer = 17 oder 27

                    ENDM

TaktEnd             MACRO

                    push edx
                    push eax
                    db 0fh,31h
                    sub eax,NowTimer
                    mov NowTimer,eax
                    pop eax
                    pop edx

                    ENDM
