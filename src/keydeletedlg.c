/* kexdeletedlg.c - The GNU Privacy Assistant
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
#include <gpgme.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "keydeletedlg.h"

typedef struct {
  /* True, if the user clicked OK, false otherwise */
  gboolean result;

  /* The toplevel window of the dialog */
  GtkWidget * window;
} GPADeleteDialog;


/* Handler for the OK button. Set result to TRUE and destroy the dialog.
 */
static void
delete_ok (GtkWidget *widget, gpointer param)
{
  GPADeleteDialog * dialog = param;
  dialog->result = TRUE;
  gtk_widget_destroy (dialog->window);
} /* delete_ok */


/* Handler for the cancel button. Just destroy the window.
 * dialog->result is false by default */
static void
delete_cancel (gpointer param)
{
  GPADeleteDialog * dialog = param;

  gtk_widget_destroy (dialog->window);
}


/* Handler for the destroy signal of the dialog window. Just quit the
 * recursive mainloop */
static void
delete_destroy (GtkWidget *widget, gpointer param)
{
  gtk_main_quit ();
}


/* Run the delete key dialog as a modal dialog and return TRUE if the
 * user chose OK, FALSE otherwise. Display information about the public
 * key key in the dialog so that the user knows which key is to be
 * deleted. If has_secret_key is true, display a special warning for
 * deleting secret keys.
 */
gboolean
gpa_delete_dialog_run (GtkWidget * parent, GpgmeKey key,
		       gboolean has_secret_key)
{
  GtkWidget * window;
  GtkWidget * vbox;
  GtkWidget * label;
  GtkWidget * info;
  GtkWidget * bbox;
  GtkWidget * button;
  GtkAccelGroup * accel_group;

  GPADeleteDialog dialog;
  dialog.result = FALSE;

  accel_group = gtk_accel_group_new ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  dialog.window = window;
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_window_set_title (GTK_WINDOW (window), _("Remove Key"));
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (delete_destroy), &dialog);

  vbox = gtk_vbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  label = gtk_label_new (_("You have selected the following key "
			   "for removal:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  
  info = gpa_key_info_new (key, parent);
  gtk_box_pack_start (GTK_BOX (vbox), info, TRUE, TRUE, 0);

  if (has_secret_key)
    {
      label = gtk_label_new (_("This key has a secret key."
			       " Deleting this key cannot be undone,"
			       " unless you have a backup copy."));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    }
  else
    {
      label = gtk_label_new (_("This key is a public key."
			       " Deleting this key cannot be undone easily,"
			       " although you may be able to get a new copy "
			       " from the owner or from a key server."));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    }
  
  label = gtk_label_new (_("Are you sure you want to delete this key?"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  /* buttons */
  bbox = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), bbox, FALSE, TRUE, 5);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);

  button = gpa_button_new (accel_group, _("_OK"));
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc)delete_ok, &dialog);

  button = gpa_button_cancel_new (accel_group, _("_Cancel"),
				  (GtkSignalFunc)delete_cancel, &dialog);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, TRUE, 0);

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gpa_window_show_centered (window, parent);

  gtk_main ();

  return dialog.result;
} /* gpa_delete_dialog_run */
