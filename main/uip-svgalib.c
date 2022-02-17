/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* user interface for svgalib (15/16 bit) */

/* this part of generator is 'slaved' from the ui-console.c file - only
   platform specific code is contained here */

#include "generator.h"

#include <vga.h>
#include <vgakeyboard.h>
#include <vgajoystick.h>

#include "uip.h"
#include "ui.h"
#include "ui-console.h"
#include "vdp.h"
#include "cpu68k.h"
#include "mem68k.h"

#define BANKSIZE 640*480*2

/*** forward reference declarations ***/

/*** static variables ***/

static uint8 uip_vga = 0;       /* flag for whether in VGA mode */
static uint8 uip_key[256];      /* keyboard state */
static uint8 uip_displaybanknum = 0;    /* view this one, write to other one */
static t_uipinfo *uip_uipinfo = NULL;   /* uipinfo */
static int uip_forceredshift = -1;      /* if set, forces red shift pos */
static int uip_forcegreenshift = -1;    /* if set, forces green shift pos */
static int uip_forceblueshift = -1;     /* if set, forces blue shift pos */

/*** Code ***/

static void uip_keyboardhandler(int scancode, int press)
{
  if (scancode >= 256)
    return;
  if (uip_key[scancode] == press)
    return;
  uip_key[scancode] = press;
  if (press) {
    if (scancode == SCANCODE_F1) {
      ui_fkeys |= 1 << 1;
    } else if (scancode == SCANCODE_F2) {
      ui_fkeys |= 1 << 2;
    } else if (scancode == SCANCODE_F3) {
      ui_fkeys |= 1 << 3;
    } else if (scancode == SCANCODE_F4) {
      ui_fkeys |= 1 << 4;
    } else if (scancode == SCANCODE_F5) {
      ui_fkeys |= 1 << 5;
    } else if (scancode == SCANCODE_F6) {
      ui_fkeys |= 1 << 6;
    } else if (scancode == SCANCODE_F7) {
      ui_fkeys |= 1 << 7;
    } else if (scancode == SCANCODE_F8) {
      ui_fkeys |= 1 << 8;
    } else if (scancode == SCANCODE_F9) {
      ui_fkeys |= 1 << 9;
    } else if (scancode == SCANCODE_F10) {
      ui_fkeys |= 1 << 10;
    } else if (scancode == SCANCODE_F11) {
      ui_fkeys |= 1 << 11;
    } else if (scancode == SCANCODE_F12) {
      ui_fkeys |= 1 << 12;
    }
  }
}

int uip_init(t_uipinfo * uipinfo)
{
  uip_uipinfo = uipinfo;
  LOG_REQUEST(("Initialising svgalib..."));
  sleep(1);
  if (vga_init()) {
    LOG_CRITICAL(("Failed to initialise SVGALib"));
    return 1;
  }
  return 0;
}

int uip_initjoysticks(void)
{
  LOG_REQUEST(("Checking for svgalib joysticks..."));
  if (joystick_init(0, JOY_CALIB_STDOUT) < 0) {
    LOG_VERBOSE(("Joystick 0 not found"));
    return 0;
  }
  LOG_NORMAL(("Joystick 0 initialised"));
  if (joystick_init(1, JOY_CALIB_STDOUT) < 0) {
    LOG_VERBOSE(("Joystick 1 not found"));
    return 1;
  }
  LOG_NORMAL(("Joystick 1 initialised"));
  return 2;
}

int uip_vgamode(void)
{
  vga_modeinfo *modeinfo;

  uip_vga = 1;
  if (vga_setmode(G640x480x32K) == -1) {
    LOG_CRITICAL(("640x480 @ 32K colours not supported by hardware"));
    LOG_CRITICAL(("Trying 640x480 @ 64K colours..."));
    if (vga_setmode(G640x480x64K) == -1) {
      LOG_CRITICAL(("640x480 @ 64K colours not supported by hardware"));
      uip_textmode();
      LOG_CRITICAL(("Unable to change SVGA mode, mode not supported by "
                    "hardware?"));
      LOG_CRITICAL(("Check /etc/vga/libvga.config to ensure your card "
                    "is not commented out"));
      return 1;
    }
  }
  if (vga_claimvideomemory(2 * BANKSIZE) == -1) {
    uip_textmode();
    LOG_CRITICAL(("Unable to allocated video memory - 2MB video card "
                  "required."));
    return 1;
  }
  if ((modeinfo = vga_getmodeinfo(vga_getcurrentmode())) == NULL) {
    uip_textmode();
    LOG_CRITICAL(("Unable to get current mode information"));
    return 1;
  }
  LOG_VERBOSE(("Resolution %dx%d bytesperpixel %d linewidth %d",
               modeinfo->width, modeinfo->height, modeinfo->bytesperpixel,
               modeinfo->linewidth));
  if (modeinfo->width != 640 || modeinfo->height != 480 ||
      modeinfo->bytesperpixel != 2 || modeinfo->linewidth * 480 > BANKSIZE ||
      (modeinfo->startaddressrange & BANKSIZE) != BANKSIZE) {
    uip_textmode();
    LOG_CRITICAL(("Unexpected mode settings"));
    return 1;
  }
  uip_uipinfo->linewidth = modeinfo->linewidth;
  if (vga_setlinearaddressing() == -1) {
    uip_textmode();
    LOG_CRITICAL(("Linear addressing mode not supported by hardware"));
    return 1;
  }
  if ((uip_uipinfo->screenmem0 = vga_getgraphmem()) == NULL) {
    uip_textmode();
    LOG_CRITICAL(("Failed to find start of screen memory"));
    return 1;
  }
  uip_uipinfo->screenmem1 = uip_uipinfo->screenmem0 + BANKSIZE;
  if (uip_forceredshift != -1 && uip_forcegreenshift != -1 &&
      uip_forceblueshift != -1) {
    uip_uipinfo->redshift = uip_forceredshift;
    uip_uipinfo->greenshift = uip_forcegreenshift;
    uip_uipinfo->blueshift = uip_forceblueshift;
  } else {
    switch (modeinfo->colors) {
    case 32768:
      LOG_VERBOSE(("Assuming colour bit positions 10,5,0 for 32k colour mode"));
      uip_uipinfo->blueshift = 0;
      uip_uipinfo->greenshift = 5;
      uip_uipinfo->redshift = 10;
      break;
    case 65536:
      LOG_VERBOSE(("Assuming colour bit positions 11,6,0 for 64k colour mode"));
      uip_uipinfo->blueshift = 0;
      uip_uipinfo->greenshift = 6;
      uip_uipinfo->redshift = 11;
      break;
    default:
      LOG_CRITICAL(("Unknown mode - %d colours?? use -c", modeinfo->colors));
      return 1;
    }
  }
  uip_displaybank(0);           /* set current to 0th bank */
  uip_clearscreen();            /* clear bank */
  ui_setupscreen();             /* setup bank */
  uip_displaybank(-1);          /* toggle bank */
  uip_clearscreen();            /* clear bank */
  ui_setupscreen();             /* setup bank */
  if (keyboard_init() == -1) {
    uip_textmode();
    LOG_CRITICAL(("Unable to initialise keyboard"));
    return 1;
  }
  memset(uip_key, 0, sizeof(uip_key));
  keyboard_seteventhandler(uip_keyboardhandler);
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
  vga_setdisplaystart(bank ? BANKSIZE : 0);
  uip_uipinfo->screenmem_w = bank ? uip_uipinfo->screenmem0 :
    uip_uipinfo->screenmem1;
  uip_displaybanknum = bank;
}

void uip_clearscreen(void)
{
  memset(uip_uipinfo->screenmem_w, 0, BANKSIZE);
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
    keyboard_setdefaulteventhandler();
    vga_setmode(0);
  }
  uip_vga = 0;
}

int uip_checkkeyboard(void)
{
  int x, y;
  int pad;
  int joystick;

  joystick_update();
  keyboard_update();
  for (pad = 0; pad < 2; pad++) {
    if ((joystick = ui_bindings[pad].joystick) >= 0) {
      x = joystick_getaxis(joystick, 0);
      y = joystick_getaxis(joystick, 1);
      mem68k_cont[pad].a = joystick_getbutton(joystick, 0);
      mem68k_cont[pad].b = joystick_getbutton(joystick, 1);
      mem68k_cont[pad].c = joystick_getbutton(joystick, 2);
      mem68k_cont[pad].start = joystick_getbutton(joystick, 3);
      mem68k_cont[pad].left = x < -64 ? 1 : 0;
      mem68k_cont[pad].right = x > 64 ? 1 : 0;
      mem68k_cont[pad].up = y < -64 ? 1 : 0;
      mem68k_cont[pad].down = y > 64 ? 1 : 0;
    } else {
      switch (ui_bindings[pad].keyboard) {
      case 0: /* main keys */
        mem68k_cont[pad].a = uip_key[SCANCODE_A] ? 1 : 0;
        mem68k_cont[pad].b = (uip_key[SCANCODE_B] ||
                              uip_key[SCANCODE_S]) ? 1 : 0;
        mem68k_cont[pad].c = (uip_key[SCANCODE_C] ||
                              uip_key[SCANCODE_D]) ? 1 : 0;
        mem68k_cont[pad].left = uip_key[SCANCODE_CURSORBLOCKLEFT] ? 1 : 0;
        mem68k_cont[pad].up = uip_key[SCANCODE_CURSORBLOCKUP] ? 1 : 0;
        mem68k_cont[pad].right = uip_key[SCANCODE_CURSORBLOCKRIGHT] ? 1 : 0;
        mem68k_cont[pad].down = uip_key[SCANCODE_CURSORBLOCKDOWN] ? 1 : 0;
        mem68k_cont[pad].start = uip_key[SCANCODE_ENTER] ? 1 : 0;
        break;
      case 1: /* left side of keyboard */
        mem68k_cont[pad].a = uip_key[SCANCODE_Z] ? 1 : 0;
        mem68k_cont[pad].b = uip_key[SCANCODE_X] ? 1 : 0;
        mem68k_cont[pad].c = uip_key[SCANCODE_C] ? 1 : 0;
        mem68k_cont[pad].left = uip_key[SCANCODE_D] ? 1 : 0;
        mem68k_cont[pad].up = uip_key[SCANCODE_R] ? 1 : 0;
        mem68k_cont[pad].right = uip_key[SCANCODE_G] ? 1 : 0;
        mem68k_cont[pad].down = uip_key[SCANCODE_F] ? 1 : 0;
        mem68k_cont[pad].start = uip_key[SCANCODE_V] ? 1 : 0;
        break;
      case 2: /* right side of keyboard */
        mem68k_cont[pad].a = uip_key[SCANCODE_COMMA] ? 1 : 0;
        mem68k_cont[pad].b = uip_key[SCANCODE_PERIOD] ? 1 : 0;
        mem68k_cont[pad].c = uip_key[SCANCODE_SLASH] ? 1 : 0;
        mem68k_cont[pad].left = uip_key[SCANCODE_CURSORBLOCKLEFT] ? 1 : 0;
        mem68k_cont[pad].up = uip_key[SCANCODE_CURSORBLOCKUP] ? 1 : 0;
        mem68k_cont[pad].right = uip_key[SCANCODE_CURSORBLOCKRIGHT] ? 1 : 0;
        mem68k_cont[pad].down = uip_key[SCANCODE_CURSORBLOCKDOWN] ? 1 : 0;
        mem68k_cont[pad].start = uip_key[SCANCODE_ENTER] ? 1 : 0;
        break;
      }
    }
  }
  return (uip_key[SCANCODE_ESCAPE] ? 1 : 0);
}

void uip_vsync(void)
{
  vga_waitretrace();
}

unsigned int uip_whichbank(void)
{
  /* returns 0 or 1 - the bank being VIEWED */
  return uip_displaybanknum;
}

uint8 uip_getchar(void)
{
  char c;

  keyboard_setdefaulteventhandler();
  keyboard_close();
  c = getchar();
  keyboard_init();
  memset(uip_key, 0, sizeof(uip_key));
  keyboard_seteventhandler(uip_keyboardhandler);
  return c;
}
