/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2000       */
/*****************************************************************************/
/*                                                                           */
/* ui-console - generic 640x480 console emulation                            */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "generator.h"

#include "cpu68k.h"
#include "cpuz80.h"
#include "ui.h"
#include "vdp.h"
#include "reg68k.h"
#include "gensound.h"
#include "mem68k.h"
#include "logo.h"
#include "font.h"
#include "uip.h"
#include "ui-console.h"

#define UI_LOGLINESIZE 128
#define UI_LOGLINES 64

uint8 ui_vdpsimple = 0;          /* 0=raster, 1=cell based plotter */
uint8 ui_clearnext = 0;          /* flag indicating redraw required */
uint8 ui_info = 1;               /* does the user want info onscreen or not */
uint8 ui_vsync = 0;              /* does the user want us to wait for vsync */

/*** forward reference declarations ***/

void ui_endframe(void);
void ui_exithandler(void);
void ui_printlog(void);
void ui_newframe(void);
void ui_convertdata(uint8 *indata, uint8 *outdata, unsigned int pixels);
void ui_plotsprite_acorn(uint16 *logo, uint16 width, uint16 height,
			 uint16 x, uint16 y);
int ui_unpackfont(void);
uint16 ui_plotstring(const char *text, uint16 xpos, uint16 ypos);
uint16 ui_plotint2(uint8 num, uint16 xpos, uint16 ypos);
void ui_plotsettings(void);
void ui_drawbox(uint16 colour, uint16 x, uint16 y, uint16 width,
		uint16 height);

/*** static variables ***/

static uint8 ui_vga = 0;         /* flag for whether in VGA more or not */
static uint8 ui_frameskip = 0;   /* 0 for dynamic */
static uint8 ui_actualskip = 0;  /* the last skip we did (1..) */
static uint8 ui_state = 0;       /* 0=stop, 1=paused, 2=play */
static uint32 ui_palcache[192];
static char *ui_initload = NULL; /* filename to load on init */
static char ui_loglines[UI_LOGLINES][UI_LOGLINESIZE];
static uint32 ui_logline = 0;
static uint8 ui_plotframe;       /* flag indicating plotting this frame */
static uint16 *ui_font;          /* unpacked font */
static uint8 bigbuffer[8192];    /* stupid no-vsnprintf platforms */
static uint16 ui_logcount = 0;   /* log counter */
static t_uipinfo ui_uipinfo;     /* uipinfo filled in by uip 'sub-system' */

/*** Program entry point ***/

int ui_init(int argc, char *argv[])
{
  int i;

  for(i = 0; i < UI_LOGLINES; i++) {
    ui_loglines[i][0] = '\0';
  }
  if (ui_unpackfont() == -1) {
    fprintf(stderr, "Failed to unpack font, out of memory?\n");
    return 1;
  }
  if (argc == 2) {
    if ((ui_initload = malloc(strlen(argv[1])+1)) == NULL) {
      fprintf(stderr, "Out of memory\n");
      return 1;
    }
    strcpy(ui_initload, argv[1]);
  }
  if (atexit(ui_exithandler) == -1) {
    LOG_CRITICAL(("Failed to set exit handler"));
    return 1;
  }
  if (uip_init(&ui_uipinfo)) {
    LOG_CRITICAL(("Failed to initialise platform dependent UI"));
    return 1;
  }
  return 0;
}

void ui_printlog(void)
{
  unsigned int i;
  char *p;

  printf("**** Last lines logged:\n");
  for (i = ui_logline; i < (UI_LOGLINES+ui_logline); i++) {
    p = ui_loglines[(i < UI_LOGLINES) ? i : (i-UI_LOGLINES)];
    if (*p)
      printf("%s\n", p);
  }
  printf("**** END\n");
}

void ui_exithandler(void)
{
  if (ui_vga)
    uip_textmode();
  ui_vga = 0;
  ui_printlog();
}

void ui_final(void)
{
  if (ui_vga)
    uip_textmode();
  ui_vga = 0;
  ui_printlog();
}

/*** ui_drawbox - plot a box ***/

void ui_drawbox(uint16 colour, uint16 x, uint16 y, uint16 width,
		uint16 height)
{
  uint16 *p, *q;
  unsigned int i;

  p = (uint16 *)(ui_uipinfo.screenmem_w+y*ui_uipinfo.linewidth+x*2);
  q = (uint16 *)((uint8 *)p+(height-1)*ui_uipinfo.linewidth);
  for (i = 0; i < width; i++)
    *p++ = *q++ = colour;
  p = (uint16 *)(ui_uipinfo.screenmem_w+y*ui_uipinfo.linewidth+x*2);
  for (i = 0; i < height; i++) {
    p[0] = colour;
    p[width-1] = colour;
    p = (uint16 *)((uint8 *)p+ui_uipinfo.linewidth);
  }
}

/*** ui_unpackfont - unpack the font file ***/

int ui_unpackfont(void)
{
  unsigned int c, y, x;
  unsigned char packed;
  uint16 *cdata;

  if ((ui_font = malloc(128*6*10*2)) == NULL) /* 128 chars, 6x, 10y, 16bit */
    return -1;
  for (c = 0; c < 128; c++) { /* 128 characters */
    cdata = ui_font+c*6*10;
    for (y = 0; y < 10; y++) { /* 10 pixels height */
      packed = generator_font[c*10+y];
      for (x = 0; x < 6; x++) { /* 6 pixels width */
	*cdata++ = (packed & 1<<(7-x)) ? 0xffff : 0;
      }
    }
  }
  return 0;
}
  
/*** ui_plotint2 - plot a 2-char wide integer ***/

uint16 ui_plotint2(uint8 num, uint16 xpos, uint16 ypos)
{
  char buf[3];

  buf[0] = '0' + (num / 10) % 10;
  buf[1] = '0' + (num % 10);
  buf[2] = '\0';
  return ui_plotstring(buf, xpos, ypos);
}

/*** ui_plotstring - plot a string ***/

uint16 ui_plotstring(const char *text, uint16 xpos, uint16 ypos)
{
  const unsigned char *p;
  const uint16 *cdata;
  unsigned int x,y;
  uint16 *scr;

  for (p = text; *p; p++) {
    if (*p > 128)
      cdata = ui_font+'.'*6*10;
    else
      cdata = ui_font+(*p)*6*10;
    for (y = 0; y < 10; y++) {
      scr = (uint16 *)(ui_uipinfo.screenmem_w+(ypos+y)*ui_uipinfo.linewidth
		       +xpos*2);
      for (x = 0; x < 6; x++)
        *scr++ = *cdata++;
    }
    xpos+= 6;
  }
  return xpos;
}

/*** ui_plotsprite - plot a sprite ***/

void ui_plotsprite_acorn(uint16 *logo, uint16 width, uint16 height,
			 uint16 x, uint16 y)
{
  uint16 a, b;
  uint16 *p, *q;
  uint16 word;
  uint8 red, green, blue;

  for (b = 0; b < height; b++) {
    p = (uint16 *)(ui_uipinfo.screenmem_w+(y+b)*ui_uipinfo.linewidth+x*2);
    for (a = 0; a < width; a++) {
      word = (((uint8 *)logo)[1]<<8) | ((uint8 *)logo)[0];
      green = (word>>5) & 0x1f;
      red = word & 0x1f;
      blue = (word>>10) & 0x1f;
      *p++ = red<<ui_uipinfo.redshift | green<<ui_uipinfo.greenshift | 
	blue<<ui_uipinfo.blueshift;
      logo++;
    }
  }
}

/*** ui_setupscreen - plot borders and stuff ***/

void ui_setupscreen(void)
{
  uint16 grey = 24<<ui_uipinfo.redshift | 24<<ui_uipinfo.greenshift |
    24<<ui_uipinfo.blueshift;
  uint16 red = 24<<ui_uipinfo.redshift;

  ui_drawbox(red, 140, 100, 360, 280);
  ui_drawbox(grey, 141, 101, 358, 278);
  if (!ui_info)
    return;
  ui_plotsprite_acorn((uint16 *)logo, 282, 40, 179, 80);
  ui_plotsprite_acorn((uint16 *)logo, 282, 40, 179, 360);
  ui_drawbox(red, 0, 415, 640, 1);
  ui_drawbox(grey, 0, 416, 640, 1);
  ui_drawbox(red, 0, 65, 640, 1);
  ui_drawbox(grey, 0, 66, 640, 1);
  ;
  /* draw marker for 320x224 box */
  // ui_drawbox(grey, 160, 128, 320, 224);
  /* draw marker for 320x240 box */
  // ui_drawbox(grey, 160, 120, 320, 240);
  /* draw marker for 256x224 box */
  // ui_drawbox(grey, 192, 128, 256, 224);
  /* draw marker for 256x240 box */
  // ui_drawbox(grey, 192, 120, 256, 240);
  ;
  ui_plotstring("Generator is (c) James Ponder 1997-2000, "
		"all rights reserved.", 0, 0);
  ui_plotstring(VERSTRING, 640-(strlen(VERSTRING)*6), 0);
  ;
  ui_plotstring("[A]       A button", 0, 420);
  ui_plotstring("[S]       B button", 0, 430);
  ui_plotstring("[D]       C button", 0, 440);
  ui_plotstring("[RETURN]  Start",    0, 450);
  ui_plotstring("[ARROWS]  D-pad",    0, 460);
  ui_plotstring("[ESCAPE]  QUIT",     0, 470);
  ;
  ui_plotstring("FPS:  00", 120, 420);
  ui_plotstring("Skip: 00", 120, 430);
  ui_plotstring(":", 622, 470);
  ;
  ui_plotstring("[F5] Toggle info:", 0, 20);
  ui_plotstring("[F6] Toggle video:", 0, 30);
  ui_plotstring("[F7] Toggle country:", 0, 40);
  ui_plotstring("[F8] Toggle plotter:", 0, 50);
  ui_plotstring("[F9] Toggle vsync:", 216, 20);
  ui_plotstring("[F12] Restart game", 216, 50);
  ;
  ui_plotstring("[1] B lo:", 64*6, 20);
  ui_plotstring("[2] B hi:", 64*6, 30);
  ui_plotstring("[3] A lo:", 64*6, 40);
  ui_plotstring("[4] A hi:", 64*6, 50);
  ui_plotstring("[5] W lo:", 78*6, 20);
  ui_plotstring("[6] W hi:", 78*6, 30);
  ui_plotstring("[7] H:   ", 78*6, 40);
  ui_plotstring("[8] S lo:", 92*6, 20);
  ui_plotstring("[9] S hi:", 92*6, 30);
  ;
  ui_plotsettings();
  ;
  sprintf(bigbuffer, "%s %X %s", gen_cartinfo.version,
	  gen_cartinfo.checksum, gen_cartinfo.country);
  ui_plotstring(bigbuffer, 216, 420);
  ui_plotstring(gen_cartinfo.name_domestic, 216, 430);
  ui_plotstring(gen_cartinfo.name_overseas, 216, 440);
}

void ui_plotsettings(void)
{
  ui_plotstring(ui_info ? "On " : "Off", 126, 20);
  ui_plotstring(vdp_pal ? "Pal " : "NTSC", 126, 30);
  ui_plotstring(vdp_overseas ? "USA/Europe" : "Japan     ", 126, 40);
  ui_plotstring(ui_vdpsimple ? "Cell (fast)  " : "Raster (slow)", 126, 50);
  ui_plotstring(ui_vsync ? "On " : "Off", 342, 20);
  ;
  ui_plotstring(vdp_layerB  ? "On " : "Off", (64+10)*6, 20);
  ui_plotstring(vdp_layerBp ? "On " : "Off", (64+10)*6, 30);
  ui_plotstring(vdp_layerA  ? "On " : "Off", (64+10)*6, 40);
  ui_plotstring(vdp_layerAp ? "On " : "Off", (64+10)*6, 50);
  ui_plotstring(vdp_layerW  ? "On " : "Off", (78+10)*6, 20);
  ui_plotstring(vdp_layerWp ? "On " : "Off", (78+10)*6, 30);
  ui_plotstring(vdp_layerH  ? "On " : "Off", (78+10)*6, 40);
  ui_plotstring(vdp_layerS  ? "On " : "Off", (92+10)*6, 20);
  ui_plotstring(vdp_layerSp ? "On " : "Off", (92+10)*6, 30);
}

/*** ui_loop - main UI loop ***/

int ui_loop(void)
{
  char *p;

  if (ui_initload) {
    p = gen_loadimage(ui_initload);
    if (p) {
      LOG_CRITICAL(("Failed to load ROM image: %s", p));
      return 1;
    }
    ui_state = 2;
  } else {
    LOG_CRITICAL(("You must specify a ROM to load"));
    return 1;
  }
  if (uip_vgamode()) {
    LOG_CRITICAL(("Failed to start VGA mode"));
    return 1;
  }
  gen_quit = 0;
  while(!uip_checkkeyboard()) {
    switch(ui_state) {
    case 0: /* stopped */
      break;
    case 1: /* paused */
      break;
    case 2: /* playing */
      ui_newframe();
      reg68k_framestep();
      break;
    }
  }
  return 0;
}

void ui_newframe(void)
{
  static int hmode = 0;
  static int skipcount = 0;
  static char frameplots[60]; /* 60 for NTSC, 50 for PAL */
  static unsigned int frameplots_i = 0;
  char buf[8];
  unsigned int i;
  int fps;
  time_t t = time(NULL);
  struct tm *tm_p = localtime(&t);

  if (frameplots_i > vdp_framerate)
    frameplots_i = 0;
  ui_plotframe = 0;
  if (ui_frameskip == 0) {
    if (sound_feedback != -1)
      ui_plotframe = 1;
  } else {
    if (cpu68k_frames % ui_frameskip == 0)
      ui_plotframe = 1;
  }
  if (!ui_plotframe) {
    skipcount++;
    frameplots[frameplots_i++] = 0;
    return;
  }
  if (hmode != (vdp_reg[12]&1))
    ui_clearnext = 2;
  if (ui_clearnext) {
    /* horizontal size has changed, so clear whole screen and setup
       borders etc again */
    ui_clearnext--;
    hmode = vdp_reg[12]&1;
    uip_clearscreen();
    ui_setupscreen();
  }
  /* count the frames we've plotted in the last vdp_framerate real frames */
  fps = 0;
  for (i = 0; i < vdp_framerate; i++) {
    if (frameplots[i])
      fps++;
  }
  frameplots[frameplots_i++] = 1;
  if (!ui_info)
    return;
  ui_plotint2(fps, 156, 420);
  ui_plotint2(skipcount+1, 156, 430);
  skipcount = 0;
  ui_plotint2(tm_p->tm_hour, 610, 470);
  ui_plotint2(tm_p->tm_min, 628, 470);
}

void ui_checkpalcache(int flag)
{
  uint32 pal;
  unsigned int col;
  uint8 *p;

  /* this code requires that there be at least 4 bits per colour, that
     is, three bits that come from the console's palette, and one more bit
     when we do a dim or bright colour, i.e. this code works with 12bpp
     upwards */

  /* this code assumes 5 bits per colour in it's calculations */

  /* the flag forces it to do the check despite the vdp_cramf buffer */

  for (col = 0; col < 64; col++) {
    if (!flag && !vdp_cramf[col])
      continue;
    vdp_cramf[col] = 0;
    p = (uint8 *)vdp_cram+2*col;
    ui_palcache[col] = /* normal */
      (p[0]&0xE)<<(ui_uipinfo.blueshift+1) |
      (p[1]&0xE)<<(ui_uipinfo.redshift+1) |
      ((p[1]&0xE0)>>4)<<(ui_uipinfo.greenshift+1);
    ui_palcache[col+64] = /* hilight */
      (p[0]&0xE)<<ui_uipinfo.blueshift |
      (p[1]&0xE)<<ui_uipinfo.redshift |
      ((p[1]&0xE0)>>4)<<ui_uipinfo.greenshift |
      (16 << ui_uipinfo.blueshift) | (16 << ui_uipinfo.redshift) |
      (16 << ui_uipinfo.greenshift);
    ui_palcache[col+128] = /* shadow */
      (p[0]&0xE)<<ui_uipinfo.blueshift |
      (p[1]&0xE)<<ui_uipinfo.redshift |
      ((p[1]&0xE0)>>4)<<ui_uipinfo.greenshift;
  }
}

/*** ui_convertdata - convert genesis data to 16 bit colour */

void ui_convertdata(uint8 *indata, uint8 *outdata, unsigned int pixels)
{
  unsigned int ui;
  uint32 data;

  ui_checkpalcache(0);
  /* not scaled, 16bpp */
  for (ui = 0; ui < pixels; ui++) {
    data = ui_palcache[indata[ui]];
    ((uint16 *)outdata)[ui] = data; /* already in local endian */
  }
}

/*** ui_line - it is time to render a line ***/

void ui_line(unsigned int line)
{  
  static uint8 gfx[320];
  uint16 *scr;

  if (ui_plotframe && !ui_vdpsimple) {
    if (((vdp_reg[12]>>1) & 3) == 3)
      vdp_renderline_interlace2(line*2+vdp_oddframe, (uint8 *)gfx);
    else
      vdp_renderline(line, (uint8 *)gfx);
    ui_convertdata(gfx, (ui_uipinfo.screenmem_w+320+((vdp_reg[12]&1)?0:64)+
                         ui_uipinfo.linewidth*(120+line+
					       ((vdp_reg[1]&1<<3)?0:8))),
                   (vdp_reg[12]&1)?320:256);
  }
}

/*** ui_endfield - end of field reached ***/

void ui_endfield(void)
{
  ui_endframe();
}

/*** ui_endframe - end of frame reached ***/

void ui_endframe(void)
{
  static uint8 gfx[(320+16)*(240+16)];
  static int counter = 0;
  unsigned int line;
  uint16 *scr;

  if (ui_plotframe) {
    if (ui_vdpsimple) {
      /* cell mode - entire frame done here */
      vdp_renderframe(gfx+(8*(320+16))+8, 320+16); /* plot frame */
      for (line = 0; line < vdp_vislines; line++) {
        ui_convertdata(gfx+8+(line+8)*(320+16),
                       (ui_uipinfo.screenmem_w+320+((vdp_reg[12]&1)?0:64)+
                         ui_uipinfo.linewidth*(120+line+
					       ((vdp_reg[1]&1<<3)?0:8))),
                       (vdp_reg[12]&1)?320:256);
      }
    }
    uip_displaybank(-1);
    if (ui_vsync)
      uip_vsync();
  }
  if (ui_frameskip == 0) {
    counter++;
    if (sound_feedback >= 0) {
      ui_actualskip = counter;
      counter = 0;
    }
  } else {
    ui_actualskip = ui_frameskip;
  }
}

/*** logging functions ***/

/* logging is done this way because this was the best I could come up with
   whilst battling with macros that can only take fixed numbers of arguments */

#define LOG_FUNC(name,level,txt) void ui_log_ ## name ## (const char *text, ...) \
{ \
  va_list ap; \
  char *ll = ui_loglines[ui_logline]; \
  char *p = bigbuffer; \
  if (gen_loglevel >= level) { \
    p+= sprintf(p, "%d ", ui_logcount++); \
    p+= sprintf(p, txt); \
    va_start(ap, text); \
    vsprintf(p, text, ap); \
    va_end(ap); \
    strncpy(ll, bigbuffer, UI_LOGLINESIZE); \
    if (++ui_logline >= UI_LOGLINES) \
      ui_logline = 0; \
  } \
}

LOG_FUNC(debug3,   7, "DEBG ");
LOG_FUNC(debug2,   6, "DEBG ");
LOG_FUNC(debug1,   5, "DEBG ");
LOG_FUNC(user,     4, "USER ");
LOG_FUNC(verbose,  3, "---- ");
LOG_FUNC(normal,   2, "---- ");
LOG_FUNC(critical, 1, "CRIT ");
LOG_FUNC(request,  0, "---- ");

/*** ui_err - log error message and quit ***/

void ui_err(const char *text, ...)
{
  va_list ap;
  char *ll = ui_loglines[ui_logline];
  char *p = bigbuffer;

  p+= sprintf(p, "ERROR: ");
  va_start(ap, text);
  vsprintf(p, text, ap);
  va_end(ap);
  strncpy(ll, bigbuffer, UI_LOGLINESIZE);
  if (++ui_logline >= UI_LOGLINES)
    ui_logline = 0;
  ui_printlog();
  exit(0); /* don't exit(1) because windows makes an error then! */
}

/*** ui_setinfo - there is new cart information ***/

char *ui_setinfo(t_cartinfo *cartinfo)
{
  return NULL;
}

/*** ui_topbit - given an integer return the top most bit set ***/

int ui_topbit(unsigned long int bits)
{
  long int bit = 31;
  unsigned long int mask = 1<<31;

  for (; bit >= 0; bit--, mask>>= 1) {
    if (bits & mask)
      return bit;
  }
  return -1;
}
