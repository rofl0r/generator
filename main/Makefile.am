## Process this file with automake to produce Makefile.in

INCLUDES = -I../hdr -I. -I../cpu68k -I../ym2612 -I../raze -I../cmz80 \
           -I../sn76496 -I../gtkopts \
           -DFNAME_TCLSCRIPT=\"${datadir}/generator/gen.tcl\" \
           @GTK_CFLAGS@ @SDL_CFLAGS@
bin_PROGRAMS = @PROGRAM@
EXTRA_PROGRAMS = generator-svgalib generator-tcltk generator-allegro \
                 generator-gtk generator-sdl
LDADD = ../cpu68k/lib68k.a ../ym2612/libym2612.a @MY_LIBS@ \
        ../sn76496/libsn76496.a

## this should be generator_tcltk_DATA but it won't let me do that
## pkgdata_DATA = gen.tcl

generator_svgalib_SOURCES = cpu68k.c mem68k.c reg68k.c event.c \
    logo.c font.c generator.c state.c cpuz80.c vdp.c gensound.c \
    memz80.c ui-console.c uip-svgalib.c uiplot.c gensoundp-unix.c
generator_svgalib_LDADD = ${LDADD} -lvga

generator_tcltk_SOURCES = cpu68k.c mem68k.c reg68k.c event.c \
    generator.c state.c cpuz80.c vdp.c gensound.c memz80.c \
    ui-tcltk.c diss68k.c gensoundp-unix.c
generator_tcltk_LDADD = ${LDADD} -ltk -ltcl ../cpu68k/tab68k.o

generator_allegro_SOURCES = cpu68k.c mem68k.c reg68k.c event.c \
    logo.c font.c generator.c state.c cpuz80.c vdp.c gensound.c \
    memz80.c ui-console.c uip-allegro.c uiplot.c gensoundp-allegro.c
generator_allegro_LDADD = ${LDADD} -lalleg

generator_gtk_SOURCES = cpu68k.c mem68k.c reg68k.c event.c \
    generator.c state.c cpuz80.c vdp.c gensound.c memz80.c \
    ui-gtk.c uiplot.c gensoundp-unix.c initcart.c patch.c \
    dib.c avi.c diss68k.c
generator_gtk_LDADD = ${LDADD} @GTK_LIBS@ @SDL_LIBS@ ../glade/libglade.a ../gtkopts/libgtkopts.a ../cpu68k/tab68k.o

generator_sdl_SOURCES = cpu68k.c mem68k.c reg68k.c event.c \
    generator.c state.c cpuz80.c vdp.c gensound.c memz80.c \
    ui-sdl.c uiplot.c gensoundp-unix.c initcart.c patch.c \
    dib.c avi.c diss68k.c
generator_sdl_LDADD = ${LDADD} @SDL_LIBS@ \
    ../gtkopts/libgtkopts.a ../cpu68k/tab68k.o

