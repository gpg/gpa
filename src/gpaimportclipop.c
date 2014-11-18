/* gpaimportclipop.c - The GpaImportClipboardOperation object.
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

#include <gpgme.h>
#include <unistd.h>
#include "gpa.h"
#include "i18n.h"
#include "gtktools.h"
#include "gpaimportclipop.h"

static GObjectClass *parent_class = NULL;

static gboolean
gpa_import_clipboard_operation_get_source (GpaImportOperation *operation);
static void
gpa_import_clipboard_operation_complete_import (GpaImportOperation *operation);

/* GObject boilerplate */

static void
gpa_import_clipboard_operation_init (GpaImportClipboardOperation *op)
{
}

static void
gpa_import_clipboard_operation_class_init (GpaImportClipboardOperationClass *klass)
{
  GpaImportOperationClass *import_class = GPA_IMPORT_OPERATION_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  import_class->get_source = gpa_import_clipboard_operation_get_source;
  import_class->complete_import = gpa_import_clipboard_operation_complete_import;
}

GType
gpa_import_clipboard_operation_get_type (void)
{
  static GType clipboard_operation_type = 0;

  if (!clipboard_operation_type)
    {
      static const GTypeInfo clipboard_operation_info =
      {
        sizeof (GpaImportClipboardOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_import_clipboard_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaImportClipboardOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_import_clipboard_operation_init,
      };

      clipboard_operation_type = g_type_register_static (GPA_IMPORT_OPERATION_TYPE,
							 "GpaImportClipboardOperation",
							 &clipboard_operation_info, 0);
    }

  return clipboard_operation_type;
}

/* Virtual methods */

static gboolean
gpa_import_clipboard_operation_get_source (GpaImportOperation *operation)
{
  gpg_error_t err;
  gchar *text;

  text = gtk_clipboard_wait_for_text (gtk_clipboard_get
                                      (GDK_SELECTION_CLIPBOARD));
  gpgme_data_release (operation->source);
  if (text)
    {
      /* Fill the data from the selection clipboard.
       */
      err = gpgme_data_new_from_mem (&operation->source,
                                     text, strlen (text), TRUE);
    }
  else
    {
      /* If the keyboard was empty, create an empty data object.
       */
      err = gpgme_data_new (&operation->source);
    }

  if (err)
    {
      gpa_gpgme_warning (err);
      return FALSE;
    }
  return TRUE;
}

static void
gpa_import_clipboard_operation_complete_import (GpaImportOperation *operation)
{
  /* Nothing special to do */
}

/* API */

GpaImportClipboardOperation*
gpa_import_clipboard_operation_new (GtkWidget *window)
{
  GpaImportClipboardOperation *op;

  op = g_object_new (GPA_IMPORT_CLIPBOARD_OPERATION_TYPE,
		     "window", window, NULL);

  return op;
}
