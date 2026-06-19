!ifndef dest
!error no destination defined!
!endif

!ifeq dest
!error no destination defined!
!endif

!ifndef objects
!error no objects defined!
!endif

!ifeq objects
!error no objects defined!
!endif

!ifndef showcmd
.silent
!endif

!ifdef debugdefault
main=debug
!else
main=nodebug
!endif

!ifndef desttype
desttype=exe
!endif

!ifeq desttype
desttype=exe
!endif

!ifdef model16
_386=
!else
_386=386
!endif

!ifndef lsys
!ifeq desttype dll
lsys=dos4gdll
!else
lsys=pmodew
!endif
!endif

!ifndef copt
copt=/ontrabmiell+ /oh /oi /zp8 /5r /fp5
!endif

!ifndef aopt
aopt=/ml /m5
!endif

!ifndef cdebopt
cdebopt = /d2
!endif

!ifndef adebopt
adebopt = /zd
!endif

!ifndef ldebopt
ldebopt = debug all
!endif

start: .symbolic
  %make $(main)

all: .symbolic $(extradepend)
  %make $(destpath)$(dest).$(desttype)

debug: .symbolic
  set coptdb=$(cdebopt)
  set aoptdb=$(adebopt)
  set loptdb=$(ldebopt)
  %make all

nodebug: .symbolic
  set coptdb=
  set aoptdb=
  set loptdb=
  %make all

cleantmp: .symbolic
  if exist tasm.cfg del tasm.cfg
  if exist wcc.cfg del wcc.cfg
  if exist $(dest).map del $(dest).map
  if exist $(dest).lnk del $(dest).lnk
  if exist $(dest).lln del $(dest).lln
  if exist $(dest).smp del $(dest).smp
  if exist *.err del *.err

clean: .symbolic
  %make cleantmp
  for %i in ($objects) do if exist %i del %i

cleanbak: .symbolic
  if exist *.bak del *.bak

cleandest: .symbolic
  %make clean
  if exist $(destpath)$(dest).$(desttype) del $(destpath)$(dest).$(desttype)
!ifeq desttype dll
  if exist $(implibpath)$(dest).lib del $(implibpath)$(dest).lib
!endif

cleanall: .symbolic
  %make cleandest cleanbak

new: .symbolic
  %make cleandest
  %make start

debugnew: .symbolic
  %make cleandest
  %make debug

nodebugnew: .symbolic
  %make cleandest
  %make nodebug

$(destpath)$(dest).exe: $(objects) $(dest).lnk $(extradestdepend)
  wlink $(%loptdb) @$(dest).lnk

$(destpath)$(dest).dll: $(objects) $(dest).lnk $(extradestdepend)
  wlink $(%loptdb) @$(dest).lnk
!ifdef implib
  wlib /b /n $(implibpath)$(dest).lib +$(destpath)$(dest).dll
!endif

$(destpath)$(dest).lib: $(objects) $(dest).lln $(extradestdepend)
  wlib @$(dest).lln

$(dest).lnk: makefile
  %create $^@
!ifeq lsys dos4gdll
  %append $^@ system begin dos4gdll
  %append $^@   option osname='DOS/4G'
  %append $^@   libpath %WATCOM%\lib386
  %append $^@   libpath %WATCOM%\lib386\dos
  %append $^@   library dosdll.lib
  %append $^@   reference _dllstart_
  %append $^@   format os2 le dll
  %append $^@ end
!endif
  %append $^@ system $(lsys)
  %append $^@ option caseexact
!ifdef lopt
!ifneq lopt
  %append $^@ option { $(lopt) }
!endif
!endif
!ifdef libraries
!ifneq libraries
  %append $^@ library { $(libraries) }
!endif
!endif
!ifdef libpath
!ifneq libpath
  %append $^@ libpath $(libpath)
!endif
!endif
  %append $^@ name $(destpath)$(dest).$(desttype)
  %append $^@ file { $(objects) }
  %append $^@ $(lcmd)
  %append $^@ option map=$(dest).map
!ifeq desttype dll
  %append $^@ option stack=0
!else
!ifdef stack
  %append $^@ option stack=$(stack)
!endif
!endif

$(dest).lln: makefile
  %create $^@
  %append $^@ $(libopt) /b /n $(destpath)$(dest).lib
  for %i in ($objects) do %append $(dest).lln +%i

wcc.cfg: makefile
  %write $^@ $(copt) /i=$(hpath);$(cpath);$(libpath)

tasm.cfg: makefile
  %write $^@ $(aopt) $(incpathpar)

.c: $(cpath)
.c.obj:
  %make wcc.cfg
  if exist $*.obj del $*.obj
  wpp$(_386) @wcc.cfg $< $(%coptdb)

.cpp: $(cpath)
.cpp.obj:
  %make wcc.cfg
  if exist $*.obj del $*.obj
  wpp$(_386) @wcc.cfg $(%coptdb) $<

.asm: $(asmpath)
.asm.obj:
  %make tasm.cfg
  if exist $*.obj del $*.obj
  $(%tasmpath)\tasm @tasm.cfg $(%aoptdb) $<

!ifndef dest
!error no destination defined!
!endif

!ifeq dest
!error no destination defined!
!endif

