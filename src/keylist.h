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

#ifndef GPA_KEYLIST_H
#define GPA_KEYLIST_H

#include <gtk/gtk.h>

/* GObject stuff */
#define GPA_KEYLIST_TYPE	  (gpa_keylist_get_type ())
#define GPA_KEYLIST(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_KEYLIST_TYPE, GpaKeyList))
#define GPA_KEYLIST_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_KEYLIST_TYPE, GpaKeyListClass))
#define GPA_IS_KEYLIST(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_KEYLIST_TYPE))
#define GPA_IS_KEYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_KEYLIST_TYPE))
#define GPA_KEYLIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_KEYLIST_TYPE, GpaKeyListClass))

typedef struct _GpaKeyList GpaKeyList;
typedef struct _GpaKeyListClass GpaKeyListClass;

struct _GpaKeyList {
  GtkTreeView parent;

  gboolean secret;
  /* Keys loaded into the model */
  GList *keys;
  /* Dialog for warning about a trustdb rebuilding */
  GtkWidget *dialog;
};

struct _GpaKeyListClass {
  GtkTreeViewClass parent_class;

  /* Signal handlers */
  void (*context_menu) (GpaKeyList *keylist);
};

GType gpa_keylist_get_type (void) G_GNUC_CONST;

/* API */

/* Create a new key list widget.
 */
GtkWidget *gpa_keylist_new (GtkWidget * window);

/* Set the key list in "brief" mode.
 */
void gpa_keylist_set_brief (GpaKeyList * keylist);

/* Set the key list in "detailed" mode
 */
void gpa_keylist_set_detailed (GpaKeyList * keylist);

/* Return true if any key is selected in the list.
 */
gboolean gpa_keylist_has_selection (GpaKeyList * keylist);

/* Return true if one, and only one, key is selected in the list.
 */
gboolean gpa_keylist_has_single_selection (GpaKeyList * keylist);

/* Return true if one, and only one, secret key is selected in the list.
 */
gboolean gpa_keylist_has_single_secret_selection (GpaKeyList * keylist);

/* Return a GList of selected keys. The caller must not dereference the keys
 * as they belong to the caller.
 */
GList *gpa_keylist_get_selected_keys (GpaKeyList * keylist);

/* Begin a reload of the keyring.
 */
void gpa_keylist_start_reload (GpaKeyList * keylist);

#endif /* GPA_KEYLIST_H */
