/* gpastreamdecryptop.c - The GpaOperation object.
 *	Copyright (C) 2007 g10 Code GmbH
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
#include "encryptdlg.h"
#include "gpawidgets.h"
#include "gpapastrings.h"
#include "gpastreamencryptop.h"

static void response_cb (GtkDialog *dialog,
                         gint response,
                         gpointer user_data);
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
gpa_stream_encrypt_operation_finalize (GObject *object)
{  
  GpaStreamEncryptOperation *op = GPA_STREAM_ENCRYPT_OPERATION (object);

  if (op->recipients)
    {
      g_slist_foreach (op->recipients, free_func, NULL);
      g_slist_free (op->recipients);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_stream_encrypt_operation_init (GpaStreamEncryptOperation *op)
{
  op->encrypt_dialog = NULL;
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

  /* Create the "Encrypt" dialog */
  op->encrypt_dialog = gpa_file_encrypt_dialog_new
    (GPA_OPERATION (op)->window);

  g_signal_connect (G_OBJECT (op->encrypt_dialog), "response",
		    G_CALLBACK (response_cb), op);
  /* We connect the done signal to two handles.  The error handler is
     called first.  */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (done_cb), op);

  gtk_window_set_title 
    (GTK_WINDOW (GPA_STREAM_OPERATION (op)->progress_dialog),
			_("Encrypting message ..."));

  gtk_widget_show_all (op->encrypt_dialog);

  return object;
}


static void
gpa_stream_encrypt_operation_class_init (GpaStreamEncryptOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_stream_encrypt_operation_constructor;
  object_class->finalize = gpa_stream_encrypt_operation_finalize;
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


static gpg_error_t
start_encryption (GpaStreamEncryptOperation *op, gpgme_key_t *keys)
{
  gpg_error_t err;
  
  /* Start the operation */
  /* Always trust keys, because any untrusted keys were already confirmed
   * by the user.
   */
/*   if (gpa_file_encrypt_dialog_sign  */
/*       (GPA_FILE_ENCRYPT_DIALOG (op->encrypt_dialog))) */
/*     { */
/*       /\* Signing was request as well.  *\/ */
/*       err = gpgme_op_encrypt_sign_start (GPA_OPERATION (op)->context->ctx, */
/* 					 op->rset, GPGME_ENCRYPT_ALWAYS_TRUST, */
/* 					 op->plain, op->cipher); */
/*     } */
/*   else */
    {
      /* Plain encryption.  */
      err = gpgme_op_encrypt_start (GPA_OPERATION (op)->context->ctx,
				    keys, GPGME_ENCRYPT_ALWAYS_TRUST,
				    GPA_STREAM_OPERATION (op)->input_stream,
				    GPA_STREAM_OPERATION (op)->output_stream);
    }
  if (err)
    {
      gpa_gpgme_warning (err);
      return err;
    }

  /* Show and update the progress dialog */
  gtk_widget_show_all (GPA_STREAM_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG 
				 (GPA_STREAM_OPERATION (op)->progress_dialog),
				 _("Message encryption"));
  return 0;
}



/*
 * Setting the recipients for the context.
 */

/* static GtkResponseType */
/* ignore_key_trust (gpgme_key_t key, GtkWidget *parent) */
/* { */
/*   GtkWidget *dialog; */
/*   GtkWidget *key_info; */
/*   GtkWidget *vbox; */
/*   GtkWidget *hbox; */
/*   GtkWidget *label; */
/*   GtkWidget *image; */
/*   GtkResponseType response; */

/*   dialog = gtk_dialog_new_with_buttons (_("Unknown Key"), GTK_WINDOW(parent), */
/* 					GTK_DIALOG_MODAL,  */
/* 					_("_Yes"), GTK_RESPONSE_YES, */
/* 					_("_No"), GTK_RESPONSE_NO, */
/* 					NULL); */
/*   gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES); */
/*   gtk_container_set_border_width (GTK_CONTAINER (dialog), 5); */

/*   hbox = gtk_hbox_new (FALSE, 6); */
/*   image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, */
/* 				    GTK_ICON_SIZE_DIALOG); */
/*   gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0); */
/*   vbox = gtk_vbox_new (FALSE, 6); */
/*   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0); */
/*   gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox); */

/*   label = gtk_label_new (_("You are going to encrypt a stream using " */
/* 			   "the following key:")); */
/*   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); */
/*   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5); */
/*   key_info = gpa_key_info_new (key); */
/*   gtk_box_pack_start (GTK_BOX (vbox), key_info, FALSE, TRUE, 5); */
/*   label = gtk_label_new (_("However, it is not certain that the key belongs " */
/* 			   "to that person.")); */
/*   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); */
/*   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5); */
/*   label = gtk_label_new (NULL); */
/*   gtk_label_set_markup (GTK_LABEL (label),  */
/* 			_("Do you <b>really</b> want to use this key?")); */
/*   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); */
/*   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5); */

/*   gtk_widget_show_all (dialog); */
/*   response = gtk_dialog_run (GTK_DIALOG (dialog)); */

/*   gtk_widget_destroy (dialog); */
/*   return response; */
/* } */

/* static void */
/* revoked_key (gpgme_key_t key, GtkWidget *parent) */
/* { */
/*   GtkWidget *dialog; */
/*   GtkWidget *key_info; */
/*   GtkWidget *vbox; */
/*   GtkWidget *hbox; */
/*   GtkWidget *label; */
/*   GtkWidget *image; */

/*   dialog = gtk_dialog_new_with_buttons (_("Revoked Key"), GTK_WINDOW(parent), */
/* 					GTK_DIALOG_MODAL,  */
/* 					_("_Close"), GTK_RESPONSE_CLOSE, */
/* 					NULL); */
/*   gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE); */
/*   gtk_container_set_border_width (GTK_CONTAINER (dialog), 5); */

/*   hbox = gtk_hbox_new (FALSE, 6); */
/*   image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, */
/* 				    GTK_ICON_SIZE_DIALOG); */
/*   gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0); */
/*   vbox = gtk_vbox_new (FALSE, 6); */
/*   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0); */
/*   gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox); */

/*   label = gtk_label_new (_("The following key has been revoked by it's owner:")); */
/*   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); */
/*   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5); */
/*   key_info = gpa_key_info_new (key); */
/*   gtk_box_pack_start (GTK_BOX (vbox), key_info, FALSE, TRUE, 5); */
/*   label = gtk_label_new (_("And can not be used for encryption.")); */
/*   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); */
/*   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5); */

/*   gtk_widget_show_all (dialog); */
/*   gtk_dialog_run (GTK_DIALOG (dialog)); */
/*   gtk_widget_destroy (dialog); */
/* } */

/* static void */
/* expired_key (gpgme_key_t key, GtkWidget *parent) */
/* { */
/*   GtkWidget *dialog; */
/*   GtkWidget *key_info; */
/*   GtkWidget *vbox; */
/*   GtkWidget *hbox; */
/*   GtkWidget *label; */
/*   GtkWidget *image; */
/*   gchar *message; */

/*   dialog = gtk_dialog_new_with_buttons (_("Revoked Key"), GTK_WINDOW(parent), */
/* 					GTK_DIALOG_MODAL,  */
/* 					_("_Close"), GTK_RESPONSE_CLOSE, */
/* 					NULL); */
/*   gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE); */
/*   gtk_container_set_border_width (GTK_CONTAINER (dialog), 5); */

/*   hbox = gtk_hbox_new (FALSE, 6); */
/*   image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, */
/* 				    GTK_ICON_SIZE_DIALOG); */
/*   gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, TRUE, 0); */
/*   vbox = gtk_vbox_new (FALSE, 6); */
/*   gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0); */
/*   gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox); */

/*   message = g_strdup_printf (_("The following key expired on %s:"), */
/*                              gpa_expiry_date_string  */
/*                              (key->subkeys->expires)); */
/*   label = gtk_label_new (message); */
/*   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); */
/*   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5); */
/*   key_info = gpa_key_info_new (key); */
/*   gtk_box_pack_start (GTK_BOX (vbox), key_info, FALSE, TRUE, 5); */
/*   label = gtk_label_new (_("And can not be used for encryption.")); */
/*   gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); */
/*   gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 5); */

/*   gtk_widget_show_all (dialog); */
/*   gtk_dialog_run (GTK_DIALOG (dialog)); */
/*   gtk_widget_destroy (dialog); */
/* } */


/* Given a list of strings, each describing one recipient parse them
   to detect duplicates, check the validity of each key and ask the
   user whether he wants to use an invalid key. 

   Try to find a key for each item in array NAMES. Items not found are
   stored as malloced strings in the newly allocated array UNKNOWN.
   Found keys are stored in the newly allocated array FOUND.  Both
   arrays are terminated by a NULL entry.  Caller needs to release
   FOUND and UNKNOWN.
 */
static gpg_error_t
parse_recipients (GpaStreamEncryptOperation *op, GSList *recipients,
                  gpgme_key_t **r_found, GSList **r_unknown)
{
  gpgme_error_t err;
  GSList *recp;
  gpgme_ctx_t ctx;
  size_t n;
  gpgme_key_t *found;
  GSList *unknown = NULL;
  int found_idx;
  gpgme_key_t key, key2;
  const char *name;

  *r_found = NULL;
  *r_unknown = NULL;

  err = gpgme_new (&ctx);
  if (err)
    return err; 

  for (n=0, recp = recipients; recp; recp = g_slist_next (recp))
    n++;

  found = xcalloc (n+1, sizeof *found);
  found_idx = 0;
  for (recp = recipients; recp; recp = g_slist_next (recp))
    {
      name = recp->data;
      if (!name)
        continue;
      key = NULL;
      err = gpgme_op_keylist_start (ctx, name, 0);
      if (!err)
        {
          err = gpgme_op_keylist_next (ctx, &key);
          if (!err && !gpgme_op_keylist_next (ctx, &key2))
            {
              /* Not found or ambiguous key specification.  */
              gpgme_key_unref (key);
              gpgme_key_unref (key2);
              key = key2 = NULL;
            }
        }
      gpgme_op_keylist_end (ctx);

      /* If a usable key has been found, put it into the list of good
         keys.  All other keys end up in the list of unknown key.  */
      if (key && !key->revoked && !key->disabled && !key->expired)
        {
          gpgme_key_ref (key);
          found[found_idx++] = key;
        }
      else if (key)
	{
	  char *p;
	  const char *warn = (key->revoked? "revoked" :
                              key->expired? "expired" : "disabled");
	  
	  n = strlen (name) + 3 + strlen (warn);
          p = xmalloc (n+1);
	  snprintf (p, n, "%s (%s)", name, warn);
          unknown = g_slist_append (unknown, p);
	}
      else /* Not found or ambiguous.  */
        unknown = g_slist_append (unknown, xstrdup (name));

      gpgme_key_unref (key);
    }
  gpgme_release (ctx);

  *r_found = found;
  *r_unknown = unknown;
  return 0;
}


/*
 * The key selection dialog has returned.
 */
static void 
response_cb (GtkDialog *dialog, int response, void *user_data)
{
  gpg_error_t err;
  GSList *recipients;
  GpaStreamEncryptOperation *op = user_data;
  gpgme_key_t *keys;
  GSList *unknown_recp;

  gtk_widget_hide (GTK_WIDGET (dialog));
  
  if (response != GTK_RESPONSE_OK)
    {
      /* The dialog was canceled, so we do nothing and complete the
       * operation */
      gpa_operation_server_finish (GPA_OPERATION (op), 
                                   gpg_error (GPG_ERR_CANCELED));
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
      return;
    }

  recipients = g_slist_append (NULL, xstrdup ("wk@gnupg.org"));

/* gpa_stream_encrypt_dialog_recipients */
/*     (GPA_STREAM_ENCRYPT_DIALOG (op->encrypt_dialog)); */
      
  err = parse_recipients (op, recipients, &keys, &unknown_recp);
  if (err)
    goto leave;


  /* Our streams work all in ascii armored mode (Either PGP or PEM) */
  gpgme_set_armor (GPA_OPERATION (op)->context->ctx, 1);
  err = start_encryption (op, keys);

 leave:
  if (keys)
    {
      int idx;
      
      for (idx=0; keys[idx]; idx++)
        gpgme_key_unref (keys[idx]);
      xfree (keys);
    }
  g_slist_foreach (unknown_recp, free_func, NULL);
  g_slist_free (unknown_recp);
  g_slist_foreach (recipients, free_func, NULL);
  g_slist_free (recipients);
  if (err)
    {
      gpa_operation_server_finish (GPA_OPERATION (op), err);
      g_signal_emit_by_name (GPA_OPERATION (op), "completed");
    }
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
      gpa_gpgme_warning (err);
      break;
    }
}

/* Operation is ready.  Tell the server.  */
static void
done_cb (GpaContext *context, gpg_error_t err, GpaStreamEncryptOperation *op)
{

  gtk_widget_hide (GPA_STREAM_OPERATION (op)->progress_dialog);

  /* Tell the server that we finished and delete ourself.  */
  gpa_operation_server_finish (GPA_OPERATION (op), err);
  g_signal_emit_by_name (GPA_OPERATION (op), "completed");
}




/* API */

GpaStreamEncryptOperation*
gpa_stream_encrypt_operation_new (GtkWidget *window,
                                  gpgme_data_t input_stream,
                                  gpgme_data_t output_stream,
                                  GSList *recipients,
                                  void *server_ctx)
{
  GpaStreamEncryptOperation *op;
  
  op = g_object_new (GPA_STREAM_ENCRYPT_OPERATION_TYPE,
		     "window", window,
		     "input_stream", input_stream,
		     "output_stream", output_stream,
                     "server-ctx", server_ctx,
		     NULL);
  if (op)
    op->recipients = recipients;

  return op;
}

