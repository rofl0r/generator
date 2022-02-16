/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* z80.c                                                                     */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <generator.h>
#include <cpuz80.h>
#include <cpu68k.h>
#include <memz80.h>
#include <ui.h>

#include <z80/z80.h>

/*** variables externed ***/

uint8 *cpuz80_ram = NULL;
uint32 cpuz80_bank = 0;
unsigned int cpuz80_active = 0;

/*** global variables ***/

static unsigned int cpuz80_lastsync = 0;
static unsigned int cpuz80_resetting = 0;

/*** cpuz80_init - initialise this sub-unit ***/

int cpuz80_init(void)
{
  cpuz80_reset();
  ui_log(LOG_CRITICAL, "z80 init");
  return 0;
}

/*** cpuz80_callback - callback ***/

int cpuz80_callback(int val) {
  (void)val;
  return 0;
}

/*** cpuz80_reset - reset z80 sub-unit ***/

void cpuz80_reset(void)
{
  if (!cpuz80_ram) {
    if ((cpuz80_ram = malloc(LEN_SRAM)) == NULL) {
      fprintf(stderr, "Out of memory\n");
      exit(1);
    }
  }
  memset(cpuz80_ram, 0, LEN_SRAM);
  cpuz80_resetcpu();
  cpuz80_bank = 0;
  cpuz80_active = 0;
  cpuz80_lastsync = 0;
  cpuz80_resetting = 0;
  z80_reset(NULL);
  z80_set_irq_callback(cpuz80_callback);
}

/*** cpuz80_resetcpu - reset z80 cpu ***/

void cpuz80_resetcpu(void)
{
  z80_reset(NULL);
  z80_set_irq_callback(cpuz80_callback);
  cpuz80_resetting = 1; /* suspends execution */
}

/*** cpuz80_unresetcpu - unreset z80 cpu ***/

void cpuz80_unresetcpu(void)
{
  cpuz80_resetting = 0; /* un-suspends execution */
}

/*** cpuz80_bankwrite - data is being written to latch ***/

void cpuz80_bankwrite(uint8 data)
{
  cpuz80_bank  = (((cpuz80_bank >> 1) | ((data & 1) <<23)) & 0xff8000);
  /*
  if (gen_debug)
    printf("BANK WRITE: %d --> %08X\n", data, cpuz80_bank);
  */
}

/*** cpuz80_stop - stop the processor ***/

void cpuz80_stop(void)
{
  cpuz80_sync();
  cpuz80_active = 0;
}

/*** cpuz80_start - start the processor ***/

void cpuz80_start(void)
{
  cpuz80_sync();
  cpuz80_active = 1;
}

/*** cpuz80_endfield - reset counters ***/

void cpuz80_endfield(void)
{
  cpuz80_lastsync = 0;
}

/*** cpuz80_sync - synchronise ***/

void cpuz80_sync(void)
{
  int cpu68k_wanted = cpu68k_clocks - cpuz80_lastsync;
  int wanted = (cpu68k_wanted<0?0:cpu68k_wanted)*7/16;
  int acheived;

  if (cpuz80_active && !cpuz80_resetting)
    acheived = z80_execute(wanted);
  cpuz80_lastsync = cpu68k_clocks;
}

/*** cpuz80_interrupt - cause an interrupt on the z80 */

void cpuz80_interrupt(void)
{
  if (!cpuz80_resetting)
    z80_set_irq_line(0, 1);
}

/*** cpuz80_portread - port read from z80 */

uint8 cpuz80_portread(uint8 port)
{
  printf("[Z80] Port read to %X\n", port);
  return 0;
}

/*** cpuz80_portwrite - z80 write to port */

void cpuz80_portwrite(uint8 port, uint8 value)
{
  printf("[Z80] Port write to %X of %X\n", port, value);
}
