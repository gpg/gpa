/* gpawidgets.h  -  The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef GPAWIDGETS_H
#define GPAWIDGETS_H

#include <gtk/gtk.h>

GtkWidget * gpa_secret_key_list_new (GtkWidget *window);
GtkWidget * gpa_public_key_list_new (GtkWidget *window);
gint gpa_key_list_selection_length (GtkWidget *clist);
GList * gpa_key_list_selected_ids (GtkWidget *clist);
gchar * gpa_key_list_selected_id (GtkWidget *clist);

GtkWidget * gpa_expiry_frame_new (GtkAccelGroup * accelGroup,
				  GDate * expiryDate);
gchar * gpa_expiry_frame_validate(GtkWidget * expiry_frame);
gboolean gpa_expiry_frame_get_expiration(GtkWidget * expiry_frame,
					 GDate ** date,
					 int * interval, gchar * unit);

#endif /* GPAWIDGETS_H */
