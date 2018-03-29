/* keylist.c - The GNU Privacy Assistant keylist.
   Copyright (C) 2003 Miguel Coca.
   Copyright (C) 2005, 2008 g10 Code GmbH.

   This file is part of GPA

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <glib/gstdio.h>

#include "gpa.h"
#include "keylist.h"
#include "gpawidgets.h"
#include "convert.h"
#include "gtktools.h"
#include "keytable.h"
#include "icons.h"
#include "format-dn.h"


/* Properties */
enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_PUBLIC_ONLY,
  PROP_PROTOCOL,
  PROP_INITIAL_KEYS,
  PROP_INITIAL_PATTERN,
  PROP_REQUESTED_USAGE,
  PROP_ONLY_USABLE_KEYS
};

/* GObject */
static GObjectClass *parent_class = NULL;


/* Symbols to access the columns.  */
typedef enum
{
  /* These are the displayed columns */
  GPA_KEYLIST_COLUMN_IMAGE,
  GPA_KEYLIST_COLUMN_KEYTYPE,
  GPA_KEYLIST_COLUMN_CREATED,
  GPA_KEYLIST_COLUMN_EXPIRY,
  GPA_KEYLIST_COLUMN_OWNERTRUST,
  GPA_KEYLIST_COLUMN_VALIDITY,
  GPA_KEYLIST_COLUMN_USERID,
  /* This column contains the gpgme_key_t */
  GPA_KEYLIST_COLUMN_KEY,
  /* These columns are used only internally for sorting */
  GPA_KEYLIST_COLUMN_HAS_SECRET,
  GPA_KEYLIST_COLUMN_CREATED_TS,
  GPA_KEYLIST_COLUMN_EXPIRY_TS,
  GPA_KEYLIST_COLUMN_OWNERTRUST_VALUE,
  GPA_KEYLIST_COLUMN_VALIDITY_VALUE,
  GPA_KEYLIST_N_COLUMNS
} GpaKeyListColumn;



static void add_trustdb_dialog (GpaKeyList * keylist);
static void gpa_keylist_next (gpgme_key_t key, gpointer data);
static void gpa_keylist_end (gpointer data);



/************************************************************
 ******************  Object Management  *********************
 ************************************************************/

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
    case PROP_PUBLIC_ONLY:
      g_value_set_boolean (value, list->public_only);
      break;
    case PROP_PROTOCOL:
      g_value_set_int (value, list->protocol);
      break;
    case PROP_INITIAL_KEYS:
      g_value_set_pointer (value, list->initial_keys);
      break;
    case PROP_INITIAL_PATTERN:
      g_value_set_string (value, list->initial_pattern);
      break;
    case PROP_REQUESTED_USAGE:
      g_value_set_int (value, list->requested_usage);
      break;
    case PROP_ONLY_USABLE_KEYS:
      g_value_set_boolean (value, list->only_usable_keys);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gpa_keylist_set_property (GObject     *object,
			  guint        prop_id,
			  const GValue *value,
			  GParamSpec  *pspec)
{
  GpaKeyList *list = GPA_KEYLIST (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      list->window = (GtkWidget*) g_value_get_object (value);
      break;
    case PROP_PUBLIC_ONLY:
      list->public_only = g_value_get_boolean (value);
      break;
    case PROP_PROTOCOL:
      list->protocol = g_value_get_int (value);
      break;
    case PROP_INITIAL_KEYS:
      list->initial_keys = (gpgme_key_t*)g_value_get_pointer (value);
      break;
    case PROP_INITIAL_PATTERN:
      list->initial_pattern = g_value_get_string (value);
      break;
    case PROP_REQUESTED_USAGE:
      list->requested_usage = g_value_get_int (value);
      break;
    case PROP_ONLY_USABLE_KEYS:
      list->only_usable_keys = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
gpa_keylist_dispose (GObject *object)
{
  GpaKeyList *list = GPA_KEYLIST (object);

  list->disposed = 1;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
gpa_keylist_finalize (GObject *object)
{
  GpaKeyList *list = GPA_KEYLIST (object);

  /* Dereference all keys in the list */
  g_list_foreach (list->keys, (GFunc) gpgme_key_unref, NULL);
  g_list_free (list->keys);
  list->keys = NULL;
  gpa_gpgme_release_keyarray (list->initial_keys);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_keylist_init (GTypeInstance *instance, void *class_ptr)
{
  GpaKeyList *list = GPA_KEYLIST (instance);
  GtkListStore *store;
  GtkTreeSelection *selection;

  /* Setup the model.  */
  store = gtk_list_store_new (GPA_KEYLIST_N_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_POINTER,
			      G_TYPE_INT,
			      G_TYPE_ULONG,
			      G_TYPE_ULONG,
			      G_TYPE_ULONG,
			      G_TYPE_LONG);

  /* Setup the view.  */
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
  gpa_keylist_set_brief (list);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  /* Load the keyring.  */
  add_trustdb_dialog (list);
  if (list->initial_keys)
    {
      /* Initialize from the provided list.  */
      int idx;
      gpgme_key_t key;

      for (idx=0; (key = list->initial_keys[idx]); idx++)
        {
          gpgme_key_ref (key);
          gpa_keylist_next (key, list);
        }
      gpa_keylist_end (list);
    }
  else
    {
      /* Initialize from the global keytable.
       *
       * We must forcefully load the secret keytable first to
       * prevent concurrent access to the TOFU database. */
      gpa_keytable_force_reload (gpa_keytable_get_secret_instance (),
                                 NULL, (GpaKeyTableEndFunc) gtk_main_quit,
                                 NULL);
      gtk_main ();

      /* Now we can load the public keyring. */
      gpa_keytable_list_keys (gpa_keytable_get_public_instance(),
                              gpa_keylist_next, gpa_keylist_end, list);
    }

}



static void
gpa_keylist_class_init (void *class_ptr, void *class_data)
{
  GpaKeyListClass *klass = class_ptr;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose = gpa_keylist_dispose;
  object_class->finalize = gpa_keylist_finalize;
  object_class->set_property = gpa_keylist_set_property;
  object_class->get_property = gpa_keylist_get_property;

  g_object_class_install_property
    (object_class, PROP_PUBLIC_ONLY,
     g_param_spec_boolean
     ("public-only", "Public-only",
      "A flag indicating that we are only interested in public keys.",
      FALSE,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property
    (object_class, PROP_PROTOCOL,
     g_param_spec_int
     ("protocol", "Protocol",
      "The gpgme protocol used to restruct the key listing.",
      GPGME_PROTOCOL_OpenPGP, GPGME_PROTOCOL_UNKNOWN, GPGME_PROTOCOL_UNKNOWN,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property
    (object_class, PROP_INITIAL_KEYS,
     g_param_spec_pointer
     ("initial-keys", "Initial-keys",
      "An array of gpgme_key_t with the initial set of keys or NULL.",
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property
    (object_class, PROP_INITIAL_PATTERN,
     g_param_spec_string
     ("initial-pattern", "Initial-pattern",
      "A string with pattern to be used for a key search or NULL.",
      NULL,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property
    (object_class, PROP_REQUESTED_USAGE,
     g_param_spec_int
     ("requested-usage", "Requested-Key-Usage",
      "A bit vector describing the requested key usage (capabilities).",
      0, 65535, 0,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property
    (object_class, PROP_ONLY_USABLE_KEYS,
     g_param_spec_boolean
     ("only-usable-keys", "Only-usable-keys",
      "Include only usable keys in the listing.",
      FALSE,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));


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
        gpa_keylist_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GpaKeyList),
        0,              /* n_preallocs */
        gpa_keylist_init,
      };

      keylist_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
                                             "GpaKeyList",
                                             &keylist_info, 0);
    }

  return keylist_type;
}



/************************************************************
 ******************  Internal Functions  ********************
 ************************************************************/

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
  /* Display this warning until the first key is received.  It may be
     shown at times when it's not needed. But it shouldn't appear for
     long those times.  */
  keylist->dialog = gtk_message_dialog_new
    (GTK_WINDOW (keylist->window), GTK_DIALOG_MODAL,
     GTK_MESSAGE_INFO, GTK_BUTTONS_NONE,
     _("GnuPG is rebuilding the trust database.\n"
       "This might take a few seconds."));

  /* Wait a second before displaying the dialog. This avoids most
     "false alarms".  That is the message will not be shown if GnuPG
     does not run a long check trustdb.  */
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


static const gchar *
get_key_pixbuf (gpgme_key_t key)
{
  gpgme_key_t seckey;

  seckey = gpa_keytable_lookup_key (gpa_keytable_get_secret_instance (),
                                    key->subkeys->fpr);
  if (seckey)
    {
      if (seckey->subkeys && seckey->subkeys->is_cardkey)
        return GPA_STOCK_SECRET_CARDKEY;
      return GPA_STOCK_SECRET_KEY;
    }
  else
    return GPA_STOCK_PUBLIC_KEY;
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


/* Note that this function takes ownership of KEY.  */
static void
gpa_keylist_next (gpgme_key_t key, gpointer data)
{
  GpaKeyList *list = data;
  GtkListStore *store;
  GtkTreeIter iter;
  const gchar *ownertrust, *validity;
  gchar *userid, *created, *expiry;
  gboolean has_secret;
  long int val_value;
  const char *keytype;

  /* Remove the dialog if it is being displayed */
  remove_trustdb_dialog (list);

  if (list->disposed)
    return;  /* Should not access our store anymore.  */

  /* Filter out keys we don't want.  */
  if (key && list->protocol != GPGME_PROTOCOL_UNKNOWN
      && key->protocol != list->protocol)
    {
      gpgme_key_unref (key);
      return;
    }

  if (key && list->requested_usage)
    {
      if ((key->can_sign && list->requested_usage & KEY_USAGE_SIGN))
        ;
      else if ((key->can_encrypt && list->requested_usage & KEY_USAGE_ENCR))
        ;
      else if ((key->can_certify && list->requested_usage & KEY_USAGE_CERT))
        ;
      else
        {
          gpgme_key_unref (key);
          return;
        }
    }

  if (key && list->only_usable_keys
      && (key->revoked || key->disabled || key->expired || key->invalid))
    {
      gpgme_key_unref (key);
      return;
    }

  /* Append the key to the list.  */
  list->keys = g_list_append (list->keys, key);
  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list)));
  /* Get the column values */
  keytype = (key->protocol == GPGME_PROTOCOL_OpenPGP? "P" :
             key->protocol == GPGME_PROTOCOL_CMS? "X" : "?");
  created = gpa_creation_date_string (key->subkeys->timestamp);
  expiry = gpa_expiry_date_string (key->subkeys->expires);
  ownertrust = gpa_key_ownertrust_string (key);
  validity = gpa_key_validity_string (key);
  if (key->protocol == GPGME_PROTOCOL_CMS)
    userid = gpa_format_dn (key->uids? key->uids->uid : NULL);
  else
    userid = gpa_gpgme_key_get_userid (key->uids);
  if (list->public_only)
    has_secret = 0;
  else
    has_secret = (!is_zero_fpr (key->subkeys->fpr)
                  && gpa_keytable_lookup_key
                  (gpa_keytable_get_secret_instance(), key->subkeys->fpr));

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
		      GPA_KEYLIST_COLUMN_KEYTYPE, keytype,
		      GPA_KEYLIST_COLUMN_CREATED, created,
		      GPA_KEYLIST_COLUMN_EXPIRY, expiry,
		      GPA_KEYLIST_COLUMN_OWNERTRUST, ownertrust,
		      GPA_KEYLIST_COLUMN_VALIDITY, validity,
		      GPA_KEYLIST_COLUMN_USERID, userid,
		      GPA_KEYLIST_COLUMN_KEY, key,
		      GPA_KEYLIST_COLUMN_HAS_SECRET, has_secret,
		      GPA_KEYLIST_COLUMN_CREATED_TS, key->subkeys->timestamp,

		      /* Set "no expiration" to a large value for sorting */
		      GPA_KEYLIST_COLUMN_EXPIRY_TS,
		      key->subkeys->expires ?
		      key->subkeys->expires : G_MAXULONG,

		      GPA_KEYLIST_COLUMN_OWNERTRUST_VALUE,
		      key->owner_trust,
		      /* Set revoked and expired keys to "never trust"
		         for sorting.  */
		      GPA_KEYLIST_COLUMN_VALIDITY_VALUE, val_value,
                      /* Store the image only if enabled.  */
		      list->public_only ? -1 : GPA_KEYLIST_COLUMN_IMAGE,
                      list->public_only ? NULL : get_key_pixbuf (key),
		      -1);
  /* Clean up */
  g_free (userid);
  g_free (created);
  g_free (expiry);
}


static void
gpa_keylist_end (gpointer data)
{
  GpaKeyList *list = data;

  remove_trustdb_dialog (list);
}


static void
gpa_keylist_clear_columns (GpaKeyList *keylist)
{
  GList *columns, *i;

  columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (keylist));
  for (i = columns; i; i = g_list_next (i))
    {
      gtk_tree_view_remove_column (GTK_TREE_VIEW (keylist),
                                   (GtkTreeViewColumn*) i->data);
    }
}


gboolean
search_keylist_function (GtkTreeModel *model, gint column,
                         const gchar *key_to_search_for, GtkTreeIter *iter,
                         gpointer search_data)
{
  gboolean result = TRUE;
  gchar *user_id;
  gint search_len;

  gtk_tree_model_get (model, iter,
                      GPA_KEYLIST_COLUMN_USERID, &user_id, -1);

  search_len = strlen (key_to_search_for);

  if (!g_ascii_strncasecmp (user_id, key_to_search_for, search_len))
    result=FALSE;

  g_free (user_id);

  return result;
}


static void
setup_columns (GpaKeyList *keylist, gboolean detailed)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  gpa_keylist_clear_columns (keylist);

  if (!keylist->public_only)
    {
      renderer = gtk_cell_renderer_pixbuf_new ();
      /* Large toolbar size is 24x24, which is large enough to
	 accomodate the key icons.  Note that those are at fixed size
	 and thus no scaling or padding is done (see icons.c).  */
      g_object_set (renderer, "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);

      column = gtk_tree_view_column_new_with_attributes
        (NULL, renderer, "stock-id",
         GPA_KEYLIST_COLUMN_IMAGE,
         NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
      gtk_tree_view_column_set_sort_column_id
        (column, GPA_KEYLIST_COLUMN_HAS_SECRET);
      gtk_tree_view_column_set_sort_indicator (column, TRUE);
    }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "text", GPA_KEYLIST_COLUMN_KEYTYPE, NULL);
  gpa_set_column_title
    (column, " ",
     _("This columns lists the type of the certificate."
       "  A 'P' denotes OpenPGP and a 'X' denotes X.509 (S/MIME)."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "text", GPA_KEYLIST_COLUMN_CREATED, NULL);
  gpa_set_column_title
    (column, _("Created"),
     _("The Creation Date is the date the certificate was created."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id
    (column, GPA_KEYLIST_COLUMN_CREATED_TS);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  if (detailed)
    {
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes
        (NULL, renderer, "text", GPA_KEYLIST_COLUMN_EXPIRY, NULL);
      gpa_set_column_title
        (column, _("Expiry Date"),
         _("The Expiry Date is the date until the certificate is valid."));
      gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
      gtk_tree_view_column_set_sort_column_id
        (column, GPA_KEYLIST_COLUMN_EXPIRY_TS);
      gtk_tree_view_column_set_sort_indicator (column, TRUE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes
        (NULL, renderer, "text", GPA_KEYLIST_COLUMN_OWNERTRUST, NULL);
      gpa_set_column_title
        (column, _("Owner Trust"),
         _("The Owner Trust has been set by you and describes how far you"
           " trust the holder of the certificate to correctly sign (certify)"
           " other certificates.  It is only meaningful for OpenPGP."));
      gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
      gtk_tree_view_column_set_sort_column_id
        (column, GPA_KEYLIST_COLUMN_OWNERTRUST_VALUE);
      gtk_tree_view_column_set_sort_indicator (column, TRUE);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes
        (NULL, renderer, "text", GPA_KEYLIST_COLUMN_VALIDITY, NULL);
      gpa_set_column_title
        (column, _("Validity"),
         _("The Validity describes the trust level the system has"
           " in this certificate.  That is how sure it is that the named"
           " user is actually that user."));
      gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
      gtk_tree_view_column_set_sort_column_id
        (column, GPA_KEYLIST_COLUMN_VALIDITY_VALUE);
      gtk_tree_view_column_set_sort_indicator (column, TRUE);
    }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "text", GPA_KEYLIST_COLUMN_USERID, NULL);
  gpa_set_column_title
    (column, _("User Name"),
     _("The User Name is the name and often also the email address "
       " of the certificate."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (keylist), column);
  gtk_tree_view_column_set_sort_column_id (column, GPA_KEYLIST_COLUMN_USERID);
  gtk_tree_view_column_set_sort_indicator (column, TRUE);

  gtk_tree_view_set_enable_search (GTK_TREE_VIEW(keylist), TRUE);
  gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW(keylist),
                                       search_keylist_function, NULL, NULL);
}



/************************************************************
 **********************  Public API  ************************
 ************************************************************/

/* Create a new key list widget.  */
GtkWidget *
gpa_keylist_new (GtkWidget *window)
{
  GtkWidget *list = (GtkWidget*) g_object_new (GPA_KEYLIST_TYPE, NULL);

  return list;
}


/* Create a new key list widget.  If PUBLIC_ONLY is set the keylist
   will be created in public_only mode.  PROTOCOL may be used to
   resctrict the list to keys of a certain protocol. If KEYS is not
   NULL, those keys will be displayed instead of listing all.  If
   PATTERN is not NULL, the search box will be filled with that
   pattern.  If REQUESTED_USAGE is not 0 only keys with the given
   usages are listed.  */
GpaKeyList *
gpa_keylist_new_with_keys (GtkWidget *window, gboolean public_only,
                           gpgme_protocol_t protocol,
                           gpgme_key_t *keys, const char *pattern,
                           int requested_usage, gboolean only_usable_keys)
{
  GpaKeyList *list;

  list = g_object_new (GPA_KEYLIST_TYPE,
                       "public-only", public_only,
                       "protocol", (int)protocol,
                       "initial-keys", gpa_gpgme_copy_keyarray (keys),
                       "initial-pattern", pattern,
                       "requested-usage", requested_usage,
                       "only-usable-keys", only_usable_keys,
                       NULL);

  return list;
}



/* Set the key list in "brief" mode.  */
void
gpa_keylist_set_brief (GpaKeyList *keylist)
{
  setup_columns (keylist, FALSE);
}


/* Set the key list in "detailed" mode.  */
void
gpa_keylist_set_detailed (GpaKeyList * keylist)
{
  setup_columns (keylist, TRUE);
}


/* Return true if any key is selected in the list.  */
gboolean
gpa_keylist_has_selection (GpaKeyList * keylist)
{
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
  return gtk_tree_selection_count_selected_rows (selection) > 0;
}


/* Return true if one, and only one, key is selected in the list.  */
gboolean
gpa_keylist_has_single_selection (GpaKeyList * keylist)
{
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
  return gtk_tree_selection_count_selected_rows (selection) == 1;
}


/* Return true if one, and only one, secret key is selected in the list.  */
gboolean
gpa_keylist_has_single_secret_selection (GpaKeyList *keylist)
{
  GtkTreeSelection *selection;

  if (keylist->public_only)
    return FALSE;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
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


/* Return a GList of selected keys. The caller must not dereference
   the keys as they belong to the caller.  Unless PROTOCOL is
   GPGME_PROTOCOL_UNKNOWN, only keys matching thhe requested protocol
   are returned.  */
GList *
gpa_keylist_get_selected_keys (GpaKeyList * keylist, gpgme_protocol_t protocol)
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

      /* Fixme: Why don't we ref KEY? */
      if (key && (protocol == GPGME_PROTOCOL_UNKNOWN
                  || protocol == key->protocol))
        keys = g_list_append (keys, key);
    }

  g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (list);

  return keys;
}


/* Return the selected key.  This function returns NULL if no or more
   than one key has been selected.  */
gpgme_key_t
gpa_keylist_get_selected_key (GpaKeyList *keylist)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GList *list;
  GtkTreePath *path;
  GtkTreeIter iter;
  GValue value = {0};
  gpgme_key_t key;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (keylist));
  if (gtk_tree_selection_count_selected_rows (selection) != 1)
    return NULL;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (keylist));
  list = gtk_tree_selection_get_selected_rows (selection, &model);
  if (!list)
    return NULL;
  path = list->data;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get_value (model, &iter, GPA_KEYLIST_COLUMN_KEY, &value);
  key = g_value_get_pointer (&value);
  g_value_unset (&value);
  gpgme_key_ref (key);

  g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (list);

  return key;
}


/* Begin a reload of the keyring. */
void
gpa_keylist_start_reload (GpaKeyList * keylist)
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
   available.  */
void
gpa_keylist_new_key (GpaKeyList * keylist, const char *fpr)
{
  /* FIXME: I don't understand the code.  Investigate this and
     implement public_only.  */

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


/* Let the keylist know that a new sceret key has been imported. */
void
gpa_keylist_imported_secret_key (GpaKeyList *keylist)
{
  /* KEYLIST is currently not used. */

  gpa_keytable_load_new (gpa_keytable_get_secret_instance (), NULL,
			 NULL, (GpaKeyTableEndFunc) gtk_main_quit, NULL);
  /* Hack. Turn the asynchronous listing into a synchronous one */
  /* FIXME:  Does that work well with the server shutdown code? */
  gtk_main ();
}


