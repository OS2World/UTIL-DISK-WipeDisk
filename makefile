
wipeDisk.exe: $(*B).def $(*B).obj makefile
   link /A:4 $(*B),$*,nul,os2,$(*B).def;

wipeDisk.obj:  $(*B).c
   cl /c /J /AC /W4 /Zi /Ox /G2cs $(*B).c

