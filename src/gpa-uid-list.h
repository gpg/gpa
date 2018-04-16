/* gpa-uid-list.h - A list to show TOFU information
 * Copyright (C) 2018 g10 Code GmbH
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GPA_UID_LIST_H
#define GPA_UID_LIST_H

#include <gtk/gtk.h>

/* Create a new User ID list.  */
GtkWidget * gpa_uid_list_new (void);

/* Set the key for which User ID information shall be shown.  */
void gpa_uid_list_set_key (GtkWidget *list, gpgme_key_t key);

#endif /* GPA_UID_LIST_H */
