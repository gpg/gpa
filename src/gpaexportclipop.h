/* gpaexportclipop.h - The GpaExportClipboardOperation object.
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

#ifndef GPA_EXPORT_CLIP_OP_H
#define GPA_EXPORT_CLIP_OP_H

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaexportop.h"

/* GObject stuff */
#define GPA_EXPORT_CLIPBOARD_OPERATION_TYPE	  (gpa_export_clipboard_operation_get_type ())
#define GPA_EXPORT_CLIPBOARD_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_EXPORT_CLIPBOARD_OPERATION_TYPE, GpaExportClipboardOperation))
#define GPA_EXPORT_CLIPBOARD_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_EXPORT_CLIPBOARD_OPERATION_TYPE, GpaExportClipboardOperationClass))
#define GPA_IS_EXPORT_CLIPBOARD_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_EXPORT_CLIPBOARD_OPERATION_TYPE))
#define GPA_IS_EXPORT_CLIPBOARD_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_EXPORT_CLIPBOARD_OPERATION_TYPE))
#define GPA_EXPORT_CLIPBOARD_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_EXPORT_CLIPBOARD_OPERATION_TYPE, GpaExportClipboardOperationClass))

typedef struct _GpaExportClipboardOperation GpaExportClipboardOperation;
typedef struct _GpaExportClipboardOperationClass GpaExportClipboardOperationClass;

struct _GpaExportClipboardOperation {
  GpaExportOperation parent;

};

struct _GpaExportClipboardOperationClass {
  GpaExportOperationClass parent_class;

};

GType gpa_export_clipboard_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Creates a new export to file operation.
 */
GpaExportClipboardOperation*
  gpa_export_clipboard_operation_new (GtkWidget *window, GList *keys,
                                      gboolean secret);

#endif
