#include "generator.h"

#ifdef RAZE
#  include "cpuz80-raze.c"
#else
#  ifdef CMZ80
#    include "cpuz80-mz80.c"
#  else
#    error "No z80 defined"
#  endif
#endif
