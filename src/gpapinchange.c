/* gpapinchange.c  - The GNU Privacy Assistant: PIN Change
   Copyright (C) 2009 g10 Code GmbH

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

/* This class is used to let the user perform a PIN change.  It has a
   couple of card specific features, for example a way to chnage the
   NullPIN as used by TCOS cards.

 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "gpa.h"

#include "gpapinchange.h"


/* Object and class definition.  */
struct _GpaPinChange
{
  GtkWindow parent;

  GtkWidget *window;

  GtkUIManager *ui_manager;

  GtkWidget *main_widget;     /* The container of all the info widgets.  */

};

struct _GpaPinChangeClass
{
  GtkWindowClass parent_class;
};


/* We need to save the parent class. */
static GObjectClass *parent_class;


/* Local prototypes */



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/





/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

/* Deinitialize this instance.  */
static void
gpa_pin_change_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* Initialize an instance of the this class.  */
static void
gpa_pin_change_init (GpaPinChange *pinchange)
{
}


/* Construct a new class object of GpaPinChange.  */
static GObject*
gpa_pin_change_constructor (GType type,
                            guint n_construct_properties,
                            GObjectConstructParam *construct_properties)
{
  GObject *object;
  /* GpaPinChange *pinchange; */

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  /* pinchange = GPA_PIN_CHANGE (object); */

  return object;
}


/* Initialize the class object.  */
static void
gpa_pin_change_class_init (GpaPinChangeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gpa_pin_change_constructor;
  object_class->finalize = gpa_pin_change_finalize;
}


/* Glue function to construct a class.  */
GType
gpa_pin_change_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaPinChangeClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_pin_change_class_init,
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  sizeof (GpaPinChange),
	  0,    /* n_preallocs */
	  (GInstanceInitFunc) gpa_pin_change_init,
	};

      this_type = g_type_register_static (GTK_TYPE_WINDOW,
                                          "GpaPinChange",
                                          &this_info, 0);
    }

  return this_type;
}



/************************************************************
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_pin_change_new (void)
{
  GObject *instance;

  instance = g_object_new (GPA_PIN_CHANGE_TYPE, NULL);
  return GTK_WIDGET (instance);
}

