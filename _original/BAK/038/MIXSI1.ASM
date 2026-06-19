
;----------------------------------------------------------------------------
;------------------------ Interpolation 1. grade ----------------------------

;****************************************************************************
;**************************** S T E R E O  **********************************

ALIGN 4
;************************ StereoMixer mit loop ******************************

   @@IStereoMixerloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                    cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                    jnb @@IStereoLoop           ;Wenn nicht dann nicht

          ;********* Sample spielen *************
ALIGN 4
      @@IStereoPlaying:
                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen

                    Interpolation

                     add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov cl,bl
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                     adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                    mov bp,es:[ecx+ecx]
                    add ds:[di+2],bp
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@IStereoMixerloop
                  jmp Ticker

@@IStereoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  or edx,32768*65536
                  jmp @@IStereoPlaying

;************************ StereoMixer ohne loop *****************************
ALIGN 4
   @@IStereoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen

                    Interpolation

                     add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov cl,bl
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                     adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                    mov bp,es:[ecx+ecx]
                    add ds:[di+2],bp
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@IStereoMixernoloop
                  jmp Ticker



