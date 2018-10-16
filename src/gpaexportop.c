/* gpaexportfileop.c - The GpaExportFileOperation object.
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
#include "gpaexportop.h"

static GObjectClass *parent_class = NULL;

/* Properties */
enum
{
  PROP_0,
  PROP_KEYS,
  PROP_SECRET
};

static gboolean gpa_export_operation_idle_cb (gpointer data);
static void gpa_export_operation_done_cb (GpaContext *context, gpg_error_t err,
			      GpaExportOperation *op);
static void gpa_export_operation_done_error_cb (GpaContext *context,
						gpg_error_t err,
						GpaExportOperation *op);

/* GObject boilerplate */

static void
gpa_export_operation_get_property (GObject     *object,
				 guint        prop_id,
				 GValue      *value,
				 GParamSpec  *pspec)
{
  GpaExportOperation *op = GPA_EXPORT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_KEYS:
      g_value_set_pointer (value, op->keys);
      break;
    case PROP_SECRET:
      g_value_set_boolean (value, op->secret);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_export_operation_set_property (GObject     *object,
				 guint        prop_id,
				 const GValue      *value,
				 GParamSpec  *pspec)
{
  GpaExportOperation *op = GPA_EXPORT_OPERATION (object);

  switch (prop_id)
    {
    case PROP_KEYS:
      op->keys = (GList*) g_value_get_pointer (value);
      /* Make sure we keep a reference for our keys */
      g_list_foreach (op->keys, (GFunc) gpgme_key_ref, NULL);
      break;
    case PROP_SECRET:
      op->secret = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_export_operation_finalize (GObject *object)
{
  GpaExportOperation *op = GPA_EXPORT_OPERATION (object);

  /* Free each key, and then the list of keys */
  g_list_foreach (op->keys, (GFunc) gpgme_key_unref, NULL);
  g_list_free (op->keys);
  /* Free the data object, if it exists */
  if (op->dest)
    {
      gpgme_data_release (op->dest);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_export_operation_init (GpaExportOperation *op)
{
  op->keys = NULL;
  op->dest = NULL;
  op->secret = 0;
}

static GObject*
gpa_export_operation_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaExportOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_EXPORT_OPERATION (object);


  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_export_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_export_operation_done_cb), op);

  /* Begin working when we are back into the main loop */
  g_idle_add (gpa_export_operation_idle_cb, op);

  return object;
}

static void
gpa_export_operation_class_init (GpaExportOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_export_operation_constructor;
  object_class->finalize = gpa_export_operation_finalize;
  object_class->set_property = gpa_export_operation_set_property;
  object_class->get_property = gpa_export_operation_get_property;

  /* Virtual methods */
  klass->get_destination = NULL;
  klass->complete_export = NULL;

  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_KEYS,
				   g_param_spec_pointer
				   ("keys", "Keys",
				    "Keys",
				    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_SECRET,
				   g_param_spec_boolean
				   ("secret", "Secret",
				    "Secret", FALSE,
				    G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_export_operation_get_type (void)
{
  static GType file_operation_type = 0;

  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaExportOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_export_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaExportOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_export_operation_init,
      };

      file_operation_type = g_type_register_static (GPA_OPERATION_TYPE,
						    "GpaExportOperation",
						    &file_operation_info, 0);
    }

  return file_operation_type;
}

/* Private functions */

static gboolean
gpa_export_operation_idle_cb (gpointer data)
{
  GpaExportOperation *op = data;
  gboolean armor = TRUE;

  if (GPA_EXPORT_OPERATION_GET_CLASS (op)->get_destination (op, &op->dest,
							    &armor))
    {
      gpg_error_t err = 0;
      const char **patterns;
      GList *k;
      int i;
      gpgme_protocol_t prot = GPGME_PROTOCOL_UNKNOWN;
      gboolean secret;

      gpgme_set_armor (GPA_OPERATION (op)->context->ctx, armor);
      /* Create the set of keys to export */
      patterns = g_malloc0 (sizeof(gchar*)*(g_list_length(op->keys)+1));
      for (i = 0, k = op->keys; k; i++, k = g_list_next (k))
	{
	  gpgme_key_t key = (gpgme_key_t) k->data;
	  patterns[i] = key->subkeys->fpr;
          if (prot == GPGME_PROTOCOL_UNKNOWN)
            prot = key->protocol;
          else if (prot != key->protocol)
            {
              gpa_window_error
                (_("Only keys of the same procotol may be exported"
                   " as a collection."), NULL);
              g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
              goto cleanup;
            }
	}
      if (prot == GPGME_PROTOCOL_UNKNOWN)
        {
          g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
          goto cleanup;  /* No keys.  */
        }
      gpgme_set_protocol (GPA_OPERATION (op)->context->ctx, prot);
      /* Export to the gpgme_data_t */
      g_object_get (op, "secret", &secret, NULL);
      err = gpgme_op_export_ext_start (GPA_OPERATION (op)->context->ctx,
				       patterns,
                                       secret? GPGME_EXPORT_MODE_SECRET : 0,
                                       op->dest);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
	}
    cleanup:
      g_free (patterns);
    }
  else
    /* Abort the operation.  */
    g_signal_emit_by_name (GPA_OPERATION (op), "completed",
			   gpg_error (GPG_ERR_CANCELED));

  return FALSE;
}

static void
gpa_export_operation_done_cb (GpaContext *context, gpg_error_t err,
			      GpaExportOperation *op)
{
  if (! err)
    GPA_EXPORT_OPERATION_GET_CLASS (op)->complete_export (op);
  g_signal_emit_by_name (GPA_OPERATION (op), "completed", err);
}

static void
gpa_export_operation_done_error_cb (GpaContext *context, gpg_error_t err,
				    GpaExportOperation *op)
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
