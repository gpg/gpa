/* keyring.c  -	 The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <gpapa.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gtktools.h"
#include "keyimportdlg.h"
#include "keyserver.h"

struct _GPAKeyImportDialog {
  /* The toplevel dialog window */
  GtkWidget * window;

  /* The filename radio button */
  GtkWidget * radio_filename;

  /* The filename entry widget */
  GtkWidget * entry_filename;

  /* The server radio button */
  GtkWidget * radio_server;

  /* The server combo box */
  GtkWidget * combo_server;

  /* The clipboard radio button */
  GtkWidget * radio_clipboard;

  /* The key id entry widget */
  GtkWidget * entry_key_id;


  /*
   * Result values:
   */

  /* True if the user clicked OK, FALSE otherwise */
  gboolean result;
  
  /* filename */
  gchar * filename;

  /* Server */
  gchar * server;

  /* key ID */
  gchar * key_id;
};
typedef struct _GPAKeyImportDialog GPAKeyImportDialog;


/* Handler for the browse button. Run a modal file dialog and set the
 * text of the file entry widget accordingly */
static void
import_browse (gpointer param)
{
  GPAKeyImportDialog * dialog = param;
  gchar * filename;

  filename = gpa_get_save_file_name (dialog->window,
				     _("Import public keys from file"),
				     NULL);
  if (filename)
    {
      gchar *utf8_filename = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
      gtk_entry_set_text (GTK_ENTRY (dialog->entry_filename),
			  utf8_filename);
      g_free (utf8_filename);
      g_free (filename);
    }
} /* import_browse */


/* Handler for the OK button. Extract the user input from the widgets
 * and destroy the main window */
void
import_ok (gpointer param)
{
  GPAKeyImportDialog * dialog = param;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_server)))
    {
      GtkWidget * entry = GTK_COMBO (dialog->combo_server)->entry;
      dialog->server = (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
      dialog->server = xstrdup_or_null (dialog->server);
      dialog->key_id = (gchar *) gtk_entry_get_text (GTK_ENTRY (dialog->entry_key_id));
      dialog->key_id = xstrdup_or_null (dialog->key_id);
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_filename)))
    {
      dialog->filename = (gchar *) gtk_entry_get_text(GTK_ENTRY(dialog->entry_filename));
      dialog->filename = g_filename_from_utf8 (dialog->filename, -1, NULL, NULL, NULL);
    }

  dialog->result = TRUE;
  gtk_widget_destroy (dialog->window);
} /* import_ok */


/* Handler for the Cancel button. Destroy the main window */
static void
import_cancel (gpointer param)
{
  GPAKeyImportDialog * dialog = param;

  gtk_widget_destroy (dialog->window);
}


/* signal handler for the destroy signal. Quit the recursive main loop
 */
static void
import_destroy (GtkWidget *widget, gpointer param)
{
  gtk_main_quit ();
}


/* Run the import dialog as a modal dialog and return TRUE if the user
 * clicked OK, FALSE otherwise
 */
gboolean
key_import_dialog_run (GtkWidget * parent, gchar ** filename, gchar ** server,
		       gchar ** key_id)
{
  GtkAccelGroup *accel_group;
  
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *server_table;
  GtkWidget *hbox;
  GtkWidget *radio;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *bbox;
  GtkWidget *combo;
  GtkWidget *label;

  GPAKeyImportDialog dialog;

  dialog.result = FALSE;
  dialog.filename = NULL;
  dialog.server = NULL;
  dialog.key_id = NULL;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  dialog.window = window;
  gtk_window_set_title (GTK_WINDOW (window), _("Import Key"));
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
			     GTK_SIGNAL_FUNC (import_destroy),
			     (gpointer) &dialog);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 10);
  gtk_table_set_row_spacing (GTK_TABLE (table), 3, 10);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 30);

  /* File name */
  radio = gpa_radio_button_new_from_widget (NULL, accel_group,
					    _("I_mport from file:"));
  dialog.radio_filename = radio;
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 0, 1, GTK_FILL, 0, 0, 0);

  hbox = gtk_hbox_new (0, FALSE);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 1, 2,
		    GTK_FILL | GTK_EXPAND, 0, 0, 0);

  entry = gtk_entry_new ();
  dialog.entry_filename = entry;
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (entry), "activate",
			     GTK_SIGNAL_FUNC (import_ok), (gpointer) &dialog);

  button = gpa_button_new (accel_group, _("B_rowse..."));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (import_browse),
			     (gpointer) &dialog);

  /* Server */
  radio = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON(radio),
					    accel_group,
					    _("Receive from _server:"));
  dialog.radio_server = radio;
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 2, 3, GTK_FILL, 0, 0, 0);

  server_table = gtk_table_new (2, 2, FALSE);
  gtk_table_attach (GTK_TABLE (table), server_table, 1, 2, 3, 4,
		    GTK_FILL | GTK_EXPAND, 0, 0, 0);

  label = gtk_label_new ("");
  gtk_table_attach (GTK_TABLE (server_table), label, 0, 1, 0, 1,
		    GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  entry = gtk_entry_new ();
  dialog.entry_key_id = entry;
  gtk_table_attach (GTK_TABLE (server_table), entry, 1, 2, 0, 1,
		    GTK_FILL | GTK_EXPAND, 0, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (entry), "activate",
			     GTK_SIGNAL_FUNC (import_ok), (gpointer) &dialog);
  gpa_connect_by_accelerator (GTK_LABEL (label), entry, accel_group,
			      _("Key _ID:"));

  label = gtk_label_new ("");
  gtk_table_attach (GTK_TABLE (server_table), label, 0, 1, 1, 2,
		    GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  combo = gtk_combo_new ();
  dialog.combo_server = combo;
  gtk_table_attach (GTK_TABLE (server_table), combo, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_combo_set_value_in_list (GTK_COMBO (combo), FALSE, FALSE);

  gtk_combo_set_popdown_strings (GTK_COMBO (combo),
                                 keyserver_get_as_glist ());
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
		      keyserver_get_current (TRUE));

  gpa_connect_by_accelerator (GTK_LABEL (label), entry, accel_group,
			      _("_Key Server:"));

  /* Clipboard */
  radio = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON(radio),
					    accel_group,
					    _("Import from _clipboard:"));
  dialog.radio_clipboard = radio;
  gtk_table_attach (GTK_TABLE (table), radio, 0, 2, 4, 5,
                    GTK_FILL, 0, 0, 0);

  /* The button box */
  bbox = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 10);
  gtk_container_set_border_width (GTK_CONTAINER (bbox), 5);

  button = gpa_button_cancel_new (accel_group, _("_Cancel"),
                                  (GtkSignalFunc) import_cancel,
				  &dialog);
  gtk_container_add (GTK_CONTAINER (bbox), button);

  button = gpa_button_new (accel_group, _("_OK"));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (import_ok), (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (bbox), button);

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gpa_window_show_centered (window, parent);

  gtk_main ();

  if (dialog.result)
    {
      *filename = dialog.filename;
      *server = dialog.server;
      *key_id = dialog.key_id;
    }

  return dialog.result;
}

