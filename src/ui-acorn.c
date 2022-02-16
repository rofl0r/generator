/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-1998       */
/*****************************************************************************/
/*                                                                           */
/* ui.c - Acorn user interface                                               */
/*                                                                           */
/* *** THIS IS OUT OF DATE / NOT SUPPORTED, AND WON'T WORK ***               */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include <sys/swis.h>
#include <sys/os.h>

#include <unistd.h>
#include <sys/time.h>

#include <generator.h>
#include <ui.h>
#include <vdp.h>
#include <cpu68k.h>
#include <reg68k.h>

typedef struct {
  uint32 flags;
  uint32 xres;
  uint32 yres;
  uint32 pixdepth;
  uint32 framerate;
  uint32 settings;
} t_modeblock;

static uint8 *screenaddr;
static char file[256];
static unsigned int ui_bank;

int ui_init(int argc, char *argv[])
{
  uint32 r[10];
  os_error *errblk;
  t_modeblock *mb = malloc(sizeof(t_modeblock)+8*4);
  uint32 *vars;
  uint32 in[2];
  uint32 out[2];
  uint32 i;

  strcpy(file, argv[1]);

  if (!mb) {
    fprintf(stderr, "Out of memory!\n");
    exit(1);
  }

  vars = &(mb->settings);

  mb->flags = 1;
  mb->xres = 320;
  mb->yres = 480;
  mb->pixdepth = 3;
  mb->framerate = 60;
  *vars++ = 0;     /* ModeFlags */
  *vars++ = 128;
  *vars++ = 3;     /* NColour */
  *vars++ = 255;
  *vars++ = 4;     /* X eig factor */
  *vars++ = 2;
  *vars++ = 5;     /* Y eig factor */
  *vars++ = 2;
  *vars++ = -1;
  r[0] = (uint32)0;
  r[1] = (uint32)mb;
  errblk = os_swi(OS_ScreenMode, r);
  if (errblk) {
    fprintf(stderr, errblk->errmess);
    exit(1);
  }
  free(mb);
  r[0] = 2;
  os_swi(OS_ReadDynamicArea, r);
  if (r[1] < 320*480*2) {
    r[1]-= 320*480*2;
    i = r[1];
    errblk = os_swi(OS_ChangeDynamicArea, r);
    if (errblk) {
      fprintf(stderr, errblk->errmess);
      exit(1);
    }
    if (r[1] != i) {
      fprintf(stderr, "Cannot allocate enough screen memory\n");
      exit(1);
    }
  }
  in[0] = 149;
  in[1] = -1;
  r[0] = (uint32)in;
  r[1] = (uint32)out;
  errblk = os_swi(OS_ReadVduVariables, r);
  if (errblk) {
    fprintf(stderr, errblk->errmess);
    exit(1);
  }
  screenaddr = (uint8 *)(out[0]);

  cpu68k_skip = 2;
  vdp_basepixel = 32;

  ui_bank = 0;

  return(0);
}

char *ui_setinfo(char *name, char *copyright)
{
  return 0;
}

int ui_loop(void)
{
  char *retstr;

  if ((retstr = gen_loadimage(file))) {
    fprintf(stderr, retstr);
    exit(1);
  }
  ui_run();
  return 0;
}

void ui_line(unsigned int line)
{
  vdp_renderline(line, screenaddr+ui_bank*320*480+640*line);
}

void ui_endframe(void)
{
  unsigned int i;
  uint16 ent;
  static uint32 cbuf[256];
  uint32 r[10];
  uint8 red, green, blue;

  ui_bank^= 1;

  r[0] = 19;
  r[1] = 0;
  r[2] = 0;
  os_swi(OS_Byte, r);

  r[0] = 113;
  r[1] = (ui_bank^1) + 1;
  os_swi(OS_Byte, r);

  r[0] = 112;
  r[1] = ui_bank + 1;
  os_swi(OS_Byte, r);

  cbuf[0] = 0;
  cbuf[1] = 0xFFFFFF00;
  cbuf[255] = 0xFFFFFF00;

  for (i = 0; i < 64; i++) {
    ent = LOCENDIAN16(((uint16 *)vdp_cram)[i]);
    blue = (ent>>9) & 7;
    green = (ent>>5) & 7;
    red = (ent>>1) & 7;
    cbuf[vdp_basepixel+i]     = blue<<29 | green<<21 | red<<13;
    cbuf[vdp_basepixel+64+i]  = blue<<28 | green<<20 | red<<12;
    cbuf[vdp_basepixel+128+i] = 0x80808000 | (blue<<28 | green<<20 | red<<12);
  }
  r[0] = -1;
  r[1] = -1;
  r[2] = (uint32)cbuf;
  r[3] = 0;
  r[4] = 0;
  os_swi(ColourTrans_WritePalette, r);
  return;
}

void ui_run(void)
{
  uint32 r[10];
  uint32 lasttime = 0;
  uint32 frames = 0;
  uint32 clocks = 0;
  struct timeval tv;
  char tmp[100];
  uint32 start;

  gettimeofday(&tv, NULL);
  start = tv.tv_sec;

  while(0==0) {
    reg68k_framestep();
    gettimeofday(&tv, NULL);
    if (tv.tv_sec != lasttime) {
      os_vdu(7);
      os_vdu(4);
      os_vdu(31);
      os_vdu(0);
      os_vdu(59);
      sprintf(tmp, "%02d %08X %04d", cpu68k_frames - frames, regs.pc,
	      tv.tv_sec - start);
      r[0] = (uint32)tmp;
      os_swi(OS_Write0, r);
      frames = cpu68k_frames;
      clocks = reg68k_clocks;
      lasttime = tv.tv_sec;;
    }
    /*
    if (cpu68k_frames > 2000) {
      os_swi(OS_ReadC, r);
    }
    */
  }
}

void ui_final(void)
{
  return;
}
int atexit(void (*func)(void)) {
  return -1;
}
