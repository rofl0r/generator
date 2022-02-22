#include "generator.h"

#include "cpuz80.h"
#include "cpu68k.h"
#include "memz80.h"
#include "z80.h"

#define BANK_INIT 0

/*** variables externed ***/

uint8 *cpuz80_ram = NULL;
uint32 cpuz80_bank = BANK_INIT;
uint8 cpuz80_active = 0;
uint8 cpuz80_resetting = 0;
unsigned int cpuz80_pending = 0;
unsigned int cpuz80_on = 1;     /* z80 turned on? */
CONTEXTMZ80 cpuz80_z80;

/*** global variables ***/

static unsigned int cpuz80_lastsync = 0;

static uint8 cpuz80_read(void* user, uint16 addr) {
	(void) user;
	return memz80_fetchbyte((uint16)addr);
}

static void cpuz80_write(void *user, uint16 addr, uint8 data) {
	(void) user;
	memz80_storebyte((uint16)addr, (uint8)data);
}

static uint8 cpuz80_portread_actual(void* user, uint8 port)
{
  (void) user;
  LOG_VERBOSE(("[Z80] Port read to %X", port));
  return 0;
}

static void cpuz80_portwrite_actual(void* user, uint8 port, uint8 value)
{
  (void) user;
  LOG_VERBOSE(("[Z80] Port write to %X of %X", port, value));
}

uint8 cpuz80_portread(uint8 port) { return cpuz80_portread_actual(0, port); }
void cpuz80_portwrite(uint8 port, uint8 value) { return cpuz80_portwrite_actual(0, port, value); }

static void jgz80_init() {
  z80_init(&cpuz80_z80);
  cpuz80_z80.read_byte = cpuz80_read;
  cpuz80_z80.write_byte = cpuz80_write;
  cpuz80_z80.port_in = cpuz80_portread;
  cpuz80_z80.port_out = cpuz80_portwrite;
}


int cpuz80_init(void)
{
  jgz80_init();
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
  cpuz80_bank = BANK_INIT;
  cpuz80_active = 0;
  cpuz80_lastsync = 0;
  cpuz80_resetting = 1;
  cpuz80_pending = 0;
  jgz80_init();
}

/*** cpuz80_updatecontext - inform z80 processor of changed context ***/

void cpuz80_updatecontext(void)
{
  /* not required with jgz80 */
}

/*** cpuz80_resetcpu - reset z80 cpu ***/

void cpuz80_resetcpu(void)
{
  cpuz80_sync();
  jgz80_init();
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
  int achieved = 0;

#if 0
  if (cpuz80_pending && z80_get_reg(Z80_REG_IFF1)) {
    z80_raise_IRQ(0xff);
    z80_lower_IRQ();
    cpuz80_pending = 0;
  }
#endif
  if (cpuz80_on && cpuz80_active && !cpuz80_resetting) {
    /* ui_log(LOG_USER, "executing %d z80 clocks @ %X", wanted,
       cpuz80_z80.z80pc); */
    achieved = z80_step_n(&cpuz80_z80, wanted);
    cpuz80_lastsync = cpuz80_lastsync + achieved * 15 / 7;
  } else {
    cpuz80_lastsync = cpu68k_clocks;
  }
}

/*** cpuz80_interrupt - cause an interrupt on the z80 */

void cpuz80_interrupt(void)
{
  if (!cpuz80_resetting) {
#if 0
    if (z80_get_reg(Z80_REG_IFF1) == 0)
      cpuz80_pending = 1;
    z80_raise_IRQ(0xff);
    z80_lower_IRQ();
#endif
    z80_gen_int(&cpuz80_z80, 0xff);
  }
}

void cpuz80_uninterrupt(void)
{
  z80_clr_int(&cpuz80_z80);
}


