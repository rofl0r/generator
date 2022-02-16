## Process this file with automake to produce Makefile.in

INCLUDES = -I.

noinst_LIBRARIES = libraze.a
libraze_a_SOURCES = raze
libraze_a_LIBADD = raze.o

raze:	raze.o
raze.o:	raze.asm raze.reg
	nasm -e raze.asm -o raze2.asm
	nasm -f @ASMOUTPUT@ raze2.asm -o raze.o -praze.reg

DISTCLEANFILES = raze.asm raze2.asm
