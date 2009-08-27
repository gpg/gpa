/* uidlist.c  -	 The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
 *	Copyright (C) 2009 g10 Code GmbH
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
#include "uidlist.h"

/*
 *  Implement a List showing user IDs.
 */

typedef enum
{
  UIDS_NAME_COLUMN,
  UIDS_STATUS_COLUMN,
  UIDS_N_COLUMNS
} UserIDsListColumn;

/* Create the list of uids. */
GtkWidget *
gpa_uidlist_new (void)
{
  GtkListStore *store;
  GtkWidget *list;

  store = gtk_list_store_new (UIDS_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
  gtk_widget_set_size_request (list, 400, 100);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                        UIDS_NAME_COLUMN, GTK_SORT_ASCENDING);

#if 0
  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_ui_mode",
                    G_CALLBACK (gpa_siglist_ui_mode_changed_cb), list);
#endif

  return list;
}

/* Remove all columns from the list. */
static void
gpa_uidlist_clear_columns (GtkWidget *list)
{
  GList *columns, *i;

  columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (list));
  for (i = columns; i; i = g_list_next (i))
    gtk_tree_view_remove_column (GTK_TREE_VIEW (list),
				 (GtkTreeViewColumn*) i->data);
}

/* Add columns to list. */
static void
gpa_uidlist_add_columns (GtkWidget *list)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
						     "text", UIDS_NAME_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Status"), renderer,
						     "text", UIDS_STATUS_COLUMN,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
}

static void
add_uid (gpgme_user_id_t uid, GtkListStore *store)
{
  GtkTreeIter iter;
  const gchar *status;
  gchar *user_id;

  user_id = uid->uid;
  /* The list of revoked signatures might not be always available */
  if (uid->revoked)
    status = _("Revoked");
  else
    status = "";

  /* Append it to the list */
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      UIDS_NAME_COLUMN, user_id,
		      UIDS_STATUS_COLUMN, status,
		      -1);
}

static void
gpa_uidlist_set_all (GtkWidget *list, const gpgme_key_t key)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  gpgme_user_id_t uid;

  /* Set the appropiate columns */
  gpa_uidlist_clear_columns (list);
  gpa_uidlist_add_columns (list);

  /* Clear the model */
  gtk_list_store_clear (store);

  /* Iterate over UID's and signatures and add unique values to the hash */
  for (uid = key->uids; uid; uid = uid->next)
    add_uid (uid, store);
}

/* Update the list of signatures */
void
gpa_uidlist_set_uids (GtkWidget *list, gpgme_key_t key)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));

  if (key)
    gpa_uidlist_set_all (list, key);
  else
    gtk_list_store_clear (store);
}
