/* gpafiledecryptop.c - The GpaOperation object.
 *	Copyright (C) 2003, Miguel Coca.
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

#include <glib.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#else
#include <io.h>
#endif

#include "gpa.h"
#include "gtktools.h"
#include "gpgmetools.h"
#include "gpafileencryptop.h"
#include "encryptdlg.h"
#include "gpawidgets.h"
#include "gpapastrings.h"

/* Internal functions */
static void gpa_file_encrypt_operation_done_cb (GpaContext *context, 
						GpgmeError err,
						GpaFileEncryptOperation *op);
static void gpa_file_encrypt_operation_response_cb (GtkDialog *dialog,
						    gint response,
						    gpointer user_data);

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_file_encrypt_operation_finalize (GObject *object)
{  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_file_encrypt_operation_class_init (GpaFileEncryptOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_file_encrypt_operation_finalize;
}

static void
gpa_file_encrypt_operation_init (GpaFileEncryptOperation *op)
{
  /* Create the "Encrypt" dialog */
  op->encrypt_dialog = gpa_file_encrypt_dialog_new 
    (GPA_OPERATION (op)->window, GPA_OPERATION (op)->options);
  g_signal_connect (G_OBJECT (op->encrypt_dialog), "response",
		    G_CALLBACK (gpa_file_encrypt_operation_response_cb), op);
  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_encrypt_operation_done_cb), op);
  /* Give a title to the progress dialog */
  gtk_window_set_title (GTK_WINDOW (GPA_FILE_OPERATION (op)->progress_dialog),
			_("Encrypting..."));
  /* Here we go... */
  gtk_widget_show_all (op->encrypt_dialog);
}

GType
gpa_file_encrypt_operation_get_type (void)
{
  static GType file_encrypt_operation_type = 0;
  
  if (!file_encrypt_operation_type)
    {
      static const GTypeInfo file_encrypt_operation_info =
      {
        sizeof (GpaFileEncryptOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_file_encrypt_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaFileEncryptOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_file_encrypt_operation_init,
      };
      
      file_encrypt_operation_type = g_type_register_static 
	(GPA_FILE_OPERATION_TYPE, "GpaFileEncryptOperation",
	 &file_encrypt_operation_info, 0);
    }
  
  return file_encrypt_operation_type;
}

/* API */

GpaFileEncryptOperation*
gpa_file_encrypt_operation_new (GpaOptions *options,
				GtkWidget *window,
				GList *files)
{
  GpaFileEncryptOperation *op;
  
  op = g_object_new (GPA_FILE_ENCRYPT_OPERATION_TYPE,
		     "options", options,
		     "window", window,
		     "input_files", files,
		     NULL);

  return op;
}

/* Internal */

static gchar*
destination_filename (const gchar *filename, gboolean armor)
{
  const gchar *extension;
  gchar *cipher_filename;

  if (!armor)
    {
      extension = ".gpg";
    }
  else
    {
      extension = ".asc";
    }
  cipher_filename = g_strconcat (filename, extension, NULL);

  return cipher_filename;
}

static gboolean
gpa_file_encrypt_operation_start (GpaFileEncryptOperation *op,
				  const gchar *plain_filename)
{
  GpgmeError err;
  
  op->cipher_filename = destination_filename 
    (plain_filename, gpgme_get_armor (GPA_OPERATION (op)->context->ctx));
  /* Open the files */
  op->plain_fd = gpa_open_input (plain_filename, &op->plain, 
				 GPA_OPERATION (op)->window);
  if (op->plain_fd == -1)
    {
      return FALSE;
    }
  op->cipher_fd = gpa_open_output (op->cipher_filename, &op->cipher,
				   GPA_OPERATION (op)->window);
  if (op->cipher_fd == -1)
    {
      gpgme_data_release (op->plain);
      close (op->plain_fd);
      return FALSE;
    }
  /* Start the operation */
  if (gpa_file_encrypt_dialog_sign 
      (GPA_FILE_ENCRYPT_DIALOG (op->encrypt_dialog)))
    {
      err = gpgme_op_encrypt_sign_start (GPA_OPERATION (op)->context->ctx,
					 op->rset, op->plain, op->cipher);
    }
  else
    {
      err = gpgme_op_encrypt_start (GPA_OPERATION (op)->context->ctx,
				    op->rset, op->plain, op->cipher);
    }
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_warning (err);
      return FALSE;
    }
  /* Show and update the progress dialog */
  gtk_widget_show_all (GPA_FILE_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG 
				 (GPA_FILE_OPERATION (op)->progress_dialog),
				 op->cipher_filename);
  return TRUE;
}

static void
gpa_file_encrypt_operation_next (GpaFileEncryptOperation *op)
{
  if (!GPA_FILE_OPERATION (op)->current ||
      !gpa_file_encrypt_operation_start (op, GPA_FILE_OPERATION (op)
					 ->current->data))
    {
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
}

static void
gpa_file_encrypt_operation_done_cb (GpaContext *context, 
				    GpgmeError err,
				    GpaFileEncryptOperation *op)
{
  /* Do clean up on the operation */
  gpgme_data_release (op->plain);
  close (op->plain_fd);
  gpgme_data_release (op->cipher);
  close (op->cipher_fd);
  gtk_widget_hide (GPA_FILE_OPERATION (op)->progress_dialog);
  if (err != GPGME_No_Error) 
    {
      /* If an error happened, (or the user canceled) delete the created file
       * and abort further encryptions
       */
      unlink (op->cipher_filename);
      g_free (op->cipher_filename);
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
  else
    {
      /* We've just created a file */
      g_signal_emit_by_name (GPA_OPERATION (op), "created_file",
			     op->cipher_filename);
      g_free (op->cipher_filename);
      /* Go to the next file in the list and encrypt it */
      gpa_file_encrypt_operation_next (op);
    }
}

/*
 * Setting the recipients for the context.
 */

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
  key_info = gpa_key_info_new (key);
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

static void
revoked_key (GpgmeKey key, GtkWidget *parent)
{
  GtkWidget *dialog;
  GtkWidget *key_info;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *image;

  dialog = gtk_dialog_new_with_buttons (_("Revoked Key"), GTK_WINDOW(parent),
					GTK_DIALOG_MODAL, 
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  hbox = gtk_hbox_new (FALSE, 6);
  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR,
				    GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox);

  label = gtk_label_new (_("The following key has been revoked by it's owner:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5);
  key_info = gpa_key_info_new (key);
  gtk_box_pack_start (GTK_BOX (vbox), key_info, FALSE, TRUE, 5);
  label = gtk_label_new (_("And can not be used for encryption."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5);

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
expired_key (GpgmeKey key, GtkWidget *parent)
{
  GtkWidget *dialog;
  GtkWidget *key_info;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *image;
  gchar *message;

  dialog = gtk_dialog_new_with_buttons (_("Revoked Key"), GTK_WINDOW(parent),
					GTK_DIALOG_MODAL, 
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  hbox = gtk_hbox_new (FALSE, 6);
  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR,
				    GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox);

  message = g_strdup_printf (_("The following key expired on %s:"),
                             gpa_expiry_date_string 
                             (gpgme_key_get_ulong_attr (key, GPGME_ATTR_EXPIRE,
                                                        NULL,0)));
  label = gtk_label_new (message);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5);
  key_info = gpa_key_info_new (key);
  gtk_box_pack_start (GTK_BOX (vbox), key_info, FALSE, TRUE, 5);
  label = gtk_label_new (_("And can not be used for encryption."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5);

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static gboolean
set_recipients (GpaFileEncryptOperation *op, GList *recipients)
{
  GList *cur;
  GpgmeError err;

  err = gpgme_recipients_new (&op->rset);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  for (cur = recipients; cur; cur = g_list_next (cur))
    {
      /* Check that all recipients are valid */
      gchar *fpr = cur->data;
      GpgmeKey key;
      GpgmeValidity valid;

      if (gpgme_get_key (GPA_OPERATION(op)->context->ctx, fpr, &key, 
			 FALSE, FALSE) == GPGME_EOF)
	{
	  /* Can't happen */
	  gpa_window_error (_("The key you selected is not available for "
			      "encryption"), GPA_OPERATION (op)->window);
	  return FALSE;
	}
      valid = gpgme_key_get_ulong_attr (key, GPGME_ATTR_VALIDITY, NULL, 0);
      /* First, make sure the key is usable (not revoked or unusable) */
      if (gpgme_key_get_ulong_attr (key, GPGME_ATTR_KEY_REVOKED, NULL, 0))
        {
          revoked_key (key, GPA_OPERATION (op)->window);
          return FALSE;
        }
      else if (gpgme_key_get_ulong_attr (key, GPGME_ATTR_KEY_EXPIRED, NULL, 0))
        {
          expired_key (key, GPA_OPERATION (op)->window);
          return FALSE;
        }
      /* Now, check it's validity */
      else if (valid == GPGME_VALIDITY_FULL || 
               valid == GPGME_VALIDITY_ULTIMATE)
	{
	  gpgme_recipients_add_name_with_validity (op->rset, cur->data, valid);
	}
      else
	{
	  /* If an untrusted key is found ask the user what to do */
	  GtkResponseType response;

	  response = ignore_key_trust (key, GPA_OPERATION (op)->window);
	  if (response == GTK_RESPONSE_YES)
	    {
	      /* Assume the key is trusted */
	      gpgme_recipients_add_name_with_validity (op->rset, cur->data,
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

/*
 * Setting the signers for the context.
 */
static gboolean
set_signers (GpaFileEncryptOperation *op, GList *signers)
{
  GList *cur;
  GpgmeError err;

  gpgme_signers_clear (GPA_OPERATION (op)->context->ctx);
  if (!signers)
    {
      /* Can't happen */
      gpa_window_error (_("You didn't select any key for signing"),
			GPA_OPERATION (op)->window);
      return FALSE;
    }
  for (cur = signers; cur; cur = g_list_next (cur))
    {
      GpgmeKey key;
      if (gpgme_get_key (GPA_OPERATION(op)->context->ctx, 
			 (char*) cur->data, &key, FALSE, FALSE) == GPGME_EOF)
	{
	  /* Can't happen */
	  gpa_window_error (_("The key you selected is not available for "
			      "signing"), GPA_OPERATION (op)->window);
	  break;
	}
      err = gpgme_signers_add (GPA_OPERATION (op)->context->ctx, key);
      if (err != GPGME_No_Error)
	{
	  gpa_gpgme_error (err);
	}
    }
  return TRUE;
}

/*
 * The key selection dialog has returned.
 */
static void gpa_file_encrypt_operation_response_cb (GtkDialog *dialog,
						    gint response,
						    gpointer user_data)
{
  GpaFileEncryptOperation *op = user_data;

  gtk_widget_hide (GTK_WIDGET (dialog));
  
  if (response == GTK_RESPONSE_OK)
    {
      gboolean success = TRUE;
      gboolean armor = gpa_file_encrypt_dialog_get_armor 
	(GPA_FILE_ENCRYPT_DIALOG (op->encrypt_dialog));
      GList *signers = gpa_file_encrypt_dialog_signers 
	(GPA_FILE_ENCRYPT_DIALOG (op->encrypt_dialog));
      GList *recipients = gpa_file_encrypt_dialog_recipients
	(GPA_FILE_ENCRYPT_DIALOG (op->encrypt_dialog));
      
      /* Set the armor value */
      gpgme_set_armor (GPA_OPERATION (op)->context->ctx, armor);
      /* Set the signers for the context */
      if (gpa_file_encrypt_dialog_sign 
	  (GPA_FILE_ENCRYPT_DIALOG (op->encrypt_dialog)))
	{
	  success = set_signers (op, signers);
	}
      /* Set the recipients for the context */
      if (success)
	{
	  success = set_recipients (op, recipients);
	}
      /* Actually run the operation or abort */
      if (success)
	{
	  gpa_file_encrypt_operation_next (op);
	}
      else
	{
	  g_signal_emit_by_name (GPA_OPERATION (op), "completed");
	}
    }
  else
    {
      /* The dialog was canceled, so we do nothing and complete the
       * operation */
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
}
