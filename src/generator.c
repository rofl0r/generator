/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* generator.c                                                               */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#include "generator.h"
#include "ui.h"
#include "memz80.h"
#include "mem68k.h"
#include "cpu68k.h"
#include "cpuz80.h"
#include "vdp.h"
#include "gensound.h"

#ifdef ALLEGRO
#include "allegro.h"
#endif

/*** variables externed in generator.h ***/

unsigned int gen_quit = 0;
unsigned int gen_debugmode = 1;
unsigned int gen_loglevel = 1; /* 2 = NORMAL, 1 = CRITICAL */
t_cartinfo gen_cartinfo;

/*** forward references ***/

void gen_nicetext(char *out, char *in, unsigned int size);
uint16 gen_checksum(uint8 *start, unsigned int length);

/*** Signal handler ***/

RETSIGTYPE gen_sighandler(int signum)
{
  if (gen_debugmode) {
    if (signum == SIGINT) {
      if (gen_quit) {
        LOG_CRITICAL(("Bye!"));
	ui_final();
	ui_err("Exiting");
      } else {
	LOG_REQUEST(("Ping - current PC = 0x%X", regs.pc));
      }
      gen_quit = 1;
    }
  } else {
    ui_final();
    exit(0);
  }
  signal(signum, gen_sighandler);
}

static char thing[] = ("\n\nIt's the year 2000 is there anyone out there?\n"
		       "\nIf you're from another planet put your hand in the "
		       "air, say yeah!\n\n");

/*** Program entry point ***/

int main(int argc, char *argv[]) {
  int retval;
  t_sr test;

  test.sr_int = 0;
  test.sr_struct.c = 1;
  if (test.sr_int != 1) {
    fprintf(stderr, "%s: compilation variable BYTES_HIGHFIRST not set "
	    "correctly\n", argv[0]);
    return 1;
  }

  /*  chdir(GAMEDIR); */

  /* initialise user interface */
  if ((retval = ui_init(argc, argv)))
    return retval;

  /* initialise 68k memory system */
  if ((retval = mem68k_init()))
    ui_err("Failed to initialise mem68k module (%d)", retval);

  /* initialise z80 memory system */
  if ((retval = memz80_init()))
    ui_err("Failed to initialise memz80 module (%d)", retval);

  /* initialise vdp system */
  if ((retval = vdp_init()))
    ui_err("Failed to initialise vdp module (%d)", retval);

  /* initialise cpu system */
  if ((retval = cpu68k_init()))
    ui_err("Failed to initialise cpu68k module (%d)", retval);

  /* initialise z80 cpu system */
  if ((retval = cpuz80_init()))
    ui_err("Failed to initialise cpuz80 module (%d)", retval);

  /* initialise sound system */
  if ((retval = sound_init()))
    ui_err("Failed to initialise sound module (%d)", retval);

  signal(SIGINT, gen_sighandler);

  /* enter user interface loop */
  return ui_loop();
}

#ifdef ALLEGRO
END_OF_MAIN();
#endif

/*** gen_reset - reset system ***/

void gen_reset(void) {
  vdp_reset();
  cpu68k_reset();
  cpuz80_reset();
  if (sound_reset()) {
    ui_err("sound failure");
  }
}

/*** gen_loadimage - load ROM image ***/

char *gen_loadimage(const char *filename) {
  int file, imagetype, bytes, bytesleft;
  struct stat statbuf;
  const char *extension;
  uint8 *buffer;
  char countrybuf[32];
  unsigned int blocks, x, i;
  uint8 *new;
  char *p;

  /* Remove current file */
  if (cpu68k_rom) {
    free(cpu68k_rom);
    cpu68k_rom = NULL;
  }

  /* Load file */
  if (stat(filename, &statbuf) != 0) {
    return("Unable to stat file.");
  }
  cpu68k_romlen = statbuf.st_size;
  /* allocate enough memory plus 16 bytes for disassembler to cope
     with the last instruction */
  if ((cpu68k_rom = malloc(cpu68k_romlen+16)) == NULL) {
    cpu68k_romlen = 0;
    return("Out of memory!");
  }
  memset(cpu68k_rom, 0, cpu68k_romlen+16);
#ifdef ALLEGRO
  if ((file = open(filename, O_RDONLY|O_BINARY, 0)) == -1) {
#else
  if ((file = open(filename, O_RDONLY, 0)) == -1) {
#endif
    perror("open");
    cpu68k_rom = NULL;
    cpu68k_romlen = 0;
    return("Unable to open file.");
  }
  buffer = cpu68k_rom;
  bytesleft = cpu68k_romlen;
  do {
    if ((bytes = read(file, buffer, bytesleft)) <= 0)
      break;
    buffer+= bytes;
    bytesleft-= bytes;
  } while (bytesleft >= 0);
  close(file);
  if (bytes == -1)
    return(strerror(errno));
  else if (bytes != 0)
    return("invalid return code from read()");
  if (bytesleft) {
    LOG_CRITICAL(("%d bytes left to read?!", bytesleft));
    return("Error whilst loading file");
  }

  imagetype = 1; /* BIN file by default */

  /* SMD file format check - Richard Bannister */
  if ((cpu68k_rom[8] == 0xAA) && (cpu68k_rom[9] == 0xBB) &&
      cpu68k_rom[10] == 0x06) {
    imagetype = 2; /* SMD file */
  }
  /* check for interleaved 'SEGA' */
  if (cpu68k_rom[0x280] == 'E' && cpu68k_rom[0x281] == 'A' &&
      cpu68k_rom[0x2280] == 'S' && cpu68k_rom[0x2281] == 'G') {
    imagetype = 2; /* SMD file */
  }

#ifndef OS_ACORN
  /* Check extension is not wrong */
  extension = filename + strlen(filename) - 3;
  if (extension > filename) {
    if (!strcasecmp(extension, "smd") && (imagetype != 2))
      LOG_REQUEST(("File extension (smd) does not match detected "
		   "type (bin)"));
    if (!strcasecmp(extension, "bin") && (imagetype != 1))
      LOG_REQUEST(("File extension (bin) does not match detected "
		   "type (smd)"));
  }
#endif

  /* convert to standard BIN file format */

  switch(imagetype) {
  case 1: /* BIN */
    break;
  case 2: /* SMD */
    blocks = (cpu68k_romlen-512)/16384;
    if (blocks*16384+512 != cpu68k_romlen)
      return("Image is corrupt.");

    if ((new = malloc(cpu68k_romlen-512)) == NULL) {
      cpu68k_rom = NULL;
      cpu68k_romlen = 0;
      return("Out of memory!");
    }

    for (i = 0; i < blocks; i++) {
      for (x = 0; x < 8192; x++) {
        new[i*16384+x*2+0] = cpu68k_rom[512+i*16384+8192+x];
        new[i*16384+x*2+1] = cpu68k_rom[512+i*16384+x];
      }
    }
    free(cpu68k_rom);
    cpu68k_rom = new;
    cpu68k_romlen-= 512;
    break;
  default:
    return("Unknown image type");
    break;
  }

  /* reset system */
  gen_reset();

  memset(&gen_cartinfo, 0, sizeof(gen_cartinfo));
  gen_nicetext(gen_cartinfo.console, (char *)(cpu68k_rom+0x100), 16);
  gen_nicetext(gen_cartinfo.copyright, (char *)(cpu68k_rom+0x110), 16);
  gen_nicetext(gen_cartinfo.name_domestic, (char *)(cpu68k_rom+0x120), 48);
  gen_nicetext(gen_cartinfo.name_overseas, (char *)(cpu68k_rom+0x150), 48);
  if (cpu68k_rom[0x180] == 'G' && cpu68k_rom[0x181] == 'M') {
    gen_cartinfo.prodtype = pt_game;
  } else if (cpu68k_rom[0x180] == 'A' && cpu68k_rom[0x181] == 'I') {
    gen_cartinfo.prodtype = pt_education;
  } else {
    gen_cartinfo.prodtype = pt_unknown;
  }
  gen_nicetext(gen_cartinfo.version, (char *)(cpu68k_rom+0x182), 12);
  gen_cartinfo.checksum = gen_checksum(((uint8 *)cpu68k_rom)+0x200,
				       cpu68k_romlen-0x200);
  gen_nicetext(gen_cartinfo.memo, (char *)(cpu68k_rom+0x1C8), 28);
  for (i = 0x1f0; i < 0x1ff; i++) {
    if (cpu68k_rom[i] == 'J')
      gen_cartinfo.flag_japan = 1;
    if (cpu68k_rom[i] == 'U')
      gen_cartinfo.flag_usa = 1;
    if (cpu68k_rom[i] == 'E')
      gen_cartinfo.flag_europe = 1;
  }
  if (cpu68k_rom[0x1f0] >= '1' && cpu68k_rom[0x1f0] <= '9') {
    gen_cartinfo.hardware = cpu68k_rom[0x1f0] - '0';
  } else if (cpu68k_rom[0x1f0] >= 'A' && cpu68k_rom[0x1f0] <= 'F') {
    gen_cartinfo.hardware = cpu68k_rom[0x1f0] - 'A' + 10;
  }
  p = gen_cartinfo.country;
  for (i = 0x1f0; i < 0x200; i++) {
    if (cpu68k_rom[i] != 0 && cpu68k_rom[i] != 32)
      *p++ = cpu68k_rom[i];
  }
  *p = '\0';

  ui_setinfo(&gen_cartinfo);

  if (gen_cartinfo.checksum != (cpu68k_rom[0x18e]<<8 | cpu68k_rom[0x18f]))
    LOG_REQUEST(("Warning: Checksum does not match in ROM"));

  LOG_REQUEST(("Loaded '%s'/'%s' (%s %X %s)", gen_cartinfo.name_domestic,
	       gen_cartinfo.name_overseas, gen_cartinfo.version,
	       gen_cartinfo.checksum, gen_cartinfo.country));

  return NULL;
}

/*** get_nicetext - take a string, remove spaces and capitalise ***/

void gen_nicetext(char *out, char *in, unsigned int size)
{
  int flag, i;
  char c;
  char *start = out;

  flag = 0; /* set if within word, e.g. make lowercase */
  i = size; /* maximum number of chars in input*/
  while ((c = *in++) && i--) {
    if (isalpha((int)c)) {
      if (!flag) {
        /* make uppercase */
	flag = 1;
	if (islower((int)c)) {
	  *out++ = c - 'z'-'Z';
	} else {
	  *out++ = c;
	}
      } else {
	/* make lowercase */
	if (isupper((int)c)) {
	  *out++ = (c) + 'z'-'Z';
	} else {
	  *out++ = c;
	}
      }
      continue;
    }
    if (c == ' ' && !flag) {
	continue;
    }
    *out++ = c;
    flag = 0;
  }
  if (out > start && out[-1] == ' ')
    out--;
  *out++ = '\0';
}

/*** gen_checksum - get Genesis-style checksum of memory block ***/

uint16 gen_checksum(uint8 *start, unsigned int length)
{
  uint16 checksum = 0;

  for (; length; length-= 2, start+= 2) {
    checksum+= start[0]<<8;
    checksum+= start[1];
  }
  return checksum;
}

/*** gen_loadsavepos - load saved position ***/

char *gen_loadsavepos(const char *filename)
{
  LOG_REQUEST(("gen_loadsavepos: %s\n", filename));
  return("Not implemented.");
}