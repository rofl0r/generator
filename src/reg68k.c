/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2000       */
/*****************************************************************************/
/*                                                                           */
/* reg68k.c                                                                  */
/*                                                                           */
/*****************************************************************************/

#include "generator.h"
#include "registers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

#include "mem68k.h"
#include "cpu68k.h"
#include "cpuz80.h"
#include "vdp.h"
#include "ui.h"
#include "compile.h"
#include "gensound.h"

/*** externed variables ***/

#if (!(defined(PROCESSOR_ARM) || defined(PROCESSOR_SPARC) \
       || defined(PROCESSOR_INTEL)))
  uint32 reg68k_pc;
  uint32 *reg68k_regs;
  t_sr reg68k_sr;
#endif

/*** forward references ***/

inline void reg68k_checkevent(void);
void reg68k_autovector(int avno);

static unsigned int reg68k_eventdelay;

void reg68k_step(void)
{
  /* PAL: 312 lines, 224 or 240 visible @ 50Hz - CPU @ 7.60Mhz */
  /* NTSC: 262 lines, 224 visible @ 60Hz - CPU @ 7.67Mhz */

  static t_ipc ipc;
  static t_iib *piib;
  jmp_buf jb;

  /* !!! entering global register usage area !!! */

  if (!setjmp(jb)) {
    if (!(piib = cpu68k_iibtable[fetchword(regs.pc)]))
      ui_err("Invalid instruction @ %08X\n", regs.pc);
    /* move PC and register block into global variables */
    reg68k_pc = regs.pc;
    reg68k_regs = regs.regs;
    reg68k_sr = regs.sr;

    cpu68k_ipc(reg68k_pc,
	       mem68k_memptr[(reg68k_pc>>12) & 0xfff](reg68k_pc & 0xFFFFFF),
	       piib, &ipc);
    cpu68k_functable[fetchword(reg68k_pc)*2+1](&ipc);
    cpu68k_clocks+= piib->clocks;
    reg68k_checkevent();
    /* restore global registers back to permanent storage */
    regs.pc = reg68k_pc;
    regs.sr = reg68k_sr;
    longjmp(jb, 1);
  }
}

inline void reg68k_checkevent(void)
{
  switch(vdp_event) {
  case 0:
    if (cpu68k_clocks >= vdp_event_start) {
      vdp_event_startofcurrentline = vdp_event_start;
      LOG_USER(("%08X due %08X, %d A: %d (cd=%d)",
		cpu68k_clocks, vdp_event_start, vdp_line, vdp_reg[10],
		vdp_hskip_countdown));
      vdp_event++;
      vdp_event_start+= vdp_clksperline_68k;
      cpuz80_sync();
      if (vdp_line == 0) {
	vdp_vblank = 0;
	vdp_hskip_countdown = vdp_reg[10];
	ui_line(0);
      }
      if (vdp_line < vdp_vislines)
	ui_line(vdp_line);
    }
    break;
  case 1:
    if (cpu68k_clocks >= vdp_event_vint) {
      LOG_USER(("%08X due %08X, %d B: %d (cd=%d)",
		cpu68k_clocks, vdp_event_vint, vdp_line, vdp_reg[10],
		vdp_hskip_countdown));
      vdp_event++;
      vdp_event_vint+= vdp_clksperline_68k;
      if (vdp_line == vdp_vislines) {
	vdp_vblank = 1;
	vdp_vsync = 1;
	if (vdp_reg[1] & 1<<5)
	  reg68k_autovector(6); /* vertical interrupt */
      }
    }
    break;
  case 2:
    if (cpu68k_clocks >= vdp_event_hint) {
      LOG_USER(("%08X due %08X, %d C: %d (cd=%d)",
		cpu68k_clocks, vdp_event_hint, vdp_line, vdp_reg[10],
		vdp_hskip_countdown));
      /* delay hdisplay if hint is delayed */
      vdp_event++;
      vdp_event_hint+= vdp_clksperline_68k;
      cpuz80_sync();
      /* if (vdp_line < vdp_vislines) */
	vdp_hblank = 1;
      if (vdp_reg[0] & 1<<4) {
	if (vdp_hskip_countdown-- == 0) {
	  /* re-initialise counter */
	  vdp_hskip_countdown = vdp_reg[10];
	  if (vdp_line < vdp_vislines)
	    reg68k_autovector(4); /* horizontal interrupt */
	  /* since this game is obviously timing sensitive, we sacrifice
	     executing the right number of 68k clocks this frame in order
	     to accurately do the moments following H-Int */
	  cpu68k_clocks = vdp_event_hint - vdp_clksperline_68k;
	}
      }
    }
    break;
  case 3:
    if (cpu68k_clocks >= vdp_event_hdisplay) {
      LOG_USER(("%08X due %08X, %d D: %d (cd=%d)",
		cpu68k_clocks, vdp_event_hdisplay, vdp_line, vdp_reg[10],
		vdp_hskip_countdown));
      vdp_event_hdisplay-= reg68k_eventdelay;
      vdp_event++;
      vdp_event_hdisplay+= vdp_clksperline_68k;
      cpuz80_sync();
      /* err
      if (vdp_line+1 < vdp_vislines)
	ui_line(vdp_line+1);
      */
    }
    break;
  case 4:
    if (cpu68k_clocks >= vdp_event_end) {
      /* end of line, do sound, platform stuff */
      LOG_USER(("%08X due %08X, %d E: %d (cd=%d)",
		cpu68k_clocks, vdp_event_end, vdp_line, vdp_reg[10],
		vdp_hskip_countdown));
      vdp_event = 0;
      vdp_event_end+= vdp_clksperline_68k;
      /* if (vdp_line < vdp_vislines) */
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
    }
    break;
  } /* switch */
}

/*** reg68k_framestep - do a frame and return ***/

void reg68k_framestep(void)
{
  /* PAL: 312 lines, 224 or 240 visible @ 50Hz - CPU @ 7.60Mhz */
  /* NTSC: 262 lines, 224 visible @ 60Hz - CPU @ 7.67Mhz */

  unsigned int startframe = cpu68k_frames;
  unsigned int index, i;
  t_ipclist *list;
  t_ipc *ipc;
  uint32 pc24;
  jmp_buf jb;
  static t_ipc step_ipc;
  static t_iib *step_piib;

  if (!setjmp(jb)) {
    /* move PC and register block into global variables */
    reg68k_pc = regs.pc;
    reg68k_regs = regs.regs;
    reg68k_sr = regs.sr;

    do {
      pc24 = reg68k_pc & 0xffffff;
      if ((pc24 & 0xff0000) == 0xff0000) {
	/* executing code from RAM, do not use compiled information */
	do {
	  step_piib = cpu68k_iibtable[fetchword(reg68k_pc)];
	  cpu68k_ipc(reg68k_pc,
		     mem68k_memptr[(reg68k_pc>>12) &
				  0xfff](reg68k_pc & 0xFFFFFF),
		     step_piib, &step_ipc);
	  cpu68k_functable[fetchword(reg68k_pc)*2+1](&step_ipc);
	  cpu68k_clocks+= step_piib->clocks;
	} while (!step_piib->flags.endblk);
	list = NULL; /* stop compiler warning ;(  */
      } else {
	index = (pc24>>1) & (LEN_IPCLISTTABLE-1);
	list = ipclist[index];
	while(list && (list->pc != pc24)) {
	  list = list->next;
	}
#ifdef PROCESSOR_ARM
	if (!list) {
	  list = cpu68k_makeipclist(pc24);
	  list->next = ipclist[index];
	  ipclist[index] = list;
	  list->compiled = compile_make(list);
	}
	list->compiled((t_ipc *)(list+1));
#else
	if (!list) {
	  /* LOG_USER(("Making IPC list @ %08x", pc24)); */
	  list = cpu68k_makeipclist(pc24);
	  list->next = ipclist[index];
	  ipclist[index] = list;
	}
	ipc = (t_ipc *)(list+1);
	do {
	  ipc->function(ipc);
	  ipc++;
	} while (*(int *)ipc);
#endif
	cpu68k_clocks+= list->clocks;
      }
      reg68k_checkevent();
    } while (cpu68k_frames == startframe);
    /* restore global registers back to permanent storage */
    regs.pc = reg68k_pc;
    regs.sr = reg68k_sr;
    longjmp(jb, 1);
  }
}

void reg68k_autovector(int avno)
{
  int curlevel = (reg68k_sr.sr_int>>8) & 7;
  uint32 tmpaddr;

  LOG_VERBOSE(("autovector %d\n", avno));
  if (curlevel < avno || avno == 7) {
    if (!reg68k_sr.sr_struct.s) {
      regs.regs[15]^= regs.sp;   /* swap A7 and SP */
      regs.sp^= regs.regs[15];
      regs.regs[15]^= regs.sp;
      reg68k_sr.sr_struct.s = 1;
    }
    regs.regs[15]-=4;
    storelong(regs.regs[15], reg68k_pc);
    regs.regs[15]-=2;
    storeword(regs.regs[15], reg68k_sr.sr_int);
    reg68k_sr.sr_struct.t = 0;
    reg68k_sr.sr_int&= ~0x0700;
    reg68k_sr.sr_int|= avno << 8;
    tmpaddr = reg68k_pc;
    reg68k_pc = fetchlong((V_AUTO+avno-1)*4);
    LOG_DEBUG1(("AUTOVECTOR %d: %X -> %X", avno, tmpaddr, reg68k_pc));
  }
}

void reg68k_vector(int vno, uint32 oldpc)
{
  LOG_DEBUG1(("Vector %d called from %8x!", vno, regs.pc));
  if (!reg68k_sr.sr_struct.s) {
    regs.regs[15]^= regs.sp;   /* swap A7 and SP */
    regs.sp^= regs.regs[15];
    regs.regs[15]^= regs.sp;
    reg68k_sr.sr_struct.s = 1;
  }
  regs.regs[15]-=4;
  storelong(regs.regs[15], oldpc);
  regs.regs[15]-=2;
  storeword(regs.regs[15], reg68k_sr.sr_int);
  reg68k_pc = fetchlong(vno*4);
  LOG_DEBUG1(("VECTOR %d: %X -> %X\n", vno, oldpc, reg68k_pc));
}   
