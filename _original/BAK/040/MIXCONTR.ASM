.code

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
                         mov eax,SampleScaleWide+[ebp*4]
              @@noWide:
                      mov ebx,eax
                      shl eax,16
                      mov ScalingLow,eax          ; low 16bit in high 32bit
                      shr ebx,16
                      mov ScalingHigh,ebx         ; high 16bit in low 32bit

 ;******************* Prepare Registers **********************

                     xor ebx,ebx
                     xor ecx,ecx

                     mov ax,ChannelVolume+[ebp*2]
                     ;mul GlobalVolume
                     ;shr ax,6
                     ;mov bh,al
                     ;mov ch,al

                     ;shr bx,8
                     ;shr cx,8
                     mov VolumeLeft+[ebp*2],ax
                     mov VolumeRight+[ebp*2],ax

                     mov eax,VolHalveOffset
                     movzx ebx,VolumeLeft+[ebp*2]
                     shl ebx,8
                     add ebx,eax                  ; VolumeLeft ready in ebx
                     movzx ecx,VolumeRight+[ebp*2]
                     shl ecx,8
                     add ecx,eax                  ; VolumeRight ready in ecx

                     mov esi,SampleOffsetHigh+[ebp*4]
                     add esi,SampleStartOffset+[ebp*4]
                     mov edx,SampleOffsetLow+[ebp*4] ;momentale Pos als Fixkomma

                     mov dx,Counter ; Ticker setzen
                     ret

PrepareMixer         endp

Interpolation       MACRO

                    ;TaktStart

                    push edx
                     ;xor eax,eax
                     shr edx,16

                     movzx ax,byte ptr es:[esi+1]
                     sub al,bl
                     sbb ah,0
                     movsx eax,ax

                     imul edx

                     ;add eax,32768
                     shr eax,16
                    pop edx
                     add bl,al

                    ;TaktEnd

                    ENDM

