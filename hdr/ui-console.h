typedef enum {
  DEINTERLACE_BOB, DEINTERLACE_WEAVE, DEINTERLACE_WEAVEFILTER
} t_interlace;

extern uint32 ui_fkeys;           /* bitmap representing function keys */

extern uint8 ui_vdpsimple;        /* 0=raster, 1=cell based plotter */
extern uint8 ui_clearnext;        /* flag indicating redraw required */
extern uint8 ui_fullscreen;       /* does the user want full screen or not */
extern uint8 ui_info;             /* does the user want info onscreen or not */
extern uint8 ui_vsync;            /* does the user want us to wait for vsync */
extern t_interlace ui_interlace;  /* user de-interlace preference */
int ui_topbit(unsigned long int bits);
void ui_setupscreen(void);
