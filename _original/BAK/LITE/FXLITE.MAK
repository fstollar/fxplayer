.AUTODEPEND

.PATH.obj = E:\DEMO\FX\LITE

#		*Translator Definitions*
CC = C:\BORLANDC\bcc +FXLITE.CFG
TASM = TASM
TLIB = C:\BORLANDC\tlib
TLINK = C:\BORLANDC\tlink
LIBPATH = C:\BORLANDC\LIB
INCLUDEPATH = C:\BORLANDC\INCLUDE;C:\BORLANDC


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
 fxlite.obj \
 play.obj

#		*Explicit Rules*
E:\demo\fx\lite\fxlite.exe: fxlite.cfg $(EXE_dependencies)
  $(TLINK) /x/c/P-/L$(LIBPATH) @&&|
c0c.obj+
e:\demo\fx\lite\fxlite.obj+
e:\demo\fx\lite\play.obj
e:\demo\fx\lite\fxlite
		# no map file
cc.lib
|


#		*Individual File Dependencies*
fx.obj: fxlite.cfg fx.c

#		*Compiler Configuration File*
fxlite.cfg: fxlite.mak
  copy &&|
-mc
-3
-f-
-ff-
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
-nE:\demo\fx\lite
-I$(INCLUDEPATH)
-L$(LIBPATH)
| fxlite.cfg


