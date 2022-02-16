#include <gtk/gtk.h>


gboolean
on_main_delete_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_main_destroy_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_mainwin_key_event                   (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_mainwin_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

gboolean
on_mainwin_leave_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_options_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_load_rom_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_rom_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_load_state_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_state_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_play_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pause_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_reset_soft_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_reset_hard_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_view_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_100_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_200_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_console_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_disassemble_rom_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_disassemble_ram_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_disassemble_vram_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_disassemble_cram_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_disassemble_vsram_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_disassemble_sram_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about_generator_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_drawingarea_main_expose_event       (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_keyval_press_event             (GtkWidget       *widget,
                                   GdkEventKey     *event,
                                   gpointer         user_data);

gboolean
on_console_remove                      (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_button_68kregs_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_68ksp_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_68kpc_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_keyval_press_event                  (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_save_gym_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stop_gym_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_start_gnm_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stop_gnm_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_load_patch_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_patch_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_game_genie_codes_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_button_opts_save_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_opts_ok_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_codes_ok_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_codes_add_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_codes_delete_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_codes_deleteall_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_codes_delete_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_codes_apply_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_button_codes_clearsel_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_start_avi_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stop_avi_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_text_diss_expose_event              (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_diss_configure_event                (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_drawingarea_diss_expose_event       (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_drawingarea_diss_expose_event       (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);
