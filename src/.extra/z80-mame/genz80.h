#include <machine.h>
#include <stdio.h>
#include <stdlib.h>
#include <memz80.h>
#include <cpuz80.h>
#define UINT8 uint8
#define UINT16 uint16
#define UINT32 uint32
#define INT8 sint8
#define INT16 sint16
#define INT32 sint32

typedef union {
#ifndef WORDS_BIGENDIAN
  struct { UINT8 l,h,h2,h3; } b;
  struct { UINT16 l,h; } w;
#else
  struct { UINT8 h3,h2,h,l; } b;
  struct { UINT16 h,l; } w;
#endif
  UINT32 d;
}       PAIR;
#define Z80_MAXDAISY 4
/* daisy-chain link */
typedef struct {
  void (*reset)(int);             /* reset callback     */
  int  (*interrupt_entry)(int);   /* entry callback     */
  void (*interrupt_reti)(int);    /* reti callback      */
  int irq_param;                  /* callback paramater */
}       Z80_DaisyChain;
#define INLINE inline
#define errorlog NULL
#define cpu_getactivecpu() 0
#define CLEAR_LINE 0
#define CALL_MAME_DEBUG /* */
#define REG_PREVIOUSPC -1
#define REG_SP_CONTENTS -2
#define Z80_INT_REQ     0x01    /* interrupt request mask       */
#define Z80_INT_IEO     0x02    /* interrupt disable mask(IEO)  */
#define cpu_readmem16(addr) memz80_fetchbyte((uint16)addr)
#define cpu_writemem16(addr,data) memz80_storebyte((uint16)addr, (data));
#define cpu_readop(addr) memz80_fetchbyte((uint16)addr)
#define cpu_readop_arg(addr) memz80_fetchbyte((uint16)addr)
#define cpu_readport(addr) cpuz80_portread((uint16)addr)
#define cpu_writeport(addr, data) cpuz80_portwrite((uint16)addr, (uint8)(data))
#define change_pc(addr) /* */
#define change_pc16(addr) /* */
