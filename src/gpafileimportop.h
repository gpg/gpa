/* gpafileimportop.h - The GpaFileImportOperation object.
 * Copyright (C) 2003 Miguel Coca.
 * Copyright (C) 2014 g10 Code GmbH.
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

#ifndef GPA_FILE_IMPORT_OP_H
#define GPA_FILE_IMPORT_OP_H

#include <glib.h>
#include <glib-object.h>
#include "gpgmetools.h"
#include "gpafileop.h"


/* GObject stuff */
#define GPA_FILE_IMPORT_OPERATION_TYPE	(gpa_file_import_operation_get_type ())

#define GPA_FILE_IMPORT_OPERATION(obj)                                  \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_FILE_IMPORT_OPERATION_TYPE,   \
                               GpaFileImportOperation))

#define GPA_FILE_IMPORT_OPERATION_CLASS(klass)                          \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_FILE_IMPORT_OPERATION_TYPE,    \
                            GpaFileImportOperationClass))

#define GPA_IS_FILE_IMPORT_OPERATION(obj)                               \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_FILE_IMPORT_OPERATION_TYPE))

#define GPA_IS_FILE_IMPORT_OPERATION_CLASS(klass)                       \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_FILE_IMPORT_OPERATION_TYPE))

#define GPA_FILE_IMPORT_OPERATION_GET_CLASS(obj)                        \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_FILE_IMPORT_OPERATION_TYPE,    \
                              GpaFileImportOperationClass))

typedef struct _GpaFileImportOperation      GpaFileImportOperation;
typedef struct _GpaFileImportOperationClass GpaFileImportOperationClass;

struct _GpaFileImportOperation
{
  GpaFileOperation parent;

  struct gpa_import_result_s counters;
};


struct _GpaFileImportOperationClass
{
  GpaFileOperationClass parent_class;
};


GType gpa_file_import_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Creates a new import operation. */
GpaFileImportOperation *
gpa_file_import_operation_new (GtkWidget *window, GList *files);

#endif
