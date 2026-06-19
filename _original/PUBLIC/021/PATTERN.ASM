;*********************************************************
;*** Routinen zum entpacken von "Pattern" und kopieren ***

ExpandPattern    proc near

                 push ax   ; al = Nr von Pattern
                 push bx
                 push cx
                 push dx
                 push si
                 push di
                 push es
                 push ds

              ;** Altes Pattern erst l”schen **

                 mov es,PatternNexSeg
                 xor di,di
 @@ClearPattern:
                 mov dword ptr es:[di],0FFFF00FFh
                 mov byte ptr es:[di+4],0
                 add di,5
                 cmp di,5*16*64
                 jb @@ClearPattern

              ;* Schleife vorbereiten *

                 xor di,di
                 xor bx,bx
                 mov ds,PatternSegment
                 xor ah,ah
                 mov si,ax       ;NrPattern
                 add si,si       ;*2
                 mov ax,ds:[si]  ;Segment von Pattern holen
                 cmp ax,0
                 jz @@noPattern
                 mov ds,ax
                 xor si,si

              ;* DIE SCHLEIFE *

  @@EntpackPattern:
                 mov al,ds:[si]    ; Raw-BytePattern
                 inc si            ; und ein weiter
                 or al,al
                 jz @@Rowfertig    ; wenn NULL

                 movzx bx,al       ; Kanalangabe
                 and bx,31         ; ausmaskieren
                 mov dx,bx
                 shl bx,2          ; und mal 5
                 add bx,dx         ; weil ein Kanal 5 Bytes braucht

                 test al,32        ; Bit 5 gesetzt
                 jz @@noNoteIns    ; wenn nicht

                 mov dx,ds:[si]    ; Note+Instrument
                 mov es:[di+bx],dx ; kopieren
                 add si,2
    @@noNoteIns:
                 test al,64        ; Bit 6 gesetzt
                 jz @@noVolume

                 mov dl,ds:[si]    ; Volume
                 mov es:[di+bx+2],dl
                 inc si
    @@noVolume:
                 test al,128       ; Bit 7 gesetzt
                 jz @@noCommand

                 mov dx,ds:[si]    ; Command+Info
                 mov es:[di+bx+3],dx
                 add si,2
    @@noCommand:
                 ;add bx,5          ; einen Channel weiter
                 ;cmp bx,80         ; wenn noch nicht letzter Kanal
                 jmp @@EntpackPattern ; und weiter ...
    @@Rowfertig:
                 xor bx,bx         ; erster Channel
                 add di,80         ; n„chste Zeile
                 cmp di,5120       ; Ende des Patterns ?
                 jb @@EntpackPattern
 @@noPattern:
                 pop ds
                 pop es
                 pop di
                 pop si
                 pop dx
                 pop cx
                 pop bx
                 pop ax
                 ret

ExpandPattern    endp

;*** Kopiert Channel von PatternNext ins PatternAktuell ***

CopyChannel      proc near

                 push ax
                 push bx         ;al=Kanalangabe
                 push si
                 push di
                 push es
                 push ds

                 mov ds,PatternNexSeg
                 mov es,PatternAktSeg

                 xor ah,ah    ;Kanalangabe
                 mov bx,ax    ;mal 5 da ein Kanal
                 shl ax,2     ;5 Bytes ben”tigt
                 add ax,bx    ;

                 mov si,ax
                 mov di,ax
                 mov bl,64

                 ;*** Schleife ***
    CopyOn:
                 mov eax,ds:[si]     ;liest 4 Bytes
                 mov es:[di],eax
                 mov bh,ds:[si+4]    ;und noch eins
                 mov es:[di+4],bh

                 add si,5*16         ;Eine Zeile nach unten
                 add di,5*16
                 dec bl              ;schon fertig
                 jnz CopyOn

                 pop ds
                 pop es
                 pop di
                 pop si
                 pop bx
                 pop ax
                 ret

CopyChannel      endp

