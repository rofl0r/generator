#include "generator.h"

#ifdef CMZ80
#  include "cpuz80-mz80.c"
#else
#  error "No z80 defined"
#endif
