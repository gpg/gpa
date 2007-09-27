/* gpagenkeyop.c - The GpaGenKeyOperation object.
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

#include <config.h>

#include <gpgme.h>
#include "gpa.h"
#include "i18n.h"
#include "gtktools.h"
#include "gpagenkeyop.h"

static GObjectClass *parent_class = NULL;

/* Signals */
enum
{
  GENERATED_KEY,
  LAST_SIGNAL
};
static guint signals [LAST_SIGNAL] = { 0 };

/* GObject boilerplate */

static void
gpa_gen_key_operation_init (GpaGenKeyOperation *op)
{
}

static void
gpa_gen_key_operation_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_gen_key_operation_class_init (GpaGenKeyOperationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_gen_key_operation_finalize;
  
  /* Signals */
  klass->generated_key = NULL;
  signals[GENERATED_KEY] =
    g_signal_new ("generated_key",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GpaGenKeyOperationClass,
				   generated_key),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__STRING,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);

}

GType
gpa_gen_key_operation_get_type (void)
{
  static GType operation_type = 0;
  
  if (!operation_type)
    {
      static const GTypeInfo operation_info =
      {
        sizeof (GpaGenKeyOperationClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_gen_key_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaGenKeyOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_gen_key_operation_init,
      };
      
      operation_type = g_type_register_static (GPA_OPERATION_TYPE,
					       "GpaGenKeyOperation",
					       &operation_info, 0);
    }
  
  return operation_type;
}

