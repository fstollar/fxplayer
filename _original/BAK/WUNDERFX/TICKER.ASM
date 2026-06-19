;******************* Ticker Timer Routinen fÅr Mixer ******
;******* INCLUDE Ticker.asm an der richtigen Stelle *******

                     push ds
                     push es   ; es = 0
                     push fs

                     xor ax,ax
                     mov fs,ax

                     mov cx,TickBytes+[ebp*2] ;CountTick wieder zurÅcksetzen
                     inc TickNow+bp           ;Tick um eins erhîhen

                     mov al,TickNow+bp    ;Hat Tick
                     cmp al,SpeedNow+bp   ;InitSpeed erreicht
                     jnz noRowPos         ;nein.. dann keine neue Row

                        mov TickNow+bp,0   ;TickNow lîschen
                        inc RowPos+bp      ;RowPos um eins erhîhen

                        mov al,RowPos+bp   ;RowPos holen
                        cmp al,PatternRow+bp  ;Ende des Patterns
                        jnz @@noNewPattern   ;nein.. dann Åberspringen

                            mov al,PatternStart+bp
                            mov RowPos+bp,al ;RowPos wieder lîschen
                            mov ax,bp        ;und den Neuen Kanal
                            call CopyChannel ;Åber den alten kopieren
                            mov PatternStart+bp,0
                            mov PatternRow+bp,64
                            mov PatternChannelFlag,0

                            movzx ax,ChannelAnzahl  ;Ist der jetzige
                            dec ax                 ;Kanal
                            cmp ax,bp             ;der letzte
                            jnz @@noNewPattern      ;nein.. dann kein neues
                                                  ;Pattern entpacken
                                mov PatternChannelFlag,1
                                push esi
                       @@skipMark:
                                mov al,PatternOrderNext
                                mov PatternOrderPlay,al
                                movzx esi,al
                                inc esi
                                add esi,OrderListPointer
                                mov al,fs:[esi]
                                inc PatternOrderNext
                                cmp al,254
                                jz @@skipMark
                                cmp al,255
                                jnz @@noPatternStart
                                    mov PatternOrderNext,0
                                    mov esi,OrderListPointer
                                    mov al,fs:[esi]
                    @@noPatternStart:
                                mov ah,PatternNext
                                mov PatternPlay,ah
                                mov PatternNext,al
                                call ExpandPattern
                                pop esi
          @@noNewPattern:
                                       ;Neue Row - neue Note
                        push edi

                        movzx ax,RowPos+bp
                        mov bx,ax
                        shl ax,2
                        add bx,ax
                        shl bx,4      ;RowPos*80

                        mov ax,bp     ;Channel
                        shl ax,2      ;*4
                        add ax,bp     ;+1=*5
                        add bx,ax

         ;**************************************************
         ;***** 1. Channel abfrage auf Global Commands *****
         ;**************************************************

                 mov ax,PatternNexSeg
                 cmp PatternChannelFlag,0
                 jz noNewSeg
                     mov ax,PatternAktSeg
            noNewSeg:
                 mov ds,ax
                 cmp bp,0
                 jnz no1Channel

                     push ebx
                     mov ah,16
              nextKanal:
                     mov al,ds:[bx+3]

           ;***** Global Volume *****

                     cmp al,22
                     jnz noGlobalVolume

                         mov al,ds:[bx+4]
                         mov GlobalVolume,al
                         jmp nextPChannel

           noGlobalVolume:

           ;***** Speed Change *****

                     cmp al,01h
                     jnz noSpeedChange

                SpeedCh: push bx
                         push ax
                         mov al,ds:[bx+4]
                         mov InitSpeed,al
                         mov ah,RowPos+bp
                         mov bx,15
                    repPSpeed:
                         mov PatternSpeedRow+bx,ah
                         mov PatternSpeed+bx,al
                         dec bx
                         jns repPSpeed
                         pop ax
                         pop bx
                         jmp nextPChannel
            noSpeedChange:

            ;***** Tempo Change *****

                     cmp al,14h
                     jnz noTempoChange

                         mov bl,ds:[bx+4]
                         cmp bl,20
                         jb exitPatternE
                         mov InitTempo,bl
                         call CalcSpeed
                         push cx
                         mov cx,ax
                         mov al,RowPos+bp
                         mov bx,15
                    repPTempo:
                         mov PatternTempoRow+bx,al
                         mov PatternTempoBytes+[ebx*2],cx
                         dec bx
                         jns repPTempo
                         pop cx
                         jmp exitPatternE
            noTempoChange:

            ;***** Pattern Jump *****
                     cmp al,02h
                     jnz noPatternJump

                         mov al,ds:[bx+4]
                         mov bx,0
                   rpPJump:
                         mov PatternStart+bx,0
                         mov PatternOrderNext,al
                         mov ah,RowPos+bp
                         inc ah
                         mov PatternRow+bx,ah
                         inc bx
                         cmp bx,16
                         jnz rpPJump
                         movzx edi,al
                         add edi,OrderListPointer
                         mov al,fs:[edi]
                         mov PatternNext,al
                         call ExpandPattern
                         jmp exitPatternE

            noPatternJump:

            ;***** Pattern Break *****
                     cmp al,03h
                     jnz noPatternB

                         mov al,ds:[bx+4]
                         mov bl,al
                         and bl,15
                         shr al,4
                         mov ah,10
                         mul ah
                         add al,bl
                         mov bx,0
                   rpPBreak:
                         mov PatternStart+bx,al
                         mov ah,RowPos+bp
                         inc ah
                         mov PatternRow+bx,ah
                         inc bx
                         cmp bx,16
                         jnz rpPBreak
                         jmp exitPatternE

              noPatternB:
              nextPChannel:
                     add bx,5
                     dec ah
                     jnz nextKanal
          exitPatternE:
                     pop ebx
          no1Channel:

                 ;*****************************
                     mov ds,PatternAktSeg

                 ;*****************************
                 ;***** Volume im Pattern *****

                        mov al,ds:[bx+2]     ;PatternVolume holen
                        cmp al,255           ;wenn 255
                        jz noPatternVolume   ;Volume nicht Ñndern

                            mov ChannelVolume+bp,al
          noPatternVolume:

             ;**********************************
             ;***** Pattern Global Effekts *****

                 cmp PatternTempoBytes+[ebp*2],0
                 jz noPTempoChange
                    mov al,RowPos+bp
                    cmp al,PatternTempoRow+bp
                    jnz noPTempoChange
                        mov cx,PatternTempoBytes+[ebp*2]
                        mov TickBytes+[ebp*2],cx
                        mov PatternTempoBytes+[ebp*2],0
                        mov PatternTempoRow+bp,0
        noPTempoChange:

                 cmp PatternSpeed+bp,0
                 jz noPSpeedChange
                    mov al,RowPos+bp
                    cmp al,PatternSpeedRow+bp
                    jnz noPSpeedChange
                        mov al,PatternSpeed+bp
                        mov SpeedNow+bp,al
                        mov PatternSpeed+bp,0
                        mov PatternSpeedRow+bp,0
        noPSpeedChange:
                 ;*************************
                 ;***** Effekt in Row *****

                         mov al,ds:[bx+3]
                         mov ah,ds:[bx+4]
                         mov ChannelEffekt+bp,al
                         or ah,ah
                         jz noInfoChange
                         mov ChannelEffektInfo+bp,ah
                noInfoChange:
                       ;*** Vibrato ***

                         cmp al,08h
                         jnz noVibrato

                             mov ah,ds:[bx+4]
                             or ah,ah
                             jz oldVibrato
                             mov al,ah
                             and al,15
                             shr ah,4
                             jz noVipSpeed
                                mov VibratoSpeed+bp,ah
                noVipspeed:  mov VibratoDepth+bp,al
                oldVibrato:
                             cmp VibratoFlag+bp,0
                             jnz noRowEffekt
                                 mov eax,ChannelPeriode+[ebp*4]
                                 mov VibratoPeriode+[ebp*4],eax
                                 mov VibratoFlag+bp,1
                                 jmp noRowEffekt
            noVibrato:
                        cmp VibratoFlag+bp,0
                        jz noClearVibrato1
                        cmp al,0Bh          ;Dual: Vibr + VolSlide
                        jz noClearVibrato1
                           push ax
                           mov eax,VibratoPeriode+[ebp*4]
                           mov ChannelPeriode+[ebp*4],eax
                           mov VibratoFlag+bp,0
                           call CalcScale
                           pop ax
               noClearVibrato1:

                       ;*** Volume Slide ***

                         cmp al,04h          ;Dxy = VolumeSlide
                         jnz noVolumeSlide

                             call DetectVolumeSlide
                             jmp noRowEffekt
            noVolumeSlide:
                         ;*** Tone Slide down ***

                         cmp al,05h      ;Exx = Tone Slide down
                         jnz noToneSlideDown

                             mov ah,ChannelEffektInfo+bp
                             cmp ah,0dfh
                             jna noFineSlideDown
                                 mov al,ah
                                 and ah,15
                                 cmp al,0efh
                                 jna EFineSlideDown
                                     shl ah,2
                                 EFineSlideDown:
                                 movzx eax,ah
                                 shl eax,4
                                 add ChannelPeriode+[ebp*4],eax

                                 call CalcScale
                                 mov ChannelEffekt+bp,0FFh

            noFineSlideDown: jmp noRowEffekt
            noToneSlideDown:

                         ;*** Tone Slide up ***

                         cmp al,06h      ;Exx = Tone Slide up
                         jnz noToneSlideUp

                             mov ah,ChannelEffektInfo+bp
                             cmp ah,0dfh
                             jna noFineSlideUp
                                 mov al,ah
                                 and ah,15
                                 cmp al,0efh
                                 jna EFineSlideUp
                                    shl ah,2
                                 EFineSlideUp:
                                 movzx eax,ah
                                 shl eax,4
                                 sub ChannelPeriode+[ebp*4],eax

                                 call CalcScale
                                 mov ChannelEffekt+bp,0FFh

            noFineSlideUp:   jmp noRowEffekt
            noToneSlideUp:


            noRowEffekt:
                  ;*************************
                  ;***** Instrument da *****

                        mov al,byte ptr ds:[bx+1]  ;PatternInstrument holen
                        or al,al          ;Instrument=0
                        jz noNewInstrument ;Wenn ja, altes Instr benutzen

                            dec al                    ;Instrument-1
                            mov ChannelSampleNr+bp,al ;Instrument speichern

                         ;* SampleDaten kopieren *
                         push ecx
                         movzx edi,al                  ;von Sample holen
                         shl edi,2
                         mov ecx,edi
                         add edi,SampleHeadPointer
                         add ecx,SampleOffsetPointer
                         mov edi,fs:[edi]             ;und Segment in es
                         mov ecx,fs:[ecx]

                         mov al,ds:[bx+2]          ;Volume holen
                         cmp al,255                ;wenn nicht 255
                         jnz noRowPosVolume        ;dann Volume speichern

                             mov al,fs:[edi+01ch]       ;SampleVolume
             noRowPosVolume:
                         mov ChannelVolume+bp,al   ;Volume speichern

                         ;* Flags zurÅcksetzen *
                         cmp byte ptr fs:[edi],1
                         jnz noPatternNote
                         mov eax,fs:[edi+10h]         ;SamplelÑnge holen
                         add eax,ecx
                         mov SampleLength+[ebp*4],eax
                         mov eax,fs:[edi+14h]         ;SampleLoopBegin holen
                         add eax,ecx
                         mov SampleLoopBegin+[ebp*4],eax
                         mov eax,fs:[edi+18h]         ;SampleLoopEnd holen
                         add eax,ecx
                         mov SampleLoopEnd+[ebp*4],eax
                         mov al,fs:[edi+1ch]         ;Sample Volume holen
                         mov SampleVolume+bp,al
                         mov al,fs:[edi+1fh]         ;SampleFlag holen
                         mov SampleFlag+bp,al
                         mov ax,fs:[edi+20h]         ;Sample Finetune holen
                         mov SampleFinetune+[ebp*2],ax
                         pop ecx

                       ;*** FMS Mapping ***

                         cmp ChannelEffekt+bp,13h    ;No EMS Mapping
                         jnz noSpCut
                         mov al,ChannelEffektInfo+bp
                         shr al,4
                         cmp al,0Dh               ;Bei Note Delay
                         jz noNewInstrument
                   noSpCut:
                            push eax
                            movzx eax,ChannelSampleNr+bp
                            shl eax,2
                            add eax,SampleOffsetPointer
                            mov eax,fs:[eax]
                            mov SampleOffset+[ebp*4],eax
                            mov esi,eax
                            pop eax
             noNewInstrument:

                  ;*** Note im Pattern ***

                         mov al,ds:[bx]       ;Note im Pattern ?
                         cmp al,255
                         jz noPatternNote     ;wenn nicht dann weiter
                         cmp al,254           ;KeyOff ?
                         jnz noKeyOff         ;nein.. dann Åberspringen

                             mov PlayingFlag+bp,0  ;Sample nicht mehr spielen
                             and edx,65535           ;SamplePosFix 0
                             push eax
                             movzx eax,ChannelSampleNr+bp
                             shl eax,2
                             add eax,SampleOffsetPointer
                             mov esi,fs:[eax]
                             pop eax
                             ;SamplePosition
                             jmp noPatternNote   ;Keine Note mehr

                      noKeyOff:
                            mov ah,al
                            shr ah,4                ;Oktave = Hi-Nibble
                            mov ChannelOctave+bp,ah

                            and al,15
                            mov ChannelNote+bp,al   ;Note = Lo-Nibble

                            ;* Note Periode berechnen *
                            movzx edi,ChannelNote+bp      ;Note von Channel
                            mov ax,PeriodeNote+[edi*2]    ;Periode lesen
                            mov SamplePeriode+[ebp*2],ax  ;und sichern

                            ;*** Special Command bei Note im Pattern ***

                               mov al,ChannelEffekt+bp
                               cmp al,0ffh
                               jz noSpecialEffekt

                               cmp al,0fh          ;Oxx = Sample Offset
                               jnz noSampleOffset

                                    mov ah,ChannelEffektInfo+bp
                                    xor al,al
                                    movzx esi,ax
                                     push eax
                                     movzx eax,ChannelSampleNr+bp
                                     shl eax,2
                                     add eax,SampleOffsetPointer
                                     add esi,fs:[eax]
                                     pop eax
                                    and edx,65535
                                    mov PlayingFlag+bp,1
                                    jmp noFlagReset
                    noSampleOffset:

                             ;*** Glissando Slide ***

                               cmp al,07h         ;Gxy = Glissando Slide
                               jnz noGlissando

                                   mov ah,ChannelEffektInfo+bp
                                   cmp ChannelPeriode+[ebp*4],0
                                   jnz noFirstGliss
                                       mov ChannelEffekt+bp,0ffh
                                       jmp noSpecialEffekt
                         noFirstGliss:
                                   mov GlissandoSpeed+bp,ah
                                   mov PlayingFlag+bp,1
                                   jmp noFlagReset
                     noGlissando:

                            ;***** Special Effekts *****

                             cmp al,13h         ;Sxy = Special Effekt
                             jnz noSpecialEffekt

                             mov ah,ChannelEffektInfo+bp
                             mov al,ah
                             shr al,4
                             ;*** Note Delay ***

                             cmp al,0Dh        ;SDx = Note Delay x Frames
                             jnz noNoteDelay

                                 jmp noFlagReset

                      noNoteDelay:
                             ;*** Note Cut ***

                             cmp al,0Ch        ;SCx = Note Cut after x Frames
                             jnz noNoteCut

                                 jmp NoSpecialEffekt
                      noNoteCut:
                      noSpecialEffekt:
                            ;*** RÅcksetzende Flags ***

                            mov PlayingFlag+bp,1         ;Channel spielt
                            and edx,65535                    ;SamplePosFix
                                     push eax
                                     movzx eax,ChannelSampleNr+bp
                                     shl eax,2
                                     add eax,SampleOffsetPointer
                                     mov esi,fs:[eax]
                                     pop eax

                   noFlagReset:
                       ;********** Note Periode berechnen **********
                       ;********************************************

                            push edx         ;Rette SamplePosFix
                            push ecx
                            movzx eax,SamplePeriode+[ebp*2]
                            shl eax,8
                            mov cl,ChannelOctave+bp
                            shr eax,cl
                            mov edx,16*8363
                            mul edx
                            movzx ecx,SampleFinetune+[ebp*2]
                            div ecx
                            add eax,8               ;Runden
                            shr eax,4               ;auf 4 Bit Åberschu·
                            mov ecx,eax             ;kÅrzen
                            mov GlissandoPeriode+[ebp*4],ecx
                            mov VibratoPeriode+[ebp*4],eax

                            cmp ChannelEffekt+bp,07h  ;wenn Glissando
                            jz EndNote                ;aktiviert

                            mov ChannelPeriode+[ebp*4],ecx
                            mov eax,14317456*256
                            xor edx,edx
                            div ecx
                            mov SampleFreq+[ebp*4],eax
                            xor edx,edx
                            shld edx,eax,16
                            shl eax,16
                            mov ecx,CalcMixSpd
                            shl ecx,4
                            div ecx
                            mov ecx,eax
                            shl eax,16
                            mov SampleScaleLow+[ebp*4],eax
                            shr ecx,16
                            mov SampleScaleHigh+[ebp*4],ecx

                     EndNote:
                            pop ecx
                            pop edx

           noPatternNote:
                        pop edi
                        jmp nothingEffekt
           noRowPos:

                ;**********************************
                ;***** Effekts TickerabhÑngig *****
                ;**********************************

                     mov al,ChannelEffekt+bp
                     cmp al,0FFh
                     jz noEffekt

                     ;*** Volume Slide ***
                     mov ah,ChannelEffektInfo+bp
                     cmp al,04h              ;Dxy = Volume Slide
                     jnz noAVolumeSlide

                         call CalcVolumeSlide
                         jmp noEffekt
           noAVolumeSlide:

                     ;*** Tone slide down ***

                     cmp al,05h              ;Exx = Tone Slide down
                     jnz noAToneSlideDown

                         movzx eax,ah
                         shl eax,4+2
                         add ChannelPeriode+[ebp*4],eax

                         call CalcScale
                         jmp noEffekt
           noAToneSlideDown:

                     ;*** Tone slide up ***

                     cmp al,06h                ;Fxx = Tone Slide up
                     jnz noAToneSlideUp

                         movzx eax,ah
                         shl eax,4+2
                         sub ChannelPeriode+[ebp*4],eax

                         call CalcScale
                         jmp noEffekt
           noAToneSlideUp:

                     ;*** Glissando slide ***

                     cmp al,07h             ;Gxx = Tone Glissando Slide
                     jnz noGlissandoSlide

                         call CalcGlissando
                         jmp noEffekt
           noGlissandoSlide:

                     ;*** Vibrato ***

                     cmp al,08h
                     jnz noVibrat

                         call CalcVibrato
                         jmp noEffekt
              noVibrat:

                     ;*** DualCommand: Vibrato+VolumeSlide ***

                     cmp al,0Bh           ;Kxy = Vibrato + VolSlide
                     jnz noVibratoVolume

                         call CalcVolumeSlide
                         call CalcVibrato
                         jmp noEffekt
       noVibratoVolume:

                     ;*** DualCommand: Glissando+VolumeSlide ***

                     cmp al,0Ch            ;Lxy = Glissando + VolSlide
                     jnz noGlissVolume

                         call CalcVolumeSlide
                         mov ah,GlissandoSpeed+bp
                         call CalcGlissando
                         jmp noEffekt
       noGlissVolume:

                   ;****** Special Effekts ******

                     cmp al,13h     ; Sxy = Special Effekts
                     jnz noSpecial

                     mov al,ah
                     shr al,4

                     ;*** Note Delay ***
                     cmp al,0Dh     ; SDx = Note Delay x Frames
                     jnz noNDelay

                          and ah,15
                          dec ah
                          jnz noRNDelay

                              mov PlayingFlag+bp,1
                              and edx,65535
                               push eax
                               movzx eax,ChannelSampleNr+bp
                               shl eax,2
                               add eax,SampleOffsetPointer
                               mov esi,fs:[eax]
                               pop eax
                              mov ChannelEffekt+bp,0FFh
                              push eax
                              push fs
                              xor ax,ax
                              mov fs,ax
                              movzx eax,ChannelSampleNr+bp
                              shl eax,2
                              add eax,SampleOffsetPointer
                              mov eax,fs:[eax]
                              mov SampleOffset+[ebp*4],eax
                              pop fs
                              pop eax
                              jmp noEffekt

                      noRNDelay:
                          add ah,0D0h
                          mov ChannelEffektInfo+bp,ah
                          jmp noEffekt
           noNDelay:
                     ;*** Note Cut ***
                     cmp al,0Ch     ; SCx = Note Cut after x Frames
                     jnz noNCut

                          and ah,15
                          dec ah
                          jnz noRNCut

                              mov PlayingFlag+bp,0
                              mov ChannelEffekt+bp,0FFh
                              jmp noEffekt

                       noRNCut:
                          add ah,0C0h
                          mov ChannelEffektInfo+bp,ah
                          jmp noEffekt
           noNCut:
           noSpecial:
                     ;******************************
           noEffekt:

      nothingEffekt:
                     mov al,ChannelVolume+bp
                     mul GlobalVolume
                     shr ax,6
                     mov ChannelGVolume+bp,al
                     mov bh,al
                     mov dh,al

                     cmp SBstereoFlag,1
                     jnz @@nopanning

                     movzx bx,ChannelPanning+bp
                     mov bx,word ptr panning+bx
                     mul bh
                     shr ax,6
                     mov bh,al

                     mov al,dh
                     mul bl
                     shr ax,6
                     mov dh,0

                     jmp @@nopanning

         panning     db 64,00, 64,09, 64,18, 64,27, 64,36, 64,45, 64,52, 64,64
                     db 64,64, 52,64, 45,64, 36,64, 27,64, 18,64, 09,64, 00,64
        @@nopanning:

                     add bh,MBlockLength/2/256
                     add dh,MBlockLength/2/256

                     pop fs
                     pop es
                     pop ds
