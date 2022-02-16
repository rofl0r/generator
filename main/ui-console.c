/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* generic 640x480 console mode */

/* I call the first field the even field and the second field the odd field,
   sorry - I counted lines from 0 when I wrote the code... strictly speaking
   this is confusing... (it confuses me) */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "generator.h"
#include "snprintf.h"

#include "cpu68k.h"
#include "cpuz80.h"
#include "ui.h"
#include "vdp.h"
#include "event.h"
#include "gensound.h"
#include "mem68k.h"
#include "logo.h"
#include "font.h"
#include "uip.h"
#include "ui-console.h"
#include "state.h"

#define UI_LOGLINESIZE 128
#define UI_LOGLINES 64

uint32 ui_fkeys = 0;

uint8 ui_vdpsimple = 0;          /* 0=raster, 1=cell based plotter */
uint8 ui_clearnext = 0;          /* flag indicating redraw required */
uint8 ui_fullscreen = 0;         /* does the user want full screen or not */
uint8 ui_info = 1;               /* does the user want info onscreen or not */
uint8 ui_vsync = 0;              /* does the user want us to wait for vsync */
t_interlace ui_interlace = DEINTERLACE_WEAVEFILTER; /* type of de-interlace */

/*** forward reference declarations ***/

static void ui_usage(void);
static void ui_plotline(unsigned int line, uint8 *gfx);
void ui_exithandler(void);
void ui_newframe(void);
void ui_convertdata(uint8 *indata, uint16 *outdata, unsigned int pixels);
void ui_plotsprite_acorn(uint16 *logo, uint16 width, uint16 height,
                	 uint16 x, uint16 y);
int ui_unpackfont(void);
uint16 ui_plotstring(const char *text, uint16 xpos, uint16 ypos);
uint16 ui_plotint2(uint8 num, uint16 xpos, uint16 ypos);
void ui_plotsettings(void);
void ui_drawbox(uint16 colour, uint16 x, uint16 y, uint16 width,
                uint16 height);
void ui_rendertoscreen(void);
void ui_render_x1(uint16 *linedata, uint16 *olddata, uint8 *screen,
                  unsigned int linewidth, unsigned int pixels);
void ui_render_x2(uint16 *linedata, uint16 *olddata, uint8 *screen,
                  unsigned int linewidth, unsigned int pixels);
void ui_render_x2h(uint16 *linedata, uint16 *olddata, uint8 *screen,
                   unsigned int linewidth, unsigned int pixels);
void ui_irender_weavefilter(uint16 *evendata, uint16 *odddata, uint8 *screen,
                            unsigned int linewidth, unsigned int pixels);
void ui_basicbits(void);
void ui_licensescreen(void);
void ui_imagescreen(void);
void ui_imagescreen_main(void);
void ui_statescreen(void);
void ui_statescreen_main(void);
int ui_statescreen_loadsavestate(int save);
void ui_resetscreen(void);
void ui_resetscreen_main(void);
int ui_saveimage(const char *type, char *filename, int buflen,
                 int *xsize, int *ysize);
int ui_setcolourbits(const char *str);

/* we store up log lines and dump them right at the end for allegro */
#ifdef ALLEGRO
void ui_printlog(void);
#endif

/*** static variables ***/

static uint8 ui_vga = 0;           /* flag for whether in VGA more or not */
static uint8 ui_frameskip = 0;     /* 0 for dynamic */
static uint8 ui_actualskip = 0;    /* the last skip we did (1..) */
static uint8 ui_state = 0;         /* 0=stop, 1=paused, 2=play */
static uint32 ui_palcache[192];
static char *ui_initload = NULL;   /* filename to load on init */
static char ui_loglines[UI_LOGLINES][UI_LOGLINESIZE];
static uint32 ui_logline = 0;
static uint8 ui_plotfield = 0;     /* flag indicating plotting this field */
static uint8 ui_plotprevfield = 0; /* did we plot the previous field? */
static uint16 *ui_font;            /* unpacked font */
static uint8 bigbuffer[8192];      /* stupid no-vsnprintf platforms */
static uint16 ui_logcount = 0;     /* log counter */
static t_uipinfo ui_uipinfo;       /* uipinfo filled in by uip 'sub-system' */
static uint16 *ui_screen0;         /* pointer to screen block for bank 0 */
static uint16 *ui_screen1;         /* pointer to screen block for bank 1 */
static uint16 *ui_newscreen;       /* pointer to new screen block */
static int ui_saverom;             /* flag to save rom and quit */

static uint16 ui_screen[3][320*240]; /* screen buffers */

/*** Program entry point ***/

int ui_init(int argc, char *argv[])
{
  int i;
  int ch;

  for(i = 0; i < UI_LOGLINES; i++)
    ui_loglines[i][0] = '\0';
  if (ui_unpackfont() == -1) {
    fprintf(stderr, "Failed to unpack font, out of memory?\n");
    return 1;
  }
  while ((ch = getopt(argc, argv, "i:c:w:f:sd:v:l:")) != -1) {
    switch (ch) {
      case 'c': /* set colour bit positions */
        if (ui_setcolourbits(optarg)) {
          fprintf(stderr, "-c option not understood, must be "
                          "'red,green,blue'\n");
          return 1;
        }
        break;
      case 'i': /* de-interlacing mode */
        if (!strcasecmp(optarg, "bob")) {
          ui_interlace = DEINTERLACE_BOB;
        } else if (!strcasecmp(optarg, "weave")) {
          ui_interlace = DEINTERLACE_WEAVE;
        } else if (!strcasecmp(optarg, "weave-filter")) {
          ui_interlace = DEINTERLACE_WEAVEFILTER;
        } else {
          fprintf(stderr, "-i option not understood, must be "
                          "bob|weave|weave-filter\n");
          return 1;
        }
        break;
      case 'd': /* enable sound debug mode */
        /* re-write this to cope with comma separated items later */
        if (!strcasecmp(optarg, "sound"))
          sound_debug = 1;
        break;
      case 's': /* save raw format rom */
        ui_saverom = 1;
        break;
      case 'l': /* set min fields / audio latency */
        sound_minfields = atoi(optarg);
        break;
      case 'f': /* set max fields / audio fragments */
        sound_maxfields = atoi(optarg);
        break;
      case 'k': /* set frame skip */
        ui_frameskip = atoi(optarg);
        break;
      case 'v': /* set log verbosity level */
        gen_loglevel = atoi(optarg);
        break;
      case 'w': /* saved game work dir */
        chdir(optarg); /* for the moment this will do */
        break;
      case '?':
      default:
        ui_usage();
    }
  }
  argc-= optind;
  argv+= optind;
  
  if (argc < 1 || argc > 3)
    ui_usage();
  if ((ui_initload = malloc(strlen(argv[0])+1)) == NULL) {
    fprintf(stderr, "Out of memory\n");
    return 1;
  }
  strcpy(ui_initload, argv[0]);

  if (atexit(ui_exithandler) == -1) {
    LOG_CRITICAL(("Failed to set exit handler"));
    return 1;
  }
  if (uip_init(&ui_uipinfo)) {
    LOG_CRITICAL(("Failed to initialise platform dependent UI"));
    return 1;
  }
  /* ui_newscreen is where the emulation data is rendered to - it is
     then compared with ui_screen0 or ui_screen1 depending on which
     screen bank it is being written to, and the delta changes are sent
     to screen memory.  The ui_screenX and ui_newscreen pointers are then
     switched, and the next frame/field begins... */
  memset(ui_screen[0], 0, sizeof(ui_screen[0]));
  memset(ui_screen[1], 0, sizeof(ui_screen[1]));
  ui_screen0 = ui_screen[0];
  ui_screen1 = ui_screen[1];
  ui_newscreen = ui_screen[2];
  return 0;
}

void ui_usage(void)
{
  fprintf(stderr, "Generator is (c) James Ponder 1997-2001, all rights "
                  "reserved. " VERSTRING "\n\n");
  fprintf(stderr, "generator [options] <rom>\n\n");
  fprintf(stderr, "  -v <verbose level> can be: 0 = none, 1 = critical, "
                  "2 = normal, 3 = verbose\n");
  fprintf(stderr, "  -d sound - turns on debug mode for sound\n");
  fprintf(stderr, "  -l <sound latency> is the number of video fields worth "
                  "of sound to\n     try to keep buffered (default 5)\n");
  fprintf(stderr, "  -f <sound fragments> is the number of video fields worth "
                  "of sound to\n     allow buffered (approx) (default 10)\n");
  fprintf(stderr, "  -w <work dir> indicates where saved games are stored "
                  "(default is current dir)\n");
  fprintf(stderr, "  -c [<r>,<g>,<b>] sets the colour bit positions of where "
                  "red, green and blue\n     5-bit values should be shifted "
                  "to (default 10,5,0 for 15-bit modes)\n");
  fprintf(stderr, "  -i <mode> selects de-interlace mode, one of: bob "
                  "weave weave-filter\n");
  fprintf(stderr, "  -k <x> forces the frame step (1=all frames, 2=every other,"
                  " etc.)\n");
  fprintf(stderr, "  -s will save the ROM to the work directory in raw "
                  "format\n\n");
  fprintf(stderr, "  ROM types supported: .rom or .smd interleaved "
                  "(autodetected)\n");
  fprintf(stderr, "  Sound latency will be kept between -l and -a values. "
                  "If you are having sound\n  problems try -l 20 -f 30\n");
  exit(1);
}

#ifdef ALLEGRO
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
#endif

void ui_exithandler(void)
{
  if (ui_vga)
    uip_textmode();
  ui_vga = 0;
#ifdef ALLEGRO
  ui_printlog();
#endif
}

void ui_final(void)
{
  if (ui_vga)
    uip_textmode();
  ui_vga = 0;
#ifdef ALLEGRO
  ui_printlog();
#endif
}

int ui_setcolourbits(const char *str)
{
  char *green, *blue;
  char s[32];
  int r, g, b;

  snprintf(s, sizeof(s), "%s", str);

  if ((green = strchr(str, ',')) == NULL)
    return -1;
  *green++ = '\0';
  if ((blue = strchr(green, ',')) == NULL)
    return -1;
  *blue++= '\0';
  if ((r = atoi(s)) < 0 || r > 15 ||
      (g = atoi(green)) < 0 || g > 15 ||
      (b = atoi(blue)) < 0 || b > 15)
    return -1;
  if (uip_setcolourbits(r, g, b))
    return -1;
  return 0;
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

  if (!ui_info) {
    if (!ui_fullscreen) {
      ui_drawbox(red, 140, 100, 360, 280);
      ui_drawbox(grey, 141, 101, 358, 278);
    }
    return;
  }
  if (ui_fullscreen) {
    ui_plotstring(":", 622, 470);
    return;
  }
  ui_drawbox(red, 140, 100, 360, 280);
  ui_drawbox(grey, 141, 101, 358, 278);
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
  ui_plotstring("Generator is (c) James Ponder 1997-2001, "
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
  ui_plotstring("[F1] License", 0, 20);
  ui_plotstring("[F2] Load/Save state", 0, 30);
  ui_plotstring("[F3] -", 0, 40);
  ui_plotstring("[F4] Save image", 0, 50);
  ;
  ui_plotstring("[F5] Toggle info:", 216, 20);
  ui_plotstring("[F6] Toggle video:", 216, 30);
  ui_plotstring("[F7] Toggle country:", 216, 40);
  ui_plotstring("[F8] Toggle plotter:", 216, 50);
  ;
  ui_plotstring("[F9] Toggle vsync:", 432, 20);
  ui_plotstring("[F10] Full screen", 432, 30);
  ui_plotstring("[F11] -", 432, 40);
  ui_plotstring("[F12] Reset", 432, 50);
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
  ui_plotstring(ui_info ? "On " : "Off", 216+126, 20);
  ui_plotstring(vdp_pal ? "Pal " : "NTSC", 216+126, 30);
  ui_plotstring(vdp_overseas ? "USA/Europe" : "Japan     ", 216+126, 40);
  ui_plotstring(ui_vdpsimple ? "Cell (fast)  " : "Raster (slow)", 216+126, 50);
  ui_plotstring(ui_vsync ? "On " : "Off", 216+342, 20);
}

/*** ui_loop - main UI loop ***/

int ui_loop(void)
{
  char *p;
  int f;

  if (ui_initload) {
    p = gen_loadimage(ui_initload);
    if (p) {
      LOG_CRITICAL(("Failed to load ROM image: %s", p));
      return 1;
    }
    if (ui_saverom) {
      snprintf(bigbuffer, sizeof(bigbuffer)-1, "%s (%X-%s)",
      gen_cartinfo.name_overseas, gen_cartinfo.checksum,
      gen_cartinfo.country);
      if ((f = open(bigbuffer, O_CREAT | O_EXCL | O_WRONLY, 0777)) == -1 || 
          write(f, cpu68k_rom, cpu68k_romlen) == -1 || close(f) == -1) {
        fprintf(stderr, "Failed to write file: %s\n", strerror(errno));
      }
      printf("Successfully wrote: %s\n", bigbuffer);
      gen_quit = 1;
      return 0;
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
      if (ui_fkeys & 1<<1)
        ui_licensescreen();
      if (ui_fkeys & 1<<2)
        ui_statescreen();
      if (ui_fkeys & 1<<4)
        ui_imagescreen();
      if (ui_fkeys & 1<<5) {
        ui_info^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1<<6) {
        vdp_pal^= 1;
        vdp_setupvideo();
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1<<7) {
        vdp_overseas^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1<<8) {
        ui_vdpsimple^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1<<9) {
        ui_vsync^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1<<10) {
        ui_fullscreen^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1<<12)
        ui_resetscreen();
      ui_fkeys = 0;
      ui_newframe();
      event_doframe();
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

  /* we store whether we plotted the previous field as this is important
     for doing weave interlacing - we don't want to weave two distant fields */
  ui_plotprevfield = ui_plotfield;

  if (frameplots_i > vdp_framerate)
    frameplots_i = 0;
  if (((vdp_reg[12] >> 1) & 3) && vdp_oddframe) {
    /* interlace mode, and we're about to do an odd field - we always leave
       ui_plotfield alone so we do fields in pairs */
  } else {
    ui_plotfield = 0;
    if (ui_frameskip == 0) {
      if (sound_feedback != -1)
        ui_plotfield = 1;
    } else {
      if (cpu68k_frames % ui_frameskip == 0)
        ui_plotfield = 1;
    }
  }
  if (!ui_plotfield) {
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
    hmode = vdp_reg[12] & 1;
    memset(uip_whichbank() ? ui_screen1 : ui_screen0, 0,
           sizeof(ui_screen[0]));
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
  if (ui_fullscreen) {
    ui_plotint2(fps, 0, 470);
  } else {
    ui_plotint2(fps, 156, 420);
    ui_plotint2(skipcount+1, 156, 430);
    skipcount = 0;
  }
  ui_plotint2(tm_p->tm_hour, 610, 470);
  ui_plotint2(tm_p->tm_min, 628, 470);
  /*
  if (!ui_fullscreen) {
    int y, i, s;
    for (i = 0; i < 8; i++) {
      for (s = 0; s < 4; s++) {
        ui_plotstring((sound_keys[i] & (1<<s)) ? "*" : ".",
                      510+(5*s), 200+10*i);
      }
    }
  }
  */
}

/* ui_checkpalcache goes through the CRAM memory in the Genesis and 
   converts it to the ui_palcache table.  The Genesis has 64 colours,
   but we store three versions of the colour table into ui_palcache - a
   normal, hilighted and dim version.  The vdp_cramf buffer has 64
   entries and is set to 1 when the game writes to CRAM, this means this
   code skips entries that have not changed, unless 'flag' is set to 1 in
   which case this updates all entries regardless. */

void ui_checkpalcache(int flag)
{
  uint32 pal;
  unsigned int col;
  uint8 *p;

  /* this code requires that there be at least 4 bits per colour, that
     is, three bits that come from the console's palette, and one more bit
     when we do a dim or bright colour, i.e. this code works with 12bpp
     upwards */

  /* the flag forces it to do the update despite the vdp_cramf buffer */

  for (col = 0; col < 64; col++) { /* the CRAM has 64 colours */
    if (!flag && !vdp_cramf[col])
      continue;
    vdp_cramf[col] = 0;
    p = (uint8 *)vdp_cram+2*col; /* point p to the two-byte CRAM entry */
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

void ui_convertdata(uint8 *indata, uint16 *outdata, unsigned int pixels)
{
  unsigned int ui;
  uint32 outdata1;
  uint32 outdata2;
  uint32 indata_val;

  ui_checkpalcache(0);
  /* not scaled, 16bpp - we do 4 pixels at a time */
  for (ui = 0; ui < pixels>>2; ui++) {
    indata_val = ((uint32 *)indata)[ui]; /* 4 bytes of in data */
    outdata1 = (ui_palcache[indata_val & 0xff] |
                ui_palcache[(indata_val>>8) & 0xff]<<16);
    outdata2 = (ui_palcache[(indata_val>>16) & 0xff] |
                ui_palcache[(indata_val>>24) & 0xff]<<16);
#ifdef WORDS_BIGENDIAN
    ((uint32 *)outdata)[(ui<<1)] = outdata2;
    ((uint32 *)outdata)[(ui<<1)+1] = outdata1;
#else
    ((uint32 *)outdata)[(ui<<1)] = outdata1;
    ((uint32 *)outdata)[(ui<<1)+1] = outdata2;
#endif
  }
}

/*** ui_render_x1 - copy to screen with delta changes ***/

void ui_render_x1(uint16 *linedata, uint16 *olddata, uint8 *screen,
                  unsigned int linewidth, unsigned int pixels)
{
  unsigned int ui;
  uint32 inval;

  if (0==1) {
    memcpy(screen, linedata, pixels*2);
  } else {
    for (ui = 0; ui < pixels>>1; ui++) {
      inval = ((uint32 *)linedata)[ui]; /* two words of input data */
      if (inval != ((uint32 *)olddata)[ui]) {
        ((uint32 *)screen)[ui] = inval;
      }
    }
  }
}

/*** ui_render_x2 - blow up screen image by two */

void ui_render_x2(uint16 *linedata, uint16 *olddata, uint8 *screen,
                  unsigned int linewidth, unsigned int pixels)
{
  unsigned int ui;
  uint32 inval, outval, mask;
  uint8 *screen2 = screen + linewidth;

  for (ui = 0; ui < pixels>>1; ui++) {
    inval = ((uint32 *)linedata)[ui];          /* two words of input data */
    mask = inval ^ ((uint32 *)olddata)[ui];    /* check for changed data */
    if (mask & 0xffff) {
      outval = inval & 0xffff;
      outval|= outval<<16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui<<1)+1]  = outval; /* write first two words */
      ((uint32 *)screen2)[(ui<<1)+1] = outval;
#else
      ((uint32 *)screen)[(ui<<1)]  = outval;   /* write first two words */
      ((uint32 *)screen2)[(ui<<1)] = outval;
#endif
    }
    if (mask & 0xffff0000) {
      outval = inval & 0xffff0000;
      outval|= outval>>16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui<<1)]  = outval;   /* write second two words */
      ((uint32 *)screen2)[(ui<<1)] = outval;
#else
      ((uint32 *)screen)[(ui<<1)+1]  = outval; /* write second two words */
      ((uint32 *)screen2)[(ui<<1)+1] = outval;
#endif
    }
  }
}

/*** ui_render_x2h - blow up by two in horizontal direction only */

void ui_render_x2h(uint16 *linedata, uint16 *olddata, uint8 *screen,
                   unsigned int linewidth, unsigned int pixels)
{
  unsigned int ui;
  uint32 inval, outval, mask;

  mask = 0xffffffff;
  for (ui = 0; ui < pixels>>1; ui++) {
    inval = ((uint32 *)linedata)[ui];          /* two words of input data */
    if (olddata)
      mask = inval ^ ((uint32 *)olddata)[ui];    /* check for changed data */
    if (mask & 0xffff) {
      outval = inval & 0xffff;
      outval|= outval<<16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui<<1)+1]  = outval; /* write first two words */
#else
      ((uint32 *)screen)[(ui<<1)]  = outval;   /* write first two words */
#endif
    }
    if (mask & 0xffff0000) {
      outval = inval & 0xffff0000;
      outval|= outval>>16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui<<1)]  = outval;   /* write second two words */
#else
      ((uint32 *)screen)[(ui<<1)+1]  = outval; /* write second two words */
#endif
    }
  }
}

/*** ui_irender_weavefilter - take even and odd fields, filter and plot ***/

void ui_irender_weavefilter(uint16 *evendata, uint16 *odddata, uint8 *screen,
                            unsigned int linewidth, unsigned int pixels)
{
  unsigned int ui;
  uint32 evenval, oddval, e_r, e_g, e_b, o_r, o_g, o_b;
  uint32 outval, w1, w2;

  for (ui = 0; ui < pixels>>1; ui++) {
    evenval = ((uint32 *)evendata)[ui];          /* two words of input data */
    oddval = ((uint32 *)odddata)[ui];           /* two words of input data */
    e_r = (evenval >> ui_uipinfo.redshift) & 0x001f001f;
    e_g = (evenval >> ui_uipinfo.greenshift) & 0x001f001f;
    e_b = (evenval >> ui_uipinfo.blueshift) & 0x001f001f;
    o_r = (oddval >> ui_uipinfo.redshift) & 0x001f001f;
    o_g = (oddval >> ui_uipinfo.greenshift) & 0x001f001f;
    o_b = (oddval >> ui_uipinfo.blueshift) & 0x001f001f;
    outval = (((e_r+o_r)>>1) & 0x001f001f) << ui_uipinfo.redshift |
             (((e_g+o_g)>>1) & 0x001f001f) << ui_uipinfo.greenshift |
             (((e_b+o_b)>>1) & 0x001f001f) << ui_uipinfo.blueshift;
    w1 = (outval & 0xffff);
    w1|= w1<<16;
    w2 = outval & 0xffff0000;
    w2|= w2>>16;
#ifdef WORDS_BIGENDIAN
      ((uint32 *)screen)[(ui<<1)] = w2;
      ((uint32 *)screen)[(ui<<1)+1] = w1;
      LOG_CRITICAL(("this code hasn't been checked"));
#else
      ((uint32 *)screen)[(ui<<1)] = w1;
      ((uint32 *)screen)[(ui<<1)+1] = w2;
#endif
  }
}

/*** ui_line - it is time to render a line ***/

void ui_line(unsigned int line)
{  
  static uint8 gfx[320];
  unsigned int yoffset = (vdp_reg[1] & 1<<3) ? 0 : 8;
  unsigned int width = (vdp_reg[12] & 1) ? 320 : 256;

  if (ui_plotfield && !ui_vdpsimple) {
    /* we are plotting this frame, and we're not doing a simple plot at
       the end of it all */
    if (((vdp_reg[12]>>1) & 3) == 3)
      vdp_renderline(line, gfx, vdp_oddframe); /* interlace */
    else
      vdp_renderline(line, gfx, 0);
    ui_convertdata(gfx, ui_newscreen+line*320, width);
  }
}

/*** ui_endfield - end of field reached ***/

void ui_endfield(void)
{
  static int counter = 0;
  unsigned int line;
  unsigned int width = (vdp_reg[12] & 1) ? 320 : 256;
  uint8 gfx[(320+16)*(240+16)];

  if (ui_plotfield) {
    if (ui_vdpsimple) {
      /* cell mode - entire frame done here */
      vdp_renderframe(gfx+(8*(320+16))+8, 320+16); /* plot frame */
      for (line = 0; line < vdp_vislines; line++) {
              ui_convertdata(gfx+8+(line+8)*(320+16),
                             ui_newscreen+line*320, width);
      }
    }
    ui_rendertoscreen(); /* plot ui_newscreen to screen */
  }
  if (ui_frameskip == 0) {
    /* dynamic frame skipping */
    counter++;
    if (sound_feedback >= 0) {
      ui_actualskip = counter;
      counter = 0;
    }
  } else {
    ui_actualskip = ui_frameskip;
  }
}

void ui_rendertoscreen(void)
{
  uint16 **oldscreenpp = uip_whichbank() ? &ui_screen1 : &ui_screen0;
  uint16 *scrtmp;
  uint16 *newlinedata, *oldlinedata;
  unsigned int line;
  unsigned int nominalwidth = (vdp_reg[12] & 1) ? 320 : 256;
  unsigned int yoffset = (vdp_reg[1] & 1<<3) ? 0 : 8;
  unsigned int xoffset = (vdp_reg[12] & 1) ? 0 : 32;
  uint8 *screen;
  uint16 *evenscreen; /* interlace: lines 0,2,etc. */
  uint16 *oddscreen;  /*            lines 1,3,etc. */

  for (line = 0; line < vdp_vislines; line++) {
    newlinedata = ui_newscreen+line*320;
    oldlinedata = *oldscreenpp+line*320;
    if (ui_fullscreen) {
      screen = (ui_uipinfo.screenmem_w+xoffset*4+
                ui_uipinfo.linewidth*2*(line+yoffset));
      switch ((vdp_reg[12] >> 1) & 3) { /* interlace mode */
      case 0:
      case 1:
      case 2:
        ui_render_x2h(newlinedata, oldlinedata, screen,
                      ui_uipinfo.linewidth, nominalwidth);
        break;
      case 3:
        /* work out which buffer contains the odd and even fields */
        if (vdp_oddframe) {
          oddscreen = ui_newscreen;
          evenscreen = uip_whichbank() ? ui_screen0 : ui_screen1;
        } else {
          evenscreen = ui_newscreen;
          oddscreen = uip_whichbank() ? ui_screen0 : ui_screen1;
        }
        switch (ui_interlace) {
        case DEINTERLACE_BOB:
          ui_render_x2(newlinedata, oldlinedata, screen,
                       ui_uipinfo.linewidth, nominalwidth);
          break;
        case DEINTERLACE_WEAVE:
          ui_render_x2h(evenscreen+line*320, NULL, screen,
                        ui_uipinfo.linewidth, nominalwidth);
          ui_render_x2h(oddscreen+line*320, NULL,
                        screen+ui_uipinfo.linewidth,
                        ui_uipinfo.linewidth, nominalwidth);
          break;
        case DEINTERLACE_WEAVEFILTER:
          ui_irender_weavefilter(evenscreen+line*320,         /* line   */
                                 oddscreen+line*320,          /* line+1 */
                                 screen,
                                 ui_uipinfo.linewidth, nominalwidth);
          ui_irender_weavefilter(oddscreen+line*320,         /* line+1 */
                                 evenscreen+line*320+320,      /* line+2 */
                                 screen+ui_uipinfo.linewidth,
                                 ui_uipinfo.linewidth, nominalwidth);
          break;
        }
        break;
      }
    } else {
      screen = (ui_uipinfo.screenmem_w+320+xoffset*2+
                ui_uipinfo.linewidth*(120+line+yoffset));
      ui_render_x1(newlinedata, oldlinedata, screen,
                   ui_uipinfo.linewidth, nominalwidth);
    }
  }
  uip_displaybank(-1);
  if (ui_vsync)
    uip_vsync();
  /* swap ui_screenX and ui_newscreen pointers */
  scrtmp = *oldscreenpp;
  *oldscreenpp = ui_newscreen;
  ui_newscreen = scrtmp;
}

void ui_basicbits(void)
{
  uint16 grey = 24<<ui_uipinfo.redshift | 24<<ui_uipinfo.greenshift |
                24<<ui_uipinfo.blueshift;
  uint16 red = 24<<ui_uipinfo.redshift;

  ui_plotstring("Generator is (c) James Ponder 1997-2001, "
                "all rights reserved.", 0, 0);
  ui_plotstring(VERSTRING, 640-(strlen(VERSTRING)*6), 0);
  ui_drawbox(red, 140, 100, 360, 280);
  ui_drawbox(grey, 141, 101, 358, 278);
  ui_plotsprite_acorn((uint16 *)logo, 282, 40, 179, 80);
  ui_plotsprite_acorn((uint16 *)logo, 282, 40, 179, 360);
}

void ui_licensescreen(void)
{
  char c;

  sound_stop();
  uip_singlebank();
  uip_clearscreen();
  ui_basicbits();
  /*            #01234567890123456789012345678901234567890123456789012 */
  ui_plotstring("Generator - Sega Genesis (Mega Drive) emulator", 160, 120);
  ui_plotstring("Copyright (C) 1997-2001 James Ponder", 160, 130);
  ui_plotstring("YM2612 chip emulation by Tatsuyuki Satoh", 160, 150);
  ui_plotstring("Multi-Z80 CPU emulator by Neil Bradley", 160, 160);
  ui_plotstring("  Portable C version written by Edward Massey", 160, 170);
  ui_plotstring("This program is free software; you can redistribute", 160,
                190);
  ui_plotstring("it and/or modify it under the terms of the GNU", 160, 200);
  ui_plotstring("General Public License version 2 as published by", 160, 210);
  ui_plotstring("the Free Software Foundation.", 160, 220);
  ui_plotstring("http://www.squish.net/generator/", 160, 240);
  ui_plotstring("[0] Return to game", 160, 280);
  while((c = uip_getchar()) != '0') {
  }
  uip_doublebank();
  ui_clearnext = 2;
  sound_start();
}

void ui_resetscreen(void)
{
  sound_stop();
  uip_singlebank();
  uip_clearscreen();
  ui_basicbits();
  ui_resetscreen_main();
  uip_doublebank();
  ui_clearnext = 2;
  sound_final();
}

void ui_resetscreen_main(void)
{
  char c;

  for (;;) {
    uip_clearmiddle();
    ui_plotstring("Reset menu", 160, 120);
    ui_plotstring("----------", 160, 130);
    ui_plotstring("[1] Soft reset", 160, 140);
    ui_plotstring("[2] Hard reset", 160, 150);
    ui_plotstring("[0] Return to game", 160, 170);
    ;
    switch((c = uip_getchar())) {
    case '0':
      return;
    case '1':
      gen_softreset();
      return;
    case '2':
      gen_reset();
      return;
    }
  }
}

void ui_statescreen(void)
{
  sound_stop();
  uip_singlebank();
  uip_clearscreen();
  ui_basicbits();
  ui_statescreen_main();
  uip_doublebank();
  ui_clearnext = 2;
  sound_start();
}

void ui_statescreen_main(void)
{
  char c;

  for (;;) {
    uip_clearmiddle();
    ui_plotstring("Load / Save state menu", 160, 120);
    ui_plotstring("----------------------", 160, 130);
    ui_plotstring("[1] Load state", 160, 140);
    ui_plotstring("[2] Save state", 160, 150);
    ui_plotstring("[0] Return to game", 160, 170);
    ;
    switch((c = uip_getchar())) {
    case '0':
      return;
    case '1':
      if (ui_statescreen_loadsavestate(0))
        return;
      break;
    case '2':
      if (ui_statescreen_loadsavestate(1))
        return;
      break;
    }
  }
}

int ui_statescreen_loadsavestate(int save)
{
  char c;
  time_t t;
  char buf[64];
  int i, y;

  for (;;) {
    uip_clearmiddle();
    if (save)
      ui_plotstring("Save state", 160, 120);
    else
      ui_plotstring("Load state", 160, 120);
    ui_plotstring("----------", 160, 130);
    for (i = 1; i <= 9; i++) {
      y = 140 + 10 * i;
      snprintf(buf, sizeof(buf), "[%d]", i);
      ui_plotstring(buf, 160, y);
      if ((t = state_date(i)) == 0) {
        ui_plotstring("empty slot", 160+4*6, y);
      } else {
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
        ui_plotstring(buf, 160+4*6, y);
      }
    }
    ui_plotstring("[0] ...back", 160, 140+10*11);
    if ((c = uip_getchar()) == '0')
      return 0;
    if (c >= '1' && c <= '9') {
      /* this is a function call! */
      if ((save ? state_save : state_load)(c - '0') == 0)
        return 1;
      uip_clearmiddle();
      if (save)
        ui_plotstring("Failed to save file.", 160, 120);
      else
        ui_plotstring("Failed to load file.", 160, 120);
      ui_plotstring("Press any key to continue", 160, 140);
      uip_getchar();
    }
  }
}

void ui_imagescreen(void)
{
  sound_stop();
  uip_singlebank();
  uip_clearscreen();
  ui_basicbits();
  ui_imagescreen_main();
  uip_doublebank();
  ui_clearnext = 2;
  sound_start();
}

void ui_imagescreen_main(void)
{
  char c;
  char filename[256];
  char buf[256];
  int xsize, ysize;

  for (;;) {
    uip_clearmiddle();
    ui_plotstring("Save screen shot menu", 160, 120);
    ui_plotstring("---------------------", 160, 130);
    ui_plotstring("[1] Save svga screen (640x480)", 160, 140);
    ui_plotstring("[2] Save game screen", 160, 150);
    ui_plotstring("[0] Return to game", 160, 170);
    ;
    c = uip_getchar();
    if (c == '0') {
      return;
    } else if (c == '1' || c == '2') {
      if (ui_saveimage(c == '1' ? "svga" : "game", filename, 256,
                       &xsize, &ysize)) {
        uip_clearmiddle();
        ui_plotstring("Failed to save image.", 160, 120);
        ui_plotstring("Press any key to continue", 160, 140);
        uip_getchar();
      } else {
        uip_clearmiddle();
        snprintf(buf, sizeof(buf), "Successfully saved %s", filename);
        ui_plotstring(buf, 160, 120);
        snprintf(buf, sizeof(buf), "(%d x %d 24 bit raw image)", xsize, ysize);
        ui_plotstring(buf, 160, 130);
        ui_plotstring("Press any key to continue", 160, 150);
        uip_getchar();
      }
    }
  }
}

int ui_saveimage(const char *type, char *filename, int buflen,
                 int *xsize, int *ysize)
{
  int i, y;
  int fd = 0;
  char out[3];
  int count;

  for (i = 1; i <= 9; i++) {
    snprintf(filename, buflen, "%s.ss%d", gen_leafname, i);
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644)) != -1)
      break;
  }
  if (i < 1 || i > 9) {
    *filename = '\0';
    LOG_CRITICAL(("Error - there are already 9 saved images"));
    return -1;
  }
  if (!strcasecmp(type, "svga")) {
    uint8 *scr;
    uint16 *line;
    /* get the bank that we're not writing/reading to now */
    scr = (ui_uipinfo.screenmem_w == ui_uipinfo.screenmem0) ?
        ui_uipinfo.screenmem1 : ui_uipinfo.screenmem0;
    for (y = 0; y < 480; y++) {
      line = (uint16 *)(scr + y * ui_uipinfo.linewidth);
      for (i = 0; i < 640; i++) {
        out[0] = ((line[i]>>ui_uipinfo.redshift) & 0x1f) << 3;
        out[1] = ((line[i]>>ui_uipinfo.greenshift) & 0x1f) << 3;
        out[2] = ((line[i]>>ui_uipinfo.blueshift) & 0x1f) << 3;
        count = write(fd, out, 3);
        if (count == -1) {
          LOG_CRITICAL(("Error whilst writing to output image file: %s",
                        strerror(errno)));
          close(fd);
          return -1;
        } else if (count != 3) {
          LOG_CRITICAL(("Short write - wrote 3 bytes, got %d", count));
          close(fd);
          return -1;
        }
      }
    }
    *xsize = 640;
    *ysize = 480;
  } else if (!strcasecmp(type, "game")) {
    uint16 data;
    uint16 *scr = uip_whichbank() ? ui_screen0 : ui_screen1;
    *xsize = (vdp_reg[12] & 1) ? 320 : 256;
    *ysize = vdp_vislines;
    for (y = 0; y < *ysize; y++) {
      for (i = 0; i < *xsize; i++) {
        data = scr[320*y+i];
        out[0] = ((data>>ui_uipinfo.redshift) & 0x1f) << 3;
        out[1] = ((data>>ui_uipinfo.greenshift) & 0x1f) << 3;
        out[2] = ((data>>ui_uipinfo.blueshift) & 0x1f) << 3;
        count = write(fd, out, 3);
        if (count == -1) {
          LOG_CRITICAL(("Error whilst writing to output image file: %s",
                        strerror(errno)));
          close(fd);
          return -1;
        } else if (count != 3) {
          LOG_CRITICAL(("Short write - wrote 3 bytes, got %d", count));
          close(fd);
          return -1;
        }
      }
    }
  } else {
    LOG_CRITICAL(("Invalid image type '%s' passed to image save routine",
                  type));
    close(fd);
    return -1;
  }
  if (close(fd)) {
    LOG_CRITICAL(("Close returned error whilst writing image"));
    return -1;
  }
  return 0;
}

/*** logging functions ***/

/* logging is done this way because this was the best I could come up with
   whilst battling with macros that can only take fixed numbers of arguments */

#ifdef ALLEGRO
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
#else
#define LOG_FUNC(name,level,txt) void ui_log_ ## name ## (const char *text, ...) \
{ \
  va_list ap; \
  if (gen_loglevel >= level) { \
    printf("[%s] ", txt); \
    va_start(ap, text); \
    vprintf(text, ap); \
    va_end(ap); \
    putchar(10); \
  } \
}
#endif

LOG_FUNC(debug3,   7, "DEBG ");
LOG_FUNC(debug2,   6, "DEBG ");
LOG_FUNC(debug1,   5, "DEBG ");
LOG_FUNC(user,     4, "USER ");
LOG_FUNC(verbose,  3, "---- ");
LOG_FUNC(normal,   2, "---- ");
LOG_FUNC(critical, 1, "CRIT ");
LOG_FUNC(request,  0, "---- "); /* this generates a warning, such is life */

/*** ui_err - log error message and quit ***/

void ui_err(const char *text, ...)
{
  va_list ap;

  printf("ERROR: ");

  va_start(ap, text);
  vfprintf(stderr, text, ap);
  va_end(ap);
  putc(10, stderr);
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
