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

/* Internal functions */
static void
gpa_operation_done_cb (GpaContext *context, GpgmeError err, GpaOperation *op);

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
  PROP_OPTIONS
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
    case PROP_OPTIONS:
      g_value_set_object (value, op->options);
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
    case PROP_OPTIONS:
      op->options = (GpaOptions*) g_value_get_object (value);
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
gpa_operation_class_init (GpaOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

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
  g_object_class_install_property (object_class,
				   PROP_OPTIONS,
				   g_param_spec_object 
				   ("options", "options",
				    "options", GPA_OPTIONS_TYPE,
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}

static void
gpa_operation_init (GpaOperation *op)
{
  op->window = NULL;
  op->options = NULL;
  op->context = gpa_context_new ();
  g_signal_connect (G_OBJECT (op->context), "done",
		    G_CALLBACK (gpa_operation_done_cb), op);
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

/* Internal functions */

static void
gpa_operation_done_cb (GpaContext *context, GpgmeError err, GpaOperation *op)
{
  /* Capture fatal errors and quit the application */
  switch (err)
    {
    case GPGME_No_Error:
    case GPGME_Canceled:
    case GPGME_No_Data:
    case GPGME_Decryption_Failed:
      /* Ignore: to be handled by subclasses */
      break;
    case GPGME_No_Passphrase:
      gpa_window_error (_("Wrong passphrase!"), op->window);
      break;
    case GPGME_EOF:
    case GPGME_General_Error:
    case GPGME_Out_Of_Core:
    case GPGME_Invalid_Value:
    case GPGME_Busy:
    case GPGME_No_Request:
    case GPGME_Exec_Error:
    case GPGME_Too_Many_Procs:
    case GPGME_Pipe_Error:
    case GPGME_Invalid_Recipients:
    case GPGME_Conflict:
    case GPGME_Not_Implemented:
    case GPGME_Read_Error:
    case GPGME_Write_Error:
    case GPGME_Invalid_Type:
    case GPGME_Invalid_Mode:
    case GPGME_File_Error:
    case GPGME_Invalid_Key:
    case GPGME_Invalid_Engine:
    case GPGME_No_Recipients:
      gpa_gpgme_warning (err);
      break;
    }
}
