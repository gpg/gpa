/* gpaimportbykeyidop.c - The GpaImportByKeyidOperation object.
 * Copyright (C) 2014 g10 Code GmbH
 *
 * This file is part of GPA
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

#include <gpgme.h>
#include <unistd.h>
#include "gpa.h"
#include "i18n.h"
#include "gtktools.h"
#include "gpaimportbykeyidop.h"
#include "server-access.h"


static GObjectClass *parent_class = NULL;

static gboolean
gpa_import_bykeyid_operation_get_source (GpaImportOperation *operation);
static void
gpa_import_bykeyid_operation_complete_import (GpaImportOperation *operation);

/* GObject boilerplate */

static void
gpa_import_bykeyid_operation_finalize (GObject *object)
{
  GpaImportByKeyidOperation *op = GPA_IMPORT_BYKEYID_OPERATION (object);

  gpgme_key_unref (op->key);
  op->key = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_import_bykeyid_operation_init (GpaImportByKeyidOperation *op)
{
  op->key = NULL;
}


static GObject*
gpa_import_bykeyid_operation_constructor (GType type,
                                          guint n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
  GObject *object;
  /* GpaImportByKeyidOperation *op; */

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  /* op = GPA_IMPORT_BYKEYID_OPERATION (object); */

  return object;
}


static void
gpa_import_bykeyid_operation_class_init (GpaImportByKeyidOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GpaImportOperationClass *import_class = GPA_IMPORT_OPERATION_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_import_bykeyid_operation_constructor;
  object_class->finalize    = gpa_import_bykeyid_operation_finalize;

  import_class->get_source  = gpa_import_bykeyid_operation_get_source;
  import_class->complete_import = gpa_import_bykeyid_operation_complete_import;
}


GType
gpa_import_bykeyid_operation_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo info =
      {
        sizeof (GpaImportByKeyidOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_import_bykeyid_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaImportByKeyidOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_import_bykeyid_operation_init,
      };

      type = g_type_register_static (GPA_IMPORT_OPERATION_TYPE,
                                     "GpaImportByKeyidOperation",
                                     &info, 0);
    }

  return type;
}


/* Virtual methods */

static gboolean
gpa_import_bykeyid_operation_get_source (GpaImportOperation *operation)
{
  GpaImportByKeyidOperation *op = GPA_IMPORT_BYKEYID_OPERATION (operation);
  int i;

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

  if (!op->key
      || op->key->protocol != GPGME_PROTOCOL_OpenPGP
      || !op->key->subkeys
      || !op->key->subkeys->keyid
      || !*op->key->subkeys->keyid)
    ;
  else if (is_gpg_version_at_least ("2.1.0"))
    {
      operation->source2 = g_malloc0_n (1 + 1, sizeof *operation->source2);
      gpgme_key_ref (op->key);
      operation->source2[0] = op->key;
      return TRUE;
    }
  else
    {

      if (server_get_key (gpa_options_get_default_keyserver
			  (gpa_options_get_instance ()),
			  op->key->subkeys->keyid,
                          &operation->source, GPA_OPERATION (op)->window))
	{
	  return TRUE;
	}
    }
  return FALSE;
}


static void
gpa_import_bykeyid_operation_complete_import (GpaImportOperation *operation)
{
  (void)operation;
}


/* API */

GpaImportByKeyidOperation*
gpa_import_bykeyid_operation_new (GtkWidget *window, gpgme_key_t key)
{
  GpaImportByKeyidOperation *op;

  op = g_object_new (GPA_IMPORT_BYKEYID_OPERATION_TYPE,
		     "window", window, NULL);
  gpgme_key_ref (key);
  op->key = key;

  return op;
}
