ALIGN 4
  ;********************* MonoMixer mit loop ************************

   @@MonoMixerloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                    cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                    jb @@MonoPlaying        ;Wenn nicht dann nicht

                      mov esi,SampleLoopBegining ;LoopBegin laden
                      and edx,65535              ;laden

          ;********* Sample spielen *************
ALIGN 4
      @@MonoPlaying:
                    TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec cx
                  jnz @@MonoMixerloop
                  jmp Ticker

  ;********************* MonoMixer ohne loop ************************

ALIGN 4
   @@MonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     add di,2               ;MixingPointer um 2 erh”hen

                    TaktEnd

                  test di,MBlockLength-1    ;Channel schon fertig
                  jz EndInnerBlock        ;nein, dann weiter
                  dec cx
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
                    jb @@StereoPlaying        ;Wenn nicht dann nicht

                      mov esi,SampleLoopBegining ;LoopBegin laden
                      and edx,65535                   ;laden

          ;********* Sample spielen *************
ALIGN 4
      @@StereoPlaying:
                    TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                     add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     push bx
                     mov bh,20h
                    mov ax,ds:[ebx+ebx]
                     pop bx
                    add ds:[di+2],ax
                     add di,4               ;MixingPointer um 2 erh”hen

                    TaktEnd

                  cmp di,MBlockLength    ;Channel schon fertig
                  jge EndInnerBlock        ;nein, dann weiter
                  dec cx
                  jnz @@StereoMixerloop
                  jmp Ticker

  ;********************* StereoMixer ohne loop ************************
ALIGN 4
   @@StereoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                  cmp esi,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    TaktStart

                    mov bl,es:[esi]      ;Aus FMS auslesen
                     add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                     adc esi,ScalingHigh
                    add ds:[di],ax           ;Auf Channel draufaddieren
                     push bx
                     mov bh,20h
                    mov ax,ds:[ebx+ebx]
                     pop bx
                    add ds:[di+2],ax
                     add di,4               ;MixingPointer um 2 erh”hen

                    TaktEnd

                  cmp di,MBlockLength    ;Channel schon fertig
                  jge EndInnerBlock        ;nein, dann weiter
                  dec cx
                  jnz @@StereoMixernoloop
                  jmp Ticker



