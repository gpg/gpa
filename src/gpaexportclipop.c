/* gpaexportclipop.c - The GpaExportClipboardOperation object.
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
#include "gpgmetools.h"
#include "gpaexportclipop.h"

static GObjectClass *parent_class = NULL;

static gboolean
gpa_export_clipboard_operation_get_destination (GpaExportOperation *operation,
						gpgme_data_t *dest,
						gboolean *armor);
static void
gpa_export_clipboard_operation_complete_export (GpaExportOperation *operation);

/* GObject boilerplate */

static void
gpa_export_clipboard_operation_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_export_clipboard_operation_init (GpaExportClipboardOperation *op)
{
}

static GObject*
gpa_export_clipboard_operation_constructor (GType type,
				  guint n_construct_properties,
				  GObjectConstructParam *construct_properties)
{
  GObject *object;
  /* GpaExportClipboardOperation *op; */

  /* Invoke parent's constructor */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  /* op = GPA_EXPORT_CLIPBOARD_OPERATION (object); */

  return object;
}

static void
gpa_export_clipboard_operation_class_init (GpaExportClipboardOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GpaExportOperationClass *export_class = GPA_EXPORT_OPERATION_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_export_clipboard_operation_constructor;
  object_class->finalize = gpa_export_clipboard_operation_finalize;
  export_class->get_destination = gpa_export_clipboard_operation_get_destination;
  export_class->complete_export = gpa_export_clipboard_operation_complete_export;
}

GType
gpa_export_clipboard_operation_get_type (void)
{
  static GType file_operation_type = 0;

  if (!file_operation_type)
    {
      static const GTypeInfo file_operation_info =
      {
        sizeof (GpaExportClipboardOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_export_clipboard_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaExportClipboardOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_export_clipboard_operation_init,
      };

      file_operation_type = g_type_register_static (GPA_EXPORT_OPERATION_TYPE,
						    "GpaExportClipboardOperation",
						    &file_operation_info, 0);
    }

  return file_operation_type;
}

/* Virtual methods */

static gboolean
gpa_export_clipboard_operation_get_destination (GpaExportOperation *operation,
						gpgme_data_t *dest,
						gboolean *armor)
{
  gpg_error_t err;
  *armor = TRUE;
  err = gpgme_data_new (dest);
  if (err)
    {
      gpa_gpgme_warning (err);
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}

static void
gpa_export_clipboard_operation_complete_export (GpaExportOperation *operation)
{
  GpaExportClipboardOperation *op = GPA_EXPORT_CLIPBOARD_OPERATION (operation);
  gboolean is_secret;
  GList *keys;
  unsigned int nkeys;

  g_object_get (op, "secret", &is_secret, "keys", &keys, NULL);
  nkeys = g_list_length (keys);
  if (!dump_data_to_clipboard (operation->dest, gtk_clipboard_get
                               (GDK_SELECTION_CLIPBOARD)))
    gpa_show_info
      (GPA_OPERATION (op)->window,
       is_secret? _("The private key has been copied to the clipboard.") :
       nkeys==1 ? _("The key has bees copied to the clipboard.") :
       /* */      _("The keys have been copied to the clipboard."));
}

/* API */

GpaExportClipboardOperation*
gpa_export_clipboard_operation_new (GtkWidget *window, GList *keys,
                                    gboolean secret)
{
  GpaExportClipboardOperation *op;

  op = g_object_new (GPA_EXPORT_CLIPBOARD_OPERATION_TYPE,
		     "window", window,
		     "keys", keys,
                     "secret", secret,
		     NULL);

  return op;
}
