;*******************************************************************
;*** Mastervolume Berechnung in den Reservierten Mastervol Array ***

MasterVolHandle dw ?
MasterVolOffset dd ?

ALIGN 4
;********************************************************+
;**** Mastervolumeing in 16 Bit

MasterCalc16 proc near

@@MixFlow:
               ;TaktStart

               mov ax, es:[si+0]
               mov bx, es:[si+2]
               mov cx,ds:[ebp+eax*2]
               mov ax, es:[si+4]
               mov dx,ds:[ebp+ebx*2]
               mov ds:[edi],cx
               mov cx,ds:[ebp+eax*2]
               mov ds:[edi+2],dx
               mov dx, es:[si+6]
               add si,8
               mov ds:[edi+4],cx
               mov bx,ds:[ebp+edx*2]
               mov ds:[edi+6],bx

               ;TaktEnd

               add edi,8
               cmp si,MBlockLength  ;Block fertig
               jnz @@MixFlow
               ret

MasterCalc16 endp

ALIGN 4
;**** Mastervoluming in 8 Bit

MasterCalc8  proc near

@@MixFlow:
               mov ax, es:[si+0]    ;Wert aus Kan„len
               mov bx, es:[si+2]
               mov cl,ds:[ebp+eax]       ;Wert aus MasterVolume Tabelle auslesen
               mov ax, es:[si+4]
               mov ch,ds:[ebp+ebx]
               mov bx, es:[si+6]
               mov dl,ds:[ebp+eax]
               mov dh,ds:[ebp+ebx]

               mov ds:[edi],cx       ;und in PlayBuffer schreiben
               mov ds:[edi+2],dx

               add si,8         ;zwei Byte im Kanal weiter
               add edi,4         ;ein Byte im PlayBuffer weiter
               cmp si,MBlockLength  ;Block fertig
               jnz @@MixFlow
               ret

MasterCalc8  endp

;****************************************************
;**** Mastervolume Tabelle im 16 Bit Format erstellen

CalcMVol16   proc near

             pushad
             push ds
             push es
             push fs
             push gs

             xor ebp,ebp

             mov fs,bp
             mov edi,MasterVolOffset
             mov ecx,ebp
    @@Clear1:
             mov dword ptr fs:[edi],0
             add edi,4
             add ecx,4
             cmp ecx,16384*2
             jb @@Clear1

       ;** Calc MVOL *****
             movzx ebx,MasterVolume
             and ebx,127
             cmp ebx,16
             jge @@noMVolcor
             mov ebx,16
   @@noMVolcor:
             mov eax,32768*16
             xor edx,edx

             div ebx

             mov ecx,eax    ;cx=c
             mov ebx,32768
             sub ebx,eax
              inc ebx
             shr ebx,1
             mov eax,ebx    ;ax=a

             add ebx,ecx    ;bx=b

          ;*** Schleife ***
  @@calcM:
             cmp ebp,eax
             jnb @@noMaster1
             mov word ptr fs:[edi+ebp*2],0
  @@noMaster1:
             cmp eax,ebp
             jg @@noMaster2
             cmp ebp,ebx
             jnb @@noMaster2

             push eax
             mov edx,ebp
             sub edx,eax
             xor eax,eax

             div ecx
             add eax,32768      ;Runden
             shr eax,16
             mov word ptr fs:[edi+ebp*2],ax
             pop eax
  @@noMaster2:
             cmp ebp,ebx
             jb @@noMaster3
             mov word ptr fs:[edi+ebp*2],65535
  @@noMaster3:
             inc ebp
             cmp ebp,65536-16384
             jb @@calcM

             pop gs
             pop fs
             pop es
             pop ds
             popad
             ret

CalcMVol16   endp

;**** Mastervolume Tabelle im 8 Bit Format erstellen

CalcMVol8    proc near

             pushad
             push ds
             push es
             push fs
             push gs

             xor ebp,ebp

             mov fs,bp
             mov edi,MasterVolOffset
             mov ecx,ebp
    @@Clear1:
             mov dword ptr fs:[edi],0
             add edi,4
             add ecx,4
             cmp ecx,16384
             jb @@Clear1

       ;** Calc MVOL *****
             movzx ebx,MasterVolume
             and ebx,127
             cmp ebx,16
             jge @@noMVolcor
             mov ebx,16
   @@noMVolcor:
             mov eax,32768*16
             xor edx,edx

             div ebx

             mov ecx,eax    ;cx=c
             mov ebx,32768
             sub ebx,eax
              inc ebx
             shr ebx,1
             mov eax,ebx    ;ax=a

             add ebx,ecx    ;bx=b

          ;*** Schleife ***
  @@calcM:
             cmp ebp,eax
             jnb @@noMaster1
             mov byte ptr fs:[edi+ebp],0
  @@noMaster1:
             cmp eax,ebp
             jg @@noMaster2
             cmp ebp,ebx
             jnb @@noMaster2

             push eax
             mov edx,ebp
             sub edx,eax
             xor eax,eax

             div ecx
             shr eax,16
             add ax,128      ;Runden
             sbb ah,0        ;FF bei šberlauf
             mov byte ptr fs:[edi+ebp],ah
             pop eax
  @@noMaster2:
             cmp ebp,ebx
             jb @@noMaster3
             mov byte ptr fs:[edi+ebp],255
  @@noMaster3:
             inc ebp
             cmp ebp,65536-16384
             jb @@calcM

             pop gs
             pop fs
             pop es
             pop ds
             popad
             ret

CalcMVol8    endp
