/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* vdp.c - Video Display Processor                                           */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "generator.h"
#include "vdp.h"
#include "cpu68k.h"
#include "ui.h"

#undef DEBUG_VDP
#undef DEBUG_VDPDMA
#undef DEBUG_VDPDATA
#undef DEBUG_VDPCRAM

#define DEBUG_VDP
#define DEBUG_VDPCRAM
#define DEBUG_VDPDATA
#define DEBUG_VDPDMA

typedef enum {
  cd_vram_fetch, cd_vram_store,
  cd_2, cd_cram_store,
  cd_vsram_fetch, cd_vsram_store,
  cd_6, cd_7,
  cd_cram_fetch, cd_9,
  cd_10, cd_11,
  cd_12, cd_13,
  cd_14, cd_15,
  cd_16, cd_17,
  cd_18, cd_19,
  cd_20, cd_21,
  cd_22, cd_23,
  cd_24, cd_25,
  cd_26, cd_27,
  cd_28, cd_29,
  cd_30, cd_31,
  cd_32, cd_vram_fill,
  cd_invalid
} t_code;

/*** variables externed ***/

unsigned int vdp_event;
unsigned int vdp_vislines;
unsigned int vdp_totlines;
unsigned int vdp_framerate;
unsigned int vdp_clock;
unsigned int vdp_68kclock;
unsigned int vdp_clksperline_68k;
unsigned int vdp_oddframe = 0;   /* odd/even frame */
unsigned int vdp_vblank = 0;     /* set during vertical blanking */
unsigned int vdp_hblank = 0;     /* set during horizontal blanking */
unsigned int vdp_interlace = 0;  /* set in interlace mode */
unsigned int vdp_line = 0;       /* current line number */
unsigned int vdp_vsync = 0;      /* a vsync just happened */
unsigned int vdp_pal = 0;        /* set for pal mode */
unsigned int vdp_overseas = 1;   /* set for overseas model */
unsigned int vdp_layerB  = 1;    /* flag */
unsigned int vdp_layerBp = 1;    /* flag */
unsigned int vdp_layerA  = 1;    /* flag */
unsigned int vdp_layerAp = 1;    /* flag */
unsigned int vdp_layerW  = 1;    /* flag */
unsigned int vdp_layerWp = 1;    /* flag */
unsigned int vdp_layerH  = 1;    /* flag */
unsigned int vdp_layerS  = 1;    /* flag */
unsigned int vdp_layerSp = 1;    /* flag */
uint8 vdp_cram[LEN_CRAM];
uint8 vdp_vsram[LEN_VSRAM];
uint8 vdp_vram[LEN_VRAM];
uint8 vdp_cramf[LEN_CRAM/2];
unsigned int vdp_event_start;
unsigned int vdp_event_vint;
unsigned int vdp_event_hint;
unsigned int vdp_event_hdisplay;
unsigned int vdp_event_end;
unsigned int vdp_event_startofcurrentline;
signed int vdp_hskip_countdown = 0; /* actual countdown */

/*** global variables ***/

uint8 vdp_reg[25];
static int vdp_collision;  /* set during a sprite collision */
static int vdp_overflow;   /* set when too many sprites in one line */
static int vdp_fifofull;   /* set when write fifo full */
static int vdp_fifoempty;  /* set when write fifo empty */
static uint16 vdp_address; /* address for data/dma transfers */
static t_code vdp_code;    /* code number for data/dma transfers */
static int vdp_complex;    /* set when simple routines can't cope */
static unsigned int vdp_ctrlflag; /* set inbetween ctrl writes */

/*** forward references ***/

void vdp_ramcopy_vram(int type);
void vdp_vramcopy(void);
void vdp_showregs(void);
void vdp_describe(void);
void vdp_eventinit(void);
void vdp_layer_simple(unsigned int layer, unsigned int priority,
		      uint8 *fielddata, unsigned int lineoffset);
inline void vdp_plotcell(uint8 *patloc, uint8 palette, uint8 flags,
			 uint8 *cellloc, unsigned int lineoffset);
int vdp_sprite(unsigned int line, unsigned int priority, uint8 *linedata,
	       uint8 *linemask, unsigned int number, uint8 *spritelist,
	       uint8 *sprite);
void vdp_sprites(unsigned int line, unsigned int priority, uint8 *linedata);
int vdp_sprite_simple(unsigned int priority, uint8 *framedata,
		      unsigned int lineoffset, unsigned int number,
		      uint8 *spritelist, uint8 *sprite);
void vdp_sprites_simple(unsigned int priority, uint8 *framedata,
			unsigned int lineoffset);
void vdp_shadow_simple(uint8 *framedata, unsigned int lineoffset);
void vdp_window(unsigned int line, unsigned int priority, uint8 *linedata);

/*** vdp_init - initialise this sub-unit ***/

int vdp_init(void)
{
  vdp_reset();
  return 0;
}

/*** vdp_setupvideo - setup parameters dependant on vdp_pal ***/

void vdp_setupvideo(void)
{
  vdp_clock = vdp_pal ? 53200000 : 53693100; /* What speed is the PAL clock? */
  vdp_68kclock = vdp_clock / 7;
  vdp_vislines = 0 ? 240 : 224;
  vdp_totlines = vdp_pal ? 312 : 262;
  vdp_framerate = vdp_pal ? 50 : 60;
  vdp_clksperline_68k = (vdp_68kclock/vdp_framerate/vdp_totlines);
}

/*** vdp_reset - reset vdp sub-unit ***/

void vdp_reset(void)
{
  int i;

  /* clear registers */

  for (i = 0; i < 25; i++)
    vdp_reg[i] = 0;

  /* set PAL/NTSC variables */

  vdp_setupvideo();
  vdp_vblank = 0;
  vdp_hblank = 0;
  vdp_oddframe = 0;
  vdp_collision = 0;
  vdp_overflow = 0;
  vdp_vsync = 0;
  vdp_fifofull = 0;
  vdp_fifoempty = 0;
  vdp_ctrlflag = 0;

  memset(vdp_cram, 0, LEN_CRAM);
  memset(vdp_vsram, 0, LEN_VSRAM);
  memset(vdp_vram, 0, LEN_VRAM);

  /* clear CRAM */

  for (i = 0; i < 64; i++) {
    (vdp_cram+i*2)[0] = (i & 7)<<1;
    (vdp_cram+i*2)[1] = (i & 7)<<5 | (i & 7)<<1;
    vdp_cramf[i] = 1;
  }
  vdp_eventinit();
  LOG_VERBOSE(("VDP: totlines = %d (%s)", vdp_totlines, vdp_pal ?
	       "PAL" : "NTSC"));
}

uint16 vdp_status(void)
{
  uint16 ret;

  /* bit      meaning (when set)
   * 0        0:ntsc 1:pal
   * 1        dma busy
   * 2        during h blanking
   * 3        during v blanking
   * 4        frame in interlace mode - 0:even 1:odd
   * 5        collision happened between non-zero pixels in two sprites
   * 6        too many sprites in one line
   * 7        v interrupt has happened
   * 8        write fifo full
   * 9        write fifo empty
   */

  ret = vdp_pal | vdp_hblank<<2 | vdp_vblank<<3 | vdp_oddframe<<4 |
    vdp_collision<<5 | vdp_overflow<<6 | vdp_vsync<<7 | vdp_fifofull<<8 |
    vdp_fifoempty<<9;

#ifdef DEBUG_VDP
  if (vdp_collision)
    LOG_VERBOSE(("%08X Collision read", vdp_collision));
#endif

  vdp_vsync = vdp_collision = vdp_overflow = 0;

  LOG_DEBUG1(("%08X STATUS READ %02X", regs.pc, ret));

  return (ret);
}

void vdp_storectrl(uint16 data)
{
  static uint16 first;
  uint8 reg;
  uint8 regdata;

  if (!vdp_ctrlflag) {
    if ((data & 0xE000) == 0x8000) {
      /* register set */
      reg = (data>>8) & 31;
      regdata = data & 255;
      if (reg > 24) {
	LOG_NORMAL(("%08X [VDP] Invalid register (%d)", regs.pc,
		      ((data>>8) & 31)));
	return;
      }
      vdp_reg[reg] = regdata;
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] Register %d set to %04X", regs.pc,
		   reg, regdata));
#endif
      return;
    } else {
      vdp_ctrlflag = 1;
      first = data;
      return;
    }
  } else {
    vdp_ctrlflag = 0;
    vdp_code = ((first>>14) & 3) | ((data>>2) & (15<<2));
    vdp_address = (first & 0x3FFF) | (data<<14 & 0xC000);
    switch(vdp_code) {
    case 0: /* vram read */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] start of vram read to address %X",
		   regs.pc, vdp_address));
#endif
      break;
    case 1: /* vram write */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] start of vram write to address %X",
		   regs.pc, vdp_address));
#endif
      break;
    case 2: /* undefined */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] start of bad transfer to address %X",
		   regs.pc, vdp_address));
#endif
      break;
    case 3: /* cram write */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] start of cram write to address %X",
		   regs.pc, vdp_address));
#endif
      break;
    case 4: /* vsram read */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] start of vsram read to address %X",
		   regs.pc, vdp_address));
#endif
      break;
    case 5: /* vsram write */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] start of vsram read to address %X",
		   regs.pc, vdp_address));
#endif
      break;
    case 6: /* undefined */
    case 7: /* undefined */
      LOG_NORMAL(("%08X [VDP] start of type %d to address %X",
		    regs.pc, vdp_code, vdp_address));
      break;
    case 8: /* cram read */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] start of cram read to address %X",
		   regs.pc, vdp_address));
#endif
      break;
    case 33: /* ram copy to vram/vram fill */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] start of vram copy/fill to address %X",
		   regs.pc, vdp_address));
#endif
      if (!(vdp_reg[23] & 1<<7)) {
	vdp_ramcopy_vram(0);
	vdp_code = cd_invalid;
      }
      break;
    case 35: /* ram copy to cram */
#ifdef DEBUG_VDPCRAM
      LOG_VERBOSE(("%08X [VDP] ram copy to cram address %X",
		   regs.pc, vdp_address));
#endif
      vdp_ramcopy_vram(1);
      vdp_code = cd_invalid;
      break;
    case 37: /* ram copy to vsram */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] ram copy to vsram address %X", regs.pc,
		   vdp_address));
#endif
      vdp_ramcopy_vram(2);
      vdp_code = cd_invalid;
      break;
    case 48: /* vram copy */
#ifdef DEBUG_VDP
      LOG_VERBOSE(("%08X [VDP] vram copy to %X", regs.pc,
		   vdp_address));
#endif
      vdp_vramcopy();
      vdp_code = cd_invalid;
      break;
    default: /* undefined */
      LOG_NORMAL(("%08X [VDP] start of type %d to address %X",
		    regs.pc, vdp_code, vdp_address));
      break;
    }
  }
}

void vdp_ramcopy_vram(int type)
{
  uint16 length = vdp_reg[19] | vdp_reg[20]<<8;
  uint8 srcbank = vdp_reg[23];
  uint16 srcoffset = vdp_reg[21] | vdp_reg[22]<<8;
  uint8 increment = vdp_reg[15];
  uint16 *srcmemory;
  uint16 data;
  uint8 *memory;
  uint16 length_t;
  uint16 vdp_address_t;
  uint32 vdp_address_mask;

  /* VRAM (type 0): reads are word-wide, writes are byte-wide */
  /* CRAM (type 1) WSRAM (type 2): reads are word-wide, writes are byte-wide */

  if (srcbank & 1<<6) {
    /* this is a ram source - note altered beast uses E00000 upwards */
    srcmemory = (uint16 *)cpu68k_ram;
    srcoffset&= 0x7fff;
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] ramcopy from RAM offset %08X type=%d "
		 "vdpaddr=%08X length=%d",
		 regs.pc, srcoffset*2, type, vdp_address, length));
#endif
  } else {
    /* this is a rom source - each bank is 128k, srcoffset is in words */
    srcmemory = (uint16 *)(cpu68k_rom + srcbank*0x20000);
#ifdef DEBUG_VDP
    LOG_VERBOSE(("%08X [VDP] ramcopy from ROM offset %08X type=%d "
		 "vdpaddr=%08X length=%d",
		 regs.pc, srcbank*0x20000+srcoffset*2, type, vdp_address,
		 length));
#endif
  }

  switch(type) {
  case 0:
    memory = vdp_vram;
    vdp_address_mask = LEN_VRAM-1;
    break;
  case 1:
    length&= 0xff; /* maybe this or maybe we just stop at the end */
    memory = vdp_cram;
    vdp_address_mask = LEN_CRAM-1;
    length_t = length;
    vdp_address_t = vdp_address & vdp_address_mask;
    for(; length_t; length_t--) {
      vdp_cramf[vdp_address_t>>1] = 1;
      vdp_address_t+= increment;
      vdp_address_t&= vdp_address_mask;
    }
    break;
  case 2:
    length&= 0xff; /* maybe this or maybe we just stop at the end */
    memory = vdp_vsram;
    vdp_address_mask = LEN_VSRAM-1;
    break;
  default:
    LOG_NORMAL(("%08X [VDP] Bad type %d", regs.pc, type));
    return;
  }
  vdp_address_t = vdp_address & vdp_address_mask;
  for(; length; length--) {
    data = LOCENDIAN16(srcmemory[srcoffset]);
#ifdef DEBUG_VDPDMA
    /*
      LOG_VERBOSE(("%08X [VDP] data %04X (ram/rom+2*%04X) -> %04X",
      regs.pc, data, srcoffset, vdp_address_t));
    */
#endif
    memory[vdp_address_t] = data>>8;
    memory[vdp_address_t+1] = data & 0xff;
    srcoffset+= 1;
    vdp_address_t+= increment;
    vdp_address_t&= vdp_address_mask;
  }
  /* don't think about writing vdp_address_t into registers - T2 is a good
     example of something that doesn't like that */
}

void vdp_vramcopy()
{
  uint16 length = vdp_reg[19] | vdp_reg[20]<<8;
  uint8 increment = vdp_reg[15];
  uint32 dstaddr = vdp_address;
  uint32 srcaddr = vdp_reg[21] | vdp_reg[22]<<8;

#ifdef DEBUG_VDPDMA
  LOG_VERBOSE(("%08X [VDP] COPY length %04X dstaddr %08X inc %02X "
	       "srcaddr %04X", regs.pc, length, dstaddr, increment, srcaddr));
#endif
  /* is this faster, is the question */
  if (increment == 1 && (dstaddr+increment*length) <= LEN_VRAM &&
      (srcaddr+increment <= LEN_VRAM) &&
      (srcaddr < dstaddr ? srcaddr+increment <= dstaddr :
                           dstaddr+increment*length <= srcaddr)) {
    memcpy(vdp_vram+dstaddr, vdp_vram+srcaddr, length);
  } else {
    while(length--) {
      if (dstaddr < LEN_VRAM && srcaddr < LEN_VRAM)
	vdp_vram[dstaddr] = vdp_vram[srcaddr];
      dstaddr++;
      srcaddr+= increment;
    }
  }
  LOG_NORMAL(("VRAM copy is untested"));
}

void vdp_fill(uint16 data)
{
  uint16 length = vdp_reg[19] | vdp_reg[20]<<8;
  uint8 increment = vdp_reg[15];
  uint8 low = data & 0xff;
  uint8 high = (data>>8) & 0xff;
  uint32 addr = vdp_address;

#ifdef DEBUG_VDP
  LOG_DEBUG1(("%08X [VDP] FILL length %04X data %04X addr %08X "
	      "inc %02X", regs.pc, length, data, addr, increment));
#endif
  if (addr + length * increment > LEN_VRAM) {
    LOG_VERBOSE(("%08X [VDP] Bound check failed for vdp_fill: "
		 "addr=%08X to addr=%08X (len=%d,inc=%d)", regs.pc, addr,
		 addr+length*increment, length, increment));
  }
  switch(increment) {
  case 1:
    if (vdp_address & 1) {
      vdp_vram[addr-1] = high;
      if (--length) {
	vdp_vram[addr] = low;
	length--;
      }
      addr++;
    } else {
      vdp_vram[addr] = low;
      if (--length && (addr+1) < LEN_VRAM) {
	vdp_vram[addr+1] = high;
	if (--length)
	  length--;
      }
      addr+= 2;
    }      
    for (; addr < LEN_VRAM && length; length--) {
      vdp_vram[addr++] = high;
    }
    break;
  case 2:
    if (vdp_address & 1) {
      vdp_vram[addr-1] = high;
      vdp_vram[addr] = low;
      length--;
      addr+= 2;
      for (; length && addr < LEN_VRAM; length--) {
	/* changed in 0.14 */
	vdp_vram[addr] = high;
	addr+= 2;
      }
    } else {
      vdp_vram[addr] = low;
      vdp_vram[addr+1] = high;
      length--;
      addr+= 2;
      for (; length && addr < LEN_VRAM; length--) {
	/* changed in 0.14 */
	vdp_vram[addr] = high;
	addr+= 2;
      }
    }
    break;
  default:
    LOG_NORMAL(("%08X [VDP] Unimplemented fill increment of %d",
		  regs.pc, increment));
  }
  /* vdp_address = addr; this can't be right, removed in 0.14 DEBUG DEBUG */
}

uint8 vdp_fetchdata_byte(void)
{
  LOG_NORMAL(("%08X [VDP] Byte fetch from %08X of type %d",
		regs.pc, vdp_address, vdp_code));
  return 0;
}

uint16 vdp_fetchdata_word(void)
{
  uint8 *memory;
  uint32 size;
  uint16 data;

  switch(vdp_code) {
  case cd_vram_fetch:
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] VRAM fetch from address %X", regs.pc,
		 vdp_address));
#endif
    memory = vdp_vram;
    size = LEN_VRAM;
    break;
  case cd_vsram_fetch:
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] VSRAM fetch from address %X",
		 regs.pc, vdp_address));
#endif
    memory = vdp_vsram;
    size = LEN_VSRAM;
    break;
  case cd_cram_fetch:
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] CRAM fetch from address %X", regs.pc,
		 vdp_address));
#endif
    memory = vdp_cram;
    size = LEN_CRAM;
    break;
  default:
    LOG_NORMAL(("%08X [VDP] Fetch in invalid mode %d", regs.pc,
		  vdp_code));
    return 0;
  }
  if (vdp_address>=size) {
    LOG_VERBOSE(("%08X [VDP] Address (%08X) out of bounds (%08X)",
		 regs.pc, vdp_address, size));
  }
  data = LOCENDIAN16(*(uint16 *)(memory+(vdp_address & ~1)));
#ifdef DEBUG_VDPDATA
  LOG_VERBOSE(("%08X [VDP] Fetch %04X from %08X", regs.pc, data,
	       vdp_address & ~1));
#endif
  vdp_address+= vdp_reg[15];
  return data;
}

void vdp_storedata_byte(uint8 data)
{
  LOG_DEBUG2(("%08X [VDP] Store data byte of %02X", regs.pc, data));
  if (vdp_ctrlflag) {
    LOG_NORMAL(("%08X [VDP] Unterminated ctrl setting", regs.pc));
    vdp_storectrl(0);
  }
  switch(vdp_code) {
  case cd_vram_store:
    if (vdp_address >= LEN_VRAM) {
      LOG_VERBOSE(("%08X [VDP] VRAM address %08X out of bounds",
		   regs.pc, vdp_address));
      return;
    }
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] vram byte store %02X in %08X", regs.pc,
		 data, vdp_address ^ 1));
#endif
    *(uint8 *)(vdp_vram+vdp_address) = data;
    vdp_address+= vdp_reg[15];
    return;
  case cd_cram_store:
    if (vdp_address>=LEN_CRAM) {
      LOG_VERBOSE(("%08X [VDP] CRAM address (%08X) out of bounds",
		   regs.pc, vdp_address));
      return;
    }
#ifdef DEBUG_VDPCRAM
    LOG_VERBOSE(("%08X [VDP] CRAM byte store %04X in %08X", regs.pc,
		 data, vdp_address));
#endif
    *(uint8 *)(vdp_cram+vdp_address) = data;
    vdp_cramf[vdp_address>>1] = 1;
    vdp_address+= vdp_reg[15];
    return;
  case cd_vsram_store:
    if (vdp_address>=LEN_VSRAM) {
      LOG_VERBOSE(("%08X [VDP] VSRAM address (%08X) out of bounds",
		   regs.pc, vdp_address));
      return;
    }
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] VSRAM byte store %04X in %08X",
		regs.pc, data, vdp_address & ~1));
#endif
    *(uint8 *)(vdp_vsram+vdp_address) = data;
    vdp_address+= vdp_reg[15];
    return;
  case cd_vram_fill:
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] VRAM byte fill with %04X to %08X",
		 regs.pc, data, vdp_address));
#endif
    vdp_fill(data | (data<<8));
    return;
  default: /* undefined */
    LOG_NORMAL(("%08X [VDP] Byte store to %08X of type %d",
		  regs.pc, vdp_address, vdp_code));
    return;
  }
}

void vdp_storedata_word(uint16 data)
{
  if (vdp_ctrlflag) {
    LOG_NORMAL(("%08X [VDP] Unterminated ctrl setting", regs.pc));
    vdp_storectrl(0);
  }
  switch(vdp_code) {
  case cd_vram_store:
    if (vdp_address>=LEN_VRAM) {
      LOG_VERBOSE(("%08X [VDP] VRAM address (%08X) out of bounds",
		   regs.pc, vdp_address));
      return;
    }
    if (vdp_address & 1)
      data = SWAP16(data);
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] VRAM store %04X in %08X", regs.pc, data,
		 vdp_address & ~1));
#endif
    *(uint16 *)(vdp_vram+(vdp_address & ~1)) = LOCENDIAN16(data);
    vdp_address+= vdp_reg[15];
    return;
  case cd_cram_store:
    if (vdp_address>=LEN_CRAM) {
      LOG_VERBOSE(("%08X [VDP] CRAM address (%08X) out of bounds",
		   regs.pc, vdp_address));
      return;
    }
#ifdef DEBUG_VDPCRAM
    LOG_VERBOSE(("%08X [VDP] CRAM store %04X in %08X", regs.pc, data,
		 vdp_address & ~1));
#endif
    *(uint16 *)(vdp_cram+(vdp_address & ~1)) = LOCENDIAN16(data);
    vdp_cramf[vdp_address>>1] = 1;
    vdp_address+= vdp_reg[15];
    return;
  case cd_vsram_store:
    if (vdp_address>=LEN_VSRAM) {
      LOG_VERBOSE(("%08X [VDP] VSRAM address (%08X) out of bounds",
		   regs.pc, vdp_address));
      return;
    }
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] VSRAM store %04X in %08X", regs.pc, data,
		 vdp_address & ~1));
#endif
    *(uint16 *)(vdp_vsram+(vdp_address & ~1)) = LOCENDIAN16(data);
    vdp_address+= vdp_reg[15];
    return;
  case cd_vram_fill:
#ifdef DEBUG_VDPDATA
    LOG_VERBOSE(("%08X [VDP] VRAM fill with %04X to %08X", regs.pc,
		 data, vdp_address));
#endif
    vdp_fill(data);
    return;
  default: /* undefined */
    LOG_NORMAL(("%08X [VDP] Word store to %08X of type %d data = %04X",
		  regs.pc, vdp_address, vdp_code, data,vdp_address));
    return;
  }
}

#define LINEDATA(offset, value, priority) \
if (value) { \
  linedata[offset] = palette*16 + value; \
} else if (priority) { \
  linedata[offset] = (linedata[offset] & 63); \
}

#define LINEDATASPR(offset, value) \
if (value) { \
  if (linemask[offset]) \
    vdp_collision = 1; \
  linemask[offset] = 1; \
  if ((vdp_reg[12] & 1<<3) && palette == 3 && value == 14) { \
    linedata[offset] = (linedata[offset] & 63) + 64; \
  } else if ((vdp_reg[12] & 1<<3) && palette == 3 && value == 15) { \
    linedata[offset] = (linedata[offset] & 63) + 128; \
  } else { \
    linedata[offset] = palette*16 + value; \
  } \
}

void vdp_sprites(unsigned int line, unsigned int priority, uint8 *linedata)
{
  uint8 interlace = (vdp_reg[12]>>1) & 3;
  uint8 *spritelist = vdp_vram + ((vdp_reg[5] & 0x7F)<<9);
  uint8 linemask[320];
  t_spriteinfo si[128]; /* 128 - max sprites supported per line */
  unsigned int sprites;
  unsigned int idx;
  int i;
  uint8 link;
  uint8 *sprite = spritelist;
  int plotter; /* flag */
  unsigned int screencells = (vdp_reg[12] & 1) ? 40 : 32;
  unsigned int maxspl = (vdp_reg[12] & 1) ? 20 : 16; /* max sprs/line */
  unsigned int cells;
  uint8 loops = 0;

  for (idx = 0; loops < 255 && idx < maxspl; loops++) {
    link = sprite[3] & 0x7F;
    si[idx].sprite = sprite;
    if (interlace == 3)
      si[idx].vpos = (LOCENDIAN16(*(uint16 *)(sprite)) & 0x3FF) - 0x100;
    else
      si[idx].vpos = (LOCENDIAN16(*(uint16 *)(sprite)) & 0x1FF) - 0x080;
    if ((signed int)line < si[idx].vpos)
      goto next;
    si[idx].vsize = 1+(sprite[2] & 3);
    if (interlace == 3)
      si[idx].vsize<<= 1;
    si[idx].vmax = si[idx].vpos + si[idx].vsize*8;
    if ((signed int)line >= si[idx].vmax)
      goto next;
    si[idx].hpos = (LOCENDIAN16(*(uint16 *)(sprite+6)) & 0x1FF) - 0x80;
    si[idx].hsize = 1+((sprite[2]>>2) & 3);
    si[idx].hplot = si[idx].hsize;
    si[idx].hmax = si[idx].hpos + si[idx].hsize*8;
    idx++;
  next:
    if (!link)
      break;
    sprite = spritelist + (link<<3);

  }
  if (idx < 1)
    return;
  sprites = idx;
  memset(linemask, 0, 320); /* initialise mask */
  plotter = 1;
  cells = (vdp_reg[12] & 1) ? 40 : 32; /* 320 or 256 pixels */
  /* loop masking */
  for (i = 0; i < (signed int)sprites; i++) {
    if (plotter == 0) {
      sprites = i;
      break;
    }
    if (si[i].hpos == -128) {
      /* mask sprite - but does it? */
      if (i > 0) {
	/* is there a higher priority sprite? */
	if (si[i-1].vpos <= (signed int)line &&
	    si[i-1].vmax > (signed int)line) {
	  /* match, mask time */
	  plotter = 0;
	}
      } else {
	/* higest priority sprite, so mask */
	plotter = 0;
      }
      si[i].hplot = 0;
    }
    if (cells >= si[i].hplot) {
      cells-= si[i].hplot;
    } else {
      si[i].hplot = cells; /* only room for this many */
      cells = 0;
    }
  }    

  {
    sint16 hpos, vpos;
    uint16 hsize, vsize, hplot;
    sint16 hmax, vmax;
    uint16 cellinfo;
    uint16 pattern;
    uint8 palette;
    uint8 *cellline;
    uint32 data;
    uint8 pixel;
    int j, k;

    /* loop around sprites until end of list marker or no more */
    for (i = sprites-1; i >= 0; i--) {
      hpos = si[i].hpos;
      vpos = si[i].vpos;
      vsize = si[i].vsize; /* doubled when in interlace mode */
      hsize = si[i].hsize;
      hmax = si[i].hmax;
      vmax = si[i].vmax;
      hplot = si[i].hplot; /* number of cells to plot for this sprite */
      cellinfo = LOCENDIAN16(*(uint16 *)(si[i].sprite+4));
      pattern = cellinfo & 0x7FF;
      palette = (cellinfo >> 13) & 3;

      if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
	cellline = vdp_vram + ((interlace == 3) ? (pattern<<6) : (pattern<<5));
	if (cellinfo & 1<<12) /* vertical flip */
	  cellline+= (vmax-line-1)*4;
	else
	  cellline+= (line-vpos)*4;
	for (k = 0; k < hsize && hplot--; k++) {
	  if (hpos > -8 && hpos < 0) {
	    if (cellinfo & 1<<11) {
	      /* horizontal flip */
	      data = LOCENDIAN32(*(uint32 *)(cellline+
					     (hsize-k*2-1)*(vsize<<5)));
	      data>>= (-hpos)*4; /* get first pixel in bottom 4 bits */
	      for (j = 0; j < 8+hpos; j++) {
		pixel = data & 15;
		LINEDATASPR(j, pixel);
		data>>= 4;
	      }
	    } else {
	      data = LOCENDIAN32(*(uint32 *)cellline);
	      data<<= (-hpos)*4; /* get first pixel in top 4 bits */
	      for (j = 0; j < 8+hpos; j++) {
		pixel = (data>>28) & 15;
		LINEDATASPR(j, pixel);
		data<<= 4;
	      }
	    }
	  } else if (hpos >= 0 && hpos <= (signed int)(screencells-1)*8) {
	    if (cellinfo & 1<<11) {
	      /* horizontal flip */
	      data = LOCENDIAN32(*(uint32 *)(cellline+
					     (hsize-k*2-1)*(vsize<<5)));
	      for (j = 0; j < 8; j++) {
		pixel = data & 15;
		LINEDATASPR(hpos+j, pixel);
		data>>= 4;
	      }
	    } else {
	      data = LOCENDIAN32(*(uint32 *)cellline);
	      for (j = 0; j < 8; j++) {
		pixel = (data>>28) & 15;
		LINEDATASPR(hpos+j, pixel);
		data<<= 4;
	      }
	    }
	  } else if (hpos > (signed int)(screencells-1)*8 &&
		     hpos < (signed int)screencells*8) {
	    if (cellinfo & 1<<11) {
	      /* horizontal flip */
	      data = LOCENDIAN32(*(uint32 *)(cellline+
					     (hsize-k*2-1)*(vsize<<5)));
	      for (j = 0; j < (signed int)(screencells*8 - hpos); j++) {
		pixel = data & 15;
		LINEDATASPR(hpos+j, pixel);
		data>>= 4;
	      }
	    } else {
	      data = LOCENDIAN32(*(uint32 *)cellline);
	      for (j = 0; j < (signed int)(screencells*8 - hpos); j++) {
		pixel = (data>>28) & 15;
		LINEDATASPR(hpos+j, pixel);
		data<<= 4;
	      }
	    }
	  }
	  cellline+= vsize<<5; /* 32 bytes per cell (note vsize is doubled
				  when interlaced) */
	  hpos+= 8;
	}
      }
    }
  }
}

void vdp_shadow(unsigned int line, uint8 *linedata)
{
  int i;
  
  /* this could be done 4 bytes at a time */
  for (i = 0; i < 320; i++)
    linedata[i] = (linedata[i] & 63) + 128;
}

void vdp_window(unsigned int line, unsigned int priority, uint8 *linedata)
{
  uint16 *patterndata = (uint16 *)(vdp_vram + ((vdp_reg[3] & 0x3e) << 10));
  uint8 winhpos = vdp_reg[17] & 0x1f;
  uint8 winvpos = vdp_reg[18] & 0x1f;
  uint8 vcell = line / 8;
  uint8 hcell;
  uint8 voffset = line & 7;
  uint8 screencells = (vdp_reg[12] & 1) ? 40 : 32;
  uint8 topbottom = vdp_reg[18] & 0x80;
  uint8 leftright = vdp_reg[17] & 0x80;
  uint8 patternshift = (vdp_reg[12] & 1) ? 6 : 5;
  uint16 cellinfo;
  uint8 *pattern;
  uint32 data;
  uint8 palette;
  uint8 pixel;
  unsigned int i;
  unsigned int wholeline = 0;

  if (topbottom) {
    /* bottom part */
    /* Battle Tank sets winvpos to 0x1F and expects layer A to be off */
    if (winvpos == 0x1F || vcell >= winvpos)
      wholeline = 1;
  } else {
    /* top part */
    if (vcell < winvpos)
      wholeline = 1;
  }
  for (hcell = 0; hcell < screencells; hcell++, linedata+=8) {
    if (!wholeline) {
      if (leftright) {
	if ((hcell>>1) < winhpos)
	  continue;
      } else {
	if ((hcell>>1) >= winhpos)
	  continue;
      }
    }
    cellinfo = LOCENDIAN16(patterndata[vcell<<patternshift | hcell]);
    if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
      palette = (cellinfo >> 13) & 3;
      pattern = vdp_vram + ((cellinfo & 2047)<<5); /* 32 bytes per pattern */
      if (cellinfo & 1<<12) {
	/* vertical flip */
	pattern+= 4*(7-(voffset & 7));
      } else {
	/* no vertical flip */
	pattern+= 4*(voffset & 7);
      }
      data = LOCENDIAN32(*(uint32 *)pattern);
      if (cellinfo & 1<<11) {
	/* horizontal flip */
	for (i = 0; i < 8; i++) {
	  pixel = data & 15;
	  LINEDATA(i, pixel, priority);
	  data>>= 4;
	}
      } else {
	for (i = 0; i < 8; i++) {
	  pixel = (data>>28) & 15;
	  LINEDATA(i, pixel, priority);
	  data<<= 4;
	}
      }
    }
  } /* hcell */
}

/* layer 0 = A (top), layer 1 = B (bottom) */

void vdp_layer(unsigned int line, unsigned int layer, unsigned int priority,
	       uint8 *linedata)
{
  int i, j;
  uint8 hsize = vdp_reg[16] & 3;
  uint8 vsize = (vdp_reg[16] >> 4) & 3;
  uint8 hmode = vdp_reg[11] & 3;
  uint8 vmode = (vdp_reg[11] >> 2) & 1;
  uint16 vramoffset = (layer ? ((vdp_reg[4] & 7) << 13) :
		       ((vdp_reg[2] & (7<<3)) << 10));
  uint16 *patterndata = (uint16 *)(vdp_vram + vramoffset);
  uint16 *hscrolldata = (uint16 *)(((vdp_reg[13] & 63) << 10)
				   + vdp_vram + layer*2);
  uint8 screencells = (vdp_reg[12] & 1) ? 40 : 32;
  uint16 hwidth, vwidth, hoffset, voffset;
  uint16 cellinfo;
  uint8 *pattern;
  uint32 data;
  uint8 palette;
  uint8 pixel;
  uint8 topbottom, leftright, winhpos, winvpos;

  if (layer == 0) {
    /* turn off A when window is on */
    topbottom = vdp_reg[18] & 0x80;
    leftright = vdp_reg[17] & 0x80;
    winhpos = vdp_reg[17] & 0x1f;
    winvpos = vdp_reg[18] & 0x1f;
    /* Battle Tank sets winvpos to 1F and expects layer A to be off */
    if (topbottom && winvpos != 0x1F) {
      if ((line>>3) >= winvpos)
	return;
    } else {
      if ((line>>3) < winvpos)
	return;
    }
  }

  /* BUG: layer A is still displayed left/right when window is on,
     only apparent with high-priority layer A and low-priority window */

  switch(hmode) {
  case 0: /* full screen */
    hoffset = (0x400 - LOCENDIAN16(hscrolldata[0])) & 0x3FF;
    break;
  case 2: /* cell scroll */
    hoffset = (0x400 - LOCENDIAN16(hscrolldata[2*(line & ~7)])) & 0x3FF;
    break;
  case 3: /* line scroll */
    hoffset = (0x400 - LOCENDIAN16(hscrolldata[2*line])) & 0x3FF;
    break;
  default:
    LOG_NORMAL(("prohibited HSCR/LSCR"));
    hoffset = 0;
    break;
  }
  hwidth = 32+hsize*32;
  vwidth = 32+vsize*32;
  hoffset&= (hwidth*8)-1; /* put offset in range */
  voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)[layer] )) & 0x3FF;
  voffset&= (vwidth*8)-1; /* put offset in range */
  cellinfo = LOCENDIAN16(patterndata[(hoffset>>3)+hwidth*(voffset>>3)]);
  if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
    palette = (cellinfo >> 13) & 3;
    pattern = vdp_vram + ((cellinfo & 2047)<<5); /* 32 bytes per pattern */
    if (cellinfo & 1<<12) {
      /* vertical flip */
      pattern+= 4*(7-(voffset & 7));
    } else {
      /* no vertical flip */
      pattern+= 4*(voffset & 7);
    }
    data = LOCENDIAN32(*(uint32 *)pattern);
    if (cellinfo & 1<<11) {
      /* horizontal flip */
      data>>= (hoffset & 7)*4; /* get first pixel in bottom 4 bits */
      for (i = 0; i < 8-(hoffset & 7); i++) {
	pixel = data & 15;
	LINEDATA(i, pixel, priority);
	data>>= 4;
      }
    } else {
      data<<= (hoffset & 7)*4; /* get first pixel in top 4 bits */
      for (i = 0; i < 8-(hoffset & 7); i++) {
	pixel = (data>>28) & 15;
	LINEDATA(i, pixel, priority);
	data<<= 4;
      }
    }
  }
  linedata+= 8-(hoffset & 7);
  hoffset+= 8;
  hoffset&= (hwidth*8)-1; /* put offset in range */
  for (j = 1; j < screencells; j++) {
    if (vmode) {
      /* 2-cell scroll */
      /* DEBUG DEBUG DEBUG awooga awooga */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)
				     [(j&~1)+layer] )) & 0x3FF;
    } else {
      /* full screen */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)[layer] )) & 0x3FF;
    }
    voffset&= (vwidth*8)-1; /* put offset in range */
    cellinfo = LOCENDIAN16(patterndata[(hoffset>>3)+hwidth*(voffset>>3)]);
    /* printf("hoff: %04X voff: %04X hwid: %04X cell: %08X info: %04X\n",
	   hoffset, voffset, hwidth, (hoffset>>3)+hwidth*(voffset>>3),
	   cellinfo); */
    /* printf("loc %08X cellinfo %08X\n",
	   (hoffset>>3)+hwidth*(voffset>>3),
	   cellinfo); */
    if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
      palette = (cellinfo >> 13) & 3;
      pattern = vdp_vram + ((cellinfo & 2047)<<5); /* 32 bytes per pattern */
      if (cellinfo & 1<<12) {
	/* vertical flip */
	pattern+= 4*(7-(voffset & 7));
      } else {
	/* no vertical flip */
	pattern+= 4*(voffset & 7);
      }
      data = LOCENDIAN32(*(uint32 *)pattern);
      if (cellinfo & 1<<11) {
	/* horizontal flip */
	for (i = 0; i < 8; i++) {
	  pixel = data & 15;
	  LINEDATA(i, pixel, priority);
	  data>>= 4;
	}
      } else {
	for (i = 0; i < 8; i++) {
	  pixel = (data>>28) & 15;
	  LINEDATA(i, pixel, priority);
	  data<<= 4;
	}
      }
    }
    linedata+= 8;
    hoffset+= 8;
    hoffset&= (hwidth*8)-1; /* put offset in range */
  }
  if (hoffset & 7) {
    if (vmode) {
      /* 2-cell scroll - note use last offset, not next */
      /* DEBUG DEBUG DEBUG awooga awooga */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)
				     [(j&~1)+layer] )) & 0x3FF;
    } else {
      /* full screen */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)[layer] )) & 0x3FF;
    }
    voffset&= (vwidth*8)-1; /* put offset in range */
    cellinfo = LOCENDIAN16(patterndata[(hoffset>>3)+hwidth*(voffset>>3)]);
    if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
      palette = (cellinfo >> 13) & 3;
      pattern = vdp_vram + ((cellinfo & 2047)<<5); /* 32 bytes per pattern */
      if (cellinfo & 1<<12) {
	/* vertical flip */
	pattern+= 4*(7-(voffset & 7));
      } else {
	/* no vertical flip */
	pattern+= 4*(voffset & 7);
      }
      data = LOCENDIAN32(*(uint32 *)pattern);
      if (cellinfo & 1<<11) {
	/* horizontal flip */
	for (i = 0; i < (hoffset & 7); i++) {
	  pixel = data & 15;
	  LINEDATA(i, pixel, priority);
	  data>>= 4;
	}
      } else {
	for (i = 0; i < (hoffset & 7); i++) {
	  pixel = (data>>28) & 15;
	  LINEDATA(i, pixel, priority);
	  data<<= 4;
	}
      }
    }
    linedata+= 8;
  }
}

void vdp_layer_interlace(unsigned int line, unsigned int layer,
			 unsigned int priority, uint8 *linedata)
{
  int i, j;
  uint8 hsize = vdp_reg[16] & 3;
  uint8 vsize = (vdp_reg[16] >> 4) & 3;
  uint8 hmode = vdp_reg[11] & 3;
  uint8 vmode = (vdp_reg[11] >> 2) & 1;
  uint16 vramoffset = (layer ? ((vdp_reg[4] & 7) << 13) :
		       ((vdp_reg[2] & (7<<3)) << 10));
  uint16 *patterndata = (uint16 *)(vdp_vram + vramoffset);
  uint16 *hscrolldata = (uint16 *)(((vdp_reg[13] & 63) << 10)
				   + vdp_vram + layer*2);
  uint8 screencells = (vdp_reg[12] & 1) ? 40 : 32;
  uint16 hwidth, vwidth, hoffset, voffset;
  uint16 cellinfo;
  uint8 *pattern;
  uint32 data;
  uint8 palette;
  uint8 pixel;
  uint8 topbottom, leftright, winhpos, winvpos;

  if (layer == 0) {
    /* turn off A when window is on */
    topbottom = vdp_reg[18] & 0x80;
    leftright = vdp_reg[17] & 0x80;
    winhpos = vdp_reg[17] & 0x1f;
    winvpos = vdp_reg[18] & 0x1f;
    if (topbottom) {
      if ((line>>4) >= winvpos)
	return;
    } else {
      if ((line>>4) < winvpos)
	return;
    }
  }

  /* BUG: layer A is still displayed left/right when window is on,
     only apparent with high-priority layer A and low-priority window */

  switch(hmode) {
  case 0: /* full screen */
    hoffset = (0x400 - LOCENDIAN16(hscrolldata[0])) & 0x3FF;
    break;
  case 2: /* cell scroll */
    /* interlace we use the actual line for the hscroll */
    hoffset = (0x400 - LOCENDIAN16(hscrolldata[2*((line>>1) & ~7)])) & 0x3FF;
    break;
  case 3: /* line scroll */
    /* interlace we use the actual line for the hscroll */
    hoffset = (0x400 - LOCENDIAN16(hscrolldata[2*(line>>1)])) & 0x3FF;
    break;
  default:
    LOG_NORMAL(("prohibited HSCR/LSCR"));
    hoffset = 0;
    break;
  }
  hwidth = 32+hsize*32;
  vwidth = 32+vsize*32;
  hoffset&= (hwidth*8)-1; /* put offset in range */
  voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)[layer] )) & 0x7FF;
  voffset&= (vwidth*16)-1; /* put offset in range */

  cellinfo = LOCENDIAN16(patterndata[(hoffset>>3)+hwidth*(voffset>>4)]);
  if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
    palette = (cellinfo >> 13) & 3;
    pattern = vdp_vram + ((cellinfo & 2047)<<6); /* 64 bytes per pattern */
    if (cellinfo & 1<<12) {
      /* vertical flip */
      pattern+= 4*(15-(voffset & 15));
    } else {
      /* no vertical flip */
      pattern+= 4*(voffset & 15);
    }
    data = LOCENDIAN32(*(uint32 *)pattern);
    if (cellinfo & 1<<11) {
      /* horizontal flip */
      data>>= (hoffset & 7)*4; /* get first pixel in bottom 4 bits */
      for (i = 0; i < 8-(hoffset & 7); i++) {
	pixel = data & 15;
	LINEDATA(i, pixel, priority);
	data>>= 4;
      }
    } else {
      data<<= (hoffset & 7)*4; /* get first pixel in top 4 bits */
      for (i = 0; i < 8-(hoffset & 7); i++) {
	pixel = (data>>28) & 15;
	LINEDATA(i, pixel, priority);
	data<<= 4;
      }
    }
  }
  linedata+= 8-(hoffset & 7);
  hoffset+= 8;
  hoffset&= (hwidth*8)-1; /* put offset in range */
  for (j = 0; j < screencells-1; j++) {
    if (vmode) {
      /* 2-cell scroll */
      /* DEBUG DEBUG DEBUG awooga awooga */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)
				     [2*(j>>1)+layer] )) & 0x7FF;
    } else {
      /* full screen */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)[layer] )) & 0x7FF;
    }
    voffset&= (vwidth*16)-1; /* put offset in range */
    cellinfo = LOCENDIAN16(patterndata[(hoffset>>3)+hwidth*(voffset>>4)]);
    /* printf("hoff: %04X voff: %04X hwid: %04X cell: %08X info: %04X\n",
	   hoffset, voffset, hwidth, (hoffset>>3)+hwidth*(voffset>>4),
	   cellinfo); */
    /* printf("loc %08X cellinfo %08X\n",
	   (hoffset>>3)+hwidth*(voffset>>4),
	   cellinfo); */
    if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
      palette = (cellinfo >> 13) & 3;
      pattern = vdp_vram + ((cellinfo & 2047)<<6); /* 64 bytes per pattern */
      if (cellinfo & 1<<12) {
	/* vertical flip */
	pattern+= 4*(15-(voffset & 15));
      } else {
	/* no vertical flip */
	pattern+= 4*(voffset & 15);
      }
      data = LOCENDIAN32(*(uint32 *)pattern);
      if (cellinfo & 1<<11) {
	/* horizontal flip */
	for (i = 0; i < 8; i++) {
	  pixel = data & 15;
	  LINEDATA(i, pixel, priority);
	  data>>= 4;
	}
      } else {
	for (i = 0; i < 8; i++) {
	  pixel = (data>>28) & 15;
	  LINEDATA(i, pixel, priority);
	  data<<= 4;
	}
      }
    }
    linedata+= 8;
    hoffset+= 8;
    hoffset&= (hwidth*8)-1; /* put offset in range */
  }
  if (hoffset & 7) {
    if (vmode) {
      /* 2-cell scroll - note use last offset, not next */
      /* DEBUG DEBUG DEBUG awooga awooga */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)
				     [2*(j>>1)+layer] )) & 0x7FF;
    } else {
      /* full screen */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)[layer] )) & 0x7FF;
    }
    voffset&= (vwidth*16)-1; /* put offset in range */
    cellinfo = LOCENDIAN16(patterndata[(hoffset>>3)+hwidth*(voffset>>4)]);
    if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
      palette = (cellinfo >> 13) & 3;
      pattern = vdp_vram + ((cellinfo & 2047)<<6); /* 64 bytes per pattern */
      if (cellinfo & 1<<12) {
	/* vertical flip */
	pattern+= 4*(15-(voffset & 15));
      } else {
	/* no vertical flip */
	pattern+= 4*(voffset & 15);
      }
      data = LOCENDIAN32(*(uint32 *)pattern);
      if (cellinfo & 1<<11) {
	/* horizontal flip */
	for (i = 0; i < (hoffset & 7); i++) {
	  pixel = data & 15;
	  LINEDATA(i, pixel, priority);
	  data>>= 4;
	}
      } else {
	for (i = 0; i < (hoffset & 7); i++) {
	  pixel = (data>>28) & 15;
	  LINEDATA(i, pixel, priority);
	  data<<= 4;
	}
      }
    }
    linedata+= 8;
  }
}

void vdp_renderline(unsigned int line, uint8 *linedata)
{
  int i;
  uint32 background;

  /* fill in background */
  background = vdp_reg[7] & 63;
  background|= background<<8;
  background|= background<<16;

  for (i = 0; i < (320 / 4); i++)
    ((uint32 *)linedata)[i] = background;

  if (vdp_reg[1] & 1<<6) {
    if (vdp_layerB)
      vdp_layer(line, 1, 0, linedata);
    if (vdp_layerA)
      vdp_layer(line, 0, 0, linedata);
    if (vdp_layerW)
      vdp_window(line, 0, linedata);
    if (vdp_layerS)
      vdp_sprites(line,  0, linedata);
    if (vdp_layerH && (vdp_reg[12] & 1<<3))
      vdp_shadow(line, linedata);
    if (vdp_layerBp)
      vdp_layer(line, 1, 1, linedata);
    if (vdp_layerAp)
      vdp_layer(line, 0, 1, linedata);
    if (vdp_layerWp)
      vdp_window(line, 1, linedata);
    if (vdp_layerSp)
      vdp_sprites(line,  1, linedata);
  }
}

void vdp_renderline_interlace2(unsigned int line, uint8 *linedata)
{
  int i;
  uint32 background;

  /* fill in background */
  background = vdp_reg[7] & 63;
  background|= background<<8;
  background|= background<<16;

  for (i = 0; i < (320 / 4); i++)
    ((uint32 *)linedata)[i] = background;

  if (vdp_reg[1] & 1<<6) {
    if (vdp_layerB)
      vdp_layer_interlace(line, 1, 0, linedata);
    if (vdp_layerA)
      vdp_layer_interlace(line, 0, 0, linedata);
    /*
      if (vdp_layerW)
      vdp_window(line, 0, linedata);
    */
    if (vdp_layerS)
      vdp_sprites(line,  0, linedata);
    if (vdp_layerH && (vdp_reg[12] & 1<<3))
      vdp_shadow(line, linedata);
    if (vdp_layerBp)
      vdp_layer_interlace(line, 1, 1, linedata);
    if (vdp_layerAp)
      vdp_layer_interlace(line, 0, 1, linedata);
    /*
      if (vdp_layerWp)
      vdp_window(line, 1, linedata);
    */
    if (vdp_layerSp)
      vdp_sprites(line,  1, linedata);
  }
}

void vdp_renderframe(uint8 *framedata, unsigned int lineoffset)
{
  unsigned int i, line;
  uint32 background;
  unsigned int vertcells = vdp_reg[1] & 1<<3 ? 30 : 28;
  uint8 *linedata;

  /* fill in background */

  background = vdp_reg[7] & 63;
  background|= background<<8;
  background|= background<<16;

  for (line = 0; line < vertcells*8; line++) {
    linedata = framedata + line*lineoffset;
    for (i = 0; i < (320 / 4); i++) {
      ((uint32 *)linedata)[i] = background;
    }
  }

  if (vdp_reg[1] & 1<<6) {
    if (vdp_layerB)
      vdp_layer_simple(1, 0, framedata, lineoffset);
    if (vdp_layerA)
      vdp_layer_simple(0, 0, framedata, lineoffset);
    if (vdp_layerS)
      vdp_sprites_simple(0, framedata, lineoffset);
    if (vdp_layerH && (vdp_reg[12] & 1<<3))
      vdp_shadow_simple(framedata, lineoffset);
    if (vdp_layerBp)
      vdp_layer_simple(1, 1, framedata, lineoffset);
    if (vdp_layerAp)
      vdp_layer_simple(0, 1, framedata, lineoffset);
    if (vdp_layerSp)
      vdp_sprites_simple(1, framedata, lineoffset);
  }
}

void vdp_showregs(void)
{
  int i;

  for (i = 0; i < 25; i++) {
    printf("[%02d] %02X: ", i, vdp_reg[i]);
    switch(i) {
    case 0:
      printf("%s ", vdp_reg[0] & 1<<1 ? "HV-stop" : "HV-enable");
      printf("%s ", vdp_reg[0] & 1<<4 ? "HInt-enable" : "HInt-disable");
      break;
    case 1:
      printf("%s ", vdp_reg[1] & 1<<3 ? "30-cell" : "28-cell");
      printf("%s ", vdp_reg[1] & 1<<4 ? "DMA-enable" : "DMA-disable");
      printf("%s ", vdp_reg[1] & 1<<5 ? "VInt-enable" : "VInt-disable");
      printf("%s ", vdp_reg[1] & 1<<6 ? "Disp-enable" : "Disp-disable");
      break;
    case 2:
      printf("Scroll A @ %04X", (vdp_reg[2] & 0x38)<<10);
      break;
    case 3:
      printf("Window @ %04X", (vdp_reg[3] & 0x3E)<<10);
      break;
    case 4:
      printf("Scroll B @ %04X", (vdp_reg[4] & 7)<<13);
      break;
    case 5:
      printf("Sprites @ %04X", (vdp_reg[5] & 0x7F)<<9);
      break;
    case 7:
      printf("bgpal %d col %d", (vdp_reg[7]>>4 & 3), (vdp_reg[7] & 15));
      break;
    case 10:
      printf("hintreg %04X", vdp_reg[10]);
      break;
    case 11:
      printf("V-mode %d H-mode %d ", (vdp_reg[11]>>2) & 1, (vdp_reg[11] & 3));
      printf("%s", (vdp_reg[11] & 1<<3) ? "ExtInt-enable" : "ExtInt-disable");
      break;
    case 12:
      printf("Interlace %d ", (vdp_reg[12]>>1) & 3);
      printf("%s ", (vdp_reg[12] & 1<<0) ? "40-cell" : "32-cell");
      printf("%s ", (vdp_reg[12] & 1<<3) ? "Shadow-enable" : "Shadow-disable");
      break;
    case 13:
      printf("Scroll A @ %04X", (vdp_reg[13] & 0x3F)<<10);
      break;
    case 15:
      printf("Autoinc %d", vdp_reg[15]);
      break;
    case 16:
      printf("Vsize %d Hsize %d", (vdp_reg[16]>>4) & 3, (vdp_reg[16] & 3));
      break;
    case 17:
      printf("Window H %s ", (vdp_reg[17] & 1<<7) ? "right" : "left");
      printf("%d", vdp_reg[17] & 0x1F);
      break;
    case 18:
      printf("Window V %s ", (vdp_reg[18] & 1<<7) ? "lower" : "upper");
      printf("%d", vdp_reg[18] & 0x1F);
      break;
    case 19:
      printf("DMA-length-low %02X", vdp_reg[19]);
      break;
    case 20:
      printf("DMA-length-high %02X", vdp_reg[20]);
      break;
    case 21:
      printf("DMA-source-low %02X", vdp_reg[21]);
      break;
    case 22:
      printf("DMA-source-mid %02X", vdp_reg[22]);
      break;
    case 23:
      printf("DMA-source-high %02X", vdp_reg[23]);
      break;
    }
    printf("\n");
  }
}

void vdp_spritelist(void)
{
  uint8 *spritelist = vdp_vram + ((vdp_reg[5] & 0x7F)<<9);
  uint8 *sprite;
  uint8 link = 0;
  uint16 pattern;
  uint8 palette;
  uint16 cellinfo;
  sint16 vpos, hpos, vmax;
  uint8 vsize, hsize;

  LOG_REQUEST(("SPRITE DUMP: (base=vram+%X)",
	       (vdp_reg[5] & 0x7f)<<9));
  do {
    sprite = spritelist + (link<<3);
    hpos = (LOCENDIAN16(*(uint16 *)(sprite+6)) & 0x1FF) - 0x80;
    vpos = (LOCENDIAN16(*(uint16 *)(sprite)) & 0x3FF) - 0x80;
    vsize = 1+(sprite[2] & 3);
    hsize = 1+((sprite[2]>>2) & 3);
    cellinfo = LOCENDIAN16(*(uint16 *)(sprite+4));
    pattern = cellinfo & 0x7FF;
    palette = (cellinfo >> 13) & 3;
    vmax = vpos + vsize*8;

    LOG_REQUEST(("Sprite %d @ %X", link,
		 (link<<3) | (vdp_reg[5] & 0x7f)<<9));
    LOG_REQUEST(("  Pos:  %d,%d", hpos, vpos));
    LOG_REQUEST(("  Size: %d,%d", hsize, vsize));
    LOG_REQUEST(("  Pri: %d, Pal: %d, Vflip: %d, Hflip: %d",
		 (cellinfo>>15 & 1), (cellinfo>>13 & 3), (cellinfo>>12 & 1),
		 (cellinfo>>11 & 1)));
    LOG_REQUEST(("  Pattern: %d @ vram+%X", (cellinfo & 0x7FF),
		 (cellinfo & 0x7FF)));
    link = sprite[3] & 0x7F;
  } while (link);
}

void vdp_describe(void)
{
  int layer;
  unsigned int line;
  uint32 o_patterndata, o_hscrolldata;
  uint16 *patterndata, *hscrolldata;
  uint8 hsize = vdp_reg[16] & 3;
  uint8 vsize = (vdp_reg[16] >> 4) & 3;
  uint8 hmode = vdp_reg[11] & 3;
  uint8 vmode = (vdp_reg[11] >> 2) & 1;
  uint16 hwidth, vwidth, hoffset, voffset, raw_hoffset;

  hwidth = 32+hsize*32;
  vwidth = 32+vsize*32;
  LOG_REQUEST(("VDP description:"));
  LOG_REQUEST(("  hsize = %d (ie. width=%d)", hsize, hwidth));
  LOG_REQUEST(("  vsize = %d (ie. width=%d)", vsize, vwidth));
  LOG_REQUEST(("  hmode = %d (0=full, 2=cell, 3=line)", hmode));
  LOG_REQUEST(("  vmode = %d (0=full, 1=2cell", vmode));

  for (layer = 0; layer < 2; layer++) {
    LOG_REQUEST(("  Layer %s:", layer == 0 ? "A" : "B"));
    o_patterndata = (layer == 0 ? ((vdp_reg[2] & 0x38)<<10) :
		     ((vdp_reg[4] & 7)<<13));
    o_hscrolldata = layer*2 + ((vdp_reg[13] & 63)<<10);
    LOG_REQUEST(("    Pattern data @ vram+%08X", o_patterndata));
    LOG_REQUEST(("    Hscroll data @ vram+%08X", o_hscrolldata));
    patterndata = (uint16 *)(vdp_vram + o_patterndata);
    hscrolldata = (uint16 *)(vdp_vram + o_hscrolldata);
    for (line = 0; line < vdp_vislines; line++) {
      switch(hmode) {
      case 0: /* full screen */
	hoffset = (0x400 - LOCENDIAN16(hscrolldata[0])) & 0x3FF;
	break;
      case 2: /* cell scroll */
	hoffset = (0x400 - LOCENDIAN16(hscrolldata[2*(line & ~7)])) & 0x3FF;
	break;
      case 3: /* line scroll */
	hoffset = (0x400 - LOCENDIAN16(hscrolldata[2*line])) & 0x3FF;
	break;
      default:
	LOG_REQUEST(("prohibited HSCR/LSCR on line %d", line));
	hoffset = 0;
	break;
      }
      raw_hoffset = hoffset;
      hoffset&= (hwidth*8)-1; /* put offset in range */
      voffset = (line + LOCENDIAN16( ((uint16 *)vdp_vsram)[layer] )) & 0x3FF;
      voffset&= (vwidth*8)-1; /* put offset in range */
      LOG_REQUEST(("     line %d: hoffset=%d=%d, voffset=%d, "
		   "firstcell=vram+%08X", line, raw_hoffset, hoffset, voffset,
		   o_patterndata+2*((hoffset>>3)+hwidth*(voffset>>3))));
    }
  }
}

void vdp_eventinit(void)
{
  /* Facts from documentation:
       H-Blank is 73.7 clock cycles long.
       The VDP settings are aquired 36 clocks after start of H-Blank.
       The display period is 413.3 clocks in duration.
       V-Int occurs 14.7us after H-Int (which is 112 clock cycles)
     Facts from clock data:
       One line takes 488 clocks (vdp_clksperline_68k)
     Assumptions:
       We 'approximate' and make H-Int occur at the same time as H-Blank.
       V-Blank starts at V-Int and ends at the start of line 0.

     vdp_event_start    = start of line, end of v-blank
     vdp_event_vint     = v-int time on line 224 (or 240)
                                (112 clocks after h-int)
     vdp_event_hint     = h-int time at end of each line
     vdp_event_hdisplay = settings are aquired and current line displayed
     vdp_event_end      = end of line, end of h-blank

     Note that if the program stays in H-Int 224 longer than 112 clocks, V-Int
     is not supposed to occur due to the processor acknowledging the wrong
     interrupt from the VDP, thus programs disable H-Ints on 223 to prevent
     this problem.  We don't worry about this.
  */
  vdp_event = 0;
  vdp_event_start = 0;
  vdp_event_vint = 112-74;
  vdp_event_hint = vdp_clksperline_68k-74;
  vdp_event_hdisplay = vdp_event_hint+36;
  vdp_event_end = vdp_clksperline_68k;
}

void vdp_endfield(void)
{
  vdp_line = 0;
  vdp_eventinit();
  vdp_oddframe^= 1; /* toggle */
  /* printf("(%d,%d,%d,%d,%d)\n", vdp_event_type,
     vdp_event_startline, vdp_event_hint, vdp_event_vdpplot,
     vdp_event_endline); */
}

inline void vdp_plotcell(uint8 *patloc, uint8 palette, uint8 flags,
			 uint8 *cellloc, unsigned int lineoffset)
{
  int y, x;
  uint8 value;
  uint32 data;

  switch(flags) {
  case 0:
    /* normal tile - no s/ten */
    for (y = 0; y < 8; y++, cellloc+= lineoffset) {
      data = LOCENDIAN32(((uint32 *)patloc)[y]);
      for (x = 0; x < 8; x++, data<<= 4) {
	value = data>>28;
	if (value)
	  cellloc[x] = palette*16 + value;
      }
    }
    break;
  case 1:
    /* h flipped tile - no s/ten */
    for (y = 0; y < 8; y++, cellloc+= lineoffset) {
      data = LOCENDIAN32(((uint32 *)patloc)[y]);
      for (x = 0; x < 8; x++, data>>= 4) {
	value = data & 15;
	if (value)
	  cellloc[x] = palette*16 + value;
      }
    }
    break;
  case 2:
    /* v flipped tile - no s/ten */
    for (y = 0; y < 8; y++, cellloc+= lineoffset) {
      data = LOCENDIAN32(((uint32 *)patloc)[7-y]);
      for (x = 0; x < 8; x++, data<<= 4) {
	value = data>>28;
	if (value)
	  cellloc[x] = palette*16 + value;
      }
    }
    break;
  case 3:
    /* h and v flipped tile - no s/ten */
    for (y = 0; y < 8; y++, cellloc+= lineoffset) {
      data = LOCENDIAN32(((uint32 *)patloc)[7-y]);
      for (x = 0; x < 8; x++, data>>= 4) {
	value = data & 15;
	if (value)
	  cellloc[x] = palette*16 + value;
      }
    }
    break;
  case 4:
    /* normal tile - s/ten enabled */
    for (y = 0; y < 8; y++, cellloc+= lineoffset) {
      data = LOCENDIAN32(((uint32 *)patloc)[y]);
      for (x = 0; x < 8; x++, data<<= 4) {
	value = data>>28;
	if (value) {
	  if (palette == 3 && value == 14) {
	    cellloc[x] = (cellloc[x] & 63) + 64;
	  } else if (palette == 3 && value == 15) {
	    cellloc[x] = (cellloc[x] & 63) + 128;
	  } else {
	    cellloc[x] = palette*16 + value;
	  }
	}
      }
    }
    break;
  case 5:
    /* h flipped tile - s/ten */
    for (y = 0; y < 8; y++, cellloc+= lineoffset) {
      data = LOCENDIAN32(((uint32 *)patloc)[y]);
      for (x = 0; x < 8; x++, data>>= 4) {
	value = data & 15;
	if (value) {
	  if (palette == 3 && value == 14) {
	    cellloc[x] = (cellloc[x] & 63) + 64;
	  } else if (palette == 3 && value == 15) {
	    cellloc[x] = (cellloc[x] & 63) + 128;
	  } else {
	    cellloc[x] = palette*16 + value;
	  }
	}
      }
    }
    break;
  case 6:
    /* v flipped tile - s/ten enabled */
    for (y = 0; y < 8; y++, cellloc+= lineoffset) {
      data = LOCENDIAN32(((uint32 *)patloc)[7-y]);
      for (x = 0; x < 8; x++, data<<= 4) {
	value = data>>28;
	if (value) {
	  if (palette == 3 && value == 14) {
	    cellloc[x] = (cellloc[x] & 63) + 64;
	  } else if (palette == 3 && value == 15) {
	    cellloc[x] = (cellloc[x] & 63) + 128;
	  } else {
	    cellloc[x] = palette*16 + value;
	  }
	}
      }
    }
    break;
  case 7:
    /* h and v flipped tile - s/ten enabled */
    for (y = 0; y < 8; y++, cellloc+= lineoffset) {
      data = LOCENDIAN32(((uint32 *)patloc)[7-y]);
      for (x = 0; x < 8; x++, data>>= 4) {
	value = data & 15;
	if (value) {
	  if (palette == 3 && value == 14) {
	    cellloc[x] = (cellloc[x] & 63) + 64;
	  } else if (palette == 3 && value == 15) {
	    cellloc[x] = (cellloc[x] & 63) + 128;
	  } else {
	    cellloc[x] = palette*16 + value;
	  }
	}
      }
    }
    break;
  default:
    ui_err("Unknown plotcell flags");
  }
}

/* must be 8*lineoffset bytes scrap before and after fielddata and also
   8 bytes before each line and 8 bytes after each line */

void vdp_layer_simple(unsigned int layer, unsigned int priority,
		      uint8 *framedata, unsigned int lineoffset)
{
  int i, j;
  uint8 hsize = vdp_reg[16] & 3;
  uint8 vsize = (vdp_reg[16] >> 4) & 3;
  uint8 hmode = vdp_reg[11] & 3;
  uint8 vmode = (vdp_reg[11] >> 2) & 1;
  uint16 vramoffset = (layer ? ((vdp_reg[4] & 7) << 13) :
		       ((vdp_reg[2] & (7<<3)) << 10));
  uint16 *patterndata = (uint16 *)(vdp_vram + vramoffset);
  uint16 *hscrolldata = (uint16 *)(((vdp_reg[13] & 63) << 10)
				   + vdp_vram + layer*2);
  uint8 screencells = (vdp_reg[12] & 1) ? 40 : 32;
  uint16 hwidth = 32+hsize*32;
  uint16 vwidth = 32+vsize*32;
  uint16 hoffset, voffset;
  uint16 cellinfo;
  uint8 *pattern;
  uint32 data;
  uint8 palette;
  uint8 value;
  unsigned int xcell, ycell;
  unsigned int pos;
  uint8 *toploc, *cellloc;
  uint8 flags;
  uint32 hscroll, vscroll;
  unsigned int x, y;

  for (xcell = 0; xcell <= screencells; xcell++) {
    if (vmode) {
      /* 2-cell scroll */
      vscroll = ((xcell >= screencells ? xcell-2 : xcell) & ~1) + layer;
    } else {
      /* full screen */
      vscroll = layer;
    }
    voffset = LOCENDIAN16( ((uint16 *)vdp_vsram)[vscroll] ) & 0x3FF;
    toploc = framedata - lineoffset*(voffset & 7);
    for (ycell = 0; ycell <= 28; ycell++, voffset+=8, toploc+=lineoffset*8) {
      switch(hmode) {
      case 0: /* full screen */
	hscroll = 0;
	break;
      case 2: /* cell scroll */
	hscroll = 2 * (ycell >= 28 ? ycell-2 : ycell) * 8;
	break;
      case 3: /* line scroll - approximation */
	hscroll = 2 * (ycell >= 28 ? ycell-2 : ycell) * 8;
	vdp_complex = 1;
	break;
      default:
	LOG_NORMAL(("prohibited HSCR/LSCR"));
	hscroll = 0;
	break;
      }
      voffset &= (vwidth*8)-1;
      hoffset = (0x400 - LOCENDIAN16(hscrolldata[hscroll])) & 0x3FF;
      hoffset = (hoffset+xcell*8) & ((hwidth*8)-1);
      cellinfo = LOCENDIAN16(patterndata[(hoffset>>3)+hwidth*(voffset>>3)]);
      if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) == priority) {
	/* plot cell */
	palette = (cellinfo >> 13) & 3;
	pattern = vdp_vram + ((cellinfo & 2047)<<5);
	flags = (cellinfo>>11) & 3; /* bit0=H flip, bit1=V flip */
        cellloc = toploc - (hoffset&7) + xcell*8;
	vdp_plotcell(pattern, palette, flags, cellloc, lineoffset);
      }
    } /* ycell */
  } /* xcell */
}

void vdp_shadow_simple(uint8 *framedata, unsigned int lineoffset)
{
  unsigned int vertcells = vdp_reg[1] & 1<<3 ? 30 : 28;
  uint8 *linedata;
  unsigned int line;
  int i;

  for (line = 0; line < vertcells*8; line++) {
    linedata = framedata + line*lineoffset;
    /* this could be done 4 bytes at a time */
    for (i = 0; i < 320; i++)
      linedata[i] = (linedata[i] & 63) + 128;
  }
}

void vdp_sprites_simple(unsigned int priority, uint8 *framedata,
			unsigned int lineoffset)
{
  uint8 *spritelist = vdp_vram + ((vdp_reg[5] & 0x7F)<<9);

  vdp_sprite_simple(priority, framedata, lineoffset, 1,
		    spritelist, spritelist);
}

int vdp_sprite_simple(unsigned int priority, uint8 *framedata,
		      unsigned int lineoffset, unsigned int number,
		      uint8 *spritelist, uint8 *sprite)
{
  int i, j;
  int plotted = 1;
  uint8 screencells = (vdp_reg[12] & 1) ? 40 : 32;
  uint8 link;
  uint16 pattern;
  uint32 data;
  uint8 palette;
  uint16 cellinfo;
  sint16 vpos, hpos, vmax;
  uint16 xcell, ycell;
  uint8 vsize, hsize;
  uint8 *cellloc;
  uint8 flags;
  uint8 *patloc;

  if (number > 80) {
    LOG_VERBOSE(("%08X [VDP] Maximum of 80 sprites exceeded", regs.pc));
    return 0;
  }

  link = sprite[3] & 0x7F;
  hpos = (LOCENDIAN16(*(uint16 *)(sprite+6)) & 0x1FF) - 0x80;
  vpos = (LOCENDIAN16(*(uint16 *)(sprite)) & 0x3FF) - 0x80;
  vsize = 1+(sprite[2] & 3);
  hsize = 1+((sprite[2]>>2) & 3);
  cellinfo = LOCENDIAN16(*(uint16 *)(sprite+4));
  pattern = cellinfo & 0x7FF;
  palette = (cellinfo >> 13) & 3;
  vmax = vpos + vsize*8;
  
  if (link) {
    if (hpos == -128)
      /* we do not support 'masking' in simple mode */
      vdp_complex = 1;
    plotted = vdp_sprite_simple(priority, framedata, lineoffset, number+1,
				spritelist, spritelist + (link<<3));
    plotted++;
  }

  if (((uint8)((cellinfo & 1<<15) ? 1 : 0)) != priority)
    return plotted;
  if (vpos >= 240 || hpos >= 320 || vpos+vsize*8 <= 0 || hpos+hsize*8 <= 0)
    /* sprite is not on screen */
    return plotted;
  flags = (cellinfo>>11) & 3; /* bit0=H flip, bit1=V flip */
  flags|= 1<<2 ; /* s/ten enable */
  switch(flags) {
  case 0:
  case 4:
    /* normal orientation */
    for (ycell = 0; ycell < vsize; ycell++) {
      if (ycell*8+vpos < -7 || ycell*8+vpos >= 240)
	/* cell out of plotting area (remember scrap area) */
	continue;
      patloc = vdp_vram + (pattern<<5) + ycell*32;
      for (xcell = 0; xcell < hsize; xcell++, patloc+= vsize*32) {
	if (xcell*8+hpos < -7 || xcell*8+hpos >= 320)
	  /* cell out of plotting area */
	  continue;
	cellloc = framedata + ((vpos+ycell*8)*lineoffset +
			       (hpos+xcell*8));
	vdp_plotcell(patloc, palette, flags, cellloc, lineoffset);
      }
    }
    break;
  case 1:
  case 5:
    /* H flip */
    for (ycell = 0; ycell < vsize; ycell++) {
      if (ycell*8+vpos < -7 || ycell*8+vpos >= 240)
	/* cell out of plotting area (remember scrap area) */
	continue;
      patloc = vdp_vram + (pattern<<5) + ycell*32 + vsize*32*(hsize-1);
      for (xcell = 0; xcell < hsize; xcell++, patloc-= vsize*32) {
	if (xcell*8+hpos < -7 || xcell*8+hpos >= 320)
	  /* cell out of plotting area */
	  continue;
	cellloc = framedata + ((vpos+ycell*8)*lineoffset +
			       (hpos+xcell*8));
	vdp_plotcell(patloc, palette, flags, cellloc, lineoffset);
      }
    }
    break;
  case 2:
  case 6:
    /* V flip */
    for (ycell = 0; ycell < vsize; ycell++) {
      if (ycell*8+vpos < -7 || ycell*8+vpos >= 240)
	/* cell out of plotting area (remember scrap area) */
	continue;
      patloc = vdp_vram + (pattern<<5) + (vsize-ycell-1)*32;
      for (xcell = 0; xcell < hsize; xcell++, patloc+= vsize*32) {
	if (xcell*8+hpos < -7 || xcell*8+hpos >= 320)
	  /* cell out of plotting area */
	  continue;
	cellloc = framedata + ((vpos+ycell*8)*lineoffset +
			       (hpos+xcell*8));
	vdp_plotcell(patloc, palette, flags, cellloc, lineoffset);
      }
    }
    break;
  case 3:
  case 7:
    /* H and V flip */
    for (ycell = 0; ycell < vsize; ycell++) {
      if (ycell*8+vpos < -7 || ycell*8+vpos >= 240)
	/* cell out of plotting area (remember scrap area) */
	continue;
      patloc = vdp_vram + (pattern<<5) + ((vsize-ycell-1)*32 +
					  vsize*32*(hsize-1));
      for (xcell = 0; xcell < hsize; xcell++, patloc-= vsize*32) {
	if (xcell*8+hpos < -7 || xcell*8+hpos >= 320)
	  /* cell out of plotting area */
	  continue;
	cellloc = framedata + ((vpos+ycell*8)*lineoffset +
			       (hpos+xcell*8));
	vdp_plotcell(patloc, palette, flags, cellloc, lineoffset);
      }
    }
    break;
  }
  return plotted;
}

uint8 vdp_gethpos(void) {
  /* vdp_event = 0/1/2 -> beginning of line until 74 clocks before end
                 3     -> between hint and hdisplay (36 clocks)
                 4     -> between hdisplay and end  (38 clocks)
     This routine goes from 0 to the maximum number allowed not within
     H-blank, and then goes slightly beyond up to hdisplay.  Then
     between hdisplay and end we go negative.  I'm not sure how negative
     it is supposed to be, this goes from -38 to 0.

     this is such a bodge - any changes, check '3 Ninjas kick back'
  */
  LOG_DEBUG1(("gethpos %X: clocks=%X : startofline=%X : hint=%X : "
	      "end=%X", vdp_event, cpu68k_clocks, vdp_event_startofcurrentline,
	      vdp_event_hint, vdp_event_end));
  if (vdp_event == 1 || vdp_event == 2) {
    if (cpu68k_clocks > vdp_event_hint)
      return (vdp_reg[12] & 1) ? 0x9f : 0x7f;
    return (((vdp_reg[12] & 1) ? 0x9f : 0x7f)*
	    ((float)(cpu68k_clocks-vdp_event_startofcurrentline)/
	     (float)(vdp_event_hint-vdp_event_startofcurrentline)));
  }
  LOG_DEBUG1(("gethpos %d %d %02X", vdp_event_end, cpu68k_clocks,
	      vdp_event_end-cpu68k_clocks));
  if (cpu68k_clocks > vdp_event_end)
    return 0xff;
  return -(vdp_event_end-cpu68k_clocks);
}
