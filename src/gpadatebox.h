/* gpadatebox.h  -  A box to show an optional date.
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

#ifndef GPADATEBOX_H
#define GPADATEBOX_H

#include <gtk/gtkbox.h>


/* Declare the Object. */
typedef struct _GpaDateBox      GpaDateBox;
typedef struct _GpaDateBoxClass GpaDateBoxClass;

GType gpa_date_box_get_type (void) G_GNUC_CONST;

#define GPA_DATE_BOX_TYPE   (gpa_date_box_get_type ())

#define GPA_DATE_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_DATE_BOX_TYPE, GpaDateBox))

#define GPA_DATE_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_DATE_BOX_TYPE, GpaDateBoxClass))

#define IS_GPA_DATE_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_DATE_BOX_TYPE))

#define IS_GPA_DATE_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_DATE_BOX_TYPE))

#define GPA_DATE_BOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_DATE_BOX_TYPE, GpaDateBoxClass))


/* The public functions.  */
GtkWidget *gpa_date_box_new (void);

void gpa_date_box_set_date (GpaDateBox *self, GDate *date);

gboolean gpa_date_box_get_date (GpaDateBox *self, GDate *r_date);


#endif /*GPADATEBOX_H*/
