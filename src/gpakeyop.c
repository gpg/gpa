/* gpakeyop.c - The GpaKeyOperation object.
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

#include "i18n.h"
#include "gtktools.h"
#include "gpakeyop.h"

/* Signals */
enum
{
  CHANGED_WOT,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_KEYS
};

static GObjectClass *parent_class = NULL;
static guint signals [LAST_SIGNAL] = { 0 };

static void
gpa_key_operation_get_property (GObject     *object,
				 guint        prop_id,
				 GValue      *value,
				 GParamSpec  *pspec)
{
  GpaKeyOperation *op = GPA_KEY_OPERATION (object);

  switch (prop_id)
    {
    case PROP_KEYS:
      g_value_set_pointer (value, op->keys);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_key_operation_set_property (GObject     *object,
				 guint        prop_id,
				 const GValue      *value,
				 GParamSpec  *pspec)
{
  GpaKeyOperation *op = GPA_KEY_OPERATION (object);

  switch (prop_id)
    {
    case PROP_KEYS:
      op->keys = (GList*) g_value_get_pointer (value);
      op->current = op->keys;
      /* Make sure we keep a reference for our keys */
      g_list_foreach (op->keys, (GFunc) gpgme_key_ref, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_key_operation_finalize (GObject *object)
{
  GpaKeyOperation *op = GPA_KEY_OPERATION (object);

  /* Free each key, and then the list of keys */
  g_list_foreach (op->keys, (GFunc) gpgme_key_unref, NULL);
  g_list_free (op->keys);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_key_operation_init (GpaKeyOperation *op)
{
  op->keys = NULL;
  op->current = NULL;
}

static GObject*
gpa_key_operation_constructor (GType type,
				guint n_construct_properties,
				GObjectConstructParam *construct_properties)
{
  GObject *object;
  /* GpaKeyOperation *op; */

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  /* op = GPA_KEY_OPERATION (object); */

  return object;
}

static void
gpa_key_operation_class_init (GpaKeyOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_key_operation_constructor;
  object_class->finalize = gpa_key_operation_finalize;
  object_class->set_property = gpa_key_operation_set_property;
  object_class->get_property = gpa_key_operation_get_property;

  /* Signals */
  signals[CHANGED_WOT] =
    g_signal_new ("changed_wot",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GpaKeyOperationClass, changed_wot),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_KEYS,
				   g_param_spec_pointer
				   ("keys", "Keys",
				    "Keys",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_key_operation_get_type (void)
{
  static GType file_operation_type = 0;

  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaKeyOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_key_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaKeyOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_key_operation_init,
      };

      file_operation_type = g_type_register_static (GPA_OPERATION_TYPE,
						    "GpaKeyOperation",
						    &file_operation_info, 0);
    }

  return file_operation_type;
}

/* API */

/* Returns the list of keys on which the operation has been called.
 */
GList*
gpa_key_operation_keys (GpaKeyOperation *op)
{
  g_return_val_if_fail (op != NULL, NULL);
  g_return_val_if_fail (GPA_IS_KEY_OPERATION (op), NULL);

  return op->keys;
}

/* Returns the key the operation is currently being applied to.
 */
gpgme_key_t
gpa_key_operation_current_key (GpaKeyOperation *op)
{
  g_return_val_if_fail (op != NULL, NULL);
  g_return_val_if_fail (GPA_IS_KEY_OPERATION (op), NULL);

  if (op->current)
    {
      return op->current->data;
    }
  else
    {
      return NULL;
    }
}
