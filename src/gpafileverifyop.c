/* gpafileverifyop.c - The GpaOperation object.
   Copyright (C) 2003 Miguel Coca.
   Copyright (C) 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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
#include "filetype.h"
#include "gpafileverifyop.h"
#include "verifydlg.h"


/* Internal functions */
static gboolean gpa_file_verify_operation_idle_cb (gpointer data);
static void gpa_file_verify_operation_done_error_cb (GpaContext *context,
						     gpg_error_t err,
						     GpaFileVerifyOperation *op);
static void gpa_file_verify_operation_done_cb (GpaContext *context,
						gpg_error_t err,
						GpaFileVerifyOperation *op);
static void gpa_file_verify_operation_response_cb (GtkDialog *dialog,
						   gint response,
						   gpointer user_data);

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_file_verify_operation_finalize (GObject *object)
{
  GpaFileVerifyOperation *op = GPA_FILE_VERIFY_OPERATION (object);

  gtk_widget_destroy (op->dialog);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_file_verify_operation_init (GpaFileVerifyOperation *op)
{
  op->sig_fd = -1;
  op->signed_text_fd = -1;
  op->sig = NULL;
  op->signed_text = NULL;
  op->signed_file = NULL;
  op->signature_file = NULL;
}

static GObject*
gpa_file_verify_operation_constructor
(GType type,
 guint n_construct_properties,
 GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileVerifyOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_FILE_VERIFY_OPERATION (object);
  /* Initialize */
  /* Start with the first file after going back into the main loop */
  g_idle_add (gpa_file_verify_operation_idle_cb, op);
  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_verify_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_verify_operation_done_cb), op);
  /* Give a title to the progress dialog */
  gtk_window_set_title (GTK_WINDOW (GPA_FILE_OPERATION (op)->progress_dialog),
			_("Verifying..."));

  /* Create the verification dialog */
  op->dialog = gpa_file_verify_dialog_new (GPA_OPERATION (op)->window);
  g_signal_connect (G_OBJECT (op->dialog), "response",
		    G_CALLBACK (gpa_file_verify_operation_response_cb), op);

  return object;
}


static void
gpa_file_verify_operation_class_init (GpaFileVerifyOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_file_verify_operation_constructor;
  object_class->finalize = gpa_file_verify_operation_finalize;
}

GType
gpa_file_verify_operation_get_type (void)
{
  static GType file_verify_operation_type = 0;

  if (!file_verify_operation_type)
    {
      static const GTypeInfo file_verify_operation_info =
      {
        sizeof (GpaFileVerifyOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_file_verify_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaFileVerifyOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_file_verify_operation_init,
      };

      file_verify_operation_type = g_type_register_static
	(GPA_FILE_OPERATION_TYPE, "GpaFileVerifyOperation",
	 &file_verify_operation_info, 0);
    }

  return file_verify_operation_type;
}

/* API */

GpaFileVerifyOperation*
gpa_file_verify_operation_new (GtkWidget *window, GList *files)
{
  GpaFileVerifyOperation *op;

  op = g_object_new (GPA_FILE_VERIFY_OPERATION_TYPE,
		     "window", window,
		     "input_files", files,
		     NULL);

  return op;
}

/* Internal */

static gboolean
ask_use_detached_sig (const gchar *file, const gchar *sig, GtkWidget *parent)
{
  GtkWidget *dialog = gtk_message_dialog_new
    (GTK_WINDOW(parent), GTK_DIALOG_MODAL,
     GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
     _("GPA found a file that could be a signature of %s. "
       "Would you like to verify it instead?\n\n"
       "The file found is: %s"), file, sig);
  gboolean result;

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  _("_Yes"), GTK_RESPONSE_YES,
			  _("_No"), GTK_RESPONSE_NO, NULL);
  result = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
  gtk_widget_destroy (dialog);

  return result;
}

/* Check whether the file is a detached signature and deduce the name of the
 * original file. Since we only have access to the filename, this is not
 * very solid.
 */
static gboolean
is_detached_sig (const gchar *filename, gchar **signature_file,
		 gchar **signed_file, GtkWidget *window)
{
  const gchar *sig_extension[] = {".sig", ".asc", ".sign"};
  int i;
  gchar *extension;

  /* First, check whether this file is a detached signature.  */
  *signed_file = g_strdup (filename);
  extension = g_strrstr (*signed_file, ".");
  if (extension)
    {
      for (i = 0; i < DIM (sig_extension); i++)
        if (g_str_equal (extension, sig_extension[i]))
          {
            *extension = '\0';  /* That makes SIGNED_FILE */
            if (g_file_test (*signed_file, G_FILE_TEST_EXISTS))
              {
                *signature_file = g_strdup (filename);
                return TRUE;
              }
            g_free (*signed_file);
            *signed_file = NULL;
            return FALSE; /* signed file does not exist.  */
          }
    }
  g_free (*signed_file);
  *signed_file = NULL;

  /* Now, check whether a detached signature exists for this file */
  for (i = 0; i < DIM (sig_extension); i++)
    {
      gchar *sig = g_strconcat (filename, sig_extension[i], NULL);

      if (g_file_test (sig, G_FILE_TEST_EXISTS)
          && ask_use_detached_sig (filename, sig, window))
        {
          *signed_file = g_strdup (filename);
          *signature_file = sig;
          return TRUE;
        }
      g_free (sig);
    }

  return FALSE;
}

static gboolean
gpa_file_verify_operation_start (GpaFileVerifyOperation *op,
				 gpa_file_item_t file_item)
{
  gpgme_error_t err;

  if (file_item->direct_in)
    {
      /* Direct input is always an inline signature.  */

      /* No copy is made.  */
      err = gpgme_data_new_from_mem (&op->sig, file_item->direct_in,
				     file_item->direct_in_len, 0);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  return FALSE;
	}

      err = gpgme_data_new (&op->plain);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  gpgme_data_release (op->sig);
	  op->sig = NULL;
	  return FALSE;
	}

      gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                          is_cms_data (file_item->direct_in,
                                       file_item->direct_in_len) ?
                          GPGME_PROTOCOL_CMS : GPGME_PROTOCOL_OpenPGP);
    }
  else
    {
      const gchar *sig_filename = file_item->filename_in;

      if (is_detached_sig (sig_filename, &op->signature_file, &op->signed_file,
			   GPA_OPERATION (op)->window))
	{
	  /* Allocate data objects for a detached signature */
	  op->sig_fd = gpa_open_input (op->signature_file, &op->sig,
				       GPA_OPERATION (op)->window);
	  if (op->sig_fd == -1)
	    {
	      return FALSE;
	    }
	  op->signed_text_fd = gpa_open_input (op->signed_file, &op->signed_text,
					       GPA_OPERATION (op)->window);
	  if (op->signed_text_fd == -1)
	    {
	      gpgme_data_release (op->sig);
	      close (op->sig_fd);
	      return FALSE;
	    }
	  op->plain = NULL;
	}
      else
	{
	  /* Allocate data object for non-detached signatures */
	  op->sig_fd = gpa_open_input (sig_filename, &op->sig,
				       GPA_OPERATION (op)->window);
	  if (op->sig_fd == -1)
	    {
	      return FALSE;
	    }
	  err = gpgme_data_new (&op->plain);
	  if (err)
	    {
	      gpgme_data_release (op->sig);
	      close (op->sig_fd);
	      return FALSE;
	    }
	  op->signed_text_fd = -1;
	  op->signed_text = NULL;
	}

      gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                          is_cms_file (sig_filename) ?
                          GPGME_PROTOCOL_CMS : GPGME_PROTOCOL_OpenPGP);
    }


  /* Start the operation */
  err = gpgme_op_verify_start (GPA_OPERATION (op)->context->ctx, op->sig,
			       op->signed_text, op->plain);
  if (err)
    {
      gpa_gpgme_warning (err);
      return FALSE;
    }
  /* Show and update the progress dialog */
  gtk_widget_show_all (GPA_FILE_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG
				 (GPA_FILE_OPERATION (op)->progress_dialog),
				 file_item->direct_name
				 ? file_item->direct_name
				 : file_item->filename_in);

  return TRUE;
}


static void
gpa_file_verify_operation_next (GpaFileVerifyOperation *op)
{
  if (! GPA_FILE_OPERATION (op)->current ||
      ! gpa_file_verify_operation_start (op, GPA_FILE_OPERATION (op)
					 ->current->data))
    {
      /* All files have been verified: show the results dialog */
      gtk_widget_show_all (op->dialog);
    }
}


static void
gpa_file_verify_operation_done_cb (GpaContext *context,
				    gpg_error_t err,
				    GpaFileVerifyOperation *op)
{
  gpa_file_item_t file_item = GPA_FILE_OPERATION (op)->current->data;

  if (file_item->direct_in)
    {
      size_t len;
      char *plain_gpgme = gpgme_data_release_and_get_mem (op->plain,
							   &len);
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
  gpgme_data_release (op->signed_text);
  op->signed_text = NULL;
  close (op->signed_text_fd);
  op->signed_text_fd = -1;
  gpgme_data_release (op->sig);
  op->sig = NULL;
  close (op->sig_fd);
  op->sig_fd = -1;

  gtk_widget_hide (GPA_FILE_OPERATION (op)->progress_dialog);
  /* Check for error */
  if (err)
    {
      /* Abort further verifications */
    }
  else
    {
      gpgme_verify_result_t result;

      result = gpgme_op_verify_result (GPA_OPERATION (op)->context->ctx);
      /* Add the file to the result dialog.  FIXME: Maybe we should
	 use the filename without the directory.  */
      gpa_file_verify_dialog_add_file (GPA_FILE_VERIFY_DIALOG (op->dialog),
				       file_item->direct_name
				       ? file_item->direct_name
				       : file_item->filename_in,
				       op->signed_file, op->signature_file,
				       result->signatures);
      /* If this was a dettached sig, reset the signed file */
      if (op->signed_file)
	{
	  g_free (op->signed_file);
	  op->signed_file = NULL;
	  g_free (op->signature_file);
	  op->signature_file = NULL;
	}
      else
	{
	  /* Otherwise, we created a "file" in direct mode.  */
	  if (file_item->direct_in)
	    g_signal_emit_by_name (GPA_OPERATION (op), "created_file",
				   file_item);
	}

      /* Go to the next file in the list and verify it */
      GPA_FILE_OPERATION (op)->current = g_list_next
	(GPA_FILE_OPERATION (op)->current);
      gpa_file_verify_operation_next (op);
    }
}

static gboolean
gpa_file_verify_operation_idle_cb (gpointer data)
{
  GpaFileVerifyOperation *op = data;

  gpa_file_verify_operation_next (op);

  return FALSE;
}

static void
gpa_file_verify_operation_response_cb (GtkDialog *dialog,
				       gint response,
				       gpointer user_data)
{
  GpaFileVerifyOperation *op = GPA_FILE_VERIFY_OPERATION (user_data);

  /* FIXME: Error handling.  */
  g_signal_emit_by_name (GPA_OPERATION (op), "completed", 0);
}


static void
gpa_file_verify_operation_done_error_cb (GpaContext *context, gpg_error_t err,
					 GpaFileVerifyOperation *op)
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
    case GPG_ERR_BAD_PASSPHRASE:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     _("Wrong passphrase!"));
      break;
    default:
      gpa_gpgme_warn (err, NULL, GPA_OPERATION (op)->context);
      break;
    }
}
