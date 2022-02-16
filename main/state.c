/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "generator.h"
#include "snprintf.h"

#include "state.h"
#include "ui.h"
#include "cpu68k.h"
#include "cpuz80.h"
#include "vdp.h"
#include "gensound.h"
#include "gensoundi.h"
#include "fm.h"

/*
 * NB:
 * states are only loaded/saved at the end of the frame
 * all local values are stored in network / 68k format (big endian)
 */

struct savestate {
  /* must ensure everything is aligned as expected */
  char s_majorver[1];
  char s_minorver[1];
  char reserved1[2]; /* align */
  char s_ram[0x10000];
  char s_vram[0x10000];
  char s_z80ram[0x2000];
  char s_cram[0x80];
  char s_vsram[0x50];
  char s_68k_regs[4*16];
  char s_68k_pc[4];
  char s_68k_sp[4];
  char s_68k_sr[2];
  char s_68k_stop[1];
  char s_68k_pending[1];
  char s_z80_af[2];
  char s_z80_bc[2];
  char s_z80_de[2];
  char s_z80_hl[2];
  char s_z80_afprime[2];
  char s_z80_bcprime[2];
  char s_z80_deprime[2];
  char s_z80_hlprime[2];
  char s_z80_ix[2];
  char s_z80_iy[2];
  char s_z80_sp[2];
  char s_z80_pc[2];
  char s_z80_i[1];
  char s_z80_r[1];
  char s_z80_active[1];
  char s_z80_halted[1]; /* was reserved in 0.31 */
  char s_z80_bank[4];
  char s_fmregs1[256];
  char s_fmregs2[256];
  char s_fmaddr1[1];
  char s_fmaddr2[1];
  char s_vdp_regs[25];
  char s_z80_im[1]; /* was reserved in 0.31 */
  char s_vdp_pal[1];
  char s_vdp_overseas[1];
  char s_vdp_ctrlflag[1];
  char s_vdp_code[1];
  char s_vdp_first[2]; /* possibly only top two bits required here */
  char s_vdp_second[2]; /* used for unterminated ctrl writes */
  char s_vdp_dmabytes[4];
  char s_vdp_address[2];
  char s_z80_iff1[1]; /* was reserved in 0.31 */
  char s_z80_iff2[1]; /* was reserved in 0.31 */
};

/*** state_date - return the modification date or 0 for non-existant ***/

time_t state_date(const int slot)
{
  char filename[256];
  struct stat statbuf;

  snprintf(filename, sizeof(filename), "%s.gt%d", gen_leafname, slot);

  if (stat(filename, &statbuf) != 0)
    return 0;
  return statbuf.st_mtime;
}

/*** state_load - load the given slot ***/

int state_load(const int slot)
{
  char filename[256];
  struct savestate *ssp;
  char *ss;
  struct stat statbuf;
  FILE *f, *a;
  int i;

  snprintf(filename, sizeof(filename), "%s.gt%d", gen_leafname, slot);
  if (stat(filename, &statbuf) != 0)
    return -1;

  if ((ss = malloc(statbuf.st_size)) == NULL) {
    LOG_CRITICAL(("Failed to allocate memory whilst loading '%s'",
                  filename));
    return -1;
  }
  if ((f = fopen(filename, "rb")) == NULL) {
    LOG_CRITICAL(("Failed to open '%s': %s", filename, strerror(errno)));
    return -1;
  }
  if (fread(ss, statbuf.st_size, 1, f) != 1) {
    if (feof(f)) {
      LOG_CRITICAL(("EOF whilst reading GTR file '%s'", filename));
      return -1;
    }
    LOG_CRITICAL(("Error whilst reading GTR file '%s': %s", filename,
		  strerror(errno)));
    return -1;
  }
  fclose(f);

  ssp = (struct savestate *)ss;

  if (ssp->s_majorver[0] != 1) {
    LOG_CRITICAL(("GTR file '%s' is version %d, and we're version 1",
                  filename, ssp->s_majorver));
    return -1;
  }
  if (statbuf.st_size != sizeof(struct savestate)) {
    LOG_CRITICAL(("GTR file '%s' is corrupt - it is not %d bytes long",
                  filename, sizeof(struct savestate)));
    return -1;
  }

  gen_reset();
  memcpy(cpu68k_ram, ssp->s_ram, sizeof(ssp->s_ram));
  memcpy(vdp_vram, ssp->s_vram, sizeof(ssp->s_vram));
  memcpy(cpuz80_ram, ssp->s_z80ram, sizeof(ssp->s_z80ram));
  memcpy(vdp_cram, ssp->s_cram, sizeof(ssp->s_cram));
  memcpy(vdp_vsram, ssp->s_vsram, sizeof(ssp->s_vsram));
  for (i = 0; i < 16; i++)
    regs.regs[i] = LOCENDIAN32(((uint32 *)ssp->s_68k_regs)[i]);
  regs.pc = LOCENDIAN32(*(uint32 *)ssp->s_68k_pc);
  regs.sp = LOCENDIAN32(*(uint32 *)ssp->s_68k_sp);
  regs.sr.sr_int = LOCENDIAN16(*(uint16 *)ssp->s_68k_sr);
  regs.stop = ssp->s_68k_stop[0];
  regs.pending = ssp->s_68k_pending[0];
#ifdef RAZE
  z80_set_reg(Z80_REG_AF,  LOCENDIAN16(*(uint16 *)ssp->s_z80_af));
  z80_set_reg(Z80_REG_BC,  LOCENDIAN16(*(uint16 *)ssp->s_z80_bc));
  z80_set_reg(Z80_REG_DE,  LOCENDIAN16(*(uint16 *)ssp->s_z80_de));
  z80_set_reg(Z80_REG_HL,  LOCENDIAN16(*(uint16 *)ssp->s_z80_hl));
  z80_set_reg(Z80_REG_AF2, LOCENDIAN16(*(uint16 *)ssp->s_z80_afprime));
  z80_set_reg(Z80_REG_BC2, LOCENDIAN16(*(uint16 *)ssp->s_z80_bcprime));
  z80_set_reg(Z80_REG_DE2, LOCENDIAN16(*(uint16 *)ssp->s_z80_deprime));
  z80_set_reg(Z80_REG_HL2, LOCENDIAN16(*(uint16 *)ssp->s_z80_hlprime));
  z80_set_reg(Z80_REG_IX,  LOCENDIAN16(*(uint16 *)ssp->s_z80_ix));
  z80_set_reg(Z80_REG_IY,  LOCENDIAN16(*(uint16 *)ssp->s_z80_iy)); 
  z80_set_reg(Z80_REG_SP,  LOCENDIAN16(*(uint16 *)ssp->s_z80_sp));
  z80_set_reg(Z80_REG_PC,  LOCENDIAN16(*(uint16 *)ssp->s_z80_pc));
  z80_set_reg(Z80_REG_IFF1, ssp->s_z80_iff1[0]);
  z80_set_reg(Z80_REG_IFF2, ssp->s_z80_iff2[0]);
  z80_set_reg(Z80_REG_IR, ssp->s_z80_i[0]<<8 | ssp->s_z80_r[0]);
  z80_set_reg(Z80_REG_IM, ssp->s_z80_im[0]);
  z80_set_reg(Z80_REG_Halted, ssp->s_z80_halted[0]);
  /* FIX:
      Z80_REG_IRQVector,   0x00 to 0xff
      Z80_REG_IRQLine,     boolean - 1 or 0
  */
#else
  cpuz80_z80.z80af = LOCENDIAN16(*(uint16 *)ssp->s_z80_af);
  cpuz80_z80.z80bc = LOCENDIAN16(*(uint16 *)ssp->s_z80_bc);
  cpuz80_z80.z80de = LOCENDIAN16(*(uint16 *)ssp->s_z80_de);
  cpuz80_z80.z80hl = LOCENDIAN16(*(uint16 *)ssp->s_z80_hl);
  cpuz80_z80.z80afprime = LOCENDIAN16(*(uint16 *)ssp->s_z80_afprime);
  cpuz80_z80.z80bcprime = LOCENDIAN16(*(uint16 *)ssp->s_z80_bcprime);
  cpuz80_z80.z80deprime = LOCENDIAN16(*(uint16 *)ssp->s_z80_deprime);
  cpuz80_z80.z80hlprime = LOCENDIAN16(*(uint16 *)ssp->s_z80_hlprime);
  cpuz80_z80.z80ix = LOCENDIAN16(*(uint16 *)ssp->s_z80_ix);
  cpuz80_z80.z80iy = LOCENDIAN16(*(uint16 *)ssp->s_z80_iy);
  cpuz80_z80.z80sp = LOCENDIAN16(*(uint16 *)ssp->s_z80_sp);
  cpuz80_z80.z80pc = LOCENDIAN16(*(uint16 *)ssp->s_z80_pc);
  cpuz80_z80.z80i = ssp->s_z80_i[0];
  cpuz80_z80.z80r = ssp->s_z80_r[0];
  cpuz80_z80.z80inInterrupt = ssp->s_z80_iff1[0];
  cpuz80_z80.z80interruptState = ssp->s_z80_iff2[0];
  cpuz80_z80.z80interruptMode = ssp->s_z80_im[0];
  cpuz80_z80.z80halted = ssp->s_z80_halted[0];
  /* FIX:
    c -> z80intAddr = z80intAddr;
    c -> z80nmiAddr = z80nmiAddr;
  */
#endif
  cpuz80_bank = LOCENDIAN32(*(uint32 *)ssp->s_z80_bank);


  memcpy(vdp_reg, ssp->s_vdp_regs, sizeof(ssp->s_vdp_regs));
  vdp_pal = ssp->s_vdp_pal[0];
  vdp_overseas = ssp->s_vdp_overseas[0];
  vdp_ctrlflag = ssp->s_vdp_ctrlflag[0];
  vdp_code = ssp->s_vdp_code[0];
  vdp_first = LOCENDIAN16(*(uint16 *)ssp->s_vdp_first);
  vdp_second = LOCENDIAN16(*(uint16 *)ssp->s_vdp_second);
  vdp_dmabytes = LOCENDIAN32(*(uint32 *)ssp->s_vdp_dmabytes);
  vdp_address = LOCENDIAN16(*(uint16 *)ssp->s_vdp_address);
  
  /* reset some other run-time stuff that isn't important enough to save */
  vdp_setupvideo();
  vdp_dmabusy = vdp_dmabytes > 0 ? 1 : 0;

  for (i = 0; i < 256; i++) {
    soundi_ym2612store(0, i);
    soundi_ym2612store(1, ssp->s_fmregs1[i]);
    soundi_ym2612store(2, i);
    soundi_ym2612store(3, ssp->s_fmregs2[i]);
  }
  soundi_ym2612store(0, ssp->s_fmaddr1[0]);
  soundi_ym2612store(1, ssp->s_fmaddr2[0]);
  cpuz80_updatecontext();
  return 0;
}

/*** state_save - save to the given slot ***/

int state_save(const int slot)
{
  char filename[256];
  struct savestate ss;
  struct savestate *ssp = &ss; /* I want a pointer like the load routines */
  struct stat statbuf;
  FILE *f, *a;
  int i;

  snprintf(filename, sizeof(filename), "%s.gt%d", gen_leafname, slot);
  if ((f = fopen(filename, "wb")) == NULL) {
    LOG_CRITICAL(("Failed to open '%s' for writing: %s",
                  filename, strerror(errno)));
    return -1;
  }
  memset(ssp, 0, sizeof(ss));
  ssp->s_majorver[0] = 1;
  ssp->s_minorver[0] = 2;
  memcpy(ssp->s_ram, cpu68k_ram, sizeof(ssp->s_ram));
  memcpy(ssp->s_vram, vdp_vram, sizeof(ssp->s_vram));
  memcpy(ssp->s_z80ram, cpuz80_ram, sizeof(ssp->s_z80ram));
  memcpy(ssp->s_cram, vdp_cram, sizeof(ssp->s_cram));
  memcpy(ssp->s_vsram, vdp_vsram, sizeof(ssp->s_vsram));
  for (i = 0; i < 16; i++)
    ((uint32 *)ssp->s_68k_regs)[i] = LOCENDIAN32(regs.regs[i]);
  *(uint32 *)ssp->s_68k_pc = LOCENDIAN32(regs.pc);
  *(uint32 *)ssp->s_68k_sp = LOCENDIAN32(regs.sp);
  *(uint16 *)ssp->s_68k_sr = LOCENDIAN16(regs.sr.sr_int);
  ssp->s_68k_stop[0] = regs.stop;
  ssp->s_68k_pending[0] = regs.pending;
#ifdef RAZE
  *(uint16 *)ssp->s_z80_af = LOCENDIAN16(z80_get_reg(Z80_REG_AF));
  *(uint16 *)ssp->s_z80_bc = LOCENDIAN16(z80_get_reg(Z80_REG_BC));
  *(uint16 *)ssp->s_z80_de = LOCENDIAN16(z80_get_reg(Z80_REG_DE));
  *(uint16 *)ssp->s_z80_hl = LOCENDIAN16(z80_get_reg(Z80_REG_HL));
  *(uint16 *)ssp->s_z80_afprime = LOCENDIAN16(z80_get_reg(Z80_REG_AF2));
  *(uint16 *)ssp->s_z80_bcprime = LOCENDIAN16(z80_get_reg(Z80_REG_BC2));
  *(uint16 *)ssp->s_z80_deprime = LOCENDIAN16(z80_get_reg(Z80_REG_DE2));
  *(uint16 *)ssp->s_z80_hlprime = LOCENDIAN16(z80_get_reg(Z80_REG_HL2));
  *(uint16 *)ssp->s_z80_ix = LOCENDIAN16(z80_get_reg(Z80_REG_IX));
  *(uint16 *)ssp->s_z80_iy = LOCENDIAN16(z80_get_reg(Z80_REG_IY));
  *(uint16 *)ssp->s_z80_sp = LOCENDIAN16(z80_get_reg(Z80_REG_SP));
  *(uint16 *)ssp->s_z80_pc = LOCENDIAN16(z80_get_reg(Z80_REG_PC));
  ssp->s_z80_i[0] = (z80_get_reg(Z80_REG_IR) >> 8) & 0xff;
  ssp->s_z80_r[0] = z80_get_reg(Z80_REG_IR) & 0xff;
  ssp->s_z80_iff1[0] = z80_get_reg(Z80_REG_IFF1);
  ssp->s_z80_iff2[0] = z80_get_reg(Z80_REG_IFF2);
  ssp->s_z80_im[0] = z80_get_reg(Z80_REG_IM);
#else
  *(uint16 *)ssp->s_z80_af = LOCENDIAN16(cpuz80_z80.z80af);
  *(uint16 *)ssp->s_z80_bc = LOCENDIAN16(cpuz80_z80.z80bc);
  *(uint16 *)ssp->s_z80_de = LOCENDIAN16(cpuz80_z80.z80de);
  *(uint16 *)ssp->s_z80_hl = LOCENDIAN16(cpuz80_z80.z80hl);
  *(uint16 *)ssp->s_z80_afprime = LOCENDIAN16(cpuz80_z80.z80afprime);
  *(uint16 *)ssp->s_z80_bcprime = LOCENDIAN16(cpuz80_z80.z80bcprime);
  *(uint16 *)ssp->s_z80_deprime = LOCENDIAN16(cpuz80_z80.z80deprime);
  *(uint16 *)ssp->s_z80_hlprime = LOCENDIAN16(cpuz80_z80.z80hlprime);
  *(uint16 *)ssp->s_z80_ix = LOCENDIAN16(cpuz80_z80.z80ix);
  *(uint16 *)ssp->s_z80_iy = LOCENDIAN16(cpuz80_z80.z80iy);
  *(uint16 *)ssp->s_z80_sp = LOCENDIAN16(cpuz80_z80.z80sp);
  *(uint16 *)ssp->s_z80_pc = LOCENDIAN16(cpuz80_z80.z80pc);
  ssp->s_z80_i[0] = cpuz80_z80.z80i;
  ssp->s_z80_r[0] = cpuz80_z80.z80r;
  ssp->s_z80_iff1[0] = cpuz80_z80.z80inInterrupt;
  ssp->s_z80_iff2[0] = cpuz80_z80.z80interruptState;
  ssp->s_z80_im[0] = cpuz80_z80.z80interruptMode;
  ssp->s_z80_halted[0] = cpuz80_z80.z80halted;
#endif
  *(uint32 *)ssp->s_z80_bank = LOCENDIAN32(cpuz80_bank);
  memcpy(ssp->s_vdp_regs, vdp_reg, sizeof(ssp->s_vdp_regs));
  ssp->s_vdp_pal[0] = vdp_pal;
  ssp->s_vdp_overseas[0] = vdp_overseas;
  ssp->s_vdp_ctrlflag[0] = vdp_ctrlflag;
  ssp->s_vdp_code[0] = vdp_code;
  *(uint16 *)ssp->s_vdp_first = LOCENDIAN16(vdp_first);
  *(uint16 *)ssp->s_vdp_second = LOCENDIAN16(vdp_second);
  *(uint32 *)ssp->s_vdp_dmabytes = LOCENDIAN32(vdp_dmabytes);
  *(uint16 *)ssp->s_vdp_address = LOCENDIAN16(vdp_address);
  memcpy(ssp->s_fmregs1, soundi_regs1, sizeof(ssp->s_fmregs1));
  memcpy(ssp->s_fmregs2, soundi_regs2, sizeof(ssp->s_fmregs2));
  ssp->s_fmaddr1[0] = soundi_address1;
  ssp->s_fmaddr2[0] = soundi_address2;
  if (fwrite(ssp, sizeof(ss), 1, f) != 1) {
    LOG_CRITICAL(("Error whilst writing GTR file '%s': %s", filename,
		  strerror(errno)));
    return -1;
  }
  fclose(f);
  return 0;
}
