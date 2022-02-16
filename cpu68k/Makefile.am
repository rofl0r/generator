## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = def68k head gen68k
BUILT_SOURCES = def68k-iibs.h def68k-funcs.h def68k-proto.h \
                cpu68k-0.c cpu68k-1.c cpu68k-2.c cpu68k-3.c \
                cpu68k-4.c cpu68k-5.c cpu68k-6.c cpu68k-7.c \
                cpu68k-8.c cpu68k-9.c cpu68k-a.c cpu68k-b.c \
                cpu68k-c.c cpu68k-d.c cpu68k-e.c cpu68k-f.c

## stage one - create iibs, funcs and proto header files from def68k.def

def68k_SOURCES = def68k.c tab68k.c
INCLUDES = -I../hdr -I.

head_SOURCES = def68k-iibs.h def68k-funcs.h def68k-proto.h

## see automake.info section on 'Support for executable extensions'

head$(EXEEXT):	def68k-iibs.h def68k-funcs.h def68k-proto.h

def68k-iibs.h:	def68k def68k.def
	./def68k
def68k-funcs.h:	def68k def68k.def
	./def68k
def68k-proto.h:	def68k def68k.def
	./def68k

## stage two - create cpu C files and create library

gen68k_SOURCES = gen68k.c tab68k.c
gen68k_DEPENDENCIES = def68k-iibs.h

noinst_LIBRARIES = lib68k.a
lib68k_a_SOURCES = cpu68k-0.c cpu68k-1.c cpu68k-2.c cpu68k-3.c \
                   cpu68k-4.c cpu68k-5.c cpu68k-6.c cpu68k-7.c \
                   cpu68k-8.c cpu68k-9.c cpu68k-a.c cpu68k-b.c \
                   cpu68k-c.c cpu68k-d.c cpu68k-e.c cpu68k-f.c
lib68k_a_DEPENDENCIES = def68k-iibs.h

MOSTLYCLEANFILES = ${lib68k_a_SOURCES} ${head_SOURCES}

cpu68k-0.c:	gen68k
	./gen68k
cpu68k-1.c:	gen68k
	./gen68k
cpu68k-2.c:	gen68k
	./gen68k
cpu68k-3.c:	gen68k
	./gen68k
cpu68k-4.c:	gen68k
	./gen68k
cpu68k-5.c:	gen68k
	./gen68k
cpu68k-6.c:	gen68k
	./gen68k
cpu68k-7.c:	gen68k
	./gen68k
cpu68k-8.c:	gen68k
	./gen68k
cpu68k-9.c:	gen68k
	./gen68k
cpu68k-a.c:	gen68k
	./gen68k
cpu68k-b.c:	gen68k
	./gen68k
cpu68k-c.c:	gen68k
	./gen68k
cpu68k-d.c:	gen68k
	./gen68k
cpu68k-e.c:	gen68k
	./gen68k
cpu68k-f.c:	gen68k
	./gen68k
