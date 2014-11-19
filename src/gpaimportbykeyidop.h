/* gpaimportbykeyidop.h - The GpaImportByKeyidOperation object.
 * Copyright (C) 2014 g10 Code GmbH
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GPA_IMPORT_BYKEYID_OP_H
#define GPA_IMPORT_BYKEYID_OP_H
#ifdef ENABLE_KEYSERVER_SUPPORT

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaimportop.h"

/* GObject stuff */
#define GPA_IMPORT_BYKEYID_OPERATION_TYPE \
  (gpa_import_bykeyid_operation_get_type ())

#define GPA_IMPORT_BYKEYID_OPERATION(obj)                               \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_IMPORT_BYKEYID_OPERATION_TYPE,\
                               GpaImportByKeyidOperation))

#define GPA_IMPORT_BYKEYID_OPERATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_IMPORT_BYKEYID_OPERATION_TYPE, \
                            GpaImportByKeyidOperationClass))

#define GPA_IS_IMPORT_BYKEYID_OPERATION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_IMPORT_BYKEYID_OPERATION_TYPE))

#define GPA_IS_IMPORT_BYKEYID_OPERATION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_IMPORT_BYKEYID_OPERATION_TYPE))

#define GPA_IMPORT_BYKEYID_OPERATION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_IMPORT_BYKEYID_OPERATION_TYPE, \
                              GpaImportByKeyidOperationClass))

typedef struct _GpaImportByKeyidOperation      GpaImportByKeyidOperation;
typedef struct _GpaImportByKeyidOperationClass GpaImportByKeyidOperationClass;

struct _GpaImportByKeyidOperation
{
  GpaImportOperation parent;

  gpgme_key_t key;
};


struct _GpaImportByKeyidOperationClass
{
  GpaImportOperationClass parent_class;
};


GType gpa_import_bykeyid_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Creates a new import by keyid operation. */
GpaImportByKeyidOperation *
gpa_import_bykeyid_operation_new (GtkWidget *window, gpgme_key_t key);

#endif /*ENABLE_KEYSERVER_SUPPORT*/
#endif /*GPA_IMPORT_BYKEYID_OP_H*/
