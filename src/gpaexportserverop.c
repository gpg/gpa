/* gpaexportserverop.c - The GpaExportServerOperation object.
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
#include "gpgmetools.h"
#include "server-access.h"
#include "confdialog.h"
#include "gpaexportserverop.h"

static GObjectClass *parent_class = NULL;

static gboolean
gpa_export_server_operation_get_destination (GpaExportOperation *operation,
						gpgme_data_t *dest,
						gboolean *armor);
static void
gpa_export_server_operation_complete_export (GpaExportOperation *operation);

/* GObject boilerplate */

static void
gpa_export_server_operation_finalize (GObject *object)
{
  GpaExportServerOperation *op = GPA_EXPORT_SERVER_OPERATION (object);

  if (op->server)
    {
      g_free (op->server);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_export_server_operation_init (GpaExportServerOperation *op)
{
  op->server = NULL;
}

static GObject*
gpa_export_server_operation_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  /* GpaExportServerOperation *op; */

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  /* op = GPA_EXPORT_SERVER_OPERATION (object); */

  return object;
}

static void
gpa_export_server_operation_class_init (GpaExportServerOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GpaExportOperationClass *export_class = GPA_EXPORT_OPERATION_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_export_server_operation_constructor;
  object_class->finalize = gpa_export_server_operation_finalize;
  export_class->get_destination = gpa_export_server_operation_get_destination;
  export_class->complete_export = gpa_export_server_operation_complete_export;
}

GType
gpa_export_server_operation_get_type (void)
{
  static GType file_operation_type = 0;

  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaExportServerOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_export_server_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaExportServerOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_export_server_operation_init,
      };

      file_operation_type = g_type_register_static (GPA_EXPORT_OPERATION_TYPE,
						    "GpaExportServerOperation",
						    &file_operation_info, 0);
    }

  return file_operation_type;
}

/* Internal */

static gboolean
confirm_send (GtkWidget *parent, const gchar *server)
{
  GtkWidget *msgbox;
  char *info;
  char *keyserver = NULL;

  if (is_gpg_version_at_least ("2.1.0"))
    {
      keyserver = gpa_load_configured_keyserver ();
      server = keyserver;
    }

  if (!server)
    {
      keyserver = gpa_configure_keyserver (parent);
      if (!keyserver)
        return FALSE;
      server = keyserver;
    }


  info = g_strdup_printf (_("The selected key(s) will be sent to a public key\n"
                            "server (\"%s\")."), server);
  g_free (keyserver);
  msgbox = gtk_message_dialog_new
    (GTK_WINDOW(parent), GTK_DIALOG_MODAL,
     GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
     "%s\n\n%s",
     info,
     _("Are you sure you want to distribute this key?"));
  g_free (info);

  gtk_dialog_add_buttons (GTK_DIALOG (msgbox),
			  _("_Yes"), GTK_RESPONSE_YES,
			  _("_No"), GTK_RESPONSE_NO, NULL);
  if (gtk_dialog_run (GTK_DIALOG (msgbox)) != GTK_RESPONSE_YES)
    {
      gtk_widget_destroy (msgbox);
      return FALSE;
    }
  gtk_widget_destroy (msgbox);
  return TRUE;
}

/* Virtual methods */

static gboolean
gpa_export_server_operation_get_destination (GpaExportOperation *operation,
					     gpgme_data_t *dest,
					     gboolean *armor)
{
  if (confirm_send (GPA_OPERATION (operation)->window,
		    gpa_options_get_default_keyserver
		    (gpa_options_get_instance ())))
    {
      gpg_error_t err;
      *armor = TRUE;
      err = gpgme_data_new (dest);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  return FALSE;
	}
      else
	{
	  return TRUE;
	}
    }
  else
    {
      return FALSE;
    }
}


/* GnuPG 2.1 method to send keys to the keyserver.  KEYLIST has a list
   of keys to be sent.  Returns true on success. */
static gboolean
send_keys (GpaExportServerOperation *op, GList *keylist)
{
  gpg_error_t err;
  GList *item;
  gpgme_key_t *keyarray;
  gpgme_key_t key;
  int i;

  keyarray = g_malloc0_n (g_list_length (keylist)+1, sizeof *keyarray);
  i = 0;
  for (item = keylist; item; i++, item = g_list_next (item))
    {
      key = (gpgme_key_t) item->data;
      if (!key || key->protocol != GPGME_PROTOCOL_OpenPGP)
        continue;
      gpgme_key_ref (key);
      keyarray[i++] = key;
    }

  gpgme_set_protocol (GPA_OPERATION (op)->context->ctx, GPGME_PROTOCOL_OpenPGP);
  err = gpgme_op_export_keys (GPA_OPERATION (op)->context->ctx,
                              keyarray, GPGME_KEYLIST_MODE_EXTERN, NULL);
  for (i=0; keyarray[i]; i++)
    gpgme_key_unref (keyarray[i]);
  g_free (keyarray);

  if (err)
    {
      gpa_show_warn (GPA_OPERATION (op)->window, NULL,
                        "%s\n\n(%s <%s>)",
                        _("Error sending key(s) to the server."),
                        gpg_strerror (err), gpg_strsource (err));
      return FALSE;
    }

  return TRUE;
}



static void
gpa_export_server_operation_complete_export (GpaExportOperation *operation)
{
  GpaExportServerOperation *op = GPA_EXPORT_SERVER_OPERATION (operation);
  int okay = 0;

  if (is_gpg_version_at_least ("2.1.0"))
    {
      /* GnuPG 2.1.0 does not anymore use the keyserver helpers and
         thus we need to use the real API for sending keys.  */
      if (send_keys (op, operation->keys))
        okay = 1;
    }
  else
    {
      gpgme_key_t key = (gpgme_key_t) operation->keys->data;

      op->server = g_strdup (gpa_options_get_default_keyserver
                             (gpa_options_get_instance ()));
      if (server_send_keys (op->server, key->subkeys->keyid, operation->dest,
                             GPA_OPERATION (op)->window))
        okay = 1;
    }

  if (okay)
    gpa_window_message (_("The keys have been sent to the server."),
                        GPA_OPERATION (op)->window);
}

/* API */

GpaExportServerOperation*
gpa_export_server_operation_new (GtkWidget *window, GList *keys)
{
  GpaExportServerOperation *op;

  op = g_object_new (GPA_EXPORT_SERVER_OPERATION_TYPE,
		     "window", window,
		     "keys", keys,
		     NULL);

  return op;
}
