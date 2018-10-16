/* gpastreamsignop.c - The GpaStreamSignOperation object.
 * Copyright (C) 2008 g10 Code GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <glib.h>

#include "gpgmetools.h"
#include "gtktools.h"
#include "filesigndlg.h"
#include "gpastreamsignop.h"



struct _GpaStreamSignOperation
{
  GpaStreamOperation parent;

  GtkWidget *sign_dialog;

  const char *sender;
  gpgme_protocol_t requested_protocol;
  gboolean detached;
};


struct _GpaStreamSignOperationClass
{
  GpaStreamOperationClass parent_class;
};



/* Indentifiers for our properties. */
enum
  {
    PROP_0,
    PROP_SENDER,
    PROP_PROTOCOL,
    PROP_DETACHED
  };



static void response_cb (GtkDialog *dialog,
                         gint response,
                         gpointer user_data);
static void done_error_cb (GpaContext *context, gpg_error_t err,
                           GpaStreamSignOperation *op);
static void done_cb (GpaContext *context, gpg_error_t err,
                     GpaStreamSignOperation *op);

static GObjectClass *parent_class;




static void
gpa_stream_sign_operation_get_property (GObject *object, guint prop_id,
                                           GValue *value, GParamSpec *pspec)
{
  GpaStreamSignOperation *op = GPA_STREAM_SIGN_OPERATION (object);

  switch (prop_id)
    {
    case PROP_SENDER:
      g_value_set_string (value, op->sender);
      break;
    case PROP_PROTOCOL:
      g_value_set_int (value, op->requested_protocol);
      break;
    case PROP_DETACHED:
      g_value_set_boolean (value, op->detached);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_stream_sign_operation_set_property (GObject *object, guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
  GpaStreamSignOperation *op = GPA_STREAM_SIGN_OPERATION (object);

  switch (prop_id)
    {
    case PROP_SENDER:
      op->sender = g_value_get_string (value);
      break;
    case PROP_PROTOCOL:
      op->requested_protocol = g_value_get_int (value);
      break;
    case PROP_DETACHED:
      op->detached = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_stream_sign_operation_finalize (GObject *object)
{
/*   GpaStreamSignOperation *op = GPA_STREAM_SIGN_OPERATION (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_stream_sign_operation_init (GpaStreamSignOperation *op)
{
  op->requested_protocol = GPGME_PROTOCOL_UNKNOWN;
}


static GObject*
gpa_stream_sign_operation_ctor (GType type, guint n_construct_properties,
                                GObjectConstructParam  *construct_properties)
{
  GObject *object;
  GpaStreamSignOperation *op;

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_STREAM_SIGN_OPERATION (object);

  op->sign_dialog = gpa_file_sign_dialog_new (GPA_OPERATION (op)->window);
  {
    GpaFileSignDialog *dialog = GPA_FILE_SIGN_DIALOG (op->sign_dialog);

    /* Note: The information here is wrong.  The actual sig_mode and
       armor settings are determined from the selected key (which
       determines the protocol).  We set the values here to those for
       OpenPGP, and force (==hide) the selection widgets.  */
    gpa_file_sign_dialog_set_armor (dialog, TRUE);
    gpa_file_sign_dialog_set_force_armor (dialog, TRUE);
    gpa_file_sign_dialog_set_sig_mode (dialog, GPGME_SIG_MODE_NORMAL);
    gpa_file_sign_dialog_set_force_sig_mode (dialog, TRUE);
  }
  g_signal_connect (G_OBJECT (op->sign_dialog), "response",
                    G_CALLBACK (response_cb), op);

  /* We connect the done signal to two handles.  The error handler is
     called first.  */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (done_cb), op);

  gtk_window_set_title
    (GTK_WINDOW (GPA_STREAM_OPERATION (op)->progress_dialog),
			_("Signing message ..."));

  if (op->sign_dialog)
    gtk_widget_show_all (GTK_WIDGET (op->sign_dialog));

  return object;
}


static void
gpa_stream_sign_operation_class_init (GpaStreamSignOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_stream_sign_operation_ctor;
  object_class->finalize = gpa_stream_sign_operation_finalize;
  object_class->set_property = gpa_stream_sign_operation_set_property;
  object_class->get_property = gpa_stream_sign_operation_get_property;

  g_object_class_install_property
    (object_class, PROP_SENDER,
     g_param_spec_pointer
     ("sender", "Sender",
      "The sender of the message in rfc-822 mailbox format or NULL.",
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property
    (object_class, PROP_PROTOCOL,
     g_param_spec_int
     ("protocol", "Protocol",
      "The requested gpgme protocol.",
      GPGME_PROTOCOL_OpenPGP, GPGME_PROTOCOL_UNKNOWN, GPGME_PROTOCOL_UNKNOWN,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property
    (object_class, PROP_DETACHED,
     g_param_spec_boolean
     ("detached", "Detached",
      "Flag requesting a detached signature.",
      FALSE,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}


GType
gpa_stream_sign_operation_get_type (void)
{
  static GType stream_sign_operation_type = 0;

  if (!stream_sign_operation_type)
    {
      static const GTypeInfo stream_sign_operation_info =
      {
        sizeof (GpaStreamSignOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_stream_sign_operation_class_init,
        NULL, /* class_finalize */
        NULL, /* class_data */
        sizeof (GpaStreamSignOperation),
        0,    /* n_preallocs */
        (GInstanceInitFunc) gpa_stream_sign_operation_init,
      };

      stream_sign_operation_type = g_type_register_static
	(GPA_STREAM_OPERATION_TYPE, "GpaStreamSignOperation",
	 &stream_sign_operation_info, 0);
    }

  return stream_sign_operation_type;
}


/* Setting the signers and the protocol for the context. The protocol
 * to use is derived from the keys.  An error will be displayed if the
 * selected keys are not all of one protocol. Returns true on
 * success.  */
static gboolean
set_signers (GpaStreamSignOperation *op, GList *signers)
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
 * Fire up the signing
 */
static void
start_signing (GpaStreamSignOperation *op)
{
  gpg_error_t err;
  int prep_only = 0;
  GList *signers;
  gpgme_protocol_t protocol;

  signers = gpa_file_sign_dialog_signers
    (GPA_FILE_SIGN_DIALOG (op->sign_dialog));
  if (!set_signers (op, signers))
    {
      err = gpg_error (GPG_ERR_NO_SECKEY);
      goto leave;
    }

  protocol = gpgme_get_protocol (GPA_OPERATION (op)->context->ctx);
  if (protocol == GPGME_PROTOCOL_OpenPGP)
    err = gpa_operation_write_status (GPA_OPERATION (op), "PROTOCOL",
                                      "OpenPGP", NULL);
  else if (protocol == GPGME_PROTOCOL_CMS)
    err = gpa_operation_write_status (GPA_OPERATION (op), "PROTOCOL",
                                      "CMS", NULL);
  else
    err = gpg_error (GPG_ERR_NO_SECKEY);
  if (err)
    goto leave;

  /* Set the output encoding.  */
  if (GPA_STREAM_OPERATION (op)->input_stream
      && GPA_STREAM_OPERATION (op)->output_stream)
    {
      if (gpgme_data_get_encoding (GPA_STREAM_OPERATION(op)->output_stream))
        gpgme_data_set_encoding
          (GPA_STREAM_OPERATION (op)->output_stream,
           gpgme_data_get_encoding (GPA_STREAM_OPERATION(op)->output_stream));
      else if (protocol == GPGME_PROTOCOL_CMS)
        gpgme_data_set_encoding (GPA_STREAM_OPERATION (op)->output_stream,
                                 GPGME_DATA_ENCODING_BASE64);
      else
        gpgme_set_armor (GPA_OPERATION (op)->context->ctx, 1);

      err = gpgme_op_sign_start (GPA_OPERATION (op)->context->ctx,
                                 GPA_STREAM_OPERATION (op)->input_stream,
                                 GPA_STREAM_OPERATION (op)->output_stream,
                                 (op->detached? GPGME_SIG_MODE_DETACH
                                  /* */       : GPGME_SIG_MODE_NORMAL));
      if (err)
        {
          gpa_gpgme_warning (err);
          goto leave;
        }

      /* Show and update the progress dialog.  */
      gtk_widget_show_all (GPA_STREAM_OPERATION (op)->progress_dialog);
      gpa_progress_dialog_set_label
        (GPA_PROGRESS_DIALOG (GPA_STREAM_OPERATION (op)->progress_dialog),
         _("Message signing"));
    }
  else
    {
      /* We are just preparing an encryption. */
      prep_only = 1;
      err = 0;
    }

 leave:
  if (err || prep_only)
    g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}


/* The recipient key selection dialog has returned.  */
static void
response_cb (GtkDialog *dialog, int response, void *user_data)
{
  GpaStreamSignOperation *op = user_data;

  gtk_widget_hide (GTK_WIDGET (dialog));

  if (response != GTK_RESPONSE_OK)
    {
      /* The dialog was canceled, so we do nothing and complete the
	 operation.  */
      g_signal_emit_by_name (GPA_OPERATION (op), "completed",
			     gpg_error (GPG_ERR_CANCELED));
      return;
    }

  start_signing (op);
}


/* This is the idle function used to start the encryption if no
   recipient key selection dialog has been requested.  */
#if 0 /* Not yet used.  */
static gboolean
start_signing_cb (void *user_data)
{
  GpaStreamSignOperation *op = user_data;

  start_signing (op);

  return FALSE;  /* Remove this callback from the event loop.  */
}
#endif

/* Show an error message. */
static void
done_error_cb (GpaContext *context, gpg_error_t err,
               GpaStreamSignOperation *op)
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

/* Operation is ready.  Tell the server.  */
static void
done_cb (GpaContext *context, gpg_error_t err, GpaStreamSignOperation *op)
{
  gtk_widget_hide (GPA_STREAM_OPERATION (op)->progress_dialog);

  if (! err)
    {
      gpgme_protocol_t protocol;
      gpgme_sign_result_t res;
      gpgme_new_signature_t sig;

      protocol = gpgme_get_protocol (GPA_OPERATION (op)->context->ctx);

      res = gpgme_op_sign_result (GPA_OPERATION (op)->context->ctx);
      if (res)
	{
	  sig = res->signatures;
	  while (sig)
	    {
	      char *str;
	      char *algo_name;

	      str = g_strdup_printf
		("%s%s", (protocol == GPGME_PROTOCOL_OpenPGP) ? "pgp-" : "",
		 gpgme_hash_algo_name (sig->hash_algo));
	      algo_name = g_ascii_strdown (str, -1);
	      g_free (str);

	      /* FIXME: Error handling.  */
	      err = gpa_operation_write_status (GPA_OPERATION (op), "MICALG",
						algo_name, NULL);

	      g_free (algo_name);
	      sig = sig->next;
	    }
	}
    }

  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}




/* Public API.  */

/* Start signing INPUT_STREAM to OUTPUT_STREAM using WINDOW.  SENDER
   gives the name of the sender's role (usually a mailbox) or is NULL
   for the default sender.

   If it is not possible to unambigiously select a signing key a key
   selection dialog offers the user a way to manually select signing
   keys.  INPUT_STREAM and OUTPUT_STREAM may be given as NULL in which
   case the function skips the actual signing step and just verifies
   the signing key.  */
GpaStreamSignOperation*
gpa_stream_sign_operation_new (GtkWidget *window,
                               gpgme_data_t input_stream,
                               gpgme_data_t output_stream,
                               const gchar *sender,
                               gpgme_protocol_t protocol,
                               gboolean detached)
{
  GpaStreamSignOperation *op;

  op = g_object_new (GPA_STREAM_SIGN_OPERATION_TYPE,
		     "window", window,
		     "input_stream", input_stream,
		     "output_stream", output_stream,
                     "sender", sender,
                     "protocol", (int)protocol,
                     "detached", detached,
		     NULL);

  return op;
}


