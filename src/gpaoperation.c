/* gpaop.c - The GpaOperation object.
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

#include "gpaoperation.h"
#include <gtk/gtk.h>
#include "gtktools.h"
#include "gpgmetools.h"
#include "i18n.h"

/* Signals */
enum
{
  COMPLETED,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
};

static GObjectClass *parent_class = NULL;
static guint signals [LAST_SIGNAL] = { 0 };

static void
gpa_operation_get_property (GObject     *object,
			    guint        prop_id,
			    GValue      *value,
			    GParamSpec  *pspec)
{
  GpaOperation *op = GPA_OPERATION (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value, op->window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_operation_set_property (GObject     *object,
			    guint        prop_id,
			    const GValue      *value,
			    GParamSpec  *pspec)
{
  GpaOperation *op = GPA_OPERATION (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      op->window = (GtkWidget*) g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_operation_finalize (GObject *object)
{
  GpaOperation *op = GPA_OPERATION (object);
  
  gpa_context_destroy (op->context);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_operation_init (GpaOperation *op)
{
  op->window = NULL;
  op->context = NULL;
}

static GObject*
gpa_operation_constructor (GType                  type,
			   guint                  n_construct_properties,
			   GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_OPERATION (object);
  /* Initialize */
  op->context = gpa_context_new ();

  return object;
}

static void
gpa_operation_class_init (GpaOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_operation_constructor;
  object_class->finalize = gpa_operation_finalize;
  object_class->set_property = gpa_operation_set_property;
  object_class->get_property = gpa_operation_get_property;

  klass->completed = NULL;

  /* Signals */
  signals[COMPLETED] =
    g_signal_new ("completed",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GpaOperationClass, completed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_WINDOW,
				   g_param_spec_object 
				   ("window", "Parent window",
				    "Parent window", GTK_TYPE_WIDGET,
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_operation_get_type (void)
{
  static GType operation_type = 0;
  
  if (!operation_type)
    {
      static const GTypeInfo operation_info =
      {
        sizeof (GpaOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_operation_init,
      };
      
      operation_type = g_type_register_static (G_TYPE_OBJECT,
					       "GpaOperation",
					       &operation_info, 0);
    }
  
  return operation_type;
}

/* API */

/* Whether the operation is currently busy (i.e. gpg is running).
 */
gboolean
gpa_operation_busy (GpaOperation *op)
{
  g_return_val_if_fail (op != NULL, FALSE);
  g_return_val_if_fail (GPA_IS_OPERATION (op), FALSE);

  return gpa_context_busy (op->context);
}

/* Destroy the operation and related resources.
 */
void
gpa_operation_destroy (GpaOperation *op)
{
  g_return_if_fail (op != NULL);
  g_return_if_fail (GPA_IS_OPERATION (op));

  g_object_run_dispose (G_OBJECT (op));
}
