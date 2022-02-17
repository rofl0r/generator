#ifndef GENERATOR_HEADER_FILE
#define GENERATOR_HEADER_FILE

#include "config.h"

#include "machine.h"

typedef union {
  struct {
#ifndef WORDS_BIGENDIAN 
    unsigned int c:1;
    unsigned int v:1;
    unsigned int z:1;
    unsigned int n:1;
    unsigned int x:1;
    unsigned int :3;
    unsigned int i0:1;
    unsigned int i1:1;
    unsigned int i2:1;
    unsigned int :2;
    unsigned int s:1;
    unsigned int :1;
    unsigned int t:1;
#else
    unsigned int t:1;
    unsigned int :1;
    unsigned int s:1;
    unsigned int :2;
    unsigned int i2:1;
    unsigned int i1:1;
    unsigned int i0:1;
    unsigned int :3;
    unsigned int x:1;
    unsigned int n:1;
    unsigned int z:1;
    unsigned int v:1;
    unsigned int c:1;
#endif
  } sr_struct;
  uint16 sr_int;
} t_sr;

#ifdef INCLUDE_REGISTERS_H
#include "registers.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif /* HAVE_INTTYPES_H */

/* VERSION set by autoconf */
/* PACKAGE set by autoconf */

/*
 * LOCENDIANxx takes data that came from a big endian source and converts it
 * into the local endian.  On a big endian machine the data will already be
 * loaded correctly, however on a little endian machine the processor will
 * have loaded the data assuming little endian data, so we need to swap the
 * byte ordering.
 *
 * LOCENDIANxxL takes data that came from a little endian source and
 * converts it into the local endian.  This means that on a little endian
 * machine the data will already be loaded correctly, however on a big
 * endian machine the processor will have loaded the data assuming big endian
 * data, so we need to swap the byte ordering.
 *
 * Both LOCENDIANxx and LOCENDIANxxL can be used in reverse - i.e. when
 * you have data in local endian that you need to write in big (LOCENDIANxx)
 * or little (LOCENDIANxxL) endian.
 *
 */

#ifdef WORDS_BIGENDIAN
#define LOCENDIAN16(y) (y)
#define LOCENDIAN32(y) (y)
#define LOCENDIAN16L(y) SWAP16(y)
#define LOCENDIAN32L(y) SWAP32(y)
#else
#define LOCENDIAN16(y) SWAP16(y)
#define LOCENDIAN32(y) SWAP32(y)
#define LOCENDIAN16L(y) (y)
#define LOCENDIAN32L(y) (y)
#endif

#define GEN_RAMLENGTH 64*1024

#define LEN_IPCLISTTABLE 16*1024

char *gen_loadimage(const char *filename);
void gen_reset(void);
void gen_softreset(void);
void gen_loadmemrom(const void *rom, int romlen);

typedef enum {
  tp_src, tp_dst
} t_type;

typedef enum {
  sz_none, sz_byte, sz_word, sz_long
} t_size;

typedef enum {
  dt_Dreg, dt_Areg, dt_Aind, dt_Ainc, dt_Adec, dt_Adis, dt_Aidx,
  dt_AbsW, dt_AbsL, dt_Pdis, dt_Pidx,
  dt_ImmB, dt_ImmW, dt_ImmL, dt_ImmS,
  dt_Imm3, dt_Imm4, dt_Imm8, dt_Imm8s, dt_ImmV,
  dt_Ill
} t_datatype;

typedef enum {
  ea_Dreg, ea_Areg, ea_Aind, ea_Ainc, ea_Adec, ea_Adis, ea_Aidx,
  ea_AbsW, ea_AbsL, ea_Pdis, ea_Pidx, ea_Imm
} t_eatypes;

typedef enum {
  i_ILLG,
  i_OR, i_ORSR,
  i_AND, i_ANDSR,
  i_EOR, i_EORSR,
  i_SUB, i_SUBA, i_SUBX,
  i_ADD, i_ADDA, i_ADDX,
  i_MULU, i_MULS,
  i_CMP, i_CMPA,
  i_BTST, i_BCHG, i_BCLR, i_BSET,
  i_MOVE, i_MOVEA,
  i_MOVEPMR, i_MOVEPRM,
  i_MOVEFSR, i_MOVETSR,
  i_MOVEMRM, i_MOVEMMR,
  i_MOVETUSP, i_MOVEFUSP,
  i_NEG, i_NEGX, i_CLR, i_NOT,
  i_ABCD, i_SBCD, i_NBCD,
  i_SWAP,
  i_PEA, i_LEA,
  i_EXT, i_EXG,
  i_TST, i_TAS, i_CHK,
  i_TRAPV, i_TRAP, i_RESET, i_NOP, i_STOP,
  i_LINK, i_UNLK,
  i_RTE, i_RTS, i_RTR,
  i_JSR, i_JMP, i_Scc, i_SF, i_DBcc, i_DBRA, i_Bcc, i_BSR,
  i_DIVU, i_DIVS,
  i_ASR, i_LSR, i_ROXR, i_ROR,
  i_ASL, i_LSL, i_ROXL, i_ROL,
  i_LINE10, i_LINE15
} t_mnemonic;

typedef struct {
  uint16 mask;              /* mask of bits that are static */
  uint16 bits;              /* bit values corresponding to bits in mask */
  t_mnemonic mnemonic;      /* instruction mnemonic */
  struct {
    int priv:1;             /* instruction is privileged if set */
    int endblk:1;           /* instruction ends a block if set */
    int imm_notzero:1;      /* immediate data cannot be 0 (if applicable) */
    int used:5;             /* bitmap of XNZVC flags inspected */
    int set:5;              /* bitmap of XNZVC flags altered */
  } flags;
  t_size size;              /* size of instruction */
  t_datatype stype;         /* type of source */
  t_datatype dtype;         /* type of destination */
  unsigned int sbitpos:4;   /* bit pos of imm data or reg part of EA */
  unsigned int dbitpos:4;   /* reg part of EA */
  unsigned int immvalue;    /* if stype is ImmS this is the value */
  unsigned int cc;          /* condition code if mnemonic is Scc/Dbcc/Bcc */
  unsigned int funcnum;     /* function number for this instruction */
  unsigned int wordlen;     /* length in words of this instruction */
  unsigned int clocks;      /* number of external clock periods */
} t_iib;                    /* instruction information block */

#define IIB_FLAG_X 1<<0
#define IIB_FLAG_N 1<<1
#define IIB_FLAG_Z 1<<2
#define IIB_FLAG_V 1<<3
#define IIB_FLAG_C 1<<4

typedef struct {
  t_mnemonic mnemonic;
  const char *name;
} t_mnemonic_table;

extern t_mnemonic_table mnemonic_table[];

extern char *condition_table[];

typedef struct {
  uint32 pc;
  uint32 sp;
  t_sr sr;
  uint16 stop;
  uint32 regs[16];
  uint16 pending;
} t_regs;

#define SR_CFLAG (1<<0)
#define SR_VFLAG (1<<1)
#define SR_ZFLAG (1<<2)
#define SR_NFLAG (1<<3)
#define SR_XFLAG (1<<4)
#define SR_SFLAG (1<<13)
#define SR_TFLAG (1<<15)

#include <errno.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <stdarg.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <time.h>
#include <limits.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <stdio.h>
#include <signal.h>
#include <assert.h>


/* Steve Snake / Charles MacDonald (msgid <3C427237.3CEC@value.net>) -
 * TAS on Genesis 1 and 2 (but not 3) do not write back with TAS */
#define BROKEN_TAS

#ifdef NOLOGGING
#  define LOG_DEBUG3(x)
#  define LOG_DEBUG2(x)
#  define LOG_DEBUG1(x)
#  define LOG_USER(x)
#  define LOG_VERBOSE(x)
#  define LOG_NORMAL(x)
#  define LOG_CRITICAL(x)
#  define LOG_REQUEST(x)
#else
#  define LOG_DEBUG3(x)   /* ui_log_debug3 x */
#  define LOG_DEBUG2(x)   /* ui_log_debug2 x */
#  define LOG_DEBUG1(x)   /* ui_log_debug1 x */
#  define LOG_USER(x)     ui_log_user x
#  define LOG_VERBOSE(x)  ui_log_verbose x
#  define LOG_NORMAL(x)   ui_log_normal x
#  define LOG_CRITICAL(x) ui_log_critical x
#  define LOG_REQUEST(x)  ui_log_request x
#endif

typedef struct {
  uint8 *sprite;       /* pointer to sprite data or NULL for end of list */
  uint8 hplot;         /* number of cells to plot */
  sint16 hpos, vpos;   /* -128 upwards, top left position */
  uint16 hsize, vsize; /* 1 to 4 for 8 to 32 pixels */
  sint16 hmax, vmax;   /* bottom right position */
} t_spriteinfo;

typedef enum {
  pt_unknown, pt_game, pt_education
} t_prodtype;

typedef struct {
  char console[17];
  char copyright[17];
  char name_domestic[49];
  char name_overseas[49];
  t_prodtype prodtype;
  char version[13];
  uint16 checksum;
  char memo[29];
  char country[17];
  uint8 flag_japan;  /* old style JUE flags */
  uint8 flag_usa;
  uint8 flag_europe;
  uint8 hardware;    /* new style 4-bit bitmap, 0=japan,2=us,3=europe */
} t_cartinfo;

typedef enum {
  musiclog_off, musiclog_gym, musiclog_gnm
} t_musiclog;

extern t_cartinfo gen_cartinfo;

extern unsigned int gen_quit;
extern unsigned int gen_debugmode;
extern unsigned int gen_autodetect;
extern unsigned int gen_modifiedrom;
extern int gen_loglevel;
extern t_musiclog gen_musiclog;
extern char gen_leafname[];

#define SWAP16(y) (( ((y)>>8) & 0x00ff) | (( ((y)<<8) & 0xff00)))
#define SWAP32(y) (( ((y)>>24) & 0x000000ff) | \
  		    (((y) >> 8)  & 0x0000ff00) | \
  		    (((y) << 8)  & 0x00ff0000) | \
          (((y) << 24) & 0xff000000) )

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

#define le_compose(v, byte, i) ((v) | ((byte) << ((i) << 3)))
#define be_compose(v, byte, i) (((v) << 8) | (byte)) 

#define PEEK(bits, endian)                \
static inline uint ## bits                \
peek_ ## endian ## bits(const void *p)    \
{                                         \
  const uint8 *q = (const uint8 *) p;     \
  uint ## bits v;                         \
  int i;                                  \
                                          \
  v = q[0];                               \
  for (i = 1; i < (bits >> 3); i++)       \
    v = endian ## _compose(v, (uint ## bits) q[i], i);   \
                                          \
  return v;                               \
}

PEEK(16, le)
PEEK(32, le)

PEEK(16, be)
PEEK(32, be)
#undef PEEK

#undef le_compose
#undef be_compose

#define POKE_LE(bits)                                   \
static inline void                                      \
poke_le ## bits(void *p, uint ## bits v)                \
{                                                       \
  unsigned int i;                                       \
  uint8 *q = (uint8 *) p;                               \
                                                        \
  for (i = 0; i < (bits >> 3); i++) {                   \
    q[i] = v;                                           \
    v >>= 8;                                            \
  }                                                     \
}

POKE_LE(16)
POKE_LE(32)
#undef POKE_LE

#define POKE_BE(bits)                                   \
static inline void                                      \
poke_be ## bits(void *p, uint ## bits v)                \
{                                                       \
  int i;                                                \
  uint8 *q = (uint8 *) p;                               \
                                                        \
  for (i = (bits >> 3); i-- > 0; /* NOTHING */) {       \
    q[i] = v;                                           \
    v >>= 8;                                            \
  }                                                     \
}

POKE_BE(16)
POKE_BE(32)
#undef POKE_BE

static inline void *
deconstify_void_ptr(const void *p)
{
  return (void *) p;
}

static inline void *
cast_to_void_ptr(void *p)
{
  return p;
}

static inline char *
cast_to_char_ptr(void *p)
{
  return p;
}

static inline char *
print_uint_hex(char *dst, size_t size, unsigned int v)
{
  static const char hexa[] = "0123456789abcdef";
  char buf[21], *p, *q;

  if (size < 1)
    return dst;

  p = &buf[sizeof buf];
  while (--p != buf) {
    *p = hexa[v & 0x0f];
    if (v < 16)
      break;
    v >>= 4;
  }

  for (q = dst; --size > 0 && p < &buf[sizeof buf]; p++) {
    *q++ = *p;
  }
  *q = '\0';
  return q; /* points to trailing NUL */
}

static inline char *
print_ulong_hex(char *dst, size_t size, unsigned long v)
{
  static const char hexa[] = "0123456789abcdef";
  char buf[128], *p, *q;

  if (size < 1)
    return dst;

  p = &buf[sizeof buf];
  while (--p != buf) {
    *p = hexa[v & 0x0f];
    if (v < 16)
      break;
    v >>= 4;
  }

  for (q = dst; --size > 0 && p < &buf[sizeof buf]; p++) {
    *q++ = *p;
  }
  *q = '\0';
  return q; /* points to trailing NUL */
}

static inline char *
print_uint(char *dst, size_t size, unsigned int v)
{
  static const char deca[] = "0123456789";
  char buf[21], *p, *q;

  if (size < 1)
    return dst;

  p = &buf[sizeof buf];
  while (--p != buf) {
    *p = deca[v % 10];
    if (v < 10)
      break;
    v /= 10;
  }

  for (q = dst; --size > 0 && p < &buf[sizeof buf]; p++) {
    *q++ = *p;
  }
  *q = '\0';
  return q; /* points to trailing NUL */
}

static inline char *
append_string(char *dst, size_t *size, const char *src)
{
  size_t left;
  const char *p;
  char *q;
  int c;
  
  q = dst;
  left = *size;
  if (left != 0) {
    for (p = src; (c = *p) != '\0' && --left != 0; ++p, ++q) {
      *q = c;
    }
 
    *q = '\0';
    *size = left;
  }
  return q;
}

static inline char *
append_uint(char *buf, size_t *size, unsigned int v)
{
  char *p;
  
  p = print_uint(buf, *size, v);
  *size -= p - buf;
  return p;
}

static inline char *
append_uint_hex(char *buf, size_t *size, unsigned int v)
{
  char *p;
  
  p = print_uint_hex(buf, *size, v);
  *size -= p - buf;
  return p;
}

static inline char *
append_ulong_hex(char *buf, size_t *size, unsigned long v)
{
  char *p;
  
  p = print_ulong_hex(buf, *size, v);
  *size -= p - buf;
  return p;
}

#endif /* GENERATOR_HEADER_FILE */
/* vi: set ts=2 sw=2 et cindent: */
