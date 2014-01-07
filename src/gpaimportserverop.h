/* gpaimportserverop.h - The GpaImportServerOperation object.
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

#ifndef GPA_IMPORT_SERVER_OP_H
#define GPA_IMPORT_SERVER_OP_H
#ifdef ENABLE_KEYSERVER_SUPPORT

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaimportop.h"

/* GObject stuff */
#define GPA_IMPORT_SERVER_OPERATION_TYPE	  (gpa_import_server_operation_get_type ())
#define GPA_IMPORT_SERVER_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_IMPORT_SERVER_OPERATION_TYPE, GpaImportServerOperation))
#define GPA_IMPORT_SERVER_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_IMPORT_SERVER_OPERATION_TYPE, GpaImportServerOperationClass))
#define GPA_IS_IMPORT_SERVER_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_IMPORT_SERVER_OPERATION_TYPE))
#define GPA_IS_IMPORT_SERVER_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_IMPORT_SERVER_OPERATION_TYPE))
#define GPA_IMPORT_SERVER_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_IMPORT_SERVER_OPERATION_TYPE, GpaImportServerOperationClass))

typedef struct _GpaImportServerOperation GpaImportServerOperation;
typedef struct _GpaImportServerOperationClass GpaImportServerOperationClass;

struct _GpaImportServerOperation {
  GpaImportOperation parent;

  char *key_id;
};

struct _GpaImportServerOperationClass {
  GpaImportOperationClass parent_class;

};

GType gpa_import_server_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Creates a new import from server operation.
 */
GpaImportServerOperation*
gpa_import_server_operation_new (GtkWidget *window);

#endif /*ENABLE_KEYSERVER_SUPPORT*/
#endif /*GPA_IMPORT_SERVER_OP_H*/
