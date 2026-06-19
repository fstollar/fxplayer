;*** Decode Effects in a S3M and set Flags-, Variables ***

oldEffects    proc near

           ;***** Old Vibrato checks ... *****

            cmp ChannelLastEffect+[ebp],8  ; was last command an Vibrato ?
            jz @@OldVibrato                    ; if not...
            cmp ChannelLastEffect+[ebp],21 ; was last effect an FineVibrato ?
            jz @@OldVibrato
            cmp ChannelLastEffect+[ebp],10 ;
            jz @@OldVibrato

            mov eax,ChannelPeriode+[ebp*4]
            mov VibratoPeriodeStart+[ebp*4],eax
            jmp @@noOldVibrato

@@OldVibrato:
            cmp ChannelEffect+[ebp],8      ; is now a vibrato again ?
            jz @@noOldVibrato              ; if yes, no trouble...
            cmp ChannelEffect+[ebp],21     ; if finevibrato
            jz @@noOldVibrato
            cmp ChannelEffect+[ebp],5      ; if Exx  (slide down)
            jz @@noOldVibrato
            cmp ChannelEffect+[ebp],6      ; if Fxx  (slide up)
            jz @@noOldVibrato
            cmp ChannelEffect+[ebp],7      ; if Gxx  (glissando)
            jz @@noOldVibrato
            cmp ChannelEffect+[ebp],11     ; if Kxy  (vibrato+volslide)
            jz @@noOldVibrato
            cmp ChannelEffect+[ebp],12     ; if Lxy  (glissando+volslide)
            jz @@noOldVibrato


            mov eax,VibratoPeriodeStart+[ebp*4]  ; reload orginal Periode
            mov ChannelPeriode+[ebp*4],eax       ; write it back

            call CalcScale                       ; calc scaling

            mov SampleScale+[ebp*4],eax          ; and write it back

@@noOldVibrato:

           ;***** Old Tremor checks *****

            cmp ChannelLastEffect+[ebp],9  ; was there Tremor befor ?
            jnz @@noOldTremor              ; if not, forget it...
            cmp ChannelEffect+[ebp],9      ; and is there now a Tremor
            jz @@noOldTremor               ; if yes then forget it too...

               mov ax,TremorVolume+[ebp*2]
               mov ChannelVolume+[ebp*2],ax

@@noOldTremor:

           ;***** Old Arpeggio checks *****

            cmp ChannelLastEffect+[ebp],10  ; was there Arpeggio befor ?
            jnz @@noOldArpeggio              ; if not, forget it...
            cmp ChannelEffect+[ebp],10      ; and is there now a Arpeggio
            jz @@noOldArpeggio               ; if yes then forget it too...

               mov eax,VibratoPeriodeStart+[ebp*4]  ; reload orginal Periode
               mov ChannelPeriode+[ebp*4],eax       ; write it back

               call CalcScale                       ; calc scaling

               mov SampleScale+[ebp*4],eax          ; and write it back

               mov ArpeggioPosition+[ebp],0

@@noOldArpeggio:


            ret

oldEffects   endp

;**************************************************************
;****** Decode Volume Slide Dxy AND do Fine Volume Slides *****

VolumeSlideValue  db MC DUP (0)

eDxy        proc near

            cmp ah,0
            jz @@noSlide

            mov VolumeSlideValue+[ebp],0

            cmp ah,0FFh            ; Dff = fine slide up by 0Fh
            jnz @@noDff            ; if not...

                mov al,0Fh
                call ChangeVolume  ; save slide value
                ret                ; RETURN
    @@noDff:

            cmp ah,00Fh            ; D0F = fine+normal slide down by 0Fh
            jnz @@noUltraSlideDown

                mov al,ah
                neg al
                mov VolumeSlideValue+[ebp],al
                call ChangeVolume
                ret

@@noUltraSlideDown:
            cmp ah,0F0h            ; DF0 = fine+normal slide up by 0Fh
            jnz @@noUltraSlideUp

                mov al,0Fh
                mov VolumeSlideValue+[ebp],al
                call ChangeVolume
                ret

@@noUltraSlideUp:

            mov al,ah              ; al=ah = xy
            and al,15              ; al has low nibble y
            shr ah,4               ; ah has high one x

            cmp ah,0Fh             ; if x!=0Fh
            jnz @@noFineSlideDown  ; then no fine slide down...

                neg al
                call ChangeVolume  ; al is slide down
                ret
@@noFineSlideDown:
            cmp al,0Fh            ; if y!=0Fh
            jnz @@noFineSlideUp   ; then no fine slide up...

                mov al,ah
                call ChangeVolume ; ah is slide up
                ret
@@noFineSlideUp:

;*** So,all fine slides are done, now let's do the normal ones ***

            cmp al,0                 ; if y = 0
            jz @@noSlideDown         ; then it is a upper slide

                neg al                        ; neg al
                mov VolumeSlideValue+[ebp],al ; and save negative slide
                ret                           ; (slide is always positiv :)
@@noSlideDown:

            mov VolumeSlideValue+[ebp],ah     ; else save pos. slide

@@noSlide:
            ret

eDxy        endp

;************************************************************************
;****** Decode Tone Slide down Exx AND do fine and extra fine slide *****

ToneSlideValue    db MC DUP (0)

eExx        proc near

            cmp ah,0
            jz @@noSlide

            mov ToneSlideValue+[ebp],0
            mov VibratoPosition+[ebp],0

            cmp ah,0E0h
            jb @@noFinerSlide

            push bx
            mov bl,ah
            and ah,15
            movzx eax,ah
            shl eax,4
            cmp bl,0F0h
            jb @@ExtraSlide
               shl eax,2
@@ExtraSlide:
            add ChannelPeriode+[ebp*4],eax

            call CalcScale

            mov SampleScale+[ebp*4],eax
            pop bx
            ret

@@noFinerSlide:

            mov ToneSlideValue+[ebp],ah

@@noSlide:
            ret

eExx        endp

;**********************************************************************
;****** Decode Tone Slide up Fxx AND do fine and extra fine slide *****

eFxx        proc near

            cmp ah,0
            jz @@noSlide

            mov ToneSlideValue+[ebp],0
            mov VibratoPosition+[ebp],0

            cmp ah,0E0h
            jb @@noFinerSlide

            push bx
            mov bl,ah
            and ah,15
            movzx eax,ah
            shl eax,4
            cmp bl,0F0h
            jb @@ExtraSlide
               shl eax,2
@@ExtraSlide:
            sub ChannelPeriode+[ebp*4],eax

            call CalcScale

            mov SampleScale+[ebp*4],eax
            pop bx
            ret

@@noFinerSlide:

            mov ToneSlideValue+[ebp],ah

@@noSlide:
            ret

eFxx        endp

;**********************************************************************
;****** Decode Glissando Slides Gxx *****

 ALIGN 4
GlissandoSlideValue  db MC DUP (0)  ; xx value of the Gxx command
GlissandoPeriode     dd MC DUP (0)  ; periode to slide to

eGxx        proc near

            cmp ChannelPeriode+[ebp*4],0
            jnz @@noZeroPeriode

                mov ChannelEffect+[ebp],0
                mov byte ptr ds:[esi+ebx+3],0
                ret
@@noZeroPeriode:

            mov ah,ds:[esi+ebx+4]
            or ah,ah
            jz @@noGlissando

               mov GlissandoSlideValue+[ebp],ah
 @@noGlissando:

            mov al,ds:[esi+ebx]         ; load Note+Octave
            cmp al,255                  ; is there any value
            jz @@noPortaNote            ; if not no note definition needed...
            cmp al,254
            jz @@noPortaNote

               mov PlayingFlag+[ebp],1         ; set flag to: play
               mov byte ptr ds:[esi+ebx],255   ; kill Note in Pattern
               mov VibratoPosition+[ebp],0

               ;** define Note for glissando

                     push ecx
                     push edx

                     cmp SampleNumber+[ebp],0   ; if no instrument is activ
                     jz @@endProc               ; then it is an error...

                     mov ah,al    ; load ah with al
                     shr ah,4     ; upper nibble of ah contains octave
                     and al,15    ; and lower the note

                     cmp al,11         ; if it is greater then 11
                     jg @@endProc      ; then it is no serious note...

                     mov ChannelNote+[ebp],al   ; Note = low-nibble
                     mov ChannelOctave+[ebp],ah ; Octave = high-nibble

                     and eax,255                ; make al to eax
                     mov ax,PeriodeNote+[eax*2] ; load periode of note
                     mov cl,ChannelOctave+[ebp] ; load octave

                            call CalcPeriode

                            mov GlissandoPeriode+[ebp*4],eax  ; save periode
                                                            ; for sliding

           @@endProc:
                     pop edx
                     pop ecx

@@noPortaNote:

            ret

eGxx        endp

;****************************************************
;****** Decode Vibrato Hxy and fine Vibrato Uxy *****

 ALIGN 4
VibratoSpeed         db MC DUP (0)  ; x value of the Hxy command
VibratoDepth         db MC DUP (0)  ; y value of the Hxy command
VibratoPosition      db MC DUP (0)  ; position of the sinustable
VibratoPeriodeStart  dd MC DUP (0)  ; periode the vibrato starts from

VibratoSinus    dw +000,+013,+025,+037,+049,+060,+071,+081
                dw +091,+099,+106,+113,+118,+122,+126,+127
                dw +128,+127,+126,+122,+118,+113,+106,+099
                dw +091,+081,+071,+060,+049,+037,+025,+013
                dw -000,-013,-025,-037,-049,-060,-071,-081
                dw -091,-099,-106,-113,-118,-122,-126,-127
                dw -128,-127,-126,-122,-118,-113,-106,-099
                dw -091,-081,-071,-060,-049,-037,-025,-013



eHxy        proc near

            mov ah,ds:[esi+ebx+4]
            cmp ah,0        ; if H00 then no new ones
            jz @@noVibrato  ; and go...

            mov al,ah       ; load al with vibrato value
            and al,15       ; in al low nibble
            shr ah,4        ; and ah high one

            mov VibratoDepth+[ebp],al
            mov VibratoSpeed+[ebp],ah

@@noVibrato:

            ret

eHxy        endp

;***********************************************************************
;*************************** Decode Tremor Ixy *************************

 ALIGN 4
TremorOn             db MC DUP (0)  ; x value of the Ixy command
TremorOff            db MC DUP (0)  ; y value of the Ixy command
TremorOnCounter      db MC DUP (0)  ; Counter of x value of the Ixy command
TremorOffCounter     db MC DUP (0)  ; Counter of y value of the Ixy command
TremorVolume         dw MC DUP (0)  ; Volume of the channel

eIxy        proc near

            mov ah,ds:[esi+ebx+4]
            cmp ah,0         ; if I00 then no new ones
            jz @@noNewTremor ; and go...

            mov al,ah       ; load al with value
            and al,15       ; in al low nibble
            shr ah,4        ; and ah high one

            inc al
            inc ah
            mov TremorOff+[ebp],al
            mov TremorOffCounter+[ebp],al
            mov TremorOn+[ebp],ah
            mov TremorOnCounter+[ebp],ah

            mov ax,ChannelVolume+[ebp*2]
            mov TremorVolume+[ebp*2],ax

@@noNewTremor:
            call TremorEffect
@@noTremor:

            ret

eIxy        endp

;*************************************************************************
;*************************** Decode Arpeggio Jxy *************************

 ALIGN 4
ArpeggioNote1         db MC DUP (0)
ArpeggioNote2         db MC DUP (0)
ArpeggioPosition      db MC DUP (0)

eJxy        proc near

            mov al,ah       ; load al with value
            and al,15       ; in al low nibble
            shr ah,4        ; and ah high one

            mov ArpeggioNote2+[ebp],al
            mov ArpeggioNote1+[ebp],ah

            call ArpeggioEffect

@@noArpeggio:

            ret

eJxy        endp

;***********************************
;****** Decode Note Retrig Qxy *****

 ALIGN 4
RetrigCounter        db MC DUP (0)  ; counter for restarting sample
RetrigCounterStarts  db MC DUP (0)  ; beginning value of counter
RetrigVolumeSlide    db MC DUP (0)  ; which volumeslide out of table

RetrigTable          db 0,-1,-2,-4,-8,-16,0,0, 0,+1,+2,+4,+8,+16,0,0

eQxy        proc near

            mov al,ah       ; load al with value
            and al,15       ; in al low nibble
            shr ah,4        ; and ah high one

            mov RetrigCounterStarts+[ebp],al

            cmp ChannelLastEffect+[ebp],17
            jz @@noRetrigStart
               mov RetrigCounter+[ebp],al
@@noRetrigStart:

            mov RetrigVolumeSlide+[ebp],ah

            call RetrigEffect

            ret

eQxy        endp

;*******************************
;****** Decode Tremolo Rxy *****

 ALIGN 4
TremoloVolumeStart  dw MC DUP (0)  ; volume the Tremolo starts from

eRxy        proc near

            mov al,ah       ; load al with value
            and al,15       ; in al low nibble
            shr ah,4        ; and ah high one

            mov VibratoDepth+[ebp],al
            mov VibratoSpeed+[ebp],ah

            ret

eRxy        endp

;****************************************************************************
;********************** Special commands Sxy decoders ***********************

;*** global Sxy

PatternLoopStart    db 0
PatternLoopEnd      db 0
PatternLoopCount    db 0

PatternDelay        db 0
PatternDelayCounter db 0

globalSxy   proc near

            mov al,ah
            and al,15
            shr ah,4


            cmp ah,0     ; (S0x - Set filter (1/0/F = on/off/for))

            cmp ah,1     ; S1x - Set glissando control x=0/1

            cmp ah,2     ; S2x - Set finetune

            cmp ah,3     ; S3x - Set vibrato waveform to x

            cmp ah,4     ; S4x - Set tremolo waveform to x

            cmp ah,8     ; S8x - Set pan position
            jnz @@noPanPosition

                mov ChannelPanning+[ebp],al
                jmp @@endGlobalSxy
@@noPanposition:
            cmp ah,10    ; (SAx - Old stereo control)
            jnz @@noOldStereoControl

                add al,7
                mov ChannelPanning+[ebp],al
                jmp @@endGlobalSxy
@@noOldStereoControl:
            cmp ah,11    ; SBx - Loop (x=count) / Start (x=0)
            jnz @@noLoop

                or al,al
                jnz @@loopStart
                    mov al,RowPosNow
                    mov PatternLoopStart,al
                    jmp @@endGlobalSxy
      @@loopStart:
                inc al
                cmp PatternLoopCount,0
                jnz @@noLoopStart
                    mov PatternLoopCount,al
      @@noLoopStart:
                dec PatternLoopCount
                jz @@endPatternLoop

                  mov PatternJmp,1
                  mov al,PatternLoopStart
                  mov RowPosJmp,al
                  jmp @@endGlobalSxy
 @@endPatternLoop:
                  mov al,RowPosNow
                  inc al
                  mov PatternLoopStart,al
                jmp @@endGlobalSxy
@@noLoop:
                         ; SCx & SDx are no global ones

            cmp ah,14    ; SEx - Pattern Delay for x notes
            jnz @@noDelay

                or al,al
                jz @@endGlobalSxy
                mov ah,PatternDelay
                or ah,ah
                jnz @@noNewDelay
                    mov PatternDelay,1
                    mov NoteCheck,0
                    mov PatternDelayCounter,al
                    jmp @@endGlobalSxy
   @@noNewDelay:
                dec PatternDelayCounter
                jnz @@endGlobalSxy
                    mov PatternDelay,0
                    mov NoteCheck,1
                jmp @@endGlobalSxy
@@noDelay:
            cmp ah,15    ; (SFx - Funk Repeat, x=speed)

@@endGlobalSxy:

            ret

globalSxy   endp

;*** normal Sxy

 ALIGN 4
DelayOldNote          db MC DUP (0)
DelayOldSample        db MC DUP (0)
DelayOldVolume        db MC DUP (0)

eSxy        proc near

            mov al,ah
            and al,15
            shr ah,4

            cmp ah,12         ; SCx - Note cut in x frames
            jnz @@noNoteCut

                cmp al,SpeedNow
                jb @@CutOK
                   xor al,al
      @@CutOK:
                mov RetrigCounter+[ebp],al
                jmp @@noSpecial
@@noNoteCut:
             cmp ah,13        ; SDx - Note Delay for x frames
             jnz @@noNoteDelay

                 cmp al,SpeedNow
                 jge @@noDelay

                 mov RetrigCounter+[ebp],al

                 mov al,ds:[esi+ebx]    ; load Note+Octave
                 mov DelayOldNote+[ebp],al
                 mov byte ptr ds:[esi+ebx],255   ; kill Note in Pattern

                 mov al,ds:[esi+ebx+1]
                 mov DelayOldSample+[ebp],al
                 mov byte ptr ds:[esi+ebx+1],0

                 mov al,ds:[esi+ebx+2]
                 mov DelayOldVolume+[ebp],al
                 mov byte ptr ds:[esi+ebx+2],255

                 jmp @@noSpecial
    @@noDelay:
                 mov RetrigCounter+[ebp],0
                 jmp @@noSpecial
@@noNoteDelay:

@@noSpecial:
            ret

eSxy        endp
