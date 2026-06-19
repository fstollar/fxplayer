;*******************************************
;*** F/X Mixerroutinen v0.3a am 11.07.96 ***

.486
LOCALS @@
;*** Allgemeine Player-Variablen ***

LastBlock           db 0  ;Welcher Block wurde als letztes gespielt
BlockOffset         dw 0  ;Offset von Block
LastTimer           dd 0
NowTimer            dd 0

ChannelAnzahl       db 16 ;Anzahl der Kan„le insgesamt
ChannelAktuell      db 0
PatternOrderPlay    db 0  ;Welches OrderPattern im Moment gespielt wird
PatternOrderNext    db 0  ;Welches OrderPattern als n„chstes kommt
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

;*** Channelabh„ngige Variablen ***

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
ChannelGVolume      db 16 DUP (0)     ;Volume mit GlobalVolume
ChannelEffekt       db 16 DUP (0FFh)  ;Effekt des Kanals
ChannelEffektInfo   db 16 DUP (0)     ;EffektInfo des Kanals
GlissandoPeriode    dd 16 DUP (0)     ;ZielPeriode bei Glissando
GlissandoSpeed      db 16 DUP (0)

SampleFlag          db 16 DUP (0)  ;Flag des Samples
SampleLength        dw 16 DUP (0)  ;L„nge des Samples auf jeweiligen Kanal
SampleLengthing     dw 0
SampleLoopBegin     dw 16 DUP (0)
SampleLoopEnd       dw 16 DUP (0)
SampleLoopEnding    dw 0
SampleVolume        db 16 DUP (0)
SampleFinetune      dw 16 DUP (0)  ;Sample Finetune value

SampleOffset        dd 16 DUP (0)  ;Sample Offset im Speicher
SamplePosition      dw 16 DUP (0)  ;Wirkliche Position des Samples
SamplePosScale      dd 16 DUP (0)  ;Position des Samples in Fixkomma
SampleScale         dd 16 DUP (0)  ;Scale Sample in Fixkomma
Scaling             dd 0
SamplePeriode       dw 16 DUP (0)
SampleFreq          dd 16 DUP (0)  ;Frequenz des Samples in DD

;Entspricht 1050 Bytes Leerraum !
;*******************************************************

public              _StartMixer   ;!!      ! ACHTUNG !       !!

_StartMixer         proc near     ;!! Vorher MUá "InitPlay"  !!
                                  ;!! aufgerufen worden sein !!
                    pushad
                    push ds
                    push es
                    push fs
                    push gs

          ;**************************************
          ;******** Vorbereitungen **************

             ;*** MasterVolume berechnen ***

                    or SB16Flag,0
                    jz noCalc16
                       call CalcMVol16        ;MasterVolume berechnen
                       jmp noCalc8
             noCalc16:
                       call CalcMVol8
             noCalc8:
             ;*** Pattern entpacken und vorbereiten ***

                    mov ds,OrderListSeg
                    xor si,si
                    mov al,ds:[si]         ;OrderNr lesen
                    mov PatternPlay,al     ;PatternNr abspeichern
                    call ExpandPattern     ;Pattern entpacken

                    mov al,15              ;ab Kanal 15
            CopyCh1:
                    call CopyChannel     ;kopiere Kanal
                    dec al               ;in PatternPlay
                    jns CopyCh1      ;bis noch nicht neg (inclusive 0)

                    inc si                 ;Eine Order weiter
                    mov al,ds:[si]         ;lies PatternNr
                    mov PatternNext,al     ;Speichere n„chstes PatternNr
                    call ExpandPattern     ;Entpacke n„chstes Pattern
                    mov PatternOrderPlay,0 ;PatternOrderPlay=0
                    mov PatternOrderNext,1 ;N„chstes PatternOrder w„re 1

             ;*** Ticks berechnen aus Starttempo ***

                    mov bl,InitTempo
                    call CalcSpeed

           ;*** Variablen fr START vorbereiten ***

                    movzx ebx,ChannelAnzahl
                    dec ebx
                    mov cl,InitSpeed
                    mov ch,cl
                    dec cl

      InitAgain:
                    mov TickBytes+[ebx*2],ax
                    mov RowPos+bx,-1
                    mov TickNow+bx,cl
                    mov SpeedNow+bx,ch
                    mov CountTick+[ebx*2],1 ;Ein Byte vor Ende
                    mov PlayingFlag+bx,0
                    dec ebx
                    jns InitAgain

       ;*********************************************
       ;*********** MIXER ROUTINE *******************

             xor ax,ax
             mov fs,ax               ;fs ist IMMER 0 Segment
             mov gs,ChannelsSeg      ;gs immer wo Channel sind
WhileNoKey:                      ;-> Hier Rcksprung nach mixen <-
             mov ChannelAktuell,0

                     mov cx,MBlockLength/16
                     mov eax,32768*65536+32768
                     xor di,di
      InitAgain2:
                     mov gs:[di+0],eax
                     mov gs:[di+4],eax
                     mov gs:[di+8],eax
                     mov gs:[di+12],eax
                     add di,16
                     dec cx
                     jnz Initagain2

             ;*** Ist Block frei ? ***

                     mov NowTimer,0
      ControlBlock:
                     inc NowTimer
                    mov al,SBBlock
                    cmp al,LastBlock  ;Ist JetzBlock=Lastblock
                    jz ControlBlock   ;Wenn ja... warten
                    mov LastBlock,al  ;Sonst LastBlock=JetztBlock

             ;*** Nach BlockNr OffsetAdr zuordnen ***

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

        ;********************************************************
        ;************* Channel Schleife *************************

     GoChannel:
                    movzx ebp,ChannelAktuell ;Aktueller Kanal holen
                    xor esi,esi              ;und alles 32 Bit Register
                    xor edi,edi              ;setzen
                    xor ebx,ebx
                    mov eax,SampleScale+[ebp*4]
                    mov Scaling,eax
                    mov ax,SampleLoopEnd+[ebp*2]
                    mov SampleLoopEnding,ax
                    mov ax,SampleLength+[ebp*2]
                    mov SampleLengthing,ax
                    mov si,SamplePosition+[ebp*2]
                    mov eax,SampleOffset+[ebp*4]
                    mov edx,SamplePosScale+[ebp*4] ;momentale Pos als Fixkomma
                    mov cx,CountTick+[ebp*2]       ;Blockl„nge
                    mov bh,ChannelGVolume+bp  ;ChannelGVolume holen
                    add bh,MBlockLength/2/256

             ;******************************************
             ;********** Innere Block Routine **********

             dec cx
             jz Ticker
             cmp PlayingFlag+bp,0  ;PlayingFlag = 0
             jz @@noSamplePlay     ;dann stille Sample schreiben
             jmp noTick

      InnerBlock:

              ;***** Tick Routinen *****

                  dec cx                   ;Hat Ticker bereits Tick erreicht
                  jnz noTick               ;nein.. dann kein Tick

                      INCLUDE Ticker.asm

                      mov ax,SampleLoopEnd+[ebp*2]
                      mov SampleLoopEnding,ax
                      mov ax,SampleLength+[ebp*2]
                      mov SampleLengthing,ax
                      mov eax,SampleScale+[ebp*4]
                      mov Scaling,eax
                      mov eax,SampleOffset+[ebp*4]

                      cmp PlayingFlag+bp,0  ;PlayingFlag = 0
                      jz @@noSamplePlay     ;dann stille Sample schreiben
         noTick:
                    ;***** Loop Routinen *****

                  cmp SampleFlag+bp,1    ;SampleFlag Bit 1 gesetzt
                  jnz noSampleLoop       ;wenn nicht dann kein Loop

                    cmp si,SampleLoopEnding   ;Ist Sample schon LoopEnd
                    jb SamplePlaying          ;Wenn nicht dann nicht

                      mov dx,SampleLoopBegin+[ebp*2] ;LoopBegin laden
                      mov si,dx          ;SamplePosition mit LoopBegin
                      shl edx,16         ;laden
                      jmp SamplePlaying  ;Springe zu SamplePlaying

      noSampleLoop:

                  cmp si,SampleLengthing        ;Ende schon erreicht
                  jnb @@SampleEnd               ;wenn noch nicht...

          ;********* Sample spielen *************

      SamplePlaying:
                    ;push edx
                    ;push eax
                    ;db 0fh,31h
                    ;mov NowTimer,eax
                    ;pop eax
                    ;pop edx   ;16h=leer

                    mov bl,fs:[esi+eax]      ;Aus FMS auslesen
                    add edx,Scaling          ;SamplePosFix + SampleScale
                    mov si,gs:[ebx+ebx]      ;mit Volume multiplizieren
                    add gs:[di],si           ;Auf Channel draufaddieren
                    mov esi,edx              ;SamplePosFix in esi
                    add di,2               ;MixingPointer um 2 erh”hen
                    add esi,128*256          ;SamplePosFix runden
                    shr esi,16               ;in si nur oberen 16 Bits (80386)

                    ;push edx
                    ;push eax
                    ;db 0fh,31h
                    ;sub eax,NowTimer
                    ;mov NowTimer,eax
                    ;pop eax
                    ;pop edx

         ;*SilentSample:

                  test di,MBlockLength-1  ;Channel schon fertig
                  jnz InnerBlock         ;nein, dann weiter

         ;***************** END INNERBLOCK *******************
 EndInnerBlock:
             mov SamplePosition+[ebp*2],si  ;SamplePosition abspeichern
             mov SamplePosScale+[ebp*4],edx ;Momentale Position als Fixkomma
             mov CountTick+[ebp*2],cx

             inc ChannelAktuell      ;Ein Kanal weiter
             mov al,ChannelAnzahl
             cmp al,ChannelAktuell   ;schon letzter Kanal
             jnz GoChannel           ;noch nicht.. dann weiter

         ;*********** End Channel Schleife *********

         ;*******************************************
         ;******** MasterVolume mixing **************

             mov es,PlayBlockSeg       ;Segment des PlayBuffers
             mov ecx,MasterVolOffset   ;Segment von MasterVolume
             mov di,BlockOffset        ;di mit Offset von PlayBlock laden
             xor esi,esi
             xor ebx,ebx
             xor ebp,ebp

                 cmp SB16Flag,0
                 jz no16Mix
                    CALL MasterCalc16
                    jmp KEYpress
         no16Mix:
                    CALL MasterCalc8

       ;********* KEY PRESSED ***********+

KEYpress:
               call PrintInfo    ;Schweinekram hinpinseln

       WhileKey:
               mov ah,01
               int 16h
               jz WhileNoKey      ;Key holen

               mov ah,0
               int 16h

               cmp ah,75         ;Cursor links
               jnz noCLinks

       noCLinks:
               cmp ah,77         ;Cursor rechts
               jnz noCRechts

                    movzx ebp,ChannelAnzahl
                    dec bp
           @@NextCh:
                    mov RowPos+bp,63  ;RowPos wieder l”schen
                    mov PatternStart+bp,0
                    mov PatternRow+bp,64
                    mov TickNow+bp,0
                    mov CountTick+[ebp*2],1
                    dec bp
                    jns @@NextCh
                    jmp WhileNoKey

       noCRechts:
               cmp ah,01         ; ESC gedrckt ?
               jnz WhileNoKey    ;wenn nicht, weitermachen

                    pop gs
                    pop fs
                    pop es
                    pop ds
                    popad
                    ret          ;und schss !!

       ;***** Routinen zur StilleSample Berechnung *****

@@SampleEnd:
                    mov PlayingFlag+bp,0
@@noSamplePlay:
                    shl cx,1
                    add di,cx
                    mov cx,0
                    cmp di,MBlockLength
                    jb Ticker

                    sub di,MBlockLength-2
                    shr di,1
                    mov cx,di
                    mov di,MBlockLength
                    jmp EndInnerBlock

_StartMixer         endp

INCLUDE Print.asm
