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

#include "gpa.h"
#include <config.h>
#include <gtk/gtk.h>
#include "gtktools.h"
#include "keyimportdlg.h"
#include "keyserver.h"

#define	XSTRDUP_OR_NULL(s)	((s != NULL) ? g_strdup(s) : NULL )

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

  /* The key id entry widget */
  GtkWidget * entry_key_id;


  /*
   * Result values:
   */
  
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
}


/* Extract the user input from the widgets and destroy the main window */
static void
import_ok (GPAKeyImportDialog * dialog)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_server)))
    {
      GtkWidget * entry = GTK_COMBO (dialog->combo_server)->entry;
      dialog->server = (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
      dialog->server = XSTRDUP_OR_NULL (dialog->server);
      dialog->key_id = (gchar *) gtk_entry_get_text (GTK_ENTRY (dialog->entry_key_id));
      dialog->key_id = XSTRDUP_OR_NULL (dialog->key_id);
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_filename)))
    {
      dialog->filename = (gchar *) gtk_entry_get_text(GTK_ENTRY(dialog->entry_filename));
      dialog->filename = g_filename_from_utf8 (dialog->filename, -1, NULL, NULL, NULL);
    }
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
  GtkWidget *combo;
  GtkWidget *label;

  GPAKeyImportDialog dialog;

  dialog.filename = NULL;
  dialog.server = NULL;
  dialog.key_id = NULL;

  window = gtk_dialog_new_with_buttons (_("Import Key"),
                                        GTK_WINDOW (parent), GTK_DIALOG_MODAL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);
  dialog.window = window;
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  vbox = GTK_DIALOG (window)->vbox;

  table = gtk_table_new (4, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
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
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

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
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
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
                      gpa_options_get_default_keyserver (gpa_options));

  gpa_connect_by_accelerator (GTK_LABEL (label), entry, accel_group,
			      _("_Key Server:"));

  gtk_widget_show_all (window);
  if (gtk_dialog_run (GTK_DIALOG (window)) == GTK_RESPONSE_OK)
    {
      import_ok (&dialog);
      *filename = dialog.filename;
      *server = dialog.server;
      *key_id = dialog.key_id;
      gtk_widget_destroy (window);
      return TRUE;
    }
  else
    {
      gtk_widget_destroy (window);
      return FALSE;
    }
}

void key_import_results_dialog_run (GtkWidget *parent, GpaImportInfo *info)
{
  GtkWidget *dialog;

  if (info->count == 0)
    {
      dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_WARNING,
                                       GTK_BUTTONS_CLOSE,
                                       _("No keys were found."));
    }
  else
    {
      dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_INFO,
                                       GTK_BUTTONS_CLOSE,
                                       _("%i public keys read\n"
                                         "%i public keys imported\n"
                                         "%i public keys unchanged\n"
                                         "%i secret keys read\n"
                                         "%i secret keys imported\n"
                                         "%i secret keys unchanged"),
                                       info->count, info->imported,
                                       info->unchanged, info->sec_read,
                                       info->sec_imported, info->sec_dups);
    }			   

  /* Run the dialog */
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
