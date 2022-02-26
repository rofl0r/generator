#ifndef MEMZ80_HEADER_FILE
#define MEMZ80_HEADER_FILE

#include "ui.h"
#include "cpuz80.h"
#include "cpu68k.h"
#include "mem68k.h"
#include "gensound.h"

extern int memz80_init(void);

static inline uint8 memz80_fetchbyte(uint16 addr) {
	switch(addr >> 8) {
	/* sram */
	case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
	case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
	case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F:
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
		return (*(uint8 *)(cpuz80_ram + (addr & 0x1fff)));
	/* yam */
	case 0x40:
		addr -= 0x4000;
		if (addr < 4)
			return sound_ym2612fetch(addr);
		LOG_CRITICAL(("[Z80] invalid YAM fetch (byte) 0x%X", addr));
		return 0;
	/* bad byte */
	case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
	bad_byte_r:
		LOG_CRITICAL(("[Z80] Invalid memory fetch (byte) 0x%X", addr));
		return 0;
	/* bank */
	case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
	case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
		/* write only */
		LOG_CRITICAL(("[Z80] Bank fetch (Byte) 0x%X", addr));
		return 0;
	/* bad */
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
	case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E:
		goto bad_byte_r;
	/* psg */
	case 0x7F:
		/* write only */
		LOG_CRITICAL(("[Z80] Invalid PSG fetch (byte) 0x%X", addr));
		return 0;
	/* 0x80 - 0x1000 - main ram */
	default:
		return (fetchbyte(cpuz80_bank | (addr - 0x8000)));
	}
}

static inline void memz80_storebyte(uint16 addr, uint8 data) {
	switch(addr >> 8) {
	/* sram */
	case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
	case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
	case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
	case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F:
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
	case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
		*(uint8 *)(cpuz80_ram + (addr & 0x1fff)) = data;
		return;
	/* yam */
	case 0x40:
		addr -= 0x4000;
		/* LOG_USER(("[YAM] (z80) store (byte) 0x%X (%d)", addr, data)); */
		if (addr < 4)
			sound_ym2612store(addr, data);
		else
			LOG_CRITICAL(("[Z80] invalid YAM store (byte) 0x%X", addr));
		return;
	/* bad byte */
	case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
	bad_byte_w:
		LOG_CRITICAL(("[Z80] Invalid memory store (byte) 0x%X = %X", addr, data));
		return;
	/* bank */
	case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
	case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
		/* set bank */
		cpuz80_bankwrite(data);
		return;
	/* bad */
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
	case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E:
		goto bad_byte_w;
	/* psg */
	case 0x7F:
		if (addr == 0x7f11)
			sound_sn76496store(data);
		else
			LOG_CRITICAL(("[Z80] Invalid memory store (byte) 0x%X", addr));
		return;
	/* 0x8000 - 0x1000 - main ram */
	default:
		/* LOG_USER(("WRITE whilst bank = %08X (%08X)", cpuz80_bank,
			addr-0x8000)); */
		storebyte(cpuz80_bank | (addr - 0x8000), data);
		return;
	}
}

#endif /* MEMZ80_HEADER_FILE */
