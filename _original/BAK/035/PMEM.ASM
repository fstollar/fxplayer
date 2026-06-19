            .code

;*** Reserviere Speicher in Paragraphen
;* Einsprung: ax=Anzahl der Paragraphen (16Byte)
;* RÅckgabe:  ax=Segment wo freier Speicherbereich
;*            Carry=kein Speicher frei

Getmem      proc near

            push bx
            mov bx,ax
            mov ax,4800h
            int 21h
            pop bx
            ret

Getmem      endp

;*** Lîse Speicherreservierung auf
;* Einsprung: ax=Segment das aufgelîst werden soll
;* RÅckgabe:  -

FreeMem     proc near

            push es
            mov es,ax
            mov ax,4900h
            int 21h
            pop es
            ret

FreeMem     endp
