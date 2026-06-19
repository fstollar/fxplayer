.AUTODEPEND

.PATH.obj = E:\DEMO\FX\FMS\

#		*Translator Definitions*
CC = c:\borlandc\bcc +FX.CFG
TASM = e:\asm\tasm.exe
TLIB = c:\borlandc\tlib
TLINK = c:\borlandc\tlink
LIBPATH = C:\BORLANDC\LIB
INCLUDEPATH = C:\BORLANDC\INCLUDE


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
 play.obj \
 fx.obj \
 str.obj

#		*Explicit Rules*
e:\demo\fx\fms\fx.exe: fx.cfg $(EXE_dependencies)
  $(TLINK) /v/x/c/3/P-/L$(LIBPATH) @&&|
c0h.obj+
e:\demo\fx\fms\fx.obj+
e:\demo\fx\fms\play.obj+
e:\demo\fx\fms\str.obj
e:\demo\fx\fms\fx.exe
		# no map file
emu.lib+
mathh.lib+
ch.lib
|


#		*Individual File Dependencies*
fx.obj: fx.cfg fx.c

#		*Compiler Configuration File*
fx.cfg: fx.mak
  copy &&|
-mh
-3
-a
-f-
-v
-G
-O
-Og
-Oe
-Om
-Ov
-Ol
-Ob
-Op
-Oi
-Z
-k-
-vi-
-wpro
-weas
-wpre
-nE:\DEMO\FX\FMS
-I$(INCLUDEPATH)
-L$(LIBPATH)
-P-.C
| fx.cfg


