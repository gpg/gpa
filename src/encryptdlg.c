/* encryptdlg.c  -  The GNU Privacy Assistant
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <gpapa.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"
#include "filemenu.h"
#include "keysmenu.h"
#include "help.h"

static gchar *
target_file_name (const gchar * filename, GpapaArmor armor)
{
  gchar *extension = NULL;
  gchar *new_filename;

  switch (armor)
    {
    case GPAPA_ARMOR:
      extension = ".asc";
      break;
    case GPAPA_NO_ARMOR:
      extension = ".gpg";
      break;
    } /* switch */
  new_filename = xstrcat2 (filename, extension);
  return new_filename;
} /* target_file_name */


struct _GPAFileEncryptDialog {
  GtkWidget *window;
  GtkWidget *clist_keys;
  GtkWidget *check_sign;
  GtkWidget *check_armor;
  GList *files;
  GList *encrypted_files;
};
typedef struct _GPAFileEncryptDialog GPAFileEncryptDialog;

void
file_encrypt_ok (gpointer param)
{
  GPAFileEncryptDialog *dialog = param;

  GList *recipients;
  GpapaArmor armor;
  gchar *target;
  GList *cur;
  GpapaFile *file;

  recipients = gpa_key_list_selected_ids (dialog->clist_keys);
  if (!recipients)
    {
      gpa_window_error (_("No recipients chosen to encrypt for."),
			dialog->window);
      return;
    } /* if */

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->check_armor)))
    armor = GPAPA_ARMOR;
  else
    armor = GPAPA_NO_ARMOR;

  cur = dialog->files;
  while (cur)
    {
      file = cur->data;
      target = target_file_name (gpapa_file_get_identifier (file, gpa_callback,
							    dialog->window),
				 armor);
      global_lastCallbackResult = GPAPA_ACTION_NONE;
      gpapa_file_encrypt (file, target, recipients, armor, gpa_callback,
			  dialog->window);
      if (global_lastCallbackResult != GPAPA_ACTION_ERROR)
	dialog->encrypted_files = g_list_prepend (dialog->encrypted_files,
						  target);
      else
	free (target);
      cur = g_list_next (cur);
    }
  g_list_free (recipients);
  gtk_main_quit ();
} /* file_encrypt_ok */

static void
file_encrypt_cancel (gpointer param)
{
  gtk_main_quit ();
}

GList *
gpa_file_encrypt_dialog_run (GtkWidget *parent, GList *files)
{
  GtkAccelGroup *accelGroup;
  GtkWidget *window;
  GtkWidget *vboxEncrypt;
  GtkWidget *labelKeys;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;
  GtkWidget *checkerSign;
  GtkWidget *checkerArmor;
  GtkWidget *hButtonBoxEncrypt;
  GtkWidget *buttonCancel;
  GtkWidget *buttonEncrypt;

  GPAFileEncryptDialog dialog;

  if (!gpapa_get_public_key_count (gpa_callback, parent))
    {
      gpa_window_error (_("No public keys available.\n"
			  "Currently, there is nobody who could read a\n"
			  "file encrypted by you."),
			parent);
      return NULL;
    } /* if */
  if (!gpapa_get_secret_key_count (gpa_callback, parent))
    {
      gpa_window_error (_("No secret keys available to encrypt."),
			parent);
      return NULL;
    } /* if */

  dialog.files = files;
  dialog.encrypted_files = NULL;

  window = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_window_set_title (GTK_WINDOW (window), _("Encrypt files"));
  dialog.window = window;

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accelGroup);

  vboxEncrypt = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxEncrypt), 5);
  gtk_container_add (GTK_CONTAINER (window), vboxEncrypt);

  labelKeys = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelKeys), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), labelKeys, FALSE, FALSE, 0);

  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), scrollerKeys, TRUE, TRUE, 0);
  gtk_widget_set_usize (scrollerKeys, 300, 120);

  clistKeys = gpa_public_key_list_new (parent);
  dialog.clist_keys = clistKeys;
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys), GTK_SELECTION_EXTENDED);
  gpa_connect_by_accelerator (GTK_LABEL (labelKeys), clistKeys, accelGroup,
			      _("_Public Keys"));


  checkerSign = gpa_check_button_new (accelGroup, _("_Sign"));
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), checkerSign, FALSE, FALSE, 0);
  dialog.check_sign = checkerSign;

  checkerArmor = gpa_check_button_new (accelGroup, _("A_rmor"));
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), checkerArmor, FALSE, FALSE, 0);
  dialog.check_armor = checkerArmor;

  hButtonBoxEncrypt = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), hButtonBoxEncrypt, FALSE, FALSE,
		      0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxEncrypt),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxEncrypt), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxEncrypt), 5);

  buttonCancel = gpa_button_cancel_new (accelGroup, _("_Cancel"),
					file_encrypt_cancel,
					(gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEncrypt), buttonCancel);

  buttonEncrypt = gpa_button_new (accelGroup, _("_OK"));
  gtk_signal_connect_object (GTK_OBJECT (buttonEncrypt), "clicked",
			     GTK_SIGNAL_FUNC (file_encrypt_ok),
			     (gpointer) &dialog);
  gtk_container_add (GTK_CONTAINER (hButtonBoxEncrypt), buttonEncrypt);

  gpa_widget_show (window, parent, _("file_encrypt.tip"));

  gtk_grab_add (window);
  gtk_main ();
  gtk_grab_remove (window);
  gtk_widget_destroy (window);

  return dialog.encrypted_files;
} /* file_encrypt_dialog */
