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

#include "gpa.h"
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <config.h>
#include "gpapastrings.h"
#include "siglist.h"

/*
 *  Implement a List showing signatures
 */

typedef enum
{
  SIG_KEYID_COLUMN,
  SIG_STATUS_COLUMN,
  SIG_USERID_COLUMN,
  SIG_N_COLUMNS
} SignatureListColumn;


/* Create the list of signatures */
GtkWidget *
gpa_siglist_new (void)
{
  GtkListStore *store;
  GtkWidget *list;

  store = gtk_list_store_new (SIG_N_COLUMNS, G_TYPE_STRING,
			      G_TYPE_STRING, G_TYPE_STRING);
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
  gtk_widget_set_size_request (list, 400, 100);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                        SIG_USERID_COLUMN, GTK_SORT_ASCENDING);

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

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("User Name"), renderer,
						     "text", SIG_USERID_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
}


typedef struct
{
  const gchar *keyid, *status;
  gchar *userid;
} KeySignature;

static void
signature_free (KeySignature *sig)
{
  g_free (sig->userid);
  g_free (sig);
}

static void
add_signature (gchar *keyid, KeySignature *sig, GtkListStore *store)
{
  GtkTreeIter iter;
  
  /* Append it to the list */
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      SIG_KEYID_COLUMN, sig->keyid, 
                      SIG_STATUS_COLUMN, sig->status,
                      SIG_USERID_COLUMN, sig->userid, -1);
}

static void
gpa_siglist_set_all (GtkWidget * list, const char *fpr)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  gpgme_key_t key;
  gpgme_error_t err;
  int i, uid;
  gpgme_ctx_t ctx = gpa_gpgme_new ();
  int old_mode = gpgme_get_keylist_mode (ctx);
  const gchar *keyid;

  /* Create the hash table */
  GHashTable *hash = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                                            (GDestroyNotify) signature_free);

  /* Set the appropiate columns */
  gpa_siglist_clear_columns (list);
  gpa_siglist_all_add_columns (list);

  /* Get the key */
  gpgme_set_keylist_mode (ctx, old_mode | GPGME_KEYLIST_MODE_SIGS);
  err = gpgme_get_key (ctx, fpr, &key, FALSE);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  gpgme_set_keylist_mode (ctx, old_mode); 

  /* Clear the model */
  gtk_list_store_clear (store);

  /* Iterate over UID's and signatures and add unique values to the hash */
  for (uid = 0; gpgme_key_get_string_attr (key, GPGME_ATTR_USERID, 0,
                                           uid); uid++)
    {
      for (i = 0; (keyid = 
                   gpgme_key_sig_get_string_attr (key, uid, GPGME_ATTR_KEYID, 
                                                  NULL, i)) != NULL; i++)
        {
          /* Here we assume (wrongly) that long KeyID are unique. But there
           * is basically no other way to do this, and in this context it
           * doens't matter that much (at most, one signature will be missing
           * from the "all" list).*/
          if (!g_hash_table_lookup (hash, keyid))
            {
              /* Add the signature to the list */
              /* FIXME: This saves the first signature on the key in each UID,
               * if they have different attributes, this may cause trouble */
              KeySignature *sig;
              sig = g_malloc (sizeof (KeySignature));
              sig->keyid = gpa_gpgme_key_sig_get_short_keyid (key, uid, i);
              sig->userid = gpa_gpgme_key_sig_get_userid (key, uid, i);
              sig->status = "";
              g_hash_table_insert (hash, (gchar*) keyid, sig);
            }
        }
    }
  /* Now, add each signature to the list */
  g_hash_table_foreach (hash, (GHFunc)add_signature, store);
  
  /* Delete the hash table */
  g_hash_table_destroy (hash);
  
  /* We don't need the key anymore */
  gpgme_key_unref (key); 

  gpgme_release (ctx);
}

static GHashTable*
revoked_signatures (gpgme_key_t key, int uid)
{
  int i;
  const gchar *keyid;
  GHashTable *hash = g_hash_table_new (g_str_hash, g_str_equal);

  for (i = 0; (keyid = gpgme_key_sig_get_string_attr (key, uid,
                                                      GPGME_ATTR_KEYID, 
                                                      NULL, i)); i++)
    {
      if (gpgme_key_sig_get_ulong_attr (key, uid, GPGME_ATTR_KEY_REVOKED, 
                                        NULL, i))
        {
          g_hash_table_insert (hash, (gchar*) keyid, (gchar*) keyid);
        }
    }

  return hash;
}

static void
gpa_siglist_set_userid (GtkWidget * list, const char *fpr, int idx)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  gpgme_key_t key;
  gpgme_error_t err;
  int i;
  gpgme_ctx_t ctx = gpa_gpgme_new ();
  int old_mode = gpgme_get_keylist_mode (ctx);
  GHashTable *revoked;

  /* Set the appropiate columns */
  gpa_siglist_clear_columns (list);
  gpa_siglist_uid_add_columns (list);

  /* Get the key */
  gpgme_set_keylist_mode (ctx, old_mode | GPGME_KEYLIST_MODE_SIGS);
  err = gpgme_get_key (ctx, fpr, &key, FALSE);
  if (err != GPGME_No_Error)
    {
      gpa_gpgme_error (err);
    }
  gpgme_set_keylist_mode (ctx, old_mode); 

  /* Get the list of revoked signatures */
  revoked = revoked_signatures (key, idx);

  /* Clear the model */
  gtk_list_store_clear (store);
  /* Add the signatures to the model */
  for (i = 0; gpgme_key_sig_get_string_attr (key, idx, GPGME_ATTR_KEYID, 
                                             NULL, i); i++)
    {
      /* Ignore revocation signatures */
      if (!gpgme_key_sig_get_ulong_attr (key, idx, GPGME_ATTR_KEY_REVOKED, 
                                         NULL, i))
        {
          GtkTreeIter iter;
          const gchar *keyid, *status;
          gchar *userid;
          
          /* The Key ID */
          keyid = gpa_gpgme_key_sig_get_short_keyid (key, idx, i);
          /* The user ID */
          userid = gpa_gpgme_key_sig_get_userid (key, idx, i);
          /* The signature status */
          status = gpa_gpgme_key_sig_get_sig_status (key, idx, i, revoked);
          /* Append it to the list */
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              SIG_KEYID_COLUMN, keyid, 
                              SIG_STATUS_COLUMN, status,
                              SIG_USERID_COLUMN, userid, -1);
          /* Clean up */
          g_free (userid);
        }
    }
  
  /* We don't need the key anymore */
  gpgme_key_unref (key);

  gpgme_release (ctx);
}

/* Update the list of signatures */
void
gpa_siglist_set_signatures (GtkWidget * list, gchar *fpr, int idx)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  if (fpr)
    {
      if (idx == -1)
        {
          gpa_siglist_set_all (list, fpr);
        }
      else
        {
          gpa_siglist_set_userid (list, fpr, idx);
        }
    }
  else
    {
      gtk_list_store_clear (store);
    }
}
