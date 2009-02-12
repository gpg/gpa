/* cm-object.h  -  Top object for card parts of the card manager.
 *      Copyright (C) 2009 g10 Code GmbH
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

#ifndef CM_OBJECT_H
#define CM_OBJECT_H

#include <gtk/gtk.h>

/* Declare the Object. */
typedef struct _GpaCMObject      GpaCMObject;
typedef struct _GpaCMObjectClass GpaCMObjectClass;

GType gpa_cm_object_get_type (void) G_GNUC_CONST;

#define GPA_CM_OBJECT_TYPE   (gpa_cm_object_get_type ())

#define GPA_CM_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_CM_OBJECT_TYPE, GpaCMObject))

#define GPA_CM_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_CM_OBJECT_TYPE, GpaCMObjectClass))

#define GPA_IS_CM_OBJECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_CM_OBJECT_TYPE))

#define GPA_IS_CM_OBJECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_CM_OBJECT_TYPE))

#define GPA_CM_OBJECT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_CM_OBJECT_TYPE, GpaCMObjectClass))


/* Object's class definition.  */
struct _GpaCMObjectClass 
{
  GtkVBoxClass parent_class;
};


/* Object definition.  */
struct _GpaCMObject
{
  GtkVBox  parent_instance;
};




/* The class specific API.  */




#endif /*CM_OBJECT_H*/
