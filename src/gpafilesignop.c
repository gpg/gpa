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
#include "gpafilesignop.h"
#include "filesigndlg.h"
#include "gpawidgets.h"

/* Internal functions */
static void gpa_file_sign_operation_done_error_cb (GpaContext *context,
						   gpg_error_t err,
						   GpaFileSignOperation *op);
static void gpa_file_sign_operation_done_cb (GpaContext *context,
						gpg_error_t err,
						GpaFileSignOperation *op);
static void gpa_file_sign_operation_response_cb (GtkDialog *dialog,
						    gint response,
						    gpointer user_data);

/* GObject */

static GObjectClass *parent_class = NULL;

/* Properties */
enum
{
  PROP_0,
  PROP_FORCE_ARMOR
};


static void
gpa_file_sign_operation_get_property (GObject *object, guint prop_id,
				      GValue *value, GParamSpec *pspec)
{
  GpaFileSignOperation *op = GPA_FILE_SIGN_OPERATION (object);

  switch (prop_id)
    {
    case PROP_FORCE_ARMOR:
      g_value_set_boolean (value, op->force_armor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_file_sign_operation_set_property (GObject *object, guint prop_id,
				      const GValue *value, GParamSpec *pspec)
{
  GpaFileSignOperation *op = GPA_FILE_SIGN_OPERATION (object);

  switch (prop_id)
    {
    case PROP_FORCE_ARMOR:
      op->force_armor = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


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
  op->force_armor = FALSE;
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
  if (op->force_armor)
    {
      /* FIXME: Currently, force_armor means also force cleartext sig mode.  */
      GpaFileSignDialog *dialog = GPA_FILE_SIGN_DIALOG (op->sign_dialog);
      gpa_file_sign_dialog_set_armor (dialog, TRUE);
      gpa_file_sign_dialog_set_force_armor (dialog, TRUE);
      gpa_file_sign_dialog_set_sig_mode (dialog, GPGME_SIG_MODE_CLEAR);
      gpa_file_sign_dialog_set_force_sig_mode (dialog, TRUE);
    }

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
  object_class->set_property = gpa_file_sign_operation_set_property;
  object_class->get_property = gpa_file_sign_operation_get_property;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_FORCE_ARMOR,
				   g_param_spec_boolean
				   ("force-armor", "Force armor",
				    "Force armor mode", FALSE,
				    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
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
gpa_file_sign_operation_new (GtkWidget *window, GList *files,
			     gboolean force_armor)
{
  GpaFileSignOperation *op;

  op = g_object_new (GPA_FILE_SIGN_OPERATION_TYPE,
		     "window", window,
		     "input_files", files,
		     "force-armor", force_armor,
		     NULL);

  return op;
}

/* Internal */

static gchar*
destination_filename (const gchar *filename, gboolean armor,
                      gpgme_protocol_t protocol,
		      gpgme_sig_mode_t sign_type)
{
  const gchar *extension;
  gchar *signature_filename;

  if (protocol == GPGME_PROTOCOL_CMS && armor
      && sign_type != GPGME_SIG_MODE_DETACH)
    extension = ".pem";
  else if (protocol == GPGME_PROTOCOL_CMS)
    extension = ".p7s";
  else if (sign_type == GPGME_SIG_MODE_DETACH)
    extension = (armor && protocol == GPGME_PROTOCOL_OPENPGP)? ".asc" : ".sig";
  else if (sign_type == GPGME_SIG_MODE_CLEAR)
    extension = ".asc";
  else
    extension = ".gpg";

  signature_filename = g_strconcat (filename, extension, NULL);

  return signature_filename;
}


static gpg_error_t
gpa_file_sign_operation_start (GpaFileSignOperation *op,
			       gpa_file_item_t file_item)
{
  gpg_error_t err;

  if (file_item->direct_in)
    {
      /* No copy is made.  */
      err = gpgme_data_new_from_mem (&op->plain, file_item->direct_in,
				     file_item->direct_in_len, 0);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  return err;
	}

      err = gpgme_data_new (&op->sig);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  gpgme_data_release (op->plain);
	  op->plain = NULL;
	  return err;
	}
    }
  else
    {
      gchar *plain_filename = file_item->filename_in;
      char *filename_used;

      file_item->filename_out = destination_filename
	(plain_filename, gpgme_get_armor (GPA_OPERATION (op)->context->ctx),
	 gpgme_get_protocol (GPA_OPERATION (op)->context->ctx), op->sign_type);

      /* Open the files */
      op->plain_fd = gpa_open_input (plain_filename, &op->plain,
				     GPA_OPERATION (op)->window);
      if (op->plain_fd == -1)
	/* FIXME: Error value.  */
	return gpg_error (GPG_ERR_GENERAL);

      op->sig_fd = gpa_open_output (file_item->filename_out, &op->sig,
				    GPA_OPERATION (op)->window,
                                    &filename_used);
      if (op->sig_fd == -1)
	{
	  gpgme_data_release (op->plain);
	  close (op->plain_fd);
          xfree (filename_used);
	  /* FIXME: Error value.  */
	  return gpg_error (GPG_ERR_GENERAL);
	}

      xfree (file_item->filename_out);
      file_item->filename_out = filename_used;
    }

  /* Start the operation */
  err = gpgme_op_sign_start (GPA_OPERATION (op)->context->ctx, op->plain,
			     op->sig, op->sign_type);
  if (err)
    {
      gpa_gpgme_warning (err);
      return err;
    }
  /* Show and update the progress dialog */
  gtk_widget_show_all (GPA_FILE_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG
				 (GPA_FILE_OPERATION (op)->progress_dialog),
				 file_item->direct_name
				 ? file_item->direct_name
				 : file_item->filename_in);

  return 0;
}


static void
gpa_file_sign_operation_next (GpaFileSignOperation *op)
{
  gpg_error_t err;

  if (! GPA_FILE_OPERATION (op)->current)
    {
      g_signal_emit_by_name (GPA_OPERATION (op), "completed", 0);
      return;
    }

  err = gpa_file_sign_operation_start (op,
				       GPA_FILE_OPERATION (op)->current->data);
  if (err)
    g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}


static void
gpa_file_sign_operation_done_cb (GpaContext *context,
				 gpg_error_t err,
				 GpaFileSignOperation *op)
{
  gpa_file_item_t file_item = GPA_FILE_OPERATION (op)->current->data;

  if (file_item->direct_in)
    {
      size_t len;
      char *sig_gpgme = gpgme_data_release_and_get_mem (op->sig, &len);
      op->sig = NULL;
      /* Do the memory allocation dance.  */

      if (sig_gpgme)
	{
	  /* Conveniently make ASCII stuff into a string.  */
	  file_item->direct_out = g_malloc (len + 1);
	  memcpy (file_item->direct_out, sig_gpgme, len);
	  gpgme_free (sig_gpgme);
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
  gpgme_data_release (op->sig);
  op->sig = NULL;
  close (op->sig_fd);
  op->sig_fd = -1;
  gtk_widget_hide (GPA_FILE_OPERATION (op)->progress_dialog);

  if (err)
    {
      if (! file_item->direct_in)
	{
	  /* If an error happened, (or the user canceled) delete the
	     created file and abort further signions.  */
	  g_unlink (op->sig_filename);
	  g_free (op->sig_filename);
	}
      g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
    }
  else
    {
      /* We've just created a file */
      g_signal_emit_by_name (GPA_OPERATION (op), "created_file",
			     file_item);
      /* Go to the next file in the list and sign it */
      GPA_FILE_OPERATION (op)->current = g_list_next
	(GPA_FILE_OPERATION (op)->current);
      gpa_file_sign_operation_next (op);
    }
}


/*
 * Setting the signers and the protocol for the context. The protocol
 * to use is derived from the keys.  An errro will be displayed if the
 * selected keys are not all of one protocol.
 */
static gboolean
set_signers (GpaFileSignOperation *op, GList *signers)
{
  GList *cur;
  gpg_error_t err;
  gpgme_protocol_t protocol = GPGME_PROTOCOL_UNKNOWN;

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
      gpgme_key_t key = cur->data;
      if (protocol == GPGME_PROTOCOL_UNKNOWN)
        protocol = key->protocol;
      else if (key->protocol != protocol)
        {
          /* Should not happen because the selection dialog should
             have not allowed to select different key types.  */
          gpa_window_error
            (_("The selected certificates are not all of the same type."
               " That is, you mixed OpenPGP and X.509 certificates."
               " Please make sure to select only certificates of the"
               " same type."),
             GPA_OPERATION (op)->window);
          return FALSE;
        }
    }

  gpgme_set_protocol (GPA_OPERATION (op)->context->ctx, protocol);

  for (cur = signers; cur; cur = g_list_next (cur))
    {
      gpgme_key_t key = cur->data;
      err = gpgme_signers_add (GPA_OPERATION (op)->context->ctx, key);
      if (err)
	gpa_gpgme_error (err);
    }

  return TRUE;
}


/*
 * The key selection dialog has returned.
 */
static void
gpa_file_sign_operation_response_cb (GtkDialog *dialog,
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
      op->sign_type = gpa_file_sign_dialog_get_sig_mode
	(GPA_FILE_SIGN_DIALOG (op->sign_dialog));

      /* Set the armor value */
      gpgme_set_armor (GPA_OPERATION (op)->context->ctx, armor);
      /* Set the signers for the context */
      success = set_signers (op, signers);
      /* Actually run the operation or abort.  */
      if (success)
	gpa_file_sign_operation_next (op);
      else
	g_signal_emit_by_name (GPA_OPERATION (op), "completed",
			       gpg_error (GPG_ERR_GENERAL));

      g_list_free (signers);
    }
  else
    /* The dialog was canceled, so we do nothing and complete the
       * operation */
    g_signal_emit_by_name (GPA_OPERATION (op), "completed",
			   gpg_error (GPG_ERR_CANCELED));
}


static void
gpa_file_sign_operation_done_error_cb (GpaContext *context, gpg_error_t err,
				       GpaFileSignOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CANCELED:
      /* Ignore these */
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
