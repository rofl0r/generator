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

extern uint8 *(*mem68k_memptr[0x1000])(uint32 addr);
extern uint8 (*mem68k_fetch_byte[0x1000])(uint32 addr);
extern uint16 (*mem68k_fetch_word[0x1000])(uint32 addr);
extern uint32 (*mem68k_fetch_long[0x1000])(uint32 addr);
extern void (*mem68k_store_byte[0x1000])(uint32 addr, uint8 data);
extern void (*mem68k_store_word[0x1000])(uint32 addr, uint16 data);
extern void (*mem68k_store_long[0x1000])(uint32 addr, uint32 data);

#define fetchbyte(addr) \
	mem68k_fetch_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)
#define fetchword(addr) \
	mem68k_fetch_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)
#define fetchlong(addr) \
	mem68k_fetch_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)

/* XXX BUG: these direct routines do not check for over-run of the 64k
   cpu68k_ram block - so writing a long at $FFFF corrupts 3 bytes of data -
   this is compensated for in the malloc() but is bad nonetheless. */

#ifdef DIRECTRAM

/* chances are a store is to RAM - optimise for this case */

static __inline__ void storebyte(uint32 addr, uint8 data)
{
  if ((addr & 0xE00000) == 0xE00000) {
    addr&= 0xffff;
    *(uint8 *)(cpu68k_ram + addr) = data;
  } else {
    mem68k_store_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data);
  }
}

static __inline__ void storeword(uint32 addr, uint16 data)
{
  /* in an ideal world we'd check bit 0 of addr, but speed is everything */
  if ((addr & 0xE00000) == 0xE00000) {
    addr&= 0xffff;
    *(uint16 *)(cpu68k_ram + addr) = LOCENDIAN16(data);
  } else {
    mem68k_store_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data);
  }
}

static __inline__ void storelong(uint32 addr, uint32 data)
{
  /* in an ideal world we'd check bit 0 of addr, but speed is everything */
  if ((addr & 0xE00000) == 0xE00000) {
    addr&= 0xffff;
#ifdef ALIGNLONGS
    *(uint16 *)(cpu68k_ram + addr) = LOCENDIAN16((uint16)(data >> 16));
    *(uint16 *)(cpu68k_ram + addr + 2) = LOCENDIAN16((uint16)(data));
#else
    *(uint32 *)(cpu68k_ram + addr) = LOCENDIAN32(data);
#endif
  } else {
    mem68k_store_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data);
  }
}

#else

#define storebyte(addr,data) \
	mem68k_store_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)
#define storeword(addr,data) \
	mem68k_store_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)
#define storelong(addr,data) \
	mem68k_store_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)

#endif

#endif /* MEM68K_HEADER_FILE */
