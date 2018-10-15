/* gpafiledecryptop.c - The GpaOperation object.
 * Copyright (C) 2003 Miguel Coca.
 * Copyright (C) 2008, 2015 g10 Code GmbH.
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

#include <glib.h>
#include <glib/gstdio.h>

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
#include "filetype.h"
#include "gpafiledecryptop.h"
#include "verifydlg.h"

/* Internal functions */
static gboolean gpa_file_decrypt_operation_idle_cb (gpointer data);
static void gpa_file_decrypt_operation_done_cb (GpaContext *context,
						gpg_error_t err,
						GpaFileDecryptOperation *op);
static void gpa_file_decrypt_operation_done_error_cb (GpaContext *context,
						      gpg_error_t err,
						      GpaFileDecryptOperation *op);

/* GObject */

static GObjectClass *parent_class = NULL;

/* Properties */
enum
{
  PROP_0,
  PROP_VERIFY
};


static void
gpa_file_decrypt_operation_get_property (GObject *object, guint prop_id,
					 GValue *value, GParamSpec *pspec)
{
  GpaFileDecryptOperation *op = GPA_FILE_DECRYPT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_VERIFY:
      g_value_set_boolean (value, op->verify);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_file_decrypt_operation_set_property (GObject *object, guint prop_id,
					 const GValue *value, GParamSpec *pspec)
{
  GpaFileDecryptOperation *op = GPA_FILE_DECRYPT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_VERIFY:
      op->verify = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_file_decrypt_operation_finalize (GObject *object)
{
  GpaFileDecryptOperation *op = GPA_FILE_DECRYPT_OPERATION (object);

  if (op->dialog)
    gtk_widget_destroy (op->dialog);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_file_decrypt_operation_init (GpaFileDecryptOperation *op)
{
  op->cipher_fd = -1;
  op->plain_fd = -1;
  op->cipher = NULL;
  op->plain = NULL;
}


static void
gpa_file_decrypt_operation_response_cb (GtkDialog *dialog,
					gint response, gpointer user_data)
{
  GpaFileDecryptOperation *op = GPA_FILE_DECRYPT_OPERATION (user_data);

  g_signal_emit_by_name (GPA_OPERATION (op), "completed", op->err);
}


static GObject*
gpa_file_decrypt_operation_constructor
(GType type,
 guint n_construct_properties,
 GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileDecryptOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_FILE_DECRYPT_OPERATION (object);
  /* Initialize */
  /* Start with the first file after going back into the main loop */
  g_idle_add (gpa_file_decrypt_operation_idle_cb, op);
  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_decrypt_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_decrypt_operation_done_cb), op);
  /* Give a title to the progress dialog */
  gtk_window_set_title (GTK_WINDOW (GPA_FILE_OPERATION (op)->progress_dialog),
			_("Decrypting..."));

  if (op->verify)
    {
      /* Create the verification dialog */
      op->dialog = gpa_file_verify_dialog_new (GPA_OPERATION (op)->window);
      g_signal_connect (G_OBJECT (op->dialog), "response",
			G_CALLBACK (gpa_file_decrypt_operation_response_cb),
			op);
    }

  return object;
}

static void
gpa_file_decrypt_operation_class_init (GpaFileDecryptOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_file_decrypt_operation_constructor;
  object_class->finalize = gpa_file_decrypt_operation_finalize;
  object_class->set_property = gpa_file_decrypt_operation_set_property;
  object_class->get_property = gpa_file_decrypt_operation_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_VERIFY,
				   g_param_spec_boolean
				   ("verify", "Verify",
				    "Verify", FALSE,
				    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_file_decrypt_operation_get_type (void)
{
  static GType file_decrypt_operation_type = 0;

  if (!file_decrypt_operation_type)
    {
      static const GTypeInfo file_decrypt_operation_info =
      {
        sizeof (GpaFileDecryptOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_file_decrypt_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaFileDecryptOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_file_decrypt_operation_init,
      };

      file_decrypt_operation_type = g_type_register_static
	(GPA_FILE_OPERATION_TYPE, "GpaFileDecryptOperation",
	 &file_decrypt_operation_info, 0);
    }

  return file_decrypt_operation_type;
}

/* API */

GpaFileDecryptOperation*
gpa_file_decrypt_operation_new (GtkWidget *window,
				GList *files)
{
  GpaFileDecryptOperation *op;

  op = g_object_new (GPA_FILE_DECRYPT_OPERATION_TYPE,
		     "window", window,
		     "input_files", files,
		     NULL);

  return op;
}


GpaFileDecryptOperation*
gpa_file_decrypt_verify_operation_new (GtkWidget *window,
				       GList *files)
{
  GpaFileDecryptOperation *op;

  op = g_object_new (GPA_FILE_DECRYPT_OPERATION_TYPE,
		     "window", window,
		     "input_files", files,
		     "verify", TRUE,
		     NULL);

  return op;
}


/* Internal */

static gchar *
destination_filename (const gchar *filename)
{
  gchar *extension, *plain_filename;

  /* Find out the destination file */
  extension = g_strrstr (filename, ".");
  if (extension && (g_str_equal (extension, ".asc") ||
		    g_str_equal (extension, ".gpg") ||
		    g_str_equal (extension, ".pgp")))
    {
      /* Remove the extension */
      plain_filename = g_strdup (filename);
      *(plain_filename + (extension-filename)) = '\0';
    }
  else
    {
      plain_filename = g_strconcat (filename, ".txt", NULL);
    }

  return plain_filename;
}

static gpg_error_t
gpa_file_decrypt_operation_start (GpaFileDecryptOperation *op,
				  gpa_file_item_t file_item)
{
  gpg_error_t err;

  if (file_item->direct_in)
    {
      /* No copy is made.  */
      err = gpgme_data_new_from_mem (&op->cipher, file_item->direct_in,
				     file_item->direct_in_len, 0);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  return err;
	}

      err = gpgme_data_new (&op->plain);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  gpgme_data_release (op->cipher);
	  op->plain = NULL;
	  return err;
	}

      gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                          is_cms_data (file_item->direct_in,
                                       file_item->direct_in_len) ?
                          GPGME_PROTOCOL_CMS : GPGME_PROTOCOL_OpenPGP);
    }
  else
    {
      gchar *cipher_filename = file_item->filename_in;
      char *filename_used;

      file_item->filename_out = destination_filename (cipher_filename);
      /* Open the files */
      op->cipher_fd = gpa_open_input (cipher_filename, &op->cipher,
				  GPA_OPERATION (op)->window);
      if (op->cipher_fd == -1)
	/* FIXME: Error value.  */
	return gpg_error (GPG_ERR_GENERAL);

      op->plain_fd = gpa_open_output (file_item->filename_out, &op->plain,
				      GPA_OPERATION (op)->window,
                                      &filename_used);
      if (op->plain_fd == -1)
	{
	  gpgme_data_release (op->cipher);
	  close (op->cipher_fd);
          xfree (filename_used);
	  /* FIXME: Error value.  */
	  return gpg_error (GPG_ERR_GENERAL);
	}

      xfree (file_item->filename_out);
      file_item->filename_out = filename_used;

      gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                          is_cms_file (cipher_filename) ?
                          GPGME_PROTOCOL_CMS : GPGME_PROTOCOL_OpenPGP);
    }

  /* Start the operation.  */
  err = gpgme_op_decrypt_verify_start (GPA_OPERATION (op)->context->ctx,
				       op->cipher, op->plain);
  if (err)
    {
      gpa_gpgme_warning (err);

      gpgme_data_release (op->plain);
      op->plain = NULL;
      close (op->plain_fd);
      op->plain_fd = -1;
      gpgme_data_release (op->cipher);
      op->cipher = NULL;
      close (op->cipher_fd);
      op->cipher_fd = -1;

      return err;
    }

  /* Show and update the progress dialog.  */
  gtk_widget_show_all (GPA_FILE_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG
				 (GPA_FILE_OPERATION (op)->progress_dialog),
				 file_item->direct_name
				 ? file_item->direct_name
				 : file_item->filename_in);
  return 0;
}


static void
gpa_file_decrypt_operation_next (GpaFileDecryptOperation *op)
{
  gpg_error_t err;

  if (! GPA_FILE_OPERATION (op)->current)
    {
      if (op->verify && op->signed_files)
	{
	  op->err = 0;
	  gtk_widget_show_all (op->dialog);
	}
      else
	g_signal_emit_by_name (GPA_OPERATION (op), "completed", 0);
      return;
    }

  err = gpa_file_decrypt_operation_start
    (op, GPA_FILE_OPERATION (op)->current->data);
  if (err)
    {
      if (op->verify && op->signed_files)
	{
	  /* All files have been verified: show the results dialog */
	  op->err = err;
	  gtk_widget_show_all (op->dialog);
	}
      else
	g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
    }
}


static void
gpa_file_decrypt_operation_done_cb (GpaContext *context,
				    gpg_error_t err,
				    GpaFileDecryptOperation *op)
{
  gpa_file_item_t file_item = GPA_FILE_OPERATION (op)->current->data;

  if (file_item->direct_in)
    {
      size_t len;
      char *plain_gpgme = gpgme_data_release_and_get_mem (op->plain, &len);
      op->plain = NULL;
      /* Do the memory allocation dance.  */

      if (plain_gpgme)
	{
	  /* Conveniently make ASCII stuff into a string.  */
	  file_item->direct_out = g_malloc (len + 1);
	  memcpy (file_item->direct_out, plain_gpgme, len);
	  gpgme_free (plain_gpgme);
	  file_item->direct_out[len] = '\0';
	  /* Yep, excluding the trailing zero.  */
	  file_item->direct_out_len = len;
	}
      else
	{
	  file_item->direct_out = NULL;
	  file_item->direct_out_len = 0;
	}
    }

  /* Do clean up on the operation */
  gpgme_data_release (op->plain);
  op->plain = NULL;
  close (op->plain_fd);
  op->plain_fd = -1;
  gpgme_data_release (op->cipher);
  op->cipher = NULL;
  close (op->cipher_fd);
  op->cipher_fd = -1;
  gtk_widget_hide (GPA_FILE_OPERATION (op)->progress_dialog);
  if (err)
    {
      if (! file_item->direct_in)
	{
	  /* If an error happened, (or the user canceled) delete the
	     created file and abort further decryptions.  */
	  g_unlink (file_item->filename_out);
	  g_free (file_item->filename_out);
	  file_item->filename_out = NULL;
	}
      /* FIXME:CLIPBOARD: Server finish?  */
      g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
    }
  else
    {
      /* We've just created a file */
      g_signal_emit_by_name (GPA_OPERATION (op), "created_file", file_item);

      if (op->verify)
	{
	  gpgme_verify_result_t result;

	  result = gpgme_op_verify_result (GPA_OPERATION (op)->context->ctx);
	  if (result->signatures)
	    {
	      /* Add the file to the result dialog.  FIXME: Maybe we
		 should use the filename without the directory.  */
	      gpa_file_verify_dialog_add_file (GPA_FILE_VERIFY_DIALOG (op->dialog),
					       file_item->direct_name
					       ? file_item->direct_name
					       : file_item->filename_in,
					       NULL, NULL,
					       result->signatures);
	      op->signed_files++;
	    }
	}

      /* Go to the next file in the list and decrypt it */
      GPA_FILE_OPERATION (op)->current = g_list_next
	(GPA_FILE_OPERATION (op)->current);
      gpa_file_decrypt_operation_next (op);
    }
}


static gboolean
gpa_file_decrypt_operation_idle_cb (gpointer data)
{
  GpaFileDecryptOperation *op = data;

  gpa_file_decrypt_operation_next (op);

  return FALSE;
}


static void
gpa_file_decrypt_operation_done_error_cb (GpaContext *context, gpg_error_t err,
					  GpaFileDecryptOperation *op)
{
  gpa_file_item_t file_item = GPA_FILE_OPERATION (op)->current->data;

  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;
    case GPG_ERR_NO_DATA:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     file_item->direct_name
                     ? _("\"%s\" contained no OpenPGP data.")
                     : _("The file \"%s\" contained no OpenPGP"
                         "data."),
                     file_item->direct_name
                     ? file_item->direct_name
                     : file_item->filename_in);
      break;
    case GPG_ERR_DECRYPT_FAILED:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     file_item->direct_name
                     ? _("\"%s\" contained no valid "
                         "encrypted data.")
                     : _("The file \"%s\" contained no valid "
                         "encrypted data."),
                     file_item->direct_name
                     ? file_item->direct_name
                     : file_item->filename_in);
      break;
    case GPG_ERR_BAD_PASSPHRASE:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     _("Wrong passphrase!"));
      break;
    default:
      gpa_gpgme_warn (err, NULL, GPA_OPERATION (op)->context);
      break;
    }
}
