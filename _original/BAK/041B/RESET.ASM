;******************************
;******** Reset Player ********
;******************************

_ResetPlay  proc far

            push es
            push ebp

            ;******************************
            ;*** Alte Variablen l”schen ***


            mov ebp,0
            ;mov es,bp
            ;mov dword ptr es:[12],0f000fff0h
            ;mov dword ptr es:[04],0f000fff0h   ; little anti-debug

            xor eax,eax
@@KillVariable:
            mov PlayingFlag+[ebp],al
            mov ChannelFlag+[ebp],al
            mov ChannelPanning+[ebp],al

            mov SampleNumber+[ebp],al

            mov ChannelOctave+[ebp],al
            mov ChannelNote+[ebp],al
            mov ChannelPeriode+[ebp*4],eax
            mov ChannelVolume+[ebp*2],ax
            mov ChannelEffect+[ebp],al
            mov ChannelLastEffect+[ebp],al
            mov ChannelEffectValue+[ebp],al
            mov ChannelPanning+[ebp],7

           ; Ticker

            mov TickNow+[ebp],al
            mov RowPosNow+[ebp],al

            mov RowPosNext,1
            mov PatternEndRow,64
            mov RowFlag,al
            mov OffsetHigh,eax
            mov PatternJmp,0
            mov RowPosJmp,0
            mov NoteCheck,0

         ;-)

            mov VolumeLeft+[ebp*2],ax
            mov VolumeRight+[ebp*2],ax

            mov VolumeSlideValue+[ebp],al
            mov ToneSlideValue+[ebp],al
            mov GlissandoSlideValue+[ebp],al
            mov GlissandoPeriode+[ebp*4],eax
            mov VibratoSpeed+[ebp],al           ; x value of the Hxy command
            mov VibratoDepth+[ebp],al           ; y value of the Hxy command
            mov VibratoPosition+[ebp],al        ; position of the sinustable
            mov VibratoPeriodeStart+[ebp*4],eax ; periode the vibrato starts from
            mov TremorOn+[ebp],al               ; x value of the Ixy command
            mov TremorOff+[ebp],al              ; y value of the Ixy command
            mov TremorOnCounter+[ebp],al        ; Counter of x value of the Ixy command
            mov TremorOffCounter+[ebp],al       ; Counter of y value of the Ixy command
            mov TremorVolume+[ebp*2],ax         ; Volume of the channel
            mov ArpeggioNote1+[ebp],al
            mov ArpeggioNote2+[ebp],al
            mov ArpeggioPosition+[ebp],al
            mov RetrigCounter+[ebp],al          ; counter for restarting sample
            mov RetrigCounterStarts+[ebp],al    ; beginning value of counter
            mov RetrigVolumeSlide+[ebp],al      ; which volumeslide out of table
            mov TremoloVolumeStart+[ebp*2],ax   ; volume the Tremolo starts from
            mov DelayOldNote+[ebp],al
            mov DelayOldSample+[ebp],al
            mov DelayOldVolume+[ebp],al

            mov PatternDelay,0

            inc ebp
            cmp ebp,MC
            jb @@KillVariable

            pop ebp
            pop es
            retf

_ResetPlay  endp

