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

#include <config.h>

#include "gpaoperation.h"
#include <gtk/gtk.h>
#include "gtktools.h"
#include "gpgmetools.h"
#include "i18n.h"

#ifndef G_PARAM_STATIC_STRINGS
#define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK \
                                | G_PARAM_STATIC_BLURB)
#endif

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
  PROP_SERVER_CTX
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
    case PROP_SERVER_CTX:
      g_value_set_pointer (value, op->server_ctx);
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
    case PROP_SERVER_CTX:
      op->server_ctx = g_value_get_pointer (value);
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
  
  g_object_unref (op->context);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_operation_init (GpaOperation *op)
{
  op->window = NULL;
  op->context = NULL;
  op->server_ctx = NULL;
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
  g_object_class_install_property 
    (object_class, PROP_WINDOW,
     g_param_spec_object ("window", "Parent window",
                          "Parent window",
                          GTK_TYPE_WIDGET,
                          G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property 
    (object_class, PROP_SERVER_CTX,
     g_param_spec_pointer ("server-ctx", "Server Context",
                           "The Assuan context of the connection",
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
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


/* Tell the UI-server that the current operation has finished with
   error code ERR.  Note that the server context will be disabled
   after this operation.  */
void
gpa_operation_server_finish (GpaOperation *op, gpg_error_t err)
{
  g_return_if_fail (op);
  g_return_if_fail (GPA_IS_OPERATION (op));
  if (op->server_ctx)
    {
      assuan_context_t ctx = op->server_ctx;
      op->server_ctx = NULL;
      gpa_run_server_continuation (ctx, err);
    }
}
