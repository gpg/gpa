/* gpakeysignop.c - The GpaKeySignOperation object.
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

#include <config.h>

#include <glib.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#else
#include <io.h>
#endif

#include "gpa.h"
#include "gpakeysignop.h"
#include "keysigndlg.h"
#include "gpgmeedit.h"
#include "gtktools.h"

/* Internal functions */
static gboolean gpa_key_sign_operation_idle_cb (gpointer data);
static void gpa_key_sign_operation_done_error_cb (GpaContext *context,
						    gpg_error_t err,
						    GpaKeySignOperation *op);
static void gpa_key_sign_operation_done_cb (GpaContext *context,
					      gpg_error_t err,
					      GpaKeySignOperation *op);

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_key_sign_operation_finalize (GObject *object)
{
  GpaKeySignOperation *op = GPA_KEY_SIGN_OPERATION (object);
  if (op->signer_key)
    {
      gpgme_key_unref (op->signer_key);
    }
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_key_sign_operation_init (GpaKeySignOperation *op)
{
  op->signer_key = NULL;
  op->signed_keys = 0;
}

static GObject*
gpa_key_sign_operation_constructor (GType type,
				      guint n_construct_properties,
				      GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaKeySignOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_KEY_SIGN_OPERATION (object);
  /* Initialize */

  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_key_sign_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_key_sign_operation_done_cb), op);
  /* Start with the first key after going back into the main loop */
  g_idle_add (gpa_key_sign_operation_idle_cb, op);

  return object;
}

static void
gpa_key_sign_operation_class_init (GpaKeySignOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_key_sign_operation_constructor;
  object_class->finalize = gpa_key_sign_operation_finalize;
}

GType
gpa_key_sign_operation_get_type (void)
{
  static GType key_sign_operation_type = 0;

  if (!key_sign_operation_type)
    {
      static const GTypeInfo key_sign_operation_info =
      {
        sizeof (GpaKeySignOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_key_sign_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaKeySignOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_key_sign_operation_init,
      };

      key_sign_operation_type = g_type_register_static
	(GPA_KEY_OPERATION_TYPE, "GpaKeySignOperation",
	 &key_sign_operation_info, 0);
    }

  return key_sign_operation_type;
}

/* API */

/* Creates a new key deletion operation.
 */
GpaKeySignOperation*
gpa_key_sign_operation_new (GtkWidget *window, GList *keys)
{
  GpaKeySignOperation *op;

  op = g_object_new (GPA_KEY_SIGN_OPERATION_TYPE,
		     "window", window,
		     "keys", keys,
		     NULL);

  return op;
}

/* Internal */

static gpg_error_t
gpa_key_sign_operation_start (GpaKeySignOperation *op)
{
  gpg_error_t err;
  gpgme_key_t key;
  gboolean sign_locally = FALSE;

  key = gpa_key_operation_current_key (GPA_KEY_OPERATION (op));
  g_return_val_if_fail (key, gpg_error (GPG_ERR_CANCELED));
  if (key->protocol != GPGME_PROTOCOL_OpenPGP)
    return 0;

  if (! gpa_key_sign_run_dialog (GPA_OPERATION (op)->window,
				 key, &sign_locally))
    return gpg_error (GPG_ERR_CANCELED);

  err = gpa_gpgme_edit_sign_start  (GPA_OPERATION(op)->context, key,
				    op->signer_key, sign_locally);
  if (err)
    {
      gpa_gpgme_warning (err);
      return err;
    }

  return 0;
}


static gboolean
gpa_key_sign_operation_idle_cb (gpointer data)
{
  GpaKeySignOperation *op = data;
  gpg_error_t err;

  /* Get the signer key and abort if there isn't one */
  op->signer_key = gpa_options_get_default_key (gpa_options_get_instance ());
  if (! op->signer_key)
    {
      gpa_window_error (_("No private key for signing."),
			GPA_OPERATION (op)->window);
      /* FIXME: Error code?  */
      g_signal_emit_by_name (GPA_OPERATION (op), "completed", 0);
      return FALSE;
    }
  gpgme_key_ref (op->signer_key);

  err = gpa_key_sign_operation_start (op);
  if (err)
    g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);

  return FALSE;
}


static void
gpa_key_sign_operation_next (GpaKeySignOperation *op)
{
  gpg_error_t err = 0;

  if (GPA_KEY_OPERATION (op)->current)
    {
      err = gpa_key_sign_operation_start (op);
      if (! err)
	return;
    }

  if (op->signed_keys > 0)
    g_signal_emit_by_name (GPA_OPERATION (op), "changed_wot");
  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}


static void
gpa_key_sign_operation_done_error_cb (GpaContext *context,
                                      gpg_error_t err,
                                      GpaKeySignOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
      op->signed_keys++;
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;
    case GPG_ERR_BAD_PASSPHRASE:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     _("Wrong passphrase!"));
      break;
    case GPG_ERR_UNUSABLE_PUBKEY:
      /* Couldn't sign because the key was expired */
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     _("This key has expired! Unable to sign."));
      break;
    case GPG_ERR_CONFLICT:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     _("This key has already been signed with your own!"));
      break;
    case GPG_ERR_NO_SECKEY:
      /* Couldn't sign because there is no default key */
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                     _("You haven't selected a default key to sign with!"));
      break;
    default:
      gpa_gpgme_warn (err, NULL, GPA_OPERATION (op)->context);
      break;
    }
}

static void
gpa_key_sign_operation_done_cb (GpaContext *context,
                                gpg_error_t err,
                                GpaKeySignOperation *op)
{
  GPA_KEY_OPERATION (op)->current = g_list_next
    (GPA_KEY_OPERATION (op)->current);
  gpa_key_sign_operation_next (op);
}
