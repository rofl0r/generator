#ifndef UI_CONSOLE_HEADER_FILE
#define UI_CONSOLE_HEADER_FILE

typedef enum {
  DEINTERLACE_BOB, DEINTERLACE_WEAVE, DEINTERLACE_WEAVEFILTER
} t_interlace;

typedef struct {
  int joystick;                   /* -1 for keyboard, joystick no otherwise */
  int keyboard;                   /* 0 for whole keyboard, 1/2 for left/right */
} t_binding;

extern uint32 ui_fkeys;           /* bitmap representing function keys */

extern uint8 ui_vdpsimple;        /* 0=raster, 1=cell based plotter */
extern uint8 ui_clearnext;        /* flag indicating redraw required */
extern uint8 ui_fullscreen;       /* does the user want full screen or not */
extern uint8 ui_info;             /* does the user want info onscreen or not */
extern uint8 ui_vsync;            /* does the user want us to wait for vsync */
extern t_interlace ui_interlace;  /* user de-interlace preference */
extern t_binding ui_bindings[2];  /* keyboard/joystick bindings for players */

int ui_topbit(unsigned long int bits);
void ui_setupscreen(void);

#endif /* UI_CONSOLE_HEADER_FILE */
