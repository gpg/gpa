/* cm-object.c  -  Top object for card parts of the card manager.
 * Copyright (C) 2009 g10 Code GmbH
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute and/or modify this part
 * of GPA under the terms of either
 *
 *   - the GNU Lesser General Public License as published by the Free
 *     Software Foundation; either version 3 of the License, or (at
 *     your option) any later version.
 *
 * or
 *
 *   - the GNU General Public License as published by the Free
 *     Software Foundation; either version 2 of the License, or (at
 *     your option) any later version.
 *
 * or both in parallel, as here.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gpa.h"

#include "cm-object.h"


/* Signals */
enum
{
  UPDATE_STATUS,
  ALERT_DIALOG,
  LAST_SIGNAL
};



/* The parent class.  */
static GObjectClass *parent_class;

/* The signal vector.  */
static guint signals [LAST_SIGNAL];

/* Local prototypes */
static void gpa_cm_object_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/




/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_cm_object_class_init (void *class_ptr, void *class_data)
{
  GpaCMObjectClass *klass = class_ptr;

  parent_class = g_type_class_peek_parent (klass);

  G_OBJECT_CLASS (klass)->finalize = gpa_cm_object_finalize;

  signals[UPDATE_STATUS] =
    g_signal_new ("update-status",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		  G_STRUCT_OFFSET (GpaCMObjectClass, update_status),
		  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
		  G_TYPE_NONE, 1, G_TYPE_STRING);

  signals[ALERT_DIALOG] =
    g_signal_new ("alert-dialog",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		  G_STRUCT_OFFSET (GpaCMObjectClass, alert_dialog),
		  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
		  G_TYPE_NONE, 1, G_TYPE_STRING);
}


static void
gpa_cm_object_init (GTypeInstance *instance, void *class_ptr)
{
/*   GpaCMObject *card = GPA_CM_OBJECT (instance); */

}


static void
gpa_cm_object_finalize (GObject *object)
{
/*   GpaCMObject *card = GPA_CM_OBJECT (object); */


  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_cm_object_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaCMObjectClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_cm_object_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaCMObject),
	  0,    /* n_preallocs */
	  gpa_cm_object_init
	};

      this_type = g_type_register_static (GTK_TYPE_VBOX,
                                          "GpaCMObject",
                                          &this_info, 0);
    }

  return this_type;
}


/************************************************************
 **********************  Public API  ************************
 ************************************************************/

/* Emit a status line names STATUSNAME plus space delimited
   arguments.  */
void
gpa_cm_object_update_status (GpaCMObject *obj, const char *text)
{
  g_return_if_fail (obj);
  g_return_if_fail (GPA_CM_OBJECT (obj));

  if (!text)
    text = "";

  g_signal_emit (obj, signals[UPDATE_STATUS], 0, text);
}

/* Emit the error message MSG.  */
void
gpa_cm_object_alert_dialog (GpaCMObject *obj, const gchar *messageg)
{
  g_return_if_fail (obj);
  g_return_if_fail (GPA_CM_OBJECT (obj));

  g_signal_emit (obj, signals[ALERT_DIALOG], 0, messageg);
}
