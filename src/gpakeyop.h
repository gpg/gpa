/* gpakeyop.h - The GpaKeyOperation object.
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

#ifndef GPA_KEY_OP_H
#define GPA_KEY_OP_H

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaoperation.h"
#include "gpaprogressdlg.h"

/* GObject stuff */
#define GPA_KEY_OPERATION_TYPE	  (gpa_key_operation_get_type ())
#define GPA_KEY_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_KEY_OPERATION_TYPE, GpaKeyOperation))
#define GPA_KEY_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_KEY_OPERATION_TYPE, GpaKeyOperationClass))
#define GPA_IS_KEY_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_KEY_OPERATION_TYPE))
#define GPA_IS_KEY_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_KEY_OPERATION_TYPE))
#define GPA_KEY_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_KEY_OPERATION_TYPE, GpaKeyOperationClass))

typedef struct _GpaKeyOperation GpaKeyOperation;
typedef struct _GpaKeyOperationClass GpaKeyOperationClass;

struct _GpaKeyOperation {
  GpaOperation parent;

  GList *keys;
  GList *current;
};

struct _GpaKeyOperationClass {
  GpaOperationClass parent_class;

  /* Signal handlers */
  void (*changed_wot) (GpaKeyOperation *operation);
};

GType gpa_key_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Returns the list of keys on which the operation has been called. 
 */
GList*
gpa_key_operation_keys (GpaKeyOperation *op);

/* Returns the key the operation is currently being applied to.
 */
gpgme_key_t
gpa_key_operation_current_key (GpaKeyOperation *op);

#endif
