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
#include "gpafiledecryptop.h"

/* Internal functions */
static gboolean gpa_file_decrypt_operation_idle_cb (gpointer data);
static void gpa_file_decrypt_operation_done_cb (GpaContext *context, 
						GpgmeError err,
						GpaFileDecryptOperation *op);
static void gpa_file_decrypt_operation_done_error_cb (GpaContext *context,
						      GpgmeError err,
						      GpaFileDecryptOperation *op);

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_file_decrypt_operation_finalize (GObject *object)
{  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_file_decrypt_operation_init (GpaFileDecryptOperation *op)
{
  op->cipher_fd = -1;
  op->plain_fd = -1;
  op->cipher = NULL;
  op->plain = NULL;
  op->plain_filename = NULL;
}

static GObject*
gpa_file_decrypt_operation_constructor (GType type,
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

  return object;
}

static void
gpa_file_decrypt_operation_class_init (GpaFileDecryptOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_file_decrypt_operation_constructor;
  object_class->finalize = gpa_file_decrypt_operation_finalize;
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

static gboolean
gpa_file_decrypt_operation_start (GpaFileDecryptOperation *op,
				  const gchar *cipher_filename)
{
  GpgmeError err;
  
  op->plain_filename = destination_filename (cipher_filename);
  /* Open the files */
  op->cipher_fd = gpa_open_input (cipher_filename, &op->cipher, 
				  GPA_OPERATION (op)->window);
  if (op->cipher_fd == -1)
    {
      return FALSE;
    }
  op->plain_fd = gpa_open_output (op->plain_filename, &op->plain,
				  GPA_OPERATION (op)->window);
  if (op->plain_fd == -1)
    {
      gpgme_data_release (op->cipher);
      close (op->cipher_fd);
      return FALSE;
    }
  /* Start the operation */
  err = gpgme_op_decrypt_start (GPA_OPERATION (op)->context->ctx, op->cipher, 
				op->plain);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_warning (err);
      return FALSE;
    }
  /* Show and update the progress dialog */
  gtk_widget_show_all (GPA_FILE_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG 
				 (GPA_FILE_OPERATION (op)->progress_dialog),
				 op->plain_filename);
  return TRUE;
}

static void
gpa_file_decrypt_operation_next (GpaFileDecryptOperation *op)
{
  if (!GPA_FILE_OPERATION (op)->current ||
      !gpa_file_decrypt_operation_start (op, GPA_FILE_OPERATION (op)
					 ->current->data))
    {
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
}

static void
gpa_file_decrypt_operation_done_cb (GpaContext *context, 
				    GpgmeError err,
				    GpaFileDecryptOperation *op)
{
  /* Do clean up on the operation */
  gpgme_data_release (op->plain);
  close (op->plain_fd);
  gpgme_data_release (op->cipher);
  close (op->cipher_fd);
  gtk_widget_hide (GPA_FILE_OPERATION (op)->progress_dialog);
  /* Check for error */
  if (err != GPGME_No_Error) 
    {
      /* If an error happened, (or the user canceled) delete the created file
       * and abort further decryptions
       */
      unlink (op->plain_filename);
      g_free (op->plain_filename);
      g_signal_emit_by_name (GPA_OPERATION (op), "completed"); 
    }
  else
    {
      /* We've just created a file */
      g_signal_emit_by_name (GPA_OPERATION (op), "created_file",
			     op->plain_filename);
      g_free (op->plain_filename);
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
gpa_file_decrypt_operation_done_error_cb (GpaContext *context, GpgmeError err,
					  GpaFileDecryptOperation *op)
{
  gchar *message;

  /* Capture fatal errors and quit the application */
  switch (err)
    {
    case GPGME_No_Error:
    case GPGME_Canceled:
      /* Ignore these */
      break;
    case GPGME_No_Data:
      message = g_strdup_printf (_("The file \"%s\" contained no OpenPGP "
				   "data."),
				 gpa_file_operation_current_file 
				 (GPA_FILE_OPERATION(op)));
      gpa_window_error (message, GPA_OPERATION (op)->window);
      g_free (message);
      break;
    case GPGME_Decryption_Failed:
      message = g_strdup_printf (_("The file \"%s\" contained no "
				   "valid encrypted data."),
				 gpa_file_operation_current_file
				 (GPA_FILE_OPERATION(op)));
      gpa_window_error (message, GPA_OPERATION (op)->window);
      g_free (message);
      break;
    case GPGME_No_Passphrase:
      gpa_window_error (_("Wrong passphrase!"), GPA_OPERATION (op)->window);
      break;
    case GPGME_Invalid_UserID:
    case GPGME_Invalid_Key:
    case GPGME_No_Recipients:
    case GPGME_File_Error:
    case GPGME_EOF:

      /* These are always unexpected errors */
    case GPGME_General_Error:
    case GPGME_Out_Of_Core:
    case GPGME_Invalid_Value:
    case GPGME_Busy:
    case GPGME_No_Request:
    case GPGME_Exec_Error:
    case GPGME_Too_Many_Procs:
    case GPGME_Pipe_Error:
    case GPGME_Conflict:
    case GPGME_Not_Implemented:
    case GPGME_Read_Error:
    case GPGME_Write_Error:
    case GPGME_Invalid_Engine:
    default:
      gpa_gpgme_warning (err);
      break;
    }
}
