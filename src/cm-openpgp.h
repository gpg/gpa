/* cm-openpgp.h  -  OpenPGP card part for the card manager.
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

#ifndef CM_OPENPGP_H
#define CM_OPENPGP_H

#include <gtk/gtk.h>

/* Declare the Object. */
typedef struct _GpaCMOpenpgp      GpaCMOpenpgp;
typedef struct _GpaCMOpenpgpClass GpaCMOpenpgpClass;

GType gpa_cm_openpgp_get_type (void) G_GNUC_CONST;

#define GPA_CM_OPENPGP_TYPE   (gpa_cm_openpgp_get_type ())

#define GPA_CM_OPENPGP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_CM_OPENPGP_TYPE, GpaCMOpenpgp))

#define GPA_CM_OPENPGP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_CM_OPENPGP_TYPE, GpaCMOpenpgpClass))

#define GPA_IS_CM_OPENPGP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_CM_OPENPGP_TYPE))

#define GPA_IS_CM_OPENPGP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_CM_OPENPGP_TYPE))

#define GPA_CM_OPENPGP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_CM_OPENPGP_TYPE, GpaCMOpenpgpClass))


/* The class specific API.  */
GtkWidget *gpa_cm_openpgp_new (void);
void gpa_cm_openpgp_reload (GtkWidget *widget, gpgme_ctx_t gpgagent);
char *gpa_cm_openpgp_get_key_attributes (GtkWidget *widget);



#endif /*CM_OPENPGP_H*/
