/* gpaexportop.h - The GpaExportOperation object.
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

#ifndef GPA_EXPORT_OP_H
#define GPA_EXPORT_OP_H

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaoperation.h"
#include "gpaprogressdlg.h"

/* GObject stuff */
#define GPA_EXPORT_OPERATION_TYPE	  (gpa_export_operation_get_type ())
#define GPA_EXPORT_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_EXPORT_OPERATION_TYPE, GpaExportOperation))
#define GPA_EXPORT_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_EXPORT_OPERATION_TYPE, GpaExportOperationClass))
#define GPA_IS_EXPORT_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_EXPORT_OPERATION_TYPE))
#define GPA_IS_EXPORT_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_EXPORT_OPERATION_TYPE))
#define GPA_EXPORT_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_EXPORT_OPERATION_TYPE, GpaExportOperationClass))

typedef struct _GpaExportOperation GpaExportOperation;
typedef struct _GpaExportOperationClass GpaExportOperationClass;

struct _GpaExportOperation {
  GpaOperation parent;

  GList *keys;
  gpgme_data_t dest;

  /*:: private ::*/
  int secret;
};

struct _GpaExportOperationClass {
  GpaOperationClass parent_class;

  /* Get the gpgme_data_t to which the keys should be exported.
   * Returns FALSE if the operation should be aborted.
   */
  gboolean (*get_destination) (GpaExportOperation *op, gpgme_data_t *dest,
			       gboolean *armor);

  /* Do whatever it takes to complete the export once the gpgme_data_t is
   * filled. Basically, this sends the data to the server, the clipboard,
   * etc.
   */
  void (*complete_export) (GpaExportOperation *op);
};

GType gpa_export_operation_get_type (void) G_GNUC_CONST;

#endif
