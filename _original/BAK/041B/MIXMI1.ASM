
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
                  sub esi,SampleLoopEnding
                  add esi,SampleLoopBegining    ;LoopBegin laden
                  ;and edx,65535              ;laden
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
                    ;TaktStart

      @@IWideMonoPlaying:
                    mov bl,ds:[esi]      ;Aus FMS auslesen
                    Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add es:[di],ax           ;Auf Channel draufaddieren
                     cmp esi,SampleLoopEnding   ;Ist Sample schon LoopEnd
                     jnb @@IWideMonoLoop            ;Wenn dann loop laden...
                    mov bl,ds:[esi]      ;Aus FMS auslesen
                    Interpolation
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add es:[di+2],ax           ;Auf Channel draufaddieren
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx
                  jnz @@IWideMonoMixerloop
                  jmp EndInnerBlock
ALIGN 4
@@IWideMonoLoop:
                  mov esi,SampleLoopBegining ;LoopBegin laden
                  and edx,65535              ;laden
                  jmp @@IWideMonoPlaying

;************************ MonoMixer ohne loop *******************************

ALIGN 4
   @@IWideMonoMixernoloop:

            ;***** Loop Routinen *****
                  ;Taktstart

                    cmp esi,SampleEnding        ;Ende schon erreicht
                    jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

                    ;TaktStart

                    mov bl,ds:[esi]      ;Aus FMS auslesen
                    Interpolation

                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add es:[di],ax           ;Auf Channel draufaddieren
                     cmp esi,SampleEnding        ;Ende schon erreicht
                     jnb @@SampleEnd               ;wenn noch nicht...
                    mov bl,ds:[esi]      ;Aus FMS auslesen
                    Interpolation
                    add edx,ScalingLow          ;SamplePosFix + SampleScale
                    mov ax,ds:[ebx+ebx]      ;mit Volume multiplizieren
                    adc esi,ScalingHigh
                    add es:[di+2],ax           ;Auf Channel draufaddieren
                     add di,4               ;MixingPointer um 2 erh”hen

                    ;TaktEnd

                  dec dx
                  jnz @@IWideMonoMixernoloop
                  jmp EndInnerBlock
