/* gpadatebutton.h  -  A button to show and select a date.
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

#ifndef GPADATEBUTTON_H
#define GPADATEBUTTON_H

#include <gtk/gtkbutton.h>


/* Declare the Object. */
typedef struct _GpaDateButton      GpaDateButton;
typedef struct _GpaDateButtonClass GpaDateButtonClass;

GType gpa_date_button_get_type (void) G_GNUC_CONST;

#define GPA_DATE_BUTTON_TYPE   (gpa_date_button_get_type ())

#define GPA_DATE_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_DATE_BUTTON_TYPE, GpaDateButton))

#define GPA_DATE_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_DATE_BUTTON_TYPE, GpaDateButtonClass))

#define IS_GPA_DATE_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_DATE_BUTTON_TYPE))

#define IS_ACCOUNTLIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_DATE_BUTTON_TYPE))

#define GPA_DATE_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_DATE_BUTTON_TYPE, GpaDateButtonClass))


/* The public functions.  */
GtkWidget *gpa_date_button_new (void);

void gpa_date_button_set_date (GpaDateButton *self, GDate *date);

gboolean gpa_date_button_get_date (GpaDateButton *self, GDate *r_date);


#endif /*GPADATEBUTTON_H*/
