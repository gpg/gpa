/* keyexportdlg.c  -	 The GNU Privacy Assistant
 *	Copyright (C) 2000 G-N-U GmbH.
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


struct _GPAKeyExportDialog {
  GtkWidget * window;
  GtkWidget * entry_filename;
  GtkWidget * check_armored;
  gchar * filename;
  gboolean armored;
};
typedef struct _GPAKeyExportDialog GPAKeyExportDialog;

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
}

void
export_cancel (gpointer param)
{
  GPAKeyExportDialog * dialog = param;

  dialog->filename = NULL;
  gtk_main_quit ();
}

void
export_ok (gpointer param)
{
  GPAKeyExportDialog * dialog = param;

  dialog->filename = gtk_entry_get_text (GTK_ENTRY (dialog->entry_filename));
  dialog->filename = xstrdup (dialog->filename);
  dialog->armored \
    = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->check_armored));

  gtk_main_quit ();
}


gboolean
key_export_dialog_run (GtkWidget * parent, gchar ** filename,
		       gboolean *armored)
{
  GtkAccelGroup *accelGroup;

  GtkWidget *windowExport;
  GtkWidget *vboxExport;
  GtkWidget *hboxFilename;
  GtkWidget *labelFilename;
  GtkWidget *entryFilename;
  GtkWidget *spaceBrowse;
  GtkWidget *buttonBrowse;
  GtkWidget *checkerArmor;
  GtkWidget *hButtonBoxExport;
  GtkWidget *buttonCancel;
  GtkWidget *buttonExport;
  GPAKeyExportDialog dialog;

  dialog.armored = 1;
  dialog.filename = NULL;
  
  windowExport = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (windowExport), _("Export keys"));
  dialog.window = windowExport;

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowExport), accelGroup);

  vboxExport = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxExport), 5);
  hboxFilename = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxFilename), 5);
  labelFilename = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hboxFilename), labelFilename, FALSE, FALSE, 0);
  entryFilename = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hboxFilename), entryFilename, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (entryFilename), "activate",
			     GTK_SIGNAL_FUNC (export_ok),
			     (gpointer) &dialog);
  gpa_connect_by_accelerator (GTK_LABEL (labelFilename), entryFilename,
			      accelGroup, _("Export to _file:"));
  spaceBrowse = gpa_space_new ();
  gtk_box_pack_start (GTK_BOX (hboxFilename), spaceBrowse, FALSE, FALSE, 5);
  buttonBrowse = gpa_button_new (accelGroup, _("_Browse..."));
  gtk_signal_connect_object (GTK_OBJECT (buttonBrowse), "clicked",
			     GTK_SIGNAL_FUNC (export_browse),
			     (gpointer) &dialog);
  gtk_box_pack_start (GTK_BOX (hboxFilename), buttonBrowse, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxExport), hboxFilename, TRUE, TRUE, 0);
  dialog.entry_filename = entryFilename;

  checkerArmor = gpa_check_button_new (accelGroup, _("a_rmor"));
  gtk_container_set_border_width (GTK_CONTAINER (checkerArmor), 5);
  gtk_box_pack_start (GTK_BOX (vboxExport), checkerArmor, FALSE, FALSE, 0);
  dialog.check_armored = checkerArmor;

  hButtonBoxExport = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxExport),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxExport), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxExport), 5);

  buttonCancel = gpa_button_cancel_new (accelGroup, _("_Cancel"),
					export_cancel, &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxExport), buttonCancel);
  buttonExport = gpa_button_new (accelGroup, _("Ok"));
  gtk_signal_connect_object (GTK_OBJECT (buttonExport), "clicked",
			     GTK_SIGNAL_FUNC (export_ok),
			     (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxExport), buttonExport);
  gtk_box_pack_start (GTK_BOX (vboxExport), hButtonBoxExport, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowExport), vboxExport);
  gpa_widget_show (windowExport, parent, _("keyring_openPublic_export.tip"));
  gtk_widget_grab_focus (entryFilename);

  gtk_grab_add (windowExport);
  gtk_main ();
  gtk_grab_remove (windowExport);
  gtk_widget_destroy (windowExport);

  if (dialog.filename)
    {
      *filename = dialog.filename;
      *armored = dialog.armored;
    }
  return dialog.filename != NULL;
}				/* keyring_export_dialog */
