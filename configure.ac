dnl Generator configure.in file - process with autoconf

  AC_INIT
AC_CONFIG_SRCDIR([cpu68k/def68k.c])
  AC_PREREQ(2.59)
  AC_CANONICAL_TARGET([])
  AM_INIT_AUTOMAKE(generator, 0.35)
  AC_CONFIG_HEADERS([config.h])
  optimum=yes

dnl See what version we're compiling

  MY_DIST_SUBDIRS="cpu68k raze cmz80 ym2612 sn76496 main glade gtkopts"
  MY_SUBDIRS="cpu68k ym2612 sn76496 main"
  MY_LIBS=""
  UNDERSCORE=""
  ASMOUTPUT=elf
  USE_SDL_AUDIO=no
  AC_ARG_WITH(coffasm,
              AS_HELP_STRING(--with-coffasm,Assemble coff files),
              ASMOUTPUT=coff; UNDERSCORE="_")
  AC_ARG_WITH(svgalib,
              AS_HELP_STRING(--with-svgalib,Create svgalib version),
              MY_PROG="generator-svgalib")
  AC_ARG_WITH(allegro,
              AS_HELP_STRING(--with-allegro,Create allegro version),
              MY_PROG="generator-allegro")
  AC_ARG_WITH(tcltk,
              AS_HELP_STRING(--with-tcltk,Create tcltk version),
              MY_PROG="generator-tcltk")
  AC_ARG_WITH(gtk,
              AS_HELP_STRING(--with-gtk,Create gtk version),
              MY_PROG="generator-gtk")
  AC_ARG_WITH(sdl,
              AS_HELP_STRING(--with-sdl,Create SDL version),
              [MY_PROG="generator-sdl"; USE_SDL_AUDIO=yes])
   if [[ "x$MY_PROG" = "x" ]]; then
    AC_MSG_ERROR(You must select a user interface type:
    svgalib allegro tcltk gtk sdl)
  fi
  if [[ "x$MY_PROG" = "xgenerator-allegro" ]]; then
    AC_DEFINE(ALLEGRO, 1, [Allegro version])
    PROGRAM='generator-allegro$(EXEEXT)'
  else
    PROGRAM=$MY_PROG
  fi
  AC_ARG_WITH(raze,
              AS_HELP_STRING(--with-raze,Use RAZE z80 emulation),
              Z80="raze")
  AC_ARG_WITH(cmz80,
              AS_HELP_STRING(--with-cmz80,Use C mz80 z80 emulation),
              Z80="cmz80")
  if [[ "x$Z80" = "xraze" ]]; then
    MY_SUBDIRS="raze $MY_SUBDIRS"
    MY_LIBS="../raze/libraze.a $MY_LIBS"
    AC_DEFINE(RAZE, 1, [Define to use RAZE z80 emulation])
  elif [[ "x$Z80" = "xcmz80" ]]; then
    MY_SUBDIRS="cmz80 $MY_SUBDIRS"
    MY_LIBS="../cmz80/libcmz80.a $MY_LIBS"
    AC_DEFINE(CMZ80, 1, [Define to use the portable C mz80 z80 emulation])
  else
    AC_MSG_ERROR(You must select a z80 emulator: raze cmz80)
  fi
  AC_DEFINE(DIRECTRAM, 1, [Define for direct ram writes (faster)])
  AC_SUBST(PROGRAM)
  AC_SUBST(ASMOUTPUT)
  AC_SUBST(UNDERSCORE)
  AC_SUBST(MY_LIBS)
  AC_SUBST(MY_SUBDIRS)
  AC_SUBST(MY_DIST_SUBDIRS)

dnl Check for processor.

  case $target_cpu in
    i?86)  AC_MSG_RESULT(Turning on x86 processor optimisations)
           AC_DEFINE(PROCESSOR_INTEL, 1, [Define if you have an x86 processor])
           ALIGNLONGS=1
           ;;
    sparc) AC_MSG_RESULT(Turning on sparc processor optimisations)
           AC_DEFINE(PROCESSOR_SPARC, 1, [Define if you have a sparc processor])
           ALIGNLONGS=0
           ;;
    arm)   AC_MSG_RESULT(Turning on arm processor optimisations)
           AC_DEFINE(PROCESSOR_ARM, 1, [Define if you have an ARM processor])
           ALIGNLONGS=1
           ;;
    *)     AC_MSG_WARN(Processor type not known - processor optimisations off!)
           ALIGNLONGS=1
           optimum=no
           ;;
  esac
  
  AC_DEFINE(ALIGNLONGS, ALIGNLONGS,
            [Undefine if your processor can read unaligned 32-bit values])

dnl Check for gcc options

  GCCVER=yes
  WARN=no
  DEBUG=no
  AC_ARG_WITH(nologging, AS_HELP_STRING(--with-nologging,Turn off all logging),
              AC_DEFINE(NOLOGGING, 1, [Define to turn off all logging]))
  AC_ARG_WITH(warnings, AS_HELP_STRING(--with-warnings,Turn on gcc warnings),
              WARN=$withval, WARN=no)
  AC_ARG_WITH(gcc, AS_HELP_STRING(--with-gcc@<:@=VER@:>@,GCC optimisation setting),
              GCCVER=$withval)
  AC_ARG_WITH(raze, AS_HELP_STRING(--with-raze,Use raze z80 emulation),
              RAZE=$withval, DEBUG=no)
  AC_ARG_WITH(debug, AS_HELP_STRING(--with-debug,Turn debugging options on),
              DEBUG=$withval, DEBUG=no)
  AC_ARG_WITH(sdl-audio, AS_HELP_STRING(--with-sdl-audio,Use SDL for audio),
              [USE_SDL_AUDIO=$withval],)

# Check soundcard header

  AC_CHECK_HEADER([soundcard.h],
    [AC_DEFINE(HAVE_SOUNDCARD_H, 1, [Define if you have soundcard.h])])
  AC_CHECK_HEADER([sys/soundcard.h],
    [AC_DEFINE(HAVE_SYS_SOUNDCARD_H, 1, [Defined if you have sys/soundcard.h])])

  if [[ "x$USE_SDL_AUDIO" != "xno" ]]; then
    AC_DEFINE(USE_SDL_AUDIO, 1, [Define to use SDL for audio])
  else
dnl At least NetBSD and OpenBSD need -lossaudio, if the interface is used

    AC_MSG_CHECKING([whether libossaudio is necessary])
    OLD_LIBS="$LIBS"

    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/ioctl.h>
#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#else
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif
#endif
]], [[
int main(int argc, char *argv[])
{
  int fd = 0;

  ioctl(fd, SNDCTL_DSP_SETFRAGMENT, (char *)0);
  return 0;
}]])],[NEED_LIBOSSAUDIO=no],[LIBS="$LIBS -lossaudio"; AC_TRY_LINK([
#include <sys/ioctl.h>
#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#else
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif
#endif
],
[
int main(int argc, char *argv[])
{
  int fd = 0;

  ioctl(fd, SNDCTL_DSP_SETFRAGMENT, (char *)0);
  return 0;
}],
      [MY_LIBS="$MY_LIBS -lossaudio"; NEED_LIBOSSAUDIO=yes],
      AC_MSG_ERROR([cannot use ossaudio. See config.log for details.]))])

    LIBS="$OLD_LIBS"
    AC_MSG_RESULT($NEED_LIBOSSAUDIO)
  fi

# Check for GCC and GCC version

  GCC_VERSION=no
  AC_MSG_CHECKING([for GCC 2])
  AC_COMPILE_IFELSE([
#if ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 0))
int main(void) { return 0; }
#else
#error This is not GCC 2.x
#endif
],
  [AC_MSG_RESULT([yes]); GCC_VERSION=2],
  [AC_MSG_RESULT([no])])

  AC_MSG_CHECKING([for GCC 3 or newer])
  AC_COMPILE_IFELSE([
#if ((__GNUC__ >= 3) && (__GNUC_MINOR__ >= 0))
int main(void) { return 0; }
#else
#error This is not GCC 3.x
#endif
],
  [AC_MSG_RESULT([yes]); GCC_VERSION=3],
  [AC_MSG_RESULT([no])])

  if [[ "x$GCCVER" != "xno" ]]; then
    GCCVER="$GCC_VERSION"
  fi

  if [[ "x$GCCVER" != "xno" ]]; then
    CFLAGS="$CFLAGS -W -Wall -Wformat=2"
    if [[ "x$WARN" != "xno" ]]; then
      CFLAGS="$CFLAGS -Wstrict-prototypes -Wunused"
    fi
  fi
  if [[ "x$DEBUG" != "xno" ]]; then
    AC_MSG_RESULT(Turning on debug flags)
    CFLAGS="$CFLAGS -g -O"
    optimum=no
  else
    if [[ "x$GCCVER" != "xno" ]]; then
      AC_MSG_RESULT(Turning on gcc optimisations)
      CFLAGS="$CFLAGS -O3 -fomit-frame-pointer"
      if [[ "x$GCCVER" = "x3" ]]; then
        AC_MSG_RESULT(Turning on gcc 3 optimisations)
        CFLAGS="$CFLAGS -fno-math-errno"
      fi
      case "$CFLAGS" in
        *mcpu*|*march*) AC_MSG_RESULT(Skipping target optimisations)
              ;;
        *) case "$target_cpu" in
             i386) AC_MSG_RESULT(Turning on i386 optimisations)
                   CFLAGS="$CFLAGS -march=i386 -ffast-math"
                   ;;
             i486) AC_MSG_RESULT(Turning on i486 optimisations)
                   CFLAGS="$CFLAGS -march=i486 -ffast-math"
                   ;;
             i586) AC_MSG_RESULT(Turning on pentium optimisations)
                   # cannot do malign-double as we link to other libraries and
                   # it would break binary compatibility
                   CFLAGS="$CFLAGS -march=pentium -ffast-math"
                   ;;
             i?86) AC_MSG_RESULT(Turning on i686 optimisations)
                   # cannot do malign-double as we link to other libraries and
                   # it would break binary compatibility
                   CFLAGS="$CFLAGS -march=i686 -ffast-math"
                   ;;
             alpha*) AC_MSG_RESULT(Adding -mieee for Alpha)
                   CFLAGS="$CFLAGS -mieee"
                   ;;
                *) AC_MSG_RESULT(Sorry unknown target CPU)
                   ;;
           esac
      esac
    else
      AC_MSG_WARN(You did not opt for gcc optimisations!)
      optimum=no
    fi
  fi
  AC_CHECK_LIB(jpeg, jpeg_start_compress, 
    [ MY_LIBS="-ljpeg $MY_LIBS"; AC_DEFINE(JPEG, 1, [JPEG features]) ])

  AC_CHECK_LIB(bz2, BZ2_bzReadOpen,
    [ MY_LIBS="-lbz2 $MY_LIBS"; AC_DEFINE(USE_BZIP2, 1, [BZIP2 compression]) ])

  AC_CHECK_LIB(z, gzopen,
    [ MY_LIBS="-lz $MY_LIBS"; AC_DEFINE(USE_ZLIB, 1, [ZLIB compression]) ])
dnl Checks for programs.

  AC_PROG_CC
  AC_PROG_CPP
  AC_PROG_RANLIB
  AC_PROG_MAKE_SET
  AC_ISC_POSIX
  AC_PROG_INSTALL
  AC_TYPE_SIGNAL

dnl Checks for header files.

  AC_HEADER_STDC

dnl Checks for libraries.

  AC_PATH_XTRA

  if [[ "x$MY_PROG" = "xgenerator-gtk" ]]; then
    MY_SUBDIRS="glade gtkopts $MY_SUBDIRS"
    AM_PATH_GTK(1.2.10,,AC_MSG_ERROR(Generator needs gtk 1.2.10 or later))
    AM_PATH_SDL(1.2.6,,AC_MSG_ERROR(Generator needs sdk 1.2.6 or later))

    MY_LIBS="$MY_LIBS $X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS"
    CFLAGS="$CFLAGS $X_CFLAGS"
  elif [[ "x$MY_PROG" = "xgenerator-sdl" ]]; then
    MY_SUBDIRS="gtkopts $MY_SUBDIRS"
    AM_PATH_SDL(1.2.6,,AC_MSG_ERROR(Generator needs sdk 1.2.6 or later))

    MY_LIBS="$MY_LIBS $X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS"
    CFLAGS="$CFLAGS $X_CFLAGS"
  elif [[ "x$USE_SDL_AUDIO" != "xno" ]]; then
    AM_PATH_SDL(1.2.6,,AC_MSG_ERROR(Generator needs sdk 1.2.6 or later))
  fi

dnl Checks for typedefs, structures, and compiler characteristics.

  if [[ $cross_compiling = no ]]; then
    AC_C_BIGENDIAN
  else
    AC_MSG_WARN([Assuming little endian for cross compilation])
    # AC_DEFINE(WORDS_BIGENDIAN, 1)
  fi
  AC_C_CONST
  AC_C_INLINE
  AC_CHECK_SIZEOF(unsigned char, 1)
  AC_CHECK_SIZEOF(unsigned short, 2)
  AC_CHECK_SIZEOF(unsigned int, 4)
  AC_CHECK_SIZEOF(unsigned long, 4)
  AC_CHECK_SIZEOF(unsigned long long, 8)
  AC_CHECK_HEADERS(sys/param.h)
  AC_CHECK_HEADERS(bzlib.h)
  AC_CHECK_HEADERS(zlib.h)
  AC_CHECK_HEADERS(tcl8.0.h)
  AC_CHECK_HEADERS(tk8.0.h)

dnl Makefile substitutions

dnl Final stuff

  AC_CONFIG_FILES([Makefile cpu68k/Makefile raze/Makefile cmz80/Makefile
             ym2612/Makefile main/Makefile sn76496/Makefile glade/Makefile
             gtkopts/Makefile raze/raze.asm])
AC_OUTPUT

dnl Moooo.
