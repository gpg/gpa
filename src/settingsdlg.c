/* settingsdlg.c - The GNU Privacy Assistant
   Copyright (C) 2002, Miguel Coca
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   GPA is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gpa.h"
#include "settingsdlg.h"
#include "gpakeyselector.h"
#include "keyserver.h"

#include "settingsdlg.h"



/* Default key section.  */
static void
key_selected_cb (GtkTreeSelection *treeselection, gpointer user_data)
{
  GpaKeySelector *sel = user_data;
  GList *selected;
  
  selected = gpa_key_selector_get_selected_keys (sel);
  gpa_options_set_default_key (gpa_options_get_instance (),
			       (gpgme_key_t) selected->data);
  g_list_free (selected);
}


static GtkWidget *
default_key_frame (void)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *list;
  GtkWidget *scroller;

  /* Build UI.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new_with_mnemonic (_("<b>Default _key</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  list = gpa_key_selector_new (TRUE);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection 
			       (GTK_TREE_VIEW (list)), GTK_SELECTION_SINGLE);
  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scroller),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
				       GTK_SHADOW_IN);

  gtk_widget_set_size_request (scroller, 320, 120);
  gtk_container_set_border_width (GTK_CONTAINER (scroller), 5);
  gtk_container_add (GTK_CONTAINER (scroller), list);
  gtk_container_add (GTK_CONTAINER (frame), scroller);

  /* Connect signals.  */
  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection
			      (GTK_TREE_VIEW (list))),
		    "changed", G_CALLBACK (key_selected_cb), list);
  return frame;
}


/* Signal handler for the "changed_ui_mode" signal.  */
static void
ui_mode_changed_for_keyserver (GpaOptions *options, gpointer param)
{
  GtkWidget *keyserver_frame = param;

  if (gpa_options_get_simplified_ui (options))
    gtk_widget_hide_all (GTK_WIDGET(keyserver_frame));
  else
    gtk_widget_show_all (GTK_WIDGET(keyserver_frame));
}

/* Default keyserver section.  */
static gboolean
keyserver_selected_cb (GtkWidget *combo, GdkEvent *event, gpointer user_data)
{
  gchar *text = gtk_combo_box_get_active_text (GTK_COMBO_BOX (combo));

  if (text != NULL && *text != '\0')
    gpa_options_set_default_keyserver (gpa_options_get_instance (), text);

  return FALSE;
}


static void
selected_from_list_cb (GtkComboBox *combo, gpointer user_data)
{
  /* Consider the text entry activated.  */
  if (gtk_combo_box_get_active (combo) != -1)
    keyserver_selected_cb (GTK_WIDGET (combo), NULL, NULL);
}


static void
append_to_combo (gpointer item, gpointer data)
{
  GtkWidget *combo = data;
  gchar *text = item;

  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), text);
}


static GtkWidget *
default_keyserver_frame (void)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *combo;
  GList *servers;

  /* Build UI */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new_with_mnemonic (_("<b>Default key_server</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  combo = gtk_combo_box_entry_new_text ();
  gtk_container_set_border_width (GTK_CONTAINER (combo), 5);
  gtk_container_add (GTK_CONTAINER (frame), combo);
  /* Set current value.  */
  servers = keyserver_get_as_glist ();
  g_list_foreach (servers, append_to_combo, combo);
  g_list_free (servers);
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combo))),
                      gpa_options_get_default_keyserver
		      (gpa_options_get_instance ()));

  /* Set default visibility.  */
  ui_mode_changed_for_keyserver (gpa_options_get_instance (), frame);

  /* Connect signals.  Try to follow instant-apply principle.  */
  g_signal_connect_swapped (G_OBJECT (gtk_bin_get_child (GTK_BIN (combo))),
			    "focus-out-event",
			    G_CALLBACK (keyserver_selected_cb), combo);
  g_signal_connect (G_OBJECT (combo),
		    "changed", G_CALLBACK (selected_from_list_cb), NULL);

  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_ui_mode",
                    G_CALLBACK (ui_mode_changed_for_keyserver), frame);
  

  return frame;
}


/* User interface section.  */
static void
advanced_mode_toggled (GtkToggleButton *button, gpointer user_data)
{
  gpa_options_set_simplified_ui (gpa_options_get_instance (), 
                                 !gtk_toggle_button_get_active (button));
}

static GtkWidget *
user_interface_mode_frame (void)
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *button;

  /* Build UI.  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  label = gtk_label_new (_("<b>User interface</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);

  button = gtk_check_button_new_with_mnemonic (_("use _advanced mode"));
  gtk_container_add (GTK_CONTAINER (frame), button);


  /* Select default value.  */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                !gpa_options_get_simplified_ui 
                                (gpa_options_get_instance ()));

  /* Connect signals.  */
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (advanced_mode_toggled), NULL);

  
  return frame;
}


/* Create a new settings dialog and return it.  The dialog is shown
   but not run.  */
GtkWidget *
gpa_settings_dialog_new (void)
{
  GtkWidget *dialog;
  GtkWidget *frame, *keyserver_frame;

  dialog = gtk_dialog_new_with_buttons (_("Settings"), NULL, 0,
                                        _("_Close"),
                                        GTK_RESPONSE_CLOSE, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);

  /* The UI mode section.  */
  frame = keyserver_frame = user_interface_mode_frame ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame);
              
  /* The default key section.  */
  frame = default_key_frame ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame);

  /* The default keyserver section.  */
  frame = default_keyserver_frame ();
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame);

  /* Close the dialog when asked to.  */
  g_signal_connect_swapped (GTK_OBJECT (dialog), 
                            "response", 
                            G_CALLBACK (gtk_widget_destroy),
                            GTK_OBJECT (dialog));

  /* Don't run the dialog here: leave that to gtk_main.  */
  gtk_widget_show_all (GTK_WIDGET (dialog));

  return dialog;
}
