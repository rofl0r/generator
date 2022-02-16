/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "generator.h"

#include "ui.h"
#include "cpu68k.h"
#include "patch.h"

/* Merlyn LeRoy's lamp.c comment: */

/*
 * "Eet ees not *I* who am crazy, eet ees I who am MAD!" - R. Ho"ek
 *
 * lamp.c - a program to control Game Genie for Genesis
 *
 * Translate between Game Genie for Genesis 8-character codes and
 * the 24-bit address & 16-bit value for that address.
 * The format is this:
 *
 * ijklm nopIJ KLMNO PABCD EFGHd efgha bcQRS TUVWX   decodes to...
 *
 * ABCDEFGH IJKLMNOP QRSTUVWX: abcdefgh ijklmnop
 * 24-bit address              16-bit data
 * MSB                    LSB  MSB           LSB
 *
 * ..where each group of five letters is a Genie code character
 * (ABCDEFGHJKLMNPRSTVWXYZ0123456789, each representing 5 bits 00000-11111),
 * and each 8-character Genie code is the 24-bit address and 16-bit data.
 *
 * For example, SCRA-BJX0 is a game genie code.  Each letter is 5 bits from
 * the table ABCDEFGHJKLMNPRSTVWXYZ0123456789, A=00000, B=00001, C=00010...
 *
 *   S     C     R     A  -  B     J     X     0
 * 01111 00010 01110 00000 00001 01000 10011 10110
 * ijklm nopIJ KLMNO PABCD EFGHd efgha bcQRS TUVWX   rearrange as...
 *
 * 00000000 10011100 01110110: 01010100 01111000
 * ABCDEFGH IJKLMNOP QRSTUVWX: abcdefgh ijklmnop
 * 24-bit address              16-bit data
 * MSB                    LSB  MSB           LSB
 *
 * Which is 009c76: 5478
 *
 * See the usage message when run with no arguments...
 * Merlyn LeRoy (Brian Westley), merlyn@digibd.com 1/4/93
 */

static const char *patch_codestring = "ABCDEFGHJKLMNPRSTVWXYZ0123456789";

/*** externed ***/

t_patchlist *patch_patchlist; /* patches */

/*** patch_savefile - save current patches to file ***/

int patch_savefile(const char *filename)
{
  FILE *f;
  t_patchlist *ent;

  if ((f = fopen(filename, "wb")) == NULL) {
    LOG_CRITICAL(("Failed to open '%s' for writing: %s",
                  filename, strerror(errno)));
    return -1;
  }
  for (ent = patch_patchlist; ent; ent = ent->next)
    fprintf(f, "%s    %s\r\n", ent->code, ent->action);
  if (fclose(f) == EOF) {
    LOG_CRITICAL(("Failed to close '%s': %s", filename, strerror(errno)));
    return -1;
  }
  return 0;
}

/*** patch_loadfile - load the patch file ***/

int patch_loadfile(const char *filename)
{
  char linebuf[256];
  char *p;
  FILE *f;
  t_patchlist *ent, **end;

  if ((f = fopen(filename, "rb")) == NULL) {
    LOG_CRITICAL(("Failed to open '%s': %s", filename, strerror(errno)));
    return -1;
  }
  patch_clearlist();
  end = &patch_patchlist;
  while (fgets(linebuf, sizeof(linebuf), f)) {
    if (linebuf[strlen(linebuf)-1] != '\n') {
      LOG_CRITICAL(("Line too long in '%s': %s", filename, strerror(errno)));
      return -1;
    }
    linebuf[strlen(linebuf)-1] = '\0'; /* remove newline */
    if (*linebuf && linebuf[strlen(linebuf)-1] == '\r')
      linebuf[strlen(linebuf)-1] = '\0'; /* remove optional cr */
    if (strlen(linebuf) < 11 && linebuf[6] != ':') {
      LOG_CRITICAL(("Invalid patch file '%s'", filename));
      return -1;
    }
    if ((ent = malloc(sizeof(t_patchlist))) == NULL)
      ui_err("out of memory");
    snprintf(ent->code, 12, "%s", linebuf);
    for (p = linebuf + 11; *p == ' '; p++) ;
    if ((ent->action = strdup(p)) == NULL)
      ui_err("out of memory");
    ent->next = NULL;
    *end = ent;
    end = &ent->next;
  }
  if (!feof(f)) {
    LOG_CRITICAL(("Error whilst reading patch file '%s'", filename));
    return -1;
  }
  if (fclose(f) == EOF) {
    LOG_CRITICAL(("Failed to close '%s': %s", filename, strerror(errno)));
    return -1;
  }
  return 0;
}

void patch_clearlist(void)
{
  t_patchlist *ent;

  /* clear out old patch list */
  while (patch_patchlist) {
    ent = patch_patchlist;
    patch_patchlist = patch_patchlist->next;
    free(ent->action);
    free(ent);
  }
}

void patch_addcode(const char *code, const char *action)
{
  t_patchlist *ent, **end;

  /* where's the end of the list */
  for (end = &patch_patchlist; *end; end = &((*end)->next)) ;

  /* create and insert */
  if ((ent = malloc(sizeof(t_patchlist))) == NULL)
    ui_err("out of memory");
  snprintf(ent->code, sizeof(ent->code), "%s", code);
  ent->action = strdup(action);
  ent->next = NULL;
  *end = ent;
}

int patch_apply(const char *code, const char *action)
{
  uint32 addr;
  uint32 data;
  char *end;

  (void)action;
  addr = strtol(code, &end, 16);
  if (*end++ != ':')
    return -1;
  data = strtol(end, &end, 16);
  if (*end)
    return -1;
  if (addr & ~0xfffffe || data & ~0xffff)
    return -1;
  if (addr < cpu68k_romlen) {
    printf("applied %X = %X\n", addr, data);
    ((uint16 *)cpu68k_rom)[addr >> 1] = LOCENDIAN16((uint16)data);
    gen_modifiedrom = 1;
  } else if ((addr & 0xe00000) == 0xe00000) {
    printf("applied %X = %X\n", addr, data);
    ((uint16 *)cpu68k_ram)[(addr & 0xfff) >> 1] = LOCENDIAN16((uint16)data);
  } else {
    return -1;
  }
  return 0;
}

/* given a game genie code, convert to 24-bit address, 16-bit data */

int patch_genietoraw(const char *code, uint32 *addr, uint16 *data)
{
  char *p;
  uint32 a, d, v;
  int i;

  if (strlen(code) != 9 || code[4] != '-')
    return -1;
  for (i = 0; i < 9; i = (i == 3 ? i+2 : i+1)) {
    if ((p = strchr(patch_codestring, code[i])) == NULL)
      return -1;
    v = p - patch_codestring;
    switch (i) {
    case 0: d|= v << 3; break;
    case 1: d|= v >> 2; a|= (v & 3) << 14; break;
    case 2: a|= v << 9; break;
    case 3: a|= (v >> 4) << 8; a|= (v & 15) << 20; break;
    case 5: a|= (v >> 1) << 16; d|= (v & 1) << 12; break;
    case 6: d|= (v >> 1) << 8; d|= (v & 1) << 15; break;
    case 7: d|= (v >> 3) << 13; a|= (v & 7) << 5; break;
    case 8: a|= v; break; 
    default: return -1;
    }
  }
  *addr = a;
  *data = d;
  return 0;
}

/* given a 24-bit address, 16-bit data, create game genie code */

int patch_rawtogenie(uint32 addr, uint16 data, char *code)
{
  uint8 v;
  int i;

  if (addr & 0xff000000)
    return -1;
  code[4] = '-';
  for (i = 0; i < 9; i = (i == 3 ? i+2 : i+1)) {
    switch (i) {
    case 0: v = (data >> 3) & 15; break;
    case 1: v = ((data & 7) << 2) | ((addr >> 14) & 3); break;
    case 2: v = (addr >> 9) & 15; break;
    case 3: v = (addr >> 20) | ((addr >> 4) & 0x10); break;
    case 5: v = ((addr >> 15) & 14) | ((data >> 12) & 1); break;
    case 6: v = ((data >> 8) & 14) | (data >> 15); break;
    case 7: v = ((data >> 10) & 0x18) | ((addr >> 5) & 7); break;
    case 8: v = addr & 15; break;
    }
    code[i] = patch_codestring[v];
  }
  return 0;
}
