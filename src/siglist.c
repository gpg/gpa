/* siglist.c  -	 The GNU Privacy Assistant
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gpa.h"
#include "siglist.h"

/*
 *  Implement a List showing signatures
 */

typedef enum
{
  SIG_KEYID_COLUMN,
  SIG_STATUS_COLUMN,
  SIG_USERID_COLUMN,
  SIG_LOCAL_COLUMN,
  SIG_LEVEL_COLUMN,
  SIG_N_COLUMNS
} SignatureListColumn;

gboolean
search_siglist_function (GtkTreeModel *model, int column,
                         const gchar *key_to_search_for, GtkTreeIter *iter,
                         gpointer search_data)
{
  gboolean result=TRUE;
  gchar *key_id, *user_id;
  gint search_len;

  gtk_tree_model_get (model, iter,
                     SIG_KEYID_COLUMN, &key_id,
                     SIG_USERID_COLUMN, &user_id, -1);

  search_len = strlen (key_to_search_for);

  if (!g_ascii_strncasecmp (key_id, key_to_search_for, search_len))
	result=FALSE;
  if (!g_ascii_strncasecmp (user_id, key_to_search_for, search_len))
	result=FALSE;

  g_free (key_id);
  g_free (user_id);

  return result;
}

static void
gpa_siglist_ui_mode_changed_cb (GpaOptions *options, GtkWidget *list);

/* Create the list of signatures */
GtkWidget *
gpa_siglist_new (void)
{
  GtkListStore *store;
  GtkWidget *list;

  store = gtk_list_store_new (SIG_N_COLUMNS, G_TYPE_STRING,
			      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN,
			      G_TYPE_STRING);
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
  gtk_widget_set_size_request (list, 400, 100);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                        SIG_USERID_COLUMN, GTK_SORT_ASCENDING);

  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (list), TRUE);
  gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (list),
                                       search_siglist_function, NULL, NULL);

  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_ui_mode",
                    G_CALLBACK (gpa_siglist_ui_mode_changed_cb), list);

  return list;
}

/* Remove all columns from the list */
static void
gpa_siglist_clear_columns (GtkWidget *list)
{
  GList *columns, *i;

  columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (list));
  for (i = columns; i; i = g_list_next (i))
    {
      gtk_tree_view_remove_column (GTK_TREE_VIEW (list),
                                   (GtkTreeViewColumn*) i->data);
    }
}

/* Add columns common to signatures on all UID's */
static void
gpa_siglist_all_add_columns (GtkWidget *list)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Key ID"), renderer,
						     "text", SIG_KEYID_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("User Name"), renderer,
						     "text", SIG_USERID_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
}

/* Add columns for signatures on one UID */
static void
gpa_siglist_uid_add_columns (GtkWidget *list)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Key ID"), renderer,
						     "text", SIG_KEYID_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Status"), renderer,
						     "markup", 
						     SIG_STATUS_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  if (!gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    {
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (_("Level"), renderer,
							 "markup", 
							 SIG_LEVEL_COLUMN,
							 NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
      
      renderer = gtk_cell_renderer_toggle_new ();
      column = gtk_tree_view_column_new_with_attributes (_("Local"), renderer,
							 "active", 
							 SIG_LOCAL_COLUMN,
							 NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
    }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("User Name"), renderer,
						     "text", SIG_USERID_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
}

static void
add_signature (gpgme_key_sig_t sig, GtkListStore *store, GHashTable *revoked)
{
  GtkTreeIter iter;
  const gchar *sig_level, *status;
  gchar *user_id;

  sig_level = gpa_gpgme_key_sig_get_level (sig);
  user_id = gpa_gpgme_key_sig_get_userid (sig);
  /* The list of revoked signatures might not be always available */
  if (revoked)
    {
      status = gpa_gpgme_key_sig_get_sig_status (sig, revoked);
    }
  else
    {
      status = "";
    }
  /* Append it to the list */
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      SIG_KEYID_COLUMN, gpa_gpgme_key_sig_get_short_keyid(sig),
		      SIG_STATUS_COLUMN, status,
                      SIG_USERID_COLUMN, user_id,
		      SIG_LEVEL_COLUMN, sig_level,
		      SIG_LOCAL_COLUMN, !sig->exportable,
		      -1);
  g_free (user_id);
}

static void add_signature_cb (gchar *keyid, gpgme_key_sig_t sig,
			      GtkListStore *store)
{
  add_signature (sig, store, NULL);
}

static void
gpa_siglist_set_all (GtkWidget * list, const gpgme_key_t key)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  gpgme_user_id_t uid;

  /* Create the hash table */
  GHashTable *hash = g_hash_table_new (g_str_hash, g_str_equal);

  /* Set the appropiate columns */
  gpa_siglist_clear_columns (list);
  gpa_siglist_all_add_columns (list);

  /* Clear the model */
  gtk_list_store_clear (store);

  /* Iterate over UID's and signatures and add unique values to the hash */
  for (uid = key->uids; uid; uid = uid->next)
    {
      gpgme_key_sig_t sig;
      for (sig = uid->signatures; sig; sig = sig->next)
        {
	  char *keyid = sig->keyid;
          /* Here we assume (wrongly) that long KeyID are unique. But there
           * is basically no other way to do this, and in this context it
           * doens't matter that much (at most, one signature will be missing
           * from the "all" list).*/
          if (!g_hash_table_lookup (hash, keyid))
            {
              /* Add the signature to the list */
              /* FIXME: This saves the first signature on the key in each UID,
               * if they have different attributes, this may cause trouble */
              g_hash_table_insert (hash, (gchar*) keyid, sig);
            }
        }
    }
  /* Now, add each signature to the list */
  g_hash_table_foreach (hash, (GHFunc)add_signature_cb, store);
  
  /* Delete the hash table */
  g_hash_table_destroy (hash);
}

static GHashTable*
revoked_signatures (gpgme_key_t key, gpgme_user_id_t uid)
{
  GHashTable *hash = g_hash_table_new (g_str_hash, g_str_equal);
  gpgme_key_sig_t sig;

  for (sig = uid->signatures; sig; sig = sig->next)
    {
      if (sig->revoked)
        {
          g_hash_table_insert (hash, (gchar*) sig->keyid, (gchar*) sig->keyid);
        }
    }

  return hash;
}

static void
gpa_siglist_set_userid (GtkWidget * list, const gpgme_key_t key,
			gpgme_user_id_t uid)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  GHashTable *revoked;
  gpgme_key_sig_t sig;

  /* Set the appropiate columns */
  gpa_siglist_clear_columns (list);
  gpa_siglist_uid_add_columns (list);

  /* Clear the model */
  gtk_list_store_clear (store);

  if (!uid)
    /* No user ID -> no signatures, do nothing here. */
    return;

  /* Get the list of revoked signatures */
  revoked = revoked_signatures (key, uid);

  /* Add the signatures to the model */
  for (sig = uid->signatures; sig; sig = sig->next)
    {
      /* Ignore revocation signatures */
      if (!sig->revoked)
        {
	  add_signature (sig, store, revoked);
        }
    }
}

/* Update the siglist to the right mode */
static void
gpa_siglist_ui_mode_changed_cb (GpaOptions *options, GtkWidget *list)
{
  /* Rebuild the columns */
  gpa_siglist_clear_columns (list);
  if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (list), "all_signatures")))
    {
      gpa_siglist_all_add_columns (list);
    }
  else
    {
      gpa_siglist_uid_add_columns (list);
    }
}

/* Update the list of signatures */
void
gpa_siglist_set_signatures (GtkWidget * list, gpgme_key_t key, int idx)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  if (key)
    {
      if (idx == -1)
        {
          gpa_siglist_set_all (list, key);
	  g_object_set_data (G_OBJECT (list), "all_signatures", 
			     GINT_TO_POINTER (TRUE));
        }
      else
        {
	  gpgme_user_id_t uid;
	  int i;
	  /* Find the right user id */
	  for (i = 0, uid = key->uids; i < idx; i++, uid = uid->next)
	    {
	    }
	  gpa_siglist_set_userid (list, key, uid);
	  g_object_set_data (G_OBJECT (list), "all_signatures", 
			     GINT_TO_POINTER (FALSE));
        }
    }
  else
    {
      gtk_list_store_clear (store);
    }
}
