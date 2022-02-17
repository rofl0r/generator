/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

/* user interface for X tcl/tk */

#include "generator.h"

#ifdef HAVE_TCL8_0_H
#  include <tcl8.0.h>
#else
#  include <tcl.h>
#endif

#ifdef HAVE_TK8_0_H
#  include <tk8.0.h>
#else
#  include <tk.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "diss68k.h"
#include "cpu68k.h"
#include "cpuz80.h"
#include "ui.h"
#include "vdp.h"
#include "mem68k.h"
#include "event.h"
#include "state.h"

#ifdef XF86DGA
#include <X11/extensions/xf86dga.h>
#endif

/*
#ifndef XInitImage
#define XInitImage _XInitImageFuncPtrs
Status _XInitImageFuncPtrs(XImage *);
#endif
*/

#define MAXDEPTHBYTES 4

/*** forward reference declarations ***/

int ui_loadimage(Tcl_Interp * interp, const char *filename);
int ui_loadsavepos(Tcl_Interp * interp, const char *filename);
int ui_redrawdump(Tcl_Interp * interp, const char *textwidget);
void ui_updateregs(void);
void ui_updatestr(char *var, char *val);
void ui_updateint(char *var, int val);
int ui_getint(int *cint, char *tclint);
int ui_setstr(const char *name, const char *value);
void ui_keypress(ClientData cd, XEvent * event);
void ui_enterleave(ClientData cd, XEvent * event);
int ui_topbit(unsigned long int bits);
void ui_endframe(void);
void ui_convertdata(uint8 *indata, uint8 *outdata, unsigned int pixels,
                    unsigned int lineoffset, unsigned int scale,
                    unsigned int depth, unsigned int basepixel);
void ui_convertdata_smooth(uint8 *indata, uint8 *outdata,
                           unsigned int pixels, unsigned int lineoffset,
                           unsigned int depth);
static char *ui_setinfo(t_cartinfo * cartinfo);

#ifdef XF86DGA
void ui_fullscreen(int onoff);
#endif

/*** static variables ***/

static Tk_Window mainwin;
static Bool xsync = False;
static Bool save = False;
static Tcl_Interp *interp;
static char *imagedata;
static XImage *image;
static GC gc = NULL;
static unsigned int scale = 1;
static unsigned int smooth = 0;
static Visual *visual;
static Colormap cmap;
static unsigned long visual_bluemask;
static unsigned long visual_redmask;
static unsigned long visual_greenmask;
static int visual_bluepos;
static int visual_redpos;
static int visual_greenpos;
static unsigned int basepixel;
static unsigned int depth;
static unsigned int frameskip = 1;
static Display *ui_display = NULL;
static Screen *ui_screen = NULL;
static int ui_screenno = 0;
static int ui_dga = 0;          /* enable/disable availability flag */
static int ui_dga_state = 0;    /* DGA on/off */
static uint32 ui_palcache[192];
static int ui_vdpsimple = 1;    /* simple vdp enable/disable */
static char *ui_initload = NULL;        /* filename to load on init */
static unsigned int state = 0;  /* 0=stop,1=pause,2=play */
static int ui_musicfile = -1;   /* fd of output file for GYM/GNM logging */

#ifdef XF86DGA
static int dga_major, dga_minor;
static int dga_eventb, dga_errorb;
static int dga_flags;
static char *dga_baseaddr;
static int dga_width, dga_banksize, dga_memsize;
static int dga_xsize, dga_ysize;
static char *dga_start;
#endif

/*** Define our own arguments for Tk to process ***/

static Tk_ArgvInfo argtable[] = {
  /*  { "-display", TK_ARGV_STRING, (char*) NULL, (char*) &display,
     "Display to use" }, */
  {"-sync", TK_ARGV_CONSTANT, (char *)True, (char *)&xsync,
   "Turn on synchronous X"},
  {"-save", TK_ARGV_CONSTANT, (char *)True, (char *)&save,
   "Save ROM as bin"},
  {"", TK_ARGV_END, (char *)NULL, (char *)NULL, (char *)NULL}
};

/*** Error handler for X protocol errors ***/

static int errorhandler(ClientData data, XErrorEvent * err)
{
  (void)data;
  ui_err("X error %d, request %d, minor %d", err->error_code,
         err->request_code, err->minor_code);
  return 0;
}

/*** Gen_Load type pathname ***/

int
Gen_Load(ClientData cdata, Tcl_Interp * interp, int objc,
         Tcl_Obj * const objv[])
{
  int i;

  (void)cdata;
  if (objc != 3) {
    Tcl_SetResult(interp,
                  "wrong # args: should be \"Gen_Load type pathName\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (Tcl_GetIntFromObj(interp, objv[1], &i) != TCL_OK)
    return TCL_ERROR;
  switch (i) {
  case 0:
    return ui_loadimage(interp, Tcl_GetStringFromObj(objv[2], NULL));
  case 1:
    return ui_loadsavepos(interp, Tcl_GetStringFromObj(objv[2], NULL));
  }
  Tcl_SetResult(interp, "invalid type: should be image (0) or savepos (1)\n",
                TCL_STATIC);
  return TCL_ERROR;
}

/*** Gen_Save pathname - saves the current state of play in a savegame ***/

int
Gen_Save(ClientData cdata, Tcl_Interp * interp, int objc,
         Tcl_Obj * const objv[])
{
  (void)cdata;
  if (objc != 2) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_Save pathName\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  LOG_NORMAL(("Save! %s\n", Tcl_GetStringFromObj(objv[1], NULL)));
  return TCL_OK;
}

/*** Gen_Dump widgetname yview <params from scrollbar> ***/

int
Gen_Dump(ClientData cdata, Tcl_Interp * interp, int argc, const char *argv[])
{
  int si_line, memtype, dumptype, offset, lines;
  double si_fraction;
  char tmp[256];
  Tcl_Obj *varobj, *outobj;
  Tk_Window window;
  uint8 *mem_where;
  unsigned int mem_start, mem_len;

  (void)cdata;
  if (!cpu68k_rom) {
    Tcl_SetResult(interp, "Nothing to display", TCL_STATIC);
    return TCL_ERROR;
  }

  window = Tk_NameToWindow(interp, argv[1], Tk_MainWindow(interp));
  if (!window) {
    Tcl_SetResult(interp, "bad widget passed to Gen_Dump", TCL_STATIC);
    return TCL_ERROR;
  }

  /* get memtype variable */
  sprintf(tmp, "%s.memtype", argv[1]);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if (!(outobj = Tcl_ObjGetVar2(interp, varobj, NULL, TCL_GLOBAL_ONLY))) {
    Tcl_SetResult(interp, ".memtype variable not set", TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_IncrRefCount(outobj);
  if (Tcl_GetIntFromObj(interp, outobj, &memtype) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(outobj);

  /* get dumptype variable */
  sprintf(tmp, "%s.dumptype", argv[1]);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if (!(outobj = Tcl_ObjGetVar2(interp, varobj, NULL, TCL_GLOBAL_ONLY))) {
    Tcl_SetResult(interp, ".dumptype variable not set", TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_IncrRefCount(outobj);
  if (Tcl_GetIntFromObj(interp, outobj, &dumptype) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(outobj);

  /* get offset variable */
  sprintf(tmp, "%s.offset", argv[1]);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if (!(outobj = Tcl_ObjGetVar2(interp, varobj, NULL, TCL_GLOBAL_ONLY))) {
    Tcl_SetResult(interp, ".offset variable not set", TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_IncrRefCount(outobj);
  if (Tcl_GetIntFromObj(interp, outobj, &offset) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(outobj);

  /* get lines variable */
  sprintf(tmp, "%s.lines", argv[1]);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if (!(outobj = Tcl_ObjGetVar2(interp, varobj, NULL, TCL_GLOBAL_ONLY))) {
    Tcl_SetResult(interp, ".lines variable not set", TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_IncrRefCount(outobj);
  if (Tcl_GetIntFromObj(interp, outobj, &lines) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(outobj);

  switch (memtype) {
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
  default:                     /* 0 */
    mem_where = cpu68k_rom;
    mem_start = 0;
    mem_len = cpu68k_romlen;
    break;
  }

  if (!strcasecmp(argv[2], "yview")) {
    switch (Tk_GetScrollInfo(interp, argc - 1, &argv[1],
              &si_fraction, &si_line)) {
    case TK_SCROLL_MOVETO:
      offset = (int)(si_fraction * mem_len) & (~1);
      break;
    case TK_SCROLL_UNITS:
      if (offset < 256 && (si_line > 0)) {
        offset = (si_line * 4) + offset;
        offset &= ~3;
      } else if ((offset <= 256) && (si_line < 0)) {
        offset = (si_line * 4) + offset;
        offset &= ~3;
      } else {
        offset = (si_line * 2) + offset;
      }
      break;
    case TK_SCROLL_PAGES:
      offset = si_line * lines * 2 + offset;
      break;
    default:
      Tcl_SetResult(interp, "unknown yview command parameter", TCL_STATIC);
      return TCL_ERROR;
    }
  } else if (!strcasecmp(argv[2], "redraw")) {
    return ui_redrawdump(interp, argv[1]);
  } else {
    Tcl_SetResult(interp, "unknown command type (must be yview or redraw)",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (offset >= (signed int)mem_len)
    offset = mem_len - 2;
  if (offset < 0)
    offset = 0;
  sprintf(tmp, "%s.offset", argv[1]);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if (Tcl_ObjSetVar2(interp, varobj, NULL, Tcl_NewIntObj(offset),
                     TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);
  ui_redrawdump(interp, argv[1]);
  return TCL_OK;
}

/*** Gen_Step - step through one instruction ***/

int
Gen_Step(ClientData cdata, Tcl_Interp * interp, int objc,
         Tcl_Obj * const objv[])
{
  (void)cdata;
  (void)objv;
  if (objc != 1) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_Step\"", TCL_STATIC);
    return TCL_ERROR;
  }
  event_dostep();
  ui_updateregs();
  return TCL_OK;
}

/*** Gen_Cont - start executing ***/

int
Gen_Cont(ClientData cdata, Tcl_Interp * interp, int objc,
         Tcl_Obj * const objv[])
{
  char *stopstr;
  uint32 stopaddr;
  Tcl_Obj *obj;
  int i;

  (void)cdata;
  (void)objv;
  if (objc != 1) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_Cont\"", TCL_STATIC);
    return TCL_ERROR;
  }
  obj = Tcl_ObjGetVar2(interp, Tcl_NewStringObj("regs.stop", -1), NULL,
                       TCL_GLOBAL_ONLY);
  stopstr = Tcl_GetStringFromObj(obj, NULL);
  if (!stopstr) {
    Tcl_SetResult(interp, "Cannot read regs.stop", TCL_STATIC);
    return TCL_ERROR;
  }
  stopaddr = strtol(stopstr, NULL, 16);
  gen_quit = 0;
  LOG_NORMAL(("Continuing from %08X", regs.pc));
  do {
    while (Tcl_DoOneEvent(TCL_ALL_EVENTS | TCL_DONT_WAIT));
    for (i = 0; i < 128; i++) {
      event_dostep();
      if ((regs.pc & 0xFFFFFF) == stopaddr || gen_quit)
        break;
    }
  }
  while ((regs.pc & 0xFFFFFF) != stopaddr && !gen_quit);
  LOG_NORMAL(("End of continuation, stopped at %08X", regs.pc));
  ui_updateregs();
  if (gen_quit)
    LOG_NORMAL(("Stopped."));
  gen_quit = 0;
  return TCL_OK;
}

/*** Gen_FrameStep - step through one frame ***/

int
Gen_FrameStep(ClientData cdata, Tcl_Interp * interp, int objc,
              Tcl_Obj * const objv[])
{
  (void)cdata;
  (void)objv;
  if (objc != 1) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_FrameStep\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  gen_quit = 0;
  do {
    event_doframe();
    while (Tcl_DoOneEvent(TCL_ALL_EVENTS | TCL_DONT_WAIT));
  }
  while (!gen_quit);
  ui_updateregs();
  if (gen_quit)
    LOG_NORMAL(("Stopped."));
  gen_quit = 0;

  return TCL_OK;
}

/*** Gen_SpriteList - print sprite information ***/

int
Gen_SpriteList(ClientData cdata, Tcl_Interp * interp, int objc,
               Tcl_Obj * const objv[])
{
  (void)cdata;
  (void)objv;
  if (objc != 1) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_SpriteList\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  vdp_spritelist();
  return TCL_OK;
}

/*** Gen_Change - there has been a change in parameter ***/

int
Gen_Change(ClientData cdata, Tcl_Interp * interp, int objc,
           Tcl_Obj * const objv[])
{
  unsigned int oldscale = scale;
  unsigned int newstate;

  (void)cdata;
  (void)objv;
  if (objc != 1) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_Change\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }

  ui_getint((int *)&ui_vdpsimple, "vdpsimple");
  ui_getint((int *)&vdp_layerB, "layerB");
  ui_getint((int *)&vdp_layerBp, "layerBp");
  ui_getint((int *)&vdp_layerA, "layerA");
  ui_getint((int *)&vdp_layerAp, "layerAp");
  ui_getint((int *)&vdp_layerW, "layerW");
  ui_getint((int *)&vdp_layerWp, "layerWp");
  ui_getint((int *)&vdp_layerH, "layerH");
  ui_getint((int *)&vdp_layerS, "layerS");
  ui_getint((int *)&vdp_layerSp, "layerSp");
  ui_getint((int *)&frameskip, "skip");
  ui_getint((int *)&newstate, "state");
  ui_getint((int *)&gen_loglevel, "loglevel");

  ui_getint(&scale, "scale");
  ui_getint(&smooth, "smooth");

  if (scale != oldscale) {
    XDestroyImage(image);
    if ((imagedata = malloc(320 * 240 * scale * scale * (depth / 8))) == NULL)
      ui_err("%s: Out of memory!");
    memset(imagedata, 0, 320 * 240 * scale * scale * (depth / 8));
    if ((image =
         XCreateImage(ui_display, visual, depth, ZPixmap, 0, imagedata,
                      320 * scale, 240 * scale, 8, 0)) == NULL) {
      Tcl_SetResult(interp, "unable to create image", TCL_STATIC);
      return TCL_ERROR;
    }
    image->bits_per_pixel = depth;
    XInitImage(image);
  }
#ifdef XF86DGA
  dga_start = dga_baseaddr + ((depth / 8) *
                              (dga_xsize * ((dga_ysize - (240 * scale)) / 2) +
                               ((dga_xsize - (320 * scale)) / 2)));
#endif

  if (newstate != state) {
    switch (newstate) {
    case 0:
      /* stop */
      break;
    case 1:
      /* pause */
      break;
    case 2:
      /* play */
      if (cpu68k_rom == NULL) {
        Tcl_SetResult(interp, "No ROM loaded\n", TCL_STATIC);
        return TCL_ERROR;
      }
      if (state == 0)
        gen_reset();
      break;
    }
    state = newstate;
  }
  return TCL_OK;
}

/*** Gen_Initialised ***/

int
Gen_Initialised(ClientData cdata, Tcl_Interp * interp, int objc,
                Tcl_Obj * const objv[])
{
  (void)cdata;
  (void)objv;
  if (objc != 1) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_Change\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  if (ui_initload) {
    char *error;
    
    if (NULL != (error = gen_loadimage(ui_initload))) {
      Tcl_SetResult(interp, error, TCL_STATIC);
      return TCL_ERROR;
    }
    ui_updateregs();
    if (save) {
      char filename[4096], *p = filename;
      size_t size = sizeof filename;
      FILE *f;

      p = append_string(p, &size, "./");
      p = append_string(p, &size, gen_cartinfo.name_overseas);
      p = append_string(p, &size, " (");
      p = append_uint_hex(p, &size, gen_cartinfo.checksum);
      p = append_string(p, &size, "-");
      p = append_string(p, &size, gen_cartinfo.country);
      p = append_string(p, &size, ")");

      f = fopen(filename, "wb");
      if (!f) {
        Tcl_SetResult(interp, strerror(errno), TCL_STATIC);
        return TCL_ERROR;
      } else {
        size_t ret, n = cpu68k_romlen;

        ret = fwrite(cpu68k_rom, 1, n,f );
        fclose(f);

        if (ret != n) {
          Tcl_SetResult(interp, strerror(errno), TCL_STATIC);
          return TCL_ERROR;
        }
      }
      LOG_REQUEST(("Saved \"%s\"", filename));
    }
  }
  return TCL_OK;
}

/*** Gen_Reset ***/

int
Gen_Reset(ClientData cdata, Tcl_Interp * interp, int objc,
          Tcl_Obj * const objv[])
{
  int resetitem;

  (void)cdata;
  if (objc != 2) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_Reset <x>\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_GetIntFromObj(interp, objv[1], &resetitem);
  switch (resetitem) {
  case 1:                      /* z80 */
    cpuz80_reset();
    break;
  default:
    Tcl_SetResult(interp, "unknown reset item", TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*** ui_getint - get a TCL integer into a C int ***/

int ui_getint(int *cint, char *tclint)
{
  Tcl_Obj *valobj, *varobj;

  varobj = Tcl_NewStringObj(tclint, -1);
  Tcl_IncrRefCount(varobj);
  if (!(valobj = Tcl_ObjGetVar2(interp, varobj, NULL, TCL_GLOBAL_ONLY))) {
    Tcl_SetResult(interp, "variable not set", TCL_STATIC);
    return TCL_ERROR;
  }
  Tcl_IncrRefCount(valobj);
  if (Tcl_GetIntFromObj(interp, valobj, cint) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(valobj);
  return TCL_OK;
}

/*** ui_setstr - store a string into a TCL variable ***/

int ui_setstr(const char *name, const char *value)
{
  Tcl_Obj *strobj, *varobj, *outobj;

  varobj = Tcl_NewStringObj((char *)name, -1);
  Tcl_IncrRefCount(varobj);
  strobj = Tcl_NewStringObj((char *)value, -1);
  Tcl_IncrRefCount(strobj);
  if (!(outobj = Tcl_ObjSetVar2(interp, varobj, NULL, strobj,
                                TCL_GLOBAL_ONLY))) {
    return -1;
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(strobj);
  return 0;
}

/*** Gen_Regs - display registers ***/

int
Gen_Regs(ClientData cdata, Tcl_Interp * interp, int objc,
         Tcl_Obj * const objv[])
{
  (void)cdata;
  (void)objv;
  if (objc != 1) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_Regs\"", TCL_STATIC);
    return TCL_ERROR;
  }
  vdp_showregs();
  return TCL_OK;
}

/*** Gen_VDPDescribe - describe vdp state ***/

int
Gen_VDPDescribe(ClientData cdata, Tcl_Interp * interp, int objc,
                Tcl_Obj * const objv[])
{
  (void)cdata;
  (void)objv;
  if (objc != 1) {
    Tcl_SetResult(interp, "wrong # args: should be \"Gen_VDPDescribe\"",
                  TCL_STATIC);
    return TCL_ERROR;
  }
  vdp_describe();
  return TCL_OK;
}

int
ui_redrawdump(Tcl_Interp * interp, const char *textwidget)
{
  int offset, line, listlen, words, memtype;
  char tmp[256], dumpline[128];
  Tcl_Obj *varobj, *outobj, *paramobj;
  uint8 *mem_where;
  unsigned int mem_start, mem_len;

  /* get offset variable */
  sprintf(tmp, "%s.offset", textwidget);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if ((outobj = Tcl_ObjGetVar2(interp, varobj, NULL, TCL_GLOBAL_ONLY |
                               TCL_LEAVE_ERR_MSG)) == NULL) {
    return TCL_ERROR;
  }
  Tcl_IncrRefCount(outobj);
  if (Tcl_GetIntFromObj(interp, outobj, &offset) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(outobj);
  Tcl_DecrRefCount(varobj);

  /* get memtype variable */
  sprintf(tmp, "%s.memtype", textwidget);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if ((outobj = Tcl_ObjGetVar2(interp, varobj, NULL, TCL_GLOBAL_ONLY |
                               TCL_LEAVE_ERR_MSG)) == NULL) {
    return TCL_ERROR;
  }
  Tcl_IncrRefCount(outobj);
  if (Tcl_GetIntFromObj(interp, outobj, &memtype) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(outobj);

  /* clear current text display */
  sprintf(tmp, "%s.main delete 1.0 end", textwidget);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if (Tcl_EvalObj(interp, varobj) == TCL_ERROR) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);

  switch (memtype) {
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
  default:                     /* 0 */
    mem_where = cpu68k_rom;
    mem_start = 0;
    mem_len = cpu68k_romlen;
    break;
  }

  line = 1;
  while (line <= 100) {         /* do dump loop */

    /* first check visibility of line by checking bbox of char 0 on the line */
    sprintf(tmp, "[%s.main bbox %d.0]", textwidget, line);
    varobj = Tcl_NewStringObj(tmp, -1);
    Tcl_IncrRefCount(varobj);
    if (Tcl_ExprObj(interp, varobj, &outobj) == TCL_ERROR) {
      return TCL_ERROR;
    }
    if (Tcl_ListObjLength(interp, outobj, &listlen) == TCL_ERROR) {
      return TCL_ERROR;
    }
    Tcl_DecrRefCount(outobj);
    Tcl_DecrRefCount(varobj);

    if (listlen == 0 || (unsigned int)offset > mem_len) {
      break;
    }

    /* this line is visible so disassemble it */
    if ((unsigned int)offset == mem_len) {
      sprintf(dumpline, "---end\n");
      words = 1;
    } else {
      words = diss68k_getdumpline(mem_start + offset, mem_where + offset,
                                  dumpline);
    }

    /* insert line at end of disassembly */
    sprintf(tmp, "%s.main insert end {%s}", textwidget, dumpline);
    varobj = Tcl_NewStringObj(tmp, -1);
    Tcl_IncrRefCount(varobj);
    if (Tcl_EvalObj(interp, varobj) == TCL_ERROR) {
      return TCL_ERROR;
    }
    Tcl_DecrRefCount(varobj);

    /* increment address and line number and loop */
    offset += words * 2;
    line++;
  }
  line--;

  /* update scrollbar */
  sprintf(tmp, "%s.bar set %f %f", textwidget, offset / (double)mem_len,
          (offset + line * 2) / (double)mem_len);
  varobj = Tcl_NewStringObj(tmp, -1);
  Tcl_IncrRefCount(varobj);
  if (Tcl_EvalObj(interp, varobj) == TCL_ERROR) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);

  /* record number of lines for paging function */
  sprintf(tmp, "%s.lines", textwidget);
  varobj = Tcl_NewStringObj(tmp, -1);
  paramobj = Tcl_NewIntObj(line);
  Tcl_IncrRefCount(varobj);
  Tcl_IncrRefCount(paramobj);
  if (Tcl_ObjSetVar2(interp, varobj, NULL, paramobj,
                     TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
    return TCL_ERROR;
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(paramobj);
  return TCL_OK;
}

/*** Program entry point ***/

int ui_init(int argc, const char *argv[])
{
  int i;
  unsigned long plane_masks[1];
  unsigned long colours[256];
  XColor xcolor;

  /* Create tcl interpreter and parse arguments */
  interp = Tcl_CreateInterp();
  if (Tk_ParseArgv(interp, NULL, &argc, argv, argtable, 0) != TCL_OK)
    ui_err("%s: %s", argv[0], Tcl_GetStringResult(interp));
  if (argc < 1 || argc > 2)
    ui_err("Usage: %s [<file>]", argv[0]);

  /* Initialise tcl and tk */
  if (Tcl_Init(interp) != TCL_OK)
    ui_err("%s: Tcl_Init failed: %s", argv[0], Tcl_GetStringResult(interp));
  if (Tk_Init(interp) != TCL_OK)
    ui_err("%s: Tk_Init failed: %s", argv[0], Tcl_GetStringResult(interp));
  if ((mainwin = Tk_MainWindow(interp)) == NULL)
    ui_err("%s: No main window!", argv[0]);

  ui_display = Tk_Display(mainwin);
  ui_screen = Tk_Screen(mainwin);
  ui_screenno = Tk_ScreenNumber(mainwin);

  /* Configure error handling */
  Tk_CreateErrorHandler(ui_display, -1, -1, -1, errorhandler, (char *)NULL);
  if (xsync)
    XSynchronize(ui_display, True);

  /* Create new Tcl commands */
  Tcl_CreateObjCommand(interp, "Gen_Load", Gen_Load, NULL, NULL);
  Tcl_CreateObjCommand(interp, "Gen_Save", Gen_Save, NULL, NULL);
  Tcl_CreateCommand(interp, "Gen_Dump", Gen_Dump, NULL, NULL);
  Tcl_CreateObjCommand(interp, "Gen_Step", Gen_Step, NULL, NULL);
  Tcl_CreateObjCommand(interp, "Gen_FrameStep", Gen_FrameStep, NULL, NULL);
  Tcl_CreateObjCommand(interp, "Gen_Cont", Gen_Cont, NULL, NULL);
  Tcl_CreateObjCommand(interp, "Gen_Change", Gen_Change, NULL, NULL);
  Tcl_CreateObjCommand(interp, "Gen_Regs", Gen_Regs, NULL, NULL);
  Tcl_CreateObjCommand(interp, "Gen_VDPDescribe", Gen_VDPDescribe, NULL,
                       NULL);
  Tcl_CreateObjCommand(interp, "Gen_SpriteList", Gen_SpriteList, NULL, NULL);
  Tcl_CreateObjCommand(interp, "Gen_Initialised", Gen_Initialised, NULL,
                       NULL);
  Tcl_CreateObjCommand(interp, "Gen_Reset", Gen_Reset, NULL, NULL);

  if ((visual = Tk_GetVisual(interp, mainwin, "truecolor 24", &depth,
                             &cmap)) == NULL) {
    depth = 0;
  }
  LOG_VERBOSE(("Asked for depth 24, got depth %d", depth));
  if (depth != 16 && depth != 24) {
    if ((visual = Tk_GetVisual(interp, mainwin, "pseudocolor 8", &depth,
                               NULL)) == NULL)
      ui_err("%s: GetVisual failed: %s", argv[0],
             Tcl_GetStringResult(interp));
    if (depth != 8)
      ui_err("%s: Depth is %d (we want 8, e.g. 256 colours).", argv[0],
             depth);
    if ((cmap = Tk_GetColormap(interp, mainwin, "new")) == 0)
      ui_err("%s: Unable to create new colormap.", argv[0]);
    if (Tk_SetWindowVisual(mainwin, visual, 8, cmap) != 1)
      ui_err("%s: Unable to set window visual/colormap.", argv[0]);
    if (XAllocColorCells(ui_display, cmap, True, plane_masks, 0,
                         colours, 32) == 0)
      ui_err("%s: Unable to allocate colors.", argv[0]);
    for (i = 0; i < 32; i++) {
      xcolor.pixel = i;
      xcolor.flags = 0;
      XQueryColor(ui_display, DefaultColormap(ui_display, ui_screenno),
                  &xcolor);
      XStoreColor(ui_display, cmap, &xcolor);
    }
    if (XAllocColorCells(ui_display, cmap, True, plane_masks, 0,
                         colours, 64 * 3) == 0)
      ui_err("%s: Unable to allocate colors.", argv[0]);
    basepixel = colours[0];
    LOG_VERBOSE(("UI: Base pixel %d", basepixel));
  } else {
    if (Tk_SetWindowVisual(mainwin, visual, depth, cmap) != 1)
      ui_err("%s: Unable to set window visual/colormap.", argv[0]);
  }

  if ((imagedata = malloc(320 * 240 * (depth / 8))) == NULL)
    ui_err("%s: Out of memory!", argv[0]);
  memset(imagedata, 0, 320 * 240 * (depth / 8));
  if ((image = XCreateImage(ui_display, visual, depth, ZPixmap, 0,
                            imagedata, 320, 240, 8, 0)) == NULL) {
    ui_err("%s: Unable to create image.", argv[0]);
  }
  /* XCreateImage is broken on my system - force these values */
  image->bytes_per_line = 320 * (depth / 8);
  image->bits_per_pixel = depth;
  if (depth != 8) {
    visual_bluemask = image->blue_mask;
    visual_redmask = image->red_mask;
    visual_greenmask = image->green_mask;
    visual_bluepos = ui_topbit(visual_bluemask) - 2;
    visual_redpos = ui_topbit(visual_redmask) - 2;
    visual_greenpos = ui_topbit(visual_greenmask) - 2;
    if (visual_bluepos == -1 || visual_redpos == -1 || visual_greenpos == -1) {
      ui_err("%s: Bad colour masks (%x(%d),%x(%d),%x(%d)).",
             argv[0], visual_redmask, visual_redpos,
             visual_greenmask, visual_greenpos,
             visual_bluemask, visual_bluepos);
    }
  }

  XInitImage(image);

  /* DGA stuff */

  ui_dga = 0;

#ifdef XF86DGA
  if (geteuid()) {
    LOG_NORMAL(("%s: You are not root and so DGA is disabled.", argv[0]));
  } else if (!XF86DGAQueryVersion(ui_display, &dga_major, &dga_minor)) {
    LOG_NORMAL(("%s: XF86DGAQueryVersion failed", argv[0]));
  } else if (!XF86DGAQueryExtension(ui_display, &dga_eventb, &dga_errorb)) {
    LOG_NORMAL(("%s: XF86DGAQueryExtension failed", argv[0]));
  } else if (!XF86DGAQueryDirectVideo(ui_display, ui_screenno, &dga_flags)) {
    LOG_NORMAL(("%s: XF86DGAQueryDirectVideo failed", argv[0]));
  } else if (!(dga_flags & XF86DGADirectPresent)) {
    LOG_NORMAL(("%s: DGA Direct video support is not present", argv[0]));
  } else if (!XF86DGAGetVideo(ui_display, ui_screenno, &dga_baseaddr,
                              &dga_width, &dga_banksize, &dga_memsize)) {
    LOG_NORMAL(("%s: XF86DGAGetVideo failed", argv[0]));
  } else if (!XF86DGAGetViewPortSize(ui_display, ui_screenno, &dga_xsize,
                                     &dga_ysize)) {
    LOG_NORMAL(("%s: XF86DGAGetViewPortsize failed", argv[0]));
  } else {
    ui_dga = 1;
    dga_start = dga_baseaddr + ((depth / 8) *
                                (dga_xsize *
                                 ((dga_ysize - (240 * scale)) / 2) +
                                 ((dga_xsize - (320 * scale)) / 2)));
    LOG_NORMAL(("DGA: Direct full-screen video enabled at "
                "%08X (%d/%d) [%d x %d].", dga_baseaddr, dga_banksize,
                dga_memsize, dga_xsize, dga_ysize));
  }
  if (!ui_dga) {
    LOG_NORMAL(("DGA has been disabled, no full-screen facilities will "
                "be available."));
  }
#else
  LOG_VERBOSE(("DGA: Not compiled in."));
#endif
  if (setuid(getuid()))
    ui_err("%s: Failed to setuid: %s", argv[0], strerror(errno));

  /* end DGA stuff */

  ui_updateint("debug", gen_debugmode);

  Tk_CreateEventHandler(mainwin, KeyPressMask | KeyReleaseMask,
                        ui_keypress, NULL);
  Tk_CreateEventHandler(mainwin, EnterWindowMask | LeaveWindowMask,
                        ui_enterleave, NULL);

  if (argc == 2) {
    if ((ui_initload = malloc(strlen(argv[1]) + 1)) == NULL) {
      fprintf(stderr, "Out of memory\n");
      return 1;
    }
    strcpy(ui_initload, argv[1]);
  }
  return 0;
}

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

void ui_final(void)
{
  XAutoRepeatOn(ui_display);
}

void ui_enterleave(ClientData cd, XEvent * event)
{

  (void)cd;
  if (event->type == EnterNotify) {
    XAutoRepeatOff(ui_display);
  } else {
    XAutoRepeatOn(ui_display);
  }
}

void ui_keypress(ClientData cd, XEvent * event)
{
  char keysym = XLookupKeysym(&event->xkey, 0);
  int type = (event->type == KeyPress);

  (void)cd;
  LOG_DEBUG3(("KEY %d: %d", type, keysym));
  switch (keysym) {
  case 'a':
  case '1':
  case 'z':
    mem68k_cont[0].a = type;
    break;
  case 'b':
  case '2':
  case 's':
  case 'x':
    mem68k_cont[0].b = type;
    break;
  case 'c':
  case '3':
  case 'd':
    mem68k_cont[0].c = type;
    break;
  case 81:
    mem68k_cont[0].left = type;
    break;
  case 82:
    mem68k_cont[0].up = type;
    break;
  case 83:
    mem68k_cont[0].right = type;
    break;
  case 84:
    mem68k_cont[0].down = type;
    break;
  case 13:
    mem68k_cont[0].start = type;
    break;
#ifdef XF86DGA
  case '#':
    if (type)
      ui_fullscreen(ui_dga_state ^ 1);
    break;
#endif
  default:
    break;
  }
}

/*** ui_updateregs - update register information ***/

void ui_updateregs(void)
{
  int i, t;
  char val[256], var[256];

  for (t = 0; t < 2; t++) {
    for (i = 0; i < 8; i++) {
      sprintf(val, "%08X", regs.regs[t * 8 + i]);
      sprintf(var, "regs.%s%d", t ? "a" : "d", i);
      ui_updatestr(var, val);
    }
  }
  sprintf(val, "%08X", regs.pc);
  ui_updatestr("regs.pc", val);
  sprintf(val, "%04X", regs.sr.sr_int);
  ui_updatestr("regs.sr", val);
  sprintf(val, "%08X", regs.sp);
  ui_updatestr("regs.sp", val);
  ui_updateint("regs.s", regs.sr.sr_struct.s);
  ui_updateint("regs.x", regs.sr.sr_struct.x);
  ui_updateint("regs.n", regs.sr.sr_struct.n);
  ui_updateint("regs.z", regs.sr.sr_struct.z);
  ui_updateint("regs.v", regs.sr.sr_struct.v);
  ui_updateint("regs.c", regs.sr.sr_struct.c);
  ui_updateint("regs.frames", cpu68k_frames);
  ui_updateint("regs.clocks", cpu68k_clocks);
}

void ui_updatestr(char *var, char *val)
{
  Tcl_Obj *valobj, *varobj;

  valobj = Tcl_NewStringObj(val, -1);
  Tcl_IncrRefCount(valobj);
  varobj = Tcl_NewStringObj(var, -1);
  Tcl_IncrRefCount(varobj);

  if (Tcl_ObjSetVar2(interp, varobj, NULL, valobj,
                     TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
    LOG_CRITICAL(("Error setting %s", val));
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(valobj);
}

void ui_updateint(char *var, int val)
{
  Tcl_Obj *valobj, *varobj;

  valobj = Tcl_NewIntObj(val);
  Tcl_IncrRefCount(valobj);
  varobj = Tcl_NewStringObj(var, -1);
  Tcl_IncrRefCount(varobj);

  if (Tcl_ObjSetVar2(interp, varobj, NULL, valobj,
                     TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
    LOG_CRITICAL(("Error setting %s", val));
  }
  Tcl_DecrRefCount(varobj);
  Tcl_DecrRefCount(valobj);
}

void ui_musiclog(uint8 *data, unsigned int length)
{
   if (-1 != ui_musicfile)
        write(ui_musicfile, data, length);
}

/*** ui_loop - main UI loop ***/

int ui_loop(void)
{
  const char *trace;

  ui_updateregs();

  /* Run Tcl script */
  if (Tcl_EvalFile(interp, FNAME_TCLSCRIPT) != TCL_OK) {
    LOG_CRITICAL(("error: %s", Tcl_GetStringResult(interp)));
    if ((trace = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY)) != NULL) {
      LOG_NORMAL(("TCL TRACE: %s", trace));
    }
    return 1;
  }

  gen_quit = 0;
  while (!gen_quit) {
    switch (state) {
    case 0:                    /* stopped */
      Tcl_DoOneEvent(TCL_ALL_EVENTS);
      break;
    case 1:                    /* paused */
      Tcl_DoOneEvent(TCL_ALL_EVENTS);
      break;
    case 2:                    /* playing */
      event_doframe();
      Tcl_DoOneEvent(TCL_ALL_EVENTS | TCL_DONT_WAIT);
      break;
    }
  }
  return 0;
}

/*** ui_loadimage - load ROM image ***/

int ui_loadimage(Tcl_Interp * interp, const char *filename)
{
  char *p;

  p = gen_loadimage(filename);
  if (!p)
    p = ui_setinfo(&gen_cartinfo);
  if (p) {
    Tcl_SetResult(interp, p, TCL_STATIC);
    return TCL_ERROR;
  }
  ui_updateregs();
  return TCL_OK;
}

/*** ui_loadsavepos - load saved position ***/

int ui_loadsavepos(Tcl_Interp * interp, const char *filename)
{
  if ((state_loadfile(filename)) != 0) {
    Tcl_SetResult(interp, "error loading file", TCL_STATIC);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*** ui_setinfo - set information window with name/copyright/etc ***/

static char *ui_setinfo(t_cartinfo * cartinfo)
{
  char buffer[128];

  if (ui_setstr("i_console", cartinfo->console) ||
      ui_setstr("i_copyright", cartinfo->copyright) ||
      ui_setstr("i_name_domestic", cartinfo->name_domestic) ||
      ui_setstr("i_name_overseas", cartinfo->name_overseas) ||
      ui_setstr("i_version", cartinfo->version) ||
      ui_setstr("i_memo", cartinfo->memo))
    return "Unable to set tcl variables";
  sprintf(buffer, "%x", cartinfo->checksum);
  if (ui_setstr("i_checksum", buffer))
    return "Unable to set checksum tcl variable";
  if (cartinfo->prodtype == pt_game) {
    if (ui_setstr("i_prodtype", "Game"))
      return "Unable to set checksum tcl variable";
  } else if (cartinfo->prodtype == pt_education) {
    if (ui_setstr("i_prodtype", "Education"))
      return "Unable to set checksum tcl variable";
  } else {
    if (ui_setstr("i_prodtype", "Unknown"))
      return "Unable to set checksum tcl variable";
  }
  return NULL;
}

/*** ui_checkpalcache - make palette cache up-to-date ***/

void ui_checkpalcache(int flag)
{
  unsigned int col;
  uint8 *p;

  /* this code requires that there be at least 4 bits per colour, that
     is, three bits that come from the console's palette, and one more bit
     when we do a dim or bright colour, i.e. this code works with 12bpp
     upwards */

  for (col = 0; col < 64; col++) {
    if (!flag && !vdp_cramf[col])
      continue;
    vdp_cramf[col] = 0;
    p = (uint8 *)vdp_cram + 2 * col;
    ui_palcache[col] =
      (((p[0] >> 1) & 7) << visual_bluepos) |
      (((p[1] >> 1) & 7) << visual_redpos) |
      (((p[1] >> 5) & 7) << visual_greenpos);
    ui_palcache[col + 64] =
      (((p[0] >> 1) & 7) << (visual_bluepos - 1)) |
      (((p[1] >> 1) & 7) << (visual_redpos - 1)) |
      (((p[1] >> 5) & 7) << (visual_greenpos - 1)) |
      (4 << visual_bluepos) | (4 << visual_redpos) | (4 << visual_greenpos);
    ui_palcache[col + 128] =
      (((p[0] >> 1) & 7) << (visual_bluepos - 1)) |
      (((p[1] >> 1) & 7) << (visual_redpos - 1)) |
      (((p[1] >> 5) & 7) << (visual_greenpos - 1));
  }
}

/*** ui_line - it is time to render a line ***/

void ui_line(int line)
{
  static uint8 gfx[320];
  char *p;
  int lineoffset;

  if (ui_vdpsimple)
    return;
  if (line < 0 || line >= (int)vdp_vislines)
    return;

#ifdef XF86DGA
  if (ui_dga_state) {
    lineoffset = dga_xsize * (depth / 8);
    p = dga_start + line * lineoffset * scale;
  } else {
    lineoffset = 320 * (depth / 8) * scale;
    p = imagedata + line * lineoffset * scale;
  }
#else
  lineoffset = 320 * (depth / 8) * scale;
  p = imagedata + line * lineoffset * scale;
#endif

  if (cpu68k_frames % frameskip != 0)
    return;

  LOG_DEBUG2(("line %d: ", line));
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
  if (scale == 2 && smooth) {
    ui_convertdata_smooth(gfx, p, 320, lineoffset, depth);
  } else {
    ui_convertdata(gfx, p, 320, lineoffset, scale, depth, basepixel);
  }
}

void
ui_convertdata(uint8 *indata, uint8 *outdata, unsigned int pixels,
               unsigned int lineoffset, unsigned int scale,
               unsigned int depth, unsigned int basepixel)
{
  char *p, *pp;
  unsigned int ui;
  unsigned int t, s;
  uint32 data;

  if (depth == 8) {
    if (scale == 1) {
      /* not scaled, 8bpp */
      for (ui = 0; ui < pixels; ui++) {
        outdata[ui] = indata[ui] + basepixel;
      }
    } else {
      /* scaled, 8bpp */
      p = outdata;
      for (ui = 0; ui < pixels; ui++, p += ui * scale) {
        pp = p;
        for (s = 0; s < scale; s++, pp += lineoffset) {
          for (t = 0; t < scale; t++) {
            pp[t] = indata[ui] + basepixel;
          }
        }
      }
    }
  } else {
    ui_checkpalcache(0);
    if (scale == 1) {
      if (depth == 16) {
        /* not scaled, 16bpp */
        for (ui = 0; ui < pixels; ui++) {
          data = ui_palcache[indata[ui]];
          ((uint16 *)outdata)[ui] = data;       /* already in local endian */
        }
      } else if (depth == 24) {
        /* not scaled, 24bpp */
        p = outdata;
        for (ui = 0; ui < pixels; ui++, p += 3) {
          data = ui_palcache[indata[ui]];
#ifdef WORDS_BIGENDIAN
          p[0] = (uint8)(data >> 16);
          p[1] = (uint8)(data >> 8);
          p[2] = (uint8)(data);
#else
          p[0] = (uint8)(data);
          p[1] = (uint8)(data >> 8);
          p[2] = (uint8)(data >> 16);
#endif
        }
      } else {
        ui_err("unknown depth %d", depth);
      }
    } else if (scale == 2) {
      if (depth == 16) {
        /* scaled by 2, 16bpp */
        p = outdata;
        for (ui = 0; ui < pixels; ui++, p += 4) {
          data = ui_palcache[indata[ui]];
          ((uint16 *)p)[0] = data;      /* already in local endian */
          ((uint16 *)p)[1] = data;
          ((uint16 *)(p + lineoffset))[0] = data;
          ((uint16 *)(p + lineoffset))[1] = data;
        }
      } else if (depth == 24) {
        /* scaled by 2, 24bpp */
        p = outdata;
        for (ui = 0; ui < pixels; ui++, p += 6) {
          data = ui_palcache[indata[ui]];
#ifdef WORDS_BIGENDIAN
          p[0] = (uint8)(data >> 16);
          p[3] = (uint8)(data >> 16);
          p[lineoffset + 0] = (uint8)(data >> 16);
          p[lineoffset + 3] = (uint8)(data >> 16);
          p[1] = (uint8)(data >> 8);
          p[4] = (uint8)(data >> 8);
          p[lineoffset + 1] = (uint8)(data >> 8);
          p[lineoffset + 4] = (uint8)(data >> 8);
          p[2] = (uint8)(data);
          p[5] = (uint8)(data);
          p[lineoffset + 2] = (uint8)(data);
          p[lineoffset + 5] = (uint8)(data);
#else
          p[0] = (uint8)(data);
          p[3] = (uint8)(data);
          p[lineoffset + 0] = (uint8)(data);
          p[lineoffset + 3] = (uint8)(data);
          p[1] = (uint8)(data >> 8);
          p[4] = (uint8)(data >> 8);
          p[lineoffset + 1] = (uint8)(data >> 8);
          p[lineoffset + 4] = (uint8)(data >> 8);
          p[2] = (uint8)(data >> 16);
          p[5] = (uint8)(data >> 16);
          p[lineoffset + 2] = (uint8)(data >> 16);
          p[lineoffset + 5] = (uint8)(data >> 16);
#endif
        }
      } else {
        ui_err("unknown depth %d", depth);
      }
    } else {
      /* scaled by more than 2 */
      for (ui = 0; ui < pixels; ui++) {
        data = ui_palcache[indata[ui]];
        p = outdata + ui * (depth / 8) * scale;
        for (s = 0; s < scale; s++, p += lineoffset) {
          for (t = 0; t < scale; t++) {
            if (depth == 16) {
              ((uint16 *)p)[t] = data;  /* already in local endian */
            } else if (depth == 24) {
              pp = p + t * 3;
#ifdef WORDS_BIGENDIAN
              pp[0] = (uint8)(data >> 16);
              pp[1] = (uint8)(data >> 8);
              pp[2] = (uint8)(data);
#else
              pp[0] = (uint8)(data);
              pp[1] = (uint8)(data >> 8);
              pp[2] = (uint8)(data >> 16);
#endif
            } else {
              ui_err("unknown depth %d", depth);
            }
          }
        }
      }
    }
  }
}

void
ui_convertdata_smooth(uint8 *indata, uint8 *outdata, unsigned int pixels,
                      unsigned int lineoffset, unsigned int depth)
{
  char *p;
  unsigned int ui;
  uint32 data1, data2, datam;

  ui_checkpalcache(0);
  if (depth == 16) {
    /* scaled by 2, 16bpp */
    p = outdata;
    data1 = ui_palcache[indata[0]];
    for (ui = 0; ui < pixels - 1; ui++, p += 4) {
      data2 = ui_palcache[indata[ui + 1]];
      datam = (((((data1 & visual_redmask) +
                  (data2 & visual_redmask)) >> 1) & visual_redmask) |
               ((((data1 & visual_greenmask) +
                  (data2 & visual_greenmask)) >> 1) & visual_greenmask) |
               ((((data1 & visual_bluemask) +
                  (data2 & visual_bluemask)) >> 1) & visual_bluemask));
      ((uint16 *)p)[0] = data1; /* already in local endian */
      ((uint16 *)p)[1] = datam;
      ((uint16 *)(p + lineoffset))[0] = data1;
      ((uint16 *)(p + lineoffset))[1] = datam;
      data1 = data2;
    }
  } else {
    /* scaled by 2, 24bpp */
    p = outdata;
    data1 = ui_palcache[indata[0]];
    for (ui = 0; ui < pixels; ui++, p += 6) {
      data2 = ui_palcache[indata[ui + 1]];
      datam = (((((data1 & visual_redmask) +
                  (data2 & visual_redmask)) >> 1) & visual_redmask) |
               ((((data1 & visual_greenmask) +
                  (data2 & visual_greenmask)) >> 1) & visual_greenmask) |
               ((((data1 & visual_bluemask) +
                  (data2 & visual_bluemask)) >> 1) & visual_bluemask));
#ifdef WORDS_BIGENDIAN
      p[0] = (uint8)(data1 >> 16);
      p[lineoffset + 0] = (uint8)(data1 >> 16);
      p[lineoffset + 3] = (uint8)(datam >> 16);
      p[3] = (uint8)(datam >> 16);
      p[1] = (uint8)(data1 >> 8);
      p[lineoffset + 1] = (uint8)(data1 >> 8);
      p[lineoffset + 4] = (uint8)(datam >> 8);
      p[4] = (uint8)(datam >> 8);
      p[2] = (uint8)(data1);
      p[lineoffset + 2] = (uint8)(data1);
      p[lineoffset + 5] = (uint8)(datam);
      p[5] = (uint8)(datam);
#else
      p[0] = (uint8)(data1);
      p[lineoffset + 0] = (uint8)(data1);
      p[lineoffset + 3] = (uint8)(datam);
      p[3] = (uint8)(datam);
      p[1] = (uint8)(data1 >> 8);
      p[lineoffset + 1] = (uint8)(data1 >> 8);
      p[lineoffset + 4] = (uint8)(datam >> 8);
      p[4] = (uint8)(datam >> 8);
      p[2] = (uint8)(data1 >> 16);
      p[lineoffset + 2] = (uint8)(data1 >> 16);
      p[lineoffset + 5] = (uint8)(datam >> 16);
      p[5] = (uint8)(datam >> 16);
#endif
      data1 = data2;
    }
  }
}

/*** ui_endfield - end of field reached ***/

void ui_endfield(void)
{
  ui_endframe();
}

/*** ui_endframe - end of frame reached ***/

void ui_endframe(void)
{
  int i;
  XColor xcolor;
  static uint8 gfx[(320 + 16) * (240 + 16)];
  char *p, *framestart;
  unsigned int lineoffset, line;

  if (cpu68k_frames % frameskip != 0)
    return;

  if (ui_vdpsimple) {
    /* simple mode - entire frame done here */
    framestart = gfx + (8 * 320) + 8;
    vdp_renderframe(framestart, 320 + 16);      /* plot frame */
#ifdef XF86DGA
    if (ui_dga_state) {
      lineoffset = dga_xsize * (depth / 8);
      p = dga_start;
    } else {
      lineoffset = 320 * (depth / 8) * scale;
      p = imagedata;
    }
#else
    lineoffset = 320 * (depth / 8) * scale;
    p = imagedata;
#endif
    for (line = 0; line < 224; line++) {
      if (scale == 2 && smooth) {
        ui_convertdata_smooth(framestart + (320 + 16) * line,
                              p + lineoffset * line * scale, 320,
                              lineoffset, depth);
      } else {
        ui_convertdata(framestart + (320 + 16) * line,
                       p + lineoffset * line * scale, 320, lineoffset,
                       scale, depth, basepixel);
      }
    }
  }
  if (!ui_dga_state) {
    /* if ui_dga_state state is set, data has already gone to screen */
    if (!gc)
      gc = XCreateGC(ui_display, Tk_WindowId(mainwin), 0, NULL);
    XPutImage(ui_display,
              Tk_WindowId(Tk_NameToWindow(interp, ".main", mainwin)),
              gc, image, 0, 0, 0, 0, 320 * scale, 240 * scale);
  }
  if (depth == 8) {
    for (i = 0; i < 64; i++) {
      if (vdp_cramf[i]) {
        xcolor.pixel = basepixel + i;
        xcolor.flags = DoRed | DoGreen | DoBlue;
        xcolor.blue = ((((uint8 *)vdp_cram)[2 * i]) & (7 << 1)) << 12;
        xcolor.red = ((((uint8 *)vdp_cram)[2 * i + 1]) & (7 << 1)) << 12;
        xcolor.green = ((((uint8 *)vdp_cram)[2 * i + 1]) & (7 << 5)) << 8;
        XStoreColor(ui_display, cmap, &xcolor);
        xcolor.pixel = basepixel + i + 64;
        xcolor.blue =
          1 << 15 | ((((uint8 *)vdp_cram)[2 * i]) & (7 << 1)) << 11;
        xcolor.red =
          1 << 15 | ((((uint8 *)vdp_cram)[2 * i + 1]) & (7 << 1)) << 11;
        xcolor.green =
          1 << 15 | ((((uint8 *)vdp_cram)[2 * i + 1]) & (7 << 5)) << 7;
        XStoreColor(ui_display, cmap, &xcolor);
        xcolor.pixel = basepixel + i + 128;
        xcolor.blue = ((((uint8 *)vdp_cram)[2 * i]) & (7 << 1)) << 11;
        xcolor.red = ((((uint8 *)vdp_cram)[2 * i + 1]) & (7 << 1)) << 11;
        xcolor.green = ((((uint8 *)vdp_cram)[2 * i + 1]) & (7 << 5)) << 7;
        XStoreColor(ui_display, cmap, &xcolor);
        vdp_cramf[i] = 0;
      }
    }
  }
}

#ifdef XF86DGA
void ui_fullscreen(int onoff)
{
  static int forkflag = 0;
  int i;

  ui_dga_state = 0;

  if (!ui_dga) {
    LOG_CRITICAL(("DGA has been disabled, see error message " "on startup."));
    return;
  }
  if (onoff) {
    if (!XF86DGADirectVideo(ui_display, ui_screenno, XF86DGADirectGraphics)) {
      LOG_CRITICAL(("Switch to DGA direct video failed"));
      return;
    }
    if (!XF86DGASetViewPort(ui_display, ui_screenno, 0, 0)) {
      LOG_CRITICAL(("Setting of DGA view port failed"));
      return;
    }
    memset(dga_baseaddr, 0, dga_banksize);
    ui_dga_state = 1;
  } else {
    if (!XF86DGADirectVideo(ui_display, ui_screenno, 0)) {
      LOG_CRITICAL(("Switch out of DGA mode failed"));
      return;
    }
  }
}
#endif

/*** logging functions ***/

/* logging is done this way because this was the best I could come up with
   whilst battling with macros that can only take fixed numbers of arguments */

#define LOG_FUNC(name,level,txt) void ui_log_ ## name (const char *text, ...) \
{ \
  va_list ap; \
  if (gen_loglevel >= level) { \
    printf("%s (%05X) ", txt, cpu68k_clocks); \
    va_start(ap, text); \
    vprintf(text, ap); \
    va_end(ap); \
    putchar(10); \
  } \
}

LOG_FUNC(debug3, 7, "DEBG ");
LOG_FUNC(debug2, 6, "DEBG ");
LOG_FUNC(debug1, 5, "DEBG ");
LOG_FUNC(user, 4, "USER ");
LOG_FUNC(verbose, 3, "---- ");
LOG_FUNC(normal, 2, "---- ");
LOG_FUNC(critical, 1, "CRIT ");
LOG_FUNC(request, 0, "---- ");

/*** ui_err - log error message and quit ***/

void ui_err(const char *text, ...)
{
  va_list ap;

  printf("ERROR: ");

  va_start(ap, text);
  vfprintf(stderr, text, ap);
  va_end(ap);
  putc(10, stderr);
  exit(1);
}

/* vi: set ts=2 sw=2 et cindent: */
