.code

Counter              dw 0

CalcTickBuffer       proc near

                     movzx ebp,ChannelAktuell

                     mov dx,CountTick+[ebp*2]
                     mov ax,BufferBytes+[ebp*2]
                     mov cx,MBlockLength/2

                     cmp SBstereoFlag,0
                     jz @@noStereo1
                        shr cx,1
         @@noStereo1:
                     xor bx,bx
                     add ax,dx

                     cmp ax,cx
                     jbe @@noOverflow

                        mov bx,ax
                        mov dx,BufferBytes+[ebp*2]
                        mov ax,MBlockLength/2
                        sub bx,ax
                        sub ax,dx
                        mov dx,ax

                        mov ax,cx
         @@noOverflow:
                     mov BufferBytes+[ebp*2],ax
                     mov CountTick+[ebp*2],bx
                     mov Counter,dx
                     ret

CalcTickBuffer       endp

PrepareMixer         proc near

                      movzx ebp,ChannelAktuell

                      mov al,SampleFlag+bp
                      mov SampleLoopFlag,al
                      mov eax,SampleLoopBegin+[ebp*4]
                      mov SampleLoopBegining,eax
                      mov eax,SampleLoopEnd+[ebp*4]
                      mov SampleLoopEnding,eax
                      mov eax,SampleLength+[ebp*4]
                      mov SampleLengthing,eax
                      mov eax,SampleScaleLow+[ebp*4]
                      mov ScalingLow,eax
                      mov eax,SampleScaleHigh+[ebp*4]
                      mov ScalingHigh,eax

 ;******************* Prepare Registers **********************

                     mov eax,VolHalveOffset
                     movzx ebx,VolumeLeft+[ebp*2]
                     shl ebx,8
                     add ebx,eax                  ; VolumeLeft fertig in ebx
                     movzx ecx,VolumeRight+[ebp*2]
                     shl ecx,8
                     add ecx,eax                  ; VolumeRight fertig in ecx

                     mov esi,SamplePosition+[ebp*4]
                     mov edx,SamplePosScale+[ebp*4] ;momentale Pos als Fixkomma

                     mov dx,CountTick+[ebp*2] ; Ticker setzen

                     ret

PrepareMixer         endp

Interpolation       MACRO

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


                    ENDM

