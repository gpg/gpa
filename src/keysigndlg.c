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
#include <gpgme.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "keysigndlg.h"


/* Run the key sign dialog for signing the public key key with the
 * default key as a modal dialog and return when the user ends the
 * dialog. If the user clicks OK, return TRUE and set sign_locally
 * according to the "sign locally" check box.
 *
 * If the user clicked Cancel, return FALSE and do not modify *sign_locally
 *
 * When in simplified_ui mode, don't show the "sign locally" check box
 * and if the user clicks OK, set *sign_locally to false.
 */
gboolean
gpa_key_sign_run_dialog (GtkWidget * parent, GpgmeKey key,
                         gboolean * sign_locally)
{
  GtkAccelGroup *accelGroup;
  GtkWidget *window;
  GtkWidget *vboxSign;
  GtkWidget *check = NULL;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *uid_box;
  GtkResponseType response;
  gint uid_count;
  gchar *string;

  window = gtk_dialog_new_with_buttons (_("Sign Key"), GTK_WINDOW(parent),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_YES,
                                        GTK_RESPONSE_YES,
                                        GTK_STOCK_NO,
                                        GTK_RESPONSE_NO,
                                        NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_YES);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accelGroup);

  vboxSign = GTK_DIALOG (window)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vboxSign), 5);

  label = gtk_label_new (_("Do you want to sign the following key?"));
  gtk_box_pack_start (GTK_BOX (vboxSign), label, FALSE, TRUE, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vboxSign), table, FALSE, TRUE, 10);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);

  /* Build this first, so that we can know how may user ID's there are */
  uid_box = gtk_vbox_new (TRUE, 0);
  for (uid_count = 0; 
       gpgme_key_get_string_attr (key, GPGME_ATTR_USERID, NULL, uid_count);
       uid_count++)
    {
      if (!gpgme_key_get_ulong_attr (key, GPGME_ATTR_UID_REVOKED, NULL, 
                                     uid_count))
        {
          string = gpa_gpgme_key_get_userid (key, uid_count);
          label = gtk_label_new (string);
          g_free (string);
          gtk_box_pack_start_defaults (GTK_BOX(uid_box), label);
          gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        }
    }

  label = gtk_label_new ( uid_count == 1 ? _("User Name:") : _("User Names:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);

  gtk_table_attach (GTK_TABLE (table), uid_box, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
  label = gtk_label_new (_("Fingerprint:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  
  string = gpa_gpgme_key_get_fingerprint (key, 0);
  label = gtk_label_new (string);
  g_free (string);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  label = gtk_label_new (_("Check the name and fingerprint carefully to"
		   " be sure that it really is the key you want to sign."));
  gtk_box_pack_start (GTK_BOX (vboxSign), label, FALSE, TRUE, 10);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  if (uid_count > 1)
    {
      label = gtk_label_new (_("All user names in this key will be signed."));
      gtk_box_pack_start (GTK_BOX (vboxSign), label, FALSE, TRUE, 10);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    }

  label = gtk_label_new (_("The key will be signed with your default"
			   " private key."));
  gtk_box_pack_start (GTK_BOX (vboxSign), label, FALSE, TRUE, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  if (!gpa_options_get_simplified_ui (gpa_options))
    {
      check = gpa_check_button_new (accelGroup, _("Sign only _locally"));
      gtk_box_pack_start (GTK_BOX (vboxSign), check, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), *sign_locally);
    }

  gtk_widget_show_all (window);
  response = gtk_dialog_run (GTK_DIALOG (window));
  if (response == GTK_RESPONSE_YES)
    {
      *sign_locally = check && 
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
      gtk_widget_destroy (window);
      return TRUE;
    }
  else
    {
      gtk_widget_destroy (window);
      return FALSE;
    }
}
