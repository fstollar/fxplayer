;*** Routinen zur Zahlenausgabe ***

PrintHex       proc near

               push ax
               push bx
               push dx
               push di
               ;al=ByteZahl
               ;Dx=Cursor

               mov bx,0b800h
               mov es,bx

               movzx di,dh
               xor dh,dh
               dec di
               mov bx,di
               shl di,2
               add di,bx
               shl di,5

               add di,dx
               add di,dx
               sub di,2    ;Offset

               mov ah,al
               shr al,4
               and ah,15

               add al,30h
               cmp al,57
               jng not1
               add al,7
         not1:
               mov dl,al
               mov dh, 15
               mov es:[di],dx   ;erstes Zeichen

               add ah,30h
               cmp ah,57
               jng not2
               add ah,7
         not2:
               mov dl,ah
               mov es:[di+2],dx  ;zweites Zeichen

               pop di
               pop dx
               pop bx
               pop ax
               ret

PrintHex       endp

PrintDez       proc near

               push ax
               push bx
               push dx
               push di
               ;al=ByteZahl
               ;Dx=Cursor

               mov bx,0b800h
               mov es,bx

               movzx di,dh
               xor dh,dh
               dec di
               mov bx,di
               shl di,2
               add di,bx
               shl di,5

               add di,dx
               add di,dx
               sub di,2    ;Offset

               xor ah,ah
               mov bl,10
               div bl

               add al,30h
               mov dl,al
               mov dh, 15
               mov es:[di],dx   ;erstes Zeichen

               add ah,30h
               mov dl,ah
               mov es:[di+2],dx  ;zweites Zeichen

               pop di
               pop dx
               pop bx
               pop ax
               ret

PrintDez       endp

PrintInfo      proc near

               mov dx,140ah
               mov al,ChannelAnzahl
               call PrintDez

               mov dx,1415h
               mov al,PatternOrderPlay
               call PrintDez
               mov ax,OrdNum
               sub ax,2
               add dx,3
               call PrintDez
               mov al,PatternPlay
               add dx,3
               call PrintDez

               add dx,11
               mov al,RowPos
               call PrintDez

               mov dx,1607h
               mov al,InitSpeed
               call PrintDez
               add dx,3
               mov al,InitTempo
               call PrintHex

               mov eax,NowTimer

               mov dx,1811h
               call PrintHex
               shr eax,8
               mov dx,180fh
               call PrintHex
               shr eax,8
               mov dx,180dh
               call PrintHex
               ret

PrintInfo      endp
