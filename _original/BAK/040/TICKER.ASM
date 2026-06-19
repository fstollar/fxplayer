;******************* Ticker Timer Routinen fr Mixer ******

VolumeLeft           dw 16 DUP (0)
VolumeRight          dw 16 DUP (0)

BufferBytes          dw 16 DUP (0)  ;Wieviele Bytes sind im Buffer
CountTick            dw 16 DUP (0)  ;Anzahl Bytes schon gespielt

TickNow              db 16 DUP (0)  ;Welcher Tick ist gerade
RowPosNow            db 16 DUP (0)  ;Welche Row spielt gerade

PatternOrderPlay    db 0   ;Welches OrderPattern im Moment gespielt wird
PatternPlay         db 0   ;Welches PatternNr im Moment gespielt wird

TickBytes            dw 0  ;Wieviel Bytes sind ein Tick
SpeedNow             db 0   ; Momentaner Speed im durchlauf

PeriodeNote         dw 1712,1616,1524,1440,1356,1280,1208,1140,1076,1016
                    dw 960,907     ; Perioden von C-4 bis H-4

RowFlag             db 0


Ticker_S3M           proc near

                     pushad
                     push ds
                     push es   ; es = 0

                     mov RowFlag,0

                     movzx ebp,ChannelAnzahl
                     dec ebp
                     mov esi,PatternAktOffset

         ;********** Global special commands **********


         ;********** New Row - new Note ***********

                     mov esi,PatternAktOffset  ; load esi wtih pattern row pointer

      @@everyChannel:
                     mov ebx,ebp          ; load ebx with channel
                     shl ebx,2            ; mul 4
                     add ebx,ebp          ; + 1 ==  > ebx = ebp*5

                ;*** Volume check ***

                     mov al,es:[esi+ebx+2]
                     cmp al,255
                     jz @@noNewVolume

                        xor ah,ah
                        mov ChannelVolume+[ebp*2],ax
        @@noNewVolume:
                ;*** Instrument check ***

                     mov al,es:[esi+ebx+1]
                     cmp al,0
                     jz @@noNewInstrument

                        mov SampleNumber+[ebp],al
                        dec al
                        call NewInstrument

   @@noNewInstrument:
                ;*** Note+Octave check ***

                     mov al,es:[esi+ebx]  ; load first byte (Note)
                     cmp al,255           ; is it empty
                     jz @@noNewNote       ; if yes, then forget it...

                        mov PlayingFlag+[ebp],1 ; set Flag to "on"
                        call NewNote            ; call "Note routine"
                                                ; eax = sample scale
                        mov SampleScale+[ebp*4],eax  ; and save Scaling
                        shr eax,1                    ; div 2
                        mov SampleScaleWide+[ebp*4],eax  ; and Scaling-Wide

                        or SampleOffsetLow+[ebp*4],low16bit ; kill offsets
                        mov SampleOffsetHigh+[ebp*4],0      ; low & high

         @@noNewNote:

                  ;***************

                     dec ebp              ; if channel > -1
                     jns @@everyChannel   ; then loop next channel...
            @@end:
                     pop es
                     pop ds
                     popad
                     ret

Ticker_S3M           endp

Ticker_S3M_Tick      proc near

                     pushad
                     push ds
                     push es   ; es = 0

                     movzx ebp,ChannelAktuell

                     mov ax,TickBytes     ; reload Counter
                     mov CountTick+[ebp*2],ax     ; with value (tempo)

                     inc TickNow+[ebp]            ; increase Ticker
                     mov RowFlag,0

                     mov al,SpeedNow
                     cmp TickNow+[ebp],al         ; if Ticker=Speed
                     jnz @@noNewRow               ; if not no new Row...

                         mov TickNow+[ebp],0

                         mov RowFlag,1             ; set new Row flag
                         inc RowPosNow+[ebp]       ; and inc Row position

                         movzx eax,ChannelAnzahl   ; last channel
                         dec eax                   ; by -1
                         cmp ebp,eax               ; only next Row
                         jnz @@noNewExpand         ; if last channel active...

                           call ExpandNextRow       ; >unpack next Row...
          @@noNewExpand:
                         cmp RowPosNow+[ebp],64    ; if Row the last one
                         jnz @@noNewPattern        ; if not no new Pattern...

                             mov RowPosNow+[ebp],0  ; Rowpos = 0
                             movzx eax,ChannelAnzahl
                             dec eax
                             cmp ebp,eax
                             jnz @@noNewPattern

                             call NextPattern

                             mov al,PatternPlay      ; load al with PatternNr
                             call ExpandFirstRow     ; >expand first new Row
            @@noNewPattern:

          @@noNewRow:

                     pop es
                     pop ds
                     popad
                     ret

Ticker_S3M_Tick      endp


NewNote              proc near

                     push ebx
                     push ecx
                     push edx
                     push esi

                     cmp al,254           ; if it is Keyoff = 254
                     jnz @@noKeyOff       ; if not then do more...

                         mov PlayingFlag+[ebp],0  ; stop playing
                         xor eax,eax              ; set Scaling to zero
                         jmp @@endProc            ; and return...
          @@noKeyOff:
                     cmp SampleNumber+[ebp],0   ; if no instrument is activ
                     jz @@error                 ; then it is an error...

                     mov ah,al    ; load ah with al
                     shr ah,4     ; upper nibble of ah contains octave
                     and al,15    ; and lower the note

                     cmp al,11    ; if it is greater then 11
                     jg @@error   ; then it no sirious note...

                     mov ChannelNote+[ebp],al   ; Note = low-nibble
                     mov ChannelOctave+[ebp],ah ; Octave = high-nibble

                     movzx eax,al
                     movzx eax,word ptr PeriodeNote+[eax*2]

                            shl eax,8
                            mov cl,ChannelOctave+[ebp]
                            shr eax,cl
                            mov edx,16*8363
                            mul edx
                            mov ecx,SampleFineTune+[ebp*4]
                            div ecx
                            add eax,8               ;Runden
                            shr eax,4               ;auf 4 Bit berschuá

                            mov ChannelPeriode+[ebp*4],eax

                            call CalcScale

           @@endProc:
                     pop esi
                     pop edx
                     pop ecx
                     pop ebx
                     ret     ; > eax = Sample Scaling


             @@error:
                     mov PlayingFlag+[ebp],0  ; stop playing
                     xor eax,eax              ; no scaling aka eax=0
                     pop esi
                     pop edx
                     pop ecx
                     pop ebx
                     ret                      ; return...

NewNote              endp

NewInstrument        proc near

                     push ebx
                     push ecx
                     push esi
                     push edi

                     movzx eax,al    ; make al to eax
                     shl eax,2       ; mul 4
                     mov edi,eax     ; load edi with eax
                     add eax,SampleHeadPointer   ; make eax to SampleHeadPointer
                     add edi,SampleOffsetPointer ; make edi to SampleOffsetPointer
                     mov eax,es:[eax]      ; and load with 32bit Pointer
                     mov edi,es:[edi]      ; and with edi of coz too

                     cmp byte ptr es:[eax],1  ; is it a sample ?
                     jnz @@error              ; if no then error...

                     mov SampleStartOffset+[ebp*4],edi  ; save 32bit start pointer

                     mov ecx,es:[eax+10h]            ; load sample length
                     mov SampleLength+[ebp*4],ecx    ; and save it
                     add ecx,edi                     ; add sample start offset
                     mov SampleEndOffset+[ebp*4],ecx  ; and save end offset

                     mov ecx,es:[eax+14h]            ; load loop start
                     add ecx,edi                     ; add offset
                     mov SampleLoopStartOffset+[ebp*4],ecx  ; save loop offset
                     mov ecx,es:[eax+18h]            ; load loop end
                     add ecx,edi                     ; ...the same...
                     mov SampleLoopEndOffset+[ebp*4],ecx  ; and save it

                     movzx cx,byte ptr es:[esi+ebx+2]
                     cmp cl,255
                     jnz @@RowVolume

                        mov cl,es:[eax+1ch]
          @@RowVolume:
                     mov ChannelVolume+[ebp*2],cx

                     mov cl,es:[eax+1fh]
                     mov SampleFlag+[ebp],cl

                     mov ecx,es:[eax+20h]
                     mov SampleFineTune+[ebp*4],ecx

           @@endProc:
                     pop edi
                     pop esi
                     pop ecx
                     pop ebx
                     ret

             @@error:
                     mov SampleStartOffset+[ebp*4],0  ; no sample offset
                     mov SampleNumber+[ebp],0          ; and no sample Nr
                     pop edi
                     pop esi
                     pop ecx
                     pop ebx
                     ret                      ; return...

NewInstrument        endp

NextPattern          proc near

                     push eax
                     push esi

       @@nextPattern:
                     inc PatternOrderPlay
       @@readPattern:
                     movzx eax,PatternOrderPlay
                     mov esi,OrderListPointer
                     mov al,es:[esi+eax]
                     cmp al,254
                     jz @@nextPattern
                     cmp al,255
                     jnz @@noOrderEnd
                         mov PatternOrderPlay,0
                         jmp @@readPattern
       @@noOrderEnd:
                     mov PatternPlay,al

                     pop esi
                     pop eax
                     ret

NextPattern          endp
