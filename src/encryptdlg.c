/* encryptdlg.c  -  The GNU Privacy Assistant
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
#include <errno.h>
#include "gpa.h"
#include "gtktools.h"
#include "gpawidgets.h"

struct _GPAFileEncryptDialog {
  GtkWidget *window;
  GtkWidget *clist_keys;
  GtkWidget *check_sign;
  GtkWidget *check_armor;
  GList *files;
  GList *encrypted_files;
};
typedef struct _GPAFileEncryptDialog GPAFileEncryptDialog;


static FILE *
open_destination_file (const gchar *filename, gboolean armor,
		       gchar **target_filename, GtkWidget *parent)
{
  const gchar *extension;
  FILE *target;
  
  if (!armor)
    {
      extension = ".gpg";
    }
  else
    {
      extension = ".asc";
    }
  *target_filename = g_strconcat (filename, extension, NULL);
  target = gpa_fopen (*target_filename, parent);
  if (!target)
    {
      g_free (*target_filename);
    }
  return target;
}

static GtkResponseType
ignore_key_trust (GpgmeKey key, GtkWidget *parent)
{
  GtkWidget *dialog;
  GtkWidget *key_info;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *image;
  GtkResponseType response;

  dialog = gtk_dialog_new_with_buttons (_("Unknown Key"), GTK_WINDOW(parent),
					GTK_DIALOG_MODAL, 
					GTK_STOCK_YES, GTK_RESPONSE_YES,
					GTK_STOCK_NO, GTK_RESPONSE_NO,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  hbox = gtk_hbox_new (FALSE, 6);
  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
				    GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox);

  label = gtk_label_new (_("You are going to encrypt a file using "
			   "the following key:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5);
  key_info = gpa_key_info_new (key, dialog);
  gtk_box_pack_start (GTK_BOX (vbox), key_info, FALSE, TRUE, 5);
  label = gtk_label_new (_("However, it is not certain that the key belongs "
			   "to that person."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), 
			_("Do you <b>really</b> want to use this key?"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5);

  gtk_widget_show_all (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);
  return response;
}

static gboolean
set_recipients (GList *recipients, GpgmeRecipients *rset, GtkWidget *parent)
{
  GList *cur;
  GpgmeError err;

  err = gpgme_recipients_new (rset);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  for (cur = recipients; cur; cur = g_list_next (cur))
    {
      /* Check that all recipients are valid */
      gchar *fpr = cur->data;
      GpgmeKey key = gpa_keytable_lookup (keytable, fpr);
      GpgmeValidity valid;
      if (!key)
	{
	  /* Can't happen */
	  gpa_window_error (_("The key you selected is not available for "
			      "encryption"), parent);
	  return FALSE;
	}
      valid = gpgme_key_get_ulong_attr (key, GPGME_ATTR_VALIDITY, NULL, 0);
      if (valid == GPGME_VALIDITY_FULL || valid == GPGME_VALIDITY_ULTIMATE)
	{
	  gpgme_recipients_add_name_with_validity (*rset, cur->data, valid);
	}
      else
	{
	  /* If an untrusted key is found ask the user what to do */
	  GtkResponseType response;

	  response = ignore_key_trust (key, parent);
	  if (response == GTK_RESPONSE_YES)
	    {
	      /* Assume the key is trusted */
	      gpgme_recipients_add_name_with_validity (*rset, cur->data,
						       GPGME_VALIDITY_FULL);
	    }
	  else
	    {
	      /* Abort the encryption */
	      return FALSE;
	    }
	}
      /* If we arrive here the key was added, so check for errors */
      if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
    }
  return TRUE;
}

static gboolean
set_signers (GList *signers, GtkWidget *parent)
{
  GList *cur;
  GpgmeError err;

  gpgme_signers_clear (ctx);
  if (!signers)
    {
      /* Can't happen */
      gpa_window_error (_("You didn't select any key for signing"), parent);
      return FALSE;
    }
  for (cur = signers; cur; cur = g_list_next (cur))
    {
      GpgmeKey key = gpa_keytable_secret_lookup (keytable, cur->data);
      if (!key)
	{
	  /* Can't happen */
	  gpa_window_error (_("The key you selected is not available for "
			      "signing"), parent);
	  break;
	}
      err = gpgme_signers_add (ctx, key);
      if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
    }
  return TRUE;
}

static gchar *
encrypt_file (const gchar *filename, GpgmeRecipients rset, gboolean sign,
	      gboolean armor, GtkWidget *parent)
{
  GpgmeError err;
  GpgmeData input, output;
  FILE *target;
  gchar *target_filename;
  
  target = open_destination_file (filename, armor, &target_filename,
				  parent);
  if (!target)
    return NULL;

  /* Create the appropiate GpgmeData's */
  err = gpa_gpgme_data_new_from_file (&input, filename, parent);
  if (err == GPGME_File_Error)
    {
      g_free (target_filename);
      fclose (target);
      return NULL;
    }
  else if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  err = gpgme_data_new (&output);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  /* Encrypt */
  if (sign)
    {
      err = gpgme_op_encrypt_sign (ctx, rset, input, output);
    }
  else
    {
      err = gpgme_op_encrypt (ctx, rset, input, output);
    }
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  /* Write the output */
  dump_data_to_file (output, target);
  fclose (target);
  gpgme_data_release (input);
  gpgme_data_release (output);

  return target_filename;
}

static void
do_encrypt (GPAFileEncryptDialog *dialog, GList *recipients, gboolean sign,
	    GList *signers, gboolean armor)
{
  GList *cur;
  GpgmeRecipients rset;
  gboolean success;
  
  success = set_recipients (recipients, &rset, dialog->window);
  if (!success)
    return;
  if (sign)
    {
      success = set_signers (signers, dialog->window);
      if (!success)
	return;
    }
  gpgme_set_armor (ctx, armor);
  
  /* Encrypt each file */
  for (cur = dialog->files; cur; cur = g_list_next (cur))
    {
      gchar *target_filename;

      target_filename = encrypt_file (cur->data, rset, sign, armor,
				      dialog->window);
      if (target_filename)
	{
	  dialog->encrypted_files = g_list_append (dialog->encrypted_files,
						   target_filename);
	}
      else
	{
	  break;
	}
    }
  g_list_free (recipients);
  gpgme_recipients_release (rset);
}

static void
file_encrypt_ok (GPAFileEncryptDialog *dialog)
{
  GList *recipients;
  gboolean sign;
  GList *signers;
  gboolean armor;
  
  recipients = gpa_key_list_selected_ids (dialog->clist_keys);
  armor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->check_armor));
  sign = FALSE;
  signers = NULL;
  
  do_encrypt (dialog, recipients, sign, signers, armor);
}

static void
select_row_cb (GtkCList *clist, gint row, gint column,
	       GdkEventButton *event, gpointer user_data)
{
  GPAFileEncryptDialog *dialog = user_data;
  
  if (g_list_length (GTK_CLIST (clist)->selection) > 0)
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->window),
					 GTK_RESPONSE_OK, TRUE);
    }
}

static void
unselect_row_cb (GtkCList *clist, gint row, gint column,
		 GdkEventButton *event, gpointer user_data)
{
  GPAFileEncryptDialog *dialog = user_data;
  
  if (g_list_length (GTK_CLIST (clist)->selection) == 0)
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog->window),
					 GTK_RESPONSE_OK, FALSE);
    }
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
  GtkResponseType response;

  GPAFileEncryptDialog dialog;

  if (!gpa_keytable_size (keytable))
    {
      gpa_window_error (_("No public keys available.\n"
			  "Currently, there is nobody who could read a\n"
			  "file encrypted by you."),
			parent);
      return NULL;
    } /* if */
  if (!gpa_keytable_secret_size (keytable))
    {
      gpa_window_error (_("No secret keys available."),
			parent);
      return NULL;
    } /* if */

  dialog.files = files;
  dialog.encrypted_files = NULL;

  window = gtk_dialog_new_with_buttons (_("Encrypt files"), GTK_WINDOW(parent),
					GTK_DIALOG_MODAL, 
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_OK);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (window), GTK_RESPONSE_OK, 
				     FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  dialog.window = window;

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accelGroup);

  vboxEncrypt = GTK_DIALOG (window)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vboxEncrypt), 5);

  labelKeys = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelKeys), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), labelKeys, FALSE, FALSE, 0);

  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), scrollerKeys, TRUE, TRUE, 0);
  gtk_widget_set_usize (scrollerKeys, 300, 120);

  clistKeys = gpa_public_key_list_new ();
  gtk_signal_connect (GTK_OBJECT (clistKeys), "select-row",
		      GTK_SIGNAL_FUNC (select_row_cb), &dialog);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "unselect-row",
		      GTK_SIGNAL_FUNC (unselect_row_cb), &dialog);
  dialog.clist_keys = clistKeys;
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys), GTK_SELECTION_MULTIPLE);
  gpa_connect_by_accelerator (GTK_LABEL (labelKeys), clistKeys, accelGroup,
			      _("_Public Keys"));

 
  checkerSign = gpa_check_button_new (accelGroup, _("_Sign"));
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), checkerSign, FALSE, FALSE, 0);
  dialog.check_sign = checkerSign;

  checkerArmor = gpa_check_button_new (accelGroup, _("A_rmor"));
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), checkerArmor, FALSE, FALSE, 0);
  dialog.check_armor = checkerArmor;

  gtk_widget_show_all (window);
  response = gtk_dialog_run (GTK_DIALOG (window));
  if (response == GTK_RESPONSE_OK)
    {
      file_encrypt_ok (&dialog);
    }
  gtk_widget_destroy (window);

  return dialog.encrypted_files;
} /* file_encrypt_dialog */
