/* gpakeypasswdop.h - The GpaKeyPasswdOperation object.
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

#ifndef GPA_KEY_PASSWD_OP_H
#define GPA_KEY_PASSWD_OP_H

#include <glib.h>
#include <glib-object.h>
#include "gpa.h"
#include "gpakeyop.h"

/* GObject stuff */
#define GPA_KEY_PASSWD_OPERATION_TYPE	  (gpa_key_passwd_operation_get_type ())
#define GPA_KEY_PASSWD_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_KEY_PASSWD_OPERATION_TYPE, GpaKeyPasswdOperation))
#define GPA_KEY_PASSWD_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_KEY_PASSWD_OPERATION_TYPE, GpaKeyPasswdOperationClass))
#define GPA_IS_KEY_PASSWD_ENCRYPT_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_KEY_PASSWD_OPERATION_TYPE))
#define GPA_IS_KEY_PASSWD_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_KEY_PASSWD_OPERATION_TYPE))
#define GPA_KEY_PASSWD_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_KEY_PASSWD_OPERATION_TYPE, GpaKeyPasswdOperationClass))

typedef struct _GpaKeyPasswdOperation GpaKeyPasswdOperation;
typedef struct _GpaKeyPasswdOperationClass GpaKeyPasswdOperationClass;

struct _GpaKeyPasswdOperation {
  GpaKeyOperation parent;
};

struct _GpaKeyPasswdOperationClass {
  GpaKeyOperationClass parent_class;
};

GType gpa_key_passwd_operation_get_type (void) G_GNUC_CONST;

/* API */

/* Creates a new key deletion operation.
 */
GpaKeyPasswdOperation*
gpa_key_passwd_operation_new (GtkWidget *window, GList *keys);

#endif
