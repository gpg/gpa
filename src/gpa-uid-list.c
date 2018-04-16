/* gpa-uid-list.c - A list to show User ID information.
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "gpa.h"
#include "convert.h"
#include "gtktools.h"
#include "keytable.h"
#include "gpa-uid-list.h"

static gboolean uid_list_query_tooltip_cb (GtkWidget *wdiget, int x, int y,
                                           gboolean keyboard_mode,
                                           GtkTooltip *tooltip,
                                           gpointer user_data);



typedef enum
{
  UID_ADDRESS,
  UID_VALIDITY,
  UID_UPDATE,
  UID_FULLUID,
  UID_N_COLUMNS
} UidListColumn;


/* Create a new user id list.  */
GtkWidget *
gpa_uid_list_new (void)
{
  GtkListStore *store;
  GtkWidget *list;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  /* Init the model */
  store = gtk_list_store_new (UID_N_COLUMNS,
			      G_TYPE_STRING,  /* address */
			      G_TYPE_STRING,  /* validity */
			      G_TYPE_STRING,  /* updated */
			      G_TYPE_STRING   /* fulluid */
                              );

  /* The view */
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);

  /* Add the columns */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", UID_ADDRESS,
						     NULL);
  gpa_set_column_title (column, _("Address"),
                        _("The mail address."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", UID_VALIDITY,
						     NULL);
  gpa_set_column_title (column, _("Validity"),
                        _("The validity of the mail address\n"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", UID_UPDATE,
						     NULL);
  gpa_set_column_title (column, _("Update"),
                _("The date the key was last updated via this mail address."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", UID_FULLUID,
						     NULL);
  gpa_set_column_title (column, _("User ID"),
                        _("The full user ID."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);


  g_object_set (list, "has-tooltip", TRUE, NULL);
  g_signal_connect (list, "query-tooltip",
                    G_CALLBACK (uid_list_query_tooltip_cb), list);

  return list;
}


/* Set the key whose user ids shall be displayed. */
void
gpa_uid_list_set_key (GtkWidget *list, gpgme_key_t key)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  GtkTreeIter iter;
  gpgme_user_id_t uid;

  /* Empty the list */
  gtk_list_store_clear (store);

  if (!key || !key->uids)
    return;

  for (uid = key->uids; uid; uid = uid->next)
    {
      char *lupd = gpa_update_origin_string (uid->last_update, uid->origin);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set
        (store, &iter,
         UID_ADDRESS,  uid->address? uid->address : "",
         UID_VALIDITY, gpa_uid_validity_string (uid),
         UID_UPDATE,   lupd,
         UID_FULLUID,  uid->uid,
         -1);

      g_free (lupd);
    }

}


/* Tooltip display callback.  */
static gboolean
uid_list_query_tooltip_cb (GtkWidget *widget, int x, int y,
                           gboolean keyboard_tip,
                           GtkTooltip *tooltip, gpointer user_data)
{
  GtkTreeView *tv = GTK_TREE_VIEW (widget);
  GtkTreeViewColumn *column;
  char *text;

  (void)user_data;

  if (!gtk_tree_view_get_tooltip_context (tv, &x, &y, keyboard_tip,
                                          NULL, NULL, NULL))
    return FALSE; /* Not at a row - do not show a tooltip.  */
  if (!gtk_tree_view_get_path_at_pos (tv, x, y, NULL, &column, NULL, NULL))
    return FALSE;

  widget = gtk_tree_view_column_get_widget (column);
  text = widget? gtk_widget_get_tooltip_text (widget) : NULL;
  if (!text)
    return FALSE; /* No tooltip desired.  */

  gtk_tooltip_set_text (tooltip, text);
  g_free (text);

  return TRUE; /* Show tooltip.  */
}
