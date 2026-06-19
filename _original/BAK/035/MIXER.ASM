;*******************************************
;*** F/X Mixerroutinen v0.5b at 20.12.96 ***

.486p
LOCALS @@

;*** Allgemeine Player-Variablen ***

IRQMixing           db 0      ;Flag fÅr Interrupt Mischen

LastBlock           db 0  ;Welcher Block wurde als letztes gespielt
BlockOffset         dw 0  ;Offset von Block
LastTimer           dd 0
NowTimer            dd 0

ChannelAnzahl       db 16 ;Anzahl der KanÑle insgesamt
ChannelAktuell      db 0
PatternOrderPlay    db 0  ;Welches OrderPattern im Moment gespielt wird
PatternOrderNext    db 0  ;Welches OrderPattern als nÑchstes kommt
PatternPlay         db 0  ;Welches PatternNr im Moment gespielt wird
PatternNext         db 0

TickBytes           dw 16 DUP (0)  ;Wieviel Bytes sind ein Tick

PeriodeNote         dw 1712,1616,1524,1440,1356,1280,1208,1140,1076,1016
                    dw 960,907     ; Perioden von C-4 bis H-4

PatternStart        db 16 DUP (0)
PatternRow          db 16 DUP (64)
PatternTempoBytes   dw 16 DUP (0)
PatternTempoRow     db 16 DUP (0)
PatternSpeed        db 16 DUP (0)
PatternSpeedRow     db 16 DUP (0)
PatternChannelFlag  db 1

;*** ChannelabhÑngige Variablen ***
ALIGN 4
PlayingFlag         db 16 DUP (0)  ;Spielt Kanal ?

CountTick           dw 16 DUP (0)  ;Anzahl Bytes schon gespielt
TickNow             db 16 DUP (0)  ;Welcher Tick ist gerade
SpeedNow            db 16 DUP (0)  ;InitSpeed im durchlauf
RowPos              db 16 DUP (0)  ;Welche Row spielt gerade

ChannelSampleNr     db 16 DUP (0)  ;Welches SampleNr spielt gerade
ChannelNote         db 16 DUP (0)
ChannelOctave       db 16 DUP (0)
ChannelPeriode      dd 16 DUP (0)     ;Periode die momentan spielt
ChannelVolume       db 16 DUP (0)     ;Wirkliche Volume des Kanals
ChannelPanning      db 16 DUP (07h)
ChannelGVLeft       db 16 DUP (0)     ;Volume on left side
ChannelGVRight      db 16 DUP (0)
ChannelEffekt       db 16 DUP (0FFh)  ;Effekt des Kanals
ChannelEffektInfo   db 16 DUP (0)     ;EffektInfo des Kanals
GlissandoPeriode    dd 16 DUP (0)     ;ZielPeriode bei Glissando
GlissandoSpeed      db 16 DUP (0)

SampleFlag          db 16 DUP (0)  ;Flag des Samples
SampleLength        dd 16 DUP (0)  ;LÑnge des Samples auf jeweiligen Kanal
SampleLoopBegin     dd 16 DUP (0)
SampleLoopEnd       dd 16 DUP (0)
SampleVolume        db 16 DUP (0)
SampleFinetune      dw 16 DUP (0)  ;Sample Finetune value

SampleOffset        dd 16 DUP (0)  ;Sample Offset im Speicher
SamplePosition      dd 16 DUP (0)  ;Wirkliche Position des Samples
SamplePosScale      dd 16 DUP (0)  ;Position des Samples in Fixkomma
SampleScaleLow      dd 16 DUP (0)  ;Scale Sample low in Fixkomma
SampleScaleHigh     dd 16 DUP (0)
SamplePeriode       dw 16 DUP (0)
SampleFreq          dd 16 DUP (0)  ;Frequenz des Samples in DD

ALIGN 4
ScalingLow          dd 0
ScalingHigh         dd 0
SampleLengthing     dd 0
SampleLoopBegining  dd 0
SampleLoopEnding    dd 0
ALIGN 4
SampleLoopFlag      db 0


;Entspricht 1050+? Bytes Leerraum !
;*******************************************************

public              _InitMixer   ;initialisiert Mixer fÅr neues S3M
public              _StartMixer  ;Aktiviert den periodischen Mischer
public              _CallMixer   ;Aufruf per Hand: berechnet Block wenn frei
public              _StopMixer   ;deaktiviert periodischen aufruf
public              _PlayFF      ;play FastForward (skip Pattern)
public              _Pause       ;pause Playback / starts Playback again
public              _Takt        ;return in dx:ax Taktcycles (only Pentium !)
public              _ChangeMode


_InitMixer          proc far

                    pushad
                    push ds
                    push es
                    push fs
                    push gs

         ;**************************************
         ;******** Vorbereitungen **************

             ;*** MasterVolume berechnen ***

                    or SB16Flag,0
                    jz @@noCalc16
                       call CalcMVol16        ;MasterVolume berechnen
                       jmp @@noCalc8
             @@noCalc16:
                       call CalcMVol8
             @@noCalc8:
             ;*** Pattern entpacken und vorbereiten ***

                    xor ax,ax
                    mov es,ax
                    mov esi,OrderListPointer
                    mov al,es:[esi]         ;OrderNr lesen
                    mov PatternPlay,al     ;PatternNr abspeichern
                    call ExpandPattern     ;Pattern entpacken

                    mov al,15              ;ab Kanal 15
            CopyCh1:
                    call CopyChannel     ;kopiere Kanal
                    dec al               ;in PatternPlay
                    jns CopyCh1      ;bis noch nicht neg (inclusive 0)

                    mov PatternOrderPlay,0 ;PatternOrderPlay=0
                    mov PatternOrderNext,1 ;NÑchstes PatternOrder wÑre 1
                    inc esi                 ;Eine Order weiter
                    mov al,es:[esi]         ;lies PatternNr
                    cmp al,255
                    jb @@PatternOrderOK
                       mov PatternOrderNext,0
                       mov PatternNext,0
                       jmp @@PatternEnd
         @@PatternOrderOK:
                    mov PatternNext,al     ;Speichere nÑchstes PatternNr
                    call ExpandPattern     ;Entpacke nÑchstes Pattern
         @@PatternEnd:
             ;*** Ticks berechnen aus Starttempo ***

                    mov bl,InitTempo
                    call CalcSpeed

           ;*** Variablen fÅr START vorbereiten ***

                    movzx ebx,ChannelAnzahl
                    dec ebx
                    mov cl,InitSpeed
                    mov ch,cl
                    dec cl

      @@InitAgain:
                    mov TickBytes+[ebx*2],ax
                    mov RowPos+bx,-1
                    mov TickNow+bx,cl
                    mov SpeedNow+bx,ch
                    mov CountTick+[ebx*2],1 ;Ein Byte vor Ende
                    mov PlayingFlag+bx,0
                    dec ebx
                    jns @@InitAgain

                    pop gs
                    pop fs
                    pop es
                    pop ds
                    popad
                    retf

_InitMixer          endp

ALIGN 4
;****************************************************************************
;************ Hauptroutine des Mixers fÅr den F/X Player ********************

Mixer               proc near      ;!! Vorher MU· "InitPlay"  !!
                                  ;!! aufgerufen worden sein !!
                    pushad
                    push ds
                    push es
                    push fs
                    push gs

       ;*********************************************
       ;*********** MIXER ROUTINE *******************

                     ;TaktStart
                     mov es,ChannelsSeg

                     mov cx,MBlockLength/4
                     xor di,di
                     mov eax,32767*65536+32767

                     rep stosd
                     ;TaktEnd

             xor ax,ax
             mov es,ax               ;es ist IMMER 0 Segment
             mov ds,ChannelsSeg      ;ds immer wo Channel sind
             mov ChannelAktuell,0

             ;*** Nach BlockNr OffsetAdr zuordnen ***

                    mov al,SBBlock
                    or al,al                       ;Ist Block0
                    jnz OffsetBlock1
                    mov BlockOffset,PBlockLength  ;Dann Offset=1000h bei 8 Bit
                     mov cl,1
                     sub cl,SB16Flag
                     shr BlockOffset,cl
                    jmp noOffset
        OffsetBlock1:
                    mov BlockOffset,0              ;sonst 0
        noOffset:
ALIGN 4
        ;********************************************************
        ;************* Channel Schleife *************************

     GoChannel:
                    movzx ebp,ChannelAktuell ;Aktueller Kanal holen
                    xor esi,esi              ;und alles 32 Bit Register
                    xor edi,edi              ;setzen
                    xor edx,edx
                    mov esi,SamplePosition+[ebp*4]
                    mov edx,SamplePosScale+[ebp*4] ;momentale Pos als Fixkomma
                    mov dx,CountTick+[ebp*2]       ;BlocklÑnge

                    mov ebx,VolHalveOffset
                    ;mov ebx,MBlockLength
                    ;shr ebx,1
                    mov ecx,ebx
                    movzx eax,ChannelGVLeft+bp  ;ChannelGVolume holen
                    shl eax,8
                    add ebx,eax
                    movzx eax,ChannelGVRight+bp
                    shl eax,8
                    add ecx,eax

             ;******************************************
             ;********** Innere Block Routine **********

              ;***** Tick Routinen *****

                  dec dx                   ;Hat Ticker bereits Tick erreicht
                  jnz noTick               ;nein.. dann kein Tick
ALIGN 4
              Ticker:
                      movzx ebp,ChannelAktuell
                      INCLUDE Ticker.asm
              noTick:
                      mov al,SampleFlag+bp
                      mov SampleLoopFlag,al
                      mov eax,SampleLoopBegin+[ebp*4]
                      mov SampleLoopBegining,eax
                      mov eax,SampleLoopEnd+[ebp*4]
                      mov SampleLoopEnding,eax
                      mov eax,SampleLength+[ebp*4]
                      mov SampleLengthing,eax
                      mov eax,SampleScaleLow+[ebp*4]
                      mov ScalingLow,eax
                      mov eax,SampleScaleHigh+[ebp*4]
                      mov ScalingHigh,eax

                      cmp PlayingFlag+bp,0  ;PlayingFlag = 0
                      jz @@noSamplePlay     ;dann stille Sample schreiben

                      cmp SBstereoFlag,1
                      jz @@StereoPlay
                         test SampleLoopFlag,1    ;SampleFlag Bit 1 gesetzt
                         jz @@MonoMixernoloop     ;wenn nicht dann kein Loop
                         jmp @@MonoMixerloop
           @@StereoPlay:
                         test SampleLoopFlag,1
                         jz @@StereoMixernoloop
                         jmp @@StereoMixerloop

ALIGN 4
         ;***************** END INNERBLOCK *******************
 EndInnerBlock:
             movzx ebp,ChannelAktuell
             mov SamplePosition+[ebp*4],esi  ;SamplePosition abspeichern
             mov SamplePosScale+[ebp*4],edx ;Momentale Position als Fixkomma
             mov CountTick+[ebp*2],dx

             inc ChannelAktuell      ;Ein Kanal weiter
             mov al,ChannelAnzahl
             cmp al,ChannelAktuell   ;schon letzter Kanal
             jnz GoChannel           ;noch nicht.. dann weiter

         ;*********** End Channel Schleife *********

         ;*******************************************
         ;******** MasterVolume mixing **************

             mov gs,PlayBlockSeg       ;Segment des PlayBuffers
             mov ecx,MasterVolOffset   ;Segment von MasterVolume
             mov di,BlockOffset        ;di mit Offset von PlayBlock laden
             xor esi,esi
             xor ebx,ebx
             xor ebp,ebp

             ;TaktStart
                 cmp SB16Flag,0
                 jz no16Mix
                    CALL MasterCalc16
                    jmp KEYpress
         no16Mix:
                    CALL MasterCalc8

       ;********* KEY PRESSED ***********+

KEYpress:
              ;TaktEnd

               ;call PrintInfo    ;Schweinekram hinpinseln

                    pop gs
                    pop fs
                    pop es
                    pop ds
                    popad
                    ret          ;und schÅss !!

INCLUDE MixMode.asm

;**************************************************************************
ALIGN 4
       ;***** Routinen zur StilleSample Berechnung *****

@@SampleEnd:
                    movzx ebp,ChannelAktuell
                    mov PlayingFlag+bp,0
@@noSamplePlay:
                    cmp SBstereoFlag,1
                    jz @@stereoSampleEnd

                    shl dx,1
                    add di,dx
                    mov dx,0
                    cmp di,MBlockLength
                    jb Ticker

                    sub di,MBlockLength-2
                    shr di,1
                    mov dx,di
                    mov di,MBlockLength
                    jmp EndInnerBlock

@@stereoSampleEnd:
                    shl dx,2
                    add di,dx
                    mov dx,0
                    cmp di,MBlockLength
                    jb Ticker

                    sub di,MBlockLength-4
                    shr di,2
                    mov dx,di
                    mov di,MBlockLength
                    jmp EndInnerBlock

Mixer               endp

;******************** Other Routines for Player itself **********************

_StartMixer         proc far

                    mov IRQMixing,1
                    retf

_StartMixer         endp

_StopMixer          proc far

                    mov IRQMixing,0
                    retf

_StopMixer          endp

_CallMixer          proc far

                    push ax

             ;*** Ist Block frei ? ***
                    mov ah,LastBlock
ALIGN 4
      ControlBlock:
                    mov al,SBBlock
                    cmp al,ah          ;Ist JetzBlock=Lastblock
                    jz noControlBlock  ;Wenn ja... warten
                    mov LastBlock,al   ;Sonst LastBlock=JetztBlock

                    call Mixer
    noControlBlock:
                    pop ax
                    retf

_CallMixer          endp

_PlayFF             proc far

                    cli
                    push ebp

                    movzx ebp,ChannelAnzahl
                    dec bp
           @@NextCh:
                    mov RowPos+bp,63  ;RowPos wieder lîschen
                    mov PatternStart+bp,0
                    mov PatternRow+bp,64
                    mov TickNow+bp,0
                    mov CountTick+[ebp*2],1
                    dec bp
                    jns @@NextCh

                    pop ebp
                    sti
                    retf

_PlayFF             endp

PauseFlag           db 0

_Pause              proc far

                    push ax
                    cmp SB16flag,0
                    jz @@8bit

                    mov ah,0D5h       ; D5h = pause 16bit
                    add ah,PauseFlag  ; D6h = start 16bit
                    call WrDSP
                    xor PauseFlag,1
                    pop ax
                    retf
             @@8bit:
                    mov ah,0D0h
                    mov al,PauseFlag
                    shl al,2
                    add ah,al
                    call WrDSP
                    xor PauseFlag,1
                    pop ax
                    retf

_Pause              endp

_Takt               proc far

                    mov eax,NowTimer
                    bswap eax
                    mov dx,ax
                    xchg dh,dl
                    bswap eax
                    retf

_Takt               endp

_SetGlobalV         proc far

                    mov GlobalVolume,bl
                    retf

_SetGlobalV         endp

_ChangeMode         proc far

                    push ebx
                    push ecx
                    cmp SBstereoFlag,1
                    jz @@stereo

                    mov eax,MixSpd
                    mov CalcMixSpd,eax
                    mov SBstereoFlag,1
                    movzx ebx,ChannelAnzahl
                    dec ebx

      @@InitAgain1:
                    mov ax,TickBytes+[ebx*2]
                    shr ax,1
                    mov TickBytes+[ebx*2],ax

                    dec ebx
                    jns @@InitAgain1
                    pop ecx
                    pop ebx
                    retf
            @@stereo:
                    mov eax,MixSpd
                    shl eax,1
                    mov CalcMixSpd,eax
                    mov SBstereoFlag,3
                    movzx ebx,ChannelAnzahl
                    dec ebx

      @@InitAgain2:
                    mov ax,TickBytes+[ebx*2]
                    shl ax,1
                    mov TickBytes+[ebx*2],ax
                    dec ebx
                    jns @@InitAgain2
                    pop ecx
                    pop ebx
                    retf

_ChangeMode         endp

;INCLUDE Print.asm
