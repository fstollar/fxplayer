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

               push ax
               push dx

               mov dx,1408h
               mov al,ChannelAnzahl
               call PrintDez

               mov dx,1410h
               mov al,PatternOrderPlay
               call PrintDez
               mov al,PatternPlay
               add dx,3
               call PrintDez

               add dx,11
               mov al,RowPosNow
               call PrintDez

               mov dx,1601h
               mov al,PlayingFlag+0
               call PrintDez
               add dx,3
               mov al,PlayingFlag+1
               call PrintDez
               add dx,3
               mov al,PlayingFlag+2
               call PrintDez
               add dx,3
               mov al,PlayingFlag+3
               call PrintDez
               add dx,3
               mov al,PlayingFlag+4
               call PrintDez
               add dx,4
               mov al,PlayingFlag+5
               call PrintDez
               add dx,3
               mov al,PlayingFlag+6
               call PrintDez
               add dx,3
               mov al,PlayingFlag+7
               call PrintDez
               add dx,3
               mov al,PlayingFlag+8
               call PrintDez
               add dx,3
               mov al,PlayingFlag+9
               call PrintDez
               add dx,4
               mov al,PlayingFlag+10
               call PrintDez
               add dx,3
               mov al,PlayingFlag+11
               call PrintDez
               add dx,3
               mov al,PlayingFlag+12
               call PrintDez
               add dx,3
               mov al,PlayingFlag+13
               call PrintDez
               add dx,3
               mov al,PlayingFlag+14
               call PrintDez
               add dx,4
               mov al,PlayingFlag+15
               call PrintDez


               mov dx,1701h
               mov ax,ChannelVolume+[0*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[1*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[2*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[3*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[4*2]
               call PrintDez
               add dx,4
               mov ax,ChannelVolume+[5*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[6*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[7*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[8*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[9*2]
               call PrintDez
               add dx,4
               mov ax,ChannelVolume+[10*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[11*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[12*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[13*2]
               call PrintDez
               add dx,3
               mov ax,ChannelVolume+[14*2]
               call PrintDez
               add dx,4
               mov ax,ChannelVolume+[15*2]
               call PrintDez


               mov dx,1801h
               mov al,ChannelPanning+[0]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[1]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[2]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[3]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[4]
               call PrintDez
               add dx,4
               mov al,ChannelPanning+[5]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[6]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[7]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[8]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[9]
               call PrintDez
               add dx,4
               mov al,ChannelPanning+[10]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[11]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[12]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[13]
               call PrintDez
               add dx,3
               mov al,ChannelPanning+[14]
               call PrintDez
               add dx,4
               mov al,ChannelPanning+[15]
               call PrintDez

               mov dx,1a01h
               mov al,SampleNumber+0
               call PrintDez
               add dx,3
               mov al,SampleNumber+1
               call PrintDez
               add dx,3
               mov al,SampleNumber+2
               call PrintDez
               add dx,3
               mov al,SampleNumber+3
               call PrintDez
               add dx,3
               mov al,SampleNumber+4
               call PrintDez
               add dx,4
               mov al,SampleNumber+5
               call PrintDez
               add dx,3
               mov al,SampleNumber+6
               call PrintDez
               add dx,3
               mov al,SampleNumber+7
               call PrintDez
               add dx,3
               mov al,SampleNumber+8
               call PrintDez
               add dx,3
               mov al,SampleNumber+9
               call PrintDez
               add dx,4
               mov al,SampleNumber+10
               call PrintDez
               add dx,3
               mov al,SampleNumber+11
               call PrintDez
               add dx,3
               mov al,SampleNumber+12
               call PrintDez
               add dx,3
               mov al,SampleNumber+13
               call PrintDez
               add dx,3
               mov al,SampleNumber+14
               call PrintDez
               add dx,4
               mov al,SampleNumber+15
               call PrintDez


               pop dx
               pop ax

               ret

PrintInfo      endp
