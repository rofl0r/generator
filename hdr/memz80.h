#ifndef MEMZ80_HEADER_FILE
#define MEMZ80_HEADER_FILE

#include "ui.h"
#include "cpuz80.h"
#include "cpu68k.h"
#include "mem68k.h"
#include "gensound.h"

extern int memz80_init(void);

static uint8 memz80_invalid_read(uint16 addr, const char *reason) {
	LOG_CRITICAL(("[Z80] invalid %s fetch (byte) 0x%X", reason, addr));
	return 0;
}

static void memz80_invalid_store(uint16 addr, const char *reason) {
	LOG_CRITICAL(("[Z80] invalid %s store (byte) 0x%X", reason, addr));
}

#ifndef UNLIKELY
#ifdef __GNUC__
#define UNLIKELY(x)     __builtin_expect((x),0)
#else
#define UNLIKELY(x) x
#endif
#endif

static inline uint8 memz80_fetchbyte(uint16 addr) {
	switch(addr >> 12) {
	/* sram */
	case 0x00: case 0x01: case 0x02: case 0x03:
		return (*(uint8 *)(cpuz80_ram + (addr & 0x1fff)));
	case 0x04:
		addr -= 0x4000;
		if(UNLIKELY(addr >= 4)) {
			addr += 0x4000;
			if((addr >> 8) == 0x40)
				return memz80_invalid_read(addr, "YAM");
			return memz80_invalid_read(addr, "memory");
		}
		return sound_ym2612fetch(addr);
	case 0x05:
	bad_byte_r:
		return memz80_invalid_read(addr, "memory");
	/* bank */
	case 0x06:
		/* write only */
		return memz80_invalid_read(addr, "bank");
	case 0x07:
		if((addr >> 8) == 0x7F)
			return memz80_invalid_read(addr, "PSG");
		goto bad_byte_r;
	/* 0x8000 - 0xffff - main ram */
	default:
		return (fetchbyte(cpuz80_bank | (addr - 0x8000)));
	}
}

static inline void memz80_storebyte(uint16 addr, uint8 data) {
	switch(addr >> 12) {
	/* sram */
	case 0x00: case 0x01: case 0x02: case 0x03:
		*(uint8 *)(cpuz80_ram + (addr & 0x1fff)) = data;
		return;
	/* yam */
	case 0x04:
		addr -= 0x4000;
		if(UNLIKELY(addr >= 4)) {
			addr += 0x4000;
			if((addr >> 8) == 0x40) {
				memz80_invalid_store(addr, "YAM");
				return;
			}
			memz80_invalid_store(addr, "memory");
			return;
		}
		sound_ym2612store(addr, data);
		return;
	/* bad byte */
	case 0x05:
	bad_byte_w:
		memz80_invalid_store(addr, "memory");
		return;
	/* bank */
	case 0x06:
		/* set bank */
		cpuz80_bankwrite(data);
		return;
	case 0x07:
		if(UNLIKELY(addr != 0x7f11)) {
			if((addr >> 8) == 0x7F)
				return memz80_invalid_store(addr, "PSG");
			goto bad_byte_w;
		}
		sound_sn76496store(data);
		return;
	/* 0x8000 - 0xffff - main ram */
	default:
		/* LOG_USER(("WRITE whilst bank = %08X (%08X)", cpuz80_bank,
			addr-0x8000)); */
		storebyte(cpuz80_bank | (addr - 0x8000), data);
		return;
	}
}

#endif /* MEMZ80_HEADER_FILE */
