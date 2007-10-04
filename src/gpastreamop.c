/* gpastreamop.c - The GpaStreamOperation object.
 *	Copyright (C) 2007 g10 Code GmbH
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

#include "i18n.h"
#include "gtktools.h"
#include "gpastreamop.h"

/* Signals */
enum
{
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_INPUT_STREAM,
  PROP_OUTPUT_STREAM,
  PROP_MESSAGE_STREAM
};

static GObjectClass *parent_class;
/* static guint signals[LAST_SIGNAL]; */


static void
gpa_stream_operation_get_property (GObject     *object,
                                   guint        prop_id,
                                   GValue      *value,
                                   GParamSpec  *pspec)
{
  GpaStreamOperation *op = GPA_STREAM_OPERATION (object);

  switch (prop_id)
    {
    case PROP_INPUT_STREAM:
      g_value_set_pointer (value, op->input_stream);
      break;
    case PROP_OUTPUT_STREAM:
      g_value_set_pointer (value, op->output_stream);
      break;
    case PROP_MESSAGE_STREAM:
      g_value_set_pointer (value, op->message_stream);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_stream_operation_set_property (GObject      *object,
                                   guint        prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GpaStreamOperation *op = GPA_STREAM_OPERATION (object);

  switch (prop_id)
    {
    case PROP_INPUT_STREAM:
      op->input_stream = (gpgme_data_t)g_value_get_pointer (value);
      break;
    case PROP_OUTPUT_STREAM:
      op->output_stream = (gpgme_data_t)g_value_get_pointer (value);
      break;
    case PROP_MESSAGE_STREAM:
      op->message_stream = (gpgme_data_t)g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_stream_operation_finalize (GObject *object)
{
  GpaStreamOperation *op = GPA_STREAM_OPERATION (object);

  gpgme_data_release (op->input_stream);
  gpgme_data_release (op->output_stream);
  gpgme_data_release (op->message_stream);
  gtk_widget_destroy (op->progress_dialog);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_stream_operation_init (GpaStreamOperation *op)
{
  op->input_stream = NULL;
  op->output_stream = NULL;
  op->message_stream = NULL;

  op->progress_dialog = NULL;
}


static GObject*
gpa_stream_operation_constructor (GType type,
                                  guint n_construct_properties,
                                  GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaStreamOperation *op;

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_STREAM_OPERATION (object);

  op->progress_dialog = gpa_progress_dialog_new (GPA_OPERATION(op)->window,
						 GPA_OPERATION(op)->context);

  return object;
}


static void
gpa_stream_operation_class_init (GpaStreamOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gpa_stream_operation_constructor;
  object_class->finalize     = gpa_stream_operation_finalize;
  object_class->set_property = gpa_stream_operation_set_property;
  object_class->get_property = gpa_stream_operation_get_property;

  /* Signals */
  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_INPUT_STREAM,
				   g_param_spec_pointer 
				   ("input_stream", "Input Stream",
				    "Data read by gpg/gpgsm",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_OUTPUT_STREAM,
				   g_param_spec_pointer 
				   ("output_stream", "Output Stream",
				    "Data written by gpg/gpgsm",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_MESSAGE_STREAM,
				   g_param_spec_pointer 
				   ("message_stream", "Message Stream",
				    "Message data read by gpg/gpgsm",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}


GType
gpa_stream_operation_get_type (void)
{
  static GType stream_operation_type = 0;
  
  if (!stream_operation_type)
    {
      static const GTypeInfo stream_operation_info =
      {
        sizeof (GpaStreamOperationClass),
        (GBaseInitFunc)NULL,
        (GBaseFinalizeFunc)NULL,
        (GClassInitFunc)gpa_stream_operation_class_init,
        NULL, /* class_finalize */
        NULL, /* class_data */
        sizeof (GpaStreamOperation),
        0,    /* n_preallocs */
        (GInstanceInitFunc)gpa_stream_operation_init,
      };
      
      stream_operation_type = g_type_register_static 
        (GPA_OPERATION_TYPE, "GpaStreamOperation",
         &stream_operation_info, 0);
    }
  
  return stream_operation_type;
}



