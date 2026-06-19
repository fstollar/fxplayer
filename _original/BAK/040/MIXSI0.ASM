
;----------------------------------------------------------------------------
;------------------------- Interpolation OFF --------------------------------

;****************************************************************************
;**************************** S T E R E O  **********************************

ALIGN 4
;************************ StereoMixer mit loop ******************************

   @@StereoMixerloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                    cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                    jnb @@StereoLoop           ;Wenn nicht dann nicht

          ;********* Sample spielen *************
ALIGN 4
      @@StereoPlaying:
                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                     add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov cl,bl
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                     adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                    mov bp,es:[ecx+ecx]
                    add ds:[di+2],bp
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx
                  jnz @@StereoMixerloop
                  jmp EndInnerBlock
ALIGN 4
@@StereoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  jmp @@StereoPlaying

;************************ StereoMixer ohne loop *****************************
ALIGN 4
   @@StereoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleEnding        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                     add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov cl,bl
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                     adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                    mov bp,es:[ecx+ecx]
                    add ds:[di+2],bp
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx
                  jnz @@StereoMixernoloop
                  jmp EndInnerBlock

