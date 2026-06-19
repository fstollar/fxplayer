;*******************************************
;*** F/X Mixerroutinen v1.0a at 15.02.97 ***

.486p
LOCALS @@
.code

MaxChannels      equ 32             ; how many channels are available
MC               equ MaxChannels    ; shortcut

low16bit         equ 32768          ; value of 16th bit on
high16bit        equ 32768*65536    ; value of 32th bit on

ALIGN 4
;*** Allgemeine Player-Variablen ***

LastBlock             db 0   ; which block was last played
BlockOffset           dw 0   ; offset of SB block
 ALIGN 4
IRQMixing             db 0   ; flag for self-mixing or not


 ALIGN 4
ChannelAktuell        db 0           ; Nr. Channel is calulating now
 ALIGN 4
ChannelAnzahl         db MC          ; max Channels playing REALLY
 ALIGN 4
ChannelFlag           db MC DUP (0)  ; Channel Flags (activ/pause/off/...)
 ALIGN 4
PlayingFlag           db MC DUP (0)  ; 0 = channel off / 1 = channel playing

BufferBytes           dw MC DUP (0)  ; how many bytes are already in buffer
CountTick             dw MC DUP (0)  ; number bytes which are for mixing

;*** Volumes for mixing ***
 ALIGN 4
VolumeLeft            dw MC DUP (0)  ; volume of the left side
VolumeRight           dw MC DUP (0)  ; volume of the right side




;*** Sample/Channel Variablen ***
; (mem) means pointer in memory
; (rel) means relative pointer

 ALIGN 4
SampleFlag            db MC DUP (0)  ; flags of the sample like loop...

 ALIGN 4
SampleOffsetHigh      dd MC DUP (0)  ; save high 32bit sample offset (rel)
SampleOffsetLow       dd MC DUP (0)  ; save low 32bit       -"-      (rel)
SampleScale           dd MC DUP (0)  ; scaling of sample (tone)

 ALIGN 4
SampleNumber          db MC DUP (0)  ; Number of Sample (0=nothing)
SampleFineTune        dd MC DUP (0)  ; FineTune of the Sample

SampleStartOffset     dd MC DUP (0)  ; startpoint of sample in memory (mem)
SampleLength          dd MC DUP (0)  ; length of sample (rel)
;SampleLoopStart       dd MC DUP (0)  ; Loop start (rel)
;SampleLoopEnd         dd MC DUP (0)  ; Loop End (rel)
SampleEndOffset       dd MC DUP (0)  ; endpointer of sample (Start+Length) (mem)
SampleLoopStartOffset dd MC DUP (0)  ; pointer of sample loop start (mem)
SampleLoopEndOffset   dd MC DUP (0)  ; pointer of loop end (mem)

SampleFreq            dd MC DUP (0)  ; frequence of the playing sample


 ALIGN 4
ChannelOctave         db MC DUP (0)  ; octave which is playing now
ChannelNote           db MC DUP (0)  ; note which is playing
ChannelPeriode        dd MC DUP (0)  ; periode of note+octave
ChannelVolume         dw MC DUP (0)  ; volume of theis channel
ChannelEffect         db MC DUP (0)  ; actual Effect active
ChannelEffectValue    db MC DUP (0)  ; actual effect value
ChannelLastEffect     db MC DUP (0)  ; effect last row
ChannelPanning        db MC DUP (7)  ; panning of the channel


;*** Dummy Variablen ***
 ALIGN 4
SampleLoopFlag      db 0  ; if actual sample is looped = 1
 ALIGN 4
ScalingLow          dd 0  ; low 16bit of scaling
ScalingHigh         dd 0  ; high 16bit of scaling
SampleEnding        dd 0  ; end of a sample
SampleLoopBegining  dd 0  ; where a loop starts
SampleLoopEnding    dd 0  ; where a loop ends


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
                    mov ds,ax
                    mov PatternOrderPlay,-1  ; PatternOrderPlay = -1

                    call NextPattern ; dann ist PatternOrderPlay = 0

                    mov al,PatternPlay
                    call ExpandFirstRow

             ;*** Ticks berechnen aus Starttempo ***

                    mov bl,InitTempo
                    call CalcSpeed
                    mov TickBytes,ax  ; Bytes fuer ein Tick

           ;*** Variablen fÅr START vorbereiten ***

                    movzx ebx,ChannelAnzahl
                    dec ebx
                    mov cl,InitSpeed
                    mov SpeedNow,cl        ; Momentane Speed
                    mov RowFlag,1

      @@InitAgain:
                    mov RowPosNow+[ebx],0
                    mov TickNow+[ebx],0
                    mov CountTick+[ebx*2],ax   ; Am Anfang eines neuen Ticks
                    mov PlayingFlag+[ebx],0
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

             xor ax,ax
             mov ds,ax           ; ds is ALWAYS ZERO !!!
             mov es,ChannelsSeg  ; es points always on mixing buffer

                     ;TaktStart

                     mov cx,MBlockLength/4
                     xor di,di
                     mov eax,32767*65536+32767

                     rep stosd
                     ;TaktEnd

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

                    movzx ebx,ChannelAnzahl
                    dec ebx
                    xor ax,ax

        @@ClearBufferCounter:
                    mov BufferBytes+[ebx*2],ax
                    dec ebx
                    jns @@ClearBufferCounter

                    xor edi,edi              ; BufferPointer auf 0

ALIGN 4
        ;********************************************************
        ;************* PlayBlock Schleife ***********************
@@GoBuffer:

                   mov ChannelAktuell,0

                    cmp RowFlag,0
                    jz @@noRow

                    call Ticker_S3M
           @@noRow:

ALIGN 4
        ;********************************************************
        ;************* Channel Schleife *************************

@@GoChannel:
                    movzx ebp,ChannelAktuell ;Aktueller Kanal holen

                    cmp ChannelFlag+[ebp],32    ;wenn Flag nicht unter 16
                    jnb @@NextChannel           ;dann naechsten

                    mov di,BufferBytes+[ebp*2]

             ;******************************************
             ;********** Innere Block Routine **********
ALIGN 4
@@InnerBlock:
              ;***** Tick Routinen *****

                      movzx ebp,ChannelAktuell

                      call CalcTickBuffer

                      cmp PlayingFlag+[ebp],0  ;PlayingFlag = 0
                      jz @@NextTick     ;dann stille Sample schreiben

                      call PrepareMixer
                      call Modify_code

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
                        ;dec SampleLoopEnding

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
             movzx ebp,ChannelAktuell           ; reload ebp with channelNr
             sub esi,SampleStartOffset+[ebp*4]  ; sub esi, real mem offset
             mov SampleOffsetHigh+[ebp*4],esi   ; save sample offset (rel)
             mov SampleOffsetLow+[ebp*4],edx    ; save sample fraction
ALIGN 4
@@NextTick:
             cmp CounterFlag,0
             jnz @@noTick

               call Ticker_S3M_Tick

             cmp BufferBytes+[ebp*2],MBlockLength
             jz @@NextChannel

             cmp RowFlag,1
             jnz @@InnerBlock

     @@noTick:

ALIGN 4
@@NextChannel:
             inc ChannelAktuell      ;Ein Kanal weiter
             mov al,ChannelAnzahl
             cmp al,ChannelAktuell   ;schon letzter Kanal
             jnz @@GoChannel         ;noch nicht.. dann weiter

         ;*********** End Channel Schleife *********

             mov ax,MBlockLength

             cmp BufferBytes,ax
             jnz @@GoBuffer

         ;*********** End Buffer Schleife *********
@@EndBuffer:

         ;*******************************************
         ;******** MasterVolume mixing **************
 ALIGN 4
             mov ebp,MasterVolOffset   ;Segment von MasterVolume
             movzx edi,BlockOffset     ; edi with block offset
             add edi,RealAdrDMA        ; add edi with offset DMA play buffer
             xor esi,esi

             xor eax,eax
             xor ebx,ebx               ; clear higher 16 bit
             xor ecx,ecx               ; -- " --
             xor edx,edx

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

               call PrintInfo    ;Schweinekram hinpinseln

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
                    mov PlayingFlag+[ebp],0

                    jmp EndInnerBlock

Mixer               endp

;****************************************************************************
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
