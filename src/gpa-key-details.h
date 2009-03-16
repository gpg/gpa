/* gpa-key-details.h  -  A widget to show key details.
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

#ifndef GPA_KEY_DETAILS_H
#define GPA_KEY_DETAILS_H

#include <gtk/gtk.h>

/* Declare the Object. */
typedef struct _GpaKeyDetails      GpaKeyDetails;
typedef struct _GpaKeyDetailsClass GpaKeyDetailsClass;

GType gpa_key_details_get_type (void) G_GNUC_CONST;

#define GPA_KEY_DETAILS_TYPE   (gpa_key_details_get_type ())

#define GPA_KEY_DETAILS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_KEY_DETAILS_TYPE, GpaKeyDetails))

#define GPA_KEY_DETAILS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass),  \
                            GPA_KEY_DETAILS_TYPE, GpaKeyDetailsClass))

#define GPA_IS_KEY_DETAILS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_KEY_DETAILS_TYPE))

#define GPA_IS_KEY_DETAILS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_KEY_DETAILS_TYPE))

#define GPA_KEY_DETAILS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),    \
                              GPA_KEY_DETAILS_TYPE, GpaKeyDetailsClass))


/* The class specific API.  */
GtkWidget *gpa_key_details_new (void);
void gpa_key_details_update (GtkWidget *keydetails,
                            gpgme_key_t key, int keycount);
void gpa_key_details_find (GtkWidget *keydetails, const char *pattern);


#endif /*GPA_KEY_DETAILS_H*/
