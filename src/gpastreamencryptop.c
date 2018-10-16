/* gpastreamdecryptop.c - The GpaOperation object.
 * Copyright (C) 2007, 2008 g10 Code GmbH
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
#include "recipientdlg.h"
#include "gpawidgets.h"
#include "gpastreamencryptop.h"
#include "selectkeydlg.h"


struct _GpaStreamEncryptOperation
{
  GpaStreamOperation parent;

  SelectKeyDlg *key_dialog;
  RecipientDlg *recp_dialog;
  GSList *recipients;
  gpgme_key_t *keys;
  gpgme_protocol_t selected_protocol;
};


struct _GpaStreamEncryptOperationClass
{
  GpaStreamOperationClass parent_class;
};



/* Indentifiers for our properties. */
enum
  {
    PROP_0,
    PROP_RECIPIENTS,
    PROP_RECIPIENT_KEYS,
    PROP_PROTOCOL
  };



static void response_cb (GtkDialog *dialog,
                         gint response,
                         gpointer user_data);
static gboolean start_encryption_cb (gpointer data);
static void done_error_cb (GpaContext *context, gpg_error_t err,
                           GpaStreamEncryptOperation *op);
static void done_cb (GpaContext *context, gpg_error_t err,
                     GpaStreamEncryptOperation *op);

static GObjectClass *parent_class;



/* Helper to be used as a GFunc for free. */
static void
free_func (void *p, void *dummy)
{
  (void)dummy;
  g_free (p);
}


static void
release_recipients (GSList *recipients)
{
  if (recipients)
    {
      g_slist_foreach (recipients, free_func, NULL);
      g_slist_free (recipients);
    }
}

/* Return a deep copy of the recipients list.  */
static GSList *
copy_recipients (GSList *recipients)
{
  GSList *recp, *newlist;

  newlist= NULL;
  for (recp = recipients; recp; recp = g_slist_next (recp))
    newlist = g_slist_append (newlist, g_strdup (recp->data));

  return newlist;
}



static void
gpa_stream_encrypt_operation_get_property (GObject *object, guint prop_id,
                                           GValue *value, GParamSpec *pspec)
{
  GpaStreamEncryptOperation *op = GPA_STREAM_ENCRYPT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_RECIPIENTS:
      g_value_set_pointer (value, op->recipients);
      break;
    case PROP_RECIPIENT_KEYS:
      g_value_set_pointer (value, op->keys);
      break;
    case PROP_PROTOCOL:
      g_value_set_int (value, op->selected_protocol);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_stream_encrypt_operation_set_property (GObject *object, guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
  GpaStreamEncryptOperation *op = GPA_STREAM_ENCRYPT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_RECIPIENTS:
      op->recipients = (GSList*)g_value_get_pointer (value);
      break;
    case PROP_RECIPIENT_KEYS:
      op->keys = (gpgme_key_t*)g_value_get_pointer (value);
      break;
    case PROP_PROTOCOL:
      op->selected_protocol = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_stream_encrypt_operation_finalize (GObject *object)
{
  GpaStreamEncryptOperation *op = GPA_STREAM_ENCRYPT_OPERATION (object);

  release_recipients (op->recipients);
  op->recipients = NULL;
  gpa_gpgme_release_keyarray (op->keys);
  op->keys = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_stream_encrypt_operation_init (GpaStreamEncryptOperation *op)
{
  op->key_dialog = NULL;
  op->recp_dialog = NULL;
  op->recipients = NULL;
  op->keys = NULL;
  op->selected_protocol = GPGME_PROTOCOL_UNKNOWN;
}


static GObject*
gpa_stream_encrypt_operation_constructor
	(GType type,
         guint n_construct_properties,
         GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaStreamEncryptOperation *op;

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_STREAM_ENCRYPT_OPERATION (object);

  /* Create the recipient key selection dialog if we don't know the
     keys yet. */
  if (!op->keys && (!op->recipients || !g_slist_length (op->recipients)))
    {
      /* No recipients - use a generic key selection dialog.  */
      op->key_dialog = select_key_dlg_new (GPA_OPERATION (op)->window);
      g_signal_connect (G_OBJECT (op->key_dialog), "response",
                        G_CALLBACK (response_cb), op);
    }
  else if (!op->keys)
    {
      /* Caller gave us some recipients - use the mail address
         matching key selectiion dialog.  */
      op->recp_dialog = recipient_dlg_new (GPA_OPERATION (op)->window);
      recipient_dlg_set_recipients (op->recp_dialog,
                                    op->recipients,
                                    op->selected_protocol);
      g_signal_connect (G_OBJECT (op->recp_dialog), "response",
                        G_CALLBACK (response_cb), op);
    }
  else
    g_idle_add (start_encryption_cb, op);


  /* We connect the done signal to two handles.  The error handler is
     called first.  */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (done_cb), op);

  gtk_window_set_title
    (GTK_WINDOW (GPA_STREAM_OPERATION (op)->progress_dialog),
			_("Encrypting message ..."));

  if (op->key_dialog)
    gtk_widget_show_all (GTK_WIDGET (op->key_dialog));
  if (op->recp_dialog)
    gtk_widget_show_all (GTK_WIDGET (op->recp_dialog));

  return object;
}


static void
gpa_stream_encrypt_operation_class_init (GpaStreamEncryptOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_stream_encrypt_operation_constructor;
  object_class->finalize = gpa_stream_encrypt_operation_finalize;
  object_class->set_property = gpa_stream_encrypt_operation_set_property;
  object_class->get_property = gpa_stream_encrypt_operation_get_property;

  g_object_class_install_property
    (object_class, PROP_RECIPIENTS,
     g_param_spec_pointer
     ("recipients", "Recipients",
      "A list of recipients in rfc-822 mailbox format.",
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property
    (object_class, PROP_RECIPIENT_KEYS,
     g_param_spec_pointer
     ("recipient-keys", "Recipient-keys",
      "An array of gpgme_key_t with the selected keys.",
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property
    (object_class, PROP_PROTOCOL,
     g_param_spec_int
     ("protocol", "Protocol",
      "The gpgme protocol currently selected.",
      GPGME_PROTOCOL_OpenPGP, GPGME_PROTOCOL_UNKNOWN, GPGME_PROTOCOL_UNKNOWN,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

}


GType
gpa_stream_encrypt_operation_get_type (void)
{
  static GType stream_encrypt_operation_type = 0;

  if (!stream_encrypt_operation_type)
    {
      static const GTypeInfo stream_encrypt_operation_info =
      {
        sizeof (GpaStreamEncryptOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_stream_encrypt_operation_class_init,
        NULL, /* class_finalize */
        NULL, /* class_data */
        sizeof (GpaStreamEncryptOperation),
        0,    /* n_preallocs */
        (GInstanceInitFunc) gpa_stream_encrypt_operation_init,
      };

      stream_encrypt_operation_type = g_type_register_static
	(GPA_STREAM_OPERATION_TYPE, "GpaStreamEncryptOperation",
	 &stream_encrypt_operation_info, 0);
    }

  return stream_encrypt_operation_type;
}


/* Return true if all keys are matching the protocol. */
static int
keys_match_protocol_p (gpgme_key_t *keys, gpgme_protocol_t protocol)
{
  int idx;

  if (!keys)
    return 1; /* No keys: assume match.  */
  for (idx = 0; keys[idx]; idx++)
    if (keys[idx]->protocol != protocol)
      return 0;
  return 1;
}


/*
 * Fire up the encryption.
 */
static void
start_encryption (GpaStreamEncryptOperation *op)
{
  gpg_error_t err;
  int prep_only = 0;

  if (!op->keys || !op->keys[0])
    {
      err = gpg_error (GPG_ERR_NO_PUBKEY);
      goto leave;
    }

  if (op->selected_protocol == GPGME_PROTOCOL_OpenPGP)
    err = gpa_operation_write_status (GPA_OPERATION (op), "PROTOCOL",
                                      "OpenPGP", NULL);
  else if (op->selected_protocol == GPGME_PROTOCOL_CMS)
    err = gpa_operation_write_status (GPA_OPERATION (op), "PROTOCOL",
                                      "CMS", NULL);
  else
    err = gpg_error (GPG_ERR_NO_PUBKEY);
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
      else if (op->selected_protocol == GPGME_PROTOCOL_CMS)
        gpgme_data_set_encoding (GPA_STREAM_OPERATION (op)->output_stream,
                                 GPGME_DATA_ENCODING_BASE64);
      else
        gpgme_set_armor (GPA_OPERATION (op)->context->ctx, 1);

      if (!keys_match_protocol_p (op->keys, op->selected_protocol))
        {
          g_debug ("the selected keys do not match the protocol");
          err = gpg_error (GPG_ERR_CONFLICT);
          goto leave;
        }

      gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                          op->selected_protocol);

      /* We always trust the keys because the recipient selection
         dialog has already sorted unusable out.  */
      err = gpgme_op_encrypt_start (GPA_OPERATION (op)->context->ctx,
                                    op->keys, GPGME_ENCRYPT_ALWAYS_TRUST,
                                    GPA_STREAM_OPERATION (op)->input_stream,
                                    GPA_STREAM_OPERATION (op)->output_stream);
      if (err)
        {
          gpa_gpgme_warning (err);
          goto leave;
        }

      /* Show and update the progress dialog.  */
      gtk_widget_show_all (GPA_STREAM_OPERATION (op)->progress_dialog);
      gpa_progress_dialog_set_label
        (GPA_PROGRESS_DIALOG (GPA_STREAM_OPERATION (op)->progress_dialog),
         _("Message encryption"));
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
  GpaStreamEncryptOperation *op = user_data;

  gtk_widget_hide (GTK_WIDGET (dialog));

  if (response != GTK_RESPONSE_OK)
    {
      /* The dialog was canceled, so we do nothing and complete the
	 operation.  */
      g_signal_emit_by_name (GPA_OPERATION (op), "completed",
                                   gpg_error (GPG_ERR_CANCELED));
      /* FIXME: We might need to destroy the widget in the KEY_DIALOG case.  */
      return;
    }

  /* Get the keys.  */
  gpa_gpgme_release_keyarray (op->keys);
  op->keys = NULL;
  if (op->key_dialog)
    op->keys = select_key_dlg_get_keys (op->key_dialog);
  else if (op->recp_dialog)
    op->keys = recipient_dlg_get_keys (op->recp_dialog, &op->selected_protocol);

  start_encryption (op);
}


/* This is the idle function used to start the encryption if no
   recipient key selection dialog has been requested.  */
static gboolean
start_encryption_cb (void *user_data)
{
  GpaStreamEncryptOperation *op = user_data;

  start_encryption (op);

  return FALSE;  /* Remove this callback from the event loop.  */
}


/*Show an error message. */
static void
done_error_cb (GpaContext *context, gpg_error_t err,
               GpaStreamEncryptOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;
/*     case GPG_ERR_BAD_PASSPHRASE: */
/*       gpa_window_error (_("Wrong passphrase!"), GPA_OPERATION (op)->window); */
/*       break; */
    default:
      gpa_gpgme_warn (err, NULL, GPA_OPERATION (op)->context);
      break;
    }
}

/* Operation is ready.  Tell the server.  */
static void
done_cb (GpaContext *context, gpg_error_t err, GpaStreamEncryptOperation *op)
{
  gtk_widget_hide (GPA_STREAM_OPERATION (op)->progress_dialog);

  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}




/* Public API.  */

/* Start encrypting INPUT_STREAM to OUTPUT_STREAM using WINDOW.
   RECIPIENTS gives a list of recipients and the function matches them
   with existing keys and selects appropriate keys.  RECP_KEYS is
   either NULL or an array with gpgme keys which will then immediatley
   be used and suppresses the recipient key selection dialog.

   If it is not possible to unambigiously select keys and SILENT is
   not given, a key selection dialog offers the user a way to manually
   select keys.  INPUT_STREAM and OUTPUT_STREAM may be given as NULL
   in which case the function skips the actual encryption step and
   just verifies the recipients.  */
GpaStreamEncryptOperation*
gpa_stream_encrypt_operation_new (GtkWidget *window,
                                  gpgme_data_t input_stream,
                                  gpgme_data_t output_stream,
                                  GSList *recipients,
                                  gpgme_key_t *recp_keys,
                                  gpgme_protocol_t protocol,
                                  int silent)
{
  GpaStreamEncryptOperation *op;

  /* Fixme: SILENT is not yet implemented.  */
  g_debug ("recipients %p  recp_keys %p", recipients, recp_keys);
  op = g_object_new (GPA_STREAM_ENCRYPT_OPERATION_TYPE,
		     "window", window,
		     "input_stream", input_stream,
		     "output_stream", output_stream,
                     "recipients", copy_recipients (recipients),
                     "recipient-keys", gpa_gpgme_copy_keyarray (recp_keys),
                     "protocol", (int)protocol,
		     NULL);

  return op;
}


/* Return an array of keys for the set of recipients of this object.
   The function also returns the selected protocol.  */
gpgme_key_t *
gpa_stream_encrypt_operation_get_keys (GpaStreamEncryptOperation *op,
                                       gpgme_protocol_t *r_protocol)
{
  g_return_val_if_fail (op, NULL);

  if (r_protocol)
    *r_protocol = op->selected_protocol;
  return gpa_gpgme_copy_keyarray (op->keys);
}
