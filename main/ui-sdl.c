/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* SDL user interface */

#include "generator.h"

#include <pwd.h>

#include "SDL.h"

#include "ui.h"
#include "uiplot.h"
#include "gtkopts.h"

#include "vdp.h"
#include "gensound.h"
#include "cpu68k.h"
#include "mem68k.h"
#include "cpuz80.h"
#include "event.h"
#include "state.h"
#include "initcart.h"
#include "patch.h"
#include "dib.h"
#include "avi.h"
#include "opengl.h"

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* !MIN */

#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* !MAX */

#define HBORDER_MAX 32
#define HBORDER_DEFAULT 8

#define VBORDER_MAX 32
#define VBORDER_DEFAULT 8

#define HMAXSIZE (320 + 2 * HBORDER_MAX)
#define VMAXSIZE (240 + 2 * VBORDER_MAX)

#define HSIZE (320 + 2 * hborder)
#define VSIZE ((vdp_vislines ? vdp_vislines : 224) + 2 * vborder)

#define DISSWIN_MAX 64

static struct {
  unsigned int a;
  unsigned int b;
  unsigned int c;
  unsigned int up;
  unsigned int down;
  unsigned int left;
  unsigned int right;
  unsigned int start;
} ui_cont[2] = {
  {
    SDLK_a, SDLK_s, SDLK_d,
    SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_RETURN
  },
  {
    SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP_MINUS,
    SDLK_KP8, SDLK_KP2, SDLK_KP4, SDLK_KP6, SDLK_KP_ENTER
  }
};

static struct {
  unsigned int a;
  unsigned int b;
  unsigned int c;
  unsigned int start;
  unsigned int left;
  unsigned int right;
  unsigned int up;
  unsigned int down;
} ui_joy[2] = {
  { 0, 1, 2, 3, 4, 5, 6, 7 },
  { 0, 1, 2, 3, 4, 5, 6, 7 },
};

typedef enum {
    DEINTERLACE_BOB, DEINTERLACE_WEAVE, DEINTERLACE_WEAVEFILTER
} t_interlace;

/*** static variables ***/

static SDL_Surface *screen = NULL;      /* SDL screen */
static float scale = 1.0;
static int fullscreen = 0;

static char ui_region_lock = 0;          /* lock region at startup -Trilkk */

static const char *ui_initload = NULL;  /* filename to load on init */
static int ui_arcade_mode = 0;          /* play ROM at start */
static char *ui_configfile = NULL;      /* configuration file */
static uint8 ui_plotfield = 0;  /* flag indicating plotting this field */
static uint8 ui_vdpsimple = 0;  /* 0=raster, 1=cell based plotter */
static int ui_running = 0;      /* running a game */
static int ui_was_paused = 0;   /* used for frame skip/delay */
static uint8 ui_frameskip = 0;  /* 0 for dynamic */
static uint8 ui_actualskip = 0; /* the last skip we did (1..) */
static uint8 ui_statusbar = 1;  /* status-bar? */
static uint8 ui_screen[3][4 * HMAXSIZE * VMAXSIZE];     /* screen buffers */
static uint8 *ui_screen0;       /* pointer to screen block for bank 0 */
static uint8 *ui_screen1;       /* pointer to screen block for bank 1 */
static uint8 *ui_newscreen;     /* pointer to new screen block */
static int ui_whichbank;        /* current viewed screen */
static int ui_locksurface;      /* lock SDL surface? */
static unsigned int ui_hborder = HBORDER_DEFAULT; /* horizontal border */
static unsigned int ui_vborder = VBORDER_DEFAULT; /* vertical border */
static unsigned int hborder = HBORDER_DEFAULT; /* actual value */
static unsigned int vborder = VBORDER_DEFAULT; /* actual value */
static int ui_musicfile = -1;   /* fd of output file for GYM/GNM logging */
static int ui_joysticks = 0;    /* number of joysticks */
static SDL_Joystick *js_handle[2] = { NULL, NULL };     /* SDL handles */
static t_avi *ui_avi = NULL;    /* Current AVI writer if applicable */
static uint8 *ui_avivideo;      /* video buffer */
static uint8 *ui_aviaudio;      /* audio buffer */
static int sound_active;

#define ui_sdl_swapbanks()	\
do {				\
  ui_whichbank ^= 1;		\
} while (0)

/* If non-zero, ui_sdl_sizechange refuses to change the mode. */
static int screenmode_lock = 0;

t_interlace ui_interlace = DEINTERLACE_WEAVEFILTER;

/*** Forward references ***/

static void ui_usage(void);
static void ui_newframe(void);
static void ui_rendertoscreen(void);
static int ui_topbit(unsigned long int bits);
static void ui_simpleplot(void);
static void ui_sdl_events(void);
static void ui_sdl_play(void);
static void ui_sdl_newoptions(void);
static void ui_sdl_redraw(void);
static void ui_sdl_quit(void);
static void ui_key_info(void);

/*** Program entry point ***/

int ui_init(int argc, const char *argv[])
{
  int ch;
  struct passwd *passwd;
  struct stat statbuf;
  int i;
  const char *name;

  fprintf(stderr, "Generator is (c) James Ponder 1997-2003, all rights "
          "reserved. v" VERSION "\n\n");

  while (-1 != (ch = getopt(argc, deconstify_void_ptr(argv), "?adc:r:w:"))) {
    switch (ch) {
    case 'a':                  /* arcade mode; play ROM at once */
      ui_arcade_mode = 1;
      fullscreen = 1;
      break;
    case 'd':                  /* turn on debug mode */
      gen_debugmode = 1;
      break;
    case 'c':                  /* configuration file */
      ui_configfile = optarg;
      break;
    case 'w':                  /* saved game work dir */
      chdir(optarg);            /* for the moment this will do */
      break;
    /*
     * Check for region locking. In this case, lock region at start and do
     * not allow for changes even in the config file. For usage when guesses
     * go wrong and modifying the config would be tedious. -Trilkk
     */
    case 'r':
      if (!strcasecmp(optarg, "europe"))
        ui_region_lock = 'E';
      else if (!strcasecmp(optarg, "japan"))
        ui_region_lock = 'J';
      else if (!strcasecmp(optarg, "usa"))
        ui_region_lock = 'U';
      else
        ui_usage();
      break;
    case '?':
    default:
      ui_usage();
    }
  }
  argc -= optind;
  argv += optind;

  if (argc != 1)
    ui_usage();

  if (argc > 0) {
    ui_initload = argv[0];
    argc--;
    argv++;
  }

  ui_key_info();
  
  if (ui_configfile == NULL) {
    if ((passwd = getpwuid(getuid())) == NULL) {
      fprintf(stderr, "Who are you? (getpwent failed)\n");
      exit(1);
    }

    {
      static const char genrc[] = "/.genrc";
      size_t size = strlen(passwd->pw_dir) + sizeof genrc;
      char *p;
    
      p = ui_configfile = malloc(size);
      p = append_string(p, &size, passwd->pw_dir);
      p = append_string(p, &size, genrc);
    }
  }
  if (stat(ui_configfile, &statbuf) != 0) {
    fprintf(stderr, "No configuration file found, using defaults.\n");
  } else {
    if (gtkopts_load(ui_configfile) != 0)
      fprintf(stderr, "Error loading configuration file, using defaults.\n");
  }

  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0) {
    fprintf(stderr, "Couldn't initialise SDL: %s\n", SDL_GetError());
    return -1;
  }
  SDL_ShowCursor(SDL_DISABLE);
  ui_joysticks = SDL_NumJoysticks();
  /* Print information about the joysticks */
  fprintf(stderr, "%d joysticks detected\n", ui_joysticks);
  for (i = 0; i < ui_joysticks; i++) {
    name = SDL_JoystickName(i);
    fprintf(stderr, "Joystick %d: %s\n", i,
                    name ? name : "Unknown Joystick");
    if (i > (signed) (sizeof js_handle / sizeof js_handle[0]))
      continue;

    js_handle[i] = SDL_JoystickOpen(i);
    if (!js_handle[i])
      fprintf(stderr, "Failed to open joystick %d: %s\n", i, SDL_GetError());
  }
  if (js_handle[0] || js_handle[1])
    SDL_JoystickEventState(SDL_ENABLE);


  ui_sdl_newoptions();

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
  ui_whichbank = 0;             /* viewing 0 */

  opengl_setup(HSIZE, VSIZE, scale);

  return 0;
}

static void ui_usage(void)
{
  fprintf(stderr, "generator [options] <rom>\n\n");
  fprintf(stderr, "  -a                  arcade mode: start in fullscreen\n");
  fprintf(stderr, "  -d                  debug mode\n");
  fprintf(stderr, "  -r <region>         set region to europe, japan or usa\n");
  fprintf(stderr, "  -w <work dir>       set work directory\n");
  fprintf(stderr, "  -c <config file>    use alternative config file\n\n");
  fprintf(stderr, "  ROM types supported: .rom or .smd interleaved "
          "(autodetected)\n");
  exit(EXIT_FAILURE);
}

static void ui_key_info(void)
{
  fprintf(stderr,
      "Key assignments:\n"
      "===================================\n"
      "A: A\n"
      "B: S\n"
      "C: D\n"
      "UP: <Arrow Up>\n"
      "DOWN: <Arrow Down>\n"
      "LEFT: <Arrow Left>\n"
      "RIGHT: <Arrow Right>\n"
      "Play/Pause: <Pause> or <Control> + P\n"
      "Soft Reset: <Control> + R\n"
      "Hard Reset: <Control> + <Shift> + R\n"
      "Quit: <Control> + Q\n"
      "Fullscreen toggle: <Control> + F\n"
      "Window 100%%: <Control> + 1\n"
      "Window 200%%: <Control> + 2\n"
      "\n"
   );
}

void ui_final(void)
{
  SDL_Quit();
  exit(EXIT_SUCCESS);
}

int ui_loop(void)
{
  if (ui_initload) {
    char *error;
    
    error = gen_loadimage(ui_initload);
    if (error) {
      fprintf(stderr, "%s\n", error);
      ui_sdl_quit();
    } else if (ui_arcade_mode) {
      ui_sdl_play();
    }
  } else {
    gen_loadmemrom(initcart, initcart_len);
  }
 
  while (!gen_quit) {
    if (ui_running) {
      ui_newframe();
      event_doframe();
    } else {
      SDL_Delay(200);
    }
    ui_sdl_events();
  }
  ui_final();
  return 0;
}

void ui_newframe(void)
{
  static int vmode = -1;
  static int pal = -1;
  static int skipcount = 0;
  static char frameplots[60];   /* 60 for NTSC, 50 for PAL */
  static unsigned int frameplots_i = 0;
  unsigned int i;
  int fps;
  static int lasttype = -1;

  if (frameplots_i > vdp_framerate)
    frameplots_i = 0;
  if (((vdp_reg[12] >> 1) & 3) && vdp_oddframe) {
    /* interlace mode, and we're about to do an odd field - we always leave
       ui_plotfield alone so we do fields in pairs, this stablises the
       display, reduces blurring */
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
  lasttype = vdp_oddframe;
  /* check for ROM or user changing the vertical size */
  if (vmode == (int)(vdp_reg[1] & (1 << 3)) || pal != (int)vdp_pal) {
    vdp_setupvideo();
    vmode = vdp_reg[1] & (1 << 3);
    pal = vdp_pal;
  }
  /* count the frames we've plotted in the last vdp_framerate real frames */
  fps = 0;
  for (i = 0; i < vdp_framerate; i++) {
    if (frameplots[i])
      fps++;
  }
  frameplots[frameplots_i++] = 1;
}

/* ui_line is called for all vdp_totlines but is offset so that line=0
   is the first line, line=(vdp_totlines-vdp_visstartline)-1 is the
   last */

void ui_line(int line)
{
  static Uint64 gfx_data[320 / sizeof(Uint64)];
  static uint8 *gfx = (uint8 *) &gfx_data;
  unsigned int width = (vdp_reg[12] & 1) ? 320 : 256;
  unsigned int offset = hborder + ((vdp_reg[12] & 1) ? 0 : 32);
  uint8 bg;
  uint32 bgpix;
  uint32 bgpix1, bgpix2;
  union {
    void   *v;
    uint8  *u8;
    uint16 *u16;
    uint32 *u32;
    Uint64 *u64;
  } location;
  unsigned int i;

  if (!ui_plotfield)
    return;
  if (ui_vdpsimple) {
    if (line == (int)(vdp_vislines >> 1))
      /* if we're in simple cell-based mode, plot when half way down screen */
      ui_simpleplot();
    return;
  }
  if (line < -(int)vborder || line >= (int)(vdp_vislines + vborder))
    return;

  location.v = ui_newscreen +
    (line + vborder) * HMAXSIZE * screen->format->BytesPerPixel;
  bg = vdp_reg[7] & 63;
  uiplot_checkpalcache(0);

  bgpix = uiplot_palcache[bg];
  bgpix1 = bgpix2 = 0; /* Dumb compiler */

  if (line < 0 || line >= (int)vdp_vislines) {
    /* in border */

    switch (screen->format->BytesPerPixel) {
    case 2:
      for (i = 0; i < HSIZE; i++)
        location.u16[i] = bgpix;
      break;
    case 4:
      for (i = 0; i < HSIZE; i++)
        location.u32[i] = bgpix;
      break;
    }

    return;
  }
  /* normal line */
  switch ((vdp_reg[12] >> 1) & 3) {
  case 0:                      /* normal */
  case 1:                      /* interlace simply doubled up */
  case 2:                      /* invalid */
    vdp_renderline(line, gfx, 0);
    break;
  case 3:                      /* interlace with double resolution */
    vdp_renderline(line, gfx, vdp_oddframe);
    break;
  }

  switch (screen->format->BytesPerPixel) {
  case 2:
    uiplot_convertdata16(gfx, &location.u16[offset], width);
    for (i = 0; i < offset; i++) {
      location.u16[i] = bgpix;
      location.u16[i + offset + width] = bgpix;
    }
    break;
  case 4:
    uiplot_convertdata32(gfx, &location.u32[offset], width);
    for (i = 0; i < offset; i++) {
      location.u32[i] = bgpix;
      location.u32[i + offset + width] = bgpix;
    }
    break;
  }
}

static void ui_simpleplot(void)
{
  static Uint64 gfx_data[(320 + 16) * (240 + 16) / sizeof(Uint64)];
  static uint8 *gfx = (uint8 *) &gfx_data;
  unsigned int width = (vdp_reg[12] & 1) ? 320 : 256;
  unsigned int offset = hborder + ((vdp_reg[12] & 1) ? 0 : 32);
  unsigned int line, i;
  union {
    void   *v;
    uint8  *u8;
    uint16 *u16;
    uint32 *u32;
  } location, location2;
  uint8 bg;
  uint32 bgpix;
  uint32 bgpix1 = 0, bgpix2 = 0;

  /* cell mode - entire frame done here */
  bg = (vdp_reg[7] & 63);
  uiplot_checkpalcache(0);
  vdp_renderframe(gfx + (8 * (320 + 16)) + 8, 320 + 16);    /* plot frame */
  location.v = ui_newscreen +
      vborder * HMAXSIZE * screen->format->BytesPerPixel;

  bgpix = uiplot_palcache[bg];
  switch (screen->format->BytesPerPixel) {
  case 2:
    for (line = 0; line < vdp_vislines; line++) {
      for (i = 0; i < offset; i++) {
        location.u16[i] = bgpix;
        location.u16[width + offset + i] = bgpix;
      }
      uiplot_convertdata16(gfx + 8 + (line + 8) * (320 + 16),
	location.u16 + offset, width);
      location.u16 += HMAXSIZE;
    }
    break;
  case 4:
    for (line = 0; line < vdp_vislines; line++) {
      for (i = 0; i < offset; i++) {
        location.u32[i] = bgpix;
	location.u32[width + offset + i] = bgpix;
      }
      uiplot_convertdata32(gfx + 8 + (line + 8) * (320 + 16),
	location.u32 + offset, width + offset);
      location.u32 += HMAXSIZE;
    }
    break;
  }

  location.v = ui_newscreen;
  location2.v = ui_newscreen + (vborder + vdp_vislines) *
      HMAXSIZE * screen->format->BytesPerPixel;
  switch (screen->format->BytesPerPixel) {
  case 2:
    for (line = 0; line < vborder; line++) {
      for (i = 0; i < HSIZE; i++) {
        location.u16[i] = bgpix;
        location2.u16[i] = bgpix;
      }
      location.u16 += HMAXSIZE;
      location2.u16 += HMAXSIZE;
    }
    break;
  case 4:
    for (line = 0; line < vborder; line++) {
      for (i = 0; i < HSIZE; i++) {
        location.u32[i] = bgpix;
        location2.u32[i] = bgpix;
      }
      location.u32 += HMAXSIZE;
      location2.u32 += HMAXSIZE;
    }
    break;
  }
}

static void ui_makeavivideo(void)
{
  unsigned int line;
  SDL_PixelFormat *f = screen->format;
  uint8 *linep = ui_newscreen;
  uint8 *video;
  uint32 c;
  unsigned int x;

  video = ui_avivideo;
  switch (screen->format->BytesPerPixel) {
  case 2:
    for (line = 0; line < ui_avi->info.height; line++) {
      for (x = 0; x < ui_avi->info.width; x++) {
        c = ((uint16 *)linep)[x];
        *video++ = ((c & f->Rmask) >> f->Rshift) << f->Rloss;
        *video++ = ((c & f->Gmask) >> f->Gshift) << f->Gloss;
        *video++ = ((c & f->Bmask) >> f->Bshift) << f->Bloss;
      }
      linep+= (HMAXSIZE * 2);
    }
    break;
  case 4:
    for (line = 0; line < ui_avi->info.height; line++) {
      for (x = 0; x < ui_avi->info.width; x++) {
        c = ((uint32 *)linep)[x];
        *video++ = ((c & f->Rmask) >> f->Rshift) << f->Rloss;
        *video++ = ((c & f->Gmask) >> f->Gshift) << f->Gloss;
        *video++ = ((c & f->Bmask) >> f->Bshift) << f->Bloss;
      }
      linep+= (HMAXSIZE * 4);
    }
    break;
  }
  avi_video(ui_avi, ui_avivideo);
}

static void ui_makeaviaudio(void)
{
  unsigned int i;

  /* 22050 doesn't divide nicely into 60 fields - that means 367.50 samples
     per field.  So we need to compensate by adding in a byte every now and
     again */

  /* audio buffer gensound_soundbuf is in local endian - avi is little */
  for (i = 0; i < sound_sampsperfield; i++) {
    ((uint16 *)ui_aviaudio)[i * 2] = LOCENDIAN16L(sound_soundbuf[0][i]);
    ((uint16 *)ui_aviaudio)[i * 2 + 1] = LOCENDIAN16L(sound_soundbuf[1][i]);
  }
  /* duplicate last byte */
  ((uint16 *)ui_aviaudio)[i * 2] = LOCENDIAN16L(sound_soundbuf[0][i - 1]);
  ((uint16 *)ui_aviaudio)[i * 2 + 1] = LOCENDIAN16L(sound_soundbuf[1][i - 1]);

  /* get partial bytes per second we should be outputting in 1000ths */
  ui_avi->error+= (ui_avi->info.sampspersec * 1000 / vdp_framerate) % 1000;
  if (ui_avi->error > 1000) {
    ui_avi->error-= 1000;
    avi_audio(ui_avi, ui_aviaudio, sound_sampsperfield + 1);
  } else {
    avi_audio(ui_avi, ui_aviaudio, sound_sampsperfield);
  }
}

/*** ui_endfield - end of field reached ***/

void ui_endfield(void)
{
  static int counter = 0, frames = 0, waitstates;
  static struct timeval tv0;
  struct timeval tv;
  long dt;
  int max_wait;

  gettimeofday(&tv, NULL);
  if (ui_avi)
    ui_makeaviaudio();

  if (ui_plotfield) {
    ui_rendertoscreen();        /* plot ui_newscreen to screen */
    ui_sdl_swapbanks();
    if (ui_avi)
      ui_makeavivideo();
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

  if (ui_was_paused) {
    ui_was_paused = 0;
    frames = 0;
  }

  dt = (tv.tv_sec - tv0.tv_sec) * 1000000 + tv.tv_usec - tv0.tv_usec;
  tv0 = tv;
  max_wait = 1000000 / vdp_framerate;
  if (sound_active)
	max_wait /= 2;
  waitstates = dt >= max_wait ? 1 : max_wait - dt;

  if (!sound_active || sound_feedback >= 0) {
    struct timespec ts = { 0, waitstates };

    nanosleep(&ts, NULL);
  }
}


void ui_rendertoscreen(void)
{
  opengl_render32(ui_newscreen, HSIZE, VSIZE, HMAXSIZE, scale);
  SDL_GL_SwapBuffers();
}

/*** logging functions ***/

/* logging is done this way because this was the best I could come up with
   whilst battling with macros that can only take fixed numbers of arguments */

#define LOG_FUNC(name,level,txt) \
void ui_log_ ## name (const char *text, ...) \
{ \
  va_list ap; \
  if (gen_loglevel >= level) { \
    fprintf(stderr, "[%s] ", txt); \
    va_start(ap, text); \
    vfprintf(stderr, text, ap); \
    va_end(ap); \
    fputc('\n', stderr); \
  } \
}

/* *INDENT-OFF* */

LOG_FUNC(debug3,   7, "DEBG ");
LOG_FUNC(debug2,   6, "DEBG ");
LOG_FUNC(debug1,   5, "DEBG ");
LOG_FUNC(user,     4, "USER ");
LOG_FUNC(verbose,  3, "---- ");
LOG_FUNC(normal,   2, "---- ");
LOG_FUNC(critical, 1, "CRIT ");
LOG_FUNC(request,  0, "---- ");  /* this generates a warning, such is life */

/* *INDENT-ON* */

/*** ui_err - log error message and quit ***/

void ui_err(const char *fmt, ...)
{
  va_list ap;

  fputs("ERROR: ", stderr);

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  putc('\n', stderr);
  ui_final();
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

/*** ui_setinfo - there is new cart information ***/

char *ui_setinfo(t_cartinfo *cartinfo)
{
  (void)cartinfo;
  return NULL;
}

static void ui_sdl_quit(void)
{
  gen_quit = 1;
  ui_final();
}

void ui_sdl_closeconsole(void)
{
  return;
}

static void ui_sdl_play(void)
{
  if (ui_running) {
    fprintf(stderr, "Generator is already playing a ROM\n");
    return;
  }
  if (cpu68k_rom == NULL || cpu68k_romlen == 0) {
    fprintf(stderr, "You must load a ROM into Generator\n");
    return;
  }
  /* start running the game */
  ui_running = 1;
  if (sound_on)
    sound_active = !sound_start();
  else
    sound_active = 0;
}

void ui_sdl_pause(void)
{
  if (!ui_running) {
    fprintf(stderr, "Generator is not playing a ROM\n");
    return;
  }
  if (sound_on)
    sound_stop();
  ui_running = 0;
  ui_was_paused = 1;
}

void ui_sdl_softreset(void)
{
  gen_softreset();
}

void ui_sdl_hardreset(void)
{
  gen_reset();
}

int ui_create_overlay(void)
{
  return -1;
}

void ui_sdl_reinit_video(void)
{
    if (screen) {
      /* SDL doesn't want this to be freed */
      screen = NULL;
    }
    if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
      SDL_QuitSubSystem(SDL_INIT_VIDEO);
    if (SDL_Init(SDL_INIT_VIDEO))
      ui_err("Couldn't reinitialise SDL: %s\n", SDL_GetError());
    else
      SDL_ShowCursor(SDL_DISABLE);
}

/* set main window size from current parameters */

void ui_sdl_sizechange(void)
{
  if (screenmode_lock)
    return;

  unsigned flags = fullscreen ? SDL_FULLSCREEN : 0;

  screen = SDL_SetVideoMode(HSIZE*scale, VSIZE*scale, 0, flags | SDL_OPENGL);
  ui_arcade_mode = 0;
  hborder = ui_hborder;
  vborder = ui_vborder;

  ui_locksurface = SDL_MUSTLOCK(screen);
  uiplot_setshifts(ui_topbit(screen->format->Rmask) - 4,
                   ui_topbit(screen->format->Gmask) - 4,
                   ui_topbit(screen->format->Bmask) - 4);

  /* Gimmick: Redraw overlay when paused. Otherwise, only a rectangle
   * with the colorkey is visible. */
  if (!ui_running && ui_screen0 && ui_screen1 && ui_newscreen)
    ui_rendertoscreen();
}

static void toggle_fullscreen(void) {
  fullscreen = !fullscreen;
  ui_sdl_sizechange();
}

static void ui_sdl_newoptions(void)
{
  const char *v;
  int i;
  unsigned int old_sound_on = sound_on;
  unsigned int old_sound_minfields = sound_minfields;
  unsigned int old_sound_maxfields = sound_maxfields;
  unsigned int old_ui_hborder = ui_hborder;
  unsigned int old_ui_vborder = ui_vborder;
  unsigned int old_ui_statusbar = ui_statusbar;

  if (ui_arcade_mode) {
    fullscreen = 1;
  } else {
    v = gtkopts_getvalue("view");
    if (!strcasecmp(v, "fullscreen")) {
      fullscreen=1;
    } else {
      i = atoi(v);
      scale = i/ 100.0;
    }
  }

  /*
   * Now if we have locked the region, let's set it here instead of loading
   * it from the configuration file.
   */

  switch (ui_region_lock) {
  case 'E':
    gen_autodetect = 0;
    vdp_overseas = 1;
    vdp_pal = 1;
    break;
  case 'J':
    gen_autodetect = 0;
    vdp_overseas = 0;
    vdp_pal = 0;
    break;
  case 'U':
    gen_autodetect = 0;
    vdp_overseas = 1;
    vdp_pal = 0;
    break;
  default:
    v = gtkopts_getvalue("autodetect");
    gen_autodetect = !strcasecmp(v, "on");

    if (!gen_autodetect) {
      /* No decent region set, let's load from the config file. */
      v = gtkopts_getvalue("region");
      vdp_overseas = !strcasecmp(v, "overseas");

      v = gtkopts_getvalue("videostd");
      vdp_pal = !strcasecmp(v, "pal");
    }
  }

  v = gtkopts_getvalue("plotter");
  ui_vdpsimple = !strcasecmp(v, "cell");

  v = gtkopts_getvalue("interlace");
  if (!strcasecmp(v, "weave"))
    ui_interlace = DEINTERLACE_WEAVE;
  else if (!strcasecmp(v, "weave-filter"))
    ui_interlace = DEINTERLACE_WEAVEFILTER;
  else
    ui_interlace = DEINTERLACE_BOB;

  v = gtkopts_getvalue("frameskip");
  if (!strcasecmp(v, "auto")) {
    ui_frameskip = 0;
  } else {
    ui_frameskip = atoi(v);
  }

  v = gtkopts_getvalue("hborder");
  ui_hborder = atoi(v);
  if (ui_hborder > HBORDER_MAX)
    ui_hborder = HBORDER_MAX;

  v = gtkopts_getvalue("vborder");
  ui_vborder = atoi(v);
  if (ui_vborder > VBORDER_MAX)
    ui_vborder = VBORDER_MAX;

  if (!fullscreen) {
    hborder = ui_hborder;
    vborder = ui_vborder;
  } else {
    hborder = 0;
    vborder = MIN(ui_vborder, 8);
  }

  v = gtkopts_getvalue("z80");
  cpuz80_on = !strcasecmp(v, "on");

  v = gtkopts_getvalue("sound");
  sound_on = !strcasecmp(v, "on");

  v = gtkopts_getvalue("fm");
  sound_fm = !strcasecmp(v, "on");

  v = gtkopts_getvalue("psg");
  sound_psg = !strcasecmp(v, "on");

  v = gtkopts_getvalue("sound_minfields");
  sound_minfields = atoi(v);

  v = gtkopts_getvalue("sound_maxfields");
  sound_maxfields = atoi(v);

  v = gtkopts_getvalue("lowpassfilter");
  sound_filter = atoi(v);

  v = gtkopts_getvalue("samplerate");
  sound_speed = atoi(v);
  if (sound_speed < 8000 || sound_speed > SOUND_MAXRATE)
    sound_speed = SOUND_SAMPLERATE;

  v = gtkopts_getvalue("loglevel");
  gen_loglevel = atoi(v);

  v = gtkopts_getvalue("statusbar");
  ui_statusbar = !strcasecmp(v, "on");

  ui_sdl_sizechange();

  if (sound_minfields != old_sound_minfields ||
      sound_maxfields != old_sound_maxfields ||
      sound_on != old_sound_on) {
    sound_stop();
    if (sound_on)
      sound_active = !sound_start();
    else
      sound_active = 0;
  }
  
  ui_joy[0].a = atoi(gtkopts_getvalue("joy1_a"));
  ui_joy[0].b = atoi(gtkopts_getvalue("joy1_b"));
  ui_joy[0].c = atoi(gtkopts_getvalue("joy1_c"));
  ui_joy[0].start = atoi(gtkopts_getvalue("joy1_start"));
  ui_joy[0].left = atoi(gtkopts_getvalue("joy1_left"));
  ui_joy[0].right = atoi(gtkopts_getvalue("joy1_right"));
  ui_joy[0].up = atoi(gtkopts_getvalue("joy1_up"));
  ui_joy[0].down = atoi(gtkopts_getvalue("joy1_down"));
  ui_joy[1].a = atoi(gtkopts_getvalue("joy2_a"));
  ui_joy[1].b = atoi(gtkopts_getvalue("joy2_b"));
  ui_joy[1].c = atoi(gtkopts_getvalue("joy2_c"));
  ui_joy[1].start = atoi(gtkopts_getvalue("joy2_start"));
  ui_joy[1].left = atoi(gtkopts_getvalue("joy2_left"));
  ui_joy[1].right = atoi(gtkopts_getvalue("joy2_right"));
  ui_joy[1].up = atoi(gtkopts_getvalue("joy2_up"));
  ui_joy[1].down = atoi(gtkopts_getvalue("joy2_down"));
}

void ui_sdl_applyoptions(void)
{
  ui_sdl_newoptions();
}

void ui_sdl_saveoptions(void)
{
  if (gtkopts_save(ui_configfile) != 0) {
    fprintf(stderr, "Failed to save configuration to '%s': %s\n",
             ui_configfile, strerror(errno));
    return;
  }
  ui_sdl_newoptions();
}

static void ui_sdl_redraw(void)
{
// SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
}

static void ui_sdl_events(void)
{
  SDL_Event event;
  unsigned int key = 0, c;

#define	LEFT(event)	((event.jaxis.value < -16384) ? 1 : 0)
#define	RIGHT(event)	((event.jaxis.value > 16384) ? 1 : 0)
#define	UP(event)	((event.jaxis.value < -16384) ? 1 : 0)
#define	DOWN(event)	((event.jaxis.value > 16384) ? 1 : 0)
#define	PRESS(event)	((event.type == SDL_JOYBUTTONDOWN) ? 1 : 0)
#define	KPRESS(event)	((event.type == SDL_KEYDOWN) ? 1 : 0)

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_JOYAXISMOTION:
        if (event.jaxis.which > 1)
          break;
        switch (event.jaxis.axis) {
          case 0:	/* left & right */
            mem68k_cont[event.jaxis.which].left = LEFT(event);
            mem68k_cont[event.jaxis.which].right = RIGHT(event);
            break;
          case 1: /* up & down */
            mem68k_cont[event.jaxis.which].up = UP(event);
            mem68k_cont[event.jaxis.which].down = DOWN(event);
            break;
          default:
            break;
        }
        break;
      case SDL_JOYBUTTONDOWN:
      case SDL_JOYBUTTONUP:
        if (event.jbutton.which < 2) {
          unsigned j = event.jbutton.which, b = event.jbutton.button, a = event.jaxis.which;
          
          if (b == ui_joy[j].a) {
            mem68k_cont[j].a = PRESS(event);
          } else if (b == ui_joy[j].b) {
            mem68k_cont[event.jbutton.which].b = PRESS(event);
          } else if (b == ui_joy[j].c) {
            mem68k_cont[j].c = PRESS(event);
          } else if (b == ui_joy[j].start) {
            mem68k_cont[j].start = PRESS(event);
          } else if (b == ui_joy[j].left) {
            mem68k_cont[a].left = PRESS(event);
          } else if (b == ui_joy[j].right) {
            mem68k_cont[a].right = PRESS(event);
          } else if (b == ui_joy[j].up) {
            mem68k_cont[a].up = PRESS(event);
          } else if (b == ui_joy[j].down) {
            mem68k_cont[a].down = PRESS(event);
          }
        }
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        key = event.key.keysym.sym;

        /* FALL THROUGH because special keys should not be overridable */ 

        switch (event.key.keysym.sym) {
          /* Special keys */
          case SDLK_q:
            if (event.key.keysym.mod & KMOD_CTRL)
              ui_sdl_quit();
            break;
          case SDLK_p:
            if ((event.key.keysym.mod & KMOD_CTRL) == 0)
              break;
            /* FALL THROUGH */
          case SDLK_PAUSE:
          if (KPRESS(event)) {
            if (ui_running)
              ui_sdl_pause();
            else
              ui_sdl_play();
          }
          break;
        case SDLK_f:
          if (KPRESS(event) && event.key.keysym.mod & KMOD_CTRL) {
            toggle_fullscreen();
          }
          break;
        case SDLK_1:
        case SDLK_2:
          if (KPRESS(event) && event.key.keysym.mod & KMOD_CTRL) {
            scale = event.key.keysym.sym == SDLK_2 ? 2.0 : 1.0;
            ui_sdl_sizechange();
          }
          break;
        case SDLK_r:
          if (event.key.keysym.mod & KMOD_CTRL) {
            if (event.key.keysym.mod & KMOD_SHIFT)
              ui_sdl_hardreset();
            else
              ui_sdl_softreset();
          }
          break;
        default: ;
                 /* Ignore */
      }

      for (c = 0; c < 2; c++) {
        if (key == ui_cont[c].a) {
          mem68k_cont[c].a = KPRESS(event);
        } else if (key == ui_cont[c].b) {
          mem68k_cont[c].b = KPRESS(event);
        } else if (key == ui_cont[c].c) {
          mem68k_cont[c].c = KPRESS(event);
        } else if (key == ui_cont[c].left) {
          mem68k_cont[c].left = KPRESS(event);
        } else if (key == ui_cont[c].right) {
          mem68k_cont[c].right = KPRESS(event);
        } else if (key == ui_cont[c].up) {
          mem68k_cont[c].up = KPRESS(event);
        } else if (key == ui_cont[c].down) {
          mem68k_cont[c].down = KPRESS(event);
        } else if (key == ui_cont[c].start) {
          mem68k_cont[c].start = KPRESS(event);
        }
      }
      break;

    case SDL_ACTIVEEVENT:
      if (event.active.state == SDL_APPMOUSEFOCUS && event.active.gain)
        SDL_ShowCursor(SDL_DISABLE);
      break;
#if 0
    case SDL_VIDEORESIZE: {
                            int w = MAX(event.resize.w, 100);
                            int h = MAX(event.resize.h, 100);

                            if ((w != screen->w || h != screen->h) &&
                                SDL_VideoModeOK(w, h, screen->format->BitsPerPixel, SDL_RESIZABLE))
                            {
                              screen = SDL_SetVideoMode(w, h, 0, SDL_RESIZABLE | SDL_OPENGL);
                            }
                            break;
                          }
#endif
    case SDL_QUIT:
                          ui_sdl_quit();
                          break;

    case SDL_SYSWMEVENT:
    case SDL_VIDEOEXPOSE:
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_JOYBALLMOTION:
    case SDL_JOYHATMOTION:
                          /* Just ignore */
                          break;
    default: ;
             LOG_NORMAL(("Unknown event type"));
    }
  }
}

void ui_sdl_mainenter(void)
{
  /* clear out current state */
  memset(mem68k_cont, 0, sizeof(mem68k_cont));
}

void ui_sdl_mainleave(void)
{
  return;
}

void ui_musiclog(uint8 *data, unsigned int length)
{
  if (-1 != ui_musicfile)
    write(ui_musicfile, data, length);
}

void ui_sdl_closemusiclog(void)
{
  if (ui_musicfile == -1) {
    fprintf(stderr, "There is no log to close");
    return;
  }
  close(ui_musicfile);
  ui_musicfile = -1;
  gen_musiclog = 0;
}

/* vi: set ts=2 sw=2 et cindent: */
