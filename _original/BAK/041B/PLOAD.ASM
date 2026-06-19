;****************** Loader v3.1 for F/X Player *******************
; v3.0 written on 31.12.96 at some lazy afternoon
; v3.1 written at 09.01.97 on some cold day
; (c) Apollo of STIGMA 1996/97

public           _SamplePrecalc
public           _LoadS3M_FMS
public           _CleanS3M_FMS

                 .code

;*** some useless data ***
FileHandle       dw 0  ; filehandle of S3M
FileSize         dw 0  ; size of module
FMSBuffer        dw 0  ; 64K buffer of base mem for loading and copying
FMSHandle        dw 0  ; FMS handle of S3M memory block
FMS32            dd ?  ; 32bit startadress of FMS memory block

;*** S3M pointers ***
HeaderPointer         dd ?  ; pointer to S3M header in FMS
OrderListPointer      dd ?  ; pointer to Orderlist in FMS

SamplePointerHandle   dw 0  ; handle of Pointertable for Samples
SampleHeadPointer     dd ?  ; 32bit pointer of SampleHeaderpointers FMS
SampleOffsetPointer   dd ?  ; 32bit pointer of Samplepointers in FMS

PatternPointerHandle  dw 0  ; handle of Pointertable of Patterns
PatternOffsetPointer  dd ?  ; 32bit pointer of Patternpointers in FMS

;*** S3M datas ***
OrdNum           dw ?
InsNum           dw ?
PatNum           dw ?
S3MFlags         dw ?
Vers             dw ?
FileFormInfo     dw ?

GlobalVolume     db ?
InitSpeed        db ?
InitTempo        db ?
MasterVolume     db ?
UltraClick       db ?
DefaultPan       db ?

;PROVISORISCH !!!!!

;*** precalc data ***
PreSampleHandle  dw 0
PreSamplePointer dd ?

_SamplePrecalc   proc far

                 pushad
                 push ds

                 xor ax,ax
                 mov ds,ax

                 mov dx,FileSize
                 shl dx,1
                 call AllocXMS              ; allocate FMS

                 mov PreSampleHandle,dx           ; save FMS Handle
                 call LockXMS                    ; and lock XMS
                 mov PreSamplePointer,eax              ; and save 32bit pointer

                 mov esi,eax

            ;********************
            ;*** read Samples ***

            ;*** loop to load Samples ***

                 mov cx,InsNum             ; move cx with instrument count
                 xor ebx,ebx
                 mov ebp,SampleHeadPointer ; point ebp to sampleheadpointers

 @@calcHeadPointers:
                 mov edi,ds:[ebp]
                 shl dword ptr ds:[edi+20h],1
                 add ebp,4                 ; increase pointer by 4
                 dec cx                    ; decrease counter
                 jnz @@calcHeadPointers    ; if not zero do more...


                 ;mov cx,InsNum
                 ;mov ebp,SampleHeadPointer
                 ;mov edi,SampleOffsetPointer

@@calcSamplePointer:
                 ;mov dword ptr es:[edi],0  ; set pointer to zero

                 ;mov ebx,es:[ebp]          ; load ebx with sampleheadpointer
                 ;cmp byte ptr es:[ebx],1   ; if no digital sample
                 ;jnz @@noSampleDataLoad    ; set 32bit pointer to zero...

                 ;cmp dword ptr es:[ebx+10h],0 ; if samplelength=0
                 ;jz  @@noSampleDataLoad    ; than stop it...

                 ;xor eax,eax               ; clear eax
                 ;mov al,es:[ebx+0dh]       ; load 8bit out of 24
                 ;shl eax,16                ; move it to 24bit
                 ;mov ax,es:[ebx+0eh]       ; load 16bit of 24bit parapointer
                 ;shl eax,4                 ; multiply by 16 for 32bit offset
                 ;add eax,esi               ; add 32bit FMS offset
                 ;mov es:[edi],eax          ; save 32bit sample offset in FMS

  @@noSampleDataLoad:
                 ;add ebp,4                 ; next sampleheader
                 ;add edi,4                 ; next samplepointer
                 ;dec cx                    ; the last ?
                 ;jnz @@calcSamplePointer   ; the same procedure as ever...

                 pop ds
                 popad

_SamplePrecalc   endp


_FreeSamplePrecalc  proc far

                 push ax
                 push esi
                 push bp
                 push es
                 push fs
                 push gs

                    mov dx,PreSampleHandle
                    or dx,dx
                    jz @@notyet1
                    call UnlockXMS
                    mov dx,PreSampleHandle
                    call FreeXMS
                    xor dx,dx
                    mov PreSampleHandle,dx
          @@notyet1:

                 pop gs
                 pop fs
                 pop es
                 pop bp
                 pop esi
                 pop ax
                 retf

_FreeSamplePrecalc  endp



;********************************************
;*** Function reads S3M and gets FMS etc. ***
;* entry: far pointer of FILENAME
;* return:  ax=0 = all ok
;                else ax=error code

_LoadS3M_FMS     proc far

sloads3m         struc
bpS3M            dw 0
retS3M           dd 0
FNameOffset      dw 0
FNameSegment     dw 0
sloads3m         ends

loads3m          equ [bp - bpS3M]   ; set bp to structure above

                 push bp
                 mov bp,sp
                 pushad
                 push ds
                 push es
                 push fs
                 push gs

            ;*** open S3M File ***

                 mov ax, loads3m.FNameSegment
                 mov ds,ax
                 mov dx, loads3m.FNameOffset

                 mov ah,3dh                 ; open S3M file
                 mov al,10010000b
                 int 21h
                 jc @@notfound              ; if not found ...
                 mov FileHandle,ax          ; save filehandle

            ;*** allokate FMS memory ***

                 cmp FMSCheck,0             ; is FMS available
                 jnz @@noFMS                ; if not ...

                 mov ah,42h                 ; LSEEK
                 mov al,02                  ; end of S3M File
                 mov bx,FileHandle
                 xor cx,cx
                 xor dx,dx
                 int 21h                    ; save in dx:ax length of S3M
                 jc @@notfound              ; if not found ...

                 shld dx,ax,6               ; S3M in KB
                 and dx,1111111111000000b   ; round down to next 64K
                 add dx,64                  ; and add 64K
                 mov FileSize,dx             ; save File size
                 call AllocXMS              ; allocate FMS
                 or al,al                   ; if error
                 jz @@noFMS                 ; no FMS memory got....

                 mov FMSHandle,dx           ; save FMS Handle
                 call LockXMS               ; and lock XMS
                 mov FMS32,eax              ; and save 32bit pointer

                 mov dx,4                   ; and 4kb of mem for pointers
                 call AllocXMS              ; allocate XMS
                 or al,al                   ; on error
                 jz @@noFMS                 ; no chance...
                 mov SamplePointerHandle,dx ; save Handle
                 call LockXMS               ; lock XMS
                 mov SampleHeadPointer,eax  ; write 32bit offset
                 add eax,256*4              ; add 1024 (256 samples)
                 mov SampleOffsetPointer,eax; and save it too

                 mov ah,42h                 ; LSEEK
                 mov al,00                  ; begin of S3M File
                 mov bx,FileHandle
                 xor cx,cx
                 xor dx,dx
                 int 21h                    ; now the filepointer is at start
                 jc @@notfound              ; if not found ...

                 mov ax,65536/16            ; memory to copy from base to FMS
                 call GetMem
                 mov FMSBuffer,ax           ; save Handle

            ;*** Load full S3M into FMS ***

                 mov ds,ax                  ; load ds with buffer
                 xor ax,ax
                 mov es,ax                  ; load es with 0 for FMS
                 mov edi,FMS32              ; load edi with FMS 32bit pointer
                 mov cx,64000               ; and load 64000 bytes blocks

      @@loadS3M:
                 mov bx,FileHandle   ; mov bx with filehandle
                 mov dx,0            ; offset of buffer is 0  :-)
                 mov ah,3Fh          ; load file via handle
                 int 21h             ; and energy.....
                 jc @@noS3M          ; if carry set error occoured...

                 xor si,si           ; offset in buffer is always zero
                 cmp ax,cx           ; if ax=cx (loadlength=64000)
                 jnz @@copyS3Mend    ; if not it is the end of S3M...

       @@copyS3M:
                 mov ebx,ds:[si]     ; move 4 bytes at once...
                 add si,4            ; increase pointer by four
                 mov es:[edi],ebx    ; and write into FMS
                 add edi,4           ; increase FMS pointer
                 sub ax,4            ; is buffer already copied ?
                 jnz @@copyS3M       ; if not do more....

                 jmp @@loadS3M       ; and load next block...

       @@copyS3Mend:
                 mov ebx,ds:[si]     ; move 4 bytes at once...
                 add si,4            ; increase pointer by four
                 mov es:[edi],ebx    ; write dword into FMS
                 add edi,4           ; increase FMS pointer
                 sub ax,4            ; is buffer ready ?
                 jnc @@copyS3Mend    ; copy block to end...

                 ; and now S3M is completly in FMS

            ;*** check if it is a valid S3M ***

                 mov esi,FMS32        ; move esi with FMS 32bit pointer

                 cmp byte ptr es:[esi+01ch],01ah
                 jnz @@noS3M                          ; no S3M ...
                 cmp byte ptr es:[esi+01dh],16 ;ST3 ?
                 jnz @@noS3M                          ; no S3M ...
                 cmp dword ptr es:[esi+02ch],'MRCS'   ; if string 'SCRM'
                 jnz @@noS3M                          ; if not ...

            ;*** interpret S3M header ***

                 mov ax,es:[esi+020h]   ; mov ax with OrdNum
                 cmp ax,2               ; if it is less than 3
                 jb @@noS3M             ; it is no valid S3M
                 mov OrdNum,ax
                 mov al,es:[esi+60h]    ; mov al with first pattern order
                 cmp al,255             ; compare with 255
                 jnb @@noS3M            ; if it is not below then shit...
                 mov ax,es:[esi+022h]   ;* Load all stuff from header
                 mov InsNum,ax
                 mov ax,es:[esi+024h]
                 mov PatNum,ax

                 mov ax,es:[esi+026h]
                 mov S3MFlags,ax
                 mov ax,es:[esi+028h]
                 mov Vers,ax
                 mov ax,es:[esi+02Ah]
                 mov FileFormInfo,ax

                 mov al,es:[esi+030h]
                 mov GlobalVolume,al
                 mov al,es:[esi+031h]
                 mov InitSpeed,al
                 mov al,es:[esi+032h]
                 mov InitTempo,al
                 mov al,es:[esi+033h]
                 mov MasterVolume,al

                 mov al,es:[esi+034h]
                 mov UltraClick,al
                 mov al,es:[esi+035h]
                 mov DefaultPan,al

              ;* check how many channels are used *

                 mov ecx,16                     ; check all 16 channels
                 mov ChannelAnzahl,cl           ; save active channels
                 mov edi,40h                    ; at adress 4Fh

        @@CopyChannels:
                 mov al,es:[esi+edi]           ; load channel settings
                 mov ChannelFlag+[edi-40h],al  ; and save them
                 cmp al,16
                 jnb @@noLastChannel
                     mov eax,edi
                     sub ax,3Fh
                     mov ChannelAnzahl,al
   @@noLastChannel:
                 inc edi                       ; else increase pointer
                 dec cl                        ; and decrease channel
                 jnz @@CopyChannels            ; check next channel...

            ;* default pannings for stereo *

                 test MasterVolume,128
                 jz @@noStereoPanning

                    mov ecx,16
                    xor edx,edx

        @@nextStereo:
                     mov al,ChannelFlag+[edx]
                     mov ah,3
                     test al,32
                     jnz @@noStereo

                       test al,8
                       jz @@noRightChannel

                          add ah,0Ch-03h
           @@noRightChannel:
                       mov ChannelPanning+[edx],ah

         @@noStereo:
                    inc edx
                    dec ecx
                    jnz @@nextStereo

  @@noStereoPanning:

            ;* load pannings if available *

                 cmp DefaultPan,252     ; e.g. (FC) hex :) seems to be a
                 jnz @@noPanLoad        ; Future Crew joke ;)

                   movzx ebx,OrdNum
                   movzx eax,InsNum
                   add ebx,eax
                   add ebx,eax
                   movzx eax,PatNum
                   add ebx,eax
                   add ebx,eax
                   add ebx,60h

                   mov ecx,16
                   xor edx,edx
       @@LoadPan:
                   mov al,es:[esi+ebx]
                   test al,32
                   jz @@nextPan

                      and al,15
                      mov ChannelPanning+[edx],al
       @@nextPan:
                   inc ebx
                   inc edx
                   dec ecx
                   jnz @@LoadPan

     @@noPanLoad:

            ;**********************
            ;*** read OrderList ***

                 mov edi,esi               ; mov edi with FMS start
                 add edi,60h               ; add 60h
                 mov OrderListPointer,edi  ; write pointer :)

            ;********************
            ;*** read Samples ***

            ;*** loop to load Samples ***

                 mov cx,InsNum             ; move cx with instrument count
                 mov edi,OrderListPointer  ; point edi to instrument pointers
                 movzx eax,OrdNum
                 add edi,eax

                 mov ebp,SampleHeadPointer ; point ebp to sampleheadpointers
 @@calcHeadPointers:
                 movzx eax,word ptr es:[edi] ; load eax with parapointer
                 shl eax,4                 ; and multiply with 16
                 add eax,esi               ; and add eax with 32bit FMS pointer
                 mov es:[ebp],eax          ; and save 32bit pointer of head
                 add edi,2                 ; inc pointer by 2
                 add ebp,4                 ; increase pointer by 4
                 dec cx                    ; decrease counter
                 jnz @@calcHeadPointers    ; if not zero do more...


                 mov cx,InsNum
                 mov ebp,SampleHeadPointer
                 mov edi,SampleOffsetPointer

@@calcSamplePointer:
                 mov dword ptr es:[edi],0  ; set pointer to zero

                 mov ebx,es:[ebp]          ; load ebx with sampleheadpointer
                 cmp byte ptr es:[ebx],1   ; if no digital sample
                 jnz @@noSampleDataLoad    ; set 32bit pointer to zero...

                 cmp dword ptr es:[ebx+10h],0 ; if samplelength=0
                 jz  @@noSampleDataLoad    ; than stop it...

                 xor eax,eax               ; clear eax
                 mov al,es:[ebx+0dh]       ; load 8bit out of 24
                 shl eax,16                ; move it to 24bit
                 mov ax,es:[ebx+0eh]       ; load 16bit of 24bit parapointer
                 shl eax,4                 ; multiply by 16 for 32bit offset
                 add eax,esi               ; add 32bit FMS offset
                 mov es:[edi],eax          ; save 32bit sample offset in FMS

  @@noSampleDataLoad:
                 add ebp,4                 ; next sampleheader
                 add edi,4                 ; next samplepointer
                 dec cx                    ; the last ?
                 jnz @@calcSamplePointer   ; the same procedure as ever...


            ;********************
            ;*** Pattern load ***

                 mov dx,PatNum             ; look up Pattern number
                 shl dx,2                  ; mul 4
                 shr dx,10                 ; div 1024 = KB
                 inc dx                    ; and one savety KB
                 call AllocXMS
                 mov PatternPointerHandle,dx
                 call LockXMS
                 mov PatternOffsetPointer,eax

            ;*** loop to load Patterns ***

                 mov ebp,eax               ; load ebp with PatternPointertable
                 mov edi,OrderListPointer  ; load edi with OrdNum
                 movzx eax,OrdNum
                 add edi,eax
                 movzx eax,InsNum          ; load eax with InsNum
                 shl eax,1                 ; InsNum * 2
                 add edi,eax               ; and add to edi to patternpointer
                 mov cx,PatNum             ; load cx with pattern counter

  @@calcPatternPointer:
                     mov dword ptr es:[ebp],0 ; load patternpointer with 0

                     movzx eax,word ptr es:[edi] ; load eax with parapointer
                     or eax,eax               ; if it is zero
                     jz @@noPatternLoad       ; it is no true pattern...

                     shl eax,4                ; mul by 16
                     add eax,esi              ; and add 32bit FMS pointer
                     mov es:[ebp],eax         ; save pointer in table
  @@noPatternLoad:
                     add edi,2                ; add parapointer_pointer by 2
                     add ebp,4                ; add patternpointer by 4
                     dec cx                   ; is it last pattern ?
                     jnz @@calcPatternPointer ; if not do again...


                 ;************************************
                 ;*** close S3M File without error ***

                 call CloseS3M

                 mov ax,FMSBuffer
                 call FreeMem

                 ;call _SamplePrecalc

                 pop gs
                 pop fs
                 pop es
                 pop ds
                 popad
                 pop bp
                 mov al,0  ; no error occoured
                 retf       ; back to live... :)

                 ;*** returns with error code ***

@@notfound:                         ;*** S3M not found

                 call _CleanS3M_FMS
                 call CloseS3M

                 pop gs
                 pop fs
                 pop es
                 pop ds
                 popad
                 pop bp
                 mov al,0ffh
                 retf

@@noS3M:                            ;*** File is no valid S3M

                 call _CleanS3M_FMS
                 call CloseS3M

                 pop gs
                 pop fs
                 pop es
                 pop ds
                 popad
                 pop bp
                 mov al,01h
                 retf

@@noFMS:                            ;*** no FMS available
                 call _CleanS3M_FMS
                 call CloseS3M

                 pop gs
                 pop fs
                 pop es
                 pop ds
                 popad
                 pop bp
                 mov al,02h
                 retf

_LoadS3M_FMS     endp

;*** Clean Memorys und Variables of the last S3M ***

_CleanS3M_FMS    proc far

                 push ax
                 push esi
                 push bp
                 push es
                 push fs
                 push gs

                    mov dx,FMSHandle
                    or dx,dx
                    jz @@notyet1
                    call UnlockXMS
                    mov dx,FMSHandle
                    call FreeXMS
                    xor dx,dx
                    mov FMSHandle,dx
          @@notyet1:
                    mov dx,SamplePointerHandle
                    or dx,dx
                    jz @@notyet2
                    call UnlockXMS
                    mov dx,SamplePointerHandle
                    call FreeXMS
                    xor dx,dx
                    mov SamplePointerHandle,dx
          @@notyet2:
                    mov dx,PatternPointerHandle
                    or dx,dx
                    jz @@notyet3
                    call UnlockXMS
                    mov dx,PatternPointerHandle
                    call FreeXMS
                    xor dx,dx
                    mov PatternPointerHandle,dx
          @@notyet3:

                 ;call _FreeSamplePrecalc

                 pop gs
                 pop fs
                 pop es
                 pop bp
                 pop esi
                 pop ax
                 retf

_CleanS3M_FMS    endp

;*** close S3M File ***

CloseS3M         proc near

                 mov bx,FileHandle
                 or bx,bx
                 jz @@notyet

                 mov ah,3Eh
                 int 21h
                 xor bx,bx
                 mov FileHandle,bx
        @@notyet:
                 ret

CloseS3M         endp


