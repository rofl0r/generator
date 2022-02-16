/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include "generator.h"
#include "vdp.h"
#include "cpu68k.h"
#include "cpuz80.h"
#include "reg68k.h"
#include "ui.h"
#include "gensound.h"

inline void event_nextevent(void)
{
  switch(vdp_event++) {
  case 0:
    LOG_DEBUG1(("%08X due %08X, %d A: %d (cd=%d)",
                cpu68k_clocks, vdp_event_start,
                vdp_line - vdp_visstartline, vdp_reg[10],
                vdp_hskip_countdown));
    if (vdp_line == (vdp_visstartline-1)) {
      vdp_vblank = 0;
    }
    if ((vdp_nextevent = vdp_event_vint - cpu68k_clocks) > 0)
      break;
    vdp_event++;
  case 1:
    LOG_DEBUG1(("%08X due %08X, %d B: %d (cd=%d)",
                cpu68k_clocks, vdp_event_vint,
                vdp_line - vdp_visstartline, vdp_reg[10],
                vdp_hskip_countdown));
    if (vdp_line == vdp_visendline) {
      vdp_vblank = 1;
      vdp_vsync = 1;
      if (vdp_reg[1] & 1<<5)
        reg68k_external_autovector(6); /* vertical interrupt */
    }
    if ((vdp_nextevent = vdp_event_hint - cpu68k_clocks) > 0)
      break;
    vdp_event++;
  case 2:
    LOG_DEBUG1(("%08X due %08X, %d C: %d (cd=%d)",
                cpu68k_clocks, vdp_event_hint,
                vdp_line - vdp_visstartline, vdp_reg[10],
                vdp_hskip_countdown));
    if (vdp_line >= vdp_visstartline && vdp_line < vdp_visendline)
      vdp_hblank = 1;
    if (vdp_line == (vdp_visstartline-1) || (vdp_line > vdp_visendline)) {
      vdp_hskip_countdown = vdp_reg[10];
      LOG_DEBUG1(("H counter reset to %d", vdp_hskip_countdown));
    }
    if (vdp_reg[0] & 1<<4) {
      LOG_DEBUG1(("pre = %d", vdp_hskip_countdown));
      if (vdp_hskip_countdown-- == 0) {
        LOG_DEBUG1(("in = %d", vdp_hskip_countdown));
        /* re-initialise counter */
        vdp_hskip_countdown = vdp_reg[10];
        LOG_DEBUG1(("H counter looped to %d", vdp_hskip_countdown));
        if (vdp_line >= vdp_visstartline-1 && vdp_line < vdp_visendline-1)
          reg68k_external_autovector(4); /* horizontal interrupt */
        /* since this game is obviously timing sensitive, we sacrifice
           executing the right number of 68k clocks this frame in order
           to accurately do the moments following H-Int */
        cpu68k_clocks = vdp_event_hint;
      }
      LOG_DEBUG1(("post = %d", vdp_hskip_countdown));
    }
    /* the 68k should be frozen for 68k ram to vram copies, so we need
       to eat CPU cyles immediately - implement this at the same time
       as re-working this event loop */
    if (vdp_dmabytes) {
      vdp_dmabytes-= (vdp_vblank || !(vdp_reg[1] & 1<<6)) /* blank mode ? */
        ? ((vdp_reg[12] & 1) ? 205 : 167) : ((vdp_reg[12] & 1) ? 18 : 16);
      if (vdp_dmabytes <= 0) {
        vdp_dmabytes = 0;
        vdp_dmabusy = 0;
      }
    }
    if ((vdp_nextevent = vdp_event_hdisplay - cpu68k_clocks) > 0)
      break;
    vdp_event++;
  case 3:
    LOG_DEBUG1(("%08X due %08X, %d D: %d (cd=%d)",
                cpu68k_clocks, vdp_event_hdisplay,
                vdp_line - vdp_visstartline, vdp_reg[10],
                vdp_hskip_countdown));
    if (vdp_line >= vdp_visstartline-1 && vdp_line < vdp_visendline-1)
      ui_line(vdp_line - vdp_visstartline + 1);
    if ((vdp_nextevent = vdp_event_end - cpu68k_clocks) > 0)
      break;
    vdp_event++;
  case 4:
    /* end of line, do sound, platform stuff */
    LOG_DEBUG1(("%08X due %08X, %d E: %d (cd=%d)",
                cpu68k_clocks, vdp_event_end,
                vdp_line-vdp_visstartline, vdp_reg[10],
                vdp_hskip_countdown));
    if (vdp_line >= vdp_visstartline && vdp_line < vdp_visendline)
      vdp_hblank = 0;
    cpuz80_sync();
    sound_process();
    vdp_line++;
    if (vdp_line == vdp_totlines) {
      ui_endfield();
      sound_endfield();
      cpuz80_interrupt();
      vdp_endfield();
      cpuz80_endfield();
      cpu68k_endfield();
      cpu68k_frames++;
    }
    vdp_event_start+= vdp_clksperline_68k;
    vdp_event_vint+= vdp_clksperline_68k;
    vdp_event_hint+= vdp_clksperline_68k;
    vdp_event_hdisplay+= vdp_clksperline_68k;
    vdp_event_end+= vdp_clksperline_68k;
    vdp_event = 0;
    vdp_nextevent = 0;
    break;
  } /* switch */
}

/*** event_doframe - execute until the end of the current frame ***/

void event_doframe(void)
{
  unsigned int startframe;

  for (startframe = cpu68k_frames;
       startframe == cpu68k_frames;
       event_nextevent()) {
    while (vdp_nextevent > 0) {
      vdp_nextevent = -reg68k_external_execute(vdp_nextevent);
    }
  }
}

/*** event_step - execute one instruction with no caching ***/

void event_dostep(void)
{
  while (vdp_nextevent <= 0)
    event_nextevent();
  vdp_nextevent-= reg68k_external_step();
}

/*** event_freeze_clocks - freeze 68k for given clock cycles ***/

void event_freeze_clocks(unsigned int clocks)
{
  cpu68k_clocks+= clocks;

  /* now find out the new time to next event - just to make sure there is
     an event to be generated by inreasing the clock */

  switch(vdp_event) {
  case 0:
    vdp_nextevent = vdp_event_start - cpu68k_clocks;
  case 1:
    vdp_nextevent = vdp_event_vint - cpu68k_clocks;
    break;
  case 2:
    vdp_nextevent = vdp_event_hint - cpu68k_clocks;
    break;
  case 3:
    vdp_nextevent = vdp_event_hdisplay - cpu68k_clocks;
    break;
  case 4:
    vdp_nextevent = vdp_event_end - cpu68k_clocks;
    break;
  } /* switch */

  /* now play catch-up in events */

  while (vdp_nextevent <= 0)
    event_nextevent();
}

/*** event_freeze - freeze 68k for given VDP byte transfer ***/

void event_freeze(unsigned int bytes)
{
  int wide = vdp_reg[12] & 1;
  unsigned int clocks, possible;
  double percent_possible;

  do {
    clocks = vdp_event_end - cpu68k_clocks;
    percent_possible = clocks/vdp_clksperline_68k;
    if (vdp_reg[1] & 1<<6 && !vdp_vblank) {
      /* vdp active */
      possible = (unsigned int)(percent_possible*(wide ? 18 : 16));
    } else {
      /* vdp inactive */
      possible = (unsigned int)(percent_possible*(wide ? 205 : 167));
    }
    if (bytes >= possible) {
      event_freeze_clocks(clocks);
      bytes-= possible;
    } else {
      event_freeze_clocks(((double)bytes/possible)*clocks);
      bytes = 0;
    }
  } while (bytes > 0);
}
