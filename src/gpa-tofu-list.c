/* gpa-tofu-list.c - A list to show TOFU information.
 * Copyright (C) 2016 g10 Code GmbH
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
#include "gpa-tofu-list.h"

#ifdef ENABLE_TOFU_INFO

static gboolean tofu_list_query_tooltip_cb (GtkWidget *wdiget, int x, int y,
                                            gboolean keyboard_mode,
                                            GtkTooltip *tooltip,
                                            gpointer user_data);



typedef enum
{
  TOFU_ADDRESS,
  TOFU_VALIDITY,
  TOFU_POLICY,
  TOFU_COUNT,
  TOFU_FIRSTSIGN,
  TOFU_LASTSIGN,
  TOFU_FIRSTENCR,
  TOFU_LASTENCR,
  TOFU_N_COLUMNS
} SubkeyListColumn;


/* Create a new subkey list.  */
GtkWidget *
gpa_tofu_list_new (void)
{
  GtkListStore *store;
  GtkWidget *list;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  /* Init the model */
  store = gtk_list_store_new (TOFU_N_COLUMNS,
			      G_TYPE_STRING,  /* address */
			      G_TYPE_STRING,  /* validity */
			      G_TYPE_STRING,  /* policy */
			      G_TYPE_STRING,  /* count */
			      G_TYPE_STRING,  /* firstsign */
			      G_TYPE_STRING,  /* lastsign */
			      G_TYPE_STRING,  /* firstencr */
			      G_TYPE_STRING   /* lastencr */
                              );

  /* The view */
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);

  /* Add the columns */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", TOFU_ADDRESS,
						     NULL);
  gpa_set_column_title (column, _("Address"),
                        _("The mail address."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", TOFU_VALIDITY,
						     NULL);
  gpa_set_column_title (column, _("Validity"),
                        _("The TOFU validity of the mail address:\n"
                          " Minimal = Only little history available\n"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", TOFU_POLICY,
						     NULL);
  gpa_set_column_title (column, _("Policy"),
                        _("The TOFU policy set for this mail address."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", TOFU_COUNT,
						     NULL);
  gpa_set_column_title (column, _("Count"),
                        _("The number of signatures seen for this address\n"
                          "and the number of encryption done to this address.")
                        );
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", TOFU_FIRSTSIGN,
						     NULL);
  gpa_set_column_title (column, _("First Sig"),
                        _("The date the first signature was verified."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", TOFU_LASTSIGN,
						     NULL);
  gpa_set_column_title (column, _("Last Sig"),
                        _("The most recent date a signature was verified."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", TOFU_FIRSTENCR,
						     NULL);
  gpa_set_column_title (column, _("First Enc"),
                        _("The date the first encrypted mail was sent."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						     "text", TOFU_LASTENCR,
						     NULL);
  gpa_set_column_title (column, _("Last Enc"),
                        _("The most recent date an encrypted mail was sent."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  g_object_set (list, "has-tooltip", TRUE, NULL);
  g_signal_connect (list, "query-tooltip",
                    G_CALLBACK (tofu_list_query_tooltip_cb), list);

  return list;
}


static const gchar *
tofu_validity_str (gpgme_tofu_info_t tofu)
{
  switch (tofu->validity)
    {
    case 0: return _("Conflict");
    case 1: return _("Unknown");
    case 2: return _("Minimal");
    case 3: return _("Basic");
    case 4: return _("Full");
    default: return "?";
    }
}


static const gchar *
tofu_policy_str (gpgme_tofu_info_t tofu)
{
  switch (tofu->policy)
    {
    case GPGME_TOFU_POLICY_NONE:    return _("None");
    case GPGME_TOFU_POLICY_AUTO:    return _("Auto");
    case GPGME_TOFU_POLICY_GOOD:    return _("Good");
    case GPGME_TOFU_POLICY_UNKNOWN: return _("Unknown");
    case GPGME_TOFU_POLICY_BAD:     return _("Bad");
    case GPGME_TOFU_POLICY_ASK:     return _("Ask");
    }
  return "?";
}


/* Set the key whose subkeys should be displayed. */
void
gpa_tofu_list_set_key (GtkWidget *list, gpgme_key_t key)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  GtkTreeIter iter;
  gpgme_user_id_t uid;
  gpgme_tofu_info_t tofu;
  char *countstr, *firstsign, *lastsign, *firstencr, *lastencr;

  /* Empty the list */
  gtk_list_store_clear (store);

  if (!key || !key->uids)
    return;

  for (uid = key->uids; uid; uid = uid->next)
    {
      if (!uid->address || !uid->tofu)
        continue;  /* No address or tofu info.  */
      tofu = uid->tofu;

      /* Note that we do not need to filter ADDRESS like we do with
       * user ids because GPGME checked that it is a valid mail
       * address.  */
      countstr = g_strdup_printf ("%hu/%hu", tofu->signcount, tofu->encrcount);
      firstsign = gpa_date_string (tofu->signfirst);
      lastsign  = gpa_date_string (tofu->signlast);
      firstencr = gpa_date_string (tofu->encrfirst);
      lastencr  = gpa_date_string (tofu->encrlast);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set
        (store, &iter,
         TOFU_ADDRESS,  uid->address,
         TOFU_VALIDITY, tofu_validity_str (tofu),
         TOFU_POLICY,   tofu_policy_str (tofu),
         TOFU_COUNT,    countstr,
         TOFU_FIRSTSIGN,firstsign,
         TOFU_LASTSIGN, lastsign,
         TOFU_FIRSTENCR,firstencr,
         TOFU_LASTENCR, lastencr,
         -1);

      g_free (countstr);
      g_free (firstsign);
      g_free (lastsign);
      g_free (firstencr);
      g_free (lastencr);
    }

}


/* Tooltip display callback.  */
static gboolean
tofu_list_query_tooltip_cb (GtkWidget *widget, int x, int y,
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
#endif /*ENABLE_TOFU_INFO*/
