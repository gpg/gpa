/* gpafileop.h - The GpaFileOperation object.
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

#ifndef GPA_FILE_OP_H
#define GPA_FILE_OP_H

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaoperation.h"
#include "gpaprogressdlg.h"

/* GObject stuff */
#define GPA_FILE_OPERATION_TYPE	  (gpa_file_operation_get_type ())
#define GPA_FILE_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_FILE_OPERATION_TYPE, GpaFileOperation))
#define GPA_FILE_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_FILE_OPERATION_TYPE, GpaFileOperationClass))
#define GPA_IS_FILE_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_FILE_OPERATION_TYPE))
#define GPA_IS_FILE_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_FILE_OPERATION_TYPE))
#define GPA_FILE_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_FILE_OPERATION_TYPE, GpaFileOperationClass))

typedef struct _GpaFileOperation GpaFileOperation;
typedef struct _GpaFileOperationClass GpaFileOperationClass;

struct gpa_file_item_s
{
  /* If not NULL, the text to operate on.  */
  gchar *direct_in;
  gsize direct_in_len;
  gchar *direct_out;
  /* Length of DIRECT_OUT (minus trailing zero).  */
  gsize direct_out_len;
  /* A displayable string identifying the text.  */
  gchar *direct_name;

  /* The filename to operate on (if DIRECT_IN is NULL).  */
  gchar *filename_in;
  gchar *filename_out;
};
typedef struct gpa_file_item_s *gpa_file_item_t; 


struct _GpaFileOperation {
  GpaOperation parent;

  GList *input_files;
  GList *current;
  GtkWidget *progress_dialog;
};

struct _GpaFileOperationClass {
  GpaOperationClass parent_class;

  /* Signal handlers */
  /* Called every time a new file is created by the operation,
   * *after* the operations is done with it. */
  void (*created_file) (GpaContext *context, const gchar *file);
};

GType gpa_file_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Returns the list of files on which the operation has been called. 
 */
GList*
gpa_file_operation_input_files (GpaFileOperation *op);

/* Returns the file the operation is currently being applied to.
 */
const gchar *
gpa_file_operation_current_file (GpaFileOperation *op);

#endif
