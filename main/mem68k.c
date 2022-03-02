/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"

#include "cpu68k.h"
#include "vdp.h"
#include "cpuz80.h"
#include "gensound.h"
#include "ui.h"

#define MEM68K_IMPL
#include "mem68k.h"

#undef DEBUG_VDP
#undef DEBUG_BUS
#undef DEBUG_SRAM
#undef DEBUG_RAM
#undef DEBUG_BANK

#ifdef DEBUG_VDP
#define VDP_LOG(x) LOG_VERBOSE(x)
#define VDP_LOG_CR(x) LOG_CRITICAL(x)
#else
#define VDP_LOG(x) do {} while(0)
#define VDP_LOG_CR(x) do {} while(0)
#endif


/*** forward references ***/

uint8 *mem68k_memptr_bad(uint32 addr);
uint8 *mem68k_memptr_rom(uint32 addr);
uint8 *mem68k_memptr_ram(uint32 addr);

t_keys mem68k_cont[2];

static uint8 mem68k_cont1ctrl;
static uint8 mem68k_cont2ctrl;
static uint8 mem68k_contEctrl;
static uint8 mem68k_cont1output;
static uint8 mem68k_cont2output;
static uint8 mem68k_contEoutput;

/*** memory map ***/

t_mem68k_def mem68k_def[] = {

  {0x000, 0x400, mem68k_memptr_rom},

  {0x400, 0x1000, mem68k_memptr_bad},

  {0xA00, 0xA10, mem68k_memptr_bad},     /* note overlaps blocks below */

  {0xA04, 0xA05, mem68k_memptr_bad},

  {0xA06, 0xA07, mem68k_memptr_bad},

  {0xA0C, 0xA10, mem68k_memptr_bad},     /* this is probably more yam/bank stuff */

  {0xA10, 0xA11, mem68k_memptr_bad},

  {0xA11, 0xA12, mem68k_memptr_bad},

  {0xC00, 0xC01, mem68k_memptr_bad},

  {0xE00, 0x1000, mem68k_memptr_ram},

  {0, 0, NULL}

};

uint8 *(*mem68k_memptr[0x1000]) (uint32 addr);

/*** initialise memory tables ***/

int mem68k_init(void)
{
  int i = 0;
  int j;

  do {
    for (j = mem68k_def[i].start; j < mem68k_def[i].end; j++) {
      mem68k_memptr[j] = mem68k_def[i].memptr;
    }
    i++;
  }
  while ((mem68k_def[i].start != 0) || (mem68k_def[i].end != 0));
  mem68k_cont1ctrl = 0;
  mem68k_cont2ctrl = 0;
  mem68k_contEctrl = 0;
  mem68k_cont1output = 0;
  mem68k_cont2output = 0;
  mem68k_contEoutput = 0;
  memset(&mem68k_cont, 0, sizeof(mem68k_cont));
  return 0;
}

/*** memptr routines - called for IPC generation so speed is not vital ***/

uint8 *mem68k_memptr_bad(uint32 addr)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory access 0x%X", regs.pc, addr));
  return cpu68k_rom;
}

uint8 *mem68k_memptr_rom(uint32 addr)
{
  if (addr < cpu68k_romlen) {
    return (cpu68k_rom + addr);
  }
  LOG_CRITICAL(("%08X [MEM] Invalid memory access to ROM 0x%X", regs.pc,
                addr));
  return cpu68k_rom;
}

uint8 *mem68k_memptr_ram(uint32 addr)
{
  /* LOG_USER(("%08X [MEM] Executing code from RAM 0x%X", regs.pc, addr)); */
  addr &= 0xffff;
  return (cpu68k_ram + addr);
}


/*** BAD fetch/store ***/

uint8 mem68k_fetch_bad_byte(uint32 addr)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory fetch (byte) 0x%X", regs.pc,
                addr));
  return 0;
}

uint16 mem68k_fetch_bad_word(uint32 addr)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory fetch (word) 0x%X", regs.pc,
                addr));
  return 0;
}

uint32 mem68k_fetch_bad_long(uint32 addr)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory fetch (long) 0x%X", regs.pc,
                addr));
  return 0;
}

void mem68k_store_bad_byte(uint32 addr, uint8 data)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory store (byte) 0x%X = %X", regs.pc,
                addr, data));
}

void mem68k_store_bad_word(uint32 addr, uint16 data)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory store (word) 0x%X = %X", regs.pc,
                addr, data));
}

void mem68k_store_bad_long(uint32 addr, uint32 data)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory store (long) 0x%X = %X", regs.pc,
                addr, data));
}


/*** ROM store ***/

void mem68k_store_rom_byte(uint32 addr, uint8 data)
{
  LOG_CRITICAL(("%08X [ROM] Invalid memory store (byte) 0x%X = %X", regs.pc,
                addr, data));
}

void mem68k_store_rom_word(uint32 addr, uint16 data)
{
  LOG_CRITICAL(("%08X [ROM] Invalid memory store (word) 0x%X = %X", regs.pc,
                addr, data));
}

void mem68k_store_rom_long(uint32 addr, uint32 data)
{
  LOG_CRITICAL(("%08X [ROM] Invalid memory store (long) 0x%X = %X", regs.pc,
                addr, data));
}

/*** YAM invalid fetch/store ***/

uint16 mem68k_fetch_yam_word(uint32 addr)
{
  BUS_CHECK("YAM");
  addr -= 0xA04000;
  LOG_CRITICAL(("%08X [YAM] Invalid memory fetch (word) 0x%X", regs.pc,
                addr));
  return 0;
}

uint32 mem68k_fetch_yam_long(uint32 addr)
{
  BUS_CHECK("YAM");
  addr -= 0xA04000;
  /* no longs please */
  LOG_CRITICAL(("%08X [YAM] Invalid memory fetch (long) 0x%X", regs.pc,
                addr));
  return 0;
}

void mem68k_store_yam_word(uint32 addr, uint16 data)
{
  BUS_CHECK("YAM");
  addr -= 0xA04000;
  LOG_CRITICAL(("%08X [YAM] Invalid memory store (word) 0x%X = %X", regs.pc,
                addr, data));
}

void mem68k_store_yam_long(uint32 addr, uint32 data)
{
  BUS_CHECK("YAM");
  addr -= 0xA04000;
  /* no longs please */
  LOG_CRITICAL(("%08X [YAM] Invalid memory store (long) 0x%X = %X", regs.pc,
                addr, data));
}


/*** BANK fetch / invalid store ***/

uint8 mem68k_fetch_bank_byte(uint32 addr)
{
  /* write only */
  LOG_CRITICAL(("%08X [BANK] Invalid memory fetch (byte) 0x%X", regs.pc,
                addr));
  return 0;
}

uint16 mem68k_fetch_bank_word(uint32 addr)
{
  /* write only */
  LOG_CRITICAL(("%08X [BANK] Invalid memory fetch (word) 0x%X", regs.pc,
                addr));
  return 0;
}

uint32 mem68k_fetch_bank_long(uint32 addr)
{
  /* write only */
  LOG_CRITICAL(("%08X [BANK] Invalid memory fetch (long) 0x%X", regs.pc,
                addr));
  return 0;
}


void mem68k_store_bank_long(uint32 addr, uint32 data)
{
  addr -= 0xA06000;
  /* no longs please */
  LOG_CRITICAL(("%08X [BANK] Invalid memory store (long) 0x%X = %X", regs.pc,
                addr, data));
}


/*** I/O fetch/store ***/

uint8 mem68k_fetch_io_byte(uint32 addr)
{
  uint8 in;

  addr -= 0xA10000;
  if ((addr & 1) == 0) {
    LOG_CRITICAL(("%08X [IO] Invalid memory fetch (byte) 0x%X",
                  regs.pc, addr));
    return 0;
  }
  switch (addr >> 1) {
  case 0:                      /* 0x1 */
    /* version */
    LOG_VERBOSE(("PAL: %d; Overseas: %d", vdp_pal, vdp_overseas));
    return (1 << 5 | vdp_pal << 6 | vdp_overseas << 7);
  case 1:                      /* 0x3 */
    /* get input state */
    in = ((mem68k_cont1output & 1 << 6)
          ? ((1 - mem68k_cont[0].up) | (1 - mem68k_cont[0].down) << 1 |
             (1 - mem68k_cont[0].left) << 2 | (1 - mem68k_cont[0].right) << 3 |
             (1 - mem68k_cont[0].b) << 4 | (1 - mem68k_cont[0].c) << 5)
          : ((1 - mem68k_cont[0].up) | (1 - mem68k_cont[0].down) << 1 |
             (1 - mem68k_cont[0].a) << 4 | (1 - mem68k_cont[0].start) << 5));
    return (in & ~mem68k_cont1ctrl) | (mem68k_cont1output & mem68k_cont1ctrl);
  case 2:                      /* 0x5 */
    /* get input state */
    in = ((mem68k_cont2output & 1 << 6)
          ? ((1 - mem68k_cont[1].up) | (1 - mem68k_cont[1].down) << 1 |
             (1 - mem68k_cont[1].left) << 2 | (1 - mem68k_cont[1].right) << 3 |
             (1 - mem68k_cont[1].b) << 4 | (1 - mem68k_cont[1].c) << 5)
          : ((1 - mem68k_cont[1].up) | (1 - mem68k_cont[1].down) << 1 |
             (1 - mem68k_cont[1].a) << 4 | (1 - mem68k_cont[1].start) << 5));
    return (in & ~mem68k_cont2ctrl) | (mem68k_cont2output & mem68k_cont2ctrl);
  case 3:                      /* 0x7 */
    LOG_NORMAL(("%08X [IO] EXT port read", regs.pc));
    /* get input state */
    in = 0;                     /* BUG: unsupported */
    return (in & ~mem68k_cont1ctrl) | (mem68k_cont1output & mem68k_cont1ctrl);
  case 4:                      /* 0x9 */
    return mem68k_cont1ctrl;
  case 5:                      /* 0xB */
    return mem68k_cont2ctrl;
  case 6:                      /* 0xD */
    return mem68k_contEctrl;
  default:
    LOG_CRITICAL(("%08X [IO] Invalid memory fetch (byte) 0x%X",
                  regs.pc, addr));
    return 0;
  }
}

uint16 mem68k_fetch_io_word(uint32 addr)
{
  return mem68k_fetch_io_byte(addr + 1);
}

uint32 mem68k_fetch_io_long(uint32 addr)
{
  return (mem68k_fetch_io_word(addr) << 16) | mem68k_fetch_io_word(addr + 2);
}

void mem68k_store_io_byte(uint32 addr, uint8 data)
{
  addr -= 0xA10000;
  switch (addr) {
  case 0x3:
    mem68k_cont1output = data;
    return;
  case 0x5:
    mem68k_cont2output = data;
    return;
  case 0x7:
    mem68k_contEoutput = data;
    LOG_NORMAL(("%08X [IO] EXT port output set to %X", regs.pc, data));
    return;
  case 0x9:
    mem68k_cont1ctrl = data;
    if (data != 0x40) {
      LOG_CRITICAL(("%08X [IO] Unknown controller 1 setting (0x%X)",
                    regs.pc, data));
    }
    return;
  case 0xB:
    mem68k_cont2ctrl = data;
    if (data != 0x40) {
      LOG_CRITICAL(("%08X [IO] Unknown controller 2 setting (0x%X)",
                    regs.pc, data));
    }
    return;
  case 0xD:
    mem68k_contEctrl = data;
    LOG_NORMAL(("%08X [IO] EXT port ctrl set to %X", regs.pc, data));
    return;
  case 0xF:
  case 0x11:
  case 0x13:
  case 0x15:
  case 0x17:
  case 0x19:
  case 0x1B:
  case 0x1D:
  case 0x1F:
    /* return; */
  default:
    LOG_CRITICAL(("%08X [IO] Invalid memory store (byte) 0x%X",
                  regs.pc, addr));
    return;
  }
}

void mem68k_store_io_word(uint32 addr, uint16 data)
{
  if (data >> 8)
    LOG_CRITICAL(("%08X [IO] Word store to %X of %X", addr, data));
  mem68k_store_io_byte(addr + 1, data & 0xff);
}

void mem68k_store_io_long(uint32 addr, uint32 data)
{
  mem68k_store_io_word(addr, (uint16)(data >> 16));
  mem68k_store_io_word(addr + 2, (uint16)data);
  return;
}


/*** CTRL store/invalid fetch ***/

uint32 mem68k_fetch_ctrl_long(uint32 addr)
{
  addr -= 0xA11000;
  /* no long access allowed */
  LOG_CRITICAL(("%08X [CTRL] Invalid memory fetch (long) 0x%X",
                regs.pc, addr));
  return 0;
}

void mem68k_store_ctrl_byte(uint32 addr, uint8 data)
{
  addr -= 0xA11000;
  if (addr == 0x000 || addr == 0x001) {
    /* z80 memory mode - not applicable for production carts */
    return;
  } else if (addr == 0x100) {
    /* bus request */
    if (data & 1) {
      cpuz80_stop();
      LOG_DEBUG1(("%08X Z80 stopped", regs.pc));
    } else {
      cpuz80_start();
      LOG_DEBUG1(("%08X Z80 started", regs.pc));
    }
  } else if (addr == 0x101) {
    return;                     /* ignore low byte */
  } else if (addr == 0x200) {
    /* z80 reset request */
    if (!(data & 1)) {
      /* cpuz80_stop(); */
      cpuz80_resetcpu();
      sound_genreset();
      LOG_DEBUG1(("%08X Z80 reset", regs.pc));
    } else {
      cpuz80_unresetcpu();
      LOG_DEBUG1(("%08X Z80 un-reset", regs.pc));
    }
  } else if (addr == 0x201) {
    return;                     /* ignore low byte */
  } else {
    LOG_CRITICAL(("%08X [CTRL] Invalid memory store (byte) 0x%X",
                  regs.pc, addr));
  }
}

void mem68k_store_ctrl_word(uint32 addr, uint16 data)
{
  addr -= 0xA11000;
  if (addr == 0x000) {
    /* z80 memory mode - not applicable for production carts */
    return;
  } else if (addr == 0x100) {
    /* bus request */
    if (data == 0x100) {
      cpuz80_stop();
      LOG_DEBUG1(("%08X Z80 stopped", regs.pc));
    } else {
      cpuz80_start();
      LOG_DEBUG1(("%08X Z80 started", regs.pc));
    }
  } else if (addr == 0x200) {
    /* z80 reset request */
    if (!(data & 0x100)) {
      /* cpuz80_stop(); */
      cpuz80_resetcpu();
      sound_genreset();
      LOG_DEBUG1(("%08X Z80 reset", regs.pc));
    } else {
      cpuz80_unresetcpu();
      LOG_DEBUG1(("%08X Z80 un-reset", regs.pc));
    }
  } else {
    LOG_CRITICAL(("%08X [CTRL] Invalid memory store (word) 0x%X",
                  regs.pc, addr));
  }
}

void mem68k_store_ctrl_long(uint32 addr, uint32 data)
{
  addr -= 0xA11000;
  /* no long access allowed */
  LOG_CRITICAL(("%08X [CTRL] Invalid memory store (long) 0x%X = %X",
                regs.pc, addr, data));
}


/*** VDP fetch/store ***/

uint16 mem68k_fetch_vdp_word(uint32 addr)
{
  BUS_CHECK("VDP");
  addr -= 0xC00000;
  switch (addr >> 1) {
  case 0:
  case 1:
    /* data port */
    VDP_LOG(("%08X [VDP] Word fetch from data port 0x%X", regs.pc, addr));
    return vdp_fetchdata();
  case 2:
  case 3:
    /* control port */
    VDP_LOG(("%08X [VDP] Word fetch from control port "
                 "(status) 0x%X", regs.pc, addr));
    return vdp_status();
  case 4:
    /* hv counter */
    {
      uint8 line8;
      uint16 hvcount;

      /* line counter advances at H-int */
      line8 = (vdp_line - vdp_visstartline + (vdp_event > 2 ? 1 : 0)) & 0xff;

      VDP_LOG(("%08X [VDP] Word fetch from hv counter 0x%X",
                   regs.pc, addr));
      if ((vdp_reg[12] >> 1) & 3) {
        /* interlace mode - replace lowest bit with highest bit */
        hvcount = ((line8 & ~1) << 8) | (line8 & 0x100) | vdp_gethpos();
        LOG_DEBUG1(("Interlace mode HV read - check this: %04X", hvcount));
        return hvcount;
      } else {
        /* non-interlace mode */
        hvcount = (line8 << 8) | vdp_gethpos();
        LOG_DEBUG1(("%08X H/V counter read = %04X", regs.pc, hvcount));
        return hvcount;
      }
    }
  case 8:
    LOG_CRITICAL(("%08X [VDP] PSG/prohibited word fetch.", regs.pc));
    return 0;
  default:
    LOG_CRITICAL(("%08X [VDP] Invalid memory fetch (word) 0x%X",
                  regs.pc, addr));
    return 0;
  }
}

uint32 mem68k_fetch_vdp_long(uint32 addr)
{
  BUS_CHECK("VDP");
  addr -= 0xC00000;
  switch (addr >> 1) {
  case 0:
    /* data port */
    VDP_LOG(("%08X [VDP] Long fetch from data port 0x%X", regs.pc, addr));
    return (vdp_fetchdata() << 16) | vdp_fetchdata();
  case 2:
    /* control port */
    VDP_LOG_CR(("%08X [VDP] Long fetch from control port 0x%X",
                 regs.pc, addr));
    return 0;
  case 4:
    /* hv counter ish */
    LOG_CRITICAL(("%08X [VDP] Long fetch from hv/prohibited 0x%X",
                  regs.pc, addr));
    return 0;
  default:
    LOG_CRITICAL(("%08X [VDP] Invalid memory fetch (word) 0x%X",
                  regs.pc, addr));
    return 0;
  }
}

void mem68k_store_vdp_byte(uint32 addr, uint8 data)
{
  addr -= 0xC00000;
  switch (addr) {
  case 0:
  case 1:
  case 2:
  case 3:
    /* data port */
    VDP_LOG(("%08X [VDP] Byte store to DATA of %X [%d][%X]", regs.pc,
                 data, vdp_reg[23] >> 6, vdp_reg[1]));
    vdp_storedata(data | (data << 8));
    return;
  case 4:
  case 5:
  case 6:
  case 7:

    VDP_LOG(("%08X [VDP] Byte store to CONTROL of %X [%d][%X]",
                 regs.pc, data, vdp_reg[23] >> 6, vdp_reg[1]));
    /* control port */
    vdp_storectrl(data | (data << 8));
    return;
  case 8:
  case 9:
    /* hv counter */
    LOG_CRITICAL(("%08X [VDP] Byte store to hv counter 0x%X", regs.pc, addr));
    return;
  case 17:
    sound_sn76496store(data);
    return;
  default:
    LOG_CRITICAL(("%08X [VDP] Invalid memory store (byte) 0x%X",
                  regs.pc, addr));
    return;
  }
}

void mem68k_store_vdp_word(uint32 addr, uint16 data)
{
  BUS_CHECK("VDP");
  addr -= 0xC00000;
  switch (addr >> 1) {
  case 0:
  case 1:
    /* data port */
    VDP_LOG_CR(("%08X [VDP] Word store to DATA of %X [%d][%X]", regs.pc,
                  data, vdp_reg[23] >> 6, vdp_reg[1]));
    vdp_storedata(data);
    return;
  case 2:
  case 3:
    VDP_LOG(("%08X [VDP] Word store to CONTROL of %X [%d][%X]",
                 regs.pc, data, vdp_reg[23] >> 6, vdp_reg[1]));
    /* control port */
    vdp_storectrl(data);
    return;
  case 4:
    /* hv counter */
    LOG_CRITICAL(("%08X [VDP] Word store to hv counter 0x%X", regs.pc, addr));
    return;
  case 8:
    LOG_CRITICAL(("%08X [VDP] PSG/prohibited word store.", regs.pc));
    return;
  default:
    LOG_CRITICAL(("%08X [VDP] Invalid memory store (word) 0x%X", regs.pc,
                  addr));
    return;
  }
}

void mem68k_store_vdp_long(uint32 addr, uint32 data)
{
  BUS_CHECK("VDP");
  addr -= 0xC00000;
  switch (addr >> 1) {
  case 0:
    /* data port */
    VDP_LOG(("%08X [VDP] Long store to DATA of %X [%d][%X]", regs.pc,
                 data, vdp_reg[23] >> 6, vdp_reg[1]));
    vdp_storedata((uint16)(data >> 16));
    vdp_storedata((uint16)(data));
    return;
  case 2:
    /* control port */
    VDP_LOG(("%08X [VDP] Long store to CONTROL of %X [%d][%X]",
                 regs.pc, data, vdp_reg[23] >> 6, vdp_reg[1]));
    vdp_storectrl((uint16)(data >> 16));
    vdp_storectrl((uint16)(data));
    return;
  case 4:
    /* hv counter */
    LOG_CRITICAL(("%08X [VDP] Long store to hv/prohibited 0x%X", regs.pc,
                  addr));
    return;
  default:
    LOG_CRITICAL(("%08X [VDP] Invalid memory store (long) 0x%X", regs.pc,
                  addr));
    return;
  }
}

