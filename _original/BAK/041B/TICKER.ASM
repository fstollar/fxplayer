;******************* Ticker Timer Routinen fr Mixer ******

TickNow              db MC DUP (0)  ;Welcher Tick ist gerade
RowPosNow            db MC DUP (0)  ;Welche Row spielt gerade
RowPosNext           db 1           ; Next row number
PatternEndRow        db 64          ; which row is the last
NewRowPos            db 0           ; in which row will the next pattern start

PatternOrderPlay     db 0   ;Welches OrderPattern im Moment gespielt wird
PatternPlay          db 0   ;Welches PatternNr im Moment gespielt wird

TickBytes            dw 0   ; how many bytes are one tick
SpeedNow             db 0   ; speed in the moment (Nr. frames per ROW)

PeriodeNote          dw 1712,1616,1524,1440,1356,1280,1208,1140,1076,1016
                     dw 960,907     ; periodes from C-4 to H-4

RowFlag             db 0
OffsetHigh          dd 0

PatternJmp          db 0  ; this flags tell it should jmp in the pattern back
RowPosJmp           db 0  ; in this Row it should jump back

NoteCheck           db 0  ; if this is set to 1, no note+volume+instr. is read

INCLUDE decode.asm

;-------------------------------------------------------------
;-- This routine is called only if a new row is encountered --

Ticker_S3M           proc near

                     pushad
                     push ds
                     push es   ; ds = 0

                     mov RowFlag,0
                     mov OffsetHigh,0

                     mov al,PatternDelay
                     mov NoteCheck,al 

         ;---------------------------------------------
         ;********** Global special commands **********

                     movzx ebp,ChannelAnzahl
                     dec ebp
                     mov esi,PatternAktOffset

      @@everyGEffect:
                     mov ebx,ebp          ; load ebx with channel
                     shl ebx,2            ; mul 4
                     add ebx,ebp          ; + 1 ==  > ebx = ebp*5

                 ;*** save old effect ***

                     mov al,ChannelEffect+[ebp]
                     mov ChannelLastEffect+[ebp],al

                 ;*** check if there is a effect ***

                     mov al,ds:[esi+ebx+3]   ; load effect number
                     mov ChannelEffect+[ebp],al
                     cmp al,255              ; is it 255
                     jz @@noGEffect           ; yes, then no effect...

                     mov ah,ds:[esi+ebx+4]  ; load effect value
                     or ah,ah
                     jz @@noNewEffectValue
                        mov ChannelEffectValue+[ebp],ah
@@noNewEffectValue:

                      cmp al,01         ; Axx : set speed to xx
                      jnz @@noSpeed     ; else...

                          mov SpeedNow,ah ; set new speed
                          jmp @@noGEffect

            @@noSpeed:
                      cmp al,02         ; Bxx : jump to order xx
                      jnz @@noOrderjump ; else...

                          dec ah
                          mov PatternOrderPlay,ah
                          mov al,RowPosNow+[ebp]
                          inc al
                          mov PatternEndRow,al
                          mov NewRowPos,0
                          jmp @@noGEffect

            @@noOrderjump:
                      cmp al,03         ; Cxx : break pattern to row xx
                      jnz @@noBreakPattern ; else...

                          mov bl,ah     ; make this SHIT BCD value
                          and bl,15     ; to a decimal one...
                          shr ah,4
                          mov al,10
                          mul ah
                          add al,bl

                          mov NewRowPos,al       ; save new row to start
                          mov al,RowPosNow+[ebp] ; and end this pattern
                          inc al                 ; in this row + 1
                          mov PatternEndRow,al   ; and do it
                          jmp @@noGEffect

            @@noBreakPattern:
                      cmp al,19         ; Sxy : Special command
                      jnz @@noSpecial   ; else...

                          Call globalSxy   ; call global Sxy decoder
                          jmp @@noGEffect
            @@noSpecial:
                      cmp al,20         ; Txx : Tempo=xx (20h..FFh)
                      jnz @@noTempo     ; else...

                          mov bl,ah         ; load bl with tempo
                          call CalcSpeed    ; calculate it
                          mov TickBytes,ax  ; and memory it

                          movzx ebx,ChannelAnzahl  ; now write it to every
                          dec ebx                  ; channel which is
                @@loop1:
                          mov CountTick+[ebx*2],ax ; available
                          dec ebx
                          jns @@loop1
                          jmp @@noGEffect

            @@noTempo:
                      cmp al,22         ; Vxx : Set global volume (0..40h)
                      jnz @@noGlobalVolume ; else...

                          mov GlobalVolume,ah     ; set global volume :)
                          jmp @@noGEffect

            @@noGlobalVolume:

          @@noGEffect:                   ; when no needable effect found
                     dec ebp             ; dec - 1
                     jns @@everyGEffect  ; if not all...

         ;-----------------------------------------
         ;********** New Row - new Note ***********

                     movzx ebp,ChannelAnzahl
                     dec ebp
                     mov esi,PatternAktOffset  ; load esi wtih pattern row pointer

      @@everyChannel:
                     mov ebx,ebp          ; load ebx with channel
                     shl ebx,2            ; mul 4
                     add ebx,ebp          ; + 1 ==  > ebx = ebp*5

                     mov OffsetHigh,0

                ;*** Effekt check (not all) ***

                     call oldEffects       ; > correct old Effects when
                                           ; > they are ending

                     mov al,ChannelEffect+[ebp]  ; load effect number
                     cmp al,255                  ; is it 255
                     jz @@noEffect               ; yes, then no effect...

                     mov ah,ChannelEffectValue+[ebp] ; load effect value

                                          ; Axx, Bxx & Cxx are global already

                                          ; Dxy, Exx, Fxx are lower

                     cmp al,7             ; Gxx = Tone Portamento, speed xx
                     jnz @@noTonePorta

                         call eGxx
                         jmp @@noEffect
        @@noTonePorta:
                                          ; Hxy, Ixy & Jxy are lower

                                          ; Kxy & Lxy are lower

                                          ; Mxy & Nxy are unused

                     cmp al,15            ; Oxx = Sample Offset to xx00h
                     jnz @@noSampleOffset

                         and eax,255*256
                         mov OffsetHigh,eax
                         jmp @@noEffect
        @@noSampleOffset:
                                          ; Pxy is unused


                                          ; Rxy are lower

                     cmp al,19            ; *Sxy = Special effect
                     jnz @@noSpecialC

                         call eSxy
                         jmp @@noEffect
        @@noSpecialC:
                                          ; Txx = Set Tempo


                                          ; Vxx = Set global volume

                                          ; Wxy = unused

                                          ; Xxx = Amiga unused command

                                          ; Yxy & Zxy are unused
         @@noEffect:

                ;*** if set to 1, read no Instrument, notes & volumes ***

                     cmp NoteCheck,0
                     jnz @@noNewNote

                ;*** Instrument check ***

                     mov al,ds:[esi+ebx+1]
                     cmp al,0
                     jz @@noNewInstrument

                        mov SampleNumber+[ebp],al
                        dec al
                        call NewInstrument

   @@noNewInstrument:

                ;*** Volume check ***

                     mov al,ds:[esi+ebx+2]
                     cmp al,255
                     jz @@noNewVolume

                        xor ah,ah
                        mov ChannelVolume+[ebp*2],ax
                        mov TremoloVolumeStart+[ebp*2],ax
        @@noNewVolume:


               ;*** Note+Octave check ***

                     mov al,ds:[esi+ebx]  ; load first byte (Note)
                     cmp al,255           ; is it empty
                     jz @@noNewNote       ; if yes, then forget it...

                        mov PlayingFlag+[ebp],1 ; set Flag to "on"
                        call NewNote            ; call "Note routine"
                                                ; eax = sample scale
                        mov SampleScale+[ebp*4],eax  ; and save Scaling

                        or SampleOffsetLow+[ebp*4],low16bit ; kill offsets
                        mov eax,OffsetHigh
                        mov SampleOffsetHigh+[ebp*4],eax    ; low & high

                        mov VibratoPosition+[ebp],0

         @@noNewNote:

                  ;*** Effect check (the rest) ***

                     mov al,ChannelEffect+[ebp]
                     mov ah,ChannelEffectValue+[ebp]

                     cmp al,255
                     jz @@noEffectRest

                     cmp al,4             ; Dxy = Volume Sliding
                     jnz @@noVolumeSlide

                         call eDxy          ; decode
                         jmp @@noEffectRest
        @@noVolumeSlide:
                     cmp al,5             ; Exx = Tone Slide down
                     jnz @@noSlideDown

                         call eExx
                         jmp @@noEffectRest
        @@noSlideDown:
                     cmp al,6             ; Fxx = Tone Slide up
                     jnz @@noSlideUp

                         call eFxx
                         jmp @@noEffectRest
        @@noSlideUp:
                     cmp al,8             ; Hxy = Vibrato, speed x, depth y
                     jnz @@noVibrato

                         call eHxy
                         jmp @@noEffectRest
        @@noVibrato:
                     cmp al,9             ; Ixy = Tremor with ontime x, offtime y
                     jnz @@noTremor

                         call eIxy
                         jmp @@noEffectRest
        @@noTremor:
                     cmp al,10            ; Jxy = Arpeggio; add halfnote x,y,0
                     jnz @@noArpeggio

                         call eJxy
                         jnz @@noEffectRest
        @@noArpeggio:
                     cmp al,11            ; Kxy = Dual command: H00 & Dxy
                     jnz @@noDualHD

                         call eDxy
                         jmp @@noEffectRest
        @@noDualHD:
                     cmp al,12            ; Lxy = Dual command: G00 & Dxy
                     jnz @@noDualGD

                         call eDxy
                         jmp @@noEffectRest
        @@noDualGD:
                     cmp al,17            ; Qxy = Retrig note
                     jnz @@noRetrig

                         call eQxy
                         jmp @@noEffectRest
        @@noRetrig:
                     cmp al,18            ; Rxy = Tremolo with speed x, depth y
                     jnz @@noTremolo

                         call eRxy
                         jmp @@noEffectRest
        @@noTremolo:
                     cmp al,21            ; Uxy = Fine vibrato
                     jnz @@noFineVibrato

                          call eHxy
                          jmp @@noEffectRest
        @@noFineVibrato:

        @@noEffectRest:
                     dec ebp              ; if channel > -1
                     jns @@everyChannel   ; then loop next channel...

            @@end:
                     pop es
                     pop ds
                     popad
                     ret

Ticker_S3M           endp

;---------------------------------------------------------------------------
;-- This routine is called every frame and decide if it will be a new row --
;-- and, of coz, will also execute all frame-effects as needed            --
;---------------------------------------------------------------------------

Ticker_S3M_Tick      proc near

                     pushad
                     push ds
                     push es   ; ds = 0

                     movzx ebp,ChannelAktuell

                     mov ax,TickBytes     ; reload Counter
                     mov CountTick+[ebp*2],ax     ; with value (tempo)

                     inc TickNow+[ebp]            ; increase Ticker
                     mov RowFlag,0

                     mov al,SpeedNow
                     cmp TickNow+[ebp],al         ; if Ticker=Speed
                     jnz @@noNewRow               ; if not no new Row...

               ;*** new row starts here

                         mov TickNow+[ebp],0

                         mov RowFlag,1             ; set new Row flag
                         ;mov al,RowPosNext         ; load next row pos
                         ;mov RowPosNow+[ebp],al    ; save new Row position

                         cmp PatternDelay,0
                         jnz @@endTick

                         inc RowPosNow+[ebp]

                         movzx eax,ChannelAnzahl   ; last channel
                         dec eax                   ; by -1
                         cmp ebp,eax               ; only next Row
                         jnz @@noLastChannel       ; if last channel active...

               ;*** if all channels have moved

                         cmp PatternJmp,0
                         jz @@noPatternJmp

                            mov PatternJmp,0
                            mov al,PatternPlay     ; load al with PatternNr
                            call ExpandFirstRow   ; >expand first new Row...

                            mov al,RowPosJmp
                            call FloatRow          ; go to Row into pattern
                            mov al,RowPosJmp
                            movzx ebx,ChannelAnzahl  ; now write it to every
                            dec ebx                  ; channel which is
                 @@loop1:
                            mov RowPosNow+[ebx],al ; available
                            dec ebx
                            jns @@loop1
                            jmp @@endTick
          @@noPatternJmp:

                         mov al,PatternEndRow      ; load al with last row
                         cmp RowPosNow+[ebp],al    ; if Row the last one
                         jnz @@noNewPattern        ; if not no new Pattern...

               ;*** if last row of pattern played

                             mov al,NewRowPos

                             movzx ebx,ChannelAnzahl
                             dec ebx
                    @@nextChannel:
                             mov RowPosNow+[ebx],al ; Rowpos = NewRowPos
                             dec ebx
                             jns @@nextChannel

                              call NextPattern      ; >fetch next pattern...

                             mov al,PatternPlay     ; load al with PatternNr
                              call ExpandFirstRow   ; >expand first new Row...

                             mov al,NewRowPos
                             call FloatRow          ; go to Row into pattern

                             mov PatternEndRow,64   ; reset endrow
                             mov NewRowPos,0        ; reset startrow
                             mov RowPosJmp,0
                             jmp @@noLastChannel
            @@noNewPattern:

            ;*** end of new pattern

                           ;inc RowPosNext
                           call ExpandNextRow       ; >unpack next Row...
            @@noLastChannel:
                             jmp @@endTick   ; no tick effekt may apear...

                    ;*** end of new row

          @@noNewRow:

                     ;***** may the effects be with you :) *****

                       call Tick_Effects

           @@endTick:
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
                     jg @@error   ; then it is no serious note...

                     mov ChannelNote+[ebp],al   ; Note = low-nibble
                     mov ChannelOctave+[ebp],ah ; Octave = high-nibble

                     and eax,255                ; make al to eax
                     mov ax,PeriodeNote+[eax*2] ; load periode of note
                     mov cl,ChannelOctave+[ebp] ; load octave

                            call CalcPeriode

                            mov ChannelPeriode+[ebp*4],eax    ; save periode
                            mov GlissandoPeriode+[ebp*4],eax  ; for sliding
                            mov VibratoPeriodeStart+[ebp*4],eax

                            call CalcScale   ; calculate scaling value...
                                             ; ...and return with eax=scale
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

;----------------------------------------------------------------------------
;--- This function loads the data of Sample Nr. al into the Sample variables
;----------------------------------------------------------------------------

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
                     mov eax,ds:[eax]      ; and load with 32bit Pointer
                     mov edi,ds:[edi]      ; and with edi of coz too

                     cmp byte ptr ds:[eax],1  ; is it a sample ?
                     jnz @@error              ; if no then error...

                     mov SampleStartOffset+[ebp*4],edi ; save 32bit start pointer

                     mov ecx,ds:[eax+10h]            ; load sample length
                     mov SampleLength+[ebp*4],ecx    ; and save it
                     add ecx,edi                     ; add sample start offset
                     mov SampleEndOffset+[ebp*4],ecx ; and save end offset

                     mov ecx,ds:[eax+14h]                ; load loop start
                     add ecx,edi                         ; add offset
                     mov SampleLoopStartOffset+[ebp*4],ecx ; save loop offset
                     mov ecx,ds:[eax+18h]                ; load loop end
                     add ecx,edi                         ; ...the same...
                     mov SampleLoopEndOffset+[ebp*4],ecx ; and save it

                     xor cx,cx
                     mov cl,ds:[eax+1ch]         ; else load sample volume

                     mov ChannelVolume+[ebp*2],cx   ; and save it
                     mov TremoloVolumeStart+[ebp*2],cx

                     mov cl,ds:[eax+1fh]            ; load flags of sample
                     mov SampleFlag+[ebp],cl        ; save...

                     mov ecx,ds:[eax+20h]           ; load finetune
                     mov SampleFineTune+[ebp*4],ecx ; save...

           @@endProc:
                     pop edi
                     pop esi
                     pop ecx
                     pop ebx
                     ret

             @@error:
                     mov SampleStartOffset+[ebp*4],0  ; no sample offset
                     mov SampleNumber+[ebp],0         ; and no sample Nr
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
                     mov al,ds:[esi+eax]
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

;----------------------------------------------------------------------------
;--- This routine 'Float' to the row which is given in al of the current  ---
;--- pattern and in which 'ExpandFirstRow' is already called              ---
;----------------------------------------------------------------------------

FloatRow             proc near

                     cmp al,00                ; if row = 0
                     jz @@noRow               ; then there we are already...

                  @@next:
                        push ax               ; save ax
                        call ExpandNextRow    ; >expand next one...
                        pop ax                ; restore ax
                        dec al                ; decrement al
                        jnz @@next            ; are we on wished row...

               @@noRow:
                     ret

FloatRow             endp

;----------------------------------------------------------------------------
;--- This routines is called EVERY Non-Row frame to execute effects
;----------------------------------------------------------------------------

Tick_Effects         proc near

                     mov al,ChannelEffect+[ebp]
                     mov ah,ChannelEffectValue+[ebp]

                     cmp al,255
                     jz @@noEffectRest

                     cmp al,4             ; Dxy = Volume Sliding
                     jnz @@noVolumeSlide

                         mov al,VolumeSlideValue+[ebp]
                         or al,al
                         jz @@noEffectRest
                         call ChangeVolume
                         jmp @@noEffectRest

        @@noVolumeSlide:
                     cmp al,5             ; Exx = Tone Slide down
                     jnz @@noSlideDown

                         mov al,ToneSlideValue+[ebp]
                         or al,al
                         jz @@noEffectRest
                         movzx eax,al
                         shl eax,4+2
                         add ChannelPeriode+[ebp*4],eax
                         call CalcScale
                         mov SampleScale+[ebp*4],eax
                         jmp @@noEffectRest

        @@noSlideDown:
                     cmp al,6             ; Fxx = Tone Slide up
                     jnz @@noSlideUp

                         mov al,ToneSlideValue+[ebp]
                         or al,al
                         jz @@noEffectRest
                         movzx eax,al
                         shl eax,4+2
                         sub ChannelPeriode+[ebp*4],eax
                         call CalcScale
                         mov SampleScale+[ebp*4],eax
                         jmp @@noEffectRest

        @@noSlideUp:
                     cmp al,7             ; Gxx = Tone Portamento, speed xx
                     jnz @@noTonePorta

                         mov eax,GlissandoPeriode+[ebp*4]
                         or eax,eax
                         jz @@noEffectRest
                         mov al,GlissandoSlideValue+[ebp]
                         or al,al
                         jz @@noEffectRest

                         call GlissandoEffect
                         jmp @@noEffectRest

        @@noTonePorta:
                     cmp al,8             ; Hxy = Vibrato, speed x, depth y
                     jnz @@noVibrato

                         call VibratoEffect
                         jmp @@noEffectRest
        @@noVibrato:
                     cmp al,9             ; Ixy = Tremor with ontime x, offtime y
                     jnz @@noTremor

                         call TremorEffect
                         jmp @@noEffectRest
        @@noTremor:
                     cmp al,10            ; Jxy = Arpeggio; add halfnote x,y,0
                     jnz @@noArpeggio

                         call ArpeggioEffect
                         jnz @@noEffectRest
        @@noArpeggio:
                     cmp al,11            ; Kxy = Dual command: H00 & Dxy
                     jnz @@noDualHD

                         call VibratoEffect
                         mov al,VolumeSlideValue+[ebp]
                         or al,al
                         jz @@noEffectRest
                         call ChangeVolume
                         jmp @@noEffectRest
        @@noDualHD:
                     cmp al,12            ; Lxy = Dual command: G00 & Dxy
                     jnz @@noDualGD

                         mov al,VolumeSlideValue+[ebp]
                         or al,al
                         jz @@noEffectRest
                         call ChangeVolume

                         mov eax,GlissandoPeriode+[ebp*4]
                         or eax,eax
                         jz @@noEffectRest
                         mov al,GlissandoSlideValue+[ebp]
                         or al,al
                         jz @@noEffectRest

                         call GlissandoEffect
                         jmp @@noEffectRest
        @@noDualGD:
                                          ; Mxy & Nxy are unused

                                          ; Oxx = Sample Offset to xx00h

                                          ; Pxy is unused

                     cmp al,17            ; Qxy = Retrig note
                     jnz @@noRetrig

                         call RetrigEffect
                         jmp @@noEffectRest
        @@noRetrig:
                     cmp al,18            ; Rxy = Tremolo with speed x, depth y
                     jnz @@noTremolo

                         call TremoloEffect
                         jmp @@noEffectRest
        @@noTremolo:
                     cmp al,19            ; *Sxy = Special effect
                     jnz @@noSpecialC

                         call SpecialEffect
                         jmp @@noEffectRest
        @@noSpecialC:
                                          ; Txx = Set Tempo

                     cmp al,21            ; Uxy = Fine vibrato
                     jnz @@noFineVibrato

                         call VibratoEffect
                         jmp @@noEffectRest
        @@noFineVibrato:
                                          ; Vxx = Set global volume

                                          ; Wxy = unused

                                          ; Xxx = Amiga unused command

                                          ; Yxy & Zxy are unused

        @@noEffectRest:

                     ret

Tick_Effects         endp
