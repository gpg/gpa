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
  gchar * passphrase;
  GtkWidget * window;
  GtkWidget * check_local;
};
typedef struct _GPAKeySignDialog GPAKeySignDialog;


/* Signal handle for the cancel button. Set result to FALSE and destroy
 * the dialog window */
static void
key_sign_cancel (gpointer param)
{
  GPAKeySignDialog * dialog = param;

  dialog->result = FALSE;
  gtk_widget_destroy (dialog->window);
} /* key_sign_cancel */


/* Signal handler for the OK button. Read the user's input and ask for
 * the password, then destroy the dialog window */
static void
key_sign_ok (gpointer param)
{
  GPAKeySignDialog * dialog = param;
  GtkWidget * check;

  dialog->result = TRUE;

  check = dialog->check_local;
  if (check && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)))
    dialog->sign_type = GPAPA_KEY_SIGN_LOCALLY;
  else
    dialog->sign_type = GPAPA_KEY_SIGN_NORMAL;

  dialog->passphrase = gpa_passphrase_run_dialog (dialog->window);

  gtk_widget_destroy (dialog->window);
} /* key_sign_ok */


/* Handle for the dialog window's destroy signal. Quit the recursive
 * main loop */
static void
key_sign_destroy (GtkWidget *widget, gpointer param)
{
  gtk_main_quit ();
}


/* Run the key sign dialog as a modal dialog and return when the user
 * ends the dialog. If the user clicks OK, return TRUE and set sign_type
 * according to the "sign locally" check box and *passphrase to the
 * passphrase. The passphrase must be freed.
 *
 * If the user clicked Cancel, return FALSE and do not modify *sign_type
 * or *passphrase.
 *
 * When in simplified_ui mode, don't show the "sign locally" check box
 * and if the user clicks OK, set *sign_type to normal.
 */
gboolean
gpa_key_sign_run_dialog (GtkWidget * parent, GpapaPublicKey *key,
			 GpapaSignType * sign_type,
			 gchar ** passphrase)
{
  GtkAccelGroup *accelGroup;
  GtkWidget *window;
  GtkWidget *vboxSign;
  GtkWidget *hButtonBoxSign;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSign;
  GtkWidget *check = NULL;
  GtkWidget *table;
  GtkWidget *label;

  GPAKeySignDialog dialog;

  dialog.sign_type = *sign_type;
  dialog.passphrase = NULL;
  dialog.check_local = NULL;

  window = gtk_window_new (GTK_WINDOW_DIALOG);
  dialog.window = window;
  gtk_window_set_title (GTK_WINDOW (window), _("Sign Key"));
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (key_sign_destroy), (gpointer)&dialog);

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accelGroup);

  vboxSign = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vboxSign);
  gtk_container_set_border_width (GTK_CONTAINER (vboxSign), 5);

  label = gtk_label_new (_("Do you want to sign the following key?"));
  gtk_box_pack_start (GTK_BOX (vboxSign), label, FALSE, TRUE, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vboxSign), table, FALSE, TRUE, 10);
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

  label = gtk_label_new (_("Check the name and fingerprint carefully to"
		   " be sure that it really is the key you want to sign."));
  gtk_box_pack_start (GTK_BOX (vboxSign), label, FALSE, TRUE, 10);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  label = gtk_label_new (_("The key will be signed with your default"
			   " private key."));
  gtk_box_pack_start (GTK_BOX (vboxSign), label, FALSE, TRUE, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  if (!gpa_simplified_ui ())
    {
      check = gpa_check_button_new (accelGroup, _("Sign only _locally"));
      dialog.check_local = check;
      gtk_box_pack_start (GTK_BOX (vboxSign), check, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
				    *sign_type == GPAPA_KEY_SIGN_LOCALLY);
    }

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

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gpa_window_show_centered (window, parent);

  dialog.result = FALSE;

  gtk_main ();

  if (dialog.result && dialog.passphrase)
    {
      *sign_type = dialog.sign_type;
      *passphrase = dialog.passphrase;
    }
  else
    {
      free (dialog.passphrase);
      dialog.result = FALSE;
    }

  return dialog.result;
} /* gpa_key_sign_run_dialog */
