;******************************
;******** Reset Player ********
;******************************

_ResetPlay  proc far

            push es
            push ebp

            ;******************************
            ;*** Alte Variablen l”schen ***


            mov ebp,0
            ;mov es,bp
            ;mov dword ptr es:[12],0f000fff0h
            ;mov dword ptr es:[04],0f000fff0h   ; little anti-debug
@@KillVariable:

            inc bp
            cmp bp,16
            jb @@KillVariable

            pop ebp
            pop es
            retf

_ResetPlay  endp

