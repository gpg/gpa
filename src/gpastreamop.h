/* gpastreamop.h - The GpaStreamOperation object.
 *	Copyright (C) 2007 g10 Code GmbH
 *
 * This file is part of GPA.
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

#ifndef GPA_STREAM_OP_H
#define GPA_STREAM_OP_H

#include <glib.h>
#include <glib-object.h>
#include "gpa.h"
#include "gpaoperation.h"
#include "gpaprogressdlg.h"

/* GObject stuff */
#define GPA_STREAM_OPERATION_TYPE \
            (gpa_stream_operation_get_type ())
#define GPA_STREAM_OPERATION(obj)       \
            (G_TYPE_CHECK_INSTANCE_CAST \
             ((obj), GPA_STREAM_OPERATION_TYPE, GpaStreamOperation))
#define GPA_STREAM_OPERATION_CLASS(klass)         \
            (G_TYPE_CHECK_CLASS_CAST              \
             ((klass), GPA_STREAM_OPERATION_TYPE, \
              GpaStreamOperationClass))
#define GPA_IS_STREAM_OPERATION(obj)    \
	    (G_TYPE_CHECK_INSTANCE_TYPE \
             ((obj), GPA_STREAM_OPERATION_TYPE))
#define GPA_IS_STREAM_OPERATION_CLASS(klass) \
            (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_STREAM_OPERATION_TYPE))
#define GPA_STREAM_OPERATION_GET_CLASS(obj) \
            (G_TYPE_INSTANCE_GET_CLASS      \
             ((obj), GPA_STREAM_OPERATION_TYPE, GpaStreamOperationClass))

typedef struct _GpaStreamOperation GpaStreamOperation;
typedef struct _GpaStreamOperationClass GpaStreamOperationClass;

struct _GpaStreamOperation {
  GpaOperation parent;

  gpgme_data_t input_stream;
  gpgme_data_t output_stream;
  gpgme_data_t message_stream;

  GtkWidget *progress_dialog;
};

struct _GpaStreamOperationClass {
  GpaOperationClass parent_class;

};

GType gpa_stream_operation_get_type (void) G_GNUC_CONST;

/*** API ***/



#endif /*GPA_STREAM_OP_H*/
