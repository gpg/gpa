/* keylist.h  -  The GNU Privacy Assistant
 *      Copyright (C) 2000, 2001 G-N-U GmbH.
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

#ifndef KEYLIST_H
#define KEYLIST_H

typedef enum
{
  GPA_KEYLIST_NAME,
  GPA_KEYLIST_ID,
  GPA_KEYLIST_EXPIRYDATE,
  GPA_KEYLIST_KEYTRUST,
  GPA_KEYLIST_OWNERTRUST,
  GPA_KEYLIST_KEY_TYPE_PIXMAP
}
GPAKeyListColumn;

GtkWidget * gpa_keylist_new (gint ncolumns, GPAKeyListColumn * columnsm,
			     gint maxcolumns, GtkWidget * window);

GList * gpa_keylist_selection (GtkWidget * keylist);
gboolean gpa_keylist_has_selection (GtkWidget * keylist);
gboolean gpa_keylist_has_single_selection (GtkWidget * keylist);
gint gpa_keylist_selection_length (GtkWidget * keylist);

gpgme_key_t gpa_keylist_current_key (GtkWidget * keylist);
gchar *gpa_keylist_current_key_id (GtkWidget * keylist);

void gpa_keylist_set_column_defs (GtkWidget * keylist,
				  gint ncolumns, GPAKeyListColumn *columns);

void gpa_keylist_update_list (GtkWidget * keylist);


#endif /* KEYLIST_H */
