/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* generic 640x480 console mode */

/* I call the first field the even field and the second field the odd field,
   sorry - I counted lines from 0 when I wrote the code... strictly speaking
   this is confusing... (it confuses me) */

/* there must be a uip-xxx.c file to go with this user interface - all
   platform dependent calls are in the uip file - see uip.h for the calls
   required */

#include "generator.h"

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
#include "uiplot.h"

#define UI_LOGLINESIZE 128
#define UI_LOGLINES 64

uint32 ui_fkeys = 0;

uint8 ui_vdpsimple = 0;         /* 0=raster, 1=cell based plotter */
uint8 ui_clearnext = 0;         /* flag indicating redraw required */
uint8 ui_fullscreen = 0;        /* does the user want full screen or not */
uint8 ui_info = 1;              /* does the user want info onscreen or not */
uint8 ui_vsync = 0;             /* does the user want us to wait for vsync */
t_binding ui_bindings[2];       /* keyboard/joystick bindings for players */

t_interlace ui_interlace = DEINTERLACE_WEAVEFILTER;

/*** forward reference declarations ***/

static void ui_usage(void);
void ui_exithandler(void);
void ui_newframe(void);
void ui_plotsprite_acorn(uint16 *logo, uint16 width, uint16 height,
                         uint16 x, uint16 y);
int ui_unpackfont(void);
uint16 ui_plotstring(const char *text, uint16 xpos, uint16 ypos);
uint16 ui_plotint2(uint8 num, uint16 xpos, uint16 ypos);
void ui_plotsettings(void);
void ui_drawbox(uint16 colour, uint16 x, uint16 y, uint16 width,
                uint16 height);
void ui_rendertoscreen(void);
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
int ui_setcontrollers(const char *str);

static void ui_simpleplot(void);

/* we store up log lines and dump them right at the end for allegro */
#ifdef ALLEGRO
void ui_printlog(void);
static uint32 ui_logline = 0;
static char ui_loglines[UI_LOGLINES][UI_LOGLINESIZE];
static uint16 ui_logcount = 0;  /* log counter */
#endif

/*** static variables ***/

static uint8 ui_vga = 0;        /* flag for whether in VGA more or not */
static uint8 ui_frameskip = 0;  /* 0 for dynamic */
static uint8 ui_actualskip = 0; /* the last skip we did (1..) */
static uint8 ui_state = 0;      /* 0=stop, 1=paused, 2=play */
static char *ui_initload = NULL;        /* filename to load on init */
static uint8 ui_plotfield = 0;  /* flag indicating plotting this field */
static uint8 ui_plotprevfield = 0;      /* did we plot the previous field? */
static uint16 *ui_font;         /* unpacked font */
static t_uipinfo ui_uipinfo;    /* uipinfo filled in by uip 'sub-system' */
static uint16 *ui_screen0;      /* pointer to screen block for bank 0 */
static uint16 *ui_screen1;      /* pointer to screen block for bank 1 */
static uint16 *ui_newscreen;    /* pointer to new screen block */
static int ui_saverom;          /* flag to save rom and quit */
static int ui_joysticks = 0;    /* number of joysticks */

static uint16 ui_screen[3][320 * 240];     /* screen buffers */

/*** Program entry point ***/

int ui_init(int argc, const char *argv[])
{
  int i;
  int ch;

  ui_bindings[0].joystick = -1; /* -1 = use keyboard */
  ui_bindings[0].keyboard = 0; /* 0 = main keyboard */
  ui_bindings[1].joystick = -1; /* -1 = use keyboard */
  ui_bindings[1].keyboard = -1; /* -1 = no keyboard */

  i = 0; /* supress warning */
#ifdef ALLEGRO
  for (i = 0; i < UI_LOGLINES; i++)
    ui_loglines[i][0] = '\0';
#endif
  if (ui_unpackfont() == -1) {
    fprintf(stderr, "Failed to unpack font, out of memory?\n");
    return 1;
  }
  while (
     -1 != (ch = getopt(argc, deconstify_void_ptr(argv),
                    "i:c:w:f:sd:v:l:j:k:"))
  ) {
    switch (ch) {
    case 'c':                  /* set colour bit positions */
      if (ui_setcolourbits(optarg)) {
        fprintf(stderr, "-c option not understood, must be "
                "'red,green,blue'\n");
        return 1;
      }
      break;
    case 'i':                  /* de-interlacing mode */
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
    case 'd':                  /* enable sound debug mode */
      /* re-write this to cope with comma separated items later */
      if (!strcasecmp(optarg, "sound"))
        sound_debug = 1;
      break;
    case 'j': /* configure keyboard bindings */
      if (ui_setcontrollers(optarg)) {
        fprintf(stderr, "-j option not understood\n");
        return 1;
      }
      break;
    case 's':                  /* save raw format rom */
      ui_saverom = 1;
      break;
    case 'l':                  /* set min fields / audio latency */
      sound_minfields = atoi(optarg);
      break;
    case 'f':                  /* set max fields / audio fragments */
      sound_maxfields = atoi(optarg);
      break;
    case 'k':                  /* set frame skip */
      ui_frameskip = atoi(optarg);
      break;
    case 'v':                  /* set log verbosity level */
      gen_loglevel = atoi(optarg);
      break;
    case 'w':                  /* saved game work dir */
      chdir(optarg);            /* for the moment this will do */
      break;
    case '?':
    default:
      ui_usage();
    }
  }
  argc -= optind;
  argv += optind;

  if (argc < 1 || argc > 3)
    ui_usage();
  if ((ui_initload = malloc(strlen(argv[0]) + 1)) == NULL) {
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
  ui_joysticks = uip_initjoysticks();
  LOG_VERBOSE(("%d joysticks detected", ui_joysticks));

  if (ui_bindings[0].joystick >= ui_joysticks) {
    LOG_CRITICAL(("Invalid joystick selected for first player"));
    return 1;
  }
  if (ui_bindings[1].joystick >= ui_joysticks) {
    LOG_CRITICAL(("Invalid joystick selected for second player"));
    return 1;
  }
  if (ui_bindings[0].joystick == ui_bindings[1].joystick &&
      ui_bindings[0].joystick >= 0) {
    LOG_CRITICAL(("The same joystick was selected for both players!"));
    return 1;
  }
  if (ui_bindings[0].joystick == -1 && ui_bindings[0].joystick == -1) {
    if (ui_bindings[0].keyboard == ui_bindings[1].keyboard) {
      LOG_CRITICAL(("The same keyboard position was selected for "
                    "both players!"));
      return 1;
    }
    if ((ui_bindings[0].keyboard == 0 && ui_bindings[1].keyboard != -1) ||
        (ui_bindings[1].keyboard == 0 && ui_bindings[0].keyboard != -1)) {
      LOG_CRITICAL(("Invalid keyboard configuration"));
      return 1;
    }
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
          "reserved. " VERSION "\n\n");
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
  fprintf(stderr, "  -j <pad1>,<pad2> sets the input device for each joypad "
          "(default key0,none)\n");
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
  for (i = ui_logline; i < (UI_LOGLINES + ui_logline); i++) {
    p = ui_loglines[(i < UI_LOGLINES) ? i : (i - UI_LOGLINES)];
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

  strncpy(s, str, sizeof s);
  s[sizeof s - 1] = '\0';

  if ((green = strchr(s, ',')) == NULL)
    return -1;
  *green++ = '\0';
  if ((blue = strchr(green, ',')) == NULL)
    return -1;
  *blue++ = '\0';
  if ((r = atoi(s)) < 0 || r > 15 ||
      (g = atoi(green)) < 0 || g > 15 || (b = atoi(blue)) < 0 || b > 15)
    return -1;
  if (uip_setcolourbits(r, g, b))
    return -1;
  return 0;
}

/* set controllers for input device from user setting.
   usage: <player1>,<player2>
   where either can be: joy0, joy1, joy2, joy3 - joypad number
                        key0                   - main keyboard
                        key1                   - left side of keyboard
                        key2                   - right side of keyboard
                        none                   - no keyboard
 */

int ui_setcontrollers(const char *str)
{
  char *cont0, *cont1;
  char s[32];
  int i;
  char *p;

  strncpy(s, str, sizeof s);
  s[sizeof s - 1] = '\0';

  cont0 = s;
  if ((cont1 = strchr(s, ',')) == NULL)
    return -1;
  *cont1++ = '\0';

  for (i = 0; i < 2; i++) {
    p = (i == 0 ? cont0 : cont1);
    if (!strncasecmp(p, "key", 3)) {
      if (p[3] < '0' || p[3] > '2')
        return -1;
      if (p[4])
        return -1;
      ui_bindings[i].joystick = -1;
      ui_bindings[i].keyboard = p[3] - '0';
    } else if (!strncasecmp(p, "joy", 3)) {
      if (p[3] < '0' || p[3] > '1')
        return -1;
      if (p[4])
        return -1;
      ui_bindings[i].joystick = p[3] - '0';
      ui_bindings[i].keyboard = -1;
    } else if (!strcasecmp(p, "none")) {
      ui_bindings[i].joystick = -1;
      ui_bindings[i].keyboard = -1;
    } else {
      return -1;
    }
  }
  return 0;
}

/*** ui_drawbox - plot a box ***/

void ui_drawbox(uint16 colour, uint16 x, uint16 y, uint16 width,
                uint16 height)
{
  uint16 *p, *q;
  unsigned int i;

  p = (uint16 *)(ui_uipinfo.screenmem_w + y * ui_uipinfo.linewidth + x * 2);
  q = (uint16 *)((uint8 *)p + (height - 1) * ui_uipinfo.linewidth);
  for (i = 0; i < width; i++)
    *p++ = *q++ = colour;
  p = (uint16 *)(ui_uipinfo.screenmem_w + y * ui_uipinfo.linewidth + x * 2);
  for (i = 0; i < height; i++) {
    p[0] = colour;
    p[width - 1] = colour;
    p = (uint16 *)((uint8 *)p + ui_uipinfo.linewidth);
  }
}

/*** ui_unpackfont - unpack the font file ***/

int ui_unpackfont(void)
{
  unsigned int c, y, x;
  unsigned char packed;
  uint16 *cdata;

  /* 128 chars, 6x, 10y, 16bit */
  if ((ui_font = malloc(128 * 6 * 10 * 2)) == NULL)
    return -1;
  for (c = 0; c < 128; c++) {   /* 128 characters */
    cdata = ui_font + c * 6 * 10;
    for (y = 0; y < 10; y++) {  /* 10 pixels height */
      packed = generator_font[c * 10 + y];
      for (x = 0; x < 6; x++) { /* 6 pixels width */
        *cdata++ = (packed & 1 << (7 - x)) ? 0xffff : 0;
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
  unsigned int x, y;
  uint16 *scr;

  for (p = text; *p; p++) {
    if (*p > 128)
      cdata = ui_font + '.' * 6 * 10;
    else
      cdata = ui_font + (*p) * 6 * 10;
    for (y = 0; y < 10; y++) {
      scr =
        (uint16 *)(ui_uipinfo.screenmem_w +
                   (ypos + y) * ui_uipinfo.linewidth + xpos * 2);
      for (x = 0; x < 6; x++)
        *scr++ = *cdata++;
    }
    xpos += 6;
  }
  return xpos;
}

/*** ui_plotsprite - plot a sprite ***/

void ui_plotsprite_acorn(uint16 *logo, uint16 width, uint16 height,
                         uint16 x, uint16 y)
{
  uint16 a, b;
  uint16 *p;
  uint16 word;
  uint8 red, green, blue;

  for (b = 0; b < height; b++) {
    p =
      (uint16 *)(ui_uipinfo.screenmem_w + (y + b) * ui_uipinfo.linewidth +
                 x * 2);
    for (a = 0; a < width; a++) {
      word = (((uint8 *)logo)[1] << 8) | ((uint8 *)logo)[0];
      green = (word >> 5) & 0x1f;
      red = word & 0x1f;
      blue = (word >> 10) & 0x1f;
      *p++ = red << ui_uipinfo.redshift | green << ui_uipinfo.greenshift |
        blue << ui_uipinfo.blueshift;
      logo++;
    }
  }
}

/*** ui_setupscreen - plot borders and stuff ***/

void ui_setupscreen(void)
{
  uint16 grey = 24 << ui_uipinfo.redshift | 24 << ui_uipinfo.greenshift |
    24 << ui_uipinfo.blueshift;
  uint16 red = 24 << ui_uipinfo.redshift;

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

#if 0
  /* draw marker for 320x224 box */
  ui_drawbox(grey, 160, 128, 320, 224);
  /* draw marker for 320x240 box */
  ui_drawbox(grey, 160, 120, 320, 240);
  /* draw marker for 256x224 box */
  ui_drawbox(grey, 192, 128, 256, 224);
  /* draw marker for 256x240 box */
  ui_drawbox(grey, 192, 120, 256, 240);
#endif

  ui_plotstring("Generator is (c) James Ponder 1997-2001, "
                "all rights reserved.", 0, 0);
  ui_plotstring(VERSION, 640 - (strlen(VERSION) * 6), 0);

  if (ui_bindings[0].joystick != -1) {
    ui_plotstring("Joystick", 0, 420);
  } else {
    if (ui_bindings[0].keyboard == 0) {
      ui_plotstring("[A]       A button", 0, 420);
      ui_plotstring("[S]       B button", 0, 430);
      ui_plotstring("[D]       C button", 0, 440);
      ui_plotstring("[RETURN]  Start", 0, 450);
      ui_plotstring("[ARROWS]  D-pad", 0, 460);
      ui_plotstring("[ESCAPE]  QUIT", 0, 470);
    } else {
      ui_plotstring("[ZXC]     1) A B C", 0, 420);
      ui_plotstring("[DGRF]    1) D-pad", 0, 430);
      ui_plotstring("[V]       1) Start", 0, 440);
      ui_plotstring("[,./]     2) A B C", 0, 450);
      ui_plotstring("[ARROWS]  2) D-pad", 0, 460);
      ui_plotstring("[RETURN]  2) Start", 0, 470);
    }
  }

  ui_plotstring("FPS:  00", 120, 420);
  ui_plotstring("Skip: 00", 120, 430);
  ui_plotstring(":", 622, 470);

  ui_plotstring("[F1] License", 0, 20);
  ui_plotstring("[F2] Load/Save state", 0, 30);
  ui_plotstring("[F3] -", 0, 40);
  ui_plotstring("[F4] Save image", 0, 50);

  ui_plotstring("[F5] Toggle info:", 216, 20);
  ui_plotstring("[F6] Toggle video:", 216, 30);
  ui_plotstring("[F7] Toggle country:", 216, 40);
  ui_plotstring("[F8] Toggle plotter:", 216, 50);

  ui_plotstring("[F9] Toggle vsync:", 432, 20);
  ui_plotstring("[F10] Full screen", 432, 30);
  ui_plotstring("[F11] -", 432, 40);
  ui_plotstring("[F12] Reset", 432, 50);

  ui_plotsettings();

  {
    char buf[4096], *p = buf;
    size_t size = sizeof buf;
    
    p = append_string(p, &size, gen_cartinfo.version);
    p = append_string(p, &size, " ");
    p = append_uint_hex(p, &size, gen_cartinfo.checksum);
    p = append_string(p, &size, gen_cartinfo.country);
    
    ui_plotstring(buf, 216, 420);
  }

  ui_plotstring(gen_cartinfo.name_domestic, 216, 430);
  ui_plotstring(gen_cartinfo.name_overseas, 216, 440);
}

void ui_plotsettings(void)
{
  ui_plotstring(ui_info ? "On " : "Off", 216 + 126, 20);
  ui_plotstring(vdp_pal ? "Pal " : "NTSC", 216 + 126, 30);
  ui_plotstring(vdp_overseas ? "USA/Europe" : "Japan     ", 216 + 126, 40);
  ui_plotstring(ui_vdpsimple ? "Cell (fast)  " : "Raster (slow)", 216 + 126,
                50);
  ui_plotstring(ui_vsync ? "On " : "Off", 216 + 342, 20);
}

/*** ui_loop - main UI loop ***/

int ui_loop(void)
{
  if (ui_initload) {
    const char *error;
    if (NULL != (error = gen_loadimage(ui_initload))) {
      LOG_CRITICAL(("Failed to load ROM image: %s", error));
      return 1;
    }
    if (ui_saverom) {
      char filename[4096], *p = filename;
      size_t size = sizeof filename;
      FILE *f;
      
      p = append_string(p, &size, gen_cartinfo.name_overseas);
      p = append_string(p, &size, " (");
      p = append_uint_hex(p, &size, gen_cartinfo.checksum);
      p = append_string(p, &size, "-");
      p = append_string(p, &size, gen_cartinfo.country);
      p = append_string(p, &size, ")");
     
      f = fopen(filename, "wb"); 
      if (NULL == f) {
        fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
      } else {
        size_t n = cpu68k_romlen;
        
        if (n != fwrite(cpu68k_rom, 1, n, f))
          fprintf(stderr, "Failed to write file: %s\n", strerror(errno));

        fclose(f);
        printf("Successfully wrote: %s\n", filename);
      }
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
  uiplot_setshifts(ui_uipinfo.redshift, ui_uipinfo.greenshift,
                   ui_uipinfo.blueshift);
  gen_quit = 0;
  while (!uip_checkkeyboard()) {
    switch (ui_state) {
    case 0:                    /* stopped */
      break;
    case 1:                    /* paused */
      break;
    case 2:                    /* playing */
      if (ui_fkeys & 1 << 1)
        ui_licensescreen();
      if (ui_fkeys & 1 << 2)
        ui_statescreen();
      if (ui_fkeys & 1 << 4)
        ui_imagescreen();
      if (ui_fkeys & 1 << 5) {
        ui_info ^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1 << 6) {
        vdp_pal ^= 1;
        vdp_setupvideo();
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1 << 7) {
        vdp_overseas ^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1 << 8) {
        ui_vdpsimple ^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1 << 9) {
        ui_vsync ^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1 << 10) {
        ui_fullscreen ^= 1;
        ui_clearnext = 2;
      }
      if (ui_fkeys & 1 << 12)
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
  static char frameplots[60];   /* 60 for NTSC, 50 for PAL */
  static unsigned int frameplots_i = 0;
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
  if (hmode != (vdp_reg[12] & 1))
    ui_clearnext = 2;
  if (ui_clearnext) {
    /* horizontal size has changed, so clear whole screen and setup
       borders etc again */
    ui_clearnext--;
    hmode = vdp_reg[12] & 1;
    memset(uip_whichbank()? ui_screen1 : ui_screen0, 0, sizeof(ui_screen[0]));
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
    ui_plotint2(skipcount + 1, 156, 430);
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

/*** ui_line - it is time to render a line ***/

void ui_line(int line)
{
  static uint8 gfx[320];
  unsigned int width = (vdp_reg[12] & 1) ? 320 : 256;

  if (!ui_plotfield)
    return;
  if (line < 0 || line >= (int)vdp_vislines)
    return;
  if (ui_vdpsimple) { 
    if (line == (int)(vdp_vislines >> 1)) 
      /* if we're in simple cell-based mode, plot when half way
       * down screen */
      ui_simpleplot();
    return;
  }
  /* we are plotting this frame, and we're not doing a simple plot at
     the end of it all */
  switch ((vdp_reg[12] >> 1) & 3) {
  case 0:                    /* normal */
  case 1:                    /* interlace simply doubled up */
  case 2:                    /* invalid */
    vdp_renderline(line, gfx, 0);
    break;
  case 3:                    /* interlace with double resolution */
    vdp_renderline(line, gfx, vdp_oddframe);
    break;
  }
  uiplot_checkpalcache(0);
  uiplot_convertdata16(gfx, ui_newscreen + line * 320, width);
}

static void ui_simpleplot(void)
{
  unsigned int line;
  unsigned int width = (vdp_reg[12] & 1) ? 320 : 256;
  uint8 gfx[(320 + 16) * (240 + 16)];

  /* cell mode - entire frame done here */
  uiplot_checkpalcache(0);
  vdp_renderframe(gfx + (8 * (320 + 16)) + 8, 320 + 16);    /* plot frame */
  for (line = 0; line < vdp_vislines; line++) {
    uiplot_convertdata16(gfx + 8 + (line + 8) * (320 + 16),
                         ui_newscreen + line * 320, width);
  }
}

/*** ui_endfield - end of field reached ***/

void ui_endfield(void)
{
  static int counter = 0;

  if (ui_plotfield) {
    ui_rendertoscreen();        /* plot ui_newscreen to screen */
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
  uint16 **oldscreenpp = uip_whichbank()? &ui_screen1 : &ui_screen0;
  uint16 *scrtmp;
  uint16 *newlinedata, *oldlinedata;
  unsigned int line;
  unsigned int nominalwidth = (vdp_reg[12] & 1) ? 320 : 256;
  unsigned int yoffset = (vdp_reg[1] & 1 << 3) ? 0 : 8;
  unsigned int xoffset = (vdp_reg[12] & 1) ? 0 : 32;
  uint8 *screen;
  uint16 *evenscreen;           /* interlace: lines 0,2,etc. */
  uint16 *oddscreen;            /*            lines 1,3,etc. */

  for (line = 0; line < vdp_vislines; line++) {
    newlinedata = ui_newscreen + line * 320;
    oldlinedata = *oldscreenpp + line * 320;
    if (ui_fullscreen) {
      screen = (ui_uipinfo.screenmem_w + xoffset * 4 +
                ui_uipinfo.linewidth * 2 * (line + yoffset));
      switch ((vdp_reg[12] >> 1) & 3) { /* interlace mode */
      case 0:
      case 1:
      case 2:
        uiplot_render16_x2h(newlinedata, oldlinedata, screen, nominalwidth);
        break;
      case 3:
        /* work out which buffer contains the odd and even fields */
        if (vdp_oddframe) {
          oddscreen = ui_newscreen;
          evenscreen = uip_whichbank()? ui_screen0 : ui_screen1;
        } else {
          evenscreen = ui_newscreen;
          oddscreen = uip_whichbank()? ui_screen0 : ui_screen1;
        }
        switch (ui_interlace) {
        case DEINTERLACE_BOB:
          uiplot_render16_x2(newlinedata, oldlinedata, screen,
                             ui_uipinfo.linewidth, nominalwidth);
          break;
        case DEINTERLACE_WEAVE:
          uiplot_render16_x2h(evenscreen + line * 320, NULL, screen,
                              nominalwidth);
          uiplot_render16_x2h(oddscreen + line * 320, NULL,
                              screen + ui_uipinfo.linewidth, nominalwidth);
          break;
        case DEINTERLACE_WEAVEFILTER:
          /* lines line+0 and line+1 */
          uiplot_irender16_weavefilter(evenscreen + line * 320,
                                       oddscreen + line * 320,
                                       screen, nominalwidth);
          /* lines line+1 and line+2 */
          uiplot_irender16_weavefilter(oddscreen + line * 320,
                                       evenscreen + line * 320 + 320,
                                       screen + ui_uipinfo.linewidth,
                                       nominalwidth);
          break;
        }
        break;
      }
    } else {
      screen = (ui_uipinfo.screenmem_w + 320 + xoffset * 2 +
                ui_uipinfo.linewidth * (120 + line + yoffset));
      uiplot_render16_x1(newlinedata, oldlinedata, screen,
                         nominalwidth);
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
  uint16 grey = 24 << ui_uipinfo.redshift | 24 << ui_uipinfo.greenshift |
    24 << ui_uipinfo.blueshift;
  uint16 red = 24 << ui_uipinfo.redshift;

  ui_plotstring("Generator is (c) James Ponder 1997-2001, "
                "all rights reserved.", 0, 0);
  ui_plotstring(VERSION, 640 - (strlen(VERSION) * 6), 0);
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
  ui_plotstring("SN76496 chip emulation by Nicola Salmoria et al", 160, 160);
#ifdef RAZE
  ui_plotstring("RAZE z80 CPU emulator by Richard Mitton", 160, 170);
#else
  ui_plotstring("Multi-Z80 CPU emulator by Neil Bradley", 160, 170);
  ui_plotstring("  Portable C version written by Edward Massey", 160, 180);
#endif
  ui_plotstring("This program is free software; you can redistribute", 160,
                200);
  ui_plotstring("it and/or modify it under the terms of the GNU", 160, 210);
  ui_plotstring("General Public License version 2 as published by", 160, 220);
  ui_plotstring("the Free Software Foundation.", 160, 230);
  ui_plotstring("http://www.squish.net/generator/", 160, 250);
  ui_plotstring("[0] Return to game", 160, 290);
  while ((c = uip_getchar()) != '0') {
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
  sound_start();
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
    switch ((c = uip_getchar())) {
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
    switch ((c = uip_getchar())) {
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
  for (;;) {
    char c;
    int i;

    uip_clearmiddle();
    if (save)
      ui_plotstring("Save state", 160, 120);
    else
      ui_plotstring("Load state", 160, 120);
    ui_plotstring("----------", 160, 130);
    
    for (i = 1; i <= 9; i++) {
      int y = 140 + 10 * i;
      time_t t;
      
      {
        char buf[64], *p = buf;
        size_t size = sizeof buf;

        p = append_string(p, &size, "[");
        p = append_uint(p, &size, i);
        p = append_string(p, &size, "]");
        ui_plotstring(buf, 160, y);
      }

      if ((t = state_date(i)) == 0) {
        ui_plotstring("empty slot", 160 + 4 * 6, y);
      } else {
        char buf[64];

        strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", localtime(&t));
        ui_plotstring(buf, 160 + 4 * 6, y);
      }
    }
    
    ui_plotstring("[0] ...back", 160, 140 + 10 * 11);
    if ((c = uip_getchar()) == '0')
      return 0;
    if (c >= '1' && c <= '9') {
      /* this is a function call! */
      if ((save ? state_save : state_load) (c - '0') == 0)
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
        char buf[1024], *p = buf;
        size_t size = sizeof buf;

        uip_clearmiddle();
        p = append_string(p, &size, "Successfully saved ");
        p = append_string(p, &size, filename);
        
        ui_plotstring(buf, 160, 120);

        p = buf;
        size = sizeof buf;
        p = append_string(p, &size, "(");
        p = append_uint(p, &size, xsize);
        p = append_string(p, &size, " x ");
        p = append_uint(p, &size, ysize);
        p = append_string(p, &size, " 24 bit raw image)");
        
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
  FILE *f = NULL;
  int i, y;
  char out[3];

  for (i = 1; i <= 9; i++) {
    char *p = filename;
    size_t size = buflen > 0 ? buflen : 0;

    p = append_string(p, &size, gen_leafname);
    p = append_string(p, &size, ".ss");
    p = append_uint(p, &size, i);
    
    if (NULL != fopen(filename, "wb"))
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
        size_t ret;
        
        out[0] = ((line[i] >> ui_uipinfo.redshift) & 0x1f) << 3;
        out[1] = ((line[i] >> ui_uipinfo.greenshift) & 0x1f) << 3;
        out[2] = ((line[i] >> ui_uipinfo.blueshift) & 0x1f) << 3;
        ret = fwrite(out, 1, sizeof out, f);
        if (sizeof out != ret) {
          LOG_CRITICAL(("Error whilst writing to output image file: %s",
                        strerror(errno)));
          fclose(f);
          return -1;
        }
      }
    }
    *xsize = 640;
    *ysize = 480;
  } else if (!strcasecmp(type, "game")) {
    uint16 data;
    uint16 *scr = uip_whichbank()? ui_screen0 : ui_screen1;
    *xsize = (vdp_reg[12] & 1) ? 320 : 256;
    *ysize = vdp_vislines;
    for (y = 0; y < *ysize; y++) {
      for (i = 0; i < *xsize; i++) {
        size_t ret;
        
        data = scr[320 * y + i];
        out[0] = ((data >> ui_uipinfo.redshift) & 0x1f) << 3;
        out[1] = ((data >> ui_uipinfo.greenshift) & 0x1f) << 3;
        out[2] = ((data >> ui_uipinfo.blueshift) & 0x1f) << 3;
        ret = fwrite(out, 1, sizeof out, f);
        if (sizeof out != ret) {
          LOG_CRITICAL(("Error whilst writing to output image file: %s",
                        strerror(errno)));
          fclose(f);
          return -1;
        }
      }
    }
  } else {
    LOG_CRITICAL(("Invalid image type '%s' passed to image save routine",
                  type));
    fclose(f);
    return -1;
  }
  if (fclose(f)) {
    LOG_CRITICAL(("Close returned error whilst writing image"));
    return -1;
  }
  return 0;
}

/*** logging functions ***/

/* logging is done this way because this was the best I could come up with
   whilst battling with macros that can only take fixed numbers of arguments */

#ifdef ALLEGRO
#define LOG_FUNC(name,level,txt) void ui_log_ ## name (const char *text, ...) \
{ \
  va_list ap; \
  if (gen_loglevel >= level) { \
    char *ll = ui_loglines[ui_logline]; \
    char buf[8192], *p = buf; \
    p += sprintf(p, "%d ", ui_logcount++); \
    p += sprintf(p, txt); \
    va_start(ap, text); \
    vsprintf(p, text, ap); \
    va_end(ap); \
    strncpy(ll, buf, UI_LOGLINESIZE); \
    if (++ui_logline >= UI_LOGLINES) \
      ui_logline = 0; \
  } \
}
#else
#define LOG_FUNC(name,level,txt) void ui_log_ ## name (const char *text, ...) \
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

LOG_FUNC(debug3, 7, "DEBG ");
LOG_FUNC(debug2, 6, "DEBG ");
LOG_FUNC(debug1, 5, "DEBG ");
LOG_FUNC(user, 4, "USER ");
LOG_FUNC(verbose, 3, "---- ");
LOG_FUNC(normal, 2, "---- ");
LOG_FUNC(critical, 1, "CRIT ");
LOG_FUNC(request, 0, "---- ");  /* this generates a warning, such is life */

/*** ui_err - log error message and quit ***/

void ui_err(const char *text, ...)
{
  va_list ap;

  printf("ERROR: ");

  va_start(ap, text);
  vfprintf(stderr, text, ap);
  va_end(ap);
  putc(10, stderr);
  exit(0);                      /* don't exit(1) because windows makes an error then! */
}

/*** ui_topbit - given an integer return the top most bit set ***/

int ui_topbit(unsigned long int bits)
{
  long int bit = 31;
  unsigned long int mask = 1 << 31;

  for (; bit >= 0; bit--, mask >>= 1) {
    if (bits & mask)
      return bit;
  }
  return -1;
}

/*** console version doesn't implement music log yet ***/

void ui_musiclog(uint8 *data, unsigned int length)
{
  (void)data;
  (void)length;
}

/* vi: set ts=2 sw=2 et cindent: */
