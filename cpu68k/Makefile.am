## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = def68k gen68k

## stage one - create iibs, funcs and proto header files from def68k.def

def68k_SOURCES = def68k.c tab68k.c
AM_CPPFLAGS = -I../hdr -I.

DEF_HDRS = def68k-iibs.h def68k-funcs.h def68k-proto.h

$(DEF_HDRS): def68k.stamp

def68k.stamp: def68k$(EXEEXT) def68k.def
	$(AM_V_GEN)./def68k$(EXEEXT)
	touch def68k.stamp

## stage two - create cpu C files and create library

CPU_SOURCE = cpu68k-0.c cpu68k-1.c cpu68k-2.c cpu68k-3.c \
             cpu68k-4.c cpu68k-5.c cpu68k-6.c cpu68k-7.c \
             cpu68k-8.c cpu68k-9.c cpu68k-a.c cpu68k-b.c \
             cpu68k-c.c cpu68k-d.c cpu68k-e.c cpu68k-f.c


gen68k_SOURCES = gen68k.c tab68k.c
gen68k_DEPENDENCIES = $(DEF_HDRS)

cpu68k.c gen68k.c: $(DEF_HDRS)

noinst_LIBRARIES = lib68k.a
lib68k_a_SOURCES = $(CPU_SOURCE)
lib68k_a_DEPENDENCIES = $(DEF_HDRS)

MOSTLYCLEANFILES = ${lib68k_a_SOURCES} ${head_SOURCES}

$(CPU_SOURCE): gen68k.stamp

gen68k.stamp: gen68k$(EXEEXT)
	./gen68k$(EXEEXT)
	touch $@

