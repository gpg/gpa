/* settingsdlg.c - The GNU Privacy Assistant
 *	Copyright (C) 2002, Miguel Coca
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "gpa.h"
#include "settingsdlg.h"
#include "gpawidgets.h"
#include "keyserver.h"

/* Default key section */
static void
key_selected_cb (GtkCList *clist, gint row, gint column,
                 GdkEventButton *event, gpointer user_data)
{
  gpa_options_set_default_key (gpa_options_get_instance (),
                               gpa_key_list_selected_id (GTK_WIDGET(clist)));
}

static GtkWidget *
default_key_frame (void)
{
  GtkWidget *frame, *label, *list, *scroller;

  /* Build UI */
  label = gtk_label_new_with_mnemonic (_("Default _key:"));
  frame = gtk_frame_new (NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  list = gpa_secret_key_list_new ();
  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scroller, 320, 120);
  gtk_container_set_border_width (GTK_CONTAINER (scroller), 5);
  gtk_container_add (GTK_CONTAINER (scroller), list);
  gtk_container_add (GTK_CONTAINER (frame), scroller);
  /* Connect signals */
  g_signal_connect (G_OBJECT (list), "select-row", 
                    G_CALLBACK (key_selected_cb), NULL);
  return frame;
}

/* Default keyserver section */
static void
keyserver_selected_cb (GtkWidget *entry, gpointer user_data)
{
  gpa_options_set_default_keyserver (gpa_options_get_instance (),
                                     gtk_entry_get_text (GTK_ENTRY (entry)));
}

static void
selected_from_list_cb (GtkList *list, GtkWidget *widget, GtkWidget *entry)
{
  /* Consider the text entry activated */
  keyserver_selected_cb (entry, NULL);
}

static GtkWidget *
default_keyserver_frame (void)
{
  GtkWidget *frame, *label, *combo;
  
  /* Build UI */
  label = gtk_label_new_with_mnemonic (_("Default key_server: "));
  frame = gtk_frame_new (NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  combo = gtk_combo_new ();
  gtk_combo_set_value_in_list (GTK_COMBO (combo), FALSE, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (combo), 5);
  gtk_container_add (GTK_CONTAINER (frame), combo);
  /* Set current value */
  gtk_combo_set_popdown_strings (GTK_COMBO (combo), keyserver_get_as_glist ());
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
                      gpa_options_get_default_keyserver 
		      (gpa_options_get_instance ()));
  /* Connect signals */
  g_signal_connect (G_OBJECT (GTK_COMBO (combo)->entry), "activate",
                    G_CALLBACK (keyserver_selected_cb), NULL);
  g_signal_connect (G_OBJECT (GTK_COMBO (combo)->list), "select-child",
                    G_CALLBACK (selected_from_list_cb),
                    GTK_COMBO (combo)->entry);


  return frame;
}

/* User interface section */

static void
advanced_mode_toggled (GtkToggleButton *yes_button, gpointer user_data)
{
  if (gtk_toggle_button_get_active (yes_button))
    {
      gpa_options_set_simplified_ui (gpa_options_get_instance (), FALSE);
    }
  else
    {
      gpa_options_set_simplified_ui (gpa_options_get_instance (), TRUE);
    }
}

static GtkWidget *
user_interface_mode_frame (void)
{
  GtkWidget *frame, *label, *hbox, *yes_button, *no_button;
  
  /* Build UI */
  label = gtk_label_new_with_mnemonic (_("Use _advanced mode:"));
  frame = gtk_frame_new (NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  hbox = gtk_hbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  yes_button = gtk_radio_button_new_with_mnemonic (NULL, _("_Yes"));
  gtk_box_pack_start_defaults (GTK_BOX (hbox), yes_button);
  no_button = gtk_radio_button_new_with_mnemonic_from_widget
    (GTK_RADIO_BUTTON (yes_button), _("_No"));
  gtk_box_pack_start_defaults (GTK_BOX (hbox), no_button);
  /* Select default value */
  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (no_button), TRUE);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (yes_button), TRUE);
    }
  /* Connect signals */
  g_signal_connect (G_OBJECT (yes_button), "toggled",
                    G_CALLBACK (advanced_mode_toggled), NULL);
  
  return frame;
}

GtkWidget *
gpa_settings_dialog_new (void)
{
  GtkWidget *dialog;
  GtkWidget *frame;

  dialog = gtk_dialog_new_with_buttons (_("Settings"),
                                        NULL,
                                        0,
                                        GTK_STOCK_CLOSE,
                                        GTK_RESPONSE_CLOSE, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);

  /* The default key section */
  frame = default_key_frame ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame);

  /* The default keyserver section */
  frame = default_keyserver_frame ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame);

  /* The UI mode section */
  frame = user_interface_mode_frame ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame);

  /* Close the dialog when asked to */
  g_signal_connect_swapped (GTK_OBJECT (dialog), 
                            "response", 
                            G_CALLBACK (gtk_widget_destroy),
                            GTK_OBJECT (dialog));

  /* Don't run the dialog here: leave that to gtk_main */
  gtk_widget_show_all (GTK_WIDGET (dialog));
  return dialog;
}
