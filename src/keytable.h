/* keytable.h  -  The GNU Privacy Assistant
 *      Copyright (C) 2002 Miguel Coca
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

/*
 * Table of all the keys in the keyring. Singleton object.
 * Acts as a key cache for key listing.
 */

#ifndef KEYTABLE_H
#define KEYTABLE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gpgme.h>
#include "gpacontext.h"

/* GObject stuff */
#define GPA_KEYTABLE_TYPE	  (gpa_keytable_get_type ())
#define GPA_KEYTABLE(obj)	  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GPA_KEYTABLE_TYPE, GpaKeyTable))
#define GPA_KEYTABLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), GPA_KEYTABLE_TYPE, GpaKeyTableClass))
#define GPA_IS_KEYTABLE(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPA_KEYTABLE_TYPE))
#define GPA_IS_KEYTABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GPA_KEYTABLE_TYPE))
#define GPA_KEYTABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GPA_KEYTABLE_TYPE, GpaKeyTableClass))

typedef struct _GpaKeyTable GpaKeyTable;
typedef struct _GpaKeyTableClass GpaKeyTableClass;

typedef void (*GpaKeyTableNextFunc) (gpgme_key_t key, gpointer data);
typedef void (*GpaKeyTableEndFunc) (gpointer data);

struct _GpaKeyTable {
  GObject parent;

  GpaContext *context;

  gboolean secret;
  gboolean new_key;
  GpaKeyTableNextFunc next;
  GpaKeyTableEndFunc end;
  gpointer data;

  GList *keys, *tmp_list;
};

struct _GpaKeyTableClass {
  GObjectClass parent_class;
};

GType gpa_keytable_get_type (void) G_GNUC_CONST;

/* Retrieve the single keytable instance (one for public keys, one for secret
 * ones).
 */
GpaKeyTable *gpa_keytable_get_public_instance ();
GpaKeyTable *gpa_keytable_get_secret_instance ();

/* List all keys, return cached copies if they are available.
 *
 * The "next" function is called for every key, providing a new
 * reference for that key that should be freed.
 *
 * The "end" function is called when the listing is complete.
 *
 * This function MAY not do anything until the application goes back into
 * the GLib main loop.
 */
void gpa_keytable_list_keys (GpaKeyTable *keytable,
			     GpaKeyTableNextFunc next,
			     GpaKeyTableEndFunc end,
			     gpointer data);

/* Same as list_keys, but forces the internal cache to be rebuilt.
 */
void gpa_keytable_force_reload (GpaKeyTable *keytable,
				GpaKeyTableNextFunc next,
				GpaKeyTableEndFunc end,
				gpointer data);

/* Load the key with the given fingerprint from GnuPG, replacing it in the
 * keytable if needed.
 */
void gpa_keytable_load_new (GpaKeyTable *keytable,
			    const char *fpr,
			    GpaKeyTableNextFunc next,
			    GpaKeyTableEndFunc end,
			    gpointer data);

/* Return the key with a given fingerprint from the keytable, NULL if
 * there is none. No reference is provided.
 */
const gpgme_key_t gpa_keytable_lookup_key (GpaKeyTable *keytable,
					   const char *fpr);

#endif /* KEYTABLE_H */
