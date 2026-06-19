;**************************************************************************
;********* Effektgeneratoren und Unterroutinen fr dieselbigen ************

;****** Routine die TickSpeed aufgrund TickAnzahl neu berechnet ****

CalcSpeed   proc near
                               ;bl=Tempo
            push edx
            push ebx

            movzx ebx,bl
            shl ebx,2           ;*4

            mov eax,CalcMixSpd      ;Freq
            mov edx,10*2        ;*2 fr doppelte Genauigkeit
            mul edx             ;*10 auf dx:ax erweitern

            div ebx             ;/(InitTempo*4)
            inc eax
            shr eax,1           ;Runden

            pop ebx
            pop edx
            ret                ;ax=Bytes pro Tick

CalcSpeed   endp

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

      @@NIX:
            pop edx
            pop ecx
            ret   ; > eax = SampleScale

CalcScale   endp
