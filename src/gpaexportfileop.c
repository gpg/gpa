/* gpaexportfileop.c - The GpaExportFileOperation object.
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
#include "gpaexportfileop.h"

static GObjectClass *parent_class = NULL;

static gboolean
gpa_export_file_operation_get_destination (GpaExportOperation *operation,
					   gpgme_data_t *dest,
					   gboolean *armor);
static void
gpa_export_file_operation_complete_export (GpaExportOperation *operation);

/* GObject boilerplate */

static void
gpa_export_file_operation_finalize (GObject *object)
{
  GpaExportFileOperation *op = GPA_EXPORT_FILE_OPERATION (object);

  /* Cleanup */
  if (op->fd != -1)
    {
      close (op->fd);
    }
  if (op->file)
    {
      g_free (op->file);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_export_file_operation_init (GpaExportFileOperation *op)
{
  op->file = NULL;
  op->fd = -1;
}

static GObject*
gpa_export_file_operation_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  /* GpaExportFileOperation *op; */

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  /* op = GPA_EXPORT_FILE_OPERATION (object); */

  return object;
}

static void
gpa_export_file_operation_class_init (GpaExportFileOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GpaExportOperationClass *export_class = GPA_EXPORT_OPERATION_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_export_file_operation_constructor;
  object_class->finalize = gpa_export_file_operation_finalize;
  export_class->get_destination = gpa_export_file_operation_get_destination;
  export_class->complete_export = gpa_export_file_operation_complete_export;
}

GType
gpa_export_file_operation_get_type (void)
{
  static GType file_operation_type = 0;

  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaExportFileOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_export_file_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaExportFileOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_export_file_operation_init,
      };

      file_operation_type = g_type_register_static (GPA_EXPORT_OPERATION_TYPE,
						    "GpaExportFileOperation",
						    &file_operation_info, 0);
    }

  return file_operation_type;
}

/* Virtual methods */

static gboolean
gpa_export_file_operation_get_destination (GpaExportOperation *operation,
					   gpgme_data_t *dest,
					   gboolean *armor)
{
  GpaExportFileOperation *op = GPA_EXPORT_FILE_OPERATION (operation);
  GtkWidget *dialog;
  GtkResponseType response;
  GtkWidget *armor_check = NULL;

  dialog = gtk_file_chooser_dialog_new
    (_("Export public keys to file"), GTK_WINDOW (GPA_OPERATION (op)->window),
     GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
     GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
						  TRUE);

  /* Customize the dialog, adding the "armor" option.  */
  if (! gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    {
      armor_check = gtk_check_button_new_with_mnemonic (_("_armor"));
      gtk_widget_show_all (armor_check);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (armor_check), *armor);
      gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog),
					 armor_check);
    }

  /* Run the dialog until there is a valid response.  */
  do
    {
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      /* Save the selected file, free'ing the previous value if required.  */
      if (op->file)
	g_free (op->file);
      op->file = g_strdup (gtk_file_chooser_get_filename
			   (GTK_FILE_CHOOSER (dialog)));
      *armor = (armor_check == NULL) ? TRUE
	: gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (armor_check));
    }
  while (response == GTK_RESPONSE_OK
	 && (op->fd = gpa_open_output_direct
	     (op->file, dest, GPA_OPERATION (op)->window)) == -1);
  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_OK);
}


static void
gpa_export_file_operation_complete_export (GpaExportOperation *operation)
{
  GpaExportFileOperation *op = GPA_EXPORT_FILE_OPERATION (operation);
  gchar *message = g_strdup_printf (_("The keys have been exported to %s."),
				    op->file);
  gpa_window_message (message, GPA_OPERATION (op)->window);
  g_free (message);
}

/* API */

GpaExportFileOperation*
gpa_export_file_operation_new (GtkWidget *window, GList *keys)
{
  GpaExportFileOperation *op;

  op = g_object_new (GPA_EXPORT_FILE_OPERATION_TYPE,
		     "window", window,
		     "keys", keys,
		     NULL);

  return op;
}
