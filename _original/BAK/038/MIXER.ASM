;*******************************************
;*** F/X Mixerroutinen v1.0a at 15.02.97 ***

.486p
LOCALS @@
.code

ALIGN 4
;*** Allgemeine Player-Variablen ***

IRQMixing           db 0      ;Flag fÅr Interrupt Mischen an/aus

LastBlock           db 0  ;Welcher Block wurde als letztes gespielt
BlockOffset         dw 0  ;Offset von Block

ALIGN 4
ChannelAnzahl       db 16 ;Anzahl der KanÑle insgesamt
ChannelAktuell      db 0  ;aktueller spielender Channel
ALIGN 4
ChannelFlag         db 16 DUP (0)  ;Channel Flags (Aktiv/Pause/Aus/...)
ALIGN 4
PlayingFlag         db 16 DUP (0)  ;Spielt Kanal ?


TickBytes           dw 16 DUP (0)  ;Wieviel Bytes sind ein Tick
BufferBytes         dw 16 DUP (0)  ;Wieviele Bytes sind ein Buffer

PatternOrderPlay    db 0  ;Welches OrderPattern im Moment gespielt wird
PatternOrderNext    db 0  ;Welches OrderPattern als nÑchstes kommt
PatternPlay         db 0  ;Welches PatternNr im Moment gespielt wird
PatternNext         db 0


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

Mixer               proc near      ;!! Vorher MU· "_InitMixer"  !!
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

             ;*** Nach BlockNr OffsetAdr zuordnen ***

                    mov al,SBBlock
                    or al,al                       ;Ist Block0
                    jnz @@OffsetBlock1
                    mov BlockOffset,PBlockLength  ;Dann Offset=1000h bei 8 Bit
                     mov cl,1
                     sub cl,SB16Flag
                     shr BlockOffset,cl
                    jmp @@noOffset
        @@OffsetBlock1:
                    mov BlockOffset,0              ;sonst 0
        @@noOffset:

            ;*** Clear Buffer Counter again ***

                    mov ebp,0
                    movzx ebx,ChannelAnzahl
                    xor ax,ax

        @@ClearBufferCounter:
                    mov BufferBytes+[ebp*2],ax
                    inc ebp
                    cmp ebp,ebx
                    jnz @@ClearBufferCounter

                    xor edi,edi              ; BufferPointer auf 0

ALIGN 4
        ;********************************************************
        ;************* PlayBlock Schleife ***********************

@@GoBuffer:
                    mov ChannelAktuell,0

ALIGN 4
        ;********************************************************
        ;************* Channel Schleife *************************

@@GoChannel:
                    movzx ebp,ChannelAktuell ;Aktueller Kanal holen

                    cmp ChannelFlag+bp,0     ;wenn Flag nicht 0
                    jnz @@NextChannel        ;dann naechsten

                    xor edi,edi              ; 0 setzen
                    mov dx,CountTick+[ebp*2]       ;BlocklÑnge

             ;******************************************
             ;********** Innere Block Routine **********

              ;***** Tick Routinen *****

                  ;dec dx                   ;Hat Ticker bereits Tick erreicht
                  jmp noTick               ;nein.. dann kein Tick
ALIGN 4
              Ticker:
                      call Ticker_S3M

              noTick:
                      movzx ebp,ChannelAktuell
                      ;call CalcTickBuffer

                      cmp PlayingFlag+bp,0  ;PlayingFlag = 0
                      jz @@noSamplePlay     ;dann stille Sample schreiben

                      call PrepareMixer

                      cmp SBinterpolation,0
                      jnz @@InterpolationPlay

;---------- Interpolation OFF --------------------------------

                        cmp SBstereoFlag,2
                        jae @@StereoPlay
                         cmp SBstereoFlag,1
                         je @@WideMonoPlay

                           test SampleLoopFlag,1   ;SampleFlag Bit 1 gesetzt
                           jz @@MonoMixernoloop    ;wenn nicht dann kein Loop
                           jmp @@MonoMixerloop
              @@WideMonoPlay:
                           test SampleLoopFlag,1
                           jz @@WideMonoMixernoloop
                           jmp @@WideMonoMixerloop

              @@StereoPlay:
                         cmp SBstereoFlag,3
                         je @@WideStereoPlay

                           test SampleLoopFlag,1
                           jz @@StereoMixernoloop
                           jmp @@StereoMixerloop
              @@WideStereoplay:
                           test SampleLoopFlag,1
                           ;jz @@WideStereoMixernoloop
                           ;jmp @@WideStereoMixerloop


;---------- Interpolation ON ---------------------------------
@@InterpolationPlay:
                     dec SampleLoopEnding
                     dec SampleLengthing

                        cmp SBstereoFlag,2
                        jae @@StereoPlayI
                         cmp SBstereoFlag,1
                         je @@WideMonoPlayI

                           test SampleLoopFlag,1   ;SampleFlag Bit 1 gesetzt
                           jz @@IMonoMixernoloop   ;wenn nicht dann kein Loop
                           jmp @@IMonoMixerloop
              @@WideMonoPlayI:
                           test SampleLoopFlag,1   ;SampleFlag Bit 1 gesetzt
                           jz @@IWideMonoMixernoloop   ;wenn nicht dann kein Loop
                           jmp @@IWideMonoMixerloop

              @@StereoPlayI:
                           test SampleLoopFlag,1
                           jz @@IStereoMixernoloop
                           jmp @@IStereoMixerloop

ALIGN 4
         ;***************** END INNERBLOCK *******************
 EndInnerBlock:
             movzx ebp,ChannelAktuell
             mov SamplePosition+[ebp*4],esi  ;SamplePosition abspeichern
             mov SamplePosScale+[ebp*4],edx ;Momentale Position als Fixkomma
             mov CountTick+[ebp*2],dx

@@NextChannel:
             inc ChannelAktuell      ;Ein Kanal weiter
             mov al,ChannelAnzahl
             cmp al,ChannelAktuell   ;schon letzter Kanal
             jnz @@GoChannel         ;noch nicht.. dann weiter

         ;*********** End Channel Schleife *********

             ;cmp CountTick+[ebp*2],0
             ;jnz @@EndBuffer

             ;call Ticker_S3M

             ;mov ax,MBlockLength/2
             ;cmp SBstereoFlag,0
             ;jz @@noStereo
             ;   shr ax,1
    ;@@noStereo:
             ;cmp BufferBytes+[ebp*2],ax
             ;jnz @@GoBuffer

         ;*********** End Buffer Schleife *********
@@EndBuffer:

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

INCLUDE MixContr.asm
INCLUDE MixMI0.asm
INCLUDE MixSI0.asm
INCLUDE MixMI1.asm
INCLUDE MixSI1.asm

;**************************************************************************
ALIGN 4
       ;***** Routinen zur StilleSample Berechnung *****

@@SampleEnd:
                    movzx ebp,ChannelAktuell
                    mov PlayingFlag+bp,0
@@noSamplePlay:
                    cmp SBstereoFlag,1
                    jae @@stereoSampleEnd

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

INCLUDE FXutil.asm
