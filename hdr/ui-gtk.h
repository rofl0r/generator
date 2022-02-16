typedef enum {
    DEINTERLACE_BOB, DEINTERLACE_WEAVE, DEINTERLACE_WEAVEFILTER
} t_interlace;

void ui_gtk_filesel(int type, int save);
void ui_gtk_about(void);
void ui_gtk_options(void);
void ui_gtk_console(void);
void ui_gtk_quit(void);
void ui_gtk_closeconsole(void);
void ui_gtk_play(void);
void ui_gtk_pause(void);
void ui_gtk_newoptions(void);
void ui_gtk_softreset(void);
void ui_gtk_hardreset(void);
void ui_gtk_saveoptions(void);
void ui_gtk_applyoptions(void);
void ui_gtk_redraw(void);
void ui_gtk_key(unsigned long key, int press);
void ui_gtk_mainenter(void);
void ui_gtk_mainleave(void);
void ui_gtk_closegymlog(void);
