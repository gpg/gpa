/* gpakeyselector.h  -  The GNU Privacy Assistant
 *      Copyright (C) 2003 Miguel Coca.
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

#ifndef GPA_KEY_SELECTOR_H
#define GPA_KEY_SELECTOR_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "gpacontext.h"

/* GObject stuff */
#define GPA_KEY_SELECTOR_TYPE	  (gpa_key_selector_get_type ())
#define GPA_KEY_SELECTOR(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_KEY_SELECTOR_TYPE, GpaKeySelector))
#define GPA_KEY_SELECTOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_KEY_SELECTOR_TYPE, GpaKeySelectorClass))
#define GPA_IS_KEY_SELECTOR(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_KEY_SELECTOR_TYPE))
#define GPA_IS_KEY_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_KEY_SELECTOR_TYPE))
#define GPA_KEY_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_KEY_SELECTOR_TYPE, GpaKeySelectorClass))

typedef struct _GpaKeySelector GpaKeySelector;
typedef struct _GpaKeySelectorClass GpaKeySelectorClass;

struct _GpaKeySelector {
  GtkTreeView parent;

  /* Whether we are listing secret or public keys */
  gboolean secret;

  /* Whether we want only usable keys.  */
  gboolean only_usable_keys;

  /* All gpgme_key_ts we hold. We keep them here to free them.
   * Ideally this should be done by the list itself, but that would
   * involve mucking around with GValues */
  GList *keys;
};

struct _GpaKeySelectorClass {
  GtkTreeViewClass parent_class;

};

GType gpa_key_selector_get_type (void) G_GNUC_CONST;

/* API */

GtkWidget *gpa_key_selector_new (gboolean secret, gboolean only_usable_keys);

/* Return a list of selected gpgme_key_t's. The caller must free the list, but
 * not dereference the keys, as they belong to the selector.
 */
GList *gpa_key_selector_get_selected_keys (GpaKeySelector * selector);

/* Return TRUE if at least one key is selected.
 */
gboolean gpa_key_selector_has_selection (GpaKeySelector * selector);

#endif /* GPA_KEY_SELECTOR_H */
