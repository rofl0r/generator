/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2000       */
/*****************************************************************************/
/*                                                                           */
/* uip-allegro - User interface for allegro (15/16 bit)                      */
/*                                                                           */
/*****************************************************************************/

#include <sys/nearptr.h>
#include <stdio.h>

#include "generator.h"
#include <allegro.h>

#include "uip.h"
#include "ui.h"
#include "ui-console.h"
#include "vdp.h"
#include "mem68k.h"

/*** forward reference declarations ***/

/*** static variables ***/

static uint8 uip_vga = 0;             /* flag for whether in VGA more or not */
static uint8 uip_displaybanknum = 0;  /* view this one, write to other one */
static BITMAP *uip_bank0;             /* first video bank */
static BITMAP *uip_bank1;             /* second video bank */
static BITMAP *uip_writebank;         /* current write bank */
static uint8 uip_keypoll = 0;         /* flag indicating requires polling */
static t_uipinfo *uip_uipinfo = NULL; /* uipinfo */

/*** Code ***/

static void uip_keyboardhandler(int scancode) {
  int press;
  press = (scancode & 128) ? 0 : 1;
  scancode = scancode & 127;
  if (press) {
    if (scancode == KEY_F5) {
      ui_info^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_F6) {
      vdp_pal^= 1;
      vdp_setupvideo();
      ui_clearnext = 2;
    } else if (scancode == KEY_F7) {
      vdp_overseas^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_F8) {
      ui_vdpsimple^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_F9) {
      ui_vsync^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_1) {
      vdp_layerB^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_2) {
      vdp_layerBp^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_3) {
      vdp_layerA^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_4) {
      vdp_layerAp^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_5) {
      vdp_layerW^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_6) {
      vdp_layerWp^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_7) {
      vdp_layerH^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_8) {
      vdp_layerS^= 1;
      ui_clearnext = 2;
    } else if (scancode == KEY_9) {
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

END_OF_FUNCTION(uip_keyboardhandler);

int uip_init(t_uipinfo *uipinfo)
{
  uip_uipinfo = uipinfo;
  check_cpu();
  printf("Initialising %s - %s (%d:%d:%d:%d:%d)\n", allegro_id, cpu_vendor,
	 cpu_family, cpu_model, cpu_fpu?1:0, cpu_mmx?1:0, cpu_3dnow?1:0);
  printf("\nNote: Allegro can take at least 5 seconds to quit!\n");
  sleep(2);
  if (allegro_init()) {
    LOG_CRITICAL(("Failed to initialise allegro"));
    return 1;
  }
  LOCK_VARIABLE(ui_info);
  LOCK_VARIABLE(vdp_pal);
  LOCK_VARIABLE(vdp_overseas);
  LOCK_VARIABLE(ui_vdpsimple);
  LOCK_VARIABLE(ui_vsync);
  LOCK_VARIABLE(ui_clearnext);
  LOCK_VARIABLE(vdp_layerB);
  LOCK_VARIABLE(vdp_layerBp);
  LOCK_VARIABLE(vdp_layerA);
  LOCK_VARIABLE(vdp_layerAp);
  LOCK_VARIABLE(vdp_layerW);
  LOCK_VARIABLE(vdp_layerWp);
  LOCK_VARIABLE(vdp_layerH);
  LOCK_VARIABLE(vdp_layerS);
  LOCK_VARIABLE(vdp_layerSp);
  LOCK_FUNCTION(uip_keyboardhandler);
  return 0;
}

int uip_vgamode(void)
{
  int depth, rate;
  unsigned long screenbase;

  for (rate = 60; rate <= 70; rate+=10) {
    for (depth = 15; depth <= 17; depth++) {
      LOG_VERBOSE(("Trying mode 640x480 depth %d rate %d", depth, rate));
      set_color_depth((depth == 17 ? 32 : depth));
      request_refresh_rate(rate);
      if ((set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 480*2) < 0)) {
	LOG_VERBOSE(("Mode not supported"));
	continue;
      }
      if (SCREEN_W != 640 || SCREEN_H != 480) {
	LOG_CRITICAL(("Screen not approriate for depth %d rate %d", depth,
		      rate));
	continue;
      }
      goto WHEE;
    }
  }
  LOG_CRITICAL(("Failed to find suitable mode"));
  return 1;
 WHEE:
  uip_vga = 1;
  uip_uipinfo->redshift   = ui_topbit(makecol(255,0,0))-4;
  uip_uipinfo->greenshift = ui_topbit(makecol(0,255,0))-4;
  uip_uipinfo->blueshift  = ui_topbit(makecol(0,0,255))-4;
  if ((uip_bank0 = create_video_bitmap(640, 480)) == NULL ||
      (uip_bank1 = create_video_bitmap(640, 480)) == NULL) {
    uip_textmode();
    LOG_CRITICAL(("Failed to allocate memory pages"));
    return 1;
  }
  if (is_linear_bitmap(uip_bank0) == 0 ||
      is_linear_bitmap(uip_bank1) == 0 ||
      is_video_bitmap(uip_bank0) == 0 ||
      is_video_bitmap(uip_bank1) == 0) {
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
  uip_uipinfo->linewidth = 2*VIRTUAL_W; /* 16 bit */
  uip_displaybank(0); /* set current to 0th bank */
  uip_clearscreen(); /* clear bank */
  ui_setupscreen(); /* setup bank */
  uip_displaybank(-1); /* toggle bank */
  uip_clearscreen(); /* clear bank */
  ui_setupscreen(); /* setup bank */
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
  uip_keypoll = keyboard_needs_poll() ? 1 : 0;
  return 0;
}

void uip_displaybank(int bank) {
  if (bank == -1)
    bank = uip_displaybanknum ^ 1;
  uip_writebank = bank ? uip_bank0 : uip_bank1;
  if (show_video_bitmap(bank ? uip_bank1 : uip_bank0))
    ui_err("Failed to page flip");
  uip_uipinfo->screenmem_w = bank ? uip_uipinfo->screenmem0 :
    uip_uipinfo->screenmem1;
  uip_displaybanknum = bank;
}

void uip_clearscreen(void) {
  clear(uip_writebank);
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
  if (uip_keypoll)
    poll_keyboard();
  mem68k_cont1_a = key[KEY_A] ? 1 : 0;
  mem68k_cont1_b = (key[KEY_B] ||
		    key[KEY_S]) ? 1 : 0;
  mem68k_cont1_c = (key[KEY_C] ||
		    key[KEY_D]) ? 1 : 0;
  mem68k_cont1_left = key[KEY_LEFT] ? 1 : 0;
  mem68k_cont1_up = key[KEY_UP] ? 1 : 0;
  mem68k_cont1_right = key[KEY_RIGHT] ? 1 : 0;
  mem68k_cont1_down = key[KEY_DOWN] ? 1 : 0;
  mem68k_cont1_start = key[KEY_ENTER] ? 1 : 0;
  if (key[KEY_F12])
    gen_reset();
  return (key[KEY_ESC] ? 1 : 0);
}

void uip_vsync(void)
{
  vsync();
}
