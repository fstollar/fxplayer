.AUTODEPEND

.PATH.obj = E:\DEMO\FX\PUBLIC

#		*Translator Definitions*
CC = c:\borlandc\bcc +FX.CFG
TASM = tasm
TLIB = tlib
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
e:\demo\fx\public\fx.exe: fx.cfg $(EXE_dependencies)
  $(TLINK) /x/c/P-/L$(LIBPATH) @&&|
c0c.obj+
e:\demo\fx\public\play.obj+
e:\demo\fx\public\fx.obj+
e:\demo\fx\public\str.obj
e:\demo\fx\public\fx
		# no map file
fp87.lib+
mathc.lib+
cc.lib
|


#		*Individual File Dependencies*
#		*Compiler Configuration File*
fx.cfg: fx.mak
  copy &&|
-mc
-3
-a
-f-
-O
-Oe
-Ob
-Z
-k-
-vi-
-wpro
-weas
-wpre
-nE:\DEMO\FX\PUBLIC
-I$(INCLUDEPATH)
-L$(LIBPATH)
-P-.C
| fx.cfg


