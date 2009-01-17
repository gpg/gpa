/* gpapinchange.h  -  The GNU Privacy Assistant: PIN Chnage
 *      Copyright (C) 2009 g10 Code GmbH
 *
 * This file is part of GPA
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

#ifndef GPAPINCHNAGE_H
#define GPAPINCHNAGE_H 1

#include <gtk/gtk.h>

/* Declare the Object. */
typedef struct _GpaPinChange GpaPinChange;
typedef struct _GpaPinChangeClass GpaPinChangeClass;

GType gpa_pin_change_get_type (void) G_GNUC_CONST;

#define GPA_PIN_CHANGE_TYPE	  (gpa_pin_change_get_type ())

#define GPA_PIN_CHANGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_PIN_CHANGE_TYPE, GpaPinChange))

#define GPA_PIN_CHANGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_PIN_CHANGE_TYPE, GpaPinChangeClass))

#define GPA_IS_PIN_CHANGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_PIN_CHANGE_TYPE))

#define GPA_IS_PIN_CHANGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_PIN_CHANGE_TYPE))

#define GPA_PIN_CHANGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_PIN_CHANGE_TYPE, GpaPinChangeClass))

/* The public API.  */

GtkWidget *gpa_pin_change_new (void);


#endif /*GPAPINCHNAGE_H*/
