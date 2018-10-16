/* gpaimportop.c - The GpaImportOperation object.
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
#include "gpa.h"
#include "i18n.h"
#include "gtktools.h"
#include "gpaimportop.h"
#include "filetype.h"
#include "gpgmetools.h"

static GObjectClass *parent_class = NULL;

/* Signals */
enum
{
  IMPORTED_KEYS,
  IMPORTED_SECRET_KEYS,
  LAST_SIGNAL
};
static guint signals [LAST_SIGNAL] = { 0 };

static gboolean gpa_import_operation_idle_cb (gpointer data);
static void gpa_import_operation_done_cb (GpaContext *context, gpg_error_t err,
			      GpaImportOperation *op);
static void gpa_import_operation_done_error_cb (GpaContext *context,
						gpg_error_t err,
						GpaImportOperation *op);

/* GObject boilerplate */

static void
gpa_import_operation_finalize (GObject *object)
{
  GpaImportOperation *op = GPA_IMPORT_OPERATION (object);
  int i;

  /* Free the data object, if it exists */
  gpgme_data_release (op->source);
  op->source = NULL;
  if (op->source2)
    {
      for (i=0; op->source2[i]; i++)
        gpgme_key_unref (op->source2[i]);
      g_free (op->source2);
      op->source2 = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_import_operation_init (GpaImportOperation *op)
{
  op->source = NULL;
  op->source2 = NULL;
}

static GObject*
gpa_import_operation_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaImportOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_IMPORT_OPERATION (object);

  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_import_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_import_operation_done_cb), op);

  /* Begin working when we are back into the main loop */
  g_idle_add (gpa_import_operation_idle_cb, op);

  return object;
}

static void
gpa_import_operation_class_init (GpaImportOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_import_operation_constructor;
  object_class->finalize = gpa_import_operation_finalize;

  /* Virtual methods */
  klass->get_source = NULL;
  klass->complete_import = NULL;

  /* Signals */
  klass->imported_keys = NULL;
  signals[IMPORTED_KEYS] =
    g_signal_new ("imported_keys",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GpaImportOperationClass, imported_keys),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  signals[IMPORTED_SECRET_KEYS] =
    g_signal_new ("imported_secret_keys",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GpaImportOperationClass, imported_keys),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

}

GType
gpa_import_operation_get_type (void)
{
  static GType file_operation_type = 0;

  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaImportOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_import_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaImportOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_import_operation_init,
      };

      file_operation_type = g_type_register_static (GPA_OPERATION_TYPE,
						    "GpaImportOperation",
						    &file_operation_info, 0);
    }

  return file_operation_type;
}

/* Private functions */

static gboolean
gpa_import_operation_idle_cb (gpointer data)
{
  GpaImportOperation *op = data;

  if (GPA_IMPORT_OPERATION_GET_CLASS (op)->get_source (op))
    {
      gpg_error_t err;

      if (op->source)
        {
          gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                              is_cms_data_ext (op->source)?
                              GPGME_PROTOCOL_CMS : GPGME_PROTOCOL_OpenPGP);
          err = gpgme_op_import_start (GPA_OPERATION (op)->context->ctx,
                                       op->source);
        }
      else if (op->source2)
        {
          /* The only protocol where an array of keys is used in GPA
             is OpenPGP.  */
          gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                              GPGME_PROTOCOL_OpenPGP);
          err = gpgme_op_import_keys_start (GPA_OPERATION (op)->context->ctx,
                                            op->source2);
        }
      else
        err = gpg_error (GPG_ERR_BUG);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
	}
    }
  else
    /* Abort the operation.  */
    g_signal_emit_by_name (GPA_OPERATION (op), "completed",
			   gpg_error (GPG_ERR_CANCELED));

  return FALSE;
}


static void
gpa_import_operation_done_cb (GpaContext *context, gpg_error_t err,
			      GpaImportOperation *op)
{
  if (! err)
    {
      struct gpa_import_result_s result;
      gpgme_import_result_t res;

      memset (&result, 0, sizeof result);

      GPA_IMPORT_OPERATION_GET_CLASS (op)->complete_import (op);

      res = gpgme_op_import_result (GPA_OPERATION (op)->context->ctx);
      if (res->imported > 0 && res->secret_imported )
	{
	  g_signal_emit_by_name (GPA_OPERATION (op), "imported_secret_keys");
	}
      else if (res->imported > 0)
	{
	  g_signal_emit_by_name (GPA_OPERATION (op), "imported_keys");
	}

      gpa_gpgme_update_import_results (&result, 0, 0, res);
      gpa_gpgme_show_import_results (GPA_OPERATION (op)->window, &result);
    }
  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}


static void
gpa_import_operation_done_error_cb (GpaContext *context, gpg_error_t err,
				    GpaImportOperation *op)
{
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
      /* Ignore these */
      break;
    default:
      gpa_gpgme_warn (err, NULL, GPA_OPERATION (op)->context);
      break;
    }
}
