/* keyexportdlg.c  -	 The GNU Privacy Assistant
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
#include "keyexportdlg.h"

struct _GPAKeyExportDialog {

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

  /* Whether to export ascii armored. Not used in the simple UI */
  GtkWidget * check_armored;

  /*
   * Result values:
   */

  /* True if the user clicked OK, FALSE otherwise */
  gboolean result;
  
  /* filename */
  gchar * filename;

  /* filename */
  gchar * server;

  /* armored */
  gboolean armored;
};
typedef struct _GPAKeyExportDialog GPAKeyExportDialog;


/* Handler for the browse button. Run a modal file dialog and set the
 * text of the file entry widget accordingly */
static void
export_browse (gpointer param)
{
  GPAKeyExportDialog * dialog = param;
  gchar * filename;

  filename = gpa_get_save_file_name (dialog->window,
				     _("Export public keys to file"),
				     NULL);
  if (filename)
    {
      gtk_entry_set_text (GTK_ENTRY (dialog->entry_filename),
			  filename);
      free (filename);
    }
} /* export_browse */


/* Handler for the Cancel button. Set the results to NULL and destroy
 * the main window */
void
export_cancel (gpointer param)
{
  GPAKeyExportDialog * dialog = param;

  dialog->filename = NULL;
  dialog->server = NULL;
  gtk_widget_destroy (dialog->window);
} /* export_cancel */


/* Handler for the OK button. Extract the user input from the widgets
 * and destroy the main window */
void
export_ok (gpointer param)
{
  GPAKeyExportDialog * dialog = param;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_server)))
    {
      GtkWidget * entry = GTK_COMBO (dialog->combo_server)->entry;
      dialog->server = gtk_entry_get_text (GTK_ENTRY (entry));
      dialog->server = xstrdup (dialog->server);
    }
  else
    {
      dialog->filename = gtk_entry_get_text(GTK_ENTRY(dialog->entry_filename));
      dialog->filename = xstrdup (dialog->filename);
    }

  if (dialog->check_armored)
    {
      /* the armored toggle button is only present in advanced UI mode
       */
      GtkToggleButton * toggle = GTK_TOGGLE_BUTTON (dialog->check_armored);
      dialog->armored = gtk_toggle_button_get_active(toggle);
    }

  dialog->result = TRUE;
  gtk_widget_destroy (dialog->window);
} /* export_ok */


/* signal handler for the destroy signal. Quit the recursive main loop
 */
static void
export_destroy (GtkWidget *widget, gpointer param)
{
  gtk_main_quit ();
}


/* Run the key export dialog as a modal dialog and return TRUE if the
 * user clicked OK, otherwise return FALSE.
 */
gboolean
key_export_dialog_run (GtkWidget * parent, gchar ** filename,
		       gchar ** keyserver, gboolean *armored)
{
  GtkAccelGroup *accel_group;

  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *radio;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *check;
  GtkWidget *bbox;
  GtkWidget *combo;
  GPAKeyExportDialog dialog;
  GList * servers;
  int i;

  dialog.result = FALSE;
  dialog.filename = NULL;
  dialog.server = NULL;
  dialog.armored = 1;
  
  window = gtk_window_new (GTK_WINDOW_DIALOG);
  dialog.window = window;
  gtk_window_set_title (GTK_WINDOW (window), _("Export keys"));
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (export_destroy), &dialog);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  table = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);

  /* File name entry */
  radio = gpa_radio_button_new_from_widget (NULL, accel_group,
					    _("E_xport to file:"));
  dialog.radio_filename = radio;
  gtk_table_attach (GTK_TABLE (table), radio, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

  entry = gtk_entry_new ();
  dialog.entry_filename = entry;
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, 
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (entry), "activate",
			     GTK_SIGNAL_FUNC (export_ok), (gpointer) &dialog);

  button = gpa_button_new (accel_group, _("B_rowse..."));
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (export_browse),
			     (gpointer) &dialog);

  /* Server combo */
  radio = gpa_radio_button_new_from_widget (GTK_RADIO_BUTTON (radio),
					    accel_group, _("_Key server:"));
  dialog.radio_server = radio;
  gtk_table_attach (GTK_TABLE (table), radio, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  combo = gtk_combo_new ();
  dialog.combo_server = combo;
  gtk_table_attach (GTK_TABLE (table), combo, 1, 3, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_combo_set_value_in_list (GTK_COMBO (combo), FALSE, FALSE);
  servers = NULL;
  for (i = 0; gpa_options.keyserver_names[i]; i++)
    {
      servers = g_list_append (servers, gpa_options.keyserver_names[i]);
    }

  gtk_combo_set_popdown_strings (GTK_COMBO (combo), servers);
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry),
		      global_keyserver);

  if (!gpa_simplified_ui ())
    {
      check = gpa_check_button_new (accel_group, _("a_rmor"));
      gtk_container_set_border_width (GTK_CONTAINER (check), 5);
      gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
      dialog.check_armored = check;
    }
  else
    dialog.check_armored = NULL;


  /* The button box */
  bbox = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 10);
  gtk_container_set_border_width (GTK_CONTAINER (bbox), 5);

  button = gpa_button_cancel_new (accel_group, _("_Cancel"), export_cancel,
				  &dialog);
  gtk_container_add (GTK_CONTAINER (bbox), button);

  button = gpa_button_new (accel_group, _("_OK"));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (export_ok), (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (bbox), button);

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gpa_window_show_centered (window, parent);

  gtk_main ();

  if (dialog.result)
    {
      *filename = dialog.filename;
      *keyserver = dialog.server;
      *armored = dialog.armored;
    }

  return dialog.result;
} /* keyring_export_dialog */
