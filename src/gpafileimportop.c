/* gpafileimportop.c - Import keys from a file.
   Copyright (C) 2003 Miguel Coca.
   Copyright (C) 2008, 2014 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#else
#include <io.h>
#endif

#include "gpa.h"
#include "gtktools.h"
#include "gpgmetools.h"
#include "filetype.h"
#include "gpafileimportop.h"


/* Internal functions */
static gboolean gpa_file_import_operation_idle_cb (gpointer data);
static void gpa_file_import_operation_done_error_cb (GpaContext *context,
						     gpg_error_t err,
						   GpaFileImportOperation *op);
static void gpa_file_import_operation_done_cb (GpaContext *context,
						gpg_error_t err,
						GpaFileImportOperation *op);

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_file_import_operation_finalize (GObject *object)
{
  /* GpaFileImportOperation *op = GPA_FILE_IMPORT_OPERATION (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_file_import_operation_init (GpaFileImportOperation *op)
{
  memset (&op->counters, 0, sizeof op->counters);
}


static GObject*
gpa_file_import_operation_constructor (GType type,
                                       guint n_construct_properties,
                             GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaFileImportOperation *op;

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  op = GPA_FILE_IMPORT_OPERATION (object);
  /* Initialize */
  /* Start with the first file after going back into the main loop */
  g_idle_add (gpa_file_import_operation_idle_cb, op);
  /* Connect to the "done" signal */
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_import_operation_done_error_cb), op);
  g_signal_connect (G_OBJECT (GPA_OPERATION (op)->context), "done",
		    G_CALLBACK (gpa_file_import_operation_done_cb), op);
  /* Give a title to the progress dialog */
  gtk_window_set_title (GTK_WINDOW (GPA_FILE_OPERATION (op)->progress_dialog),
			_("Importing..."));

  return object;
}


static void
gpa_file_import_operation_class_init (GpaFileImportOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_file_import_operation_constructor;
  object_class->finalize = gpa_file_import_operation_finalize;
}


GType
gpa_file_import_operation_get_type (void)
{
  static GType file_import_operation_type = 0;

  if (!file_import_operation_type)
    {
      static const GTypeInfo file_import_operation_info =
      {
        sizeof (GpaFileImportOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_file_import_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaFileImportOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_file_import_operation_init
      };

      file_import_operation_type = g_type_register_static
	(GPA_FILE_OPERATION_TYPE, "GpaFileImportOperation",
	 &file_import_operation_info, 0);
    }

  return file_import_operation_type;
}


/* API */


GpaFileImportOperation*
gpa_file_import_operation_new (GtkWidget *window, GList *files)
{
  GpaFileImportOperation *op;

  op = g_object_new (GPA_FILE_IMPORT_OPERATION_TYPE,
		     "window", window,
		     "input_files", files,
		     NULL);

  return op;
}


/* Internal */


static gboolean
proc_one_file (GpaFileImportOperation *op, gpa_file_item_t file_item)
{
  gpg_error_t err;
  int fd;
  gpgme_data_t data;

  if (file_item->direct_in)
    {
      /* No copy is made.  */
      err = gpgme_data_new_from_mem (&data, file_item->direct_in,
				     file_item->direct_in_len, 0);
      if (err)
	{
	  gpa_gpgme_warning (err);
	  return FALSE;
	}

      gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                          is_cms_data (file_item->direct_in,
                                       file_item->direct_in_len) ?
                          GPGME_PROTOCOL_CMS : GPGME_PROTOCOL_OpenPGP);
    }
  else
    {
      const char *filename = file_item->filename_in;

      fd = gpa_open_input (filename, &data, GPA_OPERATION (op)->window);
      if (fd == -1)
        return FALSE;

      gpgme_set_protocol (GPA_OPERATION (op)->context->ctx,
                          is_cms_file (filename) ?
                          GPGME_PROTOCOL_CMS : GPGME_PROTOCOL_OpenPGP);
    }


  /* Start importing one file.  */
  err = gpgme_op_import_start (GPA_OPERATION (op)->context->ctx, data);
  if (err)
    {
      gpa_gpgme_warning (err);
      return FALSE;
    }

  /* Show and update the progress dialog */
  gtk_widget_show_all (GPA_FILE_OPERATION (op)->progress_dialog);
  gpa_progress_dialog_set_label (GPA_PROGRESS_DIALOG
				 (GPA_FILE_OPERATION (op)->progress_dialog),
				 file_item->direct_name
				 ? file_item->direct_name
				 : file_item->filename_in);

  return TRUE;
}


static void
gpa_file_import_operation_next (GpaFileImportOperation *op)
{
  if (!GPA_FILE_OPERATION (op)->current
      || !proc_one_file (op, GPA_FILE_OPERATION (op)->current->data))
    {
      /* Finished all files.  */
      gtk_widget_hide (GPA_FILE_OPERATION (op)->progress_dialog);
      if (op->counters.imported > 0)
        {
          if (op->counters.secret_imported)
            g_signal_emit_by_name (GPA_OPERATION (op), "imported_secret_keys");
          else
            g_signal_emit_by_name (GPA_OPERATION (op), "imported_keys");
	}
      gpa_gpgme_show_import_results (GPA_OPERATION (op)->window, &op->counters);
    }
}


static gboolean
gpa_file_import_operation_idle_cb (gpointer data)
{
  GpaFileImportOperation *op = data;

  gpa_file_import_operation_next (op);

  return FALSE;
}


static void
gpa_file_import_operation_done_cb (GpaContext *context,
                                   gpg_error_t err,
                                   GpaFileImportOperation *op)
{
  if (err)
    {
      gpa_gpgme_update_import_results (&op->counters, 1, 1, NULL);
    }
  else
    {
      gpgme_import_result_t res;

      res = gpgme_op_import_result (GPA_OPERATION (op)->context->ctx);
      gpa_gpgme_update_import_results (&op->counters, 1, 0, res);

    }

  if (gpg_err_code (err) != GPG_ERR_CANCELED)
    {
      /* Go to the next file in the list and import it.  */
      GPA_FILE_OPERATION (op)->current = (g_list_next
                                          (GPA_FILE_OPERATION (op)->current));
      gpa_file_import_operation_next (op);
    }
}


static void
gpa_file_import_operation_done_error_cb (GpaContext *context, gpg_error_t err,
					 GpaFileImportOperation *op)
{
  gpa_file_item_t file_item = GPA_FILE_OPERATION (op)->current->data;

  /* FIXME: Add the errors to a list and show a dialog with all import
     errors, similar to the verify status.  */
  switch (gpg_err_code (err))
    {
    case GPG_ERR_NO_ERROR:
    case GPG_ERR_CANCELED:
      /* Ignore these */
      break;

    case GPG_ERR_NO_DATA:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                        file_item->direct_name
                        ? _("\"%s\" contained no OpenPGP data.")
                        : _("The file \"%s\" contained no OpenPGP"
                            "data."),
                        file_item->direct_name
                        ? file_item->direct_name
                        : file_item->filename_in);
      break;

    default:
      gpa_show_warn (GPA_OPERATION (op)->window, GPA_OPERATION (op)->context,
                        _("Error importing \"%s\": %s <%s>"),
                        file_item->direct_name
                        ? file_item->direct_name
                        : file_item->filename_in,
                        gpg_strerror (err), gpg_strsource (err));
      gpa_gpgme_warn (err, NULL, GPA_OPERATION (op)->context);
      break;
    }
}
