/* gpafileverifyop.c - The GpaOperation object.
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
#include "gpafileverifyop.h"
#include "verifydlg.h"

/* Internal functions */
static gboolean gpa_file_verify_operation_idle_cb (gpointer data);
static void gpa_file_verify_operation_done_error_cb (GpaContext *context,
						     GpgmeError err,
						     GpaFileVerifyOperation *op);
static void gpa_file_verify_operation_done_cb (GpaContext *context, 
						GpgmeError err,
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
}

static GObject*
gpa_file_verify_operation_constructor (GType type,
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


/* Check whether the file is a detached signature and deduce the name of the
 * original file. Since we only have access to the filename, this is not
 * very solid.
 */
static gboolean
is_detached_sig (const gchar *filename, gchar **signed_file)
{
  gchar *extension;
  *signed_file = g_strdup (filename);
  extension = g_strrstr (*signed_file, ".");
  if (extension &&
      (g_str_equal (extension, ".sig") ||
       g_str_equal (extension, ".sign")))
    {
      *extension++ = '\0';
      return TRUE;
    }
  else
    {
      g_free (*signed_file);
      *signed_file = NULL;
      return FALSE;
    }
}

static gboolean
gpa_file_verify_operation_start (GpaFileVerifyOperation *op,
				 const gchar *sig_filename)
{
  GpgmeError err;
  gchar *signed_file;

  if (is_detached_sig (sig_filename, &signed_file))
    {
      /* Allocate data objects for a detached signature */
      op->sig_fd = gpa_open_input (sig_filename, &op->sig, 
				   GPA_OPERATION (op)->window);
      if (op->sig_fd == -1)
	{
	  return FALSE;
	}
      op->signed_text_fd = gpa_open_input (signed_file, &op->signed_text,
					   GPA_OPERATION (op)->window);
      if (op->signed_text_fd == -1)
	{
	  gpgme_data_release (op->sig);
	  close (op->sig_fd);
	  return FALSE;
	}
      op->plain = NULL;
      g_free (signed_file);
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
      if (gpgme_data_new (&op->plain) != GPGME_No_Error)
	{
	  gpgme_data_release (op->sig);
	  close (op->sig_fd);
	  return FALSE;
	}
      op->signed_text_fd = -1;
      op->signed_text = NULL;
    }

  /* Start the operation */
  err = gpgme_op_verify_start (GPA_OPERATION (op)->context->ctx, op->sig,
			       op->signed_text, op->plain);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_warning (err);
      return FALSE;
    }
  /* Show and update the progress dialog */
  gtk_widget_show_all (GPA_FILE_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG 
				 (GPA_FILE_OPERATION (op)->progress_dialog),
				 sig_filename);
  return TRUE;
}

static void
gpa_file_verify_operation_next (GpaFileVerifyOperation *op)
{
  if (!GPA_FILE_OPERATION (op)->current ||
      !gpa_file_verify_operation_start (op, GPA_FILE_OPERATION (op)
					 ->current->data))
    {
      /* All files have been verified: show the results dialog */
      gtk_widget_show_all (op->dialog);
    }
}

static void
gpa_file_verify_operation_done_cb (GpaContext *context, 
				    GpgmeError err,
				    GpaFileVerifyOperation *op)
{
  /* Do clean up on the operation */
  if (op->plain) 
    {
      gpgme_data_release (op->plain);
    }
  if (op->signed_text)
    {
      gpgme_data_release (op->signed_text);
      close (op->signed_text_fd);
    }
  gpgme_data_release (op->sig);
  close (op->sig_fd);
  gtk_widget_hide (GPA_FILE_OPERATION (op)->progress_dialog);
  /* Check for error */
  if (err != GPGME_No_Error) 
    {
      /* Abort further verifications */
    }
  else
    {
      /* Add the file to the result dialog */
      gpa_file_verify_dialog_add_file (GPA_FILE_VERIFY_DIALOG (op->dialog),
				       GPA_FILE_OPERATION (op)->current->data,
				       GPA_OPERATION (op)->context->ctx);
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
  g_signal_emit_by_name (GPA_OPERATION (op), "completed");  
}

static void
gpa_file_verify_operation_done_error_cb (GpaContext *context, GpgmeError err,
					 GpaFileVerifyOperation *op)
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
    case GPGME_No_Passphrase:
      gpa_window_error (_("Wrong passphrase!"), GPA_OPERATION (op)->window);
      break;
    case GPGME_Invalid_Recipients:
    case GPGME_No_Recipients:
    case GPGME_Invalid_Key:
    case GPGME_File_Error:
    case GPGME_EOF:
    case GPGME_Decryption_Failed:

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
    case GPGME_Invalid_Type:
    case GPGME_Invalid_Mode:
    case GPGME_Invalid_Engine:
    default:
      gpa_gpgme_warning (err);
      break;
    }
}
