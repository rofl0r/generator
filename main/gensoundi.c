/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"
#include "gensoundi.h"

#ifdef JFM
#  include "jfm.h"
#else
#  include "support.h"
#  include "fm.h"
#endif

uint8 soundi_regs1[256];
uint8 soundi_regs2[256];
uint8 soundi_address1 = 0;
uint8 soundi_address2 = 0;
uint8 soundi_keys[8];

/*** soundi_ym2612fetch - fetch byte from ym2612 chip ***/

uint8 soundi_ym2612fetch(uint8 addr)
{
#ifdef JFM
  return jfm_read(soundi_ctx, addr);
#else
  return YM2612Read(0, addr);
#endif
}

/*** soundi_ym2612store - store a byte to the ym2612 chip ***/

void soundi_ym2612store(uint8 addr, uint8 data)
{
  switch (addr) {
  case 0:
    soundi_address1 = data;
    break;
  case 1:
    if (soundi_address1 == 0x28 && (data & 3) != 3)
      soundi_keys[data & 7] = data >> 4;
    if (soundi_address1 == 0x2a) {
      soundi_keys[7] = 0;
    }
    if (soundi_address1 == 0x2b)
      soundi_keys[7] = data & 0x80 ? 0xf : 0;
    soundi_regs1[soundi_address1] = data;
    break;
  case 2:
    soundi_address2 = data;
    break;
  case 3:
    soundi_regs2[soundi_address2] = data;
    break;
  }
#ifdef JFM
  jfm_write(soundi_ctx, addr, data);
#else
  YM2612Write(0, addr, data);
#endif
}

