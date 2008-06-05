/* gpaop.h - The GpaOperation object.
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

#ifndef GPA_OPERATION_H
#define GPA_OPERATION_H

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gpacontext.h"

/* GObject stuff */
#define GPA_OPERATION_TYPE	  (gpa_operation_get_type ())
#define GPA_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_OPERATION_TYPE, GpaOperation))
#define GPA_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_OPERATION_TYPE, GpaOperationClass))
#define GPA_IS_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_OPERATION_TYPE))
#define GPA_IS_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_OPERATION_TYPE))
#define GPA_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_OPERATION_TYPE, GpaOperationClass))

typedef struct _GpaOperation GpaOperation;
typedef struct _GpaOperationClass GpaOperationClass;

struct _GpaOperation {
  GObject parent;

  GtkWidget *window;
  GpaContext *context;
  char *client_title;
};

struct _GpaOperationClass {
  GObjectClass parent_class;

  /* Signal handlers */
  void (*completed) (GpaOperation *operation, gpg_error_t err);
  void (*status) (GpaOperation *operation, gchar *status);
};

GType gpa_operation_get_type (void) G_GNUC_CONST;

/*** API ***/

/* Whether the operation is currently busy (i.e. gpg is running).  */
gboolean gpa_operation_busy (GpaOperation *op);


/* If running in server mode, write a status line names STATUSNAME
   plus space delimited arguments.  */
gpg_error_t gpa_operation_write_status (GpaOperation *op, 
                                        const char *statusname, ...);


#endif
