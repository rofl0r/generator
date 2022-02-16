/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-a.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "generator.h"
#include "cpu68k.h"
#include "mem68k.h"
#include "reg68k.h"

#include "cpu68k-inline.h"

#include <stdio.h>

void cpu_op_1612a(t_ipc *ipc) /* LINE10 */ {
  /* mask f000, bits a000, mnemonic 74, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 0, stype 19, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */

  reg68k_internal_vector(V_LINE10, PC);
}

