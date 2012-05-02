/* gpastreamsignop.c - The GpaStreamSignOperation object.
   Copyright (C) 2008 g10 Code GmbH

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

#include "gpgmetools.h"
#include "gtktools.h"
#include "verifydlg.h"
#include "gpastreamdecryptop.h"


struct _GpaStreamDecryptOperation
{
  GpaStreamOperation parent;

  GtkWidget *dialog;

  gboolean no_verify;

  gpgme_protocol_t selected_protocol;
};


struct _GpaStreamDecryptOperationClass
{
  GpaStreamOperationClass parent_class;
};



/* Indentifiers for our properties. */
enum
  {
    PROP_0,
    PROP_NO_VERIFY,
    PROP_PROTOCOL
  };


static gboolean idle_cb (gpointer data);
static void response_cb (GtkDialog *dialog, gint response, gpointer user_data);
static void done_error_cb (GpaContext *context, gpg_error_t err,
                           GpaStreamDecryptOperation *op);
static void done_cb (GpaContext *context, gpg_error_t err,
                     GpaStreamDecryptOperation *op);

static GObjectClass *parent_class;



static void
gpa_stream_decrypt_operation_get_property (GObject *object, guint prop_id,
					  GValue *value, GParamSpec *pspec)
{
  GpaStreamDecryptOperation *op = GPA_STREAM_DECRYPT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_NO_VERIFY:
      g_value_set_boolean (value, op->no_verify);
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
gpa_stream_decrypt_operation_set_property (GObject *object, guint prop_id,
					  const GValue *value,
					  GParamSpec *pspec)
{
  GpaStreamDecryptOperation *op = GPA_STREAM_DECRYPT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_NO_VERIFY:
      op->no_verify = g_value_get_boolean (value);
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
gpa_stream_decrypt_operation_finalize (GObject *object)
{
  GpaStreamDecryptOperation *op = GPA_STREAM_DECRYPT_OPERATION (object);

  if (op->dialog)
    gtk_widget_destroy (op->dialog);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_stream_decrypt_operation_init (GpaStreamDecryptOperation *op)
{
  op->dialog = NULL;
}


static GObject*
gpa_stream_decrypt_operation_ctor (GType type, guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaStreamDecryptOperation *op;

  object = parent_class->constructor (type, n_construct_properties,
				      construct_properties);
  op = GPA_STREAM_DECRYPT_OPERATION (object);

  /* Start with the first file after going back into the main loop */
  g_idle_add (idle_cb, op);

  /* We connect the done signal to two handles.  The error handler is
     called first.  */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (done_cb), op);

  gtk_window_set_title
    (GTK_WINDOW (GPA_STREAM_OPERATION (op)->progress_dialog),
     _("Decrypting message ..."));

  if (! op->no_verify)
    {
      op->dialog = gpa_file_verify_dialog_new (GPA_OPERATION (op)->window);
      g_signal_connect (G_OBJECT (op->dialog), "response",
			G_CALLBACK (response_cb), op);
    }

  return object;
}


static void
gpa_stream_decrypt_operation_class_init (GpaStreamDecryptOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_stream_decrypt_operation_ctor;
  object_class->finalize = gpa_stream_decrypt_operation_finalize;
  object_class->set_property = gpa_stream_decrypt_operation_set_property;
  object_class->get_property = gpa_stream_decrypt_operation_get_property;

  g_object_class_install_property (object_class, PROP_NO_VERIFY,
				   g_param_spec_boolean
				   ("no-verify", "No Verify",
				    "Flag requesting no verify operation.",
				    FALSE,
				    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property
    (object_class, PROP_PROTOCOL,
     g_param_spec_int
     ("protocol", "Protocol",
      "The gpgme protocol currently selected.",
      GPGME_PROTOCOL_OpenPGP, GPGME_PROTOCOL_UNKNOWN, GPGME_PROTOCOL_UNKNOWN,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

}


GType
gpa_stream_decrypt_operation_get_type (void)
{
  static GType stream_decrypt_operation_type = 0;

  if (! stream_decrypt_operation_type)
    {
      static const GTypeInfo stream_decrypt_operation_info =
      {
        sizeof (GpaStreamDecryptOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_stream_decrypt_operation_class_init,
        NULL, /* class_finalize */
        NULL, /* class_data */
        sizeof (GpaStreamDecryptOperation),
        0,    /* n_preallocs */
        (GInstanceInitFunc) gpa_stream_decrypt_operation_init,
      };

      stream_decrypt_operation_type = g_type_register_static
	(GPA_STREAM_OPERATION_TYPE, "GpaStreamDecryptOperation",
	 &stream_decrypt_operation_info, 0);
    }

  return stream_decrypt_operation_type;
}


/* The decrypt status dialog has returned.  */
static void
response_cb (GtkDialog *dialog, int response, void *user_data)
{
  GpaStreamDecryptOperation *op = user_data;

  gtk_widget_hide (GTK_WIDGET (dialog));

  g_signal_emit_by_name (GPA_OPERATION (op), "completed", 0);
}


/* Show an error message. */
static void
done_error_cb (GpaContext *context, gpg_error_t err,
               GpaStreamDecryptOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;
    default:
      gpa_gpgme_warning (err);
      break;
    }
}


/* Percent-Escape special characters.  */
static gchar *
my_percent_escape (const gchar *src)
{
  gchar *esc_str;
  int new_len = 3 * strlen (src) + 1;
  gchar *dst;

  esc_str = g_malloc (new_len);

  dst = esc_str;
  while (*src)
    {
      if (*src == '%')
	{
	  *(dst++) = '%';
	  *(dst++) = '2';
	  *(dst++) = '5';
	}
      else if (*src == ':')
	{
	  /* The colon is used as field separator.  */
	  *(dst++) = '%';
	  *(dst++) = '3';
	  *(dst++) = 'a';
	}
      else if (*src == ',')
	{
	  /* The comma is used as list separator.  */
	  *(dst++) = '%';
	  *(dst++) = '2';
	  *(dst++) = 'c';
	}
      else
	*(dst++) = *(src);
      src++;
    }
  *dst = '\0';
  return esc_str;
}


/* Operation is ready.  Tell the server.  */
static void
done_cb (GpaContext *context, gpg_error_t err, GpaStreamDecryptOperation *op)
{
  gtk_widget_hide (GPA_STREAM_OPERATION (op)->progress_dialog);

  if (! err && ! op->no_verify)
    {
      gpgme_verify_result_t res;
      gpgme_signature_t sig;

      res = gpgme_op_verify_result (GPA_OPERATION (op)->context->ctx);

      for (sig = res->signatures; sig; sig = sig->next)
	{
	  char *sigsum;
	  char *sigdesc;
	  char *sigdesc_esc;

	  if (sig->summary & GPGME_SIGSUM_VALID)
	    sigsum = "green";
	  else if (sig->summary & GPGME_SIGSUM_GREEN)
	    sigsum = "yellow";
	  else if (sig->summary & GPGME_SIGSUM_KEY_MISSING)
	    sigsum = "none";
	  else
	    sigsum = "red";

          sigdesc = gpa_gpgme_get_signature_desc
            (GPA_OPERATION (op)->context->ctx, sig, NULL, NULL);

	  sigdesc_esc = my_percent_escape (sigdesc);

	  /* FIXME: Error handling.  */
	  err = gpa_operation_write_status (GPA_OPERATION (op), "SIGSTATUS",
					    sigsum, sigdesc_esc, NULL);

	  g_free (sigdesc_esc);
	  g_free (sigdesc);
	}

      if (res->signatures)
	{
	  /* Add the file to the result dialog.  */
	  gpa_file_verify_dialog_add_file
	    (GPA_FILE_VERIFY_DIALOG (op->dialog),
	     _("Document"), NULL, NULL, res->signatures);
	  gtk_widget_show_all (op->dialog);

	  /* We will complete later in response callback.  */
	  return;
	}
    }

  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}


static gboolean
idle_cb (gpointer data)
{
  GpaStreamDecryptOperation *op = data;
  GpaStreamOperation *sop = GPA_STREAM_OPERATION (op);
  gpgme_error_t err;

  gpgme_set_protocol (GPA_OPERATION (op)->context->ctx, op->selected_protocol);

  if (op->no_verify)
    err = gpgme_op_decrypt_start (GPA_OPERATION (op)->context->ctx,
				  sop->input_stream, sop->output_stream);
  else
    err = gpgme_op_decrypt_verify_start (GPA_OPERATION (op)->context->ctx,
					 sop->input_stream, sop->output_stream);

  if (err)
    {
      gpa_gpgme_warning (err);
      g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
    }

  gtk_widget_show_all (GPA_STREAM_OPERATION (op)->progress_dialog);

  return FALSE;
}


/* Public API.  */

/* FIXME: Doc.  */
GpaStreamDecryptOperation *
gpa_stream_decrypt_operation_new (GtkWidget *window,
                                  gpgme_data_t input_stream,
                                  gpgme_data_t output_stream,
                                  gboolean no_verify,
                                  gpgme_protocol_t protocol,
                                  const char *title)
{
  GpaStreamDecryptOperation *op;

  op = g_object_new (GPA_STREAM_DECRYPT_OPERATION_TYPE,
		     "window", window,
		     "input_stream", input_stream,
		     "output_stream", output_stream,
                     "no-verify", no_verify,
                     "protocol", (int) protocol,
                     "client-title", title,
		     NULL);

  return op;
}
