/* gpaimportfileop.h - The GpaImportFileOperation object.
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

#ifndef GPA_IMPORT_FILE_OP_H
#define GPA_IMPORT_FILE_OP_H

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaimportop.h"

/* GObject stuff */
#define GPA_IMPORT_FILE_OPERATION_TYPE	  (gpa_import_file_operation_get_type ())
#define GPA_IMPORT_FILE_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_IMPORT_FILE_OPERATION_TYPE, GpaImportFileOperation))
#define GPA_IMPORT_FILE_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_IMPORT_FILE_OPERATION_TYPE, GpaImportFileOperationClass))
#define GPA_IS_IMPORT_FILE_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_IMPORT_FILE_OPERATION_TYPE))
#define GPA_IS_IMPORT_FILE_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_IMPORT_FILE_OPERATION_TYPE))
#define GPA_IMPORT_FILE_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_IMPORT_FILE_OPERATION_TYPE, GpaImportFileOperationClass))

typedef struct _GpaImportFileOperation GpaImportFileOperation;
typedef struct _GpaImportFileOperationClass GpaImportFileOperationClass;

struct _GpaImportFileOperation {
  GpaImportOperation parent;

  char *file;
  int fd;
};

struct _GpaImportFileOperationClass {
  GpaImportOperationClass parent_class;

};

GType gpa_import_file_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Creates a new import from file operation.
 */
GpaImportFileOperation*
gpa_import_file_operation_new (GtkWidget *window);

#endif
