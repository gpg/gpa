/* cm-dinsig.h  -  Widget to show information about a DINSIG card.
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

#ifndef CM_DINSIG_H
#define CM_DINSIG_H

#include <gtk/gtk.h>

/* Declare the Object. */
typedef struct _GpaCMDinsig      GpaCMDinsig;
typedef struct _GpaCMDinsigClass GpaCMDinsigClass;

GType gpa_cm_dinsig_get_type (void) G_GNUC_CONST;

#define GPA_CM_DINSIG_TYPE   (gpa_cm_dinsig_get_type ())

#define GPA_CM_DINSIG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_CM_DINSIG_TYPE, GpaCMDinsig))

#define GPA_CM_DINSIG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_CM_DINSIG_TYPE, GpaCMDinsigClass))

#define GPA_IS_CM_DINSIG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_CM_DINSIG_TYPE))

#define GPA_IS_CM_DINSIG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_CM_DINSIG_TYPE))

#define GPA_CM_DINSIG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_CM_DINSIG_TYPE, GpaCMDinsigClass))


/* The class specific API.  */
GtkWidget *gpa_cm_dinsig_new (void);
void gpa_cm_dinsig_reload (GtkWidget *widget, gpgme_ctx_t gpgagent);



#endif /*CM_DINSIG_H*/
