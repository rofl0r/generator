## Process this file with automake to produce Makefile.in

INCLUDES = -I../hdr -I. -I../cpu68k -I../ym2612 -I../raze -DFNAME_TCLSCRIPT=\"${datadir}/generator/gen.tcl\"
bin_PROGRAMS = @PROGRAM@
EXTRA_PROGRAMS = generator-svgalib generator-tcltk generator-allegro
LDADD = ../cpu68k/lib68k.a ../ym2612/libym2612.a ../raze/libraze.a

## this should be generator_tcltk_DATA but it won't let me do that
pkgdata_DATA = gen.tcl

generator_svgalib_SOURCES = cpu68k.c mem68k.c reg68k.c event.c \
    logo.c font.c generator.c state.c cpuz80.c vdp.c gensoundi.c \
    memz80.c snprintf.c \
    ui-console.c uip-svgalib.c gensound.c
generator_svgalib_LDADD = ${LDADD} -lvga

generator_tcltk_SOURCES = cpu68k.c mem68k.c reg68k.c event.c \
    logo.c font.c generator.c state.c cpuz80.c vdp.c gensoundi.c \
    memz80.c snprintf.c \
    ui-tcltk.c diss68k.c gensound.c
generator_tcltk_LDADD = ${LDADD} -ltk -ltcl ../cpu68k/tab68k.o

generator_allegro_SOURCES = cpu68k.c mem68k.c reg68k.c event.c \
    logo.c font.c generator.c state.c cpuz80.c vdp.c gensoundi.c \
    memz80.c snprintf.c \
    ui-console.c uip-allegro.c gensound-allegro.c
generator_allegro_LDADD = ${LDADD}

