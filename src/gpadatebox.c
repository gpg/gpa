/* gpadatebox.c  -  A box to show an optional date.
 * Copyright (C) 2009 g10 Code GmbH
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "gpadatebutton.h"
#include "gpadatebox.h"



/* Object's class definition.  */
struct _GpaDateBoxClass 
{
  GtkHBoxClass parent_class;

};


/* Object definition.  */
struct _GpaDateBox
{
  GtkHBox parent_instance;

  GtkWidget *checkbox;   /* The check box.  */
  GtkWidget *datebtn;    /* The date button.  */
};


/* The parent class.  */
static GObjectClass *parent_class;


/* Local prototypes */
static void gpa_date_box_finalize (GObject *object);



/************************************************************ 
 *******************   Implementation   *********************
 ************************************************************/

static void
update_widgets (GpaDateBox *self)
{
  gboolean state;

  state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->checkbox));
  if (state && !self->datebtn)
    {
      self->datebtn = gpa_date_button_new ();
      gtk_box_pack_start (GTK_BOX (self), self->datebtn, FALSE, FALSE, 0);
      gtk_widget_show_all (GTK_WIDGET (self));
      gtk_button_clicked (GTK_BUTTON (self->datebtn));
    }
  else if (!state && self->datebtn)
    {
      gtk_widget_destroy (GTK_WIDGET (self->datebtn));
      self->datebtn = NULL;
      gtk_widget_show_all (GTK_WIDGET (self));
    }

}


static void
checkbox_toggled_cb (GtkToggleButton *button, gpointer user_data)
{
  GpaDateBox *self = GPA_DATE_BOX (user_data);
  (void)button;

  update_widgets (self);
}


/* Create the widgets.  */
static void
create_widgets (GpaDateBox *self)
{
  self->checkbox = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (self), self->checkbox, FALSE, FALSE, 0);
  g_signal_connect (self->checkbox, "toggled",
                    G_CALLBACK (checkbox_toggled_cb), self);

  update_widgets (self);
  gtk_widget_show_all (GTK_WIDGET (self));
} 



/************************************************************ 
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_date_box_class_init (void *class_ptr, void *class_data)
{
  GpaDateBoxClass *klass = class_ptr;
  (void)class_data;

  parent_class = g_type_class_peek_parent (klass);
  
  G_OBJECT_CLASS (klass)->finalize = gpa_date_box_finalize;
}


static void
gpa_date_box_init (GTypeInstance *instance, void *class_ptr)
{
  GpaDateBox *self = GPA_DATE_BOX (instance);
  (void)class_ptr;

  create_widgets (self);
}


static void
gpa_date_box_finalize (GObject *object)
{  
  GpaDateBox *self = GPA_DATE_BOX (object);
  (void)self;

  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_date_box_get_type (void)
{
  static GType this_type = 0;
  
  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaDateBoxClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_date_box_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaDateBox),
	  0,    /* n_preallocs */
	  gpa_date_box_init
	};
      
      this_type = g_type_register_static (GTK_TYPE_HBOX,
                                          "GpaDateBox",
                                          &this_info, 0);
    }
  
  return this_type;
}


/************************************************************ 
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_date_box_new (void)
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (GPA_DATE_BOX_TYPE, NULL));

  return obj;
}


void
gpa_date_box_set_date (GpaDateBox *self, GDate *date)
{
  g_return_if_fail (IS_GPA_DATE_BOX (self));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->checkbox), !!date);
  gpa_date_button_set_date (GPA_DATE_BUTTON (self->datebtn), date);

  update_widgets (self);
}


/* Store the current date at R_DATE.  Returns true if the date is
   valid and the check box is checked.  */
gboolean
gpa_date_box_get_date (GpaDateBox *self, GDate *r_date)
{
  g_return_val_if_fail (IS_GPA_DATE_BOX (self), FALSE);

  g_date_clear (r_date, 1);
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->checkbox)))
    return FALSE;
  return gpa_date_button_get_date (GPA_DATE_BUTTON (self->datebtn), r_date);
}
