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
#include "keyreceivedlg.h"

struct _GPAKeyReceiveDialog {
  GtkWidget * window;
  GtkWidget * entry;
};
typedef struct _GPAKeyReceiveDialog GPAKeyReceiveDialog;


static void
receive_ok (gpointer param)
{
  GPAKeyReceiveDialog * dialog = param;
  gchar *key_id;

  global_lastCallbackResult = GPAPA_ACTION_NONE;
  key_id = gtk_entry_get_text (GTK_ENTRY (dialog->entry));
  gpapa_receive_public_key_from_server (key_id, global_keyserver, gpa_callback,
					dialog->window);
  if (global_lastCallbackResult == GPAPA_ACTION_ERROR)
    {
      gpa_window_error (_("An error occured while receiving\n"
			  "the requested key from the keyserver."),
			dialog->window);
      return;
    }				/* if */
  gtk_main_quit ();
}				/* keyring_openPublic_receive_receive */

static void
receive_cancel (gpointer param)
{
  gtk_main_quit ();
}

static gboolean
receive_delete_event (GtkWidget *widget, GdkEvent *event, gpointer param)
{
  receive_cancel (param);
  return TRUE;
}


void
key_receive_run_dialog (GtkWidget * parent)
{
  GtkAccelGroup *accelGroup;
  GtkWidget *windowReceive;
  GtkWidget *vboxReceive;
  GtkWidget *hboxKey;
  GtkWidget *labelKey;
  GtkWidget *entryKey;
  GtkWidget *hButtonBoxReceive;
  GtkWidget *buttonCancel;
  GtkWidget *buttonReceive;
  GPAKeyReceiveDialog dialog;

  windowReceive = gtk_window_new (GTK_WINDOW_DIALOG);
  dialog.window = windowReceive;
  gtk_window_set_title (GTK_WINDOW (windowReceive),
			_("Receive key from server"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowReceive), accelGroup);
  gtk_signal_connect_object (GTK_OBJECT (windowReceive), "delete_event",
			     GTK_SIGNAL_FUNC (receive_delete_event),
			     (gpointer) &dialog);

  vboxReceive = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxReceive), 5);

  hboxKey = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxKey), 5);

  labelKey = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hboxKey), labelKey, FALSE, FALSE, 0);
  entryKey = gtk_entry_new ();
  dialog.entry = entryKey;
  gtk_signal_connect_object (GTK_OBJECT (entryKey), "activate",
			     GTK_SIGNAL_FUNC (receive_ok),
			     (gpointer) &dialog);
  gtk_box_pack_start (GTK_BOX (hboxKey), entryKey, TRUE, TRUE, 0);
  gpa_connect_by_accelerator (GTK_LABEL (labelKey), entryKey, accelGroup,
			      _("_Key ID: "));
  gtk_box_pack_start (GTK_BOX (vboxReceive), hboxKey, TRUE, TRUE, 0);

  hButtonBoxReceive = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxReceive),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxReceive), 10);
  buttonCancel = gpa_button_cancel_new (accelGroup, _("_Cancel"),
					receive_cancel, &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxReceive), buttonCancel);

  buttonReceive = gpa_button_new (accelGroup, _("_Receive"));
  gtk_container_add (GTK_CONTAINER (hButtonBoxReceive), buttonReceive);
  gtk_signal_connect_object (GTK_OBJECT (buttonReceive), "clicked",
			     GTK_SIGNAL_FUNC (receive_ok),
			     (gpointer) &dialog);
  gtk_box_pack_start (GTK_BOX (vboxReceive), hButtonBoxReceive, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowReceive), vboxReceive);
  gpa_window_show_centered (windowReceive, parent);
  gtk_widget_grab_focus (entryKey);

  gtk_grab_add (windowReceive);
  gtk_main ();
  gtk_grab_remove (windowReceive);
  gtk_widget_destroy (windowReceive);
}				/* keyring_openPublic_receive */
