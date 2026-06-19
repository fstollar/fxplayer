;**************************************************************************
;********* Effektgeneratoren und Unterroutinen fÅr dieselbigen ************

;****** Routine die SampleScale aufgrund ChannelPeriode neu berechnet ****

CalcScale   proc near

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
            mov ecx,eax
            shl eax,16
            mov SampleScaleLow+[ebp*4],eax
            shr ecx,16
            mov SampleScaleHigh+[ebp*4],ecx
      @@NIX:
            pop edx
            pop ecx
            ret

CalcScale   endp

;****** Routine die TickSpeed aufgrund TickAnzahl neu berechnet ****

CalcSpeed   proc near
                               ;bl=Tempo
            push edx
            push ebx

            movzx ebx,bl
            shl ebx,2           ;*4

            mov eax,CalcMixSpd      ;Freq
            mov edx,10*2        ;*2 fÅr doppelte Genauigkeit
            mul edx             ;*10 auf dx:ax erweitern

            div ebx             ;/(InitTempo*4)
            inc eax
            shr eax,1           ;Runden

            pop ebx
            pop edx
            ret                ;ax=Bytes pro Tick

CalcSpeed   endp

;***** Routine die Volume Slide entdeckt und aufschlÅsselt *****

DetectVolumeSlide proc near

                  mov ah,ChannelEffektInfo+bp
                  cmp ah,0fh
                  jz @@noVolumeSlide
                  cmp ah,0f0h
                  jz @@noVolumeSlide
                  cmp ah,0ffh
                  jnz @@noSSlid
                      mov ah,0fh
                      jmp @@noVolumeSlide
            @@noSSlid:
                   mov al,ah
                   and al,15         ;DxF = FineVolumeSlide up
                   cmp al,15
                   jnz @@noFineVolumeup
                       shr ah,4
                       add ChannelVolume+bp,ah
                       cmp ChannelVolume+bp,64
                       jng @@noOvrVol
                           mov ChannelVolume+bp,64
               @@noOvrVol:
                       mov ChannelEffekt+bp,0FFh
                       jmp @@noVolumeSlide
      @@noFineVolumeup:

                       mov al,ah
                       shr al,4       ;DFy = FineVolumeSlide down
                       cmp al,15
                       jnz @@noVolumeSlide
                           and ah,15
                           sub ChannelVolume+bp,ah
                           cmp ChannelVolume+bp,0
                           jns @@noNegVol
                               mov ChannelVolume+bp,0
                  @@noNegVol:
                            mov ChannelEffekt+bp,0FFh
      @@noVolumeSlide:
                        ret

DetectVolumeSlide endp

;***** Routine die VolumeSlide Effekt berechnet *****

CalcVolumeSlide proc near

                mov al,ah
                shr al,4   ;Hi-Nibble
                and ah,15  ;Lo-Nibble
                add ChannelVolume+bp,al
                sub ChannelVolume+bp,ah

                cmp ChannelVolume+bp,64
                jng @@noVolumeOverflow
                    mov ChannelVolume+bp,64
   @@noVolumeOverflow:
                cmp ChannelVolume+bp,0
                jns @@noVolumeUnderflow
                    mov ChannelVolume+bp,0
   @@noVolumeUnderflow:
                ret

CalcVolumeSlide endp

;***** Routine die Glissando pro Tick berechnet *****

CalcGlissando proc near

              movzx eax,ah
              shl eax,4+2
              push ebx
              mov ebx,GlissandoPeriode+[ebp*4]
              cmp ebx,ChannelPeriode+[ebp*4]
              pop ebx
              je @@noGlissandoSlide
              jg @@GlissandoDown
                 sub ChannelPeriode+[ebp*4],eax
                 mov eax,GlissandoPeriode+[ebp*4]
                 cmp eax,ChannelPeriode+[ebp*4]
                 jnge @@noOverGliss
                      mov ChannelPeriode+[ebp*4],eax
                      jmp @@noOverGliss
  @@GlissandoDown:
                  add ChannelPeriode+[ebp*4],eax
                  mov eax,GlissandoPeriode+[ebp*4]
                  cmp eax,ChannelPeriode+[ebp*4]
                  jge @@noOverGliss
                      mov ChannelPeriode+[ebp*4],eax
     @@noOverGliss:
                  call CalcScale
@@noGlissandoSlide:
                  ret

CalcGlissando     endp

;*******************************************
;***** Routinen und Daten fÅr Vibratos *****

VibratoSpeed    db 16 DUP (0)
VibratoDepth    db 16 DUP (0)
VibratoZeiger   db 16 DUP (0)
VibratoFlag     db 16 DUP (0)
VibratoPeriode  dd 16 DUP (0)

VibratoSin db 00,13,25,37,49,60,71,81,91,99,106,113,118,122,126,127
           db 127,127,126,122,118,113,106,99,91,81,71,60,49,37,25,13,00
           db -13,-25,-37,-49,-60,-71,-81,-91,-99,-106,-113,-118,-122
           db -126,-127,-128,-127,-126,-122,-118,-113,-106,-99,-91,-81
           db -71,-60,-49,-37,-25,-13,-00

CalcVibrato    proc near

               push eax
               push edx
               push esi

               movzx si,VibratoZeiger+bp
               mov al,VibratoSin+si
               imul byte ptr VibratoDepth+bp
               movsx eax,ax
               add eax,VibratoPeriode+[ebp*4]
               mov ChannelPeriode+[ebp*4],eax

               mov ax,si
               add al,VibratoSpeed+bp
               cmp al,64
               jb noVibCut
                  sub al,64
      noVibCut:
               mov VibratoZeiger+bp,al

               call CalcScale

               pop esi
               pop edx
               pop eax
               ret

CalcVibrato    endp
