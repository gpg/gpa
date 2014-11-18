/* gpaimportop.h - The GpaImportOperation object.
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

#ifndef GPA_IMPORT_OP_H
#define GPA_IMPORT_OP_H

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaoperation.h"
#include "gpaprogressdlg.h"

/* GObject stuff */
#define GPA_IMPORT_OPERATION_TYPE	  (gpa_import_operation_get_type ())
#define GPA_IMPORT_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_IMPORT_OPERATION_TYPE, GpaImportOperation))
#define GPA_IMPORT_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_IMPORT_OPERATION_TYPE, GpaImportOperationClass))
#define GPA_IS_IMPORT_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_IMPORT_OPERATION_TYPE))
#define GPA_IS_IMPORT_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_IMPORT_OPERATION_TYPE))
#define GPA_IMPORT_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_IMPORT_OPERATION_TYPE, GpaImportOperationClass))

typedef struct _GpaImportOperation GpaImportOperation;
typedef struct _GpaImportOperationClass GpaImportOperationClass;

struct _GpaImportOperation {
  GpaOperation parent;

  gpgme_data_t source;    /* Either a data object with the full key  */
  gpgme_key_t *source2;   /* or an array of key descriptions.  */
};

struct _GpaImportOperationClass {
  GpaOperationClass parent_class;

  /* Get the data from which the keys should be imported.  Returns
   * FALSE if the operation should be aborted.
   */
  gboolean (*get_source) (GpaImportOperation *op);

  /* Do whatever it takes to complete the import once the gpgme_data_t is
   * filled. Basically, this sends the data to the server, the clipboard,
   * etc.
   */
  void (*complete_import) (GpaImportOperation *op);

  /* "Some keys were imported" signal.
   */
  void (*imported_keys) (GpaImportOperation *op);
};

GType gpa_import_operation_get_type (void) G_GNUC_CONST;

#endif
