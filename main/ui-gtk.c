/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* gtk user interface */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "SDL.h"

#include "generator.h"
#include "snprintf.h"

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

#define HSIZE (320 + 2 * ui_hborder)
#define VSIZE ((vdp_vislines ? vdp_vislines : 224) + 2 * ui_vborder)

#define DISSWIN_MAX 64

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

const char *ui_gtk_keys[] = {
  "a", "b", "c", "start", "left", "right", "up", "down"
};

/*** static variables ***/

static SDL_Surface *screen = NULL;      /* SDL screen */

static char *ui_initload = NULL;        /* filename to load on init */
static char *ui_configfile = NULL;      /* configuration file */
static uint8 ui_plotfield = 0;  /* flag indicating plotting this field */
static uint8 ui_vdpsimple = 0;  /* 0=raster, 1=cell based plotter */
static int ui_running = 0;      /* running a game */
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
static unsigned int ui_hborder = HBORDER_DEFAULT;       /* horizontal border */
static unsigned int ui_vborder = VBORDER_DEFAULT;       /* vertical border */
static int ui_query_response = -1; /* query response */
static t_gtkkeys ui_cont[2];    /* keyboard key codes */
static int ui_musicfile = -1;   /* fd of output file for GYM/GNM logging */
static int ui_joysticks = 0;    /* number of joysticks */
static SDL_Joystick *js_handle[2] = { NULL, NULL };     /* SDL handles */
static t_aviinfo ui_aviinfo;    /* AVI information */
static t_avi *ui_avi = NULL;    /* Current AVI writer if applicable */
static uint8 *ui_avivideo;      /* video buffer */
static uint8 *ui_aviaudio;      /* audio buffer */
static GtkWidget *ui_disswins[DISSWIN_MAX];     /* disassembly windows */
static GdkFont *ui_dissfont;    /* disassembly window font */

static enum {
  SCREEN_100, SCREEN_200, SCREEN_FULL
} ui_screenmode = SCREEN_100;

t_interlace ui_interlace = DEINTERLACE_WEAVEFILTER;

/*** Forward references ***/

static void ui_usage(void);
static void ui_gtk_filesel_ok(GtkFileSelection * obj, gpointer user_data);
static void ui_gtk_sizechange(void);
static void ui_newframe(void);
static void ui_rendertoscreen(void);
static gint ui_gtk_configuremain(GtkWidget * widget,
                                 GdkEventConfigure * event);
static int ui_topbit(unsigned long int bits);
static void ui_gtk_opts_to_window(void);
static void ui_gtk_opts_from_window(void);
static int ui_gtk_query(const char *msg, int style);
static void ui_gtk_opts_to_menu(void);
static void ui_simpleplot(void);
static void ui_sdl_events(void);

/*** Program entry point ***/

int ui_init(int argc, char *argv[])
{
  int ch;
  GtkWidget *button, *draw, *obj;
  struct passwd *passwd;
  struct stat statbuf;
  int i;
  const char *name;

  gtk_set_locale();
  gtk_init(&argc, &argv);

  fprintf(stderr, "Generator is (c) James Ponder 1997-2003, all rights "
          "reserved. v" VERSION "\n\n");

  while ((ch = getopt(argc, argv, "?dc:w:")) != -1) {
    switch (ch) {
    case 'd':                  /* turn on debug mode */
      gen_debugmode = 1;
      break;
    case 'c':                  /* configuration file */
      ui_configfile = optarg;
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
    ui_configfile = malloc(strlen(passwd->pw_dir) + sizeof("/.genrc"));
    sprintf(ui_configfile, "%s/.genrc", passwd->pw_dir);
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
  gtk_widget_show(ui_win_main);
  draw = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main),
                                        "drawingarea_main"));
  gtk_signal_connect(GTK_OBJECT(ui_win_main), "configure_event",
                     GTK_SIGNAL_FUNC(ui_gtk_configuremain), 0);
  {
    char SDL_windowhack[32];
    snprintf(SDL_windowhack, sizeof(SDL_windowhack), "SDL_WINDOWID=%ld",
             GDK_WINDOW_XWINDOW(draw->window));
    putenv(SDL_windowhack);
  }
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0) {
    fprintf(stderr, "Couldn't initialise SDL: %s\n", SDL_GetError());
    return -1;
  }
  ui_joysticks = SDL_NumJoysticks();
  /* Print information about the joysticks */
  fprintf(stderr, "%d joysticks detected\n", ui_joysticks);
  for (i = 0; i < ui_joysticks; i++) {
    name = SDL_JoystickName(i);
    fprintf(stderr, "Joystick %d: %s\n", i,
                    name ? name : "Unknown Joystick");
  }
  js_handle[0] = SDL_JoystickOpen(0);
  js_handle[1] = SDL_JoystickOpen(1);
  SDL_JoystickEventState(SDL_ENABLE);

  ui_gtk_sizechange();
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
  return 0;
}

static void ui_usage(void)
{
  fprintf(stderr, "generator [options] [<rom>]\n\n");
  fprintf(stderr, "  -d                     debug mode\n");
  fprintf(stderr, "  -w <work dir>          set work directory\n");
  fprintf(stderr, "  -c <config file>       use alternative config file\n\n");
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
  char fps_string[8];
  GtkWidget *entry_fps;
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
  snprintf(fps_string, sizeof(fps_string), "%d", fps);
  entry_fps = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main),
                                             "entry_fps"));
  gtk_entry_set_text(GTK_ENTRY(entry_fps), fps_string);
  skipcount = 0;
}

/* ui_line is called for all vdp_totlines but is offset so that line=0
   is the first line, line=(vdp_totlines-vdp_visstartline)-1 is the
   last */

void ui_line(int line)
{
  static uint8 gfx[320];
  unsigned int width = (vdp_reg[12] & 1) ? 320 : 256;
  unsigned int offset = ui_hborder + ((vdp_reg[12] & 1) ? 0 : 32);
  uint8 bg;
  uint32 bgpix;
  uint8 *location;
  unsigned int i;

  if (!ui_plotfield)
    return;
  if (ui_vdpsimple) {
    if (line == (int)(vdp_vislines >> 1))
      /* if we're in simple cell-based mode, plot when half way down screen */
      ui_simpleplot();
    return;
  }
  if (line < -(int)ui_vborder || line >= (int)(vdp_vislines + ui_vborder))
    return;

  bg = (vdp_reg[7] & 63);
  uiplot_checkpalcache(0);
  bgpix = uiplot_palcache[bg];
  location = ui_newscreen +
    (line + ui_vborder) * HMAXSIZE * screen->format->BytesPerPixel;
  if (line < 0 || line >= (int)vdp_vislines) {
    /* in border */
    switch (screen->format->BytesPerPixel) {
    case 2:
      for (i = 0; i < HSIZE; i++) {
        ((uint16 *)location)[i] = bgpix;
      }
      break;
    case 4:
      for (i = 0; i < HSIZE; i++) {
        ((uint32 *)location)[i] = bgpix;
      }
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
    uiplot_convertdata16(gfx, ((uint16 *)location) + offset, width);
    for (i = 0; i < offset; i++) {
      ((uint16 *)location)[i] = bgpix;
      ((uint16 *)location)[i + offset + width] = bgpix;
    }
    break;
  case 4:
    uiplot_convertdata32(gfx, ((uint32 *)location) + offset, width);
    for (i = 0; i < offset; i++) {
      ((uint32 *)location)[i] = bgpix;
      ((uint32 *)location)[i + offset + width] = bgpix;
    }
    break;
  }
}

static void ui_simpleplot(void)
{
  static uint8 gfx[(320 + 16) * (240 + 16)];
  unsigned int width = (vdp_reg[12] & 1) ? 320 : 256;
  unsigned int offset = ui_hborder + ((vdp_reg[12] & 1) ? 0 : 32);
  unsigned int line, i;
  uint8 *location, *location2;
  uint8 bg;
  uint32 bgpix;
  /* cell mode - entire frame done here */
  bg = (vdp_reg[7] & 63);
  uiplot_checkpalcache(0);
  bgpix = uiplot_palcache[bg];
  vdp_renderframe(gfx + (8 * (320 + 16)) + 8, 320 + 16);    /* plot frame */
  location = ui_newscreen +
      ui_vborder * HMAXSIZE * screen->format->BytesPerPixel;
  switch (screen->format->BytesPerPixel) {
  case 2:
    for (line = 0; line < vdp_vislines; line++) {
      for (i = 0; i < offset; i++) {
        ((uint16 *)location)[i] = bgpix;
        ((uint16 *)location)[width + offset + i] = bgpix;
      }
      uiplot_convertdata16(gfx + 8 + (line + 8) * (320 + 16),
                           ((uint16 *)location) + offset, width);
      location += HMAXSIZE * 2;
    }
    break;
  case 4:
    for (line = 0; line < vdp_vislines; line++) {
      for (i = 0; i < offset; i++) {
        ((uint32 *)location)[i] = bgpix;
        ((uint32 *)location)[width + offset + i] = bgpix;
      }
      uiplot_convertdata32(gfx + 8 + (line + 8) * (320 + 16),
                           ((uint32 *)location) + offset, width);
      location += HMAXSIZE * 4;
    }
    break;
  }
  location = ui_newscreen;
  location2 = ui_newscreen + (ui_vborder + vdp_vislines) *
      HMAXSIZE * screen->format->BytesPerPixel;
  switch (screen->format->BytesPerPixel) {
  case 2:
    for (line = 0; line < ui_vborder; line++) {
      for (i = 0; i < HSIZE; i++) {
        ((uint16 *)location)[i] = bgpix;
        ((uint16 *)location2)[i] = bgpix;
      }
      location += HMAXSIZE * 2;
      location2 += HMAXSIZE * 2;
    }
    break;
  case 4:
    for (line = 0; line < ui_vborder; line++) {
      for (i = 0; i < HSIZE; i++) {
        ((uint32 *)location)[i] = bgpix;
        ((uint32 *)location2)[i] = bgpix;
      }
      location += HMAXSIZE * 4;
      location2 += HMAXSIZE * 4;
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
  static int counter = 0;

  if (ui_avi)
    ui_makeaviaudio();

  if (ui_plotfield)
    ui_rendertoscreen();        /* plot ui_newscreen to screen */

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
  uint8 **oldscreenpp = ui_whichbank ? &ui_screen1 : &ui_screen0;
  uint8 *scrtmp;
  uint8 *newlinedata, *oldlinedata;
  unsigned int line;
  uint8 *location;
  uint8 *evenscreen;            /* interlace: lines 0,2,etc. */
  uint8 *oddscreen;             /*            lines 1,3,etc. */

  if (ui_locksurface && SDL_LockSurface(screen) != 0)
    ui_err("Failed to lock SDL surface");
  for (line = 0; line < VSIZE; line++) {
    newlinedata = ui_newscreen +
        line * HMAXSIZE * screen->format->BytesPerPixel;
    oldlinedata = *oldscreenpp +
        line * HMAXSIZE * screen->format->BytesPerPixel;
    switch (ui_screenmode) {
    case SCREEN_100:
      location = screen->pixels + line * screen->pitch;
      /* we could use uiplot_renderXX_x1 routines here, but SDL wouldn't
         pick up our delta changes so we don't */
      memcpy(location, newlinedata, HSIZE * screen->format->BytesPerPixel);
      break;
    case SCREEN_200:
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
    case SCREEN_FULL:
    default:
      ui_err("invalid screen mode\n");
    }
  }
  if (ui_locksurface)
    SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
  if (ui_avi)
    ui_makeavivideo();
  ui_whichbank ^= 1;
//  if (ui_vsync)
//    uip_vsync();
  /* swap ui_screenX and ui_newscreen pointers */
  scrtmp = *oldscreenpp;
  *oldscreenpp = ui_newscreen;
  ui_newscreen = scrtmp;
}

/*** logging functions ***/

/* logging is done this way because this was the best I could come up with
   whilst battling with macros that can only take fixed numbers of arguments */

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

void ui_err(const char *text, ...)
{
  va_list ap;

  printf("ERROR: ");

  va_start(ap, text);
  vfprintf(stderr, text, ap);
  va_end(ap);
  putc(10, stderr);
  exit(0);
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
  char *file, *p;
  int fd;
  struct stat s;
  int avi_skip, avi_jpeg;

  (void)obj;
  (void)user_data;
  file = gtk_file_selection_get_filename(GTK_FILE_SELECTION(ui_win_filesel));
  gtk_widget_hide(ui_win_filesel);

  if (ui_filesel_save) {
    if (stat(file, &s) == 0) {
      snprintf(msg, sizeof(msg), "The file '%s' already exists - overwrite?",
               file);
      if (ui_gtk_query(msg, 0) != 0)
        return;
    }
  }
  *msg = '\0';

  switch ((ui_filesel_type << 1) | ui_filesel_save) {
  case 0:
    p = gen_loadimage(file);
    if (p)
      snprintf(msg, sizeof(msg), "An error occured whilst tring to load "
               "the ROM '%s': %s", file, p);
    break;
  case 1:
    if (cpu68k_rom == NULL || cpu68k_romlen == 0) {
      snprintf(msg, sizeof(msg), "There is no ROM currently in memory to "
               "save!");
      break;
    }
    if (gen_modifiedrom) {
      if (ui_gtk_query("The ROM in memory has been modified by cheat "
                       "codes - are you really sure you want to save"
                      "this ROM?", 0) != 0)
        break;
    }
    if ((fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0777)) == -1
        || write(fd, cpu68k_rom, cpu68k_romlen) == -1 || close(fd) == -1) {
      snprintf(msg, sizeof(msg), "An error occured whilst trying to save the "
               "ROM as '%s': %s", file, strerror(errno));
    }
    break;
  case 2:
    if (stat(file, &s) != 0) {
      snprintf(msg, sizeof(msg), "%s: %s", file, strerror(errno));
      break;
    }
    if (state_loadfile(file) != 0)
      snprintf(msg, sizeof(msg), "An error occured whilst trying to load "
               "state from '%s': %s", file, strerror(errno));
    break;
  case 3:
    if (state_savefile(file) != 0)
      snprintf(msg, sizeof(msg), "An error occured whilst trying to save "
               "state to '%s': %s", file, strerror(errno));
    break;
  case 5:
    if (ui_musicfile != -1) {
      snprintf(msg, sizeof(msg), "There is already a music log in progress");
      break;
    }
    if ((ui_musicfile = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0777)) == -1) {
      snprintf(msg, sizeof(msg), "An error occured whilst trying to start "
               "a GYM log to '%s': %s", file, strerror(errno));
    }
    gen_musiclog = 1;
    break;
  case 7:
    if (ui_musicfile != -1) {
      snprintf(msg, sizeof(msg), "There is already a music log in progress");
      break;
    }
    if ((ui_musicfile = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0777)) == -1) {
      snprintf(msg, sizeof(msg), "An error occured whilst trying to start "
               "a GNM log to '%s': %s", file, strerror(errno));
    }
    buf[0] = 'G'; buf[1] = 'N'; buf[2] = 'M';
    buf[3] = vdp_framerate;
    write(ui_musicfile, buf, 4);
    gen_musiclog = 2;
    break;
  case 8:
    if (stat(file, &s) != 0) {
      snprintf(msg, sizeof(msg), "%s: %s", file, strerror(errno));
      break;
    }
    if (patch_loadfile(file) != 0)
      snprintf(msg, sizeof(msg), "An error occured whilst trying to load "
               "state from '%s': %s", file, strerror(errno));
    break;
  case 9:
    if (patch_savefile(file) != 0)
      snprintf(msg, sizeof(msg), "An error occured whilst trying to save "
               "state to '%s': %s", file, strerror(errno));
    break;
  case 11:
    if (ui_avi) {
      snprintf(msg, sizeof(msg), "There is already an avi in progress");
      break;
    }
    avi_skip = atoi(gtkopts_getvalue("aviframeskip"));
    avi_jpeg = !strcasecmp(gtkopts_getvalue("aviformat"), "jpeg");
    ui_aviinfo.width = HSIZE;
    ui_aviinfo.height = VSIZE;
    ui_aviinfo.sampspersec = sound_speed;
    ui_aviinfo.fps = (vdp_framerate * 1000) / avi_skip;
    ui_aviinfo.jpegquality = atoi(gtkopts_getvalue("jpegquality"));
    if ((ui_avi = avi_open(file, &ui_aviinfo, avi_jpeg)) == NULL) {
      snprintf(msg, sizeof(msg), "An error occured whilst trying to start "
               "the AVI: %s", strerror(errno));
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
  ui_gtk_sizechange();
  if (gtk_main_level() > 0)
    gtk_main_quit();
}

void ui_gtk_pause(void)
{
  if (!ui_running) {
    ui_gtk_dialog("Generator is not playing a ROM");
    return;
  }
  ui_running = 0;
}

void ui_gtk_softreset(void)
{
  gen_softreset();
}

void ui_gtk_hardreset(void)
{
  gen_reset();
}

/* set main window size from current parameters */

void ui_gtk_sizechange(void)
{
  GtkWidget *w;

  w = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main),
                                     "drawingarea_main"));
  switch (ui_screenmode) {
  case SCREEN_100:
    gtk_widget_set_usize(w, HSIZE, VSIZE);
    screen = SDL_SetVideoMode(HSIZE, VSIZE, 0, 0);
    break;
  case SCREEN_200:
    gtk_widget_set_usize(w, HSIZE * 2, VSIZE * 2);
    screen = SDL_SetVideoMode(HSIZE * 2, VSIZE * 2, 0, 0);
    break;
  default:
    ui_err("invalid screen mode\n");
  }
  w = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main), "hbox_bottom"));
  if (ui_statusbar)
    gtk_widget_show(w);
  else
    gtk_widget_hide(w);
  ui_locksurface = SDL_MUSTLOCK(screen);
  uiplot_setshifts(ui_topbit(screen->format->Rmask) - 4,
                   ui_topbit(screen->format->Gmask) - 4,
                   ui_topbit(screen->format->Bmask) - 4);
}

void ui_gtk_newoptions(void)
{
  char buf[32];
  const char *v;
  int i, c;
  unsigned int old_ui_screenmode = ui_screenmode;
  unsigned int old_sound_minfields = sound_minfields;
  unsigned int old_sound_maxfields = sound_maxfields;
  unsigned int old_ui_hborder = ui_hborder;
  unsigned int old_ui_vborder = ui_vborder;
  unsigned int old_ui_statusbar = ui_statusbar;

  i = atoi(gtkopts_getvalue("view"));
  ui_screenmode = (i == 200) ? SCREEN_200 : SCREEN_100;

  v = gtkopts_getvalue("region");
  vdp_overseas = !strcasecmp(v, "overseas");

  v = gtkopts_getvalue("videostd");
  vdp_pal = !strcasecmp(v, "pal");

  v = gtkopts_getvalue("autodetect");
  gen_autodetect = !strcasecmp(v, "on");

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

  v = gtkopts_getvalue("loglevel");
  gen_loglevel = atoi(v);

  v = gtkopts_getvalue("statusbar");
  ui_statusbar = !strcasecmp(v, "on");

  for (c = 0; c < 2; c++) {
    snprintf(buf, sizeof(buf), "key%d_a", c + 1);
    v = gtkopts_getvalue(buf);
    ui_cont[c].a = gdk_keyval_from_name(v);
    snprintf(buf, sizeof(buf), "key%d_b", c + 1);
    v = gtkopts_getvalue(buf);
    ui_cont[c].b = gdk_keyval_from_name(v);
    snprintf(buf, sizeof(buf), "key%d_c", c + 1);
    v = gtkopts_getvalue(buf);
    ui_cont[c].c = gdk_keyval_from_name(v);
    snprintf(buf, sizeof(buf), "key%d_up", c + 1);
    v = gtkopts_getvalue(buf);
    ui_cont[c].up = gdk_keyval_from_name(v);
    snprintf(buf, sizeof(buf), "key%d_down", c + 1);
    v = gtkopts_getvalue(buf);
    ui_cont[c].down = gdk_keyval_from_name(v);
    snprintf(buf, sizeof(buf), "key%d_left", c + 1);
    v = gtkopts_getvalue(buf);
    ui_cont[c].left = gdk_keyval_from_name(v);
    snprintf(buf, sizeof(buf), "key%d_right", c + 1);
    v = gtkopts_getvalue(buf);
    ui_cont[c].right = gdk_keyval_from_name(v);
    snprintf(buf, sizeof(buf), "key%d_start", c + 1);
    v = gtkopts_getvalue(buf);
    ui_cont[c].start = gdk_keyval_from_name(v);
  }
  if (ui_screenmode != old_ui_screenmode ||
      ui_hborder != old_ui_hborder ||
      ui_vborder != old_ui_vborder || ui_statusbar != old_ui_statusbar)
    ui_gtk_sizechange();

  if (sound_minfields != old_sound_minfields ||
      sound_maxfields != old_sound_maxfields) {
    sound_reset();
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
    obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                         "hscale_skip"));
    snprintf(buf, sizeof(buf), "%d",
             (int)(gtk_range_get_adjustment(GTK_RANGE(obj))->value));
    gtkopts_setvalue("frameskip", buf);
  }

  /* video - hborder */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_hborder"));
  snprintf(buf, sizeof(buf), "%d",
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("hborder", buf);

  /* video - vborder */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_vborder"));
  snprintf(buf, sizeof(buf), "%d",
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
  snprintf(buf, sizeof(buf), "%d",
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("sound_minfields", buf);

  /* sound - max fields */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_maxfields"));
  snprintf(buf, sizeof(buf), "%d",
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("sound_maxfields", buf);

  /* sound - filter */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_filter"));
  snprintf(buf, sizeof(buf), "%d",
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("lowpassfilter", buf);

  /* logging - verbosity */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "optionmenu_level"));
  obj = GTK_OPTION_MENU(obj)->menu;
  /* now get active widget on menu */
  active = gtk_menu_get_active(GTK_MENU(obj));
  snprintf(buf, sizeof(buf), "%d",
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
  snprintf(buf, sizeof(buf), "%d",
           (int)(gtk_range_get_adjustment(GTK_RANGE(obj))->value));
  gtkopts_setvalue("aviframeskip", buf);

  /* logging - jpeg quality */

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts),
                                       "spinbutton_jpegquality"));
  snprintf(buf, sizeof(buf), "%d",
           gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(obj)));
  gtkopts_setvalue("jpegquality", buf);

  /* controls */

  for (c = 1; c <= 2; c++) {
    for (key = ui_gtk_keys;
         (char *)key < ((char *)ui_gtk_keys + sizeof(ui_gtk_keys));
         key++) {
      snprintf(buf, sizeof(buf), "entry_player%d_%s", c, *key);
      obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_opts), buf));
      v = gtk_entry_get_text(GTK_ENTRY(obj));
      snprintf(buf, sizeof(buf), "key%d_%s", c, *key);
      gtkopts_setvalue(buf, v);
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

  for (c = 1; c <= 2; c++) {
    for (key = ui_gtk_keys;
         (char *)key < ((char *)ui_gtk_keys + sizeof(ui_gtk_keys));
         key++) {
      snprintf(buf, sizeof(buf), "key%d_%s", c, *key);
      v = gtkopts_getvalue(buf);
      i = gdk_keyval_from_name(v);
      if (i) {
        v = gdk_keyval_name(i); /* just incase case is different etc. */
      } else {
        v = "";
      }
      snprintf(buf, sizeof(buf), "entry_player%d_%s", c, *key);
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
  char buf[256];

  ui_gtk_opts_from_window();
  if (gtkopts_save(ui_configfile) != 0) {
    snprintf(buf, sizeof(buf), "Failed to save configuration to '%s': %s",
             ui_configfile, strerror(errno));
    ui_gtk_dialog(buf);
    return;
  }
  gtk_widget_hide(ui_win_opts);
  ui_gtk_newoptions();
  ui_gtk_opts_to_menu();
}

void ui_gtk_redraw(void)
{
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
}

static void ui_sdl_events (void)
{
	SDL_Event event;
  #define	LEFT(event)	((event.jaxis.value < -16384) ? 1 : 0)
  #define	RIGHT(event)	((event.jaxis.value > 16384) ? 1 : 0)
  #define	UP(event)	((event.jaxis.value < -16384) ? 1 : 0)
  #define	DOWN(event)	((event.jaxis.value > 16384) ? 1 : 0)
  #define	PRESS(event)	((event.type == SDL_JOYBUTTONDOWN) ? 1 : 0)

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
			if (event.jbutton.which > 1)
				break;
			switch (event.jbutton.button) {
			case 0:
				mem68k_cont[event.jbutton.which].a = PRESS(event);
				break;
			case 1:
				mem68k_cont[event.jbutton.which].b = PRESS(event);
				break;
			case 2:
				mem68k_cont[event.jbutton.which].c = PRESS(event);
				break;
			case 3:
				mem68k_cont[event.jbutton.which].start = PRESS(event);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
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

  v = gtkopts_getvalue("view");
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main), "_100"));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(obj),
                                 !strcasecmp(v, "100"));
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_main), "_200"));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(obj),
                                 !strcasecmp(v, "200"));
}

void ui_musiclog(uint8 *data, unsigned int length)
{
  if (ui_musicfile != -1)
    write(ui_musicfile, data, length);
}

void ui_gtk_closemusiclog(void)
{
  if (ui_musicfile == -1) {
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
    data[1] = ent->action;
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
  GtkWidget *obj;
  char *data[2];
  char *code, *action;

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "entry_code"));
  code = gtk_entry_get_text(GTK_ENTRY(obj));
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "entry_action"));
  action = gtk_entry_get_text(GTK_ENTRY(obj));
  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  data[0] = (char *)code;
  data[1] = (char *)action;
  gtk_clist_append(GTK_CLIST(obj), data);
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
  char *code, *action;

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  patch_clearlist();
  for (i = 0;; i++) {
    if (gtk_clist_get_text(GTK_CLIST(obj), i, 0, &code) != 1 ||
        gtk_clist_get_text(GTK_CLIST(obj), i, 1, &action) != 1)
      break;
    patch_addcode(code, action);
  }
  gtk_widget_hide(ui_win_codes);
}

void ui_gtk_codes_apply(void)
{
  GtkWidget *obj;
  GList *slist;
  char *code, *action;
  int failed = 0;

  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(ui_win_codes),
                                       "clist_codes"));
  slist = GTK_CLIST(obj)->selection;
  for (; slist; slist = slist->next) {
    if (gtk_clist_get_text(GTK_CLIST(obj), (int)slist->data, 0, &code) != 1 ||
        gtk_clist_get_text(GTK_CLIST(obj), (int)slist->data, 1, &action) != 1) {
      failed = 1;
      break;
    }
    if (patch_apply(code, action) != 0)
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
  GtkWidget *disswin, *obj;
  GtkStyle *style;
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

//{
//  GtkWidget *adj, *scrollbar;
//
//  scrollbar = lookup_widget(disswin, "vscrollbar_diss");
//  adj = gtk_range_get_adjustment(GTK_RANGE(scrollbar));
//}
//scrollbar = lookup_widget(disswin, "vscrollbar_diss");
//gtk_signal_connect(GTK_OBJECT(GTK_RANGE(scrollbar)->adjustment),
//                   "changed", GTK_SIGNAL_FUNC(on_adjustment_changed), NULL);

void ui_gtk_redrawdiss(GtkWidget *canvas, GdkEventExpose *event)
{
  GdkRectangle *rect = &event->area;
  GtkWidget *obj, *disswin;
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
  /*
  printf("x = %d\ny = %d\nwidth = %d\nheight = %d\noff=%f\n\n", rect->x,
  rect->y, rect->width, rect->height, adj->value);
  */


}

//void ui_gtk_redrawdiss(GtkWidget *canvas, GdkEventExpose *event)
//{
//  GdkRectangle *rect = &event->area;
//  GtkWidget *obj, *disswin;
//  GtkAdjustment *adj;
//  signed int y, ypos;
//  unsigned int y_start, y_end, fontheight, i;
//
//  disswin = gtk_widget_get_toplevel(canvas);
//  obj = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(disswin), 
//                                       "scrolledwindow_diss"));
//  adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(obj));
//
//  fontheight = ui_dissfont->ascent + ui_dissfont->descent;
//  if ((y = rect->y - 2) < 0)
//    y = 0;
//  printf("y = %d\n", y);
//  y_start = (y + (int)adj->value) / fontheight;
//  y_end = (y + (int)adj->value + rect->height) / fontheight;
//  printf("val = %f start = %d height = %d\n", adj->value, y_start, fontheight);
//  ypos = -adj->value + y_start * fontheight;
//  printf("< %d %d >\n", y_start, y_end);
//  for (i = y_start; i < y_end; i++, ypos+= fontheight) {
//    gdk_draw_text(canvas->window, ui_dissfont,
//                  cavas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
//                  0, ypos);
//
//    printf("[%d] %d\n", i, ypos);
//  }
//  /*
//  printf("x = %d\ny = %d\nwidth = %d\nheight = %d\noff=%f\n\n", rect->x,
//  rect->y, rect->width, rect->height, adj->value);
//  */
//
//
//}
