
public           _LoadS3MF
public           _CleanS3M

                 .code
;*** Allgemeine Daten ***
Handle           dw 0
FMSHandle        dw 0
FMSBuffer        dw 0 ;FMS,Buffer zur öbertragung
FMS32            dd 0
FMSPointer       dd 0

;*** Speicher Segment Daten ***
S3MKopfSegment   dw 0  ;Segment wo S3MKopf
OrderListSeg     dw 0  ;*1 Segment wo OrderList

SamplePointer    dw 0  ;Pointer auf SampleHeaders im S3M
SampleHeadPara   dw 0  ;*2 Pointers auf SampleHeaders in MEM
SampleOffsetPara dw 0  ;*4 32-Bit von Sample im FMS

PatternPointer   dw 0  ;Pointer auf Pattern im S3M
PatternSegment   dw 0  ;*2 Segment (Pointer) auf Pattern in MEM
PatternLength    dw 0  ;Hier wird dummylÑnge des Patterns geladen

;*** S3M Daten ***
OrdNum           dw 0
InsNum           dw 0
PatNum           dw 0
S3MFlags         dw 0
Vers             dw 0
FileFormInfo     dw 0

GlobalVolume     db 0
InitSpeed        db 0
InitTempo        db 0
MasterVolume     db 0
UltraClick       db 0
DefaultPan       db 0

;**************************************************
;*** Funktion liest S3M und reserviert FMS etc. ***
;* Einsprung: far Pointer auf FILENAME
;* RÅckgabe:  ax=0=alles in Ordnung
;             sonst ax=FehlerCode

_LoadS3MF        proc near

sloads3m         struc
bp0              dw 0
ret0             dw 0
FNameOffset      dw 0
FNameSegment     dw 0
sloads3m         ends

loads3m          equ [bp - bp0]

                 push bp
                 mov bp,sp
                 pusha
                 push es
                 push ds

                 ;*** ôffne S3M-File ***

                 mov ax, loads3m.FNameSegment
                 mov ds,ax
                 mov dx, loads3m.FNameOffset

                 mov ah,3dh
                 mov al,10010000b
                 int 21h
                 jc notfound
                 mov Handle,ax

                 ;*** FMS Speicher allokieren ***

                 cmp FMSCheck,0
                 jnz @@noFMS

                 mov ah,42h  ;LSEEK
                 mov al,02   ;Ende des Files
                 mov bx,Handle
                 xor cx,cx
                 xor dx,dx
                 int 21h
                 jc notfound

                 shld dx,ax,6
                 ;mov dx,1024
                 call AllocXMS
                 or al,al
                 jz @@noFMS

                 mov FMSHandle,dx
                 call LockXMS
                 mov FMS32,eax

                 mov ax,65536/16 ;Speicher zum Puffern ins FMS
                 call GetMem
                 mov FMSBuffer,ax

                 ;*** Reserviere 60h Speicher fÅr S3M Kopf ***

                 mov ax,060h/16
                 call GetMem
                 mov S3MKopfSegment,ax

                 ;*** Lade S3M Kopf ***

                 mov bx,Handle

                 mov ax,4200h
                 xor cx,cx
                 xor dx,dx
                 int 21h

                 mov ds,S3MKopfSegment
                 mov dx,0              ;Offset vom Puffer
                 mov cx,060h           ;Zu lesende Bytes
                 mov ah,3Fh
                 int 21h
                 ;jc nomore

                 ;*** Ist es ein gÅltiges S3M ***

                 cmp byte ptr ds:[01ch],01ah
                 jnz nos3m
                 cmp byte ptr ds:[01dh],16 ;ST3 ?
                 jnz nos3m
                 cmp byte ptr ds:[02ch],'S'
                 jnz nos3m
                 cmp byte ptr ds:[02dh],'C'
                 jnz nos3m
                 cmp byte ptr ds:[02eh],'R'
                 jnz nos3m
                 cmp byte ptr ds:[02fh],'M'
                 jnz nos3m

                 ;*** Interpretiere S3M Kopf ***

                 mov ax,ds:[020h]
                 mov OrdNum,ax
                 mov ax,ds:[022h]
                 mov InsNum,ax
                 mov ax,ds:[024h]
                 mov PatNum,ax

                 mov ax,ds:[026h]
                 mov S3MFlags,ax
                 mov ax,ds:[028h]
                 mov Vers,ax
                 mov ax,ds:[02Ah]
                 mov FileFormInfo,ax

                 mov al,ds:[030h]
                 mov GlobalVolume,al
                 mov al,ds:[031h]
                 mov InitSpeed,al
                 mov al,ds:[032h]
                 mov InitTempo,al
                 mov al,ds:[033h]
                 mov MasterVolume,al

                 mov al,ds:[034h]
                 mov UltraClick,al
                 mov al,ds:[035h]
                 mov DefaultPan,al

                 mov cl,16
                 mov si,4fh
        DetectChannels:
                 cmp byte ptr ds:[si],128
                 jb endChanneldetect
                 dec si
                 dec cl
                 jnz DetectChannels
        endChanneldetect:
                  mov ChannelAnzahl,cl

                 ;***************************
                 ;*** Liest OrderList ein ***

                 ;*** Reserviert Speicher fÅr OrderList ***

                 mov ax,256/16
                 call GetMem
                 mov OrderListSeg,ax

                 ;*** LÑdt OrderList ***

                 mov bx,Handle
                 mov ds,OrderListSeg
                 mov dx,0              ;Offset vom Puffer
                 mov cx,OrdNum         ;Zu lesende Bytes
                 mov ah,3Fh
                 int 21h

                 ;*********************
                 ;*** Samples laden ***

                 ;*** SampleParapointers im S3M laden ***

                 mov ax,16 ;=128 Samples
                 call GetMem
                 mov SamplePointer,ax     ;Parapointers SampleHeaders im S3M

                 mov bx,Handle
                 mov ds,ax
                 mov dx,0              ;Offset vom Puffer
                 mov cx,InsNum         ;Zu lesende Bytes InsNum
                 shl cx,1              ;*2
                 mov ah,3Fh
                 int 21h

                 ;*** Schleife um Samples zu laden ***

                 mov ax,16 ;=128 SampleSegmente
                 call GetMem
                 mov SampleHeadPara,ax       ;Pointers zu SampleHeader in MEM
                 mov fs,ax
                 mov gs,SamplePointer

                 mov ax,512/16 ;=128 Sample Offset Pointer *4
                 call GetMem
                 mov SampleOffsetPara,ax     ;Para von OffsetSamples im FMS

                 mov edi,0
                 mov bp,InsNum
                 mov eax,FMS32
                 mov FMSPointer,eax

       LoadNextSample:

                     ;*** Sampleheader laden ***

                     mov ax,050h/16
                     call GetMem
                     mov fs:[edi*2],ax  ;Speichere Pointer von SampleHeader
                                        ;in Pointerarray : SampleHeadPara
                     mov bx,Handle
                     mov dx,gs:[edi*2]  ;Liest SampleHeaderParapointer
                     xor cx,cx
                     shld cx,dx,4       ;*16
                     shl dx,4
                     mov ax,04200h
                     int 21h           ;Dateipointer bewegen

                     mov bx,Handle
                     mov ds,fs:[edi*2]
                     mov dx,0            ;Offset vom Puffer
                     mov cx,050h         ;Zu lesende Bytes InsNum
                     mov ah,3Fh
                     int 21h             ;SampleHeader laden

                     cmp byte ptr ds:[0],1   ;wenn kein Digi-Sample
                     jnz noSampleDataLoad    ;nicht laden

                     ;*** SampleDaten laden ***

                     mov ax,ds:[10h]       ;wenn Sample LÑnge 0 Bytes
                     or ax,ax
                     jz noSampleDataLoad   ;dann kein "echtes" Sample

                     ;* Mit FMS *

                     mov bx,Handle
                     mov dx,ds:[0eh]   ;Liest SampleParapointer
                     xor cx,cx
                     shld cx,dx,4      ;*16
                     shl dx,4
                     mov ax,04200h
                     int 21h           ;Dateipointer bewegen

                     mov cx,ds:[10h]     ;Zu lesende Bytes SampleLength
                     mov bx,Handle
                     mov ds,FMSBuffer    ;Segment von FMSBuffer
                     mov dx,0            ;Offset vom FMSBuffer ist 0
                     mov ah,3Fh
                     int 21h             ;Sample in FMSBuffer laden

                ;*** Sample in FMS kopieren ***

                     push fs
                     mov ax,0
                     mov fs,ax
                     mov es,SampleOffsetPara
                     mov eax,FMSPointer
                     mov es:[edi*4],eax
                     xor edx,edx
                     ;mov cx,ds:[10h]
          @@CopyFMS:
                     mov bl,ds:[edx]
                     mov fs:[eax],bl
                     add edx,1
                     add eax,1
                     dec cx
                     jnz @@CopyFMS

                     mov FMSPointer,eax
                     pop fs

     noSampleDataLoad:

                 inc edi
                 dec bp
                 jnz LoadNextSample

                 ;*********************
                 ;*** Pattern laden ***

                 mov bx,Handle
                 mov dx,InsNum   ; Liest SampleParapointer
                 shl dx,1        ;60h+OrdNum+InsNum*2
                 add dx,OrdNum
                 add dx,060h
                 xor cx,cx
                 mov ax,04200h
                 int 21h           ;Dateipointer bewegen

                 mov ax,PatNum
                 shr ax,3
                 inc ax
                 mov bx,ax
                 call GetMem
                 mov PatternPointer,ax  ;Pointer auf Pattern im S3M
                 mov gs,ax

                 mov ax,bx
                 call GetMem
                 mov PatternSegment,ax  ;Pointer auf Pattern im MEM
                 mov fs,ax

                 mov cx,PatNum       ;Zu lesende Bytes PatternNum*2
                 shl cx,1
                 mov bx,Handle
                 mov ax,gs
                 mov ds,ax
                 mov dx,0            ;Offset vom Puffer
                 mov ah,3Fh
                 int 21h             ;PatternPointersS3M in MEM laden

                 ;*** Schleife um Pattern zu laden ***

                 mov edi,0
                 mov bp,PatNum

  LoadNextPattern:
                     mov ax,gs:[edi*2]   ;wenn PatternPointer=0
                     or ax,ax            ;dann nicht laden
                     jnz noClearPatternLoad
                         mov fs:[edi*2],ax
                         jmp noPatternLoad
  noClearPatternLoad:
                     mov bx,Handle
                     mov dx,ax         ; Liest PatternParapointer S3M
                     xor cx,cx
                     shld cx,dx,4      ;*16
                     shl dx,4
                     mov ax,04200h
                     int 21h           ;Dateipointer zeigt auf Pattern

                     mov cx,2          ;2 zu lesende Bytes von PatternlÑnge
                     mov bx,Handle
                     mov ax,SEG PatternLength     ;Segment von Puffer
                     mov ds,ax
                     mov dx,OFFSET PatternLength  ;Offset vom Puffer
                     mov ah,3Fh
                     int 21h                      ;PatternLength laden

                     mov ax,PatternLength
                     shr ax,4
                     inc ax
                     call GetMem
                     mov fs:[edi*2],ax

                     mov cx,PatternLength    ;lesende Bytes PatternlÑnge
                     mov bx,Handle
                     mov ds,ax
                     mov dx,0            ;Offset vom Puffer
                     mov ah,3Fh
                     int 21h             ;Pattern laden

  noPatternLoad:
                 inc edi
                 dec bp
                 jnz LoadNextPattern

                 ;*************************************
                 ;*** Schlie·e S3M File ohne Fehler ***

                 mov bx,Handle
                 mov ah,3Eh
                 int 21h

                 mov ax,FMSBuffer
                 call FreeMem

                 pop ds
                 pop es
                 popa
                 pop bp
                 mov al,0
                 ret

                 ;*** Schlie·t mit Fehlermeldung ab ***

notfound:                        ;*** S3M nicht gefunden
                 mov dx,FMSHandle
                 call UnlockXMS
                 mov dx,FMSHandle
                 call FreeXMS

                 pop ds
                 pop es
                 popa
                 pop bp
                 mov al,0ffh
                 ret

nos3m:                           ;*** File ist kein S3M
                 mov dx,FMSHandle
                 call UnlockXMS
                 mov dx,FMSHandle
                 call FreeXMS

                 call CloseS3M
                 pop ds
                 pop es
                 popa
                 pop bp
                 mov al,01h
                 ret

@@noFMS:
                 pop ds
                 pop es
                 popa
                 pop bp
                 mov al,02h
                 ret

_LoadS3MF        endp

;*** Cleant Speicher und Variablen von letztem S3M ***

_CleanS3M        proc near

                 push ax
                 push es
                 push esi
                 push bp

                 ;* Pattern frei machen *

                    xor esi,esi
                    mov bp,PatNum
                    mov fs,PatternSegment
                    mov gs,PatternPointer

      ClearNextPattern:
                    mov ax,gs:[esi*2]   ;Wenn kein NULL Pattern
                    or ax,ax
                    jz noClearPattern

                    mov ax,fs:[esi*2]   ;Befreie Speicher von Pattern
                    call FreeMem

      noClearPattern:
                    inc esi
                    dec bp
                    jnz ClearNextPattern

                 mov ax,PatternSegment
                 call FreeMem
                 mov ax,PatternPointer
                 call FreeMem

                 ;* Samples frei machen *

                    xor esi,esi
                    mov bp,InsNum
                    mov fs,SampleHeadPara

                    mov dx,FMSHandle
                    call UnlockXMS
                    mov dx,FMSHandle
                    call FreeXMS

    ClearNextSample:
                    mov ax,fs:[esi*2]
                    call FreeMem          ;Gibt SampleHeader MEM zurÅck

                    inc esi
                    dec bp
                    jnz ClearNextSample

                 mov ax,SampleOffsetPara  ;Gibt FMS Offsets zurÅck
                 call FreeMem
                 mov ax,SampleHeadPara    ;Gibt SampleHeaderPointer MEM zurÅck
                 call FreeMem
                 mov ax,SamplePointer     ;Gibt SampleHeaderPointer S3M zurÅck
                 call FreeMem

                 mov ax,OrderListSeg      ;Gibt OrderList zurÅck
                 call FreeMem
                 mov ax,S3MKopfSegment    ;Gibt S3MKopf MEM zurÅck
                 call FreeMem

                 pop bp
                 pop esi
                 pop es
                 pop ax
                 ret

_CleanS3M        endp

;*** Schlie·t S3M File ***

CloseS3m         proc near

                 mov bx,Handle
                 mov ah,3Eh
                 int 21h
                 ret

CloseS3M         endp


