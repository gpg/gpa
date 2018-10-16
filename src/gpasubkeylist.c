/* gpasubkeylist.c  -	 The GNU Privacy Assistant
 *	Copyright (C) 2003 Miguel Coca.
 *      Copyright (C) 2009 g10 Code GmbH.
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "gpa.h"
#include "convert.h"
#include "gtktools.h"
#include "keytable.h"
#include "gpasubkeylist.h"


static gboolean query_tooltip_cb (GtkWidget *wdiget, int x, int y,
                                  gboolean keyboard_mode,
                                  GtkTooltip *tooltip, gpointer user_data);



/*
 *  Implement a List showing the subkeys in a given key
 */

typedef enum
{
  SUBKEY_ID,
  SUBKEY_ALGO,
  SUBKEY_CREATED,
  SUBKEY_EXPIRE,
  SUBKEY_CAN_SIGN,
  SUBKEY_CAN_CERTIFY,
  SUBKEY_CAN_ENCRYPT,
  SUBKEY_CAN_AUTHENTICATE,
  SUBKEY_IS_CARDKEY,
  SUBKEY_CARD_NUMBER,
  SUBKEY_STATUS,
  SUBKEY_N_COLUMNS
} SubkeyListColumn;


/* Create a new subkey list.  */
GtkWidget *
gpa_subkey_list_new (void)
{
  GtkListStore *store;
  GtkWidget *list;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  /* Init the model */
  store = gtk_list_store_new (SUBKEY_N_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_STRING,
			      G_TYPE_STRING);

  /* The view */
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);

  /* Add the columns */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Subkey ID"), renderer,
						     "text", SUBKEY_ID,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Status"), renderer,
						     "text", SUBKEY_STATUS,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Algo"),
						     renderer,
						     "text", SUBKEY_ALGO,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Created"),
						     renderer,
						     "text", SUBKEY_CREATED,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Expires"),
						     renderer,
						     "text", SUBKEY_EXPIRE,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", SUBKEY_CAN_SIGN, NULL);
  gpa_set_column_title (column, _("S"), _("Can sign"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", SUBKEY_CAN_CERTIFY, NULL);
  gpa_set_column_title (column, _("C"), _("Can certify"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", SUBKEY_CAN_ENCRYPT, NULL);
  gpa_set_column_title (column, _("E"), _("Can encrypt"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", SUBKEY_CAN_AUTHENTICATE, NULL);
  gpa_set_column_title (column, _("A"), _("Can authenticate"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", SUBKEY_IS_CARDKEY, NULL);
  gpa_set_column_title (column, _("T"),
                        _("Secret key stored on a smartcard."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "text", SUBKEY_CARD_NUMBER, NULL);
  gpa_set_column_title (column, _("Card S/N"),
                        _("Serial number of the smart card."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  g_object_set (list, "has-tooltip", TRUE, NULL);
  g_signal_connect (list, "query-tooltip",
                    G_CALLBACK (query_tooltip_cb), list);

  return list;
}


const gchar *
subkey_status (gpgme_subkey_t subkey)
{
  if (subkey->revoked)
    {
      return _("Revoked");
    }
  else if (subkey->expired)
    {
      return _("Expired");
    }
  else if (subkey->disabled)
    {
      return _("Disabled");
    }
  else if (subkey->invalid)
    {
      return _("Unsigned");
    }
  else
    {
      return _("Valid");
    }
}


/* Set the key whose subkeys should be displayed. */
void
gpa_subkey_list_set_key (GtkWidget *list, gpgme_key_t key)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model
                                        (GTK_TREE_VIEW (list)));
  gpgme_subkey_t subkey, secsubkey;
  gpgme_key_t seckey;
  gchar *p, *size, *expires;
  const char *algostr;

  /* Empty the list */
  gtk_list_store_clear (store);

  if (key)
    {
      seckey = gpa_keytable_lookup_key
        (gpa_keytable_get_secret_instance (), key->subkeys->fpr);

      /* Add all the subkeys */
      for (subkey = key->subkeys; subkey; subkey = subkey->next)
	{
	  GtkTreeIter iter;

          if (seckey)
            {
              for (secsubkey = seckey->subkeys; secsubkey;
                   secsubkey = secsubkey->next)
                if (subkey->fpr && secsubkey->fpr
                    && g_str_equal (subkey->fpr, secsubkey->fpr))
                  break;
            }
          else
            secsubkey = NULL;

	  /* Append */
	  gtk_list_store_append (store, &iter);
          if (subkey->curve)
            size = g_strdup (subkey->curve);
          else
            size = g_strdup_printf ("%i bits", subkey->length);

          /* We only want "never" and not "never expires" but we need
             to take care of the ">= 2038" string.  */
          expires = gpa_expiry_date_string (subkey->expires);
          if (*expires != '>' && (p = strchr (expires, ' ')))
            *p = 0;

          algostr = gpgme_pubkey_algo_name (subkey->pubkey_algo),

          gtk_list_store_set
            (store, &iter,
             SUBKEY_ID, subkey->keyid+8,
             SUBKEY_ALGO,
             algostr,
             SUBKEY_CREATED,
             gpa_creation_date_string (subkey->timestamp),
             SUBKEY_EXPIRE,
             expires,
             SUBKEY_CAN_SIGN, subkey->can_sign,
             SUBKEY_CAN_CERTIFY, subkey->can_certify,
             SUBKEY_CAN_ENCRYPT, subkey->can_encrypt,
             SUBKEY_CAN_AUTHENTICATE, subkey->can_authenticate,
             SUBKEY_IS_CARDKEY, secsubkey? secsubkey->is_cardkey :0,
             SUBKEY_CARD_NUMBER, secsubkey? secsubkey->card_number:NULL,
             SUBKEY_STATUS, subkey_status (subkey),
             -1);
	  g_free (size);
	  g_free (expires);
	}
    }
}


/* Tooltip display callback.  */
static gboolean
query_tooltip_cb (GtkWidget *widget, int x, int y, gboolean keyboard_tip,
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
