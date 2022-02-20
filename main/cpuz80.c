#include "generator.h"

#ifdef CMZ80
#  include "cpuz80-mz80.c"
#elif defined(JGZ80)
#  include "cpuz80-jg.c"
#else
#  error "No z80 defined"
#endif
