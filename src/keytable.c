/* keytable.c  -  The GNU Privacy Assistant
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

#include <glib.h>
#include <gtk/gtk.h>
#include "gpa.h"
#include "gpgmetools.h"
#include "keytable.h"
#include "gtktools.h"

/* Internal */
static void done_cb (GpaContext *context, gpg_error_t err,
		     GpaKeyTable *keytable);
static void next_key_cb (GpaContext *context, gpgme_key_t key,
			 GpaKeyTable *keytable);

/* GObject type functions */

static void gpa_keytable_init (GpaKeyTable *keytable);
static void gpa_keytable_class_init (GpaKeyTableClass *klass);
static void gpa_keytable_finalize (GObject *object);

static GObjectClass *parent_class = NULL;

GType
gpa_keytable_get_type (void)
{
  static GType keytable_type = 0;
  
  if (!keytable_type)
    {
      static const GTypeInfo keytable_info =
      {
        sizeof (GpaKeyTableClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_keytable_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaKeyTable),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_keytable_init,
      };
      
      keytable_type = g_type_register_static (G_TYPE_OBJECT,
					      "GpaTable",
					      &keytable_info, 0);
    }
  
  return keytable_type;
}

static void
gpa_keytable_class_init (GpaKeyTableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_keytable_finalize;
}

static void
gpa_keytable_init (GpaKeyTable *keytable)
{
  keytable->next = NULL;
  keytable->end = NULL;
  keytable->data = NULL;
  keytable->context = gpa_context_new ();
  keytable->keys = NULL;
  keytable->secret = FALSE;
  keytable->initialized = FALSE;
  keytable->new_key = FALSE;
  keytable->tmp_list = NULL;
  g_signal_connect (G_OBJECT (keytable->context), "next_key",
		    G_CALLBACK (next_key_cb), keytable);
  g_signal_connect (G_OBJECT (keytable->context), "done",
		    G_CALLBACK (done_cb), keytable);
}

static void
gpa_keytable_finalize (GObject *object)
{
  GpaKeyTable *keytable = GPA_KEYTABLE (object);

  g_object_unref (keytable->context);
  g_list_foreach (keytable->keys, (GFunc) gpgme_key_unref, NULL);
  g_list_free (keytable->keys);
}

/* Internal functions */

static void reload_cache (GpaKeyTable *keytable, const char *fpr)
{
  gpg_error_t err;
  
  err = gpgme_op_keylist_start (keytable->context->ctx, fpr,
				keytable->secret);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      gpa_gpgme_warning (err);
      if (keytable->end)
	{
	  keytable->end (keytable->data);
	}
      return;
    }
  keytable->tmp_list = NULL;
}

static void done_cb (GpaContext *context, gpg_error_t err,
		     GpaKeyTable *keytable)
{
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      gpa_gpgme_warning (err);
      return;
    }
  /* Reverse the list to have the keys come up in the same order they
   * were listed */
  keytable->tmp_list = g_list_reverse (keytable->tmp_list);
  if (keytable->new_key)
    {
      /* Append the new key(s) 
       */
      keytable->keys = g_list_concat (keytable->keys, keytable->tmp_list);
    }
  else
    {
      /* Replace the list
       */
      if (keytable->keys)
	{
	  g_list_foreach (keytable->keys, (GFunc) gpgme_key_unref, 
			  NULL);
	  g_list_free (keytable->keys); 
	}
      keytable->keys = keytable->tmp_list;
    }
  keytable->initialized = TRUE;
  if (keytable->end)
    {
      keytable->end (keytable->data);
    }
}

static void next_key_cb (GpaContext *context, gpgme_key_t key,
			 GpaKeyTable *keytable)
{
  keytable->tmp_list = g_list_prepend (keytable->tmp_list, key);
  gpgme_key_ref (key);
  if (keytable->next)
    {
      keytable->next (key, keytable->data);
    }
}

static void list_cache (GpaKeyTable *keytable)
{
  GList *list = keytable->keys;
  
  for (; list; list = g_list_next (list))
    {
      gpgme_key_t key = (gpgme_key_t) list->data;
      gpgme_key_ref (key);
      if (keytable->next)
	{
	  keytable->next (key, keytable->data);
	}
    }

  if (keytable->end)
    {
      keytable->end (keytable->data);
    }
}

/* API */

static GpaKeyTable *public_instance = NULL;
static GpaKeyTable *secret_instance = NULL;

/* Create a new keytable. Internal, called from get_instance.
 */
static GpaKeyTable *
gpa_keytable_new (gboolean secret)
{
  GpaKeyTable *keytable;
  
  keytable = g_object_new (GPA_KEYTABLE_TYPE, NULL);
  keytable->secret = secret;

  return keytable;
}

/* Retrieve the public keytable instance.
 */
GpaKeyTable *gpa_keytable_get_public_instance ()
{
  if (!public_instance)
    {
      public_instance = gpa_keytable_new (FALSE);
    }
  return public_instance;
}

/* Retrieve the secret keytable instance.
 */
GpaKeyTable *gpa_keytable_get_secret_instance ()
{
  if (!secret_instance)
    {
      secret_instance = gpa_keytable_new (TRUE);
    }
  return secret_instance;
}

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
			     gpointer data)
{
  g_return_if_fail (keytable != NULL);
  g_return_if_fail (GPA_IS_KEYTABLE (keytable));

  /* Set up callbacks */
  keytable->next = next;
  keytable->end = end;
  keytable->data = data;
  /* List keys */
  if (keytable->keys)
    {
      /* There is a cached list */
      list_cache (keytable);
    }
  else 
    {
      reload_cache (keytable, NULL);
    }
}

/* Same as list_keys, but forces the internal cache to be rebuilt.
 */
void gpa_keytable_force_reload (GpaKeyTable *keytable,
				GpaKeyTableNextFunc next,
				GpaKeyTableEndFunc end,
				gpointer data)
{
  g_return_if_fail (keytable != NULL);
  g_return_if_fail (GPA_IS_KEYTABLE (keytable));

  /* Set up callbacks */
  keytable->next = next;
  keytable->end = end;
  keytable->data = data;
  /* List keys */
  reload_cache (keytable, NULL);
}

/* Load the key with the given fingerprint from GnuPG, replacing it in the
 * keytable if needed.
 */
void gpa_keytable_load_new (GpaKeyTable *keytable,
			    const char *fpr,
			    GpaKeyTableNextFunc next,
			    GpaKeyTableEndFunc end,
			    gpointer data)
{
  g_return_if_fail (keytable != NULL);
  g_return_if_fail (GPA_IS_KEYTABLE (keytable));

  /* Set up callbacks */
  keytable->next = next;
  keytable->end = end;
  keytable->data = data;
  /* List keys */
  keytable->new_key = TRUE;
  reload_cache (keytable, fpr);
}

/* Return the key with a given fingerprint from the keytable, NULL if
 * there is none. No reference is provided.
 */
const gpgme_key_t gpa_keytable_lookup_key (GpaKeyTable *keytable,
					   const char *fpr)
{
  if (keytable->initialized)
    {
      GList *cur;
      for (cur = keytable->keys; cur; cur = g_list_next (cur))
	{
	  gpgme_key_t key = (gpgme_key_t) cur->data;
	  if (g_str_equal (fpr, key->subkeys->fpr))
	    {
	      return key;
	    }
	}
      return NULL;
    }
  else
    {
      /* There is no list yet. We really, really, need to one, so we list it.
       * FIXME: This is a hack and a basic problem. Hopefully it won't cause
       * any real problems.
       */
      keytable->end = (GpaKeyTableEndFunc) gtk_main_quit;
      reload_cache (keytable, NULL);
      gtk_main ();
      keytable->end = NULL;
      return gpa_keytable_lookup_key (keytable, fpr);
    }
}
