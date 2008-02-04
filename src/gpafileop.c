/* gpafileop.c - The GpaFileOperation object.
 * Copyright (C) 2003, Miguel Coca.
 * Copyright (C) 2008 g10 Code GmbH.
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
#include "gpafileop.h"

/* Signals */
enum
{
  CREATED_FILE,
  LAST_SIGNAL
};

/* Properties */
enum
{
  PROP_0,
  PROP_INPUT_FILES
};

static GObjectClass *parent_class = NULL;
static guint signals [LAST_SIGNAL] = { 0 };

static void
gpa_file_operation_get_property (GObject     *object,
				 guint        prop_id,
				 GValue      *value,
				 GParamSpec  *pspec)
{
  GpaFileOperation *op = GPA_FILE_OPERATION (object);

  switch (prop_id)
    {
    case PROP_INPUT_FILES:
      g_value_set_pointer (value, op->input_files);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_file_operation_set_property (GObject     *object,
				 guint        prop_id,
				 const GValue      *value,
				 GParamSpec  *pspec)
{
  GpaFileOperation *op = GPA_FILE_OPERATION (object);

  switch (prop_id)
    {
    case PROP_INPUT_FILES:
      op->input_files = (GList*) g_value_get_pointer (value);
      op->current = op->input_files;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
free_file_item (gpa_file_item_t item)
{
  if (item->filename_in)
    g_free (item->filename_in);
  if (item->filename_out)
    g_free (item->filename_out);
  if (item->direct_name)
    g_free (item->direct_name);
  if (item->direct_in)
    g_free (item->direct_in);
  if (item->direct_out)
    g_free (item->direct_out);
}


static void
gpa_file_operation_finalize (GObject *object)
{
  GpaFileOperation *op = GPA_FILE_OPERATION (object);

  g_list_foreach (op->input_files, (GFunc) free_file_item, NULL);
  g_list_free (op->input_files);
  gtk_widget_destroy (op->progress_dialog);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_file_operation_init (GpaFileOperation *op)
{
  op->input_files = NULL;
  op->current = NULL;
  op->progress_dialog = NULL;
}

static GObject*
gpa_file_operation_constructor (GType type,
				guint n_construct_properties,
				GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_FILE_OPERATION (object);
  /* Initialize */
  op->progress_dialog = gpa_progress_dialog_new (GPA_OPERATION(op)->window,
						 GPA_OPERATION(op)->context);

  return object;
}

static void
gpa_file_operation_class_init (GpaFileOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_file_operation_constructor;
  object_class->finalize = gpa_file_operation_finalize;
  object_class->set_property = gpa_file_operation_set_property;
  object_class->get_property = gpa_file_operation_get_property;

  klass->created_file = NULL;

  /* Signals */
  signals[CREATED_FILE] =
    g_signal_new ("created_file",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GpaFileOperationClass, created_file),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  /* Properties */
  g_object_class_install_property (object_class,
				   PROP_INPUT_FILES,
				   g_param_spec_pointer 
				   ("input_files", "Files",
				    "Files",
				    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}

GType
gpa_file_operation_get_type (void)
{
  static GType file_operation_type = 0;
  
  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaFileOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_file_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaFileOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_file_operation_init,
      };
      
      file_operation_type = g_type_register_static (GPA_OPERATION_TYPE,
						    "GpaFileOperation",
						    &file_operation_info, 0);
    }
  
  return file_operation_type;
}


/* API */

/* Returns the list of files on which the operation has been called.  */
GList*
gpa_file_operation_input_files (GpaFileOperation *op)
{
  g_return_val_if_fail (op != NULL, NULL);
  g_return_val_if_fail (GPA_IS_FILE_OPERATION (op), NULL);

  return op->input_files;
}


/* Returns the file the operation is currently being applied to.  */
const gchar *
gpa_file_operation_current_file (GpaFileOperation *op)
{
  g_return_val_if_fail (op != NULL, NULL);
  g_return_val_if_fail (GPA_IS_FILE_OPERATION (op), NULL);

  if (op->current)
    {
      gpa_file_item_t file_item;

      file_item = op->current->data; 
      return file_item->filename_in;
    }
  else
    return NULL;
}
