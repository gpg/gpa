/* gpa-key-details.c  -  A widget to show key details.
 * Copyright (C) 2000, 2001 G-N-U GmbH.
 * Copyright (C) 2005, 2008, 2009 g10 Code GmbH.
 *
 * This file is part of GPA.
 *
 * GPA is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* 
   This widget is used to display details of a key.

 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpa.h"   
#include "convert.h"
#include "keytable.h"
#include "siglist.h"
#include "gpasubkeylist.h"
#include "gpa-key-details.h"



/* Object's class definition.  */
struct _GpaKeyDetailsClass 
{
  GtkNotebookClass parent_class;
};


/* Object definition.  */
struct _GpaKeyDetails
{
  GtkNotebook parent_instance;

  /* Widgets in the details page.  */
  GtkWidget *details_num_label;
  GtkWidget *details_table;
  GtkWidget *detail_public_private;
  GtkWidget *detail_capabilities;
  GtkWidget *detail_name;
  GtkWidget *detail_fingerprint;
  GtkWidget *detail_expiry;
  GtkWidget *detail_key_id;
  GtkWidget *detail_owner_trust;
  GtkWidget *detail_key_trust;
  GtkWidget *detail_key_type;
  GtkWidget *detail_creation;

  /* The widgets in the signatures page.  */
  GtkWidget *signatures_list;
  GtkWidget *signatures_uids;
  gint signatures_count;
  GtkWidget *signatures_hbox;

  /* The widgets in the subkeys list.  */
  GtkWidget *subkeys_page;
  GtkWidget *subkeys_list;
};


/* The parent class.  */
static GObjectClass *parent_class;



/* Local prototypes */
static void gpa_key_details_finalize (GObject *object);



/************************************************************ 
 *******************   Implementation   *********************
 ************************************************************/


/* Callback for the "changed" signal to update the popdown menu on the
   signatures page of the notebook.  */
static void
signatures_uid_changed (GtkComboBox *combo, gpointer user_data)
{
  GpaKeyDetails *kdt = user_data;
  gpgme_key_t key;

  /* FIXME  key = keyring_editor_current_key (kdt);*/ key = NULL;
  /* Note that we need to subtract one, as the first entry (with index
     0 means) "all user names".  */
  gpa_siglist_set_signatures 
    (kdt->signatures_list, key,
     gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) - 1);
}


/* Fill the details page with the properties of the public key.  */
static void
details_page_fill_key (GpaKeyDetails *kdt, gpgme_key_t key)
{
  gpgme_user_id_t uid;
  gpgme_key_t seckey;
  char *text;

  seckey = gpa_keytable_lookup_key (gpa_keytable_get_secret_instance(), 
                                    key->subkeys->fpr);
  if (seckey)
    {
#ifdef HAVE_STRUCT__GPGME_SUBKEY_CARD_NUMBER
      if (seckey->subkeys && seckey->subkeys->is_cardkey)
        gtk_label_set_text (GTK_LABEL (kdt->detail_public_private),
                            _("The key has both a smartcard based private part"
                              " and a public part"));
      else
#endif
        gtk_label_set_text (GTK_LABEL (kdt->detail_public_private),
                            _("The key has both a private and a public part"));
    }
  else
    gtk_label_set_text (GTK_LABEL (kdt->detail_public_private),
			_("The key has only a public part"));

  gtk_label_set_text (GTK_LABEL (kdt->detail_capabilities),
		      gpa_get_key_capabilities_text (key));

  /* One user ID per line.  */
  text = gpa_gpgme_key_get_userid (key->uids);
  if (key->uids)
    {
      for (uid = key->uids->next; uid; uid = uid->next)
        {
          gchar *uid_string = gpa_gpgme_key_get_userid (uid);
          gchar *tmp = text;

          text = g_strconcat (text, "\n", uid_string, NULL);
          g_free (tmp);
          g_free (uid_string);
        }
    }
  gtk_label_set_text (GTK_LABEL (kdt->detail_name), text);
  g_free (text);

  text = (gchar*) gpa_gpgme_key_get_short_keyid (key);
  gtk_label_set_text (GTK_LABEL (kdt->detail_key_id), text);

  text = gpa_gpgme_key_format_fingerprint (key->subkeys->fpr);
  gtk_label_set_text (GTK_LABEL (kdt->detail_fingerprint), text);
  g_free (text);

  text = gpa_expiry_date_string (key->subkeys->expires);
  gtk_label_set_text (GTK_LABEL (kdt->detail_expiry), text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (kdt->detail_key_trust),
                      gpa_key_validity_string (key));

  text = g_strdup_printf (_("%s %u bits"),
			  gpgme_pubkey_algo_name (key->subkeys->pubkey_algo),
			  key->subkeys->length);
  gtk_label_set_text (GTK_LABEL (kdt->detail_key_type), text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (kdt->detail_owner_trust),
                      gpa_key_ownertrust_string (key));

  text = gpa_creation_date_string (key->subkeys->timestamp);
  gtk_label_set_text (GTK_LABEL (kdt->detail_creation), text);
  g_free (text);

  gtk_widget_hide_all (kdt->details_num_label);
  gtk_widget_show_all (kdt->details_table);
  gtk_widget_set_no_show_all (kdt->details_num_label, TRUE);
  gtk_widget_set_no_show_all (kdt->details_table, FALSE);
}


/* Fill the details page with the number of selected keys.  */
static void
details_page_fill_num_keys (GpaKeyDetails *kdt, gint num_key)
{
  if (!num_key)
    gtk_label_set_text (GTK_LABEL (kdt->details_num_label),
			_("No keys selected"));
  else
    {
      char *text = g_strdup_printf (ngettext("%d key selected",
                                             "%d keys selected",
                                             num_key), num_key); 

      gtk_label_set_text (GTK_LABEL (kdt->details_num_label), text);
      g_free (text);
    }
  
  gtk_widget_show_all (kdt->details_num_label);
  gtk_widget_hide_all (kdt->details_table);
  gtk_widget_set_no_show_all (kdt->details_num_label, FALSE);
  gtk_widget_set_no_show_all (kdt->details_table, TRUE);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (kdt), 0);
}



/* Fill the signatures page with the signatures of the public key.  */
static void
signatures_page_fill_key (GpaKeyDetails *kdt, gpgme_key_t key)
{
  GtkComboBox *combo;
  int i;

  combo = GTK_COMBO_BOX (kdt->signatures_uids);
  for (i = kdt->signatures_count - 1; i >= 0; i--)
    gtk_combo_box_remove_text (combo, i);
  kdt->signatures_count = 0;

  /* Populate the popdown UID list if there is more than one UID.  */
  if (key->uids && key->uids->next)
    {
      gpgme_user_id_t uid;

      gtk_combo_box_append_text (combo, _("All signatures"));
      gtk_combo_box_set_active (combo, 0);
      for (i=1, uid = key->uids; uid; i++, uid = uid->next)
	{
	  gchar *uid_string = gpa_gpgme_key_get_userid (uid);
	  gtk_combo_box_append_text (combo, uid_string);
	  g_free (uid_string);
	}
      kdt->signatures_count = i;

      gtk_widget_show (kdt->signatures_hbox);
      gtk_widget_set_no_show_all (kdt->signatures_hbox, FALSE);
      gtk_widget_set_sensitive (kdt->signatures_uids, TRUE);
      /* Add the signatures.  */
      gpa_siglist_set_signatures (kdt->signatures_list, key, -1);
    }
  else
    {
      /* If there is just one uid, display its signatures explicitly,
         and do not show the list of uids.  */
      gtk_widget_hide (kdt->signatures_hbox);
      gtk_widget_set_no_show_all (kdt->signatures_hbox, TRUE);
      gpa_siglist_set_signatures (kdt->signatures_list, key, 0);
    }
}


/* Empty the list of signatures in the details notebook.  */
static void
signatures_page_empty (GpaKeyDetails *kdt)
{
  GtkComboBox *combo;
  gint i;

  gpa_siglist_set_signatures (kdt->signatures_list, NULL, 0);

  combo = GTK_COMBO_BOX (kdt->signatures_uids);
  gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
  for (i = kdt->signatures_count - 1; i >= 0; i--)
    gtk_combo_box_remove_text (combo, i);
  kdt->signatures_count = 0;
}


/* Fill the subkeys page.  */
static void
subkeys_page_fill_key (GpaKeyDetails *kdt, gpgme_key_t key)
{
  if (kdt->subkeys_page)
    gpa_subkey_list_set_key (kdt->subkeys_list, key);
}


/* Empty the subkeys page.  */
static void
subkeys_page_empty (GpaKeyDetails *kdt)
{
  if (kdt->subkeys_page)
    gpa_subkey_list_set_key (kdt->subkeys_list, NULL);
}



/* Add a single row to the details table.  */
static GtkWidget *
add_details_row (GtkWidget *table, gint row, gchar *text,
                 gboolean selectable)
{
  GtkWidget *widget;

  widget = gtk_label_new (text);
  gtk_table_attach (GTK_TABLE (table), widget, 0, 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.0);

  widget = gtk_label_new ("");
  gtk_label_set_selectable (GTK_LABEL (widget), selectable);
  gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);

  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, row, row + 1,
                    GTK_FILL | GTK_EXPAND, 0, 0, 0);

  return widget;
}


static void
construct_details_page (GpaKeyDetails *kdt)
{
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *viewport;
  gint table_row;

  /* Details Page */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  viewport = gtk_viewport_new (NULL, NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);
  gtk_container_add (GTK_CONTAINER (viewport), vbox);
  gtk_container_add (GTK_CONTAINER (scrolled), viewport);

  label = gtk_label_new ("");
  kdt->details_num_label = label;
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  
  table = gtk_table_new (2, 7, FALSE);
  kdt->details_table = table;
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);

  table_row = 0;
  kdt->detail_public_private = add_details_row 
    (table, table_row++, "", TRUE);
  kdt->detail_capabilities = add_details_row 
    (table, table_row++, "", TRUE);
  kdt->detail_name = add_details_row 
    (table, table_row++, _("User name:"), TRUE);
  kdt->detail_key_id = add_details_row 
    (table, table_row++, _("Key ID:"), TRUE);
  kdt->detail_fingerprint = add_details_row 
    (table, table_row++, _("Fingerprint:"), TRUE);
  kdt->detail_expiry = add_details_row 
    (table, table_row++, _("Expires at:"), FALSE); 
  kdt->detail_owner_trust = add_details_row 
    (table, table_row++, _("Owner Trust:"), FALSE);
  kdt->detail_key_trust = add_details_row 
    (table, table_row++, _("Key validity:"), FALSE);
  kdt->detail_key_type = add_details_row 
    (table, table_row++, _("Key type:"), FALSE);
  kdt->detail_creation = add_details_row 
    (table, table_row++, _("Created at:"), FALSE);

  gtk_notebook_append_page (GTK_NOTEBOOK (kdt), scrolled,
                            gtk_label_new (_("Details")));
}


/* Add the signatures page to the notebook.  */
static void
construct_signatures_page (GpaKeyDetails *kdt)
{
  GtkWidget *label;
  GtkWidget *vbox, *hbox;
  GtkWidget *combo;
  GtkWidget *scrolled;
  GtkWidget *siglist;

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  /* UID select button and label.  */
  hbox = gtk_hbox_new (FALSE, 5);
  label = gtk_label_new (_("Show signatures on user name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  combo = gtk_combo_box_new_text ();
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_set_sensitive (combo, FALSE);
  kdt->signatures_uids = combo;
  kdt->signatures_count = 0;
  kdt->signatures_hbox = hbox;
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* Signature list.  */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  siglist = gpa_siglist_new ();
  kdt->signatures_list = siglist;
  gtk_container_add (GTK_CONTAINER (scrolled), siglist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gtk_notebook_append_page (GTK_NOTEBOOK (kdt), vbox,
                            gtk_label_new (_("Signatures")));

}


/* Add the subkeys page to the notebook.  */
static void
construct_subkeys_page (GpaKeyDetails *kdt)
{
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *subkeylist;
  
  if (kdt->subkeys_page)
    return;  /* The page has already been constructed.  */

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  subkeylist = gpa_subkey_list_new ();
  gtk_container_add (GTK_CONTAINER (scrolled), subkeylist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  kdt->subkeys_list = subkeylist;
  kdt->subkeys_page = vbox;
  gtk_notebook_append_page (GTK_NOTEBOOK (kdt), kdt->subkeys_page,
                            gtk_label_new (_("Subkeys")));
/* FIXME gtk_widget_show_all (kdt->notebook_details); */
/*       keyring_update_details_notebook (editor); */
/*       idle_update_details (editor); */
}


/* Remove the subkeys page from the notebook.  */
static void
remove_subkeys_page (GpaKeyDetails *kdt)
{
  if (!kdt->subkeys_page)
    return;  /* Page has not been constructed.  */
    
  gtk_notebook_remove_page (GTK_NOTEBOOK (kdt), 2);
  kdt->subkeys_list = NULL;
  kdt->subkeys_page = NULL;
}


/* Signal handler for the "changed_ui_mode" signal.  */
static void
ui_mode_changed (GpaOptions *options, gpointer param)
{
  GpaKeyDetails *kdt = param;

  if (!gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    construct_subkeys_page (kdt);
  else
    remove_subkeys_page (kdt);
}


/* This function constructs the container holding all widgets making
   up this data widget.  It is called during instance creation.  */
static void
construct_main_widget (GpaKeyDetails *kdt)
{
/*   kdt->notebook = gtk_notebook_new (); */
/*   gtk_container_add (GTK_CONTAINER (kdt), kdt->notebook); */

  /* Details Page */
  construct_details_page (kdt);

  /* Signatures Page.  */
  construct_signatures_page (kdt);

  /* The Subkeys page is only added if the simplified UI is not used.  */
  if (!gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    construct_subkeys_page (kdt);

  /* Connect the signal to update the list of user IDs in the
     signatures page of the notebook.  */
  g_signal_connect (G_OBJECT (kdt->signatures_uids), "changed",
                    G_CALLBACK (signatures_uid_changed), kdt);

  /* Connect the signal to act on the simplified UI change signal.  */
  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_ui_mode",
                    G_CALLBACK (ui_mode_changed), kdt);
}



/************************************************************ 
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_key_details_class_init (void *class_ptr, void *class_data)
{
  GpaKeyDetailsClass *klass = class_ptr;

  parent_class = g_type_class_peek_parent (klass);
  
  G_OBJECT_CLASS (klass)->finalize = gpa_key_details_finalize;
}


static void
gpa_key_details_init (GTypeInstance *instance, void *class_ptr)
{
  GpaKeyDetails *obj = GPA_KEY_DETAILS (instance);

  construct_main_widget (obj);
}


static void
gpa_key_details_finalize (GObject *object)
{  
/*   GpaKeyDetails *obj = GPA_KEY_DETAILS (object); */

  parent_class->finalize (object);
}


/* Construct the class.  */
GType
gpa_key_details_get_type (void)
{
  static GType this_type = 0;
  
  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaKeyDetailsClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_key_details_class_init,
	  (GClassFinalizeFunc) NULL,
	  NULL, /* class_data */
	  sizeof (GpaKeyDetails),
	  0,    /* n_preallocs */
	  gpa_key_details_init
	};
      
      this_type = g_type_register_static (GTK_TYPE_NOTEBOOK,
                                          "GpaKeyDetails",
                                          &this_info, 0);
    }
  
  return this_type;
}


/************************************************************ 
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_key_details_new ()
{
  return GTK_WIDGET (g_object_new (GPA_KEY_DETAILS_TYPE, NULL));  
}


/* Update the key details widget KEYDETAILS with KEY.  The caller also
   needs to provide the number of keys, so that the widget may show a
   key count instead of a key.  The actual key details are only shown
   if KEY is not NULL and KEYCOUNT is 1.  */
void
gpa_key_details_update (GtkWidget *keydetails, gpgme_key_t key, int keycount)
{
  GpaKeyDetails *kdt;

  g_return_if_fail (GPA_IS_KEY_DETAILS (keydetails));
  kdt = GPA_KEY_DETAILS (keydetails);

  if (key && keycount == 1)
    {
      details_page_fill_key (kdt, key);
      signatures_page_fill_key (kdt, key);
      subkeys_page_fill_key (kdt, key);
    }
  else
    {
      details_page_fill_num_keys (kdt, keycount);
      signatures_page_empty (kdt);
      subkeys_page_empty (kdt);
    }
  gtk_widget_show_all (keydetails);
}


/* Find the key identified by pattern and show the details of the
   first key found.  Pattern should be the fingerprint of a key so
   that only one key will be shown.  */
void
gpa_key_details_find (GtkWidget *keydetails, const char *pattern)
{
  gpg_error_t err;
  gpgme_ctx_t ctx;
  gpgme_key_t key = NULL;
  int any = 0;

  err = gpgme_new (&ctx);
  if (err)
    {
      gpa_gpgme_error (err);
      gpa_key_details_update (keydetails, NULL, 0);
      return;
    }
  gpgme_set_protocol (ctx, GPGME_PROTOCOL_OpenPGP);

  if (!gpgme_op_keylist_start (ctx, pattern, 0))
    {
      while (!gpgme_op_keylist_next (ctx, &key))
        {
          gpa_key_details_update (keydetails, key, 1);
          gpgme_key_unref (key);
          any = 1;
          break;
        }
    }
  gpgme_op_keylist_end (ctx);
  gpgme_release (ctx);
  if (!any)
    gpa_key_details_update (keydetails, NULL, 0);
}

