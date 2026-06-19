ALIGN 4
  ;********************* MonoMixer mit loop ************************

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
                    mov ax,es:[ebx*2]      ;mit Volume multiplizieren
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
                  jmp @@MonoPlaying

  ;********************* MonoMixer ohne loop ************************

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
                    mov ax,es:[ebx*2]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@MonoMixernoloop
                  jmp Ticker

;***************************************************************************
;************** S T E R E O  ***********************************************

ALIGN 4
  ;********************* StereoMixer mit loop ************************

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
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                     adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                    mov bp,ds:[ecx+ecx]
                    add ds:[di+2],bp
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@StereoMixerloop
                  jmp Ticker

@@StereoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  jmp @@StereoPlaying

  ;********************* StereoMixer ohne loop ************************
ALIGN 4
   @@StereoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                     add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov cl,bl
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                     adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                    mov bp,ds:[ecx+ecx]
                    add ds:[di+2],bp
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec dx
                  jnz @@StereoMixernoloop
                  jmp Ticker



