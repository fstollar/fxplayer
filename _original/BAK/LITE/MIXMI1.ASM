
;----------------------------------------------------------------------------
;----------------------- Interpolation 1. grade -----------------------------

ALIGN 4
;************************ MonoMixer mit loop ********************************

   @@IMonoMixerloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                    cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                    jnb @@IMonoLoop            ;Wenn dann loop laden...

          ;********* Sample spielen *************
ALIGN 4
                    ;TaktStart
      @@IMonoPlaying:
                     mov bl,byte ptr ds:[esi]      ;Aus FMS auslesen
                     Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add es:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx
                  jnz @@IMonoMixerloop
                  jmp EndInnerBlock
ALIGN 4
@@IMonoLoop:
                  mov esi,SampleLoopBegining    ;LoopBegin laden
                  and edx,65535              ;laden
                  jmp @@IMonoPlaying

;************************ MonoMixer ohne loop *******************************

ALIGN 4
   @@IMonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleEnding        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                     mov bl,byte ptr ds:[esi]      ;Aus FMS auslesen
                     Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add es:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx
                  jnz @@IMonoMixernoloop
                  jmp EndInnerBlock


