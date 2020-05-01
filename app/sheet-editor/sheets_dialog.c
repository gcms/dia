/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * sheets_dialog.c : a sheets and objects dialog
 * Copyright (C) 2002 M.C. Nelson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <string.h>

#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "sheets.h"
#include "sheets_dialog_callbacks.h"
#include "sheets_dialog.h"

#include "intl.h"
#include "persistence.h"
#include "dia-builder.h"


static void
sheets_dialog_destroyed (GtkWidget *widget, gpointer user_data)
{
  GObject *builder = g_object_get_data (G_OBJECT (widget), "_sheet_dialogs_builder");

  g_clear_object (&builder);

  g_object_set_data (G_OBJECT (widget), "_sheet_dialogs_builder", NULL);
}


GtkWidget*
create_sheets_main_dialog (void)
{
  GtkWidget *sheets_main_dialog;
  GtkWidget *combo_left, *combo_right;
  GtkListStore *store;
  DiaBuilder *builder;

  builder = dia_builder_new ("ui/sheets-main-dialog.ui");
  sheets_main_dialog = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                           "sheets_main_dialog"));
  g_object_set_data (G_OBJECT (sheets_main_dialog), "_sheet_dialogs_builder", builder);

  g_signal_connect (G_OBJECT (sheets_main_dialog), "destroy",
                    G_CALLBACK (sheets_dialog_destroyed), NULL);

  store = gtk_list_store_new (SO_N_COL,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_POINTER);

  populate_store (store);

  combo_left = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                   "combo_left"));
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_left), GTK_TREE_MODEL (store));
  g_signal_connect (combo_left,
                    "changed",
                    G_CALLBACK (on_sheets_dialog_combo_changed),
                    NULL);

  combo_right = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                    "combo_right"));
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_right), GTK_TREE_MODEL (store));
  g_signal_connect (combo_right,
                    "changed",
                    G_CALLBACK (on_sheets_dialog_combo_changed),
                    NULL);

  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "sheets_main_dialog"),
                    "delete_event",
                    G_CALLBACK (on_sheets_main_dialog_delete_event),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_copy"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_copy_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_copy_all"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_copy_all_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_move"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_move_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_move_all"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_move_all_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_new"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_new_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_move_up"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_move_up_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_move_down"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_move_down_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_edit"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_edit_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_remove"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_remove_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_apply"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_apply_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_revert"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_revert_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_close"),
                    "clicked",
                    G_CALLBACK (on_sheets_dialog_button_close_clicked),
                    NULL);

  persistence_register_window (GTK_WINDOW (sheets_main_dialog));

  return sheets_main_dialog;
}

GtkWidget*
create_sheets_new_dialog (void)
{
  GtkWidget *sheets_new_dialog;
  DiaBuilder *builder;

  builder = dia_builder_new ("ui/sheets-new-dialog.ui");
  sheets_new_dialog = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                          "sheets_new_dialog"));
  g_object_set_data (G_OBJECT (sheets_new_dialog), "_sheet_dialogs_builder", builder);

  g_signal_connect (G_OBJECT (sheets_new_dialog), "destroy",
                    G_CALLBACK (sheets_dialog_destroyed), NULL);

  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_svg_shape"),
                    "toggled",
                    G_CALLBACK (on_sheets_new_dialog_radiobutton_svg_shape_toggled),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_sheet"),
                    "toggled",
                    G_CALLBACK (on_sheets_new_dialog_radiobutton_sheet_toggled),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_line_break"),
                    "toggled",
                    G_CALLBACK (on_sheets_new_dialog_radiobutton_line_break_toggled),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_ok"),
                    "clicked",
                    G_CALLBACK (on_sheets_new_dialog_button_ok_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_cancel"),
                    "clicked",
                    G_CALLBACK (on_sheets_new_dialog_button_cancel_clicked),
                    NULL);

  return sheets_new_dialog;
}


GtkWidget*
create_sheets_edit_dialog (void)
{
  GtkWidget *sheets_edit_dialog;
  DiaBuilder *builder;

  builder = dia_builder_new ("ui/sheets-edit-dialog.ui");
  sheets_edit_dialog = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                           "sheets_edit_dialog"));
  g_object_set_data (G_OBJECT (sheets_edit_dialog),
                     "_sheet_dialogs_builder",
                     builder);

  g_signal_connect (G_OBJECT (sheets_edit_dialog), "destroy",
                    G_CALLBACK (sheets_dialog_destroyed), NULL);

  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "entry_object_description"),
                    "changed",
                    G_CALLBACK (on_sheets_edit_dialog_entry_object_description_changed),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "entry_sheet_description"),
                    "changed",
                    G_CALLBACK (on_sheets_edit_dialog_entry_sheet_description_changed),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "entry_sheet_name"),
                    "changed",
                    G_CALLBACK (on_sheets_edit_dialog_entry_sheet_name_changed),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_ok"),
                    "clicked",
                    G_CALLBACK (on_sheets_edit_dialog_button_ok_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_cancel"),
                    "clicked",
                    G_CALLBACK (on_sheets_edit_dialog_button_cancel_clicked),
                    NULL);

  return sheets_edit_dialog;
}


GtkWidget*
create_sheets_remove_dialog (void)
{
  GtkWidget *sheets_remove_dialog;
  DiaBuilder *builder;

  builder = dia_builder_new ("ui/sheets-remove-dialog.ui");
  sheets_remove_dialog = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                             "sheets_remove_dialog"));
  g_object_set_data (G_OBJECT (sheets_remove_dialog),
                     "_sheet_dialogs_builder",
                     builder);

  g_signal_connect (G_OBJECT (sheets_remove_dialog), "destroy",
                    G_CALLBACK (sheets_dialog_destroyed), NULL);

  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_object"),
                    "toggled",
                    G_CALLBACK (on_sheets_remove_dialog_radiobutton_object_toggled),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_sheet"),
                    "toggled",
                    G_CALLBACK (on_sheets_remove_dialog_radiobutton_sheet_toggled),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_ok"),
                    "clicked",
                    G_CALLBACK (on_sheets_remove_dialog_button_ok_clicked),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "button_cancel"),
                    "clicked",
                    G_CALLBACK (on_sheets_remove_dialog_button_cancel_clicked),
                    NULL);
  /* FIXME:
  gtk_widget_grab_default (button_ok);
  */
  return sheets_remove_dialog;
}


GtkWidget*
create_sheets_shapeselection_dialog (void)
{
  GtkWidget *sheets_shapeselection_dialog;
  GtkWidget *ok_button;
  GtkWidget *cancel_button1;

  sheets_shapeselection_dialog = gtk_file_selection_new (_("Select SVG Shape File"));
  g_object_set_data (G_OBJECT (sheets_shapeselection_dialog), "sheets_shapeselection_dialog", sheets_shapeselection_dialog);
  gtk_container_set_border_width (GTK_CONTAINER (sheets_shapeselection_dialog), 10);

  ok_button = GTK_FILE_SELECTION (sheets_shapeselection_dialog)->ok_button;
  g_object_set_data (G_OBJECT (sheets_shapeselection_dialog), "ok_button", ok_button);
  gtk_widget_show (ok_button);
  gtk_widget_set_can_default (GTK_WIDGET (ok_button), TRUE);

  cancel_button1 = GTK_FILE_SELECTION (sheets_shapeselection_dialog)->cancel_button;
  g_object_set_data (G_OBJECT (sheets_shapeselection_dialog), "cancel_button1", cancel_button1);
  gtk_widget_show (cancel_button1);
  gtk_widget_set_can_default (GTK_WIDGET (cancel_button1), TRUE);

  g_signal_connect (G_OBJECT (ok_button), "clicked",
                    G_CALLBACK (on_sheets_shapeselection_dialog_button_ok_clicked),
                    NULL);
  g_signal_connect (G_OBJECT (cancel_button1), "clicked",
                    G_CALLBACK (on_sheets_shapeselection_dialog_button_cancel_clicked),
                    NULL);

  return sheets_shapeselection_dialog;
}

