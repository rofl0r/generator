/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* reg68k.h                                                                  */
/*                                                                           */
/*****************************************************************************/

#ifndef REG68K_HEADER_FILE
#define REG68K_HEADER_FILE

unsigned int reg68k_external_step(void);
unsigned int reg68k_external_execute(unsigned int clocks);
void reg68k_external_autovector(int avno);

void reg68k_internal_autovector(int avno);
void reg68k_internal_vector(int vno, uint32 oldpc);

#endif /* REG68K_HEADER_FILE */
