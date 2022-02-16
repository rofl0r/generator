#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "gtkopts.h"

#include "ui-gtk.h"

void
on_load_rom_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(0, 0);
}

void
on_save_rom_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(0, 1);
}

void
on_load_state_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(1, 0);
}

void
on_save_state_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(1, 1);
}

void
on_save_gym_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(2, 1);
}

void
on_start_gnm_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(3, 1);
}

void
on_options_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_options();
}

void
on_reset_soft_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_softreset();
}

void
on_reset_hard_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_hardreset();
}

void
on_view_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
}

void
on_100_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  gtkopts_setvalue("view", "100");
  ui_gtk_newoptions();
}

void
on_200_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  gtkopts_setvalue("view", "200");
  ui_gtk_newoptions();
}

void
on_about_generator_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_about();
}

void
on_console_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_console();
}

void
on_disassemble_rom_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *obj;

  (void)menuitem;
  (void)user_data;

  if ((obj = ui_gtk_newdiss(0)) != NULL)
    gtk_widget_show(obj);
}

void
on_disassemble_ram_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *obj;

  (void)menuitem;
  (void)user_data;

  if ((obj = ui_gtk_newdiss(1)) != NULL)
    gtk_widget_show(obj);
}

void
on_disassemble_vram_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *obj;

  (void)menuitem;
  (void)user_data;

  if ((obj = ui_gtk_newdiss(2)) != NULL)
    gtk_widget_show(obj);
}

void
on_disassemble_cram_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *obj;

  (void)menuitem;
  (void)user_data;

  if ((obj = ui_gtk_newdiss(3)) != NULL)
    gtk_widget_show(obj);
}

void
on_disassemble_vsram_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *obj;

  (void)menuitem;
  (void)user_data;

  if ((obj = ui_gtk_newdiss(4)) != NULL)
    gtk_widget_show(obj);
}

void
on_disassemble_sram_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *obj;

  (void)menuitem;
  (void)user_data;

  if ((obj = ui_gtk_newdiss(5)) != NULL)
    gtk_widget_show(obj);
}

void
on_button_68kregs_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
}

void
on_button_68ksp_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
}

void
on_button_68kpc_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
}

gboolean
on_main_delete_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  (void)widget;
  (void)event;
  (void)user_data;
  ui_gtk_quit();
  return TRUE;
}

gboolean
on_main_destroy_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  (void)widget;
  (void)event;
  (void)user_data;
  ui_gtk_quit();
  return TRUE;
}

gboolean
on_console_remove                      (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  (void)widget;
  (void)event;
  (void)user_data;
  ui_gtk_closeconsole();
  return TRUE;
}

void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_quit();
}

void
on_play_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)user_data;
  if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
    ui_gtk_play();
  }
}

void
on_pause_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)user_data;
  if (GTK_CHECK_MENU_ITEM(menuitem)->active) {
    ui_gtk_pause();
  }
}

void
on_button_opts_save_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
  ui_gtk_saveoptions();
}

void
on_button_opts_ok_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
  ui_gtk_applyoptions();
}

gboolean
on_drawingarea_main_expose_event       (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
  (void)widget;
  (void)event;
  (void)user_data;
  ui_gtk_redraw();
  return TRUE;
}

gboolean
on_mainwin_key_event                   (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
  (void)widget;
  (void)user_data;
  ui_gtk_key(event->keyval, event->type == GDK_KEY_PRESS ? 1 : 0);
  return FALSE;
}

gboolean
on_mainwin_enter_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  (void)widget;
  (void)event;
  (void)user_data;
  ui_gtk_mainenter();
  return FALSE;
}

gboolean
on_mainwin_leave_notify_event          (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data)
{
  (void)widget;
  (void)event;
  (void)user_data;
  ui_gtk_mainleave();
  return FALSE;
}

gboolean
on_keyval_press_event             (GtkWidget       *widget,
                                   GdkEventKey     *event,
                                   gpointer         user_data)
{
  (void)user_data;
  gtk_entry_set_text(GTK_ENTRY(widget), gdk_keyval_name(event->keyval));
  return TRUE;
}

void
on_stop_gym_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_closemusiclog();
}

void
on_stop_gnm_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_closemusiclog();
}

void
on_load_patch_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(4, 0);
}

void
on_save_patch_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(4, 1);
}

void
on_game_genie_codes_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_codes();
}

void
on_button_codes_ok_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
  ui_gtk_codes_ok();
}

void
on_button_codes_add_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
  ui_gtk_codes_add();
}

void
on_button_codes_delete_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
  ui_gtk_codes_delete();
}

void
on_button_codes_deleteall_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
  ui_gtk_codes_deleteall();
}

void
on_button_codes_apply_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
  ui_gtk_codes_apply();
}

void
on_button_codes_clearsel_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  (void)button;
  (void)user_data;
  ui_gtk_codes_clearsel();
}

void
on_start_avi_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_filesel(5, 1);
}

void
on_stop_avi_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void)menuitem;
  (void)user_data;
  ui_gtk_closeavi();
}

gboolean
on_drawingarea_diss_expose_event       (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
  ui_gtk_redrawdiss(widget, event);
  return TRUE;
}

