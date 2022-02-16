extern uint8 ui_vdpsimple;        /* 0=raster, 1=cell based plotter */
extern uint8 ui_clearnext;        /* flag indicating redraw required */
extern uint8 ui_info;             /* does the user want info onscreen or not */
extern uint8 ui_vsync;            /* does the user want us to wait for vsync */

int ui_topbit(unsigned long int bits);
void ui_setupscreen(void);
