/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2000       */
/*****************************************************************************/
/*                                                                           */
/* uip-svgalib - User interface for svgalib (15 bit)                         */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <unistd.h>

#include "generator.h"
#include <vga.h>
#include <vgakeyboard.h>

#include "uip.h"
#include "ui.h"
#include "ui-console.h"
#include "vdp.h"
#include "mem68k.h"

#define BANKSIZE 640*480*2

/*** forward reference declarations ***/

/*** static variables ***/

static uint8 uip_vga = 0;             /* flag for whether in VGA more or not */
static uint8 uip_displaybanknum = 0;  /* view this one, write to other one */
static t_uipinfo *uip_uipinfo = NULL; /* uipinfo */
static uint8 uip_key[256];            /* keyboard state */

/*** Code ***/

static void uip_keyboardhandler(int scancode, int press) {
  if (scancode >= 256)
    return;
  if (uip_key[scancode] == press)
    return;
  uip_key[scancode] = press;
  if (press) {
    if (scancode == SCANCODE_F5) {
      ui_info^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_F6) {
      vdp_pal^= 1;
      vdp_setupvideo();
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_F7) {
      vdp_overseas^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_F8) {
      ui_vdpsimple^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_F9) {
      ui_vsync^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_1) {
      vdp_layerB^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_2) {
      vdp_layerBp^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_3) {
      vdp_layerA^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_4) {
      vdp_layerAp^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_5) {
      vdp_layerW^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_6) {
      vdp_layerWp^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_7) {
      vdp_layerH^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_8) {
      vdp_layerS^= 1;
      ui_clearnext = 2;
    } else if (scancode == SCANCODE_9) {
      vdp_layerSp^= 1;
      ui_clearnext = 2;
    }
      /*
      {
	FILE *moo;
	int i;
	moo = fopen("/tmp/screenshot.raw", "w");
	for (i = 0; i < 640*480; i++) {
	  fputc(((((uint16 *)uip_uipinfo.screenmem)[i]>>10) & 0x1f)<<3, moo);
	  fputc(((((uint16 *)uip_uipinfo.screenmem)[i]>>5) & 0x1f)<<3, moo);
	  fputc(((((uint16 *)uip_uipinfo.screenmem)[i]>>0) & 0x1f)<<3, moo);
	}
	fclose(moo);
      }
      */
  }
}

int uip_init(t_uipinfo *uipinfo)
{
  uip_uipinfo = uipinfo;
  printf("Initialising svgalib...\n");
  sleep(1);
  if (vga_init()) {
    LOG_CRITICAL(("Failed to initialise SVGALib"));
    return 1;
  }
  return 0;
}

int uip_vgamode(void)
{
  vga_modeinfo *modeinfo;
  int cardmemory;

  uip_vga = 1;
  if (vga_setmode(G640x480x32K) == -1) {
    uip_textmode();
    LOG_CRITICAL(("Unable to change SVGA mode, mode not supported by "
		  "hardware?"));
    LOG_CRITICAL(("Check /etc/vga/libvga.config to ensure your card "
		  "is not commented out"));
    return 1;
  }
  if (vga_claimvideomemory(2*BANKSIZE) == -1) {
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
      modeinfo->bytesperpixel != 2 || modeinfo->linewidth*480 > BANKSIZE ||
      (modeinfo->startaddressrange & BANKSIZE) != BANKSIZE) {
    uip_textmode();
    LOG_CRITICAL(("Unexpected mode settings"));
    return 1;
  }
  uip_uipinfo->linewidth = modeinfo->linewidth;
  uip_uipinfo->redshift = 0;
  uip_uipinfo->greenshift = 5;
  uip_uipinfo->redshift = 10;
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
  uip_uipinfo->screenmem1 = uip_uipinfo->screenmem0+BANKSIZE;
  uip_displaybank(0); /* set current to 0th bank */
  uip_clearscreen(); /* clear bank */
  ui_setupscreen(); /* setup bank */
  uip_displaybank(-1); /* toggle bank */
  uip_clearscreen(); /* clear bank */
  ui_setupscreen(); /* setup bank */
  if (keyboard_init() == -1) {
    uip_textmode();
    LOG_CRITICAL(("Unable to initialise keyboard"));
    return 1;
  }
  memset(uip_key, 0, sizeof(uip_key));
  keyboard_seteventhandler(uip_keyboardhandler);
  return 0;
}

void uip_displaybank(int bank) {
  if (bank == -1)
    bank = uip_displaybanknum ^ 1;
  vga_setdisplaystart(bank ? BANKSIZE : 0);
  uip_uipinfo->screenmem_w = bank ? uip_uipinfo->screenmem0 :
    uip_uipinfo->screenmem1;
  uip_displaybanknum = bank;
}

void uip_clearscreen(void) {
  memset(uip_uipinfo->screenmem_w, 0, BANKSIZE);
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
  keyboard_update();
  mem68k_cont1_a = uip_key[SCANCODE_A] ? 1 : 0;
  mem68k_cont1_b = (uip_key[SCANCODE_B] ||
		    uip_key[SCANCODE_S]) ? 1 : 0;
  mem68k_cont1_c = (uip_key[SCANCODE_C] ||
		    uip_key[SCANCODE_D]) ? 1 : 0;
  mem68k_cont1_left = uip_key[SCANCODE_CURSORBLOCKLEFT] ? 1 : 0;
  mem68k_cont1_up = uip_key[SCANCODE_CURSORBLOCKUP] ? 1 : 0;
  mem68k_cont1_right = uip_key[SCANCODE_CURSORBLOCKRIGHT] ? 1 : 0;
  mem68k_cont1_down = uip_key[SCANCODE_CURSORBLOCKDOWN] ? 1 : 0;
  mem68k_cont1_start = uip_key[SCANCODE_ENTER] ? 1 : 0;
  if (uip_key[SCANCODE_F12])
    gen_reset();
  return (uip_key[SCANCODE_ESCAPE] ? 1 : 0);
}

void uip_vsync(void)
{
  vga_waitretrace();
}
