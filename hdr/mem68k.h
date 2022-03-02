#ifndef MEM68K_HEADER_FILE
#define MEM68K_HEADER_FILE

typedef enum {
  mem_byte, mem_word, mem_long
} t_memtype;

typedef struct {
  uint16 start;
  uint16 end;
  uint8 *(*memptr)(uint32 addr);
} t_mem68k_def;

typedef struct {
  unsigned int a;
  unsigned int b;
  unsigned int c;
  unsigned int up;
  unsigned int down;
  unsigned int left;
  unsigned int right;
  unsigned int start;
} t_keys;

extern t_keys mem68k_cont[2];

extern int mem68k_init(void);

/* only functions that are too big to inline, or only called in
   erroneous circumstances, are defined here as extern.
   the rest is below as inline funcs. */
extern uint8 mem68k_fetch_bad_byte(uint32 addr);
extern uint16 mem68k_fetch_bad_word(uint32 addr);
extern uint32 mem68k_fetch_bad_long(uint32 addr);
extern uint16 mem68k_fetch_yam_word(uint32 addr);
extern uint32 mem68k_fetch_yam_long(uint32 addr);
extern uint8 mem68k_fetch_bank_byte(uint32 addr);
extern uint16 mem68k_fetch_bank_word(uint32 addr);
extern uint32 mem68k_fetch_bank_long(uint32 addr);
extern uint8 mem68k_fetch_io_byte(uint32 addr);
extern uint16 mem68k_fetch_io_word(uint32 addr);
extern uint32 mem68k_fetch_io_long(uint32 addr);
extern uint32 mem68k_fetch_ctrl_long(uint32 addr);
extern uint16 mem68k_fetch_vdp_word(uint32 addr);
extern uint32 mem68k_fetch_vdp_long(uint32 addr);


extern void mem68k_store_bad_byte(uint32 addr, uint8 data);
extern void mem68k_store_bad_word(uint32 addr, uint16 data);
extern void mem68k_store_bad_long(uint32 addr, uint32 data);
extern void mem68k_store_rom_byte(uint32 addr, uint8 data);
extern void mem68k_store_rom_word(uint32 addr, uint16 data);
extern void mem68k_store_rom_long(uint32 addr, uint32 data);
extern void mem68k_store_yam_word(uint32 addr, uint16 data);
extern void mem68k_store_yam_long(uint32 addr, uint32 data);
extern void mem68k_store_bank_long(uint32 addr, uint32 data);
extern void mem68k_store_io_byte(uint32 addr, uint8 data);
extern void mem68k_store_io_word(uint32 addr, uint16 data);
extern void mem68k_store_io_long(uint32 addr, uint32 data);
extern void mem68k_store_ctrl_byte(uint32 addr, uint8 data);
extern void mem68k_store_ctrl_word(uint32 addr, uint16 data);
extern void mem68k_store_ctrl_long(uint32 addr, uint32 data);
extern void mem68k_store_vdp_byte(uint32 addr, uint8 data);
extern void mem68k_store_vdp_word(uint32 addr, uint16 data);
extern void mem68k_store_vdp_long(uint32 addr, uint32 data);

#ifdef MEM68K_IMPL

#include "ui.h"
#include "cpuz80.h"
#include "gensound.h"

#ifdef DEBUG_BUS
#define BUS_CHECK(S) if (addr & 1) { \
    LOG_CRITICAL(("%08X [" S "] Bus error 0x%X", regs.pc, addr)); \
    return 0; }
#else
#define BUS_CHECK(S) do {} while(0)
#endif

#ifdef DEBUG_SRAM
#define SRAM_LOG(x) LOG_VERBOSE(x)
#else
#define SRAM_LOG(x) do {} while(0)
#endif


static inline uint32 read_long(uint8 *mem, unsigned addr) {
#ifdef ALIGNLONGS
    return (LOCENDIAN16(*(uint16 *)(mem + addr)) << 16) |
      LOCENDIAN16(*(uint16 *)(mem + addr + 2));
#else
    return LOCENDIAN32(*(uint32 *)(mem + addr));
#endif
}

static inline void store_long(uint8 *mem, unsigned addr, uint32 data) {
#ifdef ALIGNLONGS
  *(uint16 *)(mem + addr) = LOCENDIAN16((uint16)(data >> 16));
  *(uint16 *)(mem + addr + 2) = LOCENDIAN16((uint16)(data));
#else
  *(uint32 *)(mem + addr) = LOCENDIAN32(data);
#endif
}

#ifndef UNLIKELY
#ifdef __GNUC__
#define UNLIKELY(x) __builtin_expect((x),0)
#define LIKELY(x) __builtin_expect((x),1)
#else
#define UNLIKELY(x) x
#define LIKELY(x) x
#endif
#endif

/*** ROM fetch ***/

static inline uint8 mem68k_fetch_rom_byte(uint32 addr)
{
  if (LIKELY(addr < cpu68k_romlen)) {
    return (*(uint8 *)(cpu68k_rom + addr));
  }
  LOG_CRITICAL(("%08X [ROM] Invalid memory fetch (byte) 0x%X", regs.pc,
                addr));
  return 0;
}

static inline uint16 mem68k_fetch_rom_word(uint32 addr)
{
  BUS_CHECK("ROM");
  if (LIKELY(addr < cpu68k_romlen)) {
    return LOCENDIAN16(*(uint16 *)(cpu68k_rom + addr));
  }
  LOG_CRITICAL(("%08X [ROM] Invalid memory fetch (word) 0x%X", regs.pc,
                addr));
  return 0;
}

static inline uint32 mem68k_fetch_rom_long(uint32 addr)
{
  BUS_CHECK("ROM");
  if (LIKELY(addr < cpu68k_romlen)) {
    return read_long(cpu68k_rom, addr);
  }
  LOG_CRITICAL(("%08X [ROM] Invalid memory fetch (long) 0x%X", regs.pc,
                addr));
  return 0;
}


/*** RAM fetch/store ***/

static inline uint8 mem68k_fetch_ram_byte(uint32 addr)
{
  addr &= 0xffff;
  return (*(uint8 *)(cpu68k_ram + addr));
}

static inline uint16 mem68k_fetch_ram_word(uint32 addr)
{
  addr &= 0xffff;
  return LOCENDIAN16(*(uint16 *)(cpu68k_ram + addr));
}

static inline uint32 mem68k_fetch_ram_long(uint32 addr)
{
  addr &= 0xffff;
  return read_long(cpu68k_ram, addr);
}

static inline void mem68k_store_ram_byte(uint32 addr, uint8 data)
{
  addr &= 0xffff;
  *(uint8 *)(cpu68k_ram + addr) = data;
  return;
}

static inline void mem68k_store_ram_word(uint32 addr, uint16 data)
{
  addr &= 0xffff;
  *(uint16 *)(cpu68k_ram + addr) = LOCENDIAN16(data);
  return;
}

static inline void mem68k_store_ram_long(uint32 addr, uint32 data)
{
  addr &= 0xffff;
  store_long(cpu68k_ram, addr, data);
  return;
}

/*** SRAM fetch/store ***/

static inline uint8 mem68k_fetch_sram_byte(uint32 addr)
{
  SRAM_LOG(("%08X [SRAM] Fetch byte from %X", regs.pc, addr));
  addr &= 0x1fff;
  return (*(uint8 *)(cpuz80_ram + addr));
}

static inline uint16 mem68k_fetch_sram_word(uint32 addr)
{
  uint8 data;
  BUS_CHECK("SRAM");
  SRAM_LOG(("%08X [SRAM] Fetch word from %X", regs.pc, addr));
  addr &= 0x1fff;
  /* sram word fetches are fetched with duplicated low byte data */
  data = *(uint8 *)(cpuz80_ram + addr);
  return data | (data << 8);
}

static inline uint32 mem68k_fetch_sram_long(uint32 addr)
{
  BUS_CHECK("SRAM");
  SRAM_LOG(("%08X [SRAM] Fetch long from %X", regs.pc, addr));
  addr &= 0x1fff;
  return read_long(cpuz80_ram, addr);
}

static inline void mem68k_store_sram_byte(uint32 addr, uint8 data)
{
  SRAM_LOG(("%08X [SRAM] Store byte to %X", regs.pc, addr));
  addr &= 0x1fff;
  *(uint8 *)(cpuz80_ram + addr) = data;
  return;
}

static inline void mem68k_store_sram_word(uint32 addr, uint16 data)
{
  BUS_CHECK("SRAM");
  SRAM_LOG(("%08X [SRAM] Store word to %X", regs.pc, addr));
  addr &= 0x1fff;
  /* word writes are stored with low byte cleared */
  *(uint8 *)(cpuz80_ram + addr) = data >> 8;
  return;
}

static inline void mem68k_store_sram_long(uint32 addr, uint32 data)
{
  BUS_CHECK("SRAM");
  SRAM_LOG(("%08X [SRAM] Store byte to %X", regs.pc, addr));
  addr &= 0x1fff;
  store_long(cpuz80_ram, addr, data);
  return;
}

/*** YAM valid fetch/store ***/

static inline uint8 mem68k_fetch_yam_byte(uint32 addr)
{
  addr -= 0xA04000;
  /* LOG_USER(("%08X [YAM] fetch (byte) 0x%X", regs.pc, addr)); */
  if (LIKELY(addr < 4))
    return sound_ym2612fetch(addr);
  else {
    LOG_CRITICAL(("%08X [YAM] Invalid YAM fetch (byte) 0x%X", regs.pc, addr));
    return 0;
  }
}

static inline void mem68k_store_yam_byte(uint32 addr, uint8 data)
{
  addr -= 0xA04000;
  /* LOG_USER(("%08X [YAM] (68k) store (byte) 0x%X (%d)", regs.pc, addr,
     data)); */
  if (LIKELY(addr < 4))
    sound_ym2612store(addr, data);
  else
    LOG_CRITICAL(("%08X [YAM] Invalid YAM store (byte) 0x%X", regs.pc, addr));
}

/*** BANK valid store ***/

static inline void mem68k_store_bank_byte(uint32 addr, uint8 data)
{
  addr -= 0xA06000;
  if (LIKELY(addr == 0x000)) {
    SRAM_LOG(("%08X [BANK] Store byte to %X", regs.pc, addr));
    cpuz80_bankwrite(data);
  } else {
    LOG_CRITICAL(("%08X [BANK] Invalid memory store (byte) 0x%X", regs.pc,
                  addr));
  }
}

static inline void mem68k_store_bank_word(uint32 addr, uint16 data)
{
  addr -= 0xA06000;
  if (LIKELY(addr == 0x000)) {
    SRAM_LOG(("%08X [BANK] Store word to %X", regs.pc, addr));
    cpuz80_bankwrite(data >> 8);
  } else {
    LOG_CRITICAL(("%08X [BANK] Invalid memory store (word) 0x%X", regs.pc,
                  addr));
  }
}

/*** CTRL valid fetch ***/

static inline uint8 mem68k_fetch_ctrl_byte(uint32 addr)
{
  addr -= 0xA11000;
  /* 0x000 mode (write only), 0x100 z80 busreq, 0x200 z80 reset (write only) */
  if (LIKELY(addr == 0x100))
    return cpuz80_active ? 1 : 0;

  LOG_CRITICAL(("%08X [CTRL] Invalid memory fetch (byte) 0x%X",
                regs.pc, addr));
  return 0;
}

static inline uint16 mem68k_fetch_ctrl_word(uint32 addr)
{
  addr -= 0xA11000;
  /* 0x000 mode (write only), 0x100 z80 busreq, 0x200 z80 reset (write only) */
  if (LIKELY(addr == 0x100))
    return cpuz80_active ? 0x100 : 0;

  LOG_CRITICAL(("%08X [CTRL] Invalid memory fetch (word) 0x%X",
                regs.pc, addr));
  return 0;
}


/*** VDP simple fetch ***/

static inline uint8 mem68k_fetch_vdp_byte(uint32 addr)
{
  uint16 data = mem68k_fetch_vdp_word(addr & ~1);
  return ((addr & 1) ? (data & 0xff) : ((data >> 8) & 0xff));
}


#define MEM68K_FETCH_SWITCH(OP, MODE) \
	addr &= 0xFFFFFF; \
	switch(addr >> 20) { \
	case 0:	case 1: case 2:	case 3: \
		return mem68k_ ## OP ## _rom_ ## MODE(addr); \
	case 0xC: \
		if(UNLIKELY(((addr >> 12) & 0xf) != 0)) \
			return mem68k_ ## OP ## _bad_ ## MODE(addr); \
		return mem68k_ ## OP ## _vdp_ ## MODE(addr); \
	case 0xE: case 0xF: \
		return mem68k_ ## OP ## _ram_ ## MODE(addr); \
	case 0xA: \
		switch(addr >> 16) { \
		case 0xA0: \
			switch((addr >> 12) & 0xf) { \
			case 4: \
				return mem68k_ ## OP ## _yam_ ## MODE(addr); \
			case 6: \
				return mem68k_ ## OP ## _bank_ ## MODE(addr); \
			case 0xc: \
				return mem68k_ ## OP ## _bad_ ## MODE(addr); \
			} \
			return mem68k_ ## OP ## _sram_ ## MODE(addr); /* z80 */ \
		case 0xA1: \
			switch((addr >> 12) & 0xf) { \
			case 0: \
				return mem68k_ ## OP ## _io_ ## MODE(addr); \
			case 1: \
				return mem68k_ ## OP ## _ctrl_ ## MODE(addr); \
			} \
			return mem68k_ ## OP ## _bad_ ## MODE(addr); \
		} \
		return mem68k_ ## OP ## _bad_ ## MODE(addr); \
	} \
	return mem68k_ ## OP ## _bad_ ## MODE(addr);

static inline uint8 fetchbyte(unsigned addr) {
	MEM68K_FETCH_SWITCH(fetch, byte)
}

static inline uint16 fetchword(unsigned addr) {
	MEM68K_FETCH_SWITCH(fetch, word)
}

static inline uint32 fetchlong(unsigned addr) {
	MEM68K_FETCH_SWITCH(fetch, long)
}

#define MEM68K_STORE_SWITCH(OP, MODE) \
	addr &= 0xFFFFFF; \
	switch(addr >> 20) { \
	case 0:	case 1: case 2:	case 3: \
		return mem68k_ ## OP ## _rom_ ## MODE(addr, data); \
	case 0xC: \
		if(UNLIKELY(((addr >> 12) & 0xf) != 0)) \
			return mem68k_ ## OP ## _bad_ ## MODE(addr, data); \
		return mem68k_ ## OP ## _vdp_ ## MODE(addr, data); \
	case 0xE: case 0xF: \
		return mem68k_ ## OP ## _ram_ ## MODE(addr, data); \
	case 0xA: \
		switch(addr >> 16) { \
		case 0xA0: \
			switch((addr >> 12) & 0xf) { \
			case 4: \
				return mem68k_ ## OP ## _yam_ ## MODE(addr, data); \
			case 6: \
				return mem68k_ ## OP ## _bank_ ## MODE(addr, data); \
			case 0xc: \
				return mem68k_ ## OP ## _bad_ ## MODE(addr, data); \
			} \
			return mem68k_ ## OP ## _sram_ ## MODE(addr, data); /* z80 */ \
		case 0xA1: \
			switch((addr >> 12) & 0xf) { \
			case 0: \
				return mem68k_ ## OP ## _io_ ## MODE(addr, data); \
			case 1: \
				return mem68k_ ## OP ## _ctrl_ ## MODE(addr, data); \
			} \
			return mem68k_ ## OP ## _bad_ ## MODE(addr, data); \
		} \
		return mem68k_ ## OP ## _bad_ ## MODE(addr, data); \
	} \
	return mem68k_ ## OP ## _bad_ ## MODE(addr, data);


static inline void storebyte(uint32 addr, uint8 data) {
	MEM68K_STORE_SWITCH(store, byte)
}

static inline void storeword(uint32 addr, uint16 data) {
	MEM68K_STORE_SWITCH(store, word)
}

static inline void storelong(uint32 addr, uint32 data) {
	MEM68K_STORE_SWITCH(store, long)
}

extern uint8 *(*mem68k_memptr[0x1000])(uint32 addr);

#endif /* MEM68K_IMPL */

#endif /* MEM68K_HEADER_FILE */
