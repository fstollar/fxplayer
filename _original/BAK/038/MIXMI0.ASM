;----------------------------------------------------------------------------
;-------------------------- Interpolation OFF -------------------------------

;****************************************************************************
;******************************* M O N O ************************************

ALIGN 4
;************************** MonoMixer mit loop ******************************

   @@MonoMixerloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                    cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                    jnb @@MonoLoop            ;Wenn dann loop laden...

          ;********* Sample spielen *************
ALIGN 4
      @@MonoPlaying:
                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@MonoMixerloop
                  jmp Ticker

@@MonoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  or edx,32768*65536
                  jmp @@MonoPlaying

;************************ MonoMixer ohne loop *******************************

ALIGN 4
   @@MonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@MonoMixernoloop
                  jmp Ticker

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

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    test di,MBlockLength-1    ;Channel schon fertig
                    jz EndInnerBlock        ;nein, dann weiter

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@WideMonoMixerloop
                  jmp Ticker

@@WideMonoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  or edx,32768*65536
                  jmp @@WideMonoPlaying

;************************ MonoMixer ohne loop *******************************

ALIGN 4
   @@WideMonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    test di,MBlockLength-1    ;Channel schon fertig
                    jz EndInnerBlock        ;nein, dann weiter

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,es:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@WideMonoMixernoloop
                  jmp Ticker
