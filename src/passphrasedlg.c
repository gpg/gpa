/* passphrasedlg.c  -  The GNU Privacy Assistant
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gtktools.h"
#include "passphrasedlg.h"

struct _GPAPassphraseDialog {
  gchar * passphrase;
  GtkWidget * entry;
  
};
typedef struct _GPAPassphraseDialog GPAPassphraseDialog;


static void
passphrase_ok (gpointer param)
{
  GPAPassphraseDialog * dialog = param;

  dialog->passphrase = xstrdup (gtk_entry_get_text (GTK_ENTRY(dialog->entry)));
  gtk_main_quit ();
}

static void
passphrase_cancel (gpointer param)
{
  GPAPassphraseDialog * dialog = param;

  dialog->passphrase = NULL;
  gtk_main_quit ();
}

static gboolean
passphrase_delete_event (GtkWidget *widget, GdkEvent *event, gpointer param)
{
  passphrase_cancel (param);
  return TRUE;
}

gchar *
gpa_passphrase_run_dialog (GtkWidget * parent)
{
  GtkAccelGroup *accelGroup;
  GtkWidget *windowPassphrase;
  GtkWidget *vboxPassphrase;
  GtkWidget *hboxPasswd;
  GtkWidget *labelPasswd;
  GtkWidget *entryPasswd;
  GtkWidget *hButtonBoxPassphrase;
  GtkWidget *buttonCancel;
  GtkWidget *buttonOK;
  GPAPassphraseDialog dialog;

  dialog.passphrase = NULL;

  windowPassphrase = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (windowPassphrase), _("Enter Password"));
  gtk_signal_connect_object (GTK_OBJECT (windowPassphrase), "delete_event",
			     GTK_SIGNAL_FUNC (passphrase_delete_event),
			     (gpointer)&dialog);

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowPassphrase), accelGroup);

  vboxPassphrase = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxPassphrase), 5);

  hboxPasswd = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxPasswd), 5);

  labelPasswd = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hboxPasswd), labelPasswd, FALSE, FALSE, 0);
  entryPasswd = dialog.entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entryPasswd), FALSE);
  gtk_signal_connect_object (GTK_OBJECT (entryPasswd), "activate",
			     GTK_SIGNAL_FUNC (passphrase_ok),
			     (gpointer) &dialog);
  gtk_box_pack_start (GTK_BOX (hboxPasswd), entryPasswd, TRUE, TRUE, 0);
  gpa_connect_by_accelerator (GTK_LABEL (labelPasswd), entryPasswd,
			      accelGroup, _("_Password: "));
  gtk_box_pack_start (GTK_BOX (vboxPassphrase), hboxPasswd, TRUE, TRUE, 0);

  hButtonBoxPassphrase = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxPassphrase),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxPassphrase), 10);
  buttonCancel = gpa_button_cancel_new (accelGroup, _("_Cancel"),
					passphrase_cancel, (gpointer) &dialog);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxPassphrase), 5);
  gtk_container_add (GTK_CONTAINER (hButtonBoxPassphrase), buttonCancel);
  buttonOK = gpa_button_new (accelGroup, _("_OK"));
  gtk_signal_connect_object (GTK_OBJECT (buttonOK), "clicked",
			     GTK_SIGNAL_FUNC (passphrase_ok),
			     (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxPassphrase), buttonOK);
  gtk_box_pack_start (GTK_BOX (vboxPassphrase), hButtonBoxPassphrase, FALSE,
		      FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowPassphrase), vboxPassphrase);
  gtk_widget_show_all (windowPassphrase);

  gtk_grab_add (windowPassphrase);
  gtk_main ();
  gtk_grab_remove (windowPassphrase);
  gtk_widget_destroy (windowPassphrase);

  return dialog.passphrase;
}				/* gpa_window_passphrase */
