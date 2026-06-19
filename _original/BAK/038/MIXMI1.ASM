
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
      @@IMonoPlaying:
                    ;TaktStart

                     mov bl,byte ptr es:[esi]      ;Aus FMS auslesen

                     Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@IMonoMixerloop
                  jmp Ticker

@@IMonoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  or edx,32768*65536
                  jmp @@IMonoPlaying

;************************ MonoMixer ohne loop *******************************

ALIGN 4
   @@IMonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                     mov bl,byte ptr es:[esi]      ;Aus FMS auslesen

                     Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@IMonoMixernoloop
                  jmp Ticker

;****************************************************************************
;*************************** W I D E  M O N O *******************************

ALIGN 4
;************************** MonoMixer mit loop ******************************

   @@IWideMonoMixerloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                    cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                    jnb @@IWideMonoLoop            ;Wenn dann loop laden...

          ;********* Sample spielen *************
ALIGN 4
      @@IWideMonoPlaying:
                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen

                    Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    test di,MBlockLength-1    ;Channel schon fertig
                    jz EndInnerBlock        ;nein, dann weiter

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@IWideMonoMixerloop
                  jmp Ticker

@@IWideMonoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  or edx,32768*65536
                  jmp @@IWideMonoPlaying

;************************ MonoMixer ohne loop *******************************

ALIGN 4
   @@IWideMonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen

                    Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    test di,MBlockLength-1    ;Channel schon fertig
                    jz EndInnerBlock        ;nein, dann weiter

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@IWideMonoMixernoloop
                  jmp Ticker
