.AUTODEPEND

.PATH.obj = E:\DEMO\FX\bak

#		*Translator Definitions*
CC = c:\borlandc\bcc +FX.CFG
TASM = e:\asm\tasm
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
e:\demo\fx\fx.exe: fx.cfg $(EXE_dependencies)
  $(TLINK) /v/x/c/P-/L$(LIBPATH) @&&|
c0h.obj+
e:\demo\fx\fx.obj+
e:\demo\fx\play.obj+
e:\demo\fx\str.obj
e:\demo\fx\fx.exe
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
-nE:\DEMO\FX
-I$(INCLUDEPATH)
-L$(LIBPATH)
-P-.C
| fx.cfg


