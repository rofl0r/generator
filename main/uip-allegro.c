/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* user interface for allegro (15/16 bit) */

/* this part of generator is 'slaved' from the ui-console.c file - only
   platform specific code is contained here */

#include <sys/nearptr.h>

#include "generator.h"
#include <allegro.h>

#include "uip.h"
#include "ui.h"
#include "ui-console.h"
#include "vdp.h"
#include "cpu68k.h"
#include "mem68k.h"

/*** forward reference declarations ***/

/*** static variables ***/

static uint8 uip_vga = 0;       /* flag for whether in VGA more or not */
static uint8 uip_displaybanknum = 0;    /* view this one, write to other one */
static BITMAP *uip_bank0;       /* first video bank */
static BITMAP *uip_bank1;       /* second video bank */
static BITMAP *uip_writebank;   /* current write bank */
static uint8 uip_keypoll = 0;   /* flag indicating requires polling */
static t_uipinfo *uip_uipinfo = NULL;   /* uipinfo */
static int uip_forceredshift = -1;      /* if set, forces red shift position */
static int uip_forcegreenshift = -1;    /* if set, forces green shift position */
static int uip_forceblueshift = -1;     /* if set, forces blue shift position */

/*** Code ***/

static void uip_keyboardhandler(int scancode)
{
  int press;
  press = (scancode & 128) ? 0 : 1;
  scancode = scancode & 127;
  if (press) {
    if (scancode == KEY_F1) {
      ui_fkeys |= 1 << 1;
    } else if (scancode == KEY_F2) {
      ui_fkeys |= 1 << 2;
    } else if (scancode == KEY_F3) {
      ui_fkeys |= 1 << 3;
    } else if (scancode == KEY_F4) {
      ui_fkeys |= 1 << 4;
    } else if (scancode == KEY_F5) {
      ui_fkeys |= 1 << 5;
    } else if (scancode == KEY_F6) {
      ui_fkeys |= 1 << 6;
    } else if (scancode == KEY_F7) {
      ui_fkeys |= 1 << 7;
    } else if (scancode == KEY_F8) {
      ui_fkeys |= 1 << 8;
    } else if (scancode == KEY_F9) {
      ui_fkeys |= 1 << 9;
    } else if (scancode == KEY_F10) {
      ui_fkeys |= 1 << 10;
    } else if (scancode == KEY_F11) {
      ui_fkeys |= 1 << 11;
    } else if (scancode == KEY_F12) {
      ui_fkeys |= 1 << 12;
    }
  }
}

END_OF_FUNCTION(uip_keyboardhandler);

int uip_init(t_uipinfo * uipinfo)
{
  uip_uipinfo = uipinfo;
  check_cpu();
  printf("Initialising %s - %s (%d:%d:%d:%d:%d)\n", allegro_id, cpu_vendor,
         cpu_family, cpu_model, cpu_fpu ? 1 : 0, cpu_mmx ? 1 : 0,
         cpu_3dnow ? 1 : 0);
  printf("\nNote: Allegro can take at least 5 seconds to quit!\n");
  sleep(2);
  if (allegro_init()) {
    LOG_CRITICAL(("Failed to initialise allegro"));
    return 1;
  }
  LOCK_VARIABLE(ui_info);
  LOCK_VARIABLE(vdp_overseas);
  LOCK_VARIABLE(ui_vdpsimple);
  LOCK_VARIABLE(ui_vsync);
  LOCK_VARIABLE(ui_clearnext);
  LOCK_VARIABLE(ui_fullscreen);
  LOCK_VARIABLE(ui_fkeys);
  LOCK_FUNCTION(uip_keyboardhandler);
  return 0;
}

int uip_initjoysticks(void)
{
  /* joysticks not supported - yet */
  return 0;
}

int uip_vgamode(void)
{
  int depth, rate;
  unsigned long screenbase;

  for (rate = 60; rate <= 70; rate += 10) {
    for (depth = 15; depth <= 16; depth++) {
      LOG_VERBOSE(("Trying mode 640x480 depth %d rate %d", depth, rate));
      set_color_depth(depth);
      request_refresh_rate(rate);
      if ((set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 480 * 2) < 0)) {
        LOG_VERBOSE(("Mode not supported"));
        continue;
      }
      if (SCREEN_W != 640 || SCREEN_H != 480) {
        LOG_CRITICAL(("Screen not approriate for depth %d rate %d",
                      depth, rate));
        continue;
      }
      goto WHEE;
    }
  }
  LOG_CRITICAL(("Failed to find suitable mode"));
  return 1;
WHEE:
  uip_vga = 1;
  if (uip_forceredshift != -1 && uip_forcegreenshift != -1 &&
      uip_forceblueshift != -1) {
    uip_uipinfo->redshift = uip_forceredshift;
    uip_uipinfo->greenshift = uip_forcegreenshift;
    uip_uipinfo->blueshift = uip_forceblueshift;
  } else {
    uip_uipinfo->redshift = ui_topbit(makecol(255, 0, 0)) - 4;
    uip_uipinfo->greenshift = ui_topbit(makecol(0, 255, 0)) - 4;
    uip_uipinfo->blueshift = ui_topbit(makecol(0, 0, 255)) - 4;
  }
  if ((uip_bank0 = create_video_bitmap(640, 480)) == NULL ||
      (uip_bank1 = create_video_bitmap(640, 480)) == NULL) {
    uip_textmode();
    LOG_CRITICAL(("Failed to allocate memory pages"));
    return 1;
  }
  if (is_linear_bitmap(uip_bank0) == 0 ||
      is_linear_bitmap(uip_bank1) == 0 ||
      is_video_bitmap(uip_bank0) == 0 || is_video_bitmap(uip_bank1) == 0) {
    uip_textmode();
    LOG_CRITICAL(("Allocated bitmaps not suitable or linear addressing mode "
                  "not supported by hardware"));
    return 1;
  }
  /* don't you just hate MS platforms? */
  __djgpp_nearptr_enable();
  __dpmi_get_segment_base_address(uip_bank0->seg, &screenbase);
  uip_uipinfo->screenmem0 = (uint8 *)(screenbase + uip_bank0->line[0] -
                                      __djgpp_base_address);
  __dpmi_get_segment_base_address(uip_bank1->seg, &screenbase);
  uip_uipinfo->screenmem1 = (uint8 *)(screenbase + uip_bank1->line[0] -
                                      __djgpp_base_address);
  uip_uipinfo->linewidth = 2 * VIRTUAL_W;       /* 16 bit */
  uip_displaybank(0);           /* set current to 0th bank */
  uip_clearscreen();            /* clear bank */
  ui_setupscreen();             /* setup bank */
  uip_displaybank(-1);          /* toggle bank */
  uip_clearscreen();            /* clear bank */
  ui_setupscreen();             /* setup bank */
  if (install_keyboard() == -1) {
    uip_textmode();
    LOG_CRITICAL(("Unable to initialise keyboard"));
    return 1;
  }
  if (uip_bank0->y_ofs != 0 || uip_bank1->y_ofs != 480) {
    uip_textmode();
    LOG_CRITICAL(("sorry, I don't understand this video layout"));
    return 1;
  }
  keyboard_lowlevel_callback = uip_keyboardhandler;
  uip_keypoll = keyboard_needs_poll()? 1 : 0;
  return 0;
}

int uip_setcolourbits(int red, int green, int blue)
{
  uip_forceredshift = red;
  uip_forcegreenshift = green;
  uip_forceblueshift = blue;
  return 0;
}

void uip_displaybank(int bank)
{
  if (bank == -1)
    bank = uip_displaybanknum ^ 1;
  uip_writebank = bank ? uip_bank0 : uip_bank1;
  if (show_video_bitmap(bank ? uip_bank1 : uip_bank0))
    ui_err("Failed to page flip");
  uip_uipinfo->screenmem_w = bank ? uip_uipinfo->screenmem0 :
    uip_uipinfo->screenmem1;
  uip_displaybanknum = bank;
}

void uip_clearscreen(void)
{
  int y;

  for (y = 0; y < 480; y++) {
    memset(uip_uipinfo->screenmem_w + uip_uipinfo->linewidth * y, 0, 640 * 2);
  }
}

void uip_clearmiddle(void)
{
  int i;

  for (i = 0; i < 240; i++) {
    memset(uip_uipinfo->screenmem_w +
           (uip_uipinfo->linewidth) * (120 + i) + 160 * 2, 0, 2 * 320);
  }
}

/* uip_singlebank sets write address to the current display screen */

void uip_singlebank(void)
{
  uip_uipinfo->screenmem_w = uip_displaybanknum ? uip_uipinfo->screenmem1 :
    uip_uipinfo->screenmem0;
}

/* uip_doublebank sets write address to hidden display screen */

void uip_doublebank(void)
{
  uip_uipinfo->screenmem_w = uip_displaybanknum ? uip_uipinfo->screenmem0 :
    uip_uipinfo->screenmem1;
}

void uip_textmode(void)
{
  if (uip_vga) {
    remove_keyboard();
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
  }
  uip_vga = 0;
}

int uip_checkkeyboard(void)
{
  int pad;
  int joystick;

  if (uip_keypoll)
    poll_keyboard();

  for (pad = 0; pad < 2; pad++) {
    if ((joystick = ui_bindings[pad].joystick) >= 0) {
      /* not supported - yet */
    } else {
      switch (ui_bindings[pad].keyboard) {
      case 0: /* main keys */
        mem68k_cont[pad].a = key[KEY_A] ? 1 : 0;
        mem68k_cont[pad].b = (key[KEY_B] ||
                              key[KEY_S]) ? 1 : 0;
        mem68k_cont[pad].c = (key[KEY_C] ||
                              key[KEY_D]) ? 1 : 0;
        mem68k_cont[pad].left = key[KEY_LEFT] ? 1 : 0;
        mem68k_cont[pad].up = key[KEY_UP] ? 1 : 0;
        mem68k_cont[pad].right = key[KEY_RIGHT] ? 1 : 0;
        mem68k_cont[pad].down = key[KEY_DOWN] ? 1 : 0;
        mem68k_cont[pad].start = key[KEY_ENTER] ? 1 : 0;
        break;
      case 1: /* left side of keyboard */
        mem68k_cont[pad].a = key[KEY_Z] ? 1 : 0;
        mem68k_cont[pad].b = key[KEY_X] ? 1 : 0;
        mem68k_cont[pad].c = key[KEY_C] ? 1 : 0;
        mem68k_cont[pad].left = key[KEY_D] ? 1 : 0;
        mem68k_cont[pad].up = key[KEY_R] ? 1 : 0;
        mem68k_cont[pad].right = key[KEY_G] ? 1 : 0;
        mem68k_cont[pad].down = key[KEY_F] ? 1 : 0;
        mem68k_cont[pad].start = key[KEY_V] ? 1 : 0;
        break;
      case 2: /* right side of keyboard */
        mem68k_cont[pad].a = key[KEY_COMMA] ? 1 : 0;
        mem68k_cont[pad].b = key[KEY_STOP] ? 1 : 0;
        mem68k_cont[pad].c = key[KEY_SLASH] ? 1 : 0;
        mem68k_cont[pad].left = key[KEY_LEFT] ? 1 : 0;
        mem68k_cont[pad].up = key[KEY_UP] ? 1 : 0;
        mem68k_cont[pad].right = key[KEY_RIGHT] ? 1 : 0;
        mem68k_cont[pad].down = key[KEY_DOWN] ? 1 : 0;
        mem68k_cont[pad].start = key[KEY_ENTER] ? 1 : 0;
        break;
      }
    }
  }
  return (key[KEY_ESC] ? 1 : 0);
}

unsigned int uip_whichbank(void)
{
  /* returns 0 or 1 - the bank being VIEWED */
  return uip_displaybanknum;
}

void uip_vsync(void)
{
  vsync();
}

uint8 uip_getchar(void)
{
  char c;

  keyboard_lowlevel_callback = NULL;
  c = readkey() & 0xff;
  keyboard_lowlevel_callback = uip_keyboardhandler;
  return c;
}
