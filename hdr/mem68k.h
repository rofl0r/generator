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

extern t_mem68k_def mem68k_def[];
extern unsigned int mem68k_cont1_a;
extern unsigned int mem68k_cont1_b;
extern unsigned int mem68k_cont1_c;
extern unsigned int mem68k_cont1_up;
extern unsigned int mem68k_cont1_down;
extern unsigned int mem68k_cont1_left;
extern unsigned int mem68k_cont1_right;
extern unsigned int mem68k_cont1_start;
extern unsigned int mem68k_cont2_a;
extern unsigned int mem68k_cont2_b;
extern unsigned int mem68k_cont2_c;
extern unsigned int mem68k_cont2_up;
extern unsigned int mem68k_cont2_down;
extern unsigned int mem68k_cont2_left;
extern unsigned int mem68k_cont2_right;
extern unsigned int mem68k_cont2_start;

int mem68k_init(void);

extern uint8 *(*mem68k_memptr[0x1000])(uint32 addr);
extern uint8 (*mem68k_fetch_byte[0x1000])(uint32 addr);
extern uint16 (*mem68k_fetch_word[0x1000])(uint32 addr);
extern uint32 (*mem68k_fetch_long[0x1000])(uint32 addr);
extern void (*mem68k_store_byte[0x1000])(uint32 addr, uint8 data);
extern void (*mem68k_store_word[0x1000])(uint32 addr, uint16 data);
extern void (*mem68k_store_long[0x1000])(uint32 addr, uint32 data);

#define fetchbyte(addr) mem68k_fetch_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)
#define fetchword(addr) mem68k_fetch_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)
#define fetchlong(addr) mem68k_fetch_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)

#define storebyte(addr,data) mem68k_store_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)
#define storeword(addr,data) mem68k_store_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)
#define storelong(addr,data) mem68k_store_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)
