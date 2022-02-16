dnl Generator configure.in file - process with autoconf

  AC_INIT(cpu68k/def68k.c)
  AC_PREREQ(2.52)
  AC_CANONICAL_SYSTEM
  AM_INIT_AUTOMAKE(generator, 0.35)
  AM_CONFIG_HEADER(config.h)
  optimum=yes

dnl See what version we're compiling

  MY_DIST_SUBDIRS="cpu68k raze cmz80 ym2612 sn76496 main glade gtkopts"
  MY_SUBDIRS="cpu68k ym2612 sn76496 main"
  MY_LIBS=""
  UNDERSCORE=""
  ASMOUTPUT=elf
  AC_ARG_WITH(coffasm,
              AC_HELP_STRING([--with-coffasm], [Assemble coff files]),
              ASMOUTPUT=coff; UNDERSCORE="_")
  AC_ARG_WITH(svgalib,
              AC_HELP_STRING([--with-svgalib], [Create svgalib version]),
              MY_PROG="generator-svgalib")
  AC_ARG_WITH(allegro,
              AC_HELP_STRING([--with-allegro], [Create allegro version]),
              MY_PROG="generator-allegro")
  AC_ARG_WITH(tcltk,
              AC_HELP_STRING([--with-tcltk], [Create tcltk version]),
              MY_PROG="generator-tcltk")
  AC_ARG_WITH(gtk,
              AC_HELP_STRING([--with-gtk], [Create gtk version]),
              MY_PROG="generator-gtk")
  if [[ "x$MY_PROG" = "x" ]]; then
    AC_MSG_ERROR(You must select a user interface type:
    svgalib allegro tcltk gtk)
  fi
  if [[ "x$MY_PROG" = "xgenerator-allegro" ]]; then
    AC_DEFINE(ALLEGRO, 1, [Allegro version])
    PROGRAM='generator-allegro$(EXEEXT)'
  else
    PROGRAM=$MY_PROG
  fi
  AC_ARG_WITH(raze,
              AC_HELP_STRING([--with-raze], [Use RAZE z80 emulation]),
              Z80="raze")
  AC_ARG_WITH(cmz80,
              AC_HELP_STRING([--with-cmz80], [Use C mz80 z80 emulation]),
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

  # default to gcc version 2
  GCCVER=2
  WARN=no
  DEBUG=no
  AC_ARG_WITH(nologging, AC_HELP_STRING([--with-nologging],
                                        [Turn off all logging]),
              AC_DEFINE(NOLOGGING, 1, [Define to turn off all logging]))
  AC_ARG_WITH(warnings, AC_HELP_STRING([--with-warnings],
                                       [Turn on gcc warnings]),
              WARN=$withval, WARN=no)
  AC_ARG_WITH(gcc, AC_HELP_STRING([--with-gcc@<:@=VER@:>@],
                                  [GCC optimisation setting]),
              GCCVER=$withval)
  AC_ARG_WITH(raze, AC_HELP_STRING([--with-raze],
                                   [Use raze z80 emulation]),
              RAZE=$withval, DEBUG=no)
  AC_ARG_WITH(debug, AC_HELP_STRING([--with-debug],
                                    [Turn debugging options on]),
              DEBUG=$withval, DEBUG=no)
  if [[ "x$WARN" != "xno" ]]; then
    CFLAGS="$CFLAGS -W -Wall -Wstrict-prototypes -Wformat -Wunused"
  fi
  if [[ "x$DEBUG" != "xno" ]]; then
    AC_MSG_RESULT(Turning on debug flags)
    CFLAGS="$CFLAGS -g -O"
    optimum=no
  else
    if [[ "x$GCCVER" != "xno" ]]; then
      AC_MSG_RESULT(Turning on gcc optimisations)
      CFLAGS="$CFLAGS -O3 -ffast-math -fomit-frame-pointer"
      if [[ "x$GCCVER" = "x3" ]]; then
        AC_MSG_RESULT(Turning on gcc 3 optimisations)
        CFLAGS="$CFLAGS -minline-all-stringops -fno-math-errno"
      fi
      case $target_cpu in
        i?86) AC_MSG_RESULT(Turning on pentium optimisations)
              # cannot do malign-double as we link to other libraries and it
              # would break binary compatibility
              CFLAGS="$CFLAGS -march=pentium -malign-loops=5"
              CFLAGS="$CFLAGS -malign-jumps=5 -malign-functions=5"
              ;;
      esac
    else
      AC_MSG_WARN(You did not opt for gcc optimisations!)
      optimum=no
    fi
  fi
  AC_CHECK_LIB(jpeg, jpeg_start_compress, 
    [ MY_LIBS="-ljpeg $MY_LIBS"; AC_DEFINE(JPEG, 1, [JPEG features]) ])

dnl Checks for programs.

  AC_PROG_CC
  AC_PROG_RANLIB
  AC_PROG_MAKE_SET
  AC_ISC_POSIX
  AC_PROG_INSTALL
  AC_TYPE_SIGNAL

dnl Checks for header files.

  AC_HEADER_STDC

dnl Checks for libraries.

  if [[ "x$MY_PROG" = "xgenerator-gtk" ]]; then
    MY_SUBDIRS="glade gtkopts $MY_SUBDIRS"
    AM_PATH_GTK(1.2.0,,AC_MSG_ERROR(Generator needs gtk 1.2.0 or later))
    AM_PATH_SDL(1.1.0,,AC_MSG_ERROR(Generator needs sdk 1.1.0 or later))
  fi

dnl Checks for typedefs, structures, and compiler characteristics.

  if [[ $cross_compiling = no ]]; then
    AC_C_BIGENDIAN
  else
    AC_WARN(Assuming little endian for cross compilation)
    # AC_DEFINE(WORDS_BIGENDIAN, 1)
  fi
  AC_C_CONST
  AC_C_INLINE
  AC_CHECK_SIZEOF(unsigned char, 1)
  AC_CHECK_SIZEOF(unsigned short, 2)
  AC_CHECK_SIZEOF(unsigned int, 4)
  AC_CHECK_SIZEOF(unsigned long, 4)
  AC_CHECK_SIZEOF(unsigned long long, 8)
  AC_CHECK_FUNCS(snprintf)
  AC_CHECK_HEADERS(tcl8.0.h)
  AC_CHECK_HEADERS(tk8.0.h)

dnl Makefile substitutions

dnl Final stuff

  AC_OUTPUT([Makefile cpu68k/Makefile raze/Makefile cmz80/Makefile
             ym2612/Makefile main/Makefile sn76496/Makefile glade/Makefile
             gtkopts/Makefile raze/raze.asm])

  if [[ $optimum = no ]]; then
    echo '!!! Generator was compiled non-optimally, see warnings above !!!' 1>&2
  fi

dnl Moooo.
