;----------------------------------------------------------------------------
;-------------------------- Interpolation OFF -------------------------------

;****************************************************************************
;******************************* M O N O ************************************

ALIGN 4
;************************** MonoMixer mit loop ******************************

   @@MonoMixerloop:

            ;***** Loop Routinen *****
                  ;Taktstart

LoopEndML0:         cmp esi,high16bit        ; is already loop end ?
                    jnb @@MonoLoop           ; yes, then start loop again...

          ;********* Sample spielen *************
ALIGN 4
      @@MonoPlaying:
                    ;TaktStart

                    mov bl,ds:[esi]      ;Aus FMS auslesen
ScaleLowML0:        add edx,high16bit          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
ScaleHighML0:       adc esi,high16bit
                    add es:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx                     ; decrease counter
                  jnz @@MonoMixerloop        ; not zero then go on...
                  jmp EndInnerBlock          ; else stop it...
ALIGN 4
@@MonoLoop:
                  sub esi,SampleLoopEnding
                  add esi,SampleLoopBegining ; load LoopBegin
                  ;and edx,65535              ; clear scalelow
                  jmp @@MonoPlaying          ; make things better... :)

;************************ MonoMixer ohne loop *******************************

ALIGN 4
   @@MonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

SampleEndME0:     cmp esi,high16bit              ;Ende schon erreicht
                  jnb @@SampleEnd                ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    mov bl,ds:[esi]      ;Aus FMS auslesen
ScaleLowME0:        add edx,high16bit          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
ScaleHighME0:       adc esi,high16bit
                    add es:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen


                    ;TaktEnd

                  dec dx
                  jnz @@MonoMixernoloop
                  jmp EndInnerBlock

;****************************************************************************
;*************************** W I D E  M O N O *******************************

ALIGN 4
;************************** MonoMixer mit loop ******************************

   @@WideMonoMixerloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                    cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                    jnb @@WideMonoLoop            ;Wenn dann loop laden...

          ;********* Sample spielen *************
ALIGN 4
      @@WideMonoPlaying:
                    ;TaktStart

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov bl,ds:[esi]      ;Aus FMS auslesen
                    adc esi,ScalingHigh
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                     cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                     jnb @@WideMonoLoop            ;Wenn dann loop laden...
                    mov cl,ds:[esi]      ;Aus FMS auslesen
                    add es:[di],ax           ;Auf Channel draufaddieren
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ecx+ecx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add es:[di+2],ax           ;Auf Channel draufaddieren
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx
                  jnz @@WideMonoMixerloop
                  jmp EndInnerBlock
ALIGN 4
@@WideMonoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  jmp @@WideMonoPlaying

;************************ MonoMixer ohne loop *******************************

ALIGN 4
   @@WideMonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleEnding        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov bl,ds:[esi]      ;Aus FMS auslesen
                    adc esi,ScalingHigh
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                     cmp esi,SampleEnding        ;Ende schon erreicht
                     jnb @@SampleEnd               ;wenn noch nicht...
                    mov cl,ds:[esi]      ;Aus FMS auslesen
                    add es:[di],ax           ;Auf Channel draufaddieren
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ecx+ecx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add es:[di+2],ax           ;Auf Channel draufaddieren
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx
                  jnz @@WideMonoMixernoloop
                  jmp EndInnerBlock
