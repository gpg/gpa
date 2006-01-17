/* keylist.c - The GNU Privacy Assistant keylist.
   Copyright (C) 2003 Miguel Coca.
   Copyright (C) 2005 g10 Code GmbH.

   This file is part of GPA

   GPA is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GPA; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA  */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "gpa.h"
#include "keylist.h"
#include "gpapastrings.h"
#include "gpawidgets.h"
#include "keytable.h"
#include "icons.h"

/* Internal */
static void add_trustdb_dialog (GpaKeyList * keylist);
static void remove_trustdb_dialog (GpaKeyList * keylist);

/* Callbacks */
static void gpa_keylist_next (gpgme_key_t key, gpointer data);
static void gpa_keylist_end (gpointer data);
static void gpa_keylist_clear_columns (GpaKeyList *keylist);

/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
};

/* GObject */

static GObjectClass *parent_class = NULL;

static void
gpa_keylist_get_property (GObject     *object,
			  guint        prop_id,
			  GValue      *value,
			  GParamSpec  *pspec)
{
  GpaKeyList *list = GPA_KEYLIST (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value, list->window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_keylist_set_property (GObject     *object,
			  guint        prop_id,
			  const GValue      *value,
			  GParamSpec  *pspec)
{
  GpaKeyList *list = GPA_KEYLIST (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      list->window = (GtkWidget*) g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_keylist_finalize (GObject *object)
{  
  GpaKeyList *list = GPA_KEYLIST (object);

  /* Dereference all keys in the list */
  g_list_foreach (list->keys, (GFunc) gpgme_key_unref, NULL);
  g_list_free (list->keys);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gpa_keylist_class_init (GpaKeyListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gpa_keylist_finalize;
  object_class->set_property = gpa_keylist_set_property;
  object_class->get_property = gpa_keylist_get_property;
}

typedef enum
{
  /* These are the displayed columns */
  GPA_KEYLIST_COLUMN_IMAGE,
  GPA_KEYLIST_COLUMN_KEYID,
  GPA_KEYLIST_COLUMN_EXPIRY,
  GPA_KEYLIST_COLUMN_OWNERTRUST,
  GPA_KEYLIST_COLUMN_VALIDITY,
  GPA_KEYLIST_COLUMN_USERID,
  /* This column contains the gpgme_key_t */
  GPA_KEYLIST_COLUMN_KEY,
  /* These columns are used only internally for sorting */
  GPA_KEYLIST_COLUMN_HAS_SECRET,
  GPA_KEYLIST_COLUMN_EXPIRY_TS,
  GPA_KEYLIST_COLUMN_OWNERTRUST_VALUE,
  GPA_KEYLIST_COLUMN_VALIDITY_VALUE,
  GPA_KEYLIST_N_COLUMNS
} GpaKeyListColumn;

static void
gpa_keylist_init (GpaKeyList *list)
{
  GtkListStore *store;
  GtkTreeSelection *selection = 
    gtk_tree_view_get_selection (GTK_TREE_VIEW (list));

  list->secret = FALSE;
  list->keys = NULL;
  list->dialog = NULL;
  list->timeout_id = 0;
  list->window = NULL;
  /* Init the model */
  store = gtk_list_store_new (GPA_KEYLIST_N_COLUMNS,
			      GDK_TYPE_PIXBUF,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_POINTER,
			      G_TYPE_INT,
			      G_TYPE_ULONG,
			      G_TYPE_ULONG,
			      G_TYPE_LONG);

  /* The view */
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
  gpa_keylist_set_brief (list);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  /* Load the keyring */
  add_trustdb_dialog (list);
  gpa_keytable_list_keys (gpa_keytable_get_public_instance(),
			  gpa_keylist_next, gpa_keylist_end, list);
}

GType
gpa_keylist_get_type (void)
{
  static GType keylist_type = 0;
  
  if (!keylist_type)
    {
      static const GTypeInfo keylist_info =
      {
        sizeof (GpaKeyListClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gpa_keylist_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaKeyList),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gpa_keylist_init,
      };
      
      keylist_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
						  "GpaKeyList",
						  &keylist_info, 0);
    }
  
  return keylist_type;
}

/* API */

GtkWidget *gpa_keylist_new (GtkWidget * window)
{
  GtkWidget *list = (GtkWidget*) g_object_new (GPA_KEYLIST_TYPE, NULL);

  return list;
}

void gpa_keylist_set_brief (GpaKeyList * keylist)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  gpa_keylist_clear_columns (keylist);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
						     "pixbuf",
						     GPA_KEYLIST_COLUMN_IMAGE,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_HAS_SECRET);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Key ID"), renderer,
						     "text",
						     GPA_KEYLIST_COLUMN_KEYID,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_KEYID);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("User Name"), renderer,
						     "text",
						     GPA_KEYLIST_COLUMN_USERID,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_USERID);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);
}

void gpa_keylist_set_detailed (GpaKeyList * keylist)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  gpa_keylist_clear_columns (keylist);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer,
						     "pixbuf",
						     GPA_KEYLIST_COLUMN_IMAGE,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_HAS_SECRET);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Key ID"), renderer,
						     "text",
						     GPA_KEYLIST_COLUMN_KEYID,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_KEYID);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Expiry Date"), 
						     renderer,
						     "text",
						     GPA_KEYLIST_COLUMN_EXPIRY,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_EXPIRY_TS);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Owner Trust"), 
						     renderer,
						     "text",
						     GPA_KEYLIST_COLUMN_OWNERTRUST,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_OWNERTRUST_VALUE);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Key Validity"),
						     renderer,
						     "text",
						     GPA_KEYLIST_COLUMN_VALIDITY,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_VALIDITY_VALUE);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("User Name"), renderer,
						     "text",
						     GPA_KEYLIST_COLUMN_USERID,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);  
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_USERID);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);
}

gboolean gpa_keylist_has_selection (GpaKeyList * keylist)
{
  GtkTreeSelection *selection = 
    gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
  return gtk_tree_selection_count_selected_rows (selection) > 0;
}

gboolean gpa_keylist_has_single_selection (GpaKeyList * keylist)
{
  GtkTreeSelection *selection = 
    gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
  return gtk_tree_selection_count_selected_rows (selection) == 1;
}

gboolean gpa_keylist_has_single_secret_selection (GpaKeyList * keylist)
{
  GtkTreeSelection *selection = 
    gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
  if (gtk_tree_selection_count_selected_rows (selection) == 1)
    {
      GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (keylist));
      GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
      gpgme_key_t key;
      GtkTreeIter iter;
      GtkTreePath *path = list->data;
      GValue value = {0,};

      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get_value (model, &iter, GPA_KEYLIST_COLUMN_KEY,
				&value);
      key = g_value_get_pointer (&value);
      g_value_unset(&value);      

      g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
      g_list_free (list);
      return (gpa_keytable_lookup_key (gpa_keytable_get_secret_instance(), 
				       key->subkeys->fpr) != NULL);
    }
  else
    {
      return FALSE;
    }
}

GList *gpa_keylist_get_selected_keys (GpaKeyList * keylist)
{
  GtkTreeSelection *selection = 
    gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (keylist));
  GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
  GList *keys = NULL;
  GList *cur;

  for (cur = list; cur; cur = g_list_next (cur))
    {
      gpgme_key_t key;
      GtkTreeIter iter;
      GtkTreePath *path = cur->data;
      GValue value = {0,};

      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get_value (model, &iter, GPA_KEYLIST_COLUMN_KEY,
				&value);
      key = g_value_get_pointer (&value);
      g_value_unset(&value);      

      keys = g_list_append (keys, key);
    }

  g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (list);
  
  return keys;
}

void gpa_keylist_start_reload (GpaKeyList * keylist)
{
  GtkTreeSelection *selection = 
    gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
  gtk_tree_selection_unselect_all (selection);
  gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model 
					(GTK_TREE_VIEW (keylist))));
  g_list_foreach (keylist->keys, (GFunc) gpgme_key_unref, NULL);
  g_list_free (keylist->keys);
  keylist->keys = NULL;
  add_trustdb_dialog (keylist);

  gpa_keytable_force_reload (gpa_keytable_get_public_instance (),
			     gpa_keylist_next, gpa_keylist_end, keylist);
}

/* Let the keylist know that a new key with the given fingerprint is
 * available.
 */
void gpa_keylist_new_key (GpaKeyList * keylist, const char *fpr)
{
  add_trustdb_dialog (keylist);
  gpa_keytable_load_new (gpa_keytable_get_secret_instance (), fpr,
			 NULL, (GpaKeyTableEndFunc) gtk_main_quit, NULL);
  /* Hack. Turn the asynchronous listing into a synchronous one */
  gtk_main ();
  remove_trustdb_dialog (keylist);
  /* The trustdb seems not to be updated for a --list-secret, so we
   * cdisplay the dialog both times, just in case */
  add_trustdb_dialog (keylist);
  gpa_keytable_load_new (gpa_keytable_get_public_instance (), fpr,
			 gpa_keylist_next, gpa_keylist_end, keylist);
}

/* Internal functions */

static gboolean
display_dialog (GpaKeyList * keylist)
{
  gtk_widget_show_all (keylist->dialog);

  keylist->timeout_id = 0;

  return FALSE;
}

static void
add_trustdb_dialog (GpaKeyList * keylist)
{
  /* Display this warning until the first key is received.
   * It may be shown at times when it's not needed. But it shouldn't appear
   * for long those times.
   */
  keylist->dialog = gtk_message_dialog_new (GTK_WINDOW (keylist->window),
					    GTK_DIALOG_MODAL, 
					    GTK_MESSAGE_INFO, 
					    GTK_BUTTONS_NONE,
					    _("GnuPG is rebuilding the trust "
					      "database.\nThis might take a "
					      "few seconds."));
  /* Wait a second before displaying the dialog. This avoids most 
   * "false alarms". */
  keylist->timeout_id = g_timeout_add (1000, (GSourceFunc) display_dialog, 
				       keylist);
}

static void
remove_trustdb_dialog (GpaKeyList * keylist)
{
  if (keylist->timeout_id)
    {
      g_source_remove (keylist->timeout_id);
      keylist->timeout_id = 0;
    }
  if (keylist->dialog)
    {
      gtk_widget_destroy (keylist->dialog);
      keylist->dialog = NULL;
    }
}

static GdkPixbuf*
get_key_pixbuf (gpgme_key_t key)
{
  static gboolean pixmaps_created = FALSE;
  static GdkPixbuf *secret_pixbuf = NULL;
  static GdkPixbuf *public_pixbuf = NULL;

  if (!pixmaps_created)
    {
      secret_pixbuf = gpa_create_icon_pixbuf ("blue_yellow_key");
      public_pixbuf = gpa_create_icon_pixbuf ("blue_key");
      pixmaps_created = TRUE;
    }

  if (gpa_keytable_lookup_key 
      (gpa_keytable_get_secret_instance(), 
       key->subkeys->fpr) != NULL)
    {
      return secret_pixbuf;
    }
  else
    {
      return public_pixbuf;
    }
}

static GtkWidget *gpa_keylist_elgamal_dialog (gpgme_key_t key)
{
  GtkWidget *window;
  GtkWidget *hbox, *vbox;
  GtkWidget *pixmap;
  GtkWidget *key_info;
  GtkWidget *label;
  
  window = gtk_dialog_new_with_buttons (_("GPA Warning"), NULL,
					GTK_DIALOG_MODAL,
					_("_Close"),
					GTK_RESPONSE_CLOSE,
					NULL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);
  gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_CLOSE);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (window)->vbox), hbox);

  pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
                                     GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), pixmap, TRUE, FALSE, 10);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, FALSE, 10);

  label = gtk_label_new (_("One of your secret keys contains an ElGamal "
			   "signing key. Due to a bug in GnuPG, all ElGamal "
			   "keys used with GnuPG 1.0.2 and later must be "
			   "considered compromised.\n\nPlease revoke your key "
			   "as soon as possible.\n\nThe affected key is:"));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 2);

  key_info = gpa_key_info_new (key);
  gtk_box_pack_start (GTK_BOX (vbox), key_info, TRUE, FALSE, 2);

  g_signal_connect (G_OBJECT (window), "response", 
		    G_CALLBACK (gtk_widget_destroy), NULL);

  return window;
}


/* For keys, gpg can't cope with, the fingerprint is set to all
   zero. This helper function returns true for such a FPR. */
static int
is_zero_fpr (const char *fpr)
{
  for (; *fpr; fpr++)
    if (*fpr != '0')
      return 0;
  return 1;
}

static void gpa_keylist_next (gpgme_key_t key, gpointer data)
{
  GpaKeyList *list = data;
  GtkListStore *store;
  GtkTreeIter iter;
  const gchar *keyid, *ownertrust, *validity;
  gchar *userid, *expiry;
  gboolean has_secret;
  long int val_value;

  /* Remove the dialog if it is being displayed */
  remove_trustdb_dialog (list);
  
  list->keys = g_list_append (list->keys, key);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list)));
  /* Get the column values */
  keyid = gpa_gpgme_key_get_short_keyid (key);
  expiry = gpa_expiry_date_string (key->subkeys->expires);
  ownertrust = gpa_key_ownertrust_string (key);
  validity = gpa_key_validity_string (key);
  userid = gpa_gpgme_key_get_userid (key->uids);
  has_secret = (!is_zero_fpr (key->subkeys->fpr)
                && gpa_keytable_lookup_key (gpa_keytable_get_secret_instance(),
                                            key->subkeys->fpr));

  /* Check for ElGamal signing keys and warn the user.  See
   * http://lists.gnupg.org/pipermail/gnupg-announce/2003q4/000276.html
   * for details. Fixme: We should remove this as current GnuPG
   * versions dropped all support for theses keys and revoking a key
   * is not possble anymore.
   */
  if (has_secret)
    {
      gpgme_subkey_t subkey;
      gboolean has_elg = FALSE;

      for (subkey = key->subkeys; subkey && !has_elg; subkey = subkey->next)
	{
	  if (subkey->pubkey_algo == GPGME_PK_ELG && 
	      !subkey->revoked && !subkey->expired)
	    {
	      has_elg = TRUE;
	    }
	}
      if (has_elg)
	{
	  GtkWidget *dialog = gpa_keylist_elgamal_dialog (key);
	  gtk_widget_show_all (dialog);
	}
    }
  /* Append the key to the list */
  gtk_list_store_append (store, &iter);

  /* Set an appropiate value for sorting revoked and expired keys. This 
   * includes a hack for forcing a value to a range outside the
   * usual validity values */
  if (key->subkeys->revoked)
      val_value = GPGME_VALIDITY_UNKNOWN-2;
  else if (key->subkeys->expired)
      val_value = GPGME_VALIDITY_UNKNOWN-1;
  else if (key->uids)
      val_value = key->uids->validity;
  else
      val_value = GPGME_VALIDITY_UNKNOWN;

  gtk_list_store_set (store, &iter,
		      GPA_KEYLIST_COLUMN_IMAGE, get_key_pixbuf (key),
		      GPA_KEYLIST_COLUMN_KEYID, keyid, 
		      GPA_KEYLIST_COLUMN_EXPIRY, expiry,
		      GPA_KEYLIST_COLUMN_OWNERTRUST, ownertrust,
		      GPA_KEYLIST_COLUMN_VALIDITY, validity,
		      GPA_KEYLIST_COLUMN_USERID, userid,
		      GPA_KEYLIST_COLUMN_KEY, key, 
		      GPA_KEYLIST_COLUMN_HAS_SECRET, has_secret,
		      /* Set "no expiration" to a large value for sorting */
		      GPA_KEYLIST_COLUMN_EXPIRY_TS, 
		      key->subkeys->expires ? 
		      key->subkeys->expires : G_MAXULONG,
		      GPA_KEYLIST_COLUMN_OWNERTRUST_VALUE, 
		      key->owner_trust,
		      /* Set revoked and expired keys to "never trust" for 
		       * sorting */
		      GPA_KEYLIST_COLUMN_VALIDITY_VALUE, val_value,
		      -1);
  /* Clean up */
  g_free (userid);
  g_free (expiry);
}

static void gpa_keylist_end (gpointer data)
{
  GpaKeyList *list = data;
  
  remove_trustdb_dialog (list);
}

static void gpa_keylist_clear_columns (GpaKeyList *keylist)
{
  GList *columns, *i;

  columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (keylist));
  for (i = columns; i; i = g_list_next (i))
    {
      gtk_tree_view_remove_column (GTK_TREE_VIEW (keylist),
                                   (GtkTreeViewColumn*) i->data);
    }
}
