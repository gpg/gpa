/* gpagenkeyop.h - The GpaGenKeyOperation object.
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

#ifndef GPA_GEN_KEY_OP_H
#define GPA_GEN_KEY_OP_H

#include "gpa.h"
#include <glib.h>
#include <glib-object.h>
#include "gpaoperation.h"

/* GObject stuff */
#define GPA_GEN_KEY_OPERATION_TYPE	  (gpa_gen_key_operation_get_type ())
#define GPA_GEN_KEY_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_GEN_KEY_OPERATION_TYPE, GpaGenKeyOperation))
#define GPA_GEN_KEY_OPERATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_GEN_KEY_OPERATION_TYPE, GpaGenKeyOperationClass))
#define GPA_IS_GEN_KEY_OPERATION(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_GEN_KEY_OPERATION_TYPE))
#define GPA_IS_GEN_KEY_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_GEN_KEY_OPERATION_TYPE))
#define GPA_GEN_KEY_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_GEN_KEY_OPERATION_TYPE, GpaGenKeyOperationClass))

typedef struct _GpaGenKeyOperation GpaGenKeyOperation;
typedef struct _GpaGenKeyOperationClass GpaGenKeyOperationClass;

struct _GpaGenKeyOperation {
  GpaOperation parent;

};

struct _GpaGenKeyOperationClass {
  GpaOperationClass parent_class;

  /* "A key was generated" signal.
   */
  void (*generated_key) (GpaGenKeyOperation *op, const char *new_key_fpr);
};

GType gpa_gen_key_operation_get_type (void) G_GNUC_CONST;

#endif
