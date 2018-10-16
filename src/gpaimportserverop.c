/* gpaimportserverop.c - The GpaImportServerOperation object.
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

#include <gpgme.h>
#include <unistd.h>
#include "gpa.h"
#include "i18n.h"
#include "gtktools.h"
#include "gparecvkeydlg.h"
#include "gpaimportserverop.h"
#include "server-access.h"

/* The number of keys we allow to import at once.  If we have more
   than this we terminate the dialog and ask the user to give a better
   specification of the key.  A better way to do this would be to pop
   up a dialog to allow the user to select matching keys.  */
#define MAX_KEYSEARCH_RESULTS 5


static GObjectClass *parent_class = NULL;

static gboolean
gpa_import_server_operation_get_source (GpaImportOperation *operation);
static void
gpa_import_server_operation_complete_import (GpaImportOperation *operation);

/* GObject boilerplate */

static void
gpa_import_server_operation_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_import_server_operation_init (GpaImportServerOperation *op)
{
}

static GObject*
gpa_import_server_operation_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  /* GpaImportServerOperation *op; */

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  /* op = GPA_IMPORT_SERVER_OPERATION (object); */

  return object;
}

static void
gpa_import_server_operation_class_init (GpaImportServerOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GpaImportOperationClass *import_class = GPA_IMPORT_OPERATION_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_import_server_operation_constructor;
  object_class->finalize = gpa_import_server_operation_finalize;
  import_class->get_source = gpa_import_server_operation_get_source;
  import_class->complete_import = gpa_import_server_operation_complete_import;
}

GType
gpa_import_server_operation_get_type (void)
{
  static GType server_operation_type = 0;

  if (!server_operation_type)
    {
      static const GTypeInfo server_operation_info =
      {
        sizeof (GpaImportServerOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_import_server_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaImportServerOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_import_server_operation_init,
      };

      server_operation_type = g_type_register_static (GPA_IMPORT_OPERATION_TYPE,
						    "GpaImportServerOperation",
						    &server_operation_info, 0);
    }

  return server_operation_type;
}

/* Internal */

/* Search for keys with KEYID.  Return true on success and set the
   SOURCE2 instance variable.  */
static gboolean
search_keys (GpaImportOperation *operation, const char *keyid)
{
  gpg_error_t err;
  gboolean result = FALSE;
  GpaContext *context;
  gpgme_key_t key;
  gpgme_key_t *keyarray;
  int i, nkeys;
  gpgme_keylist_mode_t listmode;
  char *mbox = NULL;

  if (!keyid || !*keyid)
    return FALSE;

  keyarray = g_malloc0_n (MAX_KEYSEARCH_RESULTS + 1, sizeof *keyarray);


  /* We need to use a separate context because the operation's context
     has already been setup and the done signal would relate to the
     actual import operation done later.  */
  context = gpa_context_new ();
  gpgme_set_protocol (context->ctx, GPGME_PROTOCOL_OpenPGP);
  /* Switch to extern-only or locate list mode.  We use --locate-key
   * iff KEYID is a single mail address.  */
  listmode = GPGME_KEYLIST_MODE_EXTERN;
  mbox = gpgme_addrspec_from_uid (keyid);
  if (mbox)
    {
      listmode = GPGME_KEYLIST_MODE_LOCATE;
      /* We already extracted the mbox - use it directly than letting
       * gnupg extract it.  */
      keyid = mbox;

      gpgme_set_ctx_flag (context->ctx, "auto-key-locate",
                          "clear,nodefault,wkd,keyserver");
    }
  err = gpgme_set_keylist_mode (context->ctx, listmode);
  if (err)
    gpa_gpgme_error (err);

  /* List keys matching the given keyid.  Actually all kind of search
     specifications can be given.  */
  nkeys = 0;
  err = gpgme_op_keylist_start (context->ctx, keyid, 0);
  while (!err && !(err = gpgme_op_keylist_next (context->ctx, &key)))
    {
      if (nkeys >= MAX_KEYSEARCH_RESULTS)
        {
          gpa_show_warn (GPA_OPERATION (operation)->window, context,
                            _("More than %d keys match your search pattern.\n"
                              "Use the long keyid or a fingerprint "
                              "for a better match"), nkeys);
          gpgme_key_unref (key);
          err = gpg_error (GPG_ERR_TRUNCATED);
          break;
        }
      keyarray[nkeys++] = key;
    }
  gpgme_op_keylist_end (context->ctx);
  if (gpg_err_code (err) == GPG_ERR_EOF)
    err = 0;

  if (!err && !nkeys)
    {
      gpa_show_warn (GPA_OPERATION (operation)->window, context,
                     _("No keys were found."));
    }
  else if (!err)
    {
      operation->source2 = keyarray;
      keyarray = NULL;
      result = TRUE;
    }
  else if (gpg_err_code (err) != GPG_ERR_TRUNCATED)
    gpa_gpgme_warn (err, NULL, context);

  g_object_unref (context);
  if (keyarray)
    {
      for (i=0; keyarray[i]; i++)
        gpgme_key_unref (keyarray[i]);
      g_free (keyarray);
    }

  gpgme_free (mbox);
  return result;
}


/* Virtual methods */

static gboolean
gpa_import_server_operation_get_source (GpaImportOperation *operation)
{
  GpaImportServerOperation *op = GPA_IMPORT_SERVER_OPERATION (operation);
  GtkWidget *dialog;
  GtkResponseType response;
  gchar *keyid;
  int i;

  dialog = gpa_receive_key_dialog_new (GPA_OPERATION (op)->window);
  gtk_widget_show_all (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  keyid = g_strdup (gpa_receive_key_dialog_get_id
		    (GPA_RECEIVE_KEY_DIALOG (dialog)));
  gtk_widget_destroy (dialog);

  /* Better reset the source variables.  */
  gpgme_data_release (operation->source);
  operation->source = NULL;
  if (operation->source2)
    {
      for (i=0; operation->source2[i]; i++)
        gpgme_key_unref (operation->source2[i]);
      g_free (operation->source2);
      operation->source2 = NULL;
    }

  if (response == GTK_RESPONSE_OK && is_gpg_version_at_least ("2.1.0"))
    {
      /* GnuPG 2.1.0 does not anymore use the keyserver helpers and
         thus we need to use the real API for receiving keys.  Given
         that there is currently no way to create a list of keys from
         the keyids to be passed to the import function we run a
         --search-keys first to get the list of matching keys and pass
         them to the actual import function (which does a --recv-keys).  */
      /* Fixme: As with server_get_key (below), this is a blocking
         operation. */
      if (search_keys (operation, keyid))
        {
          /* Okay, found key(s).  */
	  g_free (keyid);
	  return TRUE;
        }
    }
  else if (response == GTK_RESPONSE_OK)
    {
      if (server_get_key (gpa_options_get_default_keyserver
			  (gpa_options_get_instance ()),
			  keyid,
                          &operation->source, GPA_OPERATION (op)->window))
	{
	  g_free (keyid);
	  return TRUE;
	}
    }
  g_free (keyid);
  return FALSE;
}


static void
gpa_import_server_operation_complete_import (GpaImportOperation *operation)
{
  /* Nothing special to do */
}

/* API */

GpaImportServerOperation*
gpa_import_server_operation_new (GtkWidget *window)
{
  GpaImportServerOperation *op;

  op = g_object_new (GPA_IMPORT_SERVER_OPERATION_TYPE,
		     "window", window, NULL);

  return op;
}
