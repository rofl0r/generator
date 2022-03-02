#ifndef MEM68K_HEADER_FILE
#define MEM68K_HEADER_FILE

typedef enum {
  mem_byte, mem_word, mem_long
} t_memtype;

typedef struct {
  uint16 start;
  uint16 end;
  uint8 *(*memptr)(uint32 addr);
  uint8 (*fetch_byte)(uint32 addr);
  uint16 (*fetch_word)(uint32 addr);
  uint32 (*fetch_long)(uint32 addr);
  void (*store_byte)(uint32 addr, uint8 data);
  void (*store_word)(uint32 addr, uint16 data);
  void (*store_long)(uint32 addr, uint32 data);
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

extern t_mem68k_def mem68k_def[];
extern t_keys mem68k_cont[2];

int mem68k_init(void);

#ifdef MEM68K_IMPL

extern uint8 mem68k_fetch_bad_byte(uint32 addr);
extern uint16 mem68k_fetch_bad_word(uint32 addr);
extern uint32 mem68k_fetch_bad_long(uint32 addr);
extern uint8 mem68k_fetch_rom_byte(uint32 addr);
extern uint16 mem68k_fetch_rom_word(uint32 addr);
extern uint32 mem68k_fetch_rom_long(uint32 addr);
extern uint8 mem68k_fetch_sram_byte(uint32 addr);
extern uint16 mem68k_fetch_sram_word(uint32 addr);
extern uint32 mem68k_fetch_sram_long(uint32 addr);
extern uint8 mem68k_fetch_yam_byte(uint32 addr);
extern uint16 mem68k_fetch_yam_word(uint32 addr);
extern uint32 mem68k_fetch_yam_long(uint32 addr);
extern uint8 mem68k_fetch_bank_byte(uint32 addr);
extern uint16 mem68k_fetch_bank_word(uint32 addr);
extern uint32 mem68k_fetch_bank_long(uint32 addr);
extern uint8 mem68k_fetch_io_byte(uint32 addr);
extern uint16 mem68k_fetch_io_word(uint32 addr);
extern uint32 mem68k_fetch_io_long(uint32 addr);
extern uint8 mem68k_fetch_ctrl_byte(uint32 addr);
extern uint16 mem68k_fetch_ctrl_word(uint32 addr);
extern uint32 mem68k_fetch_ctrl_long(uint32 addr);
extern uint8 mem68k_fetch_vdp_byte(uint32 addr);
extern uint16 mem68k_fetch_vdp_word(uint32 addr);
extern uint32 mem68k_fetch_vdp_long(uint32 addr);
extern uint8 mem68k_fetch_ram_byte(uint32 addr);
extern uint16 mem68k_fetch_ram_word(uint32 addr);
extern uint32 mem68k_fetch_ram_long(uint32 addr);

extern void mem68k_store_bad_byte(uint32 addr, uint8 data);
extern void mem68k_store_bad_word(uint32 addr, uint16 data);
extern void mem68k_store_bad_long(uint32 addr, uint32 data);
extern void mem68k_store_rom_byte(uint32 addr, uint8 data);
extern void mem68k_store_rom_word(uint32 addr, uint16 data);
extern void mem68k_store_rom_long(uint32 addr, uint32 data);
extern void mem68k_store_sram_byte(uint32 addr, uint8 data);
extern void mem68k_store_sram_word(uint32 addr, uint16 data);
extern void mem68k_store_sram_long(uint32 addr, uint32 data);
extern void mem68k_store_yam_byte(uint32 addr, uint8 data);
extern void mem68k_store_yam_word(uint32 addr, uint16 data);
extern void mem68k_store_yam_long(uint32 addr, uint32 data);
extern void mem68k_store_bank_byte(uint32 addr, uint8 data);
extern void mem68k_store_bank_word(uint32 addr, uint16 data);
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
extern void mem68k_store_ram_byte(uint32 addr, uint8 data);
extern void mem68k_store_ram_word(uint32 addr, uint16 data);
extern void mem68k_store_ram_long(uint32 addr, uint32 data);


#ifndef UNLIKELY
#ifdef __GNUC__
#define UNLIKELY(x) __builtin_expect((x),0)
#else
#define UNLIKELY(x) x
#endif
#endif

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
extern uint8 (*mem68k_fetch_byte[0x1000])(uint32 addr);
extern uint16 (*mem68k_fetch_word[0x1000])(uint32 addr);
extern uint32 (*mem68k_fetch_long[0x1000])(uint32 addr);
extern void (*mem68k_store_byte[0x1000])(uint32 addr, uint8 data);
extern void (*mem68k_store_word[0x1000])(uint32 addr, uint16 data);
extern void (*mem68k_store_long[0x1000])(uint32 addr, uint32 data);

/* XXX BUG: these direct routines do not check for over-run of the 64k
   cpu68k_ram block - so writing a long at $FFFF corrupts 3 bytes of data -
   this is compensated for in the malloc() but is bad nonetheless. */

#endif /* MEM68K_IMPL */

#endif /* MEM68K_HEADER_FILE */
