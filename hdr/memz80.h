int memz80_init(void);

extern uint8 (*memz80_fetch_byte[0x100])(uint32 addr);
extern void (*memz80_store_byte[0x100])(uint32 addr, uint8 data);

#define memz80_fetchbyte(addr) memz80_fetch_byte[(addr)>>8](addr)

#define memz80_storebyte(addr,data) memz80_store_byte[(addr)>>8](addr,data)

typedef struct {
  uint16 start;
  uint16 end;
  uint8 (*fetch_byte)(uint32 addr);
  void (*store_byte)(uint32 addr, uint8 data);
} t_memz80_def;
