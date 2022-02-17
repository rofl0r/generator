/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* gtk user interface */

#include "generator.h"

#include <pwd.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include "SDL.h"

#include "glade/interface.h"
#include "glade/support.h"

#include "ui.h"
#include "ui-gtk.h"
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

#define HBORDER_MAX 32
#define HBORDER_DEFAULT 8

#define VBORDER_MAX 32
#define VBORDER_DEFAULT 8

#define HMAXSIZE (320 + 2 * HBORDER_MAX)
#define VMAXSIZE (240 + 2 * VBORDER_MAX)

#define HSIZE (320 + 2 * hborder)
#define VSIZE ((vdp_vislines ? vdp_vislines : 224) + 2 * vborder)

#define DISSWIN_MAX 64

/* Used if hardware supports *any* resolution or as preferred fullscreen
 * resolution if supported */
#define DEFAULT_FULLSCREEN_WIDTH 640
#define DEFAULT_FULLSCREEN_HEIGHT 480

typedef struct {
  unsigned int a;
  unsigned int b;
  unsigned int c;
  unsigned int up;
  unsigned int down;
  unsigned int left;
  unsigned int right;
  unsigned int start;
} t_gtkkeys;

typedef struct {
  unsigned int a;
  unsigned int b;
  unsigned int c;
  unsigned int start;
  unsigned int left;
  unsigned int right;
  unsigned int up;
  unsigned int down;
} t_joykeys;


const char *ui_gtk_keys[] = {
  "a", "b", "c", "start", "left", "right", "up", "down"
};

/*** static variables ***/

static SDL_Surface *screen = NULL;      /* SDL screen */
static SDL_Overlay *overlay = NULL;	/* SDL overlay */
static uint32 overlay_format;	        /* SDL overlay fourcc (or alias) */

static char ui_region_lock = 0;          /* lock region at startup -Trilkk */

static const char *ui_initload = NULL;  /* filename to load on init */
static int ui_arcade_mode = 0;          /* play ROM at start */
static char *ui_configfile = NULL;      /* configuration file */
static uint8 ui_plotfield = 0;  /* flag indicating plotting this field */
static uint8 ui_vdpsimple = 0;  /* 0=raster, 1=cell based plotter */
static int ui_running = 0;      /* running a game */
static int ui_was_paused = 0;   /* used for frame skip/delay */
static GtkWidget *ui_win_main;  /* main window */
static GtkWidget *ui_win_about; /* about window */
static GtkWidget *ui_win_console;       /* console window */
static GtkWidget *ui_win_opts;  /* options window */
static GtkWidget *ui_win_codes; /* game genie codes window */
static GtkWidget *ui_win_filesel;       /* file selection window */
static int ui_filesel_type;     /* selection type: 0=rom, 1=state */
static int ui_filesel_save;     /* 0=load, 1=save */
static GtkWidget *ui_gtk_dialog(const char *msg);
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
static int ui_query_response = -1; /* query response */
static t_gtkkeys ui_cont[2];    /* keyboard key codes */
static t_joykeys ui_joy[2];     /* joypad button assignments */
static int ui_musicfile = -1;   /* fd of output file for GYM/GNM logging */
static int ui_joysticks = 0;    /* number of joysticks */
static SDL_Joystick *js_handle[2] = { NULL, NULL };     /* SDL handles */
static t_aviinfo ui_aviinfo;    /* AVI information */
static t_avi *ui_avi = NULL;    /* Current AVI writer if applicable */
static uint8 *ui_avivideo;      /* video buffer */
static uint8 *ui_aviaudio;      /* audio buffer */
static GtkWidget *ui_disswins[DISSWIN_MAX];     /* disassembly windows */
static GdkFont *ui_dissfont;    /* disassembly window font */
static int sound_active;

#define ui_gtk_swapbanks()	\
do {				\
  ui_whichbank ^= 1;		\
} while (0)

typedef enum {
  SCREEN_100,
  SCREEN_200,
  SCREEN_FULL
} ui_screenmode_t;

static ui_screenmode_t ui_screenmode = SCREEN_100;

/* If non-zero, ui_gtk_sizechange refuses to change the mode.
 * Otherwise, ui_gtk_opts_menu() would cause undesired mode changes. */
static int screenmode_lock = 0;


static const struct {
  uint32 fourcc;
  uint32 alias;
} overlay_formats[] = {
  { SDL_UYVY_OVERLAY, SDL_UYVY_OVERLAY },
  { SDL_YUY2_OVERLAY, SDL_YUY2_OVERLAY },
  { SDL_YVYU_OVERLAY, SDL_YVYU_OVERLAY },
  { 0x56595559,       SDL_YUY2_OVERLAY },
  { 0x564E5559,       SDL_YUY2_OVERLAY },
  { 0x564E5955,       SDL_UYVY_OVERLAY },
  { 0x32323459,       SDL_UYVY_OVERLAY }
};

t_interlace ui_interlace = DEINTERLACE_WEAVEFILTER;

/*** Forward references ***/

static void ui_usage(void);
void ui_gtk_resize_win(GtkWidget *widget, GtkAllocation *allocation,
	gpointer user_data);
static void ui_gtk_filesel_ok(GtkFileSelection * obj, gpointer user_data);
static void ui_gtk_sizechange(ui_screenmode_t newmode);
static void ui_newframe(void);
static void ui_rendertoscreen(void);
static gint ui_gtk_configuremain(GtkWidget * widget, GdkEventConfigure * event);
static int ui_topbit(unsigned long int bits);
static void ui_gtk_opts_to_window(void);
static void ui_gtk_opts_from_window(void);
static int ui_gtk_query(const char *msg, int style);
static void ui_gtk_opts_to_menu(void);
static void ui_simpleplot(void);
static void ui_sdl_events(void);
static void ui_sdl_window_hack(int on);
static gboolean ui_handle_sdl_events(gpointer data);

/*** Program entry point ***/

int ui_init(int argc, const char *argv[])
{
  int ch;
  GtkWidget *button, *obj;
  struct passwd *passwd;
  struct stat statbuf;
  int i;
  const char *name;

  gtk_set_locale();
  gtk_init(&argc, deconstify_void_ptr(&argv));

  fprintf(stderr, "Generator is (c) James Ponder 1997-2003, all rights "
          "reserved. v" VERSION "\n\n");

  while (-1 != (ch = getopt(argc, deconstify_void_ptr(argv), "?adc:r:w:"))) {
    switch (ch) {
    case 'a':                  /* arcade mode; play ROM at once */
      ui_arcade_mode = 1;
      ui_screenmode = SCREEN_FULL;
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

  if (argc > 0) {
    ui_initload = argv[0];
    argc--;
    argv++;
  }

  if (argc != 0)
    ui_usage();

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

  /* create gtk windows */

  ui_win_main = create_mainwin();
  ui_win_about = create_about();
  ui_win_console = create_console();
  ui_win_opts = create_opts();
  ui_win_codes = create_codes();
  ui_win_filesel = gtk_file_selection_new("Select file...");
  button = GTK_FILE_SELECTION(ui_win_filesel)->ok_button;
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(ui_gtk_filesel_ok), NULL);
  button = GTK_FILE_SELECTION(ui_win_filesel)->cancel_button;
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_hide),
                            GTK_OBJECT(ui_win_filesel));
  /* load our font */
  ui_dissfont = gdk_font_load
      ("-*-fixed-medium-r-normal-*-*-*-*-*-*-80-iso8859-1");
  gdk_font_ref(ui_dissfont);
 
  /* set codes window so that code list is reorderable */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  gtk_clist_set_reorderable(GTK_CLIST(obj), 1);

  /* open main window */

  gtk_widget_add_events(ui_win_main, GDK_KEY_RELEASE_MASK);
  gtk_window_set_title(GTK_WINDOW(ui_win_main), "Generator/gtk " VERSION);
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main), "debug"));
  if (!gen_debugmode)
    gtk_widget_hide(obj);
  gtk_widget_show_now(ui_win_main);
  gtk_signal_connect(GTK_OBJECT(ui_win_main), "configure_event",
                     GTK_SIGNAL_FUNC(ui_gtk_configuremain), 0);
  ui_sdl_window_hack(1);
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

  gtk_signal_connect(
    gtk_object_get_data(GTK_OBJECT(ui_win_main), "drawingarea_main"),
	  "size-allocate",
    GTK_SIGNAL_FUNC(ui_gtk_resize_win),
    NULL);

  ui_gtk_newoptions();
  ui_gtk_opts_to_menu();

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
  fprintf(stderr, "Please use the GTK menu to quit this program properly.\n");
  gtk_timeout_add(200, ui_handle_sdl_events, NULL);
  return 0;
}

static void ui_sdl_window_hack(int on)
{
  unsetenv("SDL_WINDOWID"); /* Make sure there is only a single instance */
  if (on) {
    char buf[64], *p = buf;
    size_t size = sizeof buf;
    GtkWidget *draw;

    memset(buf, 0, sizeof buf);
    draw = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main),
             "drawingarea_main"));
    p = append_string(p, &size, "SDL_WINDOWID=0x");
    p = append_ulong_hex(p, &size, GDK_WINDOW_XWINDOW(draw->window));
    putenv(buf);
  }
}

static void ui_usage(void)
{
  fprintf(stderr, "generator [options] [<rom>]\n\n");
  fprintf(stderr, "  -a                  arcade mode: start in fullscreen\n");
  fprintf(stderr, "  -d                  debug mode\n");
  fprintf(stderr, "  -r <region>         set region to europe, japan or usa\n");
  fprintf(stderr, "  -w <work dir>       set work directory\n");
  fprintf(stderr, "  -c <config file>    use alternative config file\n\n");
  fprintf(stderr, "  ROM types supported: .rom or .smd interleaved "
          "(autodetected)\n");
  exit(1);
}

void ui_final(void)
{
  gdk_font_unref(ui_dissfont);
  gdk_key_repeat_restore();
  SDL_Quit();
  gtk_exit(0);
}

int ui_loop(void)
{
  char *p;

  if (ui_initload) {
    p = gen_loadimage(ui_initload);
    if (p)
      ui_gtk_dialog(p);
    else if (ui_arcade_mode)
      ui_gtk_play();
  } else {
    gen_loadmemrom(initcart, initcart_len);
  }
 
  while (!gen_quit) {
    if (ui_running) {
      while (gtk_events_pending())
        gtk_main_iteration_do(0);
      ui_sdl_events();
      ui_newframe();
      event_doframe();
    } else {
      gtk_main();
    }
  }
  ui_final();
  return 0;
}

static int frames_per_second = 0;

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
  if (!ui_statusbar)
    return;

  {
    char buf[8];
    size_t size = sizeof buf;
    GtkWidget *entry_fps;
    
    frames_per_second = fps;
    (void) append_uint(buf, &size, fps);
    entry_fps = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main),
                                             "entry_fps"));
    gtk_entry_set_text(GTK_ENTRY(entry_fps), buf);
  }

  skipcount = 0;
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

  if (overlay) {
    bgpix = uiplot_palcache_yuv[bg];

    switch (overlay_format) {
    case SDL_YUY2_OVERLAY:
      YUY2_SINGLE(bgpix, bgpix1, bgpix2);
      uiplot_convertdata_yuy2(gfx, &location.u16[offset], width);
      break;
    case SDL_YVYU_OVERLAY:
      YVYU_SINGLE(bgpix, bgpix1, bgpix2);
      uiplot_convertdata_yvyu(gfx, &location.u16[offset], width);
      break;
    case SDL_UYVY_OVERLAY:
      UYVY_SINGLE(bgpix, bgpix1, bgpix2);
      uiplot_convertdata_uyvy(gfx, &location.u16[offset], width);
      break;
    default:
      bgpix1 = bgpix2 = 0;
      g_assert_not_reached();
    }
  } else {
    bgpix = uiplot_palcache[bg];
    bgpix1 = bgpix2 = 0; /* Dumb compiler */
  }

  if (line < 0 || line >= (int)vdp_vislines) {
    /* in border */

    if (overlay) {
      Uint32 bg, *p = location.u32, *q = p + HSIZE / 2;

      bg = bgpix1 | (bgpix2 << 16);

      while (p < q)
        *p++ = bg;

#if 0
        ((uint32 *)location)[i] = bgpix1 & 0;
        ((uint32 *)location)[i + 1] = bgpix2 | 0xffff;
#endif
   
    } else {

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

  if (overlay) {
    switch (overlay_format) {
    case SDL_YUY2_OVERLAY:
      uiplot_convertdata_yuy2(gfx, &location.u16[offset], width);
      break;
    case SDL_YVYU_OVERLAY:
      uiplot_convertdata_yvyu(gfx, &location.u16[offset], width);
      break;
    case SDL_UYVY_OVERLAY:
      uiplot_convertdata_uyvy(gfx, &location.u16[offset], width);
      break;
    default:
      bgpix1 = bgpix2 = 0;
      g_assert_not_reached();
    }
    for (i = 0; i < offset; i += 2) {
      location.u16[i] = bgpix1;
      location.u16[i + 1] = bgpix2;
      location.u16[i + offset + width] = bgpix1;
      location.u16[i + 1 + offset + width] = bgpix2;
    }
  } else {

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

  if (overlay) {
    void (*convert_func)(uint8 *, uint16 *, unsigned int);

    bgpix = uiplot_palcache_yuv[bg];

    switch (overlay_format) {
    case SDL_YUY2_OVERLAY:
      YUY2_SINGLE(bgpix, bgpix1, bgpix2);
      convert_func = uiplot_convertdata_yuy2;
      break;
    case SDL_YVYU_OVERLAY:
      YUY2_SINGLE(bgpix, bgpix1, bgpix2);
      convert_func = uiplot_convertdata_yuy2;
    case SDL_UYVY_OVERLAY:
      UYVY_SINGLE(bgpix, bgpix1, bgpix2);
      convert_func = uiplot_convertdata_uyvy;
      break;
    default:
      convert_func = NULL; 
      g_assert_not_reached();
    }

    for (line = 0; line < vdp_vislines; line++) {

      for (i = 0; i < offset; i+=2) {
        location.u16[i] = bgpix1;
        location.u16[i + 1] = bgpix2;
        location.u16[width + offset + i] = bgpix1;
        location.u16[width + offset + i + 1] = bgpix2;
      }

      convert_func(gfx + 8 + (line + 8) * (320 + 16),
	&location.u16[offset], width);
      location.u16 += HMAXSIZE;
    }

  } else {

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

  }

  location.v = ui_newscreen;

  if (overlay) {

    location2.v = ui_newscreen + (vborder + vdp_vislines) * HMAXSIZE * 2;
    for (line = 0; line < vborder; line++) {
      for (i = 0; i < HSIZE; i += 2) {
        location.u16[i] = bgpix1;
        location.u16[i + 1] = bgpix2;
        location2.u16[i] = bgpix1;
        location2.u16[i + 1] = bgpix2;
      }
      location.u16 += HMAXSIZE;
      location2.u16 += HMAXSIZE;
    }

  } else {

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
    ui_gtk_swapbanks();
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
  uint8 **oldscreenpp = ui_whichbank ? &ui_screen1 : &ui_screen0;
  uint8 *newlinedata, *oldlinedata;
  unsigned int line;
  uint8 *location;
  uint8 *evenscreen;            /* interlace: lines 0,2,etc. */
  uint8 *oddscreen;             /*            lines 1,3,etc. */

  if (overlay)
    SDL_LockYUVOverlay(overlay);
  else if (ui_locksurface && SDL_LockSurface(screen) != 0)
    ui_err("Failed to lock SDL surface");

  for (line = 0; line < VSIZE; line++) {
    newlinedata = ui_newscreen +
        line * HMAXSIZE * screen->format->BytesPerPixel;
    oldlinedata = *oldscreenpp +
        line * HMAXSIZE * screen->format->BytesPerPixel;

    if (overlay) {
      memcpy(*overlay->pixels + line * overlay->pitches[0],
	newlinedata, HSIZE*2);
    } else {


    switch (ui_screenmode) {
    case SCREEN_100:
      location = screen->pixels + line * screen->pitch;
      /* we could use uiplot_renderXX_x1 routines here, but SDL wouldn't
         pick up our delta changes so we don't */
      memcpy(location, newlinedata, HSIZE * screen->format->BytesPerPixel);
      break;
    case SCREEN_200:
    case SCREEN_FULL:
      location = screen->pixels + 2 * line * screen->pitch;
      if (screen->format->BytesPerPixel != 2 &&
          screen->format->BytesPerPixel != 4)
        ui_err("unsupported mode for 200%% scaling\n");
      switch ((vdp_reg[12] >> 1) & 3) { /* interlace mode */
      case 0:
      case 1:
      case 2:
        switch (screen->format->BytesPerPixel) {
        case 2:
          uiplot_render16_x2((uint16 *)newlinedata, NULL, location,
                             screen->pitch, HSIZE);
          break;
        case 4:
          uiplot_render32_x2((uint32 *)newlinedata, NULL, location,
                             screen->pitch, HSIZE);
          break;
        }
        break;
      case 3:
        /* work out which buffer contains the odd and even fields */
        if (vdp_oddframe) {
          oddscreen = ui_newscreen;
          evenscreen = ui_whichbank ? ui_screen0 : ui_screen1;
        } else {
          evenscreen = ui_newscreen;
          oddscreen = ui_whichbank ? ui_screen0 : ui_screen1;
        }
        switch (ui_interlace) {
        case DEINTERLACE_BOB:
          switch (screen->format->BytesPerPixel) {
          case 2:
            uiplot_render16_x2((uint16 *)newlinedata, NULL, location,
                               screen->pitch, HSIZE);
            break;
          case 4:
            uiplot_render32_x2((uint32 *)newlinedata, NULL, location,
                               screen->pitch, HSIZE);
            break;
          }
          break;
        case DEINTERLACE_WEAVE:
          evenscreen += line * HMAXSIZE * screen->format->BytesPerPixel;
          oddscreen += line * HMAXSIZE * screen->format->BytesPerPixel;
          switch (screen->format->BytesPerPixel) {
          case 2:
            uiplot_render16_x2h((uint16 *)evenscreen, NULL, location, HSIZE);
            uiplot_render16_x2h((uint16 *)oddscreen, NULL,
                                location + screen->pitch, HSIZE);
            break;
          case 4:
            uiplot_render32_x2h((uint32 *)evenscreen, NULL, location, HSIZE);
            uiplot_render32_x2h((uint32 *)oddscreen, NULL,
                                location + screen->pitch, HSIZE);
            break;
          }
          break;
        case DEINTERLACE_WEAVEFILTER:
          evenscreen += line * HMAXSIZE * screen->format->BytesPerPixel;
          oddscreen += line * HMAXSIZE * screen->format->BytesPerPixel;
          switch (screen->format->BytesPerPixel) {
          case 2:
            /* lines line+0 and line+1 */
            uiplot_irender16_weavefilter((uint16 *)evenscreen,
                                         (uint16 *)oddscreen,
                                         location, HSIZE);
            /* lines line+1 and line+2 */
            uiplot_irender16_weavefilter((uint16 *)oddscreen,
                                         ((uint16 *)evenscreen) + HMAXSIZE,
                                         location + screen->pitch, HSIZE);
            break;
          case 4:
            /* lines line+0 and line+1 */
            uiplot_irender32_weavefilter((uint32 *)evenscreen,
                                         (uint32 *)oddscreen,
                                         location, HSIZE);
            /* lines line+1 and line+2 */
            uiplot_irender32_weavefilter((uint32 *)oddscreen,
                                         ((uint32 *)evenscreen) + HMAXSIZE,
                                         location + screen->pitch, HSIZE);
            break;
          }
        }
        break;
      }
      break;
    default:
      ui_err("invalid screen mode\n");
    }
  }

  }

  ui_gtk_redraw();
  if (overlay)
    SDL_UnlockYUVOverlay(overlay);
  else if (ui_locksurface)
    SDL_UnlockSurface(screen);
#if 0
  if (ui_vsync)
    uip_vsync();
#endif
  ui_gtk_swapbanks();
}

/*** logging functions ***/

/* logging is done this way because this was the best I could come up with
   whilst battling with macros that can only take fixed numbers of arguments */

#define LOG_FUNC(name,level,txt) void ui_log_ ## name (const char *text, ...) \
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

/*** gtk functions ***/

gint ui_gtk_configuremain(GtkWidget *widget, GdkEventConfigure *event)
{
  (void)widget;
  (void)event;
  /* if we were allowing resizing this is where we'd do it, but note
     to check whether it's just the location or the size being changed
     as a call to SDL to set the video mode will erase the screen and
     then everything would go black */
  return TRUE;
}

void ui_gtk_filesel_ok(GtkFileSelection *obj, gpointer user_data)
{
  char msg[512];
  char buf[8];
  char *file;
  struct stat s;
  int avi_skip, avi_jpeg;

  (void)obj;
  (void)user_data;
  file = gtk_file_selection_get_filename(GTK_FILE_SELECTION(ui_win_filesel));
  gtk_widget_hide(ui_win_filesel);

  if (ui_filesel_save) {
    if (0 == stat(file, &s)) {
      size_t size = sizeof msg;
      char *p = msg;
    
      p = append_string(p, &size, "The file \""); 
      p = append_string(p, &size, file); 
      p = append_string(p, &size, "\" already exists - overwrite?"); 

      if (ui_gtk_query(msg, 0) != 0)
        return;
    }
  }
  *msg = '\0';

  switch ((ui_filesel_type << 1) | ui_filesel_save) {
  case 0:
    {
      const char *error;
      
      error = gen_loadimage(file);
      if (error) {
        size_t size = sizeof msg;
        char *p = msg;

        p = append_string(p, &size,
              "An error occured whilst tring to load the ROM \"");
        p = append_string(p, &size, file);
        p = append_string(p, &size, "\": ");
        p = append_string(p, &size, error);
      }
    }
    break;
  case 1:
    {
      size_t size = sizeof msg;
      char *p = msg;
      FILE *f;

      if (NULL == cpu68k_rom || 0 == cpu68k_romlen) {
        p = append_string(p, &size,
              "There is no ROM currently in memory to save!");
        break;
      }
      if (gen_modifiedrom) {
        if (
            0 != ui_gtk_query("The ROM in memory has been modified by cheat "
              "codes - are you really sure you want to save this ROM?", 0)
           )
          break;
      }
      f = fopen(file, "wb");
      if (f) {
        size_t ret, n = cpu68k_romlen;

        ret = fwrite(cpu68k_rom, 1, n, f);
        fclose(f);

        if (ret != n) {
          p = append_string(p, &size,
                "An error occured whilst trying to save the ROM as \"");
          p = append_string(p, &size, file);
          p = append_string(p, &size, "\": ");
          p = append_string(p, &size, strerror(errno));
        }
      }
    }
    break;
  case 2:
    if (0 != stat(file, &s)) {
      size_t size = sizeof msg;
      char *p = msg;
      
      p = append_string(p, &size, "stat() failed for \"");
      p = append_string(p, &size, file);
      p = append_string(p, &size, "\": ");
      p = append_string(p, &size, strerror(errno));
      break;
    }
    if (state_loadfile(file) != 0) {
      size_t size = sizeof msg;
      char *p = msg;

      p = append_string(p, &size,
            "An error occured whilst trying to load state from \"");
      p = append_string(p, &size, file);
      p = append_string(p, &size, "\": ");
      p = append_string(p, &size, strerror(errno));
    }
    break;
  case 3:
    if (0 != state_savefile(file)) {
      size_t size = sizeof msg;
      char *p = msg;

      p = append_string(p, &size,
            "An error occured whilst trying to save state to \"");
      p = append_string(p, &size, file);
      p = append_string(p, &size, "\": ");
      p = append_string(p, &size, strerror(errno));
    }
    break;
  case 5:
    {
      size_t size = sizeof msg;
      char *p = msg;
      
      if (-1 != ui_musicfile) {
        p = append_string(p, &size, "There is already a music log in progress");
        break;
      }

      ui_musicfile = open(file, O_CREAT | O_WRONLY | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH /* 0644 */);

      if (-1 == ui_musicfile) {
        p = append_string(p, &size,
              "An error occured whilst trying to start a GYM log to \"");
        p = append_string(p, &size, file);
        p = append_string(p, &size, "\": ");
        p = append_string(p, &size, strerror(errno));
      }
    
      gen_musiclog = 1;
    }
    break;
  case 7:
    {
      size_t size = sizeof msg;
      char *p = msg;

      if (-1 != ui_musicfile) {
        p = append_string(p, &size, "There is already a music log in progress");
        break;
      }
      
      ui_musicfile = open(file, O_CREAT | O_WRONLY | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH /* 0644 */);
      
      if (-1 == ui_musicfile) {
        p = append_string(p, &size,
              "An error occured whilst trying to start a GNM log to \"");
        p = append_string(p, &size, file);
        p = append_string(p, &size, "\": ");
        p = append_string(p, &size, strerror(errno));
      }

      memcpy(buf, "GNM", 3);
      buf[3] = vdp_framerate;
      write(ui_musicfile, buf, 4);
      gen_musiclog = 2;
    }
    break;
  case 8:
    if (0 != stat(file, &s)) {
      size_t size = sizeof msg;
      char *p = msg;

      p = append_string(p, &size,
            "stat() failed for \"");
      p = append_string(p, &size, file);
      p = append_string(p, &size, "\": ");
      p = append_string(p, &size, strerror(errno));
      break;
    }
    if (0 != patch_loadfile(file)) {
      size_t size = sizeof msg;
      char *p = msg;

      p = append_string(p, &size,
            "An error occured whilst trying to load state from \"");
      p = append_string(p, &size, file);
      p = append_string(p, &size, "\": ");
      p = append_string(p, &size, strerror(errno));
    }
    break;
  case 9:
    if (0 != patch_savefile(file)) {
      size_t size = sizeof msg;
      char *p = msg;

      p = append_string(p, &size,
            "An error occured whilst trying to save state to \"");
      p = append_string(p, &size, file);
      p = append_string(p, &size, "\": ");
      p = append_string(p, &size, strerror(errno));
    }
    break;
  case 11:
    if (ui_avi) {
      size_t size = sizeof msg;
      
      (void) append_string(msg, &size, "There is already an avi in progress");
      break;
    }
    avi_skip = atoi(gtkopts_getvalue("aviframeskip"));
    avi_jpeg = !strcasecmp(gtkopts_getvalue("aviformat"), "jpeg");
    ui_aviinfo.width = HSIZE;
    ui_aviinfo.height = VSIZE;
    ui_aviinfo.sampspersec = sound_speed;
    ui_aviinfo.fps = (vdp_framerate * 1000) / avi_skip;
    ui_aviinfo.jpegquality = atoi(gtkopts_getvalue("jpegquality"));
    if (NULL == (ui_avi = avi_open(file, &ui_aviinfo, avi_jpeg))) {
      size_t size = sizeof msg;
      char *p = msg;
      
      p = append_string(p, &size,
            "An error occured whilst trying to start the AVI: ");
      p = append_string(p, &size, strerror(errno));
      break;
    }
    if ((ui_aviaudio = malloc((sound_sampsperfield + 1) * 4)) == NULL ||
        (ui_avivideo = malloc(ui_avi->linebytes * VSIZE)) == NULL)
      ui_err("out of memory");
    ui_frameskip = avi_skip;
    break;
  default:
    strcpy(msg, "Not implemented.");
    break;
  }
  if (msg[0])
    ui_gtk_dialog(msg);
}

GtkWidget *ui_gtk_dialog(const char *msg)
{
  GtkWidget *dialog, *label, *button_ok;

  dialog = gtk_dialog_new();
  label = gtk_label_new(msg);
  gtk_label_set_line_wrap(GTK_LABEL(label), 1);
  button_ok = gtk_button_new_with_label("OK");
  gtk_signal_connect_object(GTK_OBJECT(button_ok), "clicked",
                            GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT(dialog));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
                    button_ok);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
  gtk_window_set_modal(GTK_WINDOW(dialog), 1);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 150);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_widget_show_all(dialog);
  return dialog;
}

/* supporting function for ui_gtk_query */

static void ui_gtk_response(GtkButton *button, gpointer func_data)
{
  (void)button;
  ui_query_response = GPOINTER_TO_INT(func_data);
  if (gtk_main_level() > 0)
    gtk_main_quit();
}

/* ask a question and return 0 for OK/Yes, -1 for Cancel/No */

static int ui_gtk_query(const char *msg, int style)
{
  GtkWidget *dialog, *label, *button_a, *button_b, *hbox;

  dialog = gtk_dialog_new();
  label = gtk_label_new(msg);
  hbox = gtk_hbox_new(1, 8);
  gtk_label_set_line_wrap(GTK_LABEL(label), 1);
  switch (style) {
  default:
  case 0:
    button_a = gtk_button_new_with_label("OK");
    button_b = gtk_button_new_with_label("Cancel");
    break;
  case 1:
    button_a = gtk_button_new_with_label("Yes");
    button_b = gtk_button_new_with_label("No");
    break;
  }
  gtk_box_pack_end(GTK_BOX(hbox), button_b, 0, 1, 0);
  gtk_box_pack_end(GTK_BOX(hbox), button_a, 0, 1, 0);
  gtk_signal_connect(GTK_OBJECT(button_a), "clicked",
                     GTK_SIGNAL_FUNC(ui_gtk_response),
                     GINT_TO_POINTER(0));
  gtk_signal_connect(GTK_OBJECT(button_b), "clicked",
                     GTK_SIGNAL_FUNC(ui_gtk_response),
                     GINT_TO_POINTER(-1));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
                    hbox);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
  gtk_window_set_modal(GTK_WINDOW(dialog), 1);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 150);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_widget_show_all(dialog);
  ui_query_response = -1;
  gtk_main();
  gtk_widget_destroy(dialog);
  return ui_query_response;
}

void ui_gtk_filesel(int type, int save)
{
  ui_filesel_type = type;
  ui_filesel_save = save;

  switch ((type << 1) | save) {
  case 0:
    gtk_window_set_title(GTK_WINDOW(ui_win_filesel),
                         "Choose ROM file (BIN or SMD format) to load");
    break;
  case 1:
    gtk_window_set_title(GTK_WINDOW(ui_win_filesel),
                         "Choose filename to save ROM file (BIN format) to");
    break;
  case 2:
    gtk_window_set_title(GTK_WINDOW(ui_win_filesel),
                         "Choose state file to load");
    break;
  case 3:
    gtk_window_set_title(GTK_WINDOW(ui_win_filesel),
                         "Choose filename to save state to");
    break;
  case 5:
    gtk_window_set_title(GTK_WINDOW(ui_win_filesel),
                         "Choose where to start GYM logging");
    break;
  case 7:
    gtk_window_set_title(GTK_WINDOW(ui_win_filesel),
                         "Choose where to start GNM logging");
    break;
  }
  gtk_widget_show(ui_win_filesel);
}

void ui_gtk_about(void)
{
  gtk_widget_show(ui_win_about);
}

void ui_gtk_options(void)
{
  ui_gtk_opts_to_window();
  gtk_widget_show(ui_win_opts);
}

void ui_gtk_console(void)
{
  gtk_widget_show(ui_win_console);
}

void ui_gtk_quit(void)
{
  gen_quit = 1;
  if (gtk_main_level() > 0)
    gtk_main_quit();
}

void ui_gtk_closeconsole(void)
{
  gtk_widget_hide(ui_win_console);
}

void ui_gtk_play(void)
{
  if (ui_running) {
    ui_gtk_dialog("Generator is already playing a ROM");
    return;
  }
  if (cpu68k_rom == NULL || cpu68k_romlen == 0) {
    ui_gtk_dialog("You must load a ROM into Generator");
    return;
  }
  /* start running the game */
  ui_running = 1;
  if (sound_on)
    sound_active = !sound_start();
  else
    sound_active = 0;
  if (gtk_main_level() > 0)
    gtk_main_quit();
}

void ui_gtk_pause(void)
{
  if (!ui_running) {
    ui_gtk_dialog("Generator is not playing a ROM");
    return;
  }
  if (sound_on)
    sound_stop();
  ui_running = 0;
  ui_was_paused = 1;
}

void ui_gtk_softreset(void)
{
  gen_softreset();
}

void ui_gtk_hardreset(void)
{
  gen_reset();
}

static int
create_overlay(int allow_software_overlay)
{
  unsigned int i;
  
  for (i = 0; i < sizeof overlay_formats / sizeof overlay_formats[0]; i++) {
    overlay = SDL_CreateYUVOverlay(HSIZE, VSIZE,
                  overlay_formats[i].fourcc, screen);
    if (overlay) {
      if (allow_software_overlay || overlay->hw_overlay) {
        const uint8 *id = (uint8 *) &overlay_formats[i].fourcc;
        LOG_REQUEST(("Using overlay with format fourcc=0x%08x (%c%c%c%c)",
          overlay_formats[i].fourcc, id[0], id[1], id[2], id[3]));
        overlay_format = overlay_formats[i].alias;
        return 0;
      }
      
      SDL_FreeYUVOverlay(overlay);
      overlay = NULL;
    }
  }
  return -1;
}
 
int ui_create_overlay(void)
{
  if (0 == create_overlay(0)) {
    LOG_REQUEST(("YUV overlay is hardware accelerated"));
    return 0;
  }
  if (0 == create_overlay(1)) {
    LOG_REQUEST(("YUV overlay is NOT hardware accelerated"));
    return 0;
  }
  LOG_REQUEST(("No YUV overlay available"));
  return -1;
}

void ui_gtk_reinit_video(void)
{
    if (overlay) {
      SDL_FreeYUVOverlay(overlay);
      overlay = NULL;
    }
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

static SDL_Surface *ui_sdl_set_fullscreen(gboolean use_maximum)
{
  SDL_Rect **modes;
  int i;
  int w = 0, h = 0;

  modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
  if (modes == (SDL_Rect **) -1) {
    w = DEFAULT_FULLSCREEN_WIDTH;
    h = DEFAULT_FULLSCREEN_HEIGHT;
    LOG_VERBOSE(("Hardware supports any resolution; using defaults."));
  } else {

    for (i = 0; modes[i]; i++) {
      LOG_VERBOSE(("%dx%d", modes[i]->w, modes[i]->h));
      if (
          (
            (use_maximum && modes[i]->w >= DEFAULT_FULLSCREEN_WIDTH) ||
            (!use_maximum && modes[i]->w == DEFAULT_FULLSCREEN_WIDTH)
          ) &&
          3 * modes[i]->w == 4 * modes[i]->h
      ) {
        w = modes[i]->w;
        h = modes[i]->h;
        break;
      }
    }
        
    for (i = 0; !w && !h && modes[i]; i++) {

      /* Use the first best mode with 4:3 aspect ratio */
      if (3 * modes[i]->w == 4 * modes[i]->h) {
        w = modes[i]->w;
        h = modes[i]->h;
        break;
      }
    }

    /* Use first best mode if there's no 4:3 mode available */
    if (!w && !h && i) {
      LOG_NORMAL(("No video mode with 4:3 aspect ratio found", w, h));
      w = modes[0]->w;
      h = modes[0]->h;
    }
  }

  LOG_NORMAL(("Switching to fullscreen (%d x %d)", w, h));
  if (w && h) {
    return SDL_SetVideoMode(w, h, 0, SDL_FULLSCREEN);
  }

  LOG_CRITICAL(("No video mode for fullscreen available"));
  return NULL;
}

/* set main window size from current parameters */

void ui_gtk_sizechange(ui_screenmode_t newmode)
{
  GtkWidget *w;
  ui_screenmode_t oldmode = ui_screenmode;
  int overlay_failed = 0;

  w = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main),
                                     "drawingarea_main"));

  if (screenmode_lock)
    return;

modeswitch:

  switch (newmode) {
  case SCREEN_100:
    ui_arcade_mode = 0;
    hborder = ui_hborder;
    vborder = ui_vborder;
    gtk_widget_show(ui_win_main);
    gtk_widget_set_usize(w, HSIZE, VSIZE);
    ui_sdl_window_hack(1);
    ui_gtk_reinit_video();
    screen = SDL_SetVideoMode(HSIZE, VSIZE, 0, SDL_RESIZABLE);
    ui_screenmode = SCREEN_100;
    gtkopts_setvalue("view", "100");
    break;
  case SCREEN_200:
    ui_arcade_mode = 0;
    hborder = ui_hborder;
    vborder = ui_vborder;
    gtk_widget_show(ui_win_main);
    gtk_widget_set_usize(w, HSIZE * 2, VSIZE * 2);
    ui_sdl_window_hack(1);
    ui_gtk_reinit_video();
    screen = SDL_SetVideoMode(HSIZE * 2, VSIZE * 2, 0, SDL_RESIZABLE);
    ui_screenmode = SCREEN_200;
    gtkopts_setvalue("view", "200");
    break;
  case SCREEN_FULL:
    hborder = 0;
    vborder = MIN(ui_vborder, 8);
    gtk_widget_hide(ui_win_main);
    ui_sdl_window_hack(0);
    ui_gtk_reinit_video();
    screen = ui_sdl_set_fullscreen(!overlay_failed);
    if (!screen) {
      LOG_CRITICAL(("failed to initialise fullscreen mode"));
      newmode = SCREEN_100;
      goto modeswitch;
    }
    ui_screenmode = SCREEN_FULL;
    gtkopts_setvalue("view", "fullscreen");
    break;
  default:
    ui_err("invalid screen mode\n");
  }

  if (overlay && (ui_screenmode != oldmode)) {
    SDL_FreeYUVOverlay(overlay);
    overlay = NULL;
  }

  if (!overlay && !overlay_failed && 0 != ui_create_overlay()) {
    overlay_failed = 1;
    goto modeswitch;
  }

#if 0
  if (overlay) {
	  int i;
    Uint32 key;

    printf("colorkey=%lx\n", (long) screen->format->colorkey);
    key = SDL_MapRGB(screen->format, 0, 0, 0);
    SDL_SetColorKey(screen, SDL_SRCCOLORKEY, key);
    SDL_LockSurface(screen);
    for (i = 0; i < screen->h; i++) {
      memset((char *) screen->pixels + i * screen->pitch, 0, screen->pitch);
    }
    SDL_UnlockSurface(screen);
    SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
  }
#endif

  w = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main), "hbox_bottom"));
  if (ui_statusbar)
    gtk_widget_show(w);
  else
    gtk_widget_hide(w);

  gtk_window_set_policy(GTK_WINDOW(ui_win_main),
    overlay != NULL, overlay != NULL, overlay != NULL);
  ui_locksurface = SDL_MUSTLOCK(screen);
  uiplot_setshifts(ui_topbit(screen->format->Rmask) - 4,
                   ui_topbit(screen->format->Gmask) - 4,
                   ui_topbit(screen->format->Bmask) - 4);

  /* Gimmick: Redraw overlay when paused. Otherwise, only a rectangle
   * with the colorkey is visible. */
  if (!ui_running && overlay && ui_screen0 && ui_screen1 && ui_newscreen)
    ui_rendertoscreen();
}

static const char * 
key_opt_name(unsigned controller, const char *key)
{
  static char buf[128];
  char *p = buf;
  size_t size = sizeof buf;
      
  p = append_string(p, &size, "key");
  p = append_uint(p, &size, 1 + controller);
  p = append_string(p, &size, "_");
  p = append_string(p, &size, key);

  return buf;
}

static const char * 
joy_opt_name(unsigned controller, const char *key)
{
  static char buf[128];
  char *p = buf;
  size_t size = sizeof buf;
      
  p = append_string(p, &size, "joy");
  p = append_uint(p, &size, 1 + controller);
  p = append_string(p, &size, "_");
  p = append_string(p, &size, key);

  return buf;
}

void ui_gtk_newoptions(void)
{
  const char *v;
  int i, c;
  ui_screenmode_t screenmode = ui_screenmode;
  unsigned int old_sound_on = sound_on;
  unsigned int old_sound_minfields = sound_minfields;
  unsigned int old_sound_maxfields = sound_maxfields;
  unsigned int old_ui_hborder = ui_hborder;
  unsigned int old_ui_vborder = ui_vborder;
  unsigned int old_ui_statusbar = ui_statusbar;

  if (ui_arcade_mode) {
    screenmode = SCREEN_FULL;
  } else {
    v = gtkopts_getvalue("view");
    if (!strcasecmp(v, "fullscreen")) {
      screenmode = SCREEN_FULL;
    } else {
      i = atoi(v);
      screenmode = i == 200 ? SCREEN_200 : SCREEN_100;
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
    /* No decent region set, let's load from the config file. */
    v = gtkopts_getvalue("autodetect");
    gen_autodetect = !strcasecmp(v, "on");
    
    if (!gen_autodetect) {
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

  if (screenmode != SCREEN_FULL) {
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

  for (c = 0; c < 2; c++) {
    /* keyboard controls */
    v = gtkopts_getvalue(key_opt_name(c, "a"));
    ui_cont[c].a = gdk_keyval_from_name(v);
    
    v = gtkopts_getvalue(key_opt_name(c, "b"));
    ui_cont[c].b = gdk_keyval_from_name(v);
    
    v = gtkopts_getvalue(key_opt_name(c, "c"));
    ui_cont[c].c = gdk_keyval_from_name(v);
    
    v = gtkopts_getvalue(key_opt_name(c, "up"));
    ui_cont[c].up = gdk_keyval_from_name(v);
    
    v = gtkopts_getvalue(key_opt_name(c, "down"));
    ui_cont[c].down = gdk_keyval_from_name(v);
    
    v = gtkopts_getvalue(key_opt_name(c, "left"));
    ui_cont[c].left = gdk_keyval_from_name(v);
    
    v = gtkopts_getvalue(key_opt_name(c, "right"));
    ui_cont[c].right = gdk_keyval_from_name(v);
    
    v = gtkopts_getvalue(key_opt_name(c, "start"));
    ui_cont[c].start = gdk_keyval_from_name(v);
    
    /* joypad controls */
    v = gtkopts_getvalue(joy_opt_name(c, "a"));
    ui_joy[c].a = atoi(v);
    
    v = gtkopts_getvalue(joy_opt_name(c, "b"));
    ui_joy[c].b = atoi(v);
    
    v = gtkopts_getvalue(joy_opt_name(c, "c"));
    ui_joy[c].c = atoi(v);
    
    v = gtkopts_getvalue(joy_opt_name(c, "start"));
    ui_joy[c].start = atoi(v);
    
    v = gtkopts_getvalue(joy_opt_name(c, "left"));
    ui_joy[c].left = atoi(v);
    
    v = gtkopts_getvalue(joy_opt_name(c, "right"));
    ui_joy[c].right = atoi(v);
    
    v = gtkopts_getvalue(joy_opt_name(c, "up"));
    ui_joy[c].up = atoi(v);
    
    v = gtkopts_getvalue(joy_opt_name(c, "down"));
    ui_joy[c].down = atoi(v);
  }

  if (
      screenmode != ui_screenmode ||
      ui_hborder != old_ui_hborder ||
      ui_vborder != old_ui_vborder ||
      ui_statusbar != old_ui_statusbar
  ) {
    ui_gtk_sizechange(screenmode);
  }

  if (
    sound_minfields != old_sound_minfields ||
    sound_maxfields != old_sound_maxfields ||
    sound_on != old_sound_on
  ) {
    sound_stop();
    sound_active = sound_on ? !sound_start() : 0;
  }
}

static void ui_gtk_opts_from_window(void)
{
  GtkWidget *obj, *active;
  char buf[64];
  int c;
  const char *v;
  const char **key;

  /* hardware - region */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_domestic"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("region", "domestic");
  else
    gtkopts_setvalue("region", "overseas");

  /* hardware - video standard */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_pal"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("videostd", "pal");
  else
    gtkopts_setvalue("videostd", "ntsc");

  /* hardware - auto detect */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_autodetect"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("autodetect", "on");
  else
    gtkopts_setvalue("autodetect", "off");

  /* video - plotter */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_line"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("plotter", "line");
  else
    gtkopts_setvalue("plotter", "cell");

  /* video - interlace mode */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "optionmenu_interlace"));
  /* now get active widget on menu */
  obj = GTK_BIN(GTK_OPTION_MENU(obj))->child;
  gtkopts_setvalue("interlace", GTK_LABEL(obj)->label);

  /* video - frame skip */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_auto"));
  if (GTK_TOGGLE_BUTTON(obj)->active) {
    gtkopts_setvalue("frameskip", "auto");
  } else {
    size_t size = sizeof buf;
    
    obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                         "hscale_skip"));
    (void) append_uint(buf, &size,
             gtk_range_get_adjustment(GTK_RANGE(obj))->value);
    gtkopts_setvalue("frameskip", buf);
  }

  /* video - hborder */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_hborder"));
  print_uint(buf, sizeof buf,
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("hborder", buf);

  /* video - vborder */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_vborder"));
  print_uint(buf, sizeof buf,
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("vborder", buf);

  /* sound - z80 */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_z80"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("z80", "on");
  else
    gtkopts_setvalue("z80", "off");

  /* sound - on/off */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_sound"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("sound", "on");
  else
    gtkopts_setvalue("sound", "off");

  /* sound - psg */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_psg"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("psg", "on");
  else
    gtkopts_setvalue("psg", "off");

  /* sound - fm */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_fm"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("fm", "on");
  else
    gtkopts_setvalue("fm", "off");

  /* sound - min fields */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_minfields"));
  print_uint(buf, sizeof buf,
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("sound_minfields", buf);

  /* sound - max fields */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_maxfields"));
  print_uint(buf, sizeof buf,
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("sound_maxfields", buf);

  /* sound - filter */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_filter"));
  print_uint(buf, sizeof buf,
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("lowpassfilter", buf);

  /* logging - verbosity */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "optionmenu_level"));
  obj = GTK_OPTION_MENU(obj)->menu;
  /* now get active widget on menu */
  active = gtk_menu_get_active(GTK_MENU(obj));
  print_uint(buf, sizeof buf,
    g_list_index(GTK_MENU_SHELL(obj)->children, active));
  gtkopts_setvalue("loglevel", buf);

  /* logging - sound */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_debugsound"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("debugsound", "on");
  else
    gtkopts_setvalue("debugsound", "off");

  /* logging - status bar */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_statusbar"));
  if (GTK_TOGGLE_BUTTON(obj)->active)
    gtkopts_setvalue("statusbar", "on");
  else
    gtkopts_setvalue("statusbar", "off");

  /* logging - avi format */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "optionmenu_aviformat"));
  /* now get active widget on menu */
  obj = GTK_BIN(GTK_OPTION_MENU(obj))->child;
  gtkopts_setvalue("aviformat", GTK_LABEL(obj)->label);

  /* logging - avi frame skip */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "hscale_avi"));
  print_uint(buf, sizeof buf,
    gtk_range_get_adjustment(GTK_RANGE(obj))->value);
  gtkopts_setvalue("aviframeskip", buf);

  /* logging - jpeg quality */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_jpegquality"));
  print_uint(buf, sizeof buf,
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("jpegquality", buf);

  /* controls */

  for (c = 0; c < 2; c++) {
    const char *end = cast_to_char_ptr(ui_gtk_keys) + sizeof ui_gtk_keys;
    
    for (key = ui_gtk_keys; cast_to_char_ptr(key) < end; key++) {
      {
        size_t size = sizeof buf;
        char *p = buf;

        p = append_string(p, &size, "entry_player");
        p = append_uint(p, &size, 1 + c);
        p = append_string(p, &size, "_");
        p = append_string(p, &size, *key);
      }
      
      obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts), buf));
      v = gtk_entry_get_text(GTK_ENTRY(obj));
      gtkopts_setvalue(key_opt_name(c, *key), v);
    }

  }
}

static void ui_gtk_opts_to_window(void)
{
  GtkWidget *obj;
  const char *v;
  const char **key;
  int i, c;
  char buf[32];

  /* hardware - region */

  v = gtkopts_getvalue("region");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_domestic"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj),
                               !strcasecmp(v, "domestic"));
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_overseas"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj),
                               !strcasecmp(v, "overseas"));

  /* hardware - video standard */

  v = gtkopts_getvalue("videostd");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_pal"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), !strcasecmp(v, "pal"));
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_ntsc"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj),
                               !strcasecmp(v, "ntsc"));

  /* hardware - auto detect */

  v = gtkopts_getvalue("autodetect");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_autodetect"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), !strcasecmp(v, "on"));

  /* video - plotter */

  v = gtkopts_getvalue("plotter");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_line"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj),
                               !strcasecmp(v, "line"));
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "radiobutton_cell"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj),
                               !strcasecmp(v, "cell"));

  /* video - interlace mode */

  v = gtkopts_getvalue("interlace");
  if (!strcasecmp(v, "bob")) {
    i = 0;
  } else if (!strcasecmp(v, "weave")) {
    i = 1;
  } else {
    i = 2;
  }
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "optionmenu_interlace"));
  gtk_option_menu_set_history(GTK_OPTION_MENU(obj), i);

  /* video - frame skip */

  v = gtkopts_getvalue("frameskip");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_auto"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj),
                               !strcasecmp(v, "auto"));
  i = atoi(v);
  if (i < 1)
    i = 1;
  if (i > 10)
    i = 10;
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "hscale_skip"));
  gtk_adjustment_set_value(gtk_range_get_adjustment(GTK_RANGE(obj)), i);

  /* video - hborder */

  v = gtkopts_getvalue("hborder");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_hborder"));
  i = atoi(v);
  if (i < 0)
    i = 0;
  if (i > HBORDER_MAX)
    i = HBORDER_MAX;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), i);

  /* video - vborder */

  v = gtkopts_getvalue("vborder");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_vborder"));
  i = atoi(v);
  if (i < 0)
    i = 0;
  if (i > VBORDER_MAX)
    i = VBORDER_MAX;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), i);

  /* sound - z80 */

  v = gtkopts_getvalue("z80");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_z80"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), !strcasecmp(v, "on"));

  /* sound - on/off */

  v = gtkopts_getvalue("sound");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_sound"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), !strcasecmp(v, "on"));

  /* sound - psg */

  v = gtkopts_getvalue("psg");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_psg"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), !strcasecmp(v, "on"));

  /* sound - fm */

  v = gtkopts_getvalue("fm");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_fm"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), !strcasecmp(v, "on"));

  /* sound - min fields */

  v = gtkopts_getvalue("sound_minfields");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_minfields"));
  i = atoi(v);
  if (i < 1)
    i = 1;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), i);

  /* sound - max fields */

  v = gtkopts_getvalue("sound_maxfields");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_maxfields"));
  i = atoi(v);
  if (i < 1)
    i = 1;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), i);

  /* sound - filter */

  v = gtkopts_getvalue("lowpassfilter");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_filter"));
  i = atoi(v);
  if (i < 1)
    i = 1;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), i);

  /* logging - verbosity */

  v = gtkopts_getvalue("loglevel");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "optionmenu_level"));
  i = atoi(v);
  gtk_option_menu_set_history(GTK_OPTION_MENU(obj), i);

  /* logging - sound */

  v = gtkopts_getvalue("debugsound");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_debugsound"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), !strcasecmp(v, "on"));

  /* logging - status bar */

  v = gtkopts_getvalue("statusbar");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "checkbutton_statusbar"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obj), !strcasecmp(v, "on"));

  /* logging - avi format */

  v = gtkopts_getvalue("aviformat");
  if (!strcasecmp(v, "rgb")) {
    i = 0;
  } else {
    i = 1;
  }
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "optionmenu_aviformat"));
  gtk_option_menu_set_history(GTK_OPTION_MENU(obj), i);

  /* logging - avi frame skip */

  i = atoi(gtkopts_getvalue("aviframeskip"));
  if (i < 1)
    i = 1;
  if (i > 10)
    i = 10;
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "hscale_avi"));
  gtk_adjustment_set_value(gtk_range_get_adjustment(GTK_RANGE(obj)), i);
  
  /* logging - jpeg quality */

  v = gtkopts_getvalue("jpegquality");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_jpegquality"));
  i = atoi(v);
  if (i < 1)
    i = 1;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(obj), i);

  /* controls */

  for (c = 0; c < 2; c++) {
    const char *end = cast_to_char_ptr(ui_gtk_keys) + sizeof ui_gtk_keys;

    for (key = ui_gtk_keys; cast_to_char_ptr(key) < end; key++) {
      v = gtkopts_getvalue(key_opt_name(c, *key));
      i = gdk_keyval_from_name(v);
      if (i) {
        v = gdk_keyval_name(i); /* just incase case is different etc. */
      } else {
        v = "";
      }
      
      {
        size_t size = sizeof buf;
        char *p = buf;

        p = append_string(p, &size, "entry_player");
        p = append_uint(p, &size, 1 + c);
        p = append_string(p, &size, "_");
        p = append_string(p, &size, *key);
      }
      obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts), buf));
      gtk_entry_set_text(GTK_ENTRY(obj), v);
    }
  }
}

void ui_gtk_applyoptions(void)
{
  ui_gtk_opts_from_window();
  gtk_widget_hide(ui_win_opts);
  ui_gtk_newoptions();
  ui_gtk_opts_to_menu();
}

void ui_gtk_saveoptions(void)
{
  ui_gtk_opts_from_window();
  if (0 != gtkopts_save(ui_configfile)) {
    char buf[512], *p = buf;
    size_t size = sizeof buf;
    
    p = append_string(p, &size, "Failed to save configuration to \"");
    p = append_string(p, &size, ui_configfile);
    p = append_string(p, &size, "\": ");
    p = append_string(p, &size, strerror(errno));

    ui_gtk_dialog(buf);
    return;
  }
  gtk_widget_hide(ui_win_opts);
  ui_gtk_newoptions();
  ui_gtk_opts_to_menu();
}

void ui_gtk_redraw(void)
{
  SDL_Rect rect;

  rect.x = 0;
  rect.y = 0;
  rect.w = screen->w;
  rect.h = screen->h;

  if (overlay) {
    SDL_LockSurface(screen);
    SDL_DisplayYUVOverlay(overlay, &rect);
    SDL_UnlockSurface(screen);
  } else
    SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);

}

void ui_gtk_key(unsigned long key, int press)
{
  t_gtkkeys *cont;
  int c;

  for (c = 0; c < 2; c++) {
    cont = &ui_cont[c];
    if (key == cont->a) {
      mem68k_cont[c].a = press;
    } else if (key == cont->b) {
      mem68k_cont[c].b = press;
    } else if (key == cont->c) {
      mem68k_cont[c].c = press;
    } else if (key == cont->left) {
      mem68k_cont[c].left = press;
    } else if (key == cont->right) {
      mem68k_cont[c].right = press;
    } else if (key == cont->up) {
      mem68k_cont[c].up = press;
    } else if (key == cont->down) {
      mem68k_cont[c].down = press;
    } else if (key == cont->start) {
      mem68k_cont[c].start = press;
    }
  }

  if (key == GDK_Pause && press) {
    if (ui_running)
      ui_gtk_pause();
    else
      ui_gtk_play();
  }
}

static gboolean ui_handle_sdl_events(gpointer data)
{
  (void)data;
  ui_sdl_events();
  return TRUE;
}

static void ui_sdl_events(void)
{
  SDL_Event event;
  int key_gtk = 0;

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
        /* Use alphanumerical characters directly */
        if (
          (event.key.keysym.sym >= SDLK_a && event.key.keysym.sym <= SDLK_z) ||
          (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9)
        ) {
          key_gtk = event.key.keysym.sym;
        } else {
          /* Convert a reasonable subset to GDK key values */
          switch (event.key.keysym.sym) {
          case SDLK_RETURN:	      key_gtk = GDK_Return; break;
          case SDLK_SPACE:	      key_gtk = GDK_space; break;
          case SDLK_LEFT: 	      key_gtk = GDK_Left; break;
          case SDLK_RIGHT: 	      key_gtk = GDK_Right; break;
          case SDLK_UP: 		      key_gtk = GDK_Up; break;
          case SDLK_DOWN: 	      key_gtk = GDK_Down; break;
          case SDLK_INSERT: 	    key_gtk = GDK_Insert; break;
          case SDLK_HOME: 	      key_gtk = GDK_Home; break;
          case SDLK_END: 		      key_gtk = GDK_End; break;
          case SDLK_PAGEUP: 	    key_gtk = GDK_Page_Up; break;
          case SDLK_PAGEDOWN:     key_gtk = GDK_Page_Down; break;
          case SDLK_ASTERISK:     key_gtk = GDK_multiply; break;
          case SDLK_PLUS: 	      key_gtk = GDK_plus; break;
          case SDLK_COMMA: 	      key_gtk = GDK_comma; break;
          case SDLK_MINUS: 	      key_gtk = GDK_minus; break;
          case SDLK_PERIOD: 	    key_gtk = GDK_period; break;
          case SDLK_SLASH: 	      key_gtk = GDK_slash; break;
          case SDLK_COLON: 	      key_gtk = GDK_colon; break;
          case SDLK_SEMICOLON: 	  key_gtk = GDK_semicolon; break;
          case SDLK_LESS: 	      key_gtk = GDK_less; break;
          case SDLK_EQUALS: 	    key_gtk = GDK_equal; break;
          case SDLK_GREATER: 	    key_gtk = GDK_greater; break;
          case SDLK_QUESTION: 	  key_gtk = GDK_question; break;
          case SDLK_AT: 		      key_gtk = GDK_at; break;
          case SDLK_KP0:		      key_gtk	= GDK_KP_0; break;
          case SDLK_KP1:		      key_gtk	= GDK_KP_1; break;
          case SDLK_KP2:		      key_gtk	= GDK_KP_2; break;
          case SDLK_KP3:		      key_gtk	= GDK_KP_3; break;
          case SDLK_KP4:		      key_gtk	= GDK_KP_4; break;
          case SDLK_KP5:		      key_gtk	= GDK_KP_5; break;
          case SDLK_KP6:		      key_gtk	= GDK_KP_6; break;
          case SDLK_KP7:		      key_gtk	= GDK_KP_7; break;
          case SDLK_KP8:		      key_gtk	= GDK_KP_8; break;
          case SDLK_KP9:		      key_gtk	= GDK_KP_9; break;
          case SDLK_KP_PERIOD: 	  key_gtk = GDK_KP_Decimal; break;
          case SDLK_KP_DIVIDE: 	  key_gtk = GDK_KP_Divide; break;
          case SDLK_KP_MULTIPLY: 	key_gtk = GDK_KP_Multiply; break;
          case SDLK_KP_MINUS: 	  key_gtk = GDK_KP_Subtract; break;
          case SDLK_KP_PLUS: 	    key_gtk = GDK_KP_Add; break;
          case SDLK_KP_ENTER: 	  key_gtk = GDK_KP_Enter; break;
          case SDLK_KP_EQUALS: 	  key_gtk = GDK_KP_Equal; break;
          default: ;
                   /* Ignore */
          }
        }

        /* FALL THROUGH because special keys should not be overridable */ 

        switch (event.key.keysym.sym) {
          /* Special keys */
        case SDLK_q:
          if (event.key.keysym.mod & KMOD_CTRL)
            ui_gtk_quit();
          break;
        case SDLK_p:
          if ((event.key.keysym.mod & KMOD_CTRL) == 0)
            break;
            /* FALL THROUGH */
        case SDLK_PAUSE:
          if (KPRESS(event)) {
            if (ui_running)
              ui_gtk_pause();
            else
              ui_gtk_play();
          }
          break;
        case SDLK_f:
          if (KPRESS(event) && event.key.keysym.mod & KMOD_CTRL) {
            ui_gtk_sizechange(ui_screenmode != SCREEN_FULL
                ? SCREEN_FULL : SCREEN_100);
            ui_gtk_opts_to_menu();
          }
          break;
        case SDLK_1:
        case SDLK_2:
          if (KPRESS(event) && event.key.keysym.mod & KMOD_CTRL) {
            ui_gtk_sizechange(event.key.keysym.sym == SDLK_2
                ? SCREEN_200 : SCREEN_100);
            ui_gtk_opts_to_menu();
          }
          break;
        case SDLK_r:
          if (event.key.keysym.mod & KMOD_CTRL) {
            if (event.key.keysym.mod & KMOD_SHIFT)
              ui_gtk_hardreset();
            else
              ui_gtk_softreset();
          }
          break;
        default: /* Ignore */;
        }

        if (key_gtk)
          ui_gtk_key(key_gtk, KPRESS(event));
        break;

      case SDL_ACTIVEEVENT:
        if (event.active.state == SDL_APPMOUSEFOCUS && event.active.gain)
          SDL_ShowCursor(SDL_DISABLE);
        break;
      case SDL_VIDEORESIZE:
        {
          int w = MAX(event.resize.w, 100);
          int h = MAX(event.resize.h, 100);

          if (
            (w != screen->w || h != screen->h) &&
            SDL_VideoModeOK(w, h, screen->format->BitsPerPixel, SDL_RESIZABLE)
          ) {
            screen = SDL_SetVideoMode(w, h, 0, SDL_RESIZABLE);
          }
          break;
        }
      case SDL_QUIT:
        ui_gtk_quit();
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

void ui_gtk_resize_win(GtkWidget *widget, GtkAllocation *allocation,
    gpointer user_data)
{
  int w = allocation->width;
  int h = allocation->height;

  (void)user_data;
  (void)widget;
  
  if (
    screen &&
    (screen->w != w || screen->h != h) &&
	  SDL_VideoModeOK(w, h, screen->format->BitsPerPixel, SDL_RESIZABLE)
  ) {
    screen = SDL_SetVideoMode(w, h, 0, SDL_RESIZABLE);
  }
}

void ui_gtk_mainenter(void)
{
  /* clear out current state */
  memset(mem68k_cont, 0, sizeof(mem68k_cont));
  gdk_key_repeat_disable();
}

void ui_gtk_mainleave(void)
{
  gdk_key_repeat_restore();
}

static void ui_gtk_opts_to_menu(void)
{
  GtkWidget *obj;
  const char *v;

  /* view */

  screenmode_lock++;
  v = gtkopts_getvalue("view");

  if (!strcasecmp(v, "100")) {
    obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main), "zoom_100"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(obj), TRUE);
  }
  if (!strcasecmp(v, "200")) {
    obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main), "zoom_200"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(obj), TRUE);
  }
  if (!strcasecmp(v, "fullscreen")) {
    obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main), "zoom_full"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(obj), TRUE);
  }
  screenmode_lock--;
}

void ui_musiclog(uint8 *data, unsigned int length)
{
  if (-1 != ui_musicfile)
    write(ui_musicfile, data, length);
}

void ui_gtk_closemusiclog(void)
{
  if (-1 == ui_musicfile) {
    ui_gtk_dialog("There is no log to close");
    return;
  }
  close(ui_musicfile);
  ui_musicfile = -1;
  gen_musiclog = 0;
}

static void ui_gtk_codes_to_window(void)
{
  GtkWidget *obj;
  t_patchlist *ent;
  char *data[2];

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  gtk_clist_clear(GTK_CLIST(obj));
  for (ent = patch_patchlist; ent; ent = ent->next) {
    data[0] = ent->code;
    data[1] = ent->comment;
    gtk_clist_append(GTK_CLIST(obj), data);
  }
}

void ui_gtk_codes(void)
{
  ui_gtk_codes_to_window();
  gtk_widget_show(ui_win_codes);
}

void ui_gtk_codes_add(void)
{
  char *data[2];
  char *code, *comment;
  uint32 addr;
  uint16 value;

  code = gtk_entry_get_text(
    GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(ui_win_codes), "entry_action")));
  comment = gtk_entry_get_text(
    GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(ui_win_codes), "entry_comment")));
  
  if (0 != patch_genietoraw(code, &addr, &value)) {
    LOG_NORMAL(("Invalid game genie code"));
    return;
  }

  data[0] = code;
  data[1] = comment;
  gtk_clist_append(
    GTK_CLIST(gtk_object_get_data(GTK_OBJECT(ui_win_codes), "clist_codes")),
    data);
}

void ui_gtk_codes_delete(void)
{
  GtkWidget *obj;
  GList *slist;

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  while ((slist = GTK_CLIST(obj)->selection) != NULL) {
    gtk_clist_remove(GTK_CLIST(obj), (int)slist->data);
  }
}

void ui_gtk_codes_deleteall(void)
{
  GtkWidget *obj;

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  gtk_clist_clear(GTK_CLIST(obj));
}

void ui_gtk_codes_clearsel(void)
{
  GtkWidget *obj;

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  gtk_clist_unselect_all(GTK_CLIST(obj));
}

void ui_gtk_codes_ok(void)
{
  GtkWidget *obj;
  int i;
  char *code, *comment;

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  patch_clearlist();
  for (i = 0;; i++) {
    if (gtk_clist_get_text(GTK_CLIST(obj), i, 0, &code) != 1 ||
        gtk_clist_get_text(GTK_CLIST(obj), i, 1, &comment) != 1)
      break;
    patch_addcode(code, comment);
  }
  gtk_widget_hide(ui_win_codes);
}

void ui_gtk_codes_apply(void)
{
  GtkWidget *obj;
  GList *slist;
  char *code;
  int failed = 0;

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  slist = GTK_CLIST(obj)->selection;
  for (; slist; slist = slist->next) {
    if (gtk_clist_get_text(GTK_CLIST(obj), (int)slist->data, 0, &code) != 1) {
      failed = 1;
      break;
    }
    if (patch_apply(code) != 0)
      failed = 1;
  }
  if (failed)
    ui_gtk_dialog("Unable to apply one or more cheats (either invalid cheat "
                  "code or out of bounds for currently loaded ROM)");
  cpu68k_clearcache();
}

void ui_gtk_closeavi(void)
{
  if (ui_avi == NULL) {
    ui_gtk_dialog("There is no avi to close");
    return;
  }
  if (avi_close(ui_avi) != 0) {
    ui_gtk_dialog(strerror(errno));
    return;
  }
  free(ui_avivideo);
  free(ui_aviaudio);
  ui_avi = NULL;
  ui_avivideo = NULL;
  ui_aviaudio = NULL;
  ui_gtk_newoptions(); /* restore ui_frameskip */
}

GtkWidget *ui_gtk_newdiss(unsigned int type)
{
  GtkWidget *disswin;
  unsigned int i;
  GtkWidget **dw;

  disswin = create_diss();
  gtk_object_set_data(GTK_OBJECT(disswin), "generator_diss_type", 
                      GINT_TO_POINTER(type));
  gtk_object_set_data(GTK_OBJECT(disswin), "generator_diss_offset",
                      GINT_TO_POINTER(0));

  /* store in our list */
  dw = ui_disswins;
  for (i = 0; i < DISSWIN_MAX; i++, dw++) {
    if (*dw == NULL) {
      *dw = disswin;
      break;
    }
  }
  if (i == DISSWIN_MAX) {
    ui_gtk_dialog("Too many disassembly windows!");
    gtk_widget_destroy(disswin);
    return NULL;
  }
  return disswin;
}

int ui_gtk_destroydiss(GtkWidget *disswin)
{
  unsigned int i;
  GtkWidget **dw;

  dw = ui_disswins;
  for (i = 0; i < DISSWIN_MAX; i++, dw++) {
    if (*dw == disswin) {
      gtk_widget_destroy(disswin);
      *dw = NULL;
      return 0;
    }
  }
  return -1;
}

#include "hdr/diss68k.h" /* for protoype of diss68k_getdumpline() */

void ui_gtk_redrawdiss(GtkWidget *canvas, GdkEventExpose *event)
{
  GdkRectangle *rect = &event->area;
  GtkWidget *disswin;
  signed int yoff = -2;
  signed int y;
  unsigned int y_start, y_end, fontheight, i;
  uint8 *mem_where;
  uint32 mem_start;
  uint32 mem_len;
  uint32 type, offset;
  char dumpline[256];
  char *p;
  int words;

  disswin = gtk_widget_get_toplevel(canvas);
  offset = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(disswin),
                                               "generator_diss_offset"));
  type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(disswin),
                                             "generator_diss_type"));
  switch (type) {
  case 0:
    mem_where = cpu68k_rom;
    mem_start = 0;
    mem_len = cpu68k_romlen;
    break;
  case 1:
    mem_where = cpu68k_ram;
    mem_start = 0xFF0000;
    mem_len = 0x10000;
    break;
  case 2:
    mem_where = vdp_vram;
    mem_start = 0x0;
    mem_len = LEN_VRAM;
    break;
  case 3:
    mem_where = vdp_cram;
    mem_start = 0x0;
    mem_len = LEN_CRAM;
    break;
  case 4:
    mem_where = vdp_vsram;
    mem_start = 0x0;
    mem_len = LEN_VSRAM;
    break;
  case 5:
    mem_where = cpuz80_ram;
    mem_start = 0x0;
    mem_len = LEN_SRAM;
    break;
  default:
    return;
  }

  fontheight = ui_dissfont->ascent + ui_dissfont->descent;
  if ((y = rect->y - yoff) < 0)
    y = 0;
  y_start = y / fontheight;
  y_end = (y + rect->height) / fontheight;
  offset+= y_start * 2;
  for (i = y_start; i <= y_end; i++) {
    printf("offset=%X\n", offset);
    p = dumpline;
    *p++ = '>';
    *p++ = ' ';
    if (offset >= mem_len) {
    } else {
      words = diss68k_getdumpline(mem_start + offset, mem_where + offset, p);
      offset+= words * 2;
    }
    gdk_draw_rectangle(canvas->window,
                       canvas->style->bg_gc[GTK_WIDGET_STATE(canvas)],
                       TRUE, 0, i * fontheight + yoff,
                       4096, fontheight);
    gdk_draw_text(canvas->window, ui_dissfont,
                  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
                  2, i * fontheight + yoff + fontheight, dumpline,
                  strlen(dumpline));
  }

#if 0
  printf("x = %d\ny = %d\nwidth = %d\nheight = %d\noff=%f\n\n", rect->x,
    rect->y, rect->width, rect->height, adj->value);
#endif

}

/* vi: set ts=2 sw=2 et cindent: */
