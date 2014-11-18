/* gpaimportfileop.c - The GpaImportFileOperation object.
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

#include <gpgme.h>
#include <unistd.h>
#include "gpa.h"
#include "i18n.h"
#include "gtktools.h"
#include "gpaimportfileop.h"

static GObjectClass *parent_class = NULL;

static gboolean
gpa_import_file_operation_get_source (GpaImportOperation *operation);
static void
gpa_import_file_operation_complete_import (GpaImportOperation *operation);

/* GObject boilerplate */

static void
gpa_import_file_operation_finalize (GObject *object)
{
  GpaImportFileOperation *op = GPA_IMPORT_FILE_OPERATION (object);

  /* Cleanup */
  if (op->fd != -1)
    close (op->fd);
  if (op->file)
    g_free (op->file);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_import_file_operation_init (GpaImportFileOperation *op)
{
  op->file = NULL;
  op->fd = -1;
}

static GObject*
gpa_import_file_operation_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  /* GpaImportFileOperation *op; */

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  /* op = GPA_IMPORT_FILE_OPERATION (object); */

  return object;
}

static void
gpa_import_file_operation_class_init (GpaImportFileOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GpaImportOperationClass *import_class = GPA_IMPORT_OPERATION_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_import_file_operation_constructor;
  object_class->finalize = gpa_import_file_operation_finalize;
  import_class->get_source = gpa_import_file_operation_get_source;
  import_class->complete_import = gpa_import_file_operation_complete_import;
}

GType
gpa_import_file_operation_get_type (void)
{
  static GType file_operation_type = 0;

  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaImportFileOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_import_file_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaImportFileOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_import_file_operation_init,
      };

      file_operation_type = g_type_register_static (GPA_IMPORT_OPERATION_TYPE,
						    "GpaImportFileOperation",
						    &file_operation_info, 0);
    }

  return file_operation_type;
}

/* Virtual methods */

static gboolean
gpa_import_file_operation_get_source (GpaImportOperation *operation)
{
  GpaImportFileOperation *op = GPA_IMPORT_FILE_OPERATION (operation);
  GtkWidget *dialog;
  GtkResponseType response;

  dialog = gtk_file_chooser_dialog_new
    (_("Import keys from file"),
     GTK_WINDOW (GPA_OPERATION (op)->window),
     GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
     GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

  gpgme_data_release (operation->source);
  operation->source = NULL;

  /* Run the dialog until there is a valid response.  */
  do
    {
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      /* Save the selected file, free'ing the previous value if required.  */
      if (op->file)
	g_free (op->file);
      op->file = g_strdup (gtk_file_chooser_get_filename
			   (GTK_FILE_CHOOSER (dialog)));
    }
  while (response == GTK_RESPONSE_OK
	 && (op->fd = gpa_open_input (op->file,
                                      &operation->source,
                                      GPA_OPERATION (op)->window)) == -1);
  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_OK);
}

static void
gpa_import_file_operation_complete_import (GpaImportOperation *operation)
{
  /* Nothing special to do */
}

/* API */

GpaImportFileOperation*
gpa_import_file_operation_new (GtkWidget *window)
{
  GpaImportFileOperation *op;

  op = g_object_new (GPA_IMPORT_FILE_OPERATION_TYPE,
		     "window", window, NULL);

  return op;
}
