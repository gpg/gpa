/* cm-object.h  -  Top object for card parts of the card manager.
 *      Copyright (C) 2009 g10 Code GmbH
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

  /* Signal handlers */
  void (*update_status) (GpaCMObject *obj, gchar *status);
  void (*alert_dialog) (GpaCMObject *obj, gchar *message);
};


/* Object definition.  */
struct _GpaCMObject
{
  GtkVBox  parent_instance;

  /* Private.  Fixme:  Hide them.  */
  gpgme_ctx_t agent_ctx;
};




/* The class specific API.  */

void gpa_cm_object_update_status (GpaCMObject *obj, const char *text);
void gpa_cm_object_alert_dialog (GpaCMObject *obj, const gchar *messageg);


#endif /*CM_OBJECT_H*/
