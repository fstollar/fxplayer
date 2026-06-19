;***************************************************************************
;*** functions for controlling the mixer, preparing inner mixer loops... ***
;*** (c) 1997 Apollo / STIGMA for v0.40


.code


Modify_code          proc near

                     mov eax,ScalingLow
                     mov dword ptr ScaleLowML0+[3],eax
                     mov dword ptr ScaleLowME0+[3],eax

                     mov eax,ScalingHigh
                     mov dword ptr ScaleHighML0+[3],eax
                     mov dword ptr ScaleHighME0+[3],eax

                     mov eax,SampleLoopEnding
                     mov dword ptr LoopEndML0+[3],eax

                     mov eax,SampleEnding
                     mov dword ptr SampleEndME0+[3],eax

                     ret

Modify_code          endp


PrepareMixer         proc near

                      mov al,SampleFlag+[ebp]
                      mov SampleLoopFlag,al

                      mov eax,SampleLoopStartOffset+[ebp*4]
                      mov SampleLoopBegining,eax

                      mov eax,SampleLoopEndOffset+[ebp*4]
                      mov SampleLoopEnding,eax

                      mov eax,SampleEndOffset+[ebp*4]
                      mov SampleEnding,eax

                      mov eax,SampleScale+[ebp*4] ; load 32bit scaling factor

                      test SBstereoFlag,1
                      jz @@noWide
                         shr eax,1
              @@noWide:
                      mov ebx,eax
                      shl eax,16
                      mov ScalingLow,eax          ; low 16bit in high 32bit
                      shr ebx,16
                      mov ScalingHigh,ebx         ; high 16bit in low 32bit

 ;******************* Prepare Registers **********************

                     xor ebx,ebx
                     xor ecx,ecx

                     mov ax,ChannelVolume+[ebp*2]  ; look actual volume
                     mul GlobalVolume              ; multiply with global volume
                     add ax,32                     ; round it
                     shr ax,6                      ; and cut down to 64
                     mov bx,ax                     ; save it

                     test SBstereoFlag,2
                     jz @@noPanning

                        call Panning
         @@noPanning:
                     mov VolumeLeft+[ebp*2],ax     ; into left/
                     mov VolumeRight+[ebp*2],bx    ; right values

                     mov eax,VolHalveOffset        ; load half-offset of volume table
                     movzx ebx,VolumeLeft+[ebp*2]
                     shl ebx,8
                     add ebx,eax                  ; VolumeLeft ready in ebx
                     movzx ecx,VolumeRight+[ebp*2]
                     shl ecx,8
                     add ecx,eax                  ; VolumeRight ready in ecx

                     mov esi,SampleOffsetHigh+[ebp*4]
                     add esi,SampleStartOffset+[ebp*4]
                     mov edx,SampleOffsetLow+[ebp*4] ;momentale Pos als Fixkomma

                     mov dx,Counter  ; set counter into dx
                     ret

PrepareMixer         endp

Interpolation       MACRO

                    ;TaktStart

                    mov ax,bx
                    push edx
                     mov al,byte ptr ds:[esi+1]
                     shr edx,24
                     sub ax,bx

                     mul dx

                    pop edx
                    add bl,ah

                    ;TaktEnd

                    ENDM


;-------------------------------

Counter              dw 0
CounterFlag          db 0

CalcTickBuffer       proc near

                     mov dx,CountTick+[ebp*2]
                     mov ax,BufferBytes+[ebp*2]
                     mov cx,MBlockLength

                     cmp SBstereoFlag,0
                     jz @@noStereo1

                ;******* STEREO *******

                        xor bx,bx
                        add ax,dx
                        add ax,dx
                        add ax,dx
                        add ax,dx
                        mov CounterFlag,0

                        cmp ax,cx
                        jbe @@noOverflowS

                            sub cx,BufferBytes+[ebp*2]
                            shr cx,2
                            mov dx,cx           ; dx Counter fertig

                            mov bx,ax
                            mov ax,MBlockLength ; ax BufferBytes fertig
                            sub bx,ax
                            shr bx,2            ; bx CounterTick fertig

                            mov CounterFlag,1

         @@noOverflowS:
                     jmp @@ready

           ;*********** MONO ************
         @@noStereo1:
                     xor bx,bx
                     add ax,dx
                     add ax,dx
                     mov CounterFlag,0

                     cmp ax,cx
                     jbe @@noOverflowM

                        sub cx,BufferBytes+[ebp*2]
                        shr cx,1
                        mov dx,cx           ; dx Counter fertig

                        mov bx,ax
                        mov ax,MBlockLength ; ax BufferBytes fertig
                        sub bx,ax
                        shr bx,1            ; bx CounterTick fertig

                        mov CounterFlag,1

         @@noOverflowM:
         @@ready:
                     mov BufferBytes+[ebp*2],ax  ; pos in the buffer
                     mov CountTick+[ebp*2],bx    ; how much left after loop
                     mov Counter,dx              ; new counter variable

                     ret

CalcTickBuffer       endp


PanningTable    db 64,00, 64,09, 64,18, 64,27, 64,36, 64,45, 64,54
                db 64,64
                db 56,64, 48,64, 40,64, 32,64, 24,64, 16,64, 08,64, 00,64

Panning              proc near

                     mov cl,ChannelPanning+[ebp]
                     and ecx,255
                     add ecx,ecx

                     mov dl,PanningTable+[ecx]
                     mul dl
                     xchg ax,bx

                     mov dl,PanningTable+[ecx+1]
                     mul dl
                     xchg ax,bx

                     add ax,32
                     add bx,32
                     shr ax,6
                     shr bx,6

                     ret

Panning              endp
