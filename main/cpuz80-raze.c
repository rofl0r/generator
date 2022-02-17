/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"

#include "cpuz80.h"
#include "cpu68k.h"
#include "memz80.h"
#include "ui.h"

#include "raze.h"

/*** variables externed ***/

uint8 *cpuz80_ram = NULL;
uint32 cpuz80_bank = 0;
uint8 cpuz80_active = 0;
uint8 cpuz80_resetting = 0;
unsigned int cpuz80_pending = 0;
unsigned int cpuz80_on = 1;     /* z80 turned on? */

/*** global variables ***/

static unsigned int cpuz80_lastsync = 0;

int cpuz80_init(void)
{
  cpuz80_reset();
  return 0;
}

/*** cpuz80_reset - reset z80 sub-unit ***/

void cpuz80_reset(void)
{
  int i;

  if (!cpuz80_ram) {
    if ((cpuz80_ram = malloc(LEN_SRAM)) == NULL) {
      fprintf(stderr, "Out of memory\n");
      exit(1);
    }
  }
  memset(cpuz80_ram, 0, LEN_SRAM);
  cpuz80_bank = 0;
  cpuz80_active = 0;
  cpuz80_lastsync = 0;
  cpuz80_resetting = 1;
  cpuz80_pending = 0;
  z80_init_memmap();
  z80_map_fetch(0x0000, 0x3fff, cpuz80_ram);
  z80_map_fetch(0x4000, 0x7fff, cpuz80_ram);    /* ok? */
  z80_map_fetch(0x8000, 0xbfff, cpuz80_ram);    /* ok? */
  z80_map_fetch(0xc000, 0xffff, cpuz80_ram);    /* ok? */
  z80_add_read(0x000, 0x3fff, Z80_MAP_DIRECT, cpuz80_ram);
  z80_add_write(0x000, 0x3fff, Z80_MAP_DIRECT, cpuz80_ram);
  for (i = 1; (memz80_def[i].start != 0) || (memz80_def[i].end != 0); i++) {
    z80_add_read(memz80_def[i].start << 8, (memz80_def[i].end << 8) - 1,
                 Z80_MAP_HANDLED, memz80_def[i].fetch_byte);
    z80_add_write(memz80_def[i].start << 8, (memz80_def[i].end << 8) - 1,
                  Z80_MAP_HANDLED, memz80_def[i].store_byte);
  }
  z80_end_memmap();
  z80_reset();
}

/*** cpuz80_updatecontext - inform z80 processor of changed context ***/

void cpuz80_updatecontext(void)
{
  /* not required with raze */
}

/*** cpuz80_resetcpu - reset z80 cpu ***/

void cpuz80_resetcpu(void)
{
  cpuz80_sync();
  z80_reset();
  cpuz80_resetting = 1;         /* suspends execution */
}

/*** cpuz80_unresetcpu - unreset z80 cpu ***/

void cpuz80_unresetcpu(void)
{
  if (cpuz80_resetting)
    cpuz80_sync();
  cpuz80_resetting = 0;         /* un-suspends execution */
}

/*** cpuz80_bankwrite - data is being written to latch ***/

void cpuz80_bankwrite(uint8 data)
{
  cpuz80_bank = (((cpuz80_bank >> 1) | ((data & 1) << 23)) & 0xff8000);
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
  if (!cpuz80_active)
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
  int wanted = (cpu68k_wanted < 0 ? 0 : cpu68k_wanted) * 7 / 15;
  int achieved;

  if (cpuz80_pending && z80_get_reg(Z80_REG_IFF1)) {
    z80_raise_IRQ(0xff);
    z80_lower_IRQ();
    cpuz80_pending = 0;
  }
  if (cpuz80_on && cpuz80_active && !cpuz80_resetting) {
    /* ui_log(LOG_USER, "executing %d z80 clocks @ %X", wanted,
       cpuz80_z80.z80pc); */
    achieved = z80_emulate(wanted);
    cpuz80_lastsync = cpuz80_lastsync + achieved * 15 / 7;
  } else {
    cpuz80_lastsync = cpu68k_clocks;
  }
}

/*** cpuz80_interrupt - cause an interrupt on the z80 */

void cpuz80_interrupt(void)
{
  if (!cpuz80_resetting) {
    if (z80_get_reg(Z80_REG_IFF1) == 0)
      cpuz80_pending = 1;
    z80_raise_IRQ(0xff);
    z80_lower_IRQ();
  }
}

void cpuz80_uninterrupt(void)
{
  z80_lower_IRQ();
}
