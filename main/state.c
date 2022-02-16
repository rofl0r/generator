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
#include "fm.h"

typedef struct _t_statelist {
  struct _t_statelist *next;
  char *mod;
  char *name;
  uint8 instance;
  uint32 bytes;
  uint32 size;
  uint8 *data;
} t_statelist;

FILE *state_outputfile;         /* the file handle to place data blocks */
uint8 state_transfermode;       /* 0 = save, 1 = load */
uint8 state_major;              /* major version */
uint8 state_minor;              /* minor version */
t_statelist *state_statelist;   /* loaded state */

/*
 * NB:
 * states are only loaded/saved at the end of the frame
 */

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

  snprintf(filename, sizeof(filename), "%s.gt%d", gen_leafname, slot);
  return state_loadfile(filename);
}

/*** state_save - save to the given slot ***/

int state_save(const int slot)
{
  char filename[256];

  snprintf(filename, sizeof(filename), "%s.gt%d", gen_leafname, slot);
  return state_savefile(filename);
}

void state_transfer8(const char *mod, const char *name, uint8 instance,
                     uint8 *data, uint32 size)
{
  t_statelist *l;
  uint8 buf[4];
  uint32 i;

  if (state_transfermode == 0) {
    /* save */
    fwrite(mod, strlen(mod)+1, 1, state_outputfile);
    fwrite(name, strlen(name)+1, 1, state_outputfile);
    buf[0] = instance;
    buf[1] = 1; /* bytes per object */
    fwrite(buf, 2, 1, state_outputfile);
    buf[0] = (size >> 24) & 0xff;
    buf[1] = (size >> 16) & 0xff;
    buf[2] = (size >> 8) & 0xff;
    buf[3] = size & 0xff;
    fwrite(buf, 4, 1, state_outputfile);
    fwrite(data, size, 1, state_outputfile);
  } else {
    /* load */
    for (l = state_statelist; l; l = l->next) {
      if (!strcasecmp(l->mod, mod) && !strcasecmp(l->name, name) &&
          l->instance == instance && l->size == size && l->bytes == 1) {
        for (i = 0; i < size; i++)
          data[i] = l->data[i];
        LOG_VERBOSE(("Loaded %s %s (%d)", mod, name, instance));
        break;
      }
    }
    if (l == NULL) {
      LOG_CRITICAL(("bad %s/%s\n", mod, name));
      memset(data, 0, size);
    }
  }
}

void state_transfer16(const char *mod, const char *name, uint8 instance,
                      uint16 *data, uint32 size)
{
  t_statelist *l;
  uint8 buf[4];
  uint32 i;

  if (state_transfermode == 0) {
    /* save */
    fwrite(mod, strlen(mod)+1, 1, state_outputfile);
    fwrite(name, strlen(name)+1, 1, state_outputfile);
    buf[0] = instance;
    buf[1] = 2; /* bytes per object */
    fwrite(buf, 2, 1, state_outputfile);
    buf[0] = (size >> 24) & 0xff;
    buf[1] = (size >> 16) & 0xff;
    buf[2] = (size >> 8) & 0xff;
    buf[3] = size & 0xff;
    fwrite(buf, 4, 1, state_outputfile);
    for (i = 0; i < size; i++) {
      buf[0] = (data[i] >> 8) & 0xff;
      buf[1] = data[i] & 0xff;
      fwrite(buf, 2, 1, state_outputfile);
    }
  } else {
    /* load */
    for (l = state_statelist; l; l = l->next) {
      if (!strcasecmp(l->mod, mod) && !strcasecmp(l->name, name) &&
          l->instance == instance && l->size == size && l->bytes == 2) {
        for (i = 0; i < size; i++)
          data[i] = ((((uint8 *)l->data)[(i << 1)] << 8) |
                     (((uint8 *)l->data)[(i << 1) + 1]));
        LOG_VERBOSE(("Loaded %s %s (%d)", mod, name, instance));
        break;
      }
    }
    if (l == NULL) {
      LOG_CRITICAL(("bad %s/%s\n", mod, name));
      memset(data, 0, size * 2);
    }
  }
}

void state_transfer32(const char *mod, const char *name, uint8 instance,
                      uint32 *data, uint32 size)
{
  t_statelist *l;
  uint8 buf[4];
  uint32 i;

  if (state_transfermode == 0) {
    /* save */
    fwrite(mod, strlen(mod)+1, 1, state_outputfile);
    fwrite(name, strlen(name)+1, 1, state_outputfile);
    buf[0] = instance;
    buf[1] = 4; /* bytes per object */
    fwrite(buf, 2, 1, state_outputfile);
    buf[0] = (size >> 24) & 0xff;
    buf[1] = (size >> 16) & 0xff;
    buf[2] = (size >> 8) & 0xff;
    buf[3] = size & 0xff;
    fwrite(buf, 4, 1, state_outputfile);
    for (i = 0; i < size; i++) {
      buf[0] = (data[i] >> 24) & 0xff;
      buf[1] = (data[i] >> 16) & 0xff;
      buf[2] = (data[i] >> 8) & 0xff;
      buf[3] = data[i] & 0xff;
      fwrite(buf, 4, 1, state_outputfile);
    }
  } else {
    /* load */
    for (l = state_statelist; l; l = l->next) {
      if (!strcasecmp(l->mod, mod) && !strcasecmp(l->name, name) &&
          l->instance == instance && l->size == size && l->bytes == 4) {
        for (i = 0; i < size; i++)
          data[i] = ((((uint8 *)l->data)[(i << 2)] << 24) |
                     (((uint8 *)l->data)[(i << 2) + 1] << 16) |
                     (((uint8 *)l->data)[(i << 2) + 2] << 8) |
                     (((uint8 *)l->data)[(i << 2) + 3]));
        LOG_VERBOSE(("Loaded %s %s (%d)", mod, name, instance));
        break;
      }
    }
    if (l == NULL) {
      LOG_CRITICAL(("bad %s/%s\n", mod, name));
      memset(data, 0, size * 4);
    }
  }
}

/*** state_dotransfer - do transfer of data, either save or load ***/

static void state_dotransfer(unsigned int mode)
{
  uint8 i8, i8b;
  uint16 i16;

  (void)i8b;
  state_transfermode = mode; /* 0 = save, 1 = load */
  state_transfer8("ver", "major", 0, &state_major, 1);
  state_transfer8("ver" ,"minor", 0, &state_minor, 1);
  state_transfer8("vdp", "vram", 0, vdp_vram, LEN_VRAM);
  state_transfer8("vdp", "cram", 0, vdp_cram, LEN_CRAM);
  state_transfer8("vdp", "vsram", 0, vdp_vsram, LEN_VSRAM);
  state_transfer8("vdp", "regs", 0, vdp_reg, 25);
  state_transfer8("vdp", "pal", 0, &vdp_pal, 1);
  state_transfer8("vdp", "overseas", 0, &vdp_overseas, 1);
  state_transfer8("vdp", "ctrlflag", 0, &vdp_ctrlflag, 1);
  /* this cast is probably very bad */
  state_transfer8("vdp", "code", 0, (uint8 *)&vdp_code, 1);
  state_transfer16("vdp", "first", 0, &vdp_first, 1);
  state_transfer16("vdp", "second", 0, &vdp_second, 1);
  state_transfer32("vdp", "dmabytes", 0, &vdp_dmabytes, 1);
  state_transfer16("vdp", "address", 0, &vdp_address, 1);
  state_transfer8("68k", "ram", 0, cpu68k_ram, 0x10000);
  state_transfer32("68k", "regs", 0, regs.regs, 16);
  state_transfer32("68k", "pc", 0, &regs.pc, 1);
  state_transfer32("68k", "sp", 0, &regs.sp, 1);
  state_transfer16("68k", "sr", 0, &regs.sr.sr_int, 1);
  state_transfer16("68k", "stop", 0, &regs.stop, 1);
  state_transfer16("68k", "pending", 0, &regs.pending, 1);
  state_transfer8("z80", "ram", 0, cpuz80_ram, LEN_SRAM);
  state_transfer8("z80", "active", 0, &cpuz80_active, 1);
  state_transfer8("z80", "resetting", 0, &cpuz80_resetting, 1);
  state_transfer32("z80", "bank", 0, &cpuz80_bank, 1);
#ifdef RAZE
  if (state_transfermode == 0) {
    /* save */
    i16 = z80_get_reg(Z80_REG_AF); state_transfer16("z80", "af", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_BC); state_transfer16("z80", "bc", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_DE); state_transfer16("z80", "de", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_HL); state_transfer16("z80", "hl", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_AF2); state_transfer16("z80", "af2", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_BC2); state_transfer16("z80", "bc2", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_DE2); state_transfer16("z80", "de2", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_HL2); state_transfer16("z80", "hl2", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_IX); state_transfer16("z80", "ix", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_IY); state_transfer16("z80", "iy", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_SP); state_transfer16("z80", "sp", 0, &i16, 1);
    i16 = z80_get_reg(Z80_REG_PC); state_transfer16("z80", "pc", 0, &i16, 1);
    i8 = (z80_get_reg(Z80_REG_IR) >> 8) & 0xff;
    state_transfer8("z80", "i", 0, &i8, 1);
    i8 = z80_get_reg(Z80_REG_IR) & 0xff;
    state_transfer8("z80", "r", 0, &i8, 1);
    i8 = z80_get_reg(Z80_REG_IFF1); state_transfer8("z80", "iff1", 0, &i8, 1);
    i8 = z80_get_reg(Z80_REG_IFF2); state_transfer8("z80", "iff2", 0, &i8, 1);
    i8 = z80_get_reg(Z80_REG_IM); state_transfer8("z80", "im", 0, &i8, 1);
    i8 = z80_get_reg(Z80_REG_Halted);
    state_transfer8("z80", "halted", 0, &i8, 1);
  } else {
    /* load */
    state_transfer16("z80", "af", 0, &i16, 1); z80_set_reg(Z80_REG_AF, i16);
    state_transfer16("z80", "bc", 0, &i16, 1); z80_set_reg(Z80_REG_BC, i16);
    state_transfer16("z80", "de", 0, &i16, 1); z80_set_reg(Z80_REG_DE, i16);
    state_transfer16("z80", "hl", 0, &i16, 1); z80_set_reg(Z80_REG_HL, i16);
    state_transfer16("z80", "af2", 0, &i16, 1); z80_set_reg(Z80_REG_AF2, i16);
    state_transfer16("z80", "bc2", 0, &i16, 1); z80_set_reg(Z80_REG_BC2, i16);
    state_transfer16("z80", "de2", 0, &i16, 1); z80_set_reg(Z80_REG_DE2, i16);
    state_transfer16("z80", "hl2", 0, &i16, 1); z80_set_reg(Z80_REG_HL2, i16);
    state_transfer16("z80", "ix", 0, &i16, 1); z80_set_reg(Z80_REG_IX, i16);
    state_transfer16("z80", "iy", 0, &i16, 1); z80_set_reg(Z80_REG_IY, i16);
    state_transfer16("z80", "sp", 0, &i16, 1); z80_set_reg(Z80_REG_SP, i16);
    state_transfer16("z80", "pc", 0, &i16, 1); z80_set_reg(Z80_REG_PC, i16);
    state_transfer8("z80", "i", 0, &i8, 1);
    state_transfer8("z80", "r", 0, &i8b, 1);
    z80_set_reg(Z80_REG_IR, ((uint16)i8 << 8) | i8b);
    state_transfer8("z80", "iff1", 0, &i8, 1); z80_set_reg(Z80_REG_IFF1, i8);
    state_transfer8("z80", "iff2", 0, &i8, 1); z80_set_reg(Z80_REG_IFF2, i8);
    state_transfer8("z80", "im", 0, &i8, 1); z80_set_reg(Z80_REG_IM, i8);
    state_transfer8("z80", "halted", 0, &i8, 1);
    z80_set_reg(Z80_REG_Halted, i8);
  }
#else
  state_transfer16("z80", "af", 0, &cpuz80_z80.z80af, 1);
  state_transfer16("z80", "bc", 0, &cpuz80_z80.z80bc, 1);
  state_transfer16("z80", "de", 0, &cpuz80_z80.z80de, 1);
  state_transfer16("z80", "hl", 0, &cpuz80_z80.z80hl, 1);
  state_transfer16("z80", "af2", 0, &cpuz80_z80.z80afprime, 1);
  state_transfer16("z80", "bc2", 0, &cpuz80_z80.z80bcprime, 1);
  state_transfer16("z80", "de2", 0, &cpuz80_z80.z80deprime, 1);
  state_transfer16("z80", "hl2", 0, &cpuz80_z80.z80hlprime, 1);
  state_transfer16("z80", "ix", 0, &cpuz80_z80.z80ix, 1);
  state_transfer16("z80", "iy", 0, &cpuz80_z80.z80iy, 1);
  state_transfer16("z80", "sp", 0, &cpuz80_z80.z80sp, 1);
  state_transfer16("z80", "pc", 0, &cpuz80_z80.z80pc, 1);
  state_transfer8("z80", "i", 0, &cpuz80_z80.z80i, 1);
  state_transfer8("z80", "r", 0, &cpuz80_z80.z80r, 1);
  if (state_transfermode == 0) {
    /* save */
    i8 = cpuz80_z80.z80inInterrupt;
    state_transfer8("z80", "iff1", 0, &i8, 1);
    i8 = cpuz80_z80.z80interruptState;
    state_transfer8("z80", "iff2", 0, &i8, 1);
    i8 = cpuz80_z80.z80interruptMode;
    state_transfer8("z80", "im", 0, &i8, 1);
    i8 = cpuz80_z80.z80halted;
    state_transfer8("z80", "halted", 0, &i8, 1);
  } else {
    /* load */
    state_transfer8("z80", "iff1", 0, &i8, 1);
    cpuz80_z80.z80inInterrupt = i8;
    state_transfer8("z80", "iff2", 0, &i8, 1);
    cpuz80_z80.z80interruptState = i8;
    state_transfer8("z80", "im", 0, &i8, 1);
    cpuz80_z80.z80interruptMode = i8;
    state_transfer8("z80", "halted", 0, &i8, 1);
    cpuz80_z80.z80halted = i8;
  }
#endif
  YM2612_save_state();

  /* XXX: FIX ME!
     Z80_REG_IRQVector,   0x00 to 0xff
     Z80_REG_IRQLine,     boolean - 1 or 0
   */

  /* XXX: FIX ME!
     c -> z80intAddr = z80intAddr;
     c -> z80nmiAddr = z80nmiAddr;
   */
}  

/*** state_savefile - save to the given filename */

int state_savefile(const char *filename)
{
  if ((state_outputfile = fopen(filename, "wb")) == NULL) {
    LOG_CRITICAL(("Failed to open '%s' for writing: %s",
                  filename, strerror(errno)));
    return -1;
  }
  fprintf(state_outputfile, "Generator " VERSION " saved state\n");
  state_major = 2;
  state_minor = 0;
  state_dotransfer(0); /* save */
  fclose(state_outputfile);
  return 0;
}

/*** state_loadfile - load the given filename ***/

int state_loadfile(const char *filename)
{
  char *blk;
  uint8 *p, *e;
  struct stat statbuf;
  FILE *f;
  t_statelist *ent;

  if (stat(filename, &statbuf) != 0) {
    errno = ENOENT;
    return -1;
  }

  if ((blk = malloc(statbuf.st_size)) == NULL) {
    LOG_CRITICAL(("Failed to allocate memory whilst loading '%s'", filename));
    return -1;
  }
  if ((f = fopen(filename, "rb")) == NULL) {
    LOG_CRITICAL(("Failed to open '%s': %s", filename, strerror(errno)));
    free(blk);
    return -1;
  }
  if (fread(blk, statbuf.st_size, 1, f) != 1) {
    if (feof(f)) {
      LOG_CRITICAL(("EOF whilst reading save state file '%s'", filename));
      free(blk);
      return -1;
    }
    LOG_CRITICAL(("Error whilst reading save state file '%s': %s", filename,
                  strerror(errno)));
    free(blk);
    return -1;
  }
  fclose(f);

  p = blk;
  e = blk + statbuf.st_size;

  /* skip first line comment */
  while (p < e && *p++ != '\n') ;
  if (p >= e)
    goto OVERRUN;

  /* loop around blocks creating structure */
  state_statelist = NULL;
  for (;;) {
    /* mod(1+), name(1+), instance(1), bytes(1), size(4), data(0+) */
    if (e == p)
      /* EOF */
      break;
    if ((ent = malloc(sizeof(t_statelist))) == NULL)
      ui_err("out of memory");
    if ((e-p) < 8)
      goto OVERRUN;
    ent->mod = p;
    while (p < e && *p++) ;
    if ((e-p) < 7)
      goto OVERRUN;
    ent->name = p;
    while (p < e && *p++) ;
    if ((e-p) < 6)
      goto OVERRUN;
    ent->instance = p[0];
    ent->bytes = p[1];
    ent->size = (p[2] << 24) | (p[3] << 16) | (p[4] << 8) | p[5];
    if ((e-p) < (int)(ent->bytes * ent->size))
      goto OVERRUN;
    p+= 6;
    ent->data = p;
    p+= ent->bytes * ent->size;
    ent->next = state_statelist;
    state_statelist = ent;
  }

  /* reset */
  gen_reset();

  state_dotransfer(1); /* load into place */

  /* free memory */
  free(blk);
  while (state_statelist) {
    ent = state_statelist;
    state_statelist = state_statelist->next;
    free(ent);
  }

  if (state_major != 2) {
    LOG_CRITICAL(("Save state file '%s' is version %d, and we're version 2",
                  filename, state_major));
    errno = EINVAL;
    return -1;
  }

  /* reset some other run-time stuff that isn't important enough to save */
  vdp_setupvideo();
  vdp_dmabusy = vdp_dmabytes > 0 ? 1 : 0;
  cpuz80_updatecontext();
  return 0;
OVERRUN:
  LOG_CRITICAL(("Invalid state file '%s': overrun encountered", filename));
  errno = EINVAL;
  free(blk);
  while (state_statelist) {
    ent = state_statelist;
    state_statelist = state_statelist->next;
    free(ent);
  }
  return -1;
}

