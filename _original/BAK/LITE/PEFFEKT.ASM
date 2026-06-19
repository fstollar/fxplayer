;**************************************************************************
;********* Effektgeneratoren und Unterroutinen fr dieselbigen ************

SpecialEffect     proc near

                  mov al,ah
                  and al,15
                  shr ah,4

                  cmp ah,12      ; SCx - Note cut in x frames
                  jnz @@noNoteCut

                      mov al,RetrigCounter+[ebp]
                      or al,al
                      jz @@endSpecial

                      dec RetrigCounter+[ebp]
                      jnz @@endSpecial

                      mov PlayingFlag+[ebp],0
                      jmp @@endSpecial
@@noNoteCut:
                  cmp ah,13      ; SDx - Note Delay for x frames
                  jnz @@noNoteDelay

                      mov al,RetrigCounter+[ebp]
                      or al,al
                      jz @@endSpecial

                      dec RetrigCounter+[ebp]
                      jnz @@endSpecial

                ;*** Instrument check ***

                     mov al,DelayOldSample+[ebp]
                     cmp al,0
                     jz @@noNewInstrument

                        mov SampleNumber+[ebp],al
                        dec al
                        call NewInstrument

   @@noNewInstrument:
                ;*** Volume check ***

                      mov al,DelayOldVolume+[ebp]
                      cmp al,255
                      jz @@noNewVolume

                        xor ah,ah
                        mov ChannelVolume+[ebp*2],ax
                        mov TremoloVolumeStart+[ebp*2],ax
        @@noNewVolume:
                ;*** Note+Octave check ***

                     mov al,DelayOldNote+[ebp]  ; load first byte (Note)
                     cmp al,255            ; is it empty
                     jz @@noNewNote        ; if yes, then forget it...

                        mov PlayingFlag+[ebp],1 ; set Flag to "on"
                        call NewNote            ; call "Note routine"
                                                ; eax = sample scale
                        mov SampleScale+[ebp*4],eax  ; and save Scaling

                        or SampleOffsetLow+[ebp*4],low16bit ; kill offsets
                        mov eax,OffsetHigh
                        mov SampleOffsetHigh+[ebp*4],eax    ; low & high

                        mov VibratoPosition+[ebp],0

         @@noNewNote:

                      jmp @@endSpecial
@@noNoteDelay:
@@endSpecial:
                  ret

SpecialEffect     endp

TremoloEffect     proc near

                  push ebx
                  push edx

                  xor eax,eax
                  cmp VibratoDepth+[ebp],0
                  ;jz @@noDepth

                  movzx bx,VibratoPosition+[ebp]
                  add bx,bx

                  mov ax,VibratoSinus+[bx]
                  movzx bx,VibratoDepth+[ebp]

                  imul bx
                  sar ax,4+2

  @@noDepth:
                  mov bx,TremoloVolumeStart+[ebp*2]
                  mov ChannelVolume+[ebp*2],bx

                  call ChangeVolume

                  mov al,VibratoSpeed+[ebp]
                  add al,VibratoPosition+[ebp]
                  and al,63
                  mov VibratoPosition+[ebp],al

                  pop edx
                  pop ebx
@@noTremolo:
                  ret

TremoloEffect     endp

RetrigEffect      proc near

                  cmp RetrigCounterStarts+[ebp],0
                  jz @@noRetrig

                  cmp RetrigCounter+[ebp],0
                  jnz @@noRetrig


                      or SampleOffsetLow+[ebp*4],low16bit ; kill offsets
                      xor eax,eax
                      mov SampleOffsetHigh+[ebp*4],eax    ; low & high

                      mov al,RetrigCounterStarts+[ebp]
                      mov RetrigCounter+[ebp],al

                      mov al,RetrigVolumeSlide+[ebp]
                      push ax

                      and eax,255
                      mov al,RetrigTable+[eax]
                      call ChangeVolume

                      pop ax

                      cmp al,06
                      jnz @@no23retrig

                      mov ax,ChannelVolume+[ebp*2]
                      push bx
                      mov bl,170
                      mul bl
                      pop bx
                      shr ax,8
                      mov ChannelVolume+[ebp*2],ax
                      jmp @@noRetrig

@@no23retrig:
                      cmp al,07
                      jnz @@no12retrig

                      mov ax,ChannelVolume+[ebp*2]
                      shr ax,1
                      neg al
                      call ChangeVolume
                      jmp @@noRetrig

@@no12retrig:
                      cmp al,0Eh
                      jnz @@no32retrig

                      mov ax,ChannelVolume+[ebp*2]
                      shr ax,1
                      call ChangeVolume
                      jmp @@noRetrig

@@no32retrig:
                      cmp al,0Fh
                      jnz @@no2retrig

                      mov ax,ChannelVolume+[ebp*2]
                      call ChangeVolume
                      jmp @@noRetrig
@@no2retrig:

@@noRetrig:
                  dec RetrigCounter+[ebp]

                  ret

RetrigEffect      endp

ArpeggioEffect    proc near

                  mov al,ArpeggioNote1+[ebp]
                  or al, ArpeggioNote2+[ebp]
                  jz @@noArpeggio

                  push ebx
                  push ecx
                  push edx

                  mov eax,VibratoPeriodeStart+[ebp*4]

                  cmp ArpeggioPosition+[ebp],2
                  jz @@ClearArpeggio

                  mov bl,ArpeggioNote1+[ebp]
                  cmp ArpeggioPosition+[ebp],0
                  jz @@Arpeggio0Position

                     mov bl,ArpeggioNote2+[ebp]
 @@Arpeggio0Position:

                  add bl,ChannelNote+[ebp]
                  mov cl,ChannelOctave+[ebp]

                  cmp bl,11                    ; if note is higher as 11
                  jng @@noOctaveOverrun        ; if not...
                      sub bl,12                  ; then it overruns an octave
                      inc cl                     ; so calc note of next octave
@@noOctaveOverrun:
                  mov al,bl                     ; load into al=new note

                  and eax,255                ; make al to eax
                  mov ax,PeriodeNote+[eax*2] ; load periode of note

                  call CalcPeriode

 @@ClearArpeggio:
                  mov ChannelPeriode+[ebp*4],eax

                  call CalcScale
                  mov SampleScale+[ebp*4],eax

                  mov al,ArpeggioPosition+[ebp]
                  inc al
                  cmp al,3
                  jnz @@noOver
                      mov al,0
        @@noOver:
                  mov ArpeggioPosition+[ebp],al

                  pop edx
                  pop ecx
                  pop ebx
@@noArpeggio:
                  ret

ArpeggioEffect     endp


TremorEffect      proc near

                  cmp TremorOnCounter+[ebp],0
                  jz @@TremorOff

                  mov ax,TremorVolume+[ebp*2]
                  mov ChannelVolume+[ebp*2],ax

                  dec TremorOnCounter+[ebp]
                  jnz @@TremorEnd

                  mov al,TremorOff+[ebp]
                  mov TremorOffCounter+[ebp],al
                  jmp @@TremorEnd

@@TremorOff:
                  xor ax,ax
                  mov ChannelVolume+[ebp*2],ax

                  dec TremorOffCounter+[ebp]
                  jnz @@TremorEnd

                  mov al,TremorOn+[ebp]
                  mov TremorOnCounter+[ebp],al
 @@TremorEnd:
                  ret

TremorEffect      endp

VibratoEffect     proc near

                  push ebx
                  push edx

                  xor eax,eax
                  cmp VibratoDepth+[ebp],0
                  jz @@noDepth

                  movzx bx,VibratoPosition+[ebp]
                  add bx,bx

                  mov ax,VibratoSinus+[bx]
                  movzx bx,VibratoDepth+[ebp]

                  imul bx
                  movsx eax,ax

                  cmp ChannelEffect+[ebp],21
                  jnz @@noFineVibrato
                      sar eax,2
@@noFineVibrato:
  @@noDepth:
                  add eax,VibratoPeriodeStart+[ebp*4]
                  mov ChannelPeriode+[ebp*4],eax

                  call CalcScale
                  mov SampleScale+[ebp*4],eax

                  mov al,VibratoSpeed+[ebp]
                  add al,VibratoPosition+[ebp]
                  and al,63
                  mov VibratoPosition+[ebp],al

                  pop edx
                  pop ebx
@@noVibrato:
                  ret

VibratoEffect     endp

GlissandoEffect   proc near

                  mov eax,ChannelPeriode+[ebp*4]
                  cmp eax,GlissandoPeriode+[ebp*4]
                  je @@endGlissando
                  jb @@positivPorta

                  movzx eax,GlissandoSlideValue+[ebp]
                  shl eax,4+2
                  sub ChannelPeriode+[ebp*4],eax

                  mov eax,GlissandoPeriode+[ebp*4]
                  cmp ChannelPeriode+[ebp*4],eax
                  jg @@PortaIT
                      mov ChannelPeriode+[ebp*4],eax
                      jmp @@PortaIT
  @@positivPorta:
                  movzx eax,GlissandoSlideValue+[ebp]
                  shl eax,4+2
                  add ChannelPeriode+[ebp*4],eax

                  mov eax,GlissandoPeriode+[ebp*4]
                  cmp ChannelPeriode+[ebp*4],eax
                  jng @@PortaIT
                      mov ChannelPeriode+[ebp*4],eax

        @@PortaIT:
                   call CalcScale
                   mov SampleScale+[ebp*4],eax

  @@endGlissando:
                   ret

GlissandoEffect   endp

;***** Change Channel Volume by value al and clip overflows *****

ChangeVolume proc near
                       ; al = value, ebp = channel
             push bx
             mov bx,ChannelVolume+[ebp*2]

             movsx ax,al
             add bx,ax

             cmp bx,0
             jns @@noNegativ

                 mov bx,0
@@noNegativ:
             cmp bx,64
             jna @@noOverflow

                 mov bx,64
@@noOverflow:
             mov ChannelVolume+[ebp*2],bx
             pop bx
             ret

ChangeVolume endp

;****** Routine die TickSpeed aufgrund TickAnzahl neu berechnet ****

CalcSpeed   proc near
                               ;bl=Tempo
            push edx
            push ebx

            movzx ebx,bl
            shl ebx,2           ;*4

            mov eax,CalcMixSpd      ;Freq
            mov edx,10*2        ;*2 fr doppelte Genauigkeit
            mul edx             ;*10 auf dx:ax erweitern

            div ebx             ;/(InitTempo*4)
            inc eax
            shr eax,1           ;Runden

            pop ebx
            pop edx
            ret                ;ax=Bytes pro Tick

CalcSpeed   endp

;****** Routine die SampleScale aufgrund ChannelPeriode neu berechnet ****

CalcScale                   proc near

                            push ecx
                            push edx

                            mov ecx,ChannelPeriode+[ebp*4]
                            cmp ecx,256
                            jb @@NIX

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

                     @@NIX:
                            pop edx
                            pop ecx
                            ret   ; > eax = SampleScale

CalcScale                   endp

;****** Routine die mittels noten periode und octave, periode berechnet *****

CalcPeriode                 proc near

                            ; eax = Periode of note
                            ; cl = octave

                            shl eax,12        ; mul by 4096 (for fixkomma)
                            shr eax,cl        ; and shift periode octave times
                            mov edx,16*8363   ; load C4SPD=8363*16
                            mul edx           ; multiply
                            mov ecx,SampleFineTune+[ebp*4] ; load finetune
                            div ecx                 ; and divide it
                            add eax,128             ; round up to 8th bit
                            shr eax,8               ; and kill lower ones

                            ; eax = new periode

                            ret

CalcPeriode                 endp
