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
#include "gpa-marshal.h"

#ifndef G_PARAM_STATIC_STRINGS
#define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK \
                                | G_PARAM_STATIC_BLURB)
#endif

/* Signals */
enum
{
  COMPLETED,
  STATUS,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_CLIENT_TITLE
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
    case PROP_CLIENT_TITLE:
      g_value_set_string (value, op->client_title);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_operation_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{
  GpaOperation *op = GPA_OPERATION (object);
  
  switch (prop_id)
    {
    case PROP_WINDOW:
      op->window = (GtkWidget*) g_value_get_object (value);
      break;
    case PROP_CLIENT_TITLE:
      g_free (op->client_title);
      op->client_title = g_value_dup_string (value);
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
  g_free (op->client_title);
  op->client_title = NULL;
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_operation_init (GpaOperation *op)
{
  op->window = NULL;
  op->context = NULL;
  op->client_title = NULL;
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
  klass->status = NULL;

  /* Signals.  */
  signals[COMPLETED] =
    g_signal_new ("completed",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		  G_STRUCT_OFFSET (GpaOperationClass, completed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__INT,
		  G_TYPE_NONE, 1, G_TYPE_INT);

  signals[STATUS] =
    g_signal_new ("status",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		  G_STRUCT_OFFSET (GpaOperationClass, status),
		  NULL, NULL,
		  gpa_marshal_INT__STRING_STRING,
		  G_TYPE_INT, 2, G_TYPE_STRING, G_TYPE_STRING);


  /* Properties.  */
  g_object_class_install_property 
    (object_class, PROP_WINDOW,
     g_param_spec_object ("window", "Parent window",
                          "Parent window",
                          GTK_TYPE_WIDGET,
                          G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property 
    (object_class, PROP_CLIENT_TITLE,
     g_param_spec_string 
     ("client-title", "Client Title",
      "The client suggested title for the operation or NULL.", 
      NULL,
      G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
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

/* Whether the operation is currently busy (i.e. gpg is running).  */
gboolean
gpa_operation_busy (GpaOperation *op)
{
  g_return_val_if_fail (op != NULL, FALSE);
  g_return_val_if_fail (GPA_IS_OPERATION (op), FALSE);

  return gpa_context_busy (op->context);
}


/* Emit a status line names STATUSNAME plus space delimited
   arguments.  */
gpg_error_t
gpa_operation_write_status (GpaOperation *op, const char *statusname, ...)
{
  gpg_error_t err = 0;
  va_list arg_ptr;
  char buf[950], *p;
  const char *text;
  size_t n;

  g_return_val_if_fail (op, gpg_error (GPG_ERR_BUG));
  g_return_val_if_fail (GPA_IS_OPERATION (op), gpg_error (GPG_ERR_BUG));

  va_start (arg_ptr, statusname);

  p = buf; 
  n = 0;
  while ((text = va_arg (arg_ptr, const char *)))
    {
      if (n)
	{
	  *p++ = ' ';
	  n++;
	}
      for ( ; *text && n < DIM (buf)-2; n++)
	*p++ = *text++;
    }
  *p = 0;

  /* FIXME: Return value might require an allocator to not only get
     the last one.  */
  g_signal_emit (GPA_OPERATION (op), signals[STATUS], 0,
		 statusname, buf, &err);
  
  va_end (arg_ptr);

  return err;
}
