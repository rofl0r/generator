/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* mem68k.c                                                                  */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>

#include "generator.h"
#include "cpu68k.h"
#include "mem68k.h"
#include "vdp.h"
#include "cpuz80.h"
#include "gensound.h"
#include "ui.h"

#undef DEBUG_BUS
#undef DEBUG_SRAM
#undef DEBUG_RAM
#undef DEBUG_BANK

/*** forward references ***/

uint8 *mem68k_memptr_bad(uint32 addr);
uint8 *mem68k_memptr_rom(uint32 addr);
uint8 *mem68k_memptr_ram(uint32 addr);

uint8  mem68k_fetch_bad_byte(uint32 addr);
uint16 mem68k_fetch_bad_word(uint32 addr);
uint32 mem68k_fetch_bad_long(uint32 addr);
uint8  mem68k_fetch_rom_byte(uint32 addr);
uint16 mem68k_fetch_rom_word(uint32 addr);
uint32 mem68k_fetch_rom_long(uint32 addr);
uint8  mem68k_fetch_sram_byte(uint32 addr);
uint16 mem68k_fetch_sram_word(uint32 addr);
uint32 mem68k_fetch_sram_long(uint32 addr);
uint8  mem68k_fetch_yam_byte(uint32 addr);
uint16 mem68k_fetch_yam_word(uint32 addr);
uint32 mem68k_fetch_yam_long(uint32 addr);
uint8  mem68k_fetch_bank_byte(uint32 addr);
uint16 mem68k_fetch_bank_word(uint32 addr);
uint32 mem68k_fetch_bank_long(uint32 addr);
uint8  mem68k_fetch_io_byte(uint32 addr);
uint16 mem68k_fetch_io_word(uint32 addr);
uint32 mem68k_fetch_io_long(uint32 addr);
uint8  mem68k_fetch_ctrl_byte(uint32 addr);
uint16 mem68k_fetch_ctrl_word(uint32 addr);
uint32 mem68k_fetch_ctrl_long(uint32 addr);
uint8  mem68k_fetch_vdp_byte(uint32 addr);
uint16 mem68k_fetch_vdp_word(uint32 addr);
uint32 mem68k_fetch_vdp_long(uint32 addr);
uint8  mem68k_fetch_ram_byte(uint32 addr);
uint16 mem68k_fetch_ram_word(uint32 addr);
uint32 mem68k_fetch_ram_long(uint32 addr);

void mem68k_store_bad_byte(uint32 addr,  uint8  data);
void mem68k_store_bad_word(uint32 addr,  uint16 data);
void mem68k_store_bad_long(uint32 addr,  uint32 data);
void mem68k_store_rom_byte(uint32 addr,  uint8  data);
void mem68k_store_rom_word(uint32 addr,  uint16 data);
void mem68k_store_rom_long(uint32 addr,  uint32 data);
void mem68k_store_sram_byte(uint32 addr, uint8  data);
void mem68k_store_sram_word(uint32 addr, uint16 data);
void mem68k_store_sram_long(uint32 addr, uint32 data);
void mem68k_store_yam_byte(uint32 addr,  uint8  data);
void mem68k_store_yam_word(uint32 addr,  uint16 data);
void mem68k_store_yam_long(uint32 addr,  uint32 data);
void mem68k_store_bank_byte(uint32 addr, uint8  data);
void mem68k_store_bank_word(uint32 addr, uint16 data);
void mem68k_store_bank_long(uint32 addr, uint32 data);
void mem68k_store_io_byte(uint32 addr,   uint8  data);
void mem68k_store_io_word(uint32 addr,   uint16 data);
void mem68k_store_io_long(uint32 addr,   uint32 data);
void mem68k_store_ctrl_byte(uint32 addr, uint8  data);
void mem68k_store_ctrl_word(uint32 addr, uint16 data);
void mem68k_store_ctrl_long(uint32 addr, uint32 data);
void mem68k_store_vdp_byte(uint32 addr,  uint8  data);
void mem68k_store_vdp_word(uint32 addr,  uint16 data);
void mem68k_store_vdp_long(uint32 addr,  uint32 data);
void mem68k_store_ram_byte(uint32 addr,  uint8  data);
void mem68k_store_ram_word(uint32 addr,  uint16 data);
void mem68k_store_ram_long(uint32 addr,  uint32 data);

unsigned int mem68k_cont1_a = 0;
unsigned int mem68k_cont1_b = 0;
unsigned int mem68k_cont1_c = 0;
unsigned int mem68k_cont1_up = 0;
unsigned int mem68k_cont1_down = 0;
unsigned int mem68k_cont1_left = 0;
unsigned int mem68k_cont1_right = 0;
unsigned int mem68k_cont1_start = 0;

unsigned int mem68k_cont2_a = 0;
unsigned int mem68k_cont2_b = 0;
unsigned int mem68k_cont2_c = 0;
unsigned int mem68k_cont2_up = 0;
unsigned int mem68k_cont2_down = 0;
unsigned int mem68k_cont2_left = 0;
unsigned int mem68k_cont2_right = 0;
unsigned int mem68k_cont2_start = 0;

static unsigned int mem68k_cont1store;
static unsigned int mem68k_cont2store;

/*** memory map ***/

t_mem68k_def mem68k_def[] = {

  { 0x000, 0x400, mem68k_memptr_rom,
    mem68k_fetch_rom_byte, mem68k_fetch_rom_word, mem68k_fetch_rom_long,
    mem68k_store_rom_byte, mem68k_store_rom_word, mem68k_store_rom_long },

  { 0x400, 0x1000, mem68k_memptr_bad,
    mem68k_fetch_bad_byte, mem68k_fetch_bad_word, mem68k_fetch_bad_long,
    mem68k_store_bad_byte, mem68k_store_bad_word, mem68k_store_bad_long },

  { 0xA00, 0xA02, mem68k_memptr_bad,
    mem68k_fetch_sram_byte, mem68k_fetch_sram_word, mem68k_fetch_sram_long,
    mem68k_store_sram_byte, mem68k_store_sram_word, mem68k_store_sram_long },

  { 0xA04, 0xA05, mem68k_memptr_bad,
    mem68k_fetch_yam_byte, mem68k_fetch_yam_word, mem68k_fetch_yam_long,
    mem68k_store_yam_byte, mem68k_store_yam_word, mem68k_store_yam_long },

  { 0xA06, 0xA07, mem68k_memptr_bad,
    mem68k_fetch_bank_byte, mem68k_fetch_bank_word, mem68k_fetch_bank_long,
    mem68k_store_bank_byte, mem68k_store_bank_word, mem68k_store_bank_long },

  { 0xA10, 0xA11, mem68k_memptr_bad,
    mem68k_fetch_io_byte, mem68k_fetch_io_word, mem68k_fetch_io_long,
    mem68k_store_io_byte, mem68k_store_io_word, mem68k_store_io_long },

  { 0xA11, 0xA12, mem68k_memptr_bad,
    mem68k_fetch_ctrl_byte, mem68k_fetch_ctrl_word, mem68k_fetch_ctrl_long,
    mem68k_store_ctrl_byte, mem68k_store_ctrl_word, mem68k_store_ctrl_long },

  { 0xC00, 0xC01, mem68k_memptr_bad,
    mem68k_fetch_vdp_byte, mem68k_fetch_vdp_word, mem68k_fetch_vdp_long,
    mem68k_store_vdp_byte, mem68k_store_vdp_word, mem68k_store_vdp_long },

  { 0xFF0, 0x1000, mem68k_memptr_ram,
    mem68k_fetch_ram_byte, mem68k_fetch_ram_word, mem68k_fetch_ram_long,
    mem68k_store_ram_byte, mem68k_store_ram_word, mem68k_store_ram_long },

  { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

};

uint8 *(*mem68k_memptr[0x1000])(uint32 addr);
uint8 (*mem68k_fetch_byte[0x1000])(uint32 addr);
uint16 (*mem68k_fetch_word[0x1000])(uint32 addr);
uint32 (*mem68k_fetch_long[0x1000])(uint32 addr);
void (*mem68k_store_byte[0x1000])(uint32 addr, uint8 data);
void (*mem68k_store_word[0x1000])(uint32 addr, uint16 data);
void (*mem68k_store_long[0x1000])(uint32 addr, uint32 data);

/*** initialise memory tables ***/

int mem68k_init(void)
{
  int i = 0;
  int j;

  do {
    for (j = mem68k_def[i].start; j < mem68k_def[i].end; j++) {
      mem68k_memptr[j] = mem68k_def[i].memptr;
      mem68k_fetch_byte[j] = mem68k_def[i].fetch_byte;
      mem68k_fetch_word[j] = mem68k_def[i].fetch_word;
      mem68k_fetch_long[j] = mem68k_def[i].fetch_long;
      mem68k_store_byte[j] = mem68k_def[i].store_byte;
      mem68k_store_word[j] = mem68k_def[i].store_word;
      mem68k_store_long[j] = mem68k_def[i].store_long;
    }
    i++;
  } while ((mem68k_def[i].start != 0) || (mem68k_def[i].end != 0));
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
    return(cpu68k_rom+addr);
  }
  LOG_CRITICAL(("%08X [MEM] Invalid memory access to ROM 0x%X", regs.pc,
		addr));
  return cpu68k_rom;
}

uint8 *mem68k_memptr_ram(uint32 addr)
{
  /* LOG_USER(("%08X [MEM] Executing code from RAM 0x%X\n", regs.pc, addr)); */
  addr-= 0xFF0000;
  return(cpu68k_ram+addr);
}


/*** BAD fetch/store ***/

uint8 mem68k_fetch_bad_byte(uint32 addr)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory fetch (byte) 0x%X", regs.pc,	addr));
  return 0;
}

uint16 mem68k_fetch_bad_word(uint32 addr)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory fetch (word) 0x%X", regs.pc, addr));
  return 0;
}

uint32 mem68k_fetch_bad_long(uint32 addr)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory fetch (long) 0x%X", regs.pc, addr));
  return 0;
}

void mem68k_store_bad_byte(uint32 addr, uint8 data)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory store (byte) 0x%X", regs.pc, addr));
}

void mem68k_store_bad_word(uint32 addr, uint16 data)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory store (word) 0x%X", regs.pc, addr));
}

void mem68k_store_bad_long(uint32 addr, uint32 data)
{
  LOG_CRITICAL(("%08X [MEM] Invalid memory store (long) 0x%X", regs.pc, addr));
}


/*** ROM fetch/store ***/

uint8 mem68k_fetch_rom_byte(uint32 addr)
{
  if (addr < cpu68k_romlen) {
    return (*(uint8 *)(cpu68k_rom+addr));
  }
  LOG_CRITICAL(("%08X [ROM] Invalid memory fetch (byte) 0x%X", regs.pc, addr));
  return 0;
}

uint16 mem68k_fetch_rom_word(uint32 addr)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [ROM] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
  if (addr < cpu68k_romlen) {
    return LOCENDIAN16(*(uint16 *)(cpu68k_rom+addr));
  }
  LOG_CRITICAL(("%08X [ROM] Invalid memory fetch (word) 0x%X", regs.pc, addr));
  return 0;
}

uint32 mem68k_fetch_rom_long(uint32 addr)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [ROM] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
  if (addr < cpu68k_romlen) {
#ifdef ALIGNLONGS
    return (LOCENDIAN16(*(uint16 *)(cpu68k_rom+addr))<<16) |
      LOCENDIAN16(*(uint16 *)(cpu68k_rom+addr+2));
#else
    return LOCENDIAN32(*(uint32 *)(cpu68k_rom+addr));
#endif
  }
  LOG_CRITICAL(("%08X [ROM] Invalid memory fetch (long) 0x%X", regs.pc, addr));
  return 0;
}

void mem68k_store_rom_byte(uint32 addr, uint8 data)
{
  LOG_CRITICAL(("%08X [ROM] Invalid memory store (byte) 0x%X", regs.pc, addr));
}

void mem68k_store_rom_word(uint32 addr, uint16 data)
{
  LOG_CRITICAL(("%08X [ROM] Invalid memory store (word) 0x%X", regs.pc, addr));
}

void mem68k_store_rom_long(uint32 addr, uint32 data)
{
  LOG_CRITICAL(("%08X [ROM] Invalid memory store (long) 0x%X", regs.pc, addr));
}


/*** SRAM fetch/store ***/

uint8 mem68k_fetch_sram_byte(uint32 addr)
{
#ifdef DEBUG_SRAM
  LOG_VERBOSE(("%08X [SRAM] Fetch byte from %X", regs.pc, addr));
#endif
  addr-= 0xA00000;
  return (*(uint8 *)(cpuz80_ram+addr));
}

uint16 mem68k_fetch_sram_word(uint32 addr)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [SRAM] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
#ifdef DEBUG_SRAM
  LOG_VERBOSE(("%08X [SRAM] Fetch word from %X", regs.pc, addr));
#endif
  addr-= 0xA00000;
  return LOCENDIAN16(*(uint16 *)(cpuz80_ram+addr));
}

uint32 mem68k_fetch_sram_long(uint32 addr)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [SRAM] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
#ifdef DEBUG_SRAM
  LOG_VERBOSE(("%08X [SRAM] Fetch long from %X", regs.pc, addr));
#endif
  addr-= 0xA00000;
#ifdef ALIGNLONGS
  return (LOCENDIAN16(*(uint16 *)(cpuz80_ram+addr))<<16) |
    LOCENDIAN16(*(uint16 *)(cpuz80_ram+addr+2));
#else
  return LOCENDIAN32(*(uint32 *)(cpuz80_ram+addr));
#endif
}

void mem68k_store_sram_byte(uint32 addr, uint8 data)
{
#ifdef DEBUG_SRAM
  LOG_VERBOSE(("%08X [SRAM] Store byte to %X", regs.pc, addr));
#endif
  addr-= 0xA00000;
  *(uint8 *)(cpuz80_ram+addr) = data;
  return;
}

void mem68k_store_sram_word(uint32 addr, uint16 data)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [SRAM] Bus error 0x%X", regs.pc, addr));
    return;
  }
#endif
#ifdef DEBUG_SRAM
  LOG_VERBOSE(("%08X [SRAM] Store word to %X", regs.pc, addr));
#endif
  addr-= 0xA00000;
  *(uint16 *)(cpuz80_ram+addr) = LOCENDIAN16(data);
  return;
}

void mem68k_store_sram_long(uint32 addr, uint32 data)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [SRAM] Bus error 0x%X", regs.pc, addr));
    return;
  }
#endif
#ifdef DEBUG_SRAM
  LOG_VERBOSE(("%08X [SRAM] Store byte to %X", regs.pc, addr));
#endif
  addr-= 0xA00000;
#ifdef ALIGNLONGS
  *(uint16 *)(cpuz80_ram+addr) = LOCENDIAN16((uint16)(data>>16));
  *(uint16 *)(cpuz80_ram+addr+2) = LOCENDIAN16((uint16)(data));
#else
  *(uint32 *)(cpuz80_ram+addr) = LOCENDIAN32(data);
#endif
  return;
}


/*** YAM fetch/store ***/

uint8 mem68k_fetch_yam_byte(uint32 addr)
{
  addr-= 0xA04000;
  /* LOG_USER(("%08X [YAM] fetch (byte) 0x%X", regs.pc, addr)); */
  if (addr < 4) {
    return sound_ym2612fetch(addr);
  } else {
    LOG_CRITICAL(("%08X [YAM] Invalid YAM fetch (byte) 0x%X", regs.pc, addr));
    return 0;
  }
}

uint16 mem68k_fetch_yam_word(uint32 addr)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [YAM] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
  addr-= 0xA04000;
  LOG_CRITICAL(("%08X [YAM] Invalid memory fetch (word) 0x%X", regs.pc, addr));
  return 0;
}

uint32 mem68k_fetch_yam_long(uint32 addr)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [YAM] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
  addr-= 0xA04000;
  /* no longs please */
  LOG_CRITICAL(("%08X [YAM] Invalid memory fetch (long) 0x%X", regs.pc, addr));
  return 0;
}

void mem68k_store_yam_byte(uint32 addr, uint8 data)
{
  addr-= 0xA04000;
  /* LOG_USER(("%08X [YAM] (68k) store (byte) 0x%X (%d)", regs.pc, addr,
     data)); */
  if (addr < 4)
    sound_ym2612store(addr, data);
  else
    LOG_CRITICAL(("%08X [YAM] Invalid YAM store (byte) 0x%X", regs.pc, addr));
}

void mem68k_store_yam_word(uint32 addr, uint16 data)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [YAM] Bus error 0x%X", regs.pc, addr));
    return;
  }
#endif
  addr-= 0xA04000;
  LOG_CRITICAL(("%08X [YAM] Invalid memory store (word) 0x%X", regs.pc, addr));
}

void mem68k_store_yam_long(uint32 addr, uint32 data)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [YAM] Bus error 0x%X", regs.pc, addr));
    return;
  }
#endif
  addr-= 0xA04000;
  /* no longs please */
  LOG_CRITICAL(("%08X [YAM] Invalid memory store (long) 0x%X", regs.pc, addr));
}


/*** BANK fetch/store ***/

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

void mem68k_store_bank_byte(uint32 addr, uint8 data)
{
  addr-= 0xA06000;
  if (addr == 0x000) {
#ifdef DEBUG_SRAM
    LOG_VERBOSE(("%08X [BANK] Store byte to %X", regs.pc, addr));
#endif
    cpuz80_bankwrite(data);
  } else {
    LOG_CRITICAL(("%08X [BANK] Invalid memory store (byte) 0x%X", regs.pc,
		  addr));
  }
}

void mem68k_store_bank_word(uint32 addr, uint16 data)
{
  addr-= 0xA06000;
  if (addr == 0x000) {
#ifdef DEBUG_SRAM
    LOG_VERBOSE(("%08X [BANK] Store word to %X", regs.pc, addr));
#endif
    cpuz80_bankwrite(data>>8);
  } else {
    LOG_CRITICAL(("%08X [BANK] Invalid memory store (word) 0x%X", regs.pc,
		  addr));
  }
}

void mem68k_store_bank_long(uint32 addr, uint32 data)
{
  addr-= 0xA06000;
  /* no longs please */
  LOG_CRITICAL(("%08X [BANK] Invalid memory store (long) 0x%X", regs.pc,
		addr));
}


/*** I/O fetch/store ***/

uint8 mem68k_fetch_io_byte(uint32 addr)
{
  /* LOG_USER(("%08X [IO] Memory fetch (byte) 0x%X", regs.pc, addr)); */
  addr-= 0xA10000;
  switch (addr) {
  case 0:
    LOG_CRITICAL(("%08X [IO] Invalid memory fetch (byte) 0x%X",
		  regs.pc, addr));
    return 0;
  case 0x1:
    /* version */
    return (1<<5 | vdp_pal<<6 | vdp_overseas<<7);
  case 0x3:
    LOG_USER(("%08X [IO] getting controller 1 state - %02X store",
	      regs.pc, mem68k_cont1store));
    if (mem68k_cont1store & 1<<6) {
      return (1<<6 | (1-mem68k_cont1_up) | (1-mem68k_cont1_down)<<1 |
	      (1-mem68k_cont1_left)<<2 | (1-mem68k_cont1_right)<<3 |
	      (1-mem68k_cont1_b)<<4 | (1-mem68k_cont1_c)<<5);
    } else {
      return ((1-mem68k_cont1_up) | (1-mem68k_cont1_down)<<1 |
	      (1-mem68k_cont1_a)<<4 | (1-mem68k_cont1_start)<<5);
    }
  case 0x5:
    LOG_USER(("%08X [IO] getting controller 2 state - %02X store",
	      regs.pc, mem68k_cont2store));
    if (mem68k_cont2store & 1<<6) {
      return (1<<6 | (1-mem68k_cont2_up) | (1-mem68k_cont2_down)<<1 |
	      (1-mem68k_cont2_left)<<2 | (1-mem68k_cont2_right)<<3 |
	      (1-mem68k_cont2_b)<<4 | (1-mem68k_cont2_c)<<5);
    } else {
      return ((1-mem68k_cont2_up) | (1-mem68k_cont2_down)<<1 |
	      (1-mem68k_cont2_a)<<4 | (1-mem68k_cont2_start)<<5);
    }
  case 0x9:
  case 0xB:
    return 0;
  default:
    LOG_CRITICAL(("%08X [IO] Invalid memory fetch (byte) 0x%X",
		  regs.pc, addr));
    return 0;
  }
}

uint16 mem68k_fetch_io_word(uint32 addr)
{
  /* LOG_USER(("%08X [IO] Memory fetch (word) 0x%X", regs.pc, addr)); */
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [ROM] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
  addr-= 0xA10000;
  switch (addr>>1) {
  case 0: /* version */
    return (1<<5 | vdp_pal<<6 | vdp_overseas<<7);
  case 1: /* data 1 */
    return 0;
  case 2: /* data 2 */
    return 0;
  case 3: /* data E */
    return 0;
  case 4: /* ctrl 1 */
  case 5: /* ctrl 2 */
  case 6: /* ctrl E */
  case 7: /* TxData 1 */
  case 8: /* RxData 1 */
  case 9: /* S-Mode 1 */
  case 10: /* TxData 2 */
  case 11: /* RxData 2 */
  case 12: /* S-Mode 2 */
  case 13: /* TxData E */
  case 14: /* RxData E */
  case 15: /* S-Mode E */
    LOG_CRITICAL(("%08X [IO] Invalid memory fetch (word) 0x%X",
		  regs.pc, addr));
    return 0;
  default:
    LOG_CRITICAL(("%08X [IO] Invalid memory fetch (word) 0x%X",
		  regs.pc, addr));
    return 0;
  }
}

uint32 mem68k_fetch_io_long(uint32 addr)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [ROM] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
  if (addr >= 0xA1001E) {
    LOG_CRITICAL(("%08X [IO] Invalid memory fetch (long) 0x%X",
		  regs.pc, addr));
    return 0;
  }
  return (mem68k_fetch_io_word(addr)<<16) | mem68k_fetch_io_word(addr+2);
}

void mem68k_store_io_byte(uint32 addr, uint8 data)
{
  /* LOG_USER(("%08X [IO] Memory store (byte) 0x%X", regs.pc, addr)); */
  addr-= 0xA10000;
  switch (addr) {
  case 0x3:
    mem68k_cont1store = data;
    return;
  case 0x5:
    mem68k_cont2store = data;
    return;
  case 0x9:
    if (data != 0x40) {
     LOG_CRITICAL(("%08X [IO] Invalid controller 1 setting (0x%X)",
		   regs.pc, data));
    }
    return;
  case 0xB:
    if (data != 0x40) {
      LOG_CRITICAL(("%08X [IO] Invalid controller 2 setting (0x%X)",
		    regs.pc, data));
    }
    return;
  case 0xD:
  case 0xF:
  case 0x11:
  case 0x13:
  case 0x15:
  case 0x17:
  case 0x19:
  case 0x1B:
  case 0x1D:
  case 0x1F:
    return;
  default:
    LOG_CRITICAL(("%08X [IO] Invalid memory store (byte) 0x%X",
		  regs.pc, addr));
    return;
  }
}

void mem68k_store_io_word(uint32 addr, uint16 data)
{
  /* LOG_USER(("%08X [IO] Memory store (word) 0x%X", regs.pc, addr)); */
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [ROM] Bus error 0x%X", regs.pc, addr));
    return;
  }
#endif
  addr-= 0xA10000;
  switch (addr>>1) {
  case 1: /* data 1 */
  case 2: /* data 2 */
  case 3: /* data E */
    LOG_CRITICAL(("%08X [IO] Word store to DATA 0x%X", regs.pc, addr));
    return;
  case 4: /* ctrl 1 */
  case 5: /* ctrl 2 */
  case 6: /* ctrl E */
    LOG_CRITICAL(("%08X [IO] Word store to CTRL 0x%X", regs.pc, addr));
    return;
  case 7: /* TxData 1 */
  case 8: /* RxData 1 */
  case 9: /* S-Mode 1 */
  case 10: /* TxData 2 */
  case 11: /* RxData 2 */
  case 12: /* S-Mode 2 */
  case 13: /* TxData E */
  case 14: /* RxData E */
  case 15: /* S-Mode E */
    LOG_CRITICAL(("%08X [IO] Invalid memory store (word) 0x%X",
		  regs.pc, addr));
    return;
  default:
    LOG_CRITICAL(("%08X [IO] Invalid memory store (word) 0x%X",
		  regs.pc, addr));
  }
}

void mem68k_store_io_long(uint32 addr, uint32 data)
{
  addr-= 0xA10000;
  if (addr >= 0x1E) {
    LOG_CRITICAL(("%08X [IO] Invalid memory store (long) 0x%X",
		  regs.pc, addr));
    return;
  }
  mem68k_store_io_word(addr, (uint16)(data<<16));
  mem68k_store_io_word(addr+2, (uint16)data);
  return;
}


/*** CTRL fetch/store ***/

uint8 mem68k_fetch_ctrl_byte(uint32 addr)
{
  addr-= 0xA11000;
  /* 0x000 mode (write only), 0x100 z80 busreq, 0x200 z80 reset (write only) */
  if (addr == 0x100) {
    return cpuz80_active ? 1 : 0;
  }
  LOG_CRITICAL(("%08X [CTRL] Invalid memory fetch (byte) 0x%X",
		regs.pc, addr));
  return 0;
}

uint16 mem68k_fetch_ctrl_word(uint32 addr)
{
  addr-= 0xA11000;
  /* 0x000 mode (write only), 0x100 z80 busreq, 0x200 z80 reset (write only) */
  if (addr == 0x100) {
    return cpuz80_active ? 0x100 : 0;
  }
  LOG_CRITICAL(("%08X [CTRL] Invalid memory fetch (word) 0x%X",
		regs.pc, addr));
  return 0;
}

uint32 mem68k_fetch_ctrl_long(uint32 addr)
{
  addr-= 0xA11000;
  /* no long access allowed */
  LOG_CRITICAL(("%08X [CTRL] Invalid memory fetch (long) 0x%X",
		regs.pc, addr));
  return 0;
}

void mem68k_store_ctrl_byte(uint32 addr, uint8 data)
{
  addr-= 0xA11000;
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
    return; /* ignore low byte */
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
    return; /* ignore low byte */
  } else {
    LOG_CRITICAL(("%08X [CTRL] Invalid memory store (byte) 0x%X",
		  regs.pc, addr));
  }
}

void mem68k_store_ctrl_word(uint32 addr, uint16 data)
{
  addr-= 0xA11000;
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
  addr-= 0xA11000;
  /* no long access allowed */
  LOG_CRITICAL(("%08X [CTRL] Invalid memory store (long) 0x%X",
		regs.pc, addr));
}


/*** VDP fetch/store ***/

uint8 mem68k_fetch_vdp_byte(uint32 addr)
{
  addr-= 0xC00000;
  switch(addr) {
  case 0:
  case 1:
  case 2:
  case 3:
    /* data port */
    LOG_CRITICAL(("%08X [VDP] Byte fetch from data port 0x%X",
		  regs.pc, addr));
    return 0;
  case 4:
    /* control port */
    return vdp_status()>>8; /* upper 8 bits */
  case 5:
    /* control port */
    return vdp_status(); /* lower 8 bits */
  case 6:
  case 7:
    /* unknown control port */
    LOG_CRITICAL(("%08X [VDP] Byte fetch from control port 0x%X",
		  regs.pc, addr));
    return 0;
  case 8:
    /* hv counter */
    return mem68k_fetch_vdp_word(0xc00008)>>8; /* upper 8 bits */
  case 9:
    /* hv counter */
    return mem68k_fetch_vdp_word(0xc00008); /* lower 8 bits */
  case 17:
    LOG_CRITICAL(("%08X [VDP] PSG byte fetch.", regs.pc));
    return 0;
  default:
    LOG_CRITICAL(("%08X [VDP] Invalid memory fetch (byte) 0x%X",
		  regs.pc, addr));
    return 0;
  }
}

uint16 mem68k_fetch_vdp_word(uint32 addr)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [VDP] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
  addr-= 0xC00000;
  switch(addr>>1) {
  case 0:
  case 1:
    /* data port */
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Word fetch from data port 0x%X",
		 regs.pc, addr));
#endif
    return vdp_fetchdata_word();
  case 2:
  case 3:
    /* control port */
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Word fetch from control port "
		 "(status) 0x%X", regs.pc, addr));
#endif
    return vdp_status();
  case 4:
    /* hv counter */
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Word fetch from hv counter 0x%X",
		 regs.pc, addr));
#endif
    /* we always return 0 for the horizontal position */
    if (vdp_interlace) {
      /* interlace mode - replace lowest bit with highest bit */
      LOG_CRITICAL(("Interlace mode HV read - check this"));
      return ((((vdp_line & ~1)<<8) | (vdp_line & 0x100)) & 0xff00) |
	vdp_gethpos();
    } else {
      /* non-interlace mode */
      LOG_VERBOSE(("%02X", vdp_gethpos()));
      return (vdp_line<<8) | vdp_gethpos();
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
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [VDP] Bus error 0x%X", regs.pc, addr));
    return 0;
  }
#endif
  addr-= 0xC00000;
  switch(addr>>1) {
  case 0:
    /* data port */
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Long fetch from data port 0x%X",
		 regs.pc, addr));
#endif
    return (vdp_fetchdata_word()<<16) | vdp_fetchdata_word();
  case 2:
    /* control port */
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Long fetch from control port 0x%X",
		 regs.pc, addr));
#endif
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
  addr-= 0xC00000;
  switch(addr) {
  case 0:
  case 1:
  case 2:
  case 3:
    /* data port */
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Byte store to data port 0x%X\n", regs.pc, addr));
#endif
    vdp_storedata_byte(data);
    return;
  case 4:
  case 5:
  case 6:
  case 7:
    /* control port */
    LOG_CRITICAL(("%08X [VDP] Byte store to control port 0x%X", regs.pc,
		  addr));
    return;
  case 8:
  case 9:
    /* hv counter */
    LOG_CRITICAL(("%08X [VDP] Byte store to hv counter 0x%X", regs.pc, addr));
    return;
  case 17:
    sound_psgwrite(data);
    return;
  default:
    LOG_CRITICAL(("%08X [VDP] Invalid memory store (byte) 0x%X",
		  regs.pc, addr));
    return;
  }
}

void mem68k_store_vdp_word(uint32 addr, uint16 data)
{
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [VDP] Bus error 0x%X", regs.pc, addr));
    return;
  }
#endif
  addr-= 0xC00000;
  switch(addr>>1) {
  case 0:
  case 1:
    /* data port */
#ifdef DEBUG_VDP
    LOG_CRITICAL(("%08X [VDP] Word store to data port 0x%X", regs.pc, addr));
#endif
    vdp_storedata_word(data);
    return;
  case 2:
  case 3:
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Word store to control port 0x%X", regs.pc, addr));
#endif
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
#ifdef DEBUG_BUS
  if (addr & 1) {
    LOG_CRITICAL(("%08X [VDP] Bus error 0x%X", regs.pc, addr));
    return;
  }
#endif
  addr-= 0xC00000;
  switch(addr>>1) {
  case 0:
    /* data port */
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Long store to data port 0x%X\n", regs.pc, addr));
#endif
    vdp_storedata_word((uint16)(data>>16));
    vdp_storedata_word((uint16)(data));
    return;
  case 2:
    /* control port */
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] Long store to control port 0x%X", regs.pc, addr));
#endif
    vdp_storectrl((uint16)(data>>16));
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


/*** RAM fetch/store ***/

uint8 mem68k_fetch_ram_byte(uint32 addr)
{
  addr-= 0xFF0000;
  return (*(uint8 *)(cpu68k_ram+addr));
}

uint16 mem68k_fetch_ram_word(uint32 addr)
{
  addr-= 0xFF0000;
  return LOCENDIAN16(*(uint16 *)(cpu68k_ram+addr));
}

uint32 mem68k_fetch_ram_long(uint32 addr)
{
  addr-= 0xFF0000;
#ifdef ALIGNLONGS
  return (LOCENDIAN16(*(uint16 *)(cpu68k_ram+addr))<<16) |
    LOCENDIAN16(*(uint16 *)(cpu68k_ram+addr+2));
#else
  return LOCENDIAN32(*(uint32 *)(cpu68k_ram+addr));
#endif
}

void mem68k_store_ram_byte(uint32 addr, uint8 data)
{
  addr-= 0xFF0000;
  *(uint8 *)(cpu68k_ram+addr) = data;
  return;
}

void mem68k_store_ram_word(uint32 addr, uint16 data)
{
  addr-= 0xFF0000;
  *(uint16 *)(cpu68k_ram+addr) = LOCENDIAN16(data);
  return;
}

void mem68k_store_ram_long(uint32 addr, uint32 data)
{
  addr-= 0xFF0000;
#ifdef ALIGNLONGS
  *(uint16 *)(cpu68k_ram+addr) = LOCENDIAN16((uint16)(data>>16));
  *(uint16 *)(cpu68k_ram+addr+2) = LOCENDIAN16((uint16)(data));
#else
  *(uint32 *)(cpu68k_ram+addr) = LOCENDIAN32(data);
#endif
  return;
}
