;*********************************************************
;*** Routinen zum entpacken von "Pattern" ***

PatternPointer   dd ?

ExpandFirstRow   proc near

                 pushad    ; al = Nr von Pattern
                 push ds

                 xor bx,bx
                 mov ds,bx

              ;** Altes Pattern erst l”schen **

                 mov edi,PatternAktOffset
                 mov ebx,0FFFF00FFh
                 xor cl,cl
                 mov ch,16
ALIGN 4
 @@ClearRow:
                 mov dword ptr ds:[edi],ebx
                 mov byte ptr ds:[edi+4],cl
                 add edi,5
                 dec ch
                 jnz @@ClearRow

              ;* Schleife vorbereiten *

                 mov edi,PatternAktOffset
                 xor ebx,ebx
                 xor ah,ah

                 movzx esi,al       ; NrPattern
                 shl esi,2          ; *4 for Offsettable
                 add esi,PatternOffsetPointer
                 mov esi,ds:[esi]   ; get 32bit Offset of Pattern
                 cmp esi,0
                 jz @@noPattern
                 add esi,2          ; add 2 coz length-word in pattern

ALIGN 4
              ;* DIE SCHLEIFE *

  @@EntpackPattern:
                 mov al,ds:[esi]    ; Raw-BytePattern
                 inc esi            ; und ein weiter
                 or al,al
                 jz @@Rowfertig    ; wenn NULL

                 movzx ebx,al       ; Kanalangabe
                 and ebx,31         ; ausmaskieren
                 mov edx,ebx
                 shl ebx,2          ; und mal 5
                 add ebx,edx        ; weil ein Kanal 5 Bytes braucht

                 test al,32        ; Bit 5 gesetzt
                 jz @@noNoteIns    ; wenn nicht

                 mov dx,ds:[esi]    ; Note+Instrument
                 mov ds:[edi+ebx],dx ; kopieren
                 add esi,2
    @@noNoteIns:
                 test al,64        ; Bit 6 gesetzt
                 jz @@noVolume

                 mov dl,ds:[esi]    ; Volume
                 mov ds:[edi+ebx+2],dl
                 inc esi
    @@noVolume:
                 test al,128       ; Bit 7 gesetzt
                 jz @@noCommand

                 mov dx,ds:[esi]    ; Command+Info
                 mov ds:[edi+ebx+3],dx
                 add esi,2
    @@noCommand:
                 jmp @@EntpackPattern
    @@Rowfertig:
                 mov PatternPointer,esi
    @@noPattern:
                 pop ds
                 popad
                 ret

ExpandFirstRow   endp

ExpandNextRow    proc near

                 pushad
                 push ds

                 xor bx,bx
                 mov ds,bx

              ;** Altes Pattern erst l”schen **

                 mov edi,PatternAktoffset
                 mov ebx,0FFFF00FFh
                 xor cl,cl
                 mov ch,16
ALIGN 4
 @@ClearRow:
                 mov dword ptr ds:[edi],ebx
                 mov byte ptr ds:[edi+4],cl
                 add edi,5
                 dec ch
                 jnz @@ClearRow

              ;* Schleife vorbereiten *

                 mov edi,PatternAktOffset
                 xor ebx,ebx
                 xor ah,ah

                 mov esi,PatternPointer  ; where did I stop it last time ?
ALIGN 4
              ;* DIE SCHLEIFE *

  @@EntpackPattern:
                 mov al,ds:[esi]    ; Raw-BytePattern
                 inc esi            ; und ein weiter
                 or al,al
                 jz @@Rowfertig    ; wenn NULL

                 movzx ebx,al       ; Kanalangabe
                 and ebx,31         ; ausmaskieren
                 mov edx,ebx
                 shl ebx,2          ; und mal 5
                 add ebx,edx        ; weil ein Kanal 5 Bytes braucht

                 test al,32        ; Bit 5 gesetzt
                 jz @@noNoteIns    ; wenn nicht

                 mov dx,ds:[esi]    ; Note+Instrument
                 mov ds:[edi+ebx],dx ; kopieren
                 add esi,2
    @@noNoteIns:
                 test al,64        ; Bit 6 gesetzt
                 jz @@noVolume

                 mov dl,ds:[esi]    ; Volume
                 mov ds:[edi+ebx+2],dl
                 inc esi
    @@noVolume:
                 test al,128       ; Bit 7 gesetzt
                 jz @@noCommand

                 mov dx,ds:[esi]    ; Command+Info
                 mov ds:[edi+ebx+3],dx
                 add esi,2
    @@noCommand:
                 jmp @@EntpackPattern
    @@Rowfertig:
                 mov PatternPointer,esi
    @@noPattern:
                 pop ds
                 popad
                 ret

ExpandNextRow    endp
