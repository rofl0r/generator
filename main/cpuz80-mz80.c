/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"
#include "cpuz80.h"
#include "cpu68k.h"
#include "memz80.h"
#include "ui.h"

UINT8 cpuz80_read_actual(UINT32 addr, struct MemoryReadByte *me);
void cpuz80_write_actual(UINT32 addr, UINT8 data, struct MemoryWriteByte *me);
UINT16 cpuz80_ioread_actual(UINT16 addr, struct z80PortRead *me);
void cpuz80_iowrite_actual(UINT16 addr, UINT8 data, struct z80PortWrite *me);

/*** variables externed ***/

uint8 *cpuz80_ram = NULL;
uint32 cpuz80_bank = 0;
uint8 cpuz80_resetting = 0;
uint8 cpuz80_active = 0;
unsigned int cpuz80_on = 1;     /* z80 turned on? */
CONTEXTMZ80 cpuz80_z80;

/*** global variables ***/

static unsigned int cpuz80_lastsync = 0;

static struct MemoryReadByte cpuz80_read[] = {
  {0x0000, 0xFFFF, cpuz80_read_actual, NULL},
  {-1, -1, NULL, NULL}
};

static struct MemoryWriteByte cpuz80_write[] = {
  {0x0000, 0xFFFF, cpuz80_write_actual, NULL},
  {-1, -1, NULL, NULL}
};

static struct z80PortRead cpuz80_ioread[] = {
  {0x0000, 0x00FF, cpuz80_ioread_actual, NULL},
  {-1, -1, NULL, NULL}
};

static struct z80PortWrite cpuz80_iowrite[] = {
  {0x0000, 0x00FF, cpuz80_iowrite_actual, NULL},
  {-1, -1, NULL, NULL}
};

UINT8 cpuz80_read_actual(UINT32 addr, struct MemoryReadByte *me)
{
  (void)me;
  return memz80_fetchbyte((uint16)addr);
}

void cpuz80_write_actual(UINT32 addr, UINT8 data, struct MemoryWriteByte *me)
{
  (void)me;
  memz80_storebyte((uint16)addr, (uint8)data);
}

UINT16 cpuz80_ioread_actual(UINT16 addr, struct z80PortRead *me)
{
  (void)me;
  return cpuz80_portread((uint16)addr);
}

void cpuz80_iowrite_actual(UINT16 addr, UINT8 data, struct z80PortWrite *me)
{
  (void)me;
  cpuz80_portwrite((uint16)addr, (uint8)data);
}

/*** cpuz80_init - initialise this sub-unit ***/

int cpuz80_init(void)
{
  /* mz80init(); - comment in this line if you use assembler z80 */
  cpuz80_reset();
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
  cpuz80_bank = 0;
  cpuz80_active = 0;
  cpuz80_lastsync = 0;
  cpuz80_resetting = 1;
  memset(&cpuz80_z80, 0, sizeof(cpuz80_z80));
  cpuz80_z80.z80Base = cpuz80_ram;
  cpuz80_z80.z80MemRead = cpuz80_read;
  cpuz80_z80.z80MemWrite = cpuz80_write;
  cpuz80_z80.z80IoRead = cpuz80_ioread;
  cpuz80_z80.z80IoWrite = cpuz80_iowrite;
  mz80SetContext(&cpuz80_z80);
  mz80reset();
}

/*** cpuz80_updatecontext - inform z80 processor of changed context ***/

void cpuz80_updatecontext(void)
{
  mz80SetContext(&cpuz80_z80);
}

/*** cpuz80_resetcpu - reset z80 cpu ***/

void cpuz80_resetcpu(void)
{
  mz80reset();
  cpuz80_resetting = 1;         /* suspends execution */
}

/*** cpuz80_unresetcpu - unreset z80 cpu ***/

void cpuz80_unresetcpu(void)
{
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

  if (cpuz80_on && cpuz80_active && !cpuz80_resetting) {
    /* ui_log(LOG_USER, "executing %d z80 clocks @ %X", wanted,
       cpuz80_z80.z80pc); */
    mz80exec(wanted);
    achieved = mz80GetElapsedTicks(1);
    cpuz80_lastsync = cpuz80_lastsync + achieved * 15 / 7;
  } else {
    cpuz80_lastsync = cpu68k_clocks;
  }
}

/*** cpuz80_interrupt - cause an interrupt on the z80 */

void cpuz80_interrupt(void)
{
  if (!cpuz80_resetting)
    mz80int(0);
}

/*** cpuz80_portread - port read from z80 */

uint8 cpuz80_portread(uint8 port)
{
  LOG_VERBOSE(("[Z80] Port read to %X", port));
  return 0;
}

/*** cpuz80_portwrite - z80 write to port */

void cpuz80_portwrite(uint8 port, uint8 value)
{
  LOG_VERBOSE(("[Z80] Port write to %X of %X", port, value));
}
