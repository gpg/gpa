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

static GObjectClass *parent_class = NULL;

static gboolean
gpa_import_server_operation_get_source (GpaImportOperation *operation,
					gpgme_data_t *source);
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

/* Virtual methods */

static gboolean
gpa_import_server_operation_get_source (GpaImportOperation *operation,
					gpgme_data_t *source)
{
  GpaImportServerOperation *op = GPA_IMPORT_SERVER_OPERATION (operation);
  GtkWidget *dialog = gpa_receive_key_dialog_new (GPA_OPERATION (op)->window);
  GtkResponseType response;
  gchar *keyid;

  gtk_widget_show_all (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  keyid = g_strdup (gpa_receive_key_dialog_get_id
		    (GPA_RECEIVE_KEY_DIALOG (dialog)));
  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_OK)
    {
      if (server_get_key (gpa_options_get_default_keyserver
			  (gpa_options_get_instance ()),
			  keyid, source, GPA_OPERATION (op)->window))
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
