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
#include "gpafilesignop.h"
#include "filesigndlg.h"
#include "gpawidgets.h"
#include "gpapastrings.h"

/* Internal functions */
static void gpa_file_sign_operation_done_error_cb (GpaContext *context, 
						   GpgmeError err,
						   GpaFileSignOperation *op);
static void gpa_file_sign_operation_done_cb (GpaContext *context, 
						GpgmeError err,
						GpaFileSignOperation *op);
static void gpa_file_sign_operation_response_cb (GtkDialog *dialog,
						    gint response,
						    gpointer user_data);

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_file_sign_operation_finalize (GObject *object)
{  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_file_sign_operation_init (GpaFileSignOperation *op)
{
  op->sign_dialog = NULL;
  op->sign_type = GPGME_SIG_MODE_NORMAL;
  op->sig_fd = -1;
  op->plain_fd = -1;
  op->sig = NULL;
  op->plain = NULL;
  op->sig_filename = NULL;
}

static GObject*
gpa_file_sign_operation_constructor (GType type,
				     guint n_construct_properties,
				     GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileSignOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_FILE_SIGN_OPERATION (object);
  /* Initialize */
  /* Create the "Sign" dialog */
  op->sign_dialog = gpa_file_sign_dialog_new (GPA_OPERATION (op)->window);
  g_signal_connect (G_OBJECT (op->sign_dialog), "response",
		    G_CALLBACK (gpa_file_sign_operation_response_cb), op);
  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_sign_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_sign_operation_done_cb), op);
  /* Give a title to the progress dialog */
  gtk_window_set_title (GTK_WINDOW (GPA_FILE_OPERATION (op)->progress_dialog),
			_("Signing..."));
  /* Here we go... */
  gtk_widget_show_all (op->sign_dialog);

  return object;
}

static void
gpa_file_sign_operation_class_init (GpaFileSignOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_file_sign_operation_constructor;
  object_class->finalize = gpa_file_sign_operation_finalize;
}

GType
gpa_file_sign_operation_get_type (void)
{
  static GType file_sign_operation_type = 0;
  
  if (!file_sign_operation_type)
    {
      static const GTypeInfo file_sign_operation_info =
      {
        sizeof (GpaFileSignOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_file_sign_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaFileSignOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_file_sign_operation_init,
      };
      
      file_sign_operation_type = g_type_register_static 
	(GPA_FILE_OPERATION_TYPE, "GpaFileSignOperation",
	 &file_sign_operation_info, 0);
    }
  
  return file_sign_operation_type;
}

/* API */

GpaFileSignOperation*
gpa_file_sign_operation_new (GtkWidget *window, GList *files)
{
  GpaFileSignOperation *op;
  
  op = g_object_new (GPA_FILE_SIGN_OPERATION_TYPE,
		     "window", window,
		     "input_files", files,
		     NULL);

  return op;
}

/* Internal */

static gchar*
destination_filename (const gchar *filename, gboolean armor, 
		      GpgmeSigMode sign_type)
{
  const gchar *extension;
  gchar *signature_filename;

  if (sign_type == GPGME_SIG_MODE_DETACH)
    {
      extension = ".sig";
    }
  else if (sign_type == GPGME_SIG_MODE_CLEAR)
    {
      extension = ".asc";
    }
  else
    {
      extension = ".gpg";
    }
  signature_filename = g_strconcat (filename, extension, NULL);

  return signature_filename;
}

static gboolean
gpa_file_sign_operation_start (GpaFileSignOperation *op,
			       const gchar *plain_filename)
{
  GpgmeError err;
  
  op->sig_filename = destination_filename 
    (plain_filename, gpgme_get_armor (GPA_OPERATION (op)->context->ctx),
     op->sign_type);
  /* Open the files */
  op->plain_fd = gpa_open_input (plain_filename, &op->plain, 
				 GPA_OPERATION (op)->window);
  if (op->plain_fd == -1)
    {
      return FALSE;
    }
  op->sig_fd = gpa_open_output (op->sig_filename, &op->sig,
				GPA_OPERATION (op)->window);
  if (op->sig_fd == -1)
    {
      gpgme_data_release (op->plain);
      close (op->plain_fd);
      return FALSE;
    }
  /* Start the operation */
  err = gpgme_op_sign_start (GPA_OPERATION (op)->context->ctx, op->plain,
			     op->sig, op->sign_type);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_warning (err);
      return FALSE;
    }
  /* Show and update the progress dialog */
  gtk_widget_show_all (GPA_FILE_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG 
				 (GPA_FILE_OPERATION (op)->progress_dialog),
				 op->sig_filename);
  return TRUE;
}

static void
gpa_file_sign_operation_next (GpaFileSignOperation *op)
{
  if (!GPA_FILE_OPERATION (op)->current ||
      !gpa_file_sign_operation_start (op, GPA_FILE_OPERATION (op)
					 ->current->data))
    {
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
}

static void
gpa_file_sign_operation_done_cb (GpaContext *context, 
				    GpgmeError err,
				    GpaFileSignOperation *op)
{
  /* Do clean up on the operation */
  gpgme_data_release (op->plain);
  close (op->plain_fd);
  gpgme_data_release (op->sig);
  close (op->sig_fd);
  gtk_widget_hide (GPA_FILE_OPERATION (op)->progress_dialog);
  if (err != GPGME_No_Error) 
    {
      /* If an error happened, (or the user canceled) delete the created file
       * and abort further signions
       */
      unlink (op->sig_filename);
      g_free (op->sig_filename);
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
  else
    {
      /* We've just created a file */
      g_signal_emit_by_name (GPA_OPERATION (op), "created_file",
			     op->sig_filename);
      g_free (op->sig_filename);
      /* Go to the next file in the list and sign it */
      GPA_FILE_OPERATION (op)->current = g_list_next 
	(GPA_FILE_OPERATION (op)->current);
      gpa_file_sign_operation_next (op);
    }
}

/*
 * Setting the signers for the context.
 */
static gboolean
set_signers (GpaFileSignOperation *op, GList *signers)
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
      GpgmeKey key = cur->data;
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
static void gpa_file_sign_operation_response_cb (GtkDialog *dialog,
						    gint response,
						    gpointer user_data)
{
  GpaFileSignOperation *op = user_data;

  gtk_widget_hide (GTK_WIDGET (dialog));
  
  if (response == GTK_RESPONSE_OK)
    {
      gboolean success = TRUE;
      gboolean armor = gpa_file_sign_dialog_get_armor 
	(GPA_FILE_SIGN_DIALOG (op->sign_dialog));
      GList *signers = gpa_file_sign_dialog_signers 
	(GPA_FILE_SIGN_DIALOG (op->sign_dialog));
      op->sign_type = gpa_file_sign_dialog_get_sign_type
	(GPA_FILE_SIGN_DIALOG (op->sign_dialog));
      
      /* Set the armor value */
      gpgme_set_armor (GPA_OPERATION (op)->context->ctx, armor);
      /* Set the signers for the context */
      success = set_signers (op, signers);
      /* Actually run the operation or abort */
      if (success)
	{
	  gpa_file_sign_operation_next (op);
	}
      else
	{
	  g_signal_emit_by_name (GPA_OPERATION (op), "completed");
	}

      g_list_free (signers);
    }
  else
    {
      /* The dialog was canceled, so we do nothing and complete the
       * operation */
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
}

static void
gpa_file_sign_operation_done_error_cb (GpaContext *context, GpgmeError err,
				       GpaFileSignOperation *op)
{
  /* Capture fatal errors and quit the application */
  switch (err)
    {
    case GPGME_No_Error:
    case GPGME_Canceled:
      /* Ignore these */
      break;
    case GPGME_No_Passphrase:
      gpa_window_error (_("Wrong passphrase!"), GPA_OPERATION (op)->window);
      break;
    case GPGME_Invalid_UserID:
    case GPGME_No_Recipients:
    case GPGME_Invalid_Key:
    case GPGME_File_Error:
    case GPGME_EOF:
    case GPGME_Decryption_Failed:
    case GPGME_No_Data:

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
