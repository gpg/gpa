/* keysigndlg.c  -  The GNU Privacy Assistant
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gpapa.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "passphrasedlg.h"
#include "keysigndlg.h"

struct _GPAKeySignDialog {
  gboolean result;
  GpapaSignType sign_type;
  gchar * key_id;
  gchar * passphrase;
  GtkWidget * window;
  GtkWidget * check_local;
  GtkCList * clist_keys;
};
typedef struct _GPAKeySignDialog GPAKeySignDialog;

static void
key_sign_cancel (gpointer param)
{
  GPAKeySignDialog * dialog = param;

  dialog->result = FALSE;
  gtk_main_quit ();
} /* key_sign_cancel */

static gboolean
key_sign_delete (GtkWidget *widget, GdkEvent *event, gpointer param)
{
  key_sign_cancel (param);
  return FALSE;
}


static void
key_sign_ok (gpointer param)
{
  GPAKeySignDialog * dialog = param;
  gint row;

  dialog->result = TRUE;
  if (dialog->clist_keys->selection)
    {
      row = GPOINTER_TO_INT (dialog->clist_keys->selection->data);
      dialog->key_id = gtk_clist_get_row_data (dialog->clist_keys, row);
      dialog->key_id = xstrdup (dialog->key_id);

      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->check_local)))
	dialog->sign_type = GPAPA_KEY_SIGN_LOCALLY;
      else
	dialog->sign_type = GPAPA_KEY_SIGN_NORMAL;
      
      dialog->passphrase = gpa_passphrase_run_dialog (dialog->window);
    }
  gtk_main_quit ();
} /* key_sign_ok */



gboolean
gpa_key_sign_run_dialog (GtkWidget * parent, GpapaPublicKey *key,
			 GpapaSignType * sign_type,
			 gchar ** key_id, gchar ** passphrase)
{
  GtkAccelGroup *accelGroup;
  GtkWidget *window;
  GtkWidget *vboxSign;
  GtkWidget *vboxWho;
  GtkWidget *labelWho;
  GtkWidget *scrollerWho;
  GtkWidget *clistWho;
  GtkWidget *hButtonBoxSign;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSign;
  GtkWidget *check;
  GtkWidget *table;
  GtkWidget *label;

  GPAKeySignDialog dialog;

  dialog.sign_type = *sign_type;
  dialog.key_id = NULL;
  dialog.passphrase = NULL;

  if (gpapa_get_secret_key_count (gpa_callback, parent) == 0)
    {
      gpa_window_error (_("No secret keys available for signing.\n"
			  "Import a secret key first!"),
			parent);
      return FALSE;
    } /* if */

  window = gtk_window_new (GTK_WINDOW_DIALOG);
  dialog.window = window;
  gtk_window_set_title (GTK_WINDOW (window), _("Sign Keys"));
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (key_sign_delete),
		      (gpointer)&dialog);

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accelGroup);

  vboxSign = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vboxSign);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSign), 5);

  label = gtk_label_new (_("Do you want to sign the following key?"));
  gtk_box_pack_start (GTK_BOX (vboxSign), label, FALSE, TRUE, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vboxSign), table, FALSE, TRUE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);

  label = gtk_label_new (_("User ID:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);

  label = gtk_label_new (gpapa_key_get_name (GPAPA_KEY (key), gpa_callback,
					     parent));
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  label = gtk_label_new (_("Fingerprint:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  
  label = gtk_label_new (gpapa_public_key_get_fingerprint (key, gpa_callback,
							   parent));
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  vboxWho = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vboxSign), vboxWho, TRUE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxWho), 5);

  labelWho = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelWho), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vboxWho), labelWho, FALSE, TRUE, 0);

  scrollerWho = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vboxWho), scrollerWho, TRUE, TRUE, 0);
  gtk_widget_set_usize (scrollerWho, 260, 75);

  clistWho = gpa_secret_key_list_new (parent);
  gtk_container_add (GTK_CONTAINER (scrollerWho), clistWho);
  dialog.clist_keys = GTK_CLIST (clistWho);
  gpa_connect_by_accelerator (GTK_LABEL (labelWho), clistWho, accelGroup,
			      _("Sign _as "));

  check = gpa_check_button_new (accelGroup, _("Sign only _locally"));
  dialog.check_local = check;
  gtk_box_pack_start (GTK_BOX (vboxSign), check, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
				*sign_type == GPAPA_KEY_SIGN_LOCALLY);

  hButtonBoxSign = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vboxSign), hButtonBoxSign, FALSE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxSign),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxSign), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxSign), 5);

  buttonCancel = gpa_button_cancel_new (accelGroup, _("_Cancel"),
					key_sign_cancel, &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSign), buttonCancel);

  buttonSign = gpa_button_new (accelGroup, _("_OK"));
  gtk_signal_connect_object (GTK_OBJECT (buttonSign), "clicked",
			     GTK_SIGNAL_FUNC (key_sign_ok),
			     (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxSign), buttonSign);
  gpa_window_show_centered (window, parent);
  /* set tip to "key_sign.tip" */

  gtk_grab_add (window);
  gtk_main ();
  gtk_grab_remove (window);
  gtk_widget_destroy (window);

  if (dialog.result && dialog.key_id && dialog.passphrase)
    {
      *sign_type = dialog.sign_type;
      *key_id = dialog.key_id;
      *passphrase = dialog.passphrase;
    }
  else
    {
      free (dialog.key_id);
      free (dialog.passphrase);
      dialog.result = FALSE;
    }

  return dialog.result;
} /* gpa_key_sign_run_dialog */
