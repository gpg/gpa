/* gpadatebutton.c  -  A button to show and select a date.
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
#include "i18n.h"



/* Object's class definition.  */
struct _GpaDateButtonClass
{
  GtkButtonClass parent_class;

  /* The signal function for "date-set". */
  void (*date_set)(GpaDateButton *self);
};


/* Object definition.  */
struct _GpaDateButton
{
  GtkButton parent_instance;

  GtkWidget *dialog;    /* NULL or the dialog popup window.  */
  GtkWidget *calendar;  /* The calendar object.  */

  GtkWidget *label;

  guint current_year;
  guint current_month;  /* 1..12 ! */
  guint current_day;

  int ignore_next_selection;
};


/* The parent class.  */
static GObjectClass *parent_class;


/* Local prototypes */
static void gpa_date_button_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/

static void
update_widgets (GpaDateButton *self)
{
  char buf[20];

  if (!self->current_day && !self->current_month && !self->current_year)
    *buf = 0;
  else
    snprintf (buf, sizeof buf, "%04d-%02d-%02d",
              self->current_year, self->current_month, self->current_day);

  gtk_label_set_text (GTK_LABEL (self->label), *buf? buf : _("(not set)"));
  if (self->calendar && *buf)
    {
      gtk_calendar_select_month (GTK_CALENDAR (self->calendar),
                                 self->current_month-1, self->current_year);
      gtk_calendar_select_day (GTK_CALENDAR (self->calendar),
                               self->current_day);
    }
}



/* Signal handler for "destroy" to the dialog window.  */
static void
destroy_cb (GtkWidget *widget, gpointer user_data)
{
  GpaDateButton *self = GPA_DATE_BUTTON (user_data);

  self->dialog = NULL;
}


static void
day_selected_cb (GtkWidget *widget, gpointer user_data)
{
  GpaDateButton *self = GPA_DATE_BUTTON (user_data);

  if (self->ignore_next_selection)
    {
      self->ignore_next_selection = 0;
      return;
    }

  gtk_calendar_get_date (GTK_CALENDAR (self->calendar),
                         &self->current_year,
                         &self->current_month,
                         &self->current_day);
  self->current_month++;
  update_widgets (self);

  g_signal_emit_by_name (self, "date-set");

  gtk_widget_destroy (self->dialog);
}

static void
month_changed_cb (GtkWidget *widget, gpointer user_data)
{
  GpaDateButton *self = GPA_DATE_BUTTON (user_data);

  self->ignore_next_selection = 1;
}


/* Create the widgets.  */
static void
create_widgets (GpaDateButton *self)
{
  self->label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (self->label), GTK_JUSTIFY_LEFT);

  update_widgets (self);
  gtk_widget_show (self->label);
  gtk_container_add (GTK_CONTAINER (self), self->label);
}



/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

/* Overloaded method for clicked.  */
static void
gpa_date_button_clicked (GtkButton *button)
{
  GpaDateButton *self = GPA_DATE_BUTTON (button);
  GtkWidget *toplevel;

  if (!self->dialog)
    {
      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
      if (!gtk_widget_is_toplevel (toplevel))
        toplevel = NULL;

      self->dialog = gtk_dialog_new_with_buttons
        (NULL,
         GTK_WINDOW (toplevel),
         (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
         GTK_STOCK_CLOSE, GTK_RESPONSE_REJECT,
         NULL);

      g_signal_connect (self->dialog, "destroy",
                        G_CALLBACK (destroy_cb), self);
      g_signal_connect_swapped (self->dialog, "response",
                                G_CALLBACK (gtk_widget_destroy), self->dialog);

      self->calendar = gtk_calendar_new ();
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (self->dialog)->vbox),
                         self->calendar);

      g_signal_connect (self->calendar, "day-selected",
                        G_CALLBACK (day_selected_cb), self);
      g_signal_connect (self->calendar, "month-changed",
                        G_CALLBACK (month_changed_cb), self);

      gtk_widget_show_all (self->dialog);
    }

  update_widgets (self);
  gtk_window_present (GTK_WINDOW (self->dialog));
}


static void
gpa_date_button_class_init (void *class_ptr, void *class_data)
{
  GpaDateButtonClass *klass = class_ptr;

  (void)class_data;

  parent_class = g_type_class_peek_parent (klass);

  G_OBJECT_CLASS (klass)->finalize = gpa_date_button_finalize;
  GTK_BUTTON_CLASS (klass)->clicked = gpa_date_button_clicked;

  g_signal_new ("date-set",
                G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (GpaDateButtonClass, date_set),
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE, 0);
}


static void
gpa_date_button_init (GTypeInstance *instance, void *class_ptr)
{
  GpaDateButton *self = GPA_DATE_BUTTON (instance);

  (void)class_ptr;

  create_widgets (self);
}


static void
gpa_date_button_finalize (GObject *object)
{
  GpaDateButton *self = GPA_DATE_BUTTON (object);
  (void)self;

  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_date_button_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaDateButtonClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_date_button_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaDateButton),
	  0,    /* n_preallocs */
	  gpa_date_button_init
	};

      this_type = g_type_register_static (GTK_TYPE_BUTTON,
                                          "GpaDateButton",
                                          &this_info, 0);
    }

  return this_type;
}


/************************************************************
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_date_button_new (void)
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (GPA_DATE_BUTTON_TYPE, NULL));

  return obj;
}


void
gpa_date_button_set_date (GpaDateButton *self, GDate *date)
{
  g_return_if_fail (IS_GPA_DATE_BUTTON (self));

  if (!date)
    {
      self->current_day = 0;
      self->current_month = 0;
      self->current_year = 0;
    }
  else
    {
      self->current_day = g_date_get_day (date);
      self->current_month = g_date_get_month (date);
      self->current_year = g_date_get_year (date);
    }

  update_widgets (self);
}


/* Store the current date at R_DATE.  Returns true if the date is
   valid.  */
gboolean
gpa_date_button_get_date (GpaDateButton *self, GDate *r_date)
{
  g_return_val_if_fail (IS_GPA_DATE_BUTTON (self), FALSE);

  g_date_clear (r_date, 1);
  if (!g_date_valid_dmy (self->current_day,
                         self->current_month, self->current_year))
    return FALSE;

  g_date_set_dmy (r_date, self->current_day,
                  self->current_month, self->current_year);
  return TRUE;
}
