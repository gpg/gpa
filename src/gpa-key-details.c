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
#include "certchain.h"
#include "gpasubkeylist.h"
#include "gpa-uid-list.h"
#include "gpa-tofu-list.h"
#include "gpa-key-details.h"
#include "gtktools.h"


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
  GtkWidget *detail_last_update;

  /* The widgets in the user ID page.  */
  GtkWidget *uid_page;
  GtkWidget *uid_list;

  /* The widgets in the signatures page.  */
  GtkWidget *signatures_page;
  GtkWidget *signatures_list;
  GtkWidget *signatures_uids;
  GtkWidget *certchain_list;

  /* The widgets in the subkeys page.  */
  GtkWidget *subkeys_page;
  GtkWidget *subkeys_list;

  /* The widgets in the TOFU page.  */
  GtkWidget *tofu_page;
  GtkWidget *tofu_list;

  /* The key currently shown or NULL.  */
  gpgme_key_t current_key;

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

  if (kdt->signatures_list)
    {
      /* Note that we need to subtract one, as the first entry (with
         index 0 means) "all user names".  */
      gpa_siglist_set_signatures
        (kdt->signatures_list, kdt->current_key,
         gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) - 1);
    }
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
      if (seckey->subkeys && seckey->subkeys->is_cardkey)
        gtk_label_set_text (GTK_LABEL (kdt->detail_public_private),
                            _("The key has both a smartcard based private part"
                              " and a public part"));
      else
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

  text = gpa_gpgme_key_format_fingerprint (key->subkeys->fpr);
  gtk_label_set_text (GTK_LABEL (kdt->detail_fingerprint), text);
  g_free (text);

  text = (gchar*) gpa_gpgme_key_get_short_keyid (key);
  gtk_label_set_text (GTK_LABEL (kdt->detail_key_id), text);

  text = gpa_expiry_date_string (key->subkeys->expires);
  gtk_label_set_text (GTK_LABEL (kdt->detail_expiry), text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (kdt->detail_key_trust),
                      gpa_key_validity_string (key));

  text = gpgme_pubkey_algo_string (key->subkeys);
  gtk_label_set_text (GTK_LABEL (kdt->detail_key_type), text? text : "?");
  gpgme_free (text);

  text = g_strdup_printf (_("%s %u bits"),
                          gpgme_pubkey_algo_name (key->subkeys->pubkey_algo)?
                          gpgme_pubkey_algo_name (key->subkeys->pubkey_algo):
                          (key->subkeys->curve? "ECC" : "?"),
                          key->subkeys->length);
  if (key->subkeys->curve)
    {
      char *text2;
      text2 = g_strdup_printf ("%s, %s", text, key->subkeys->curve);
      g_free (text);
      text = text2;
    }
  gpa_add_tooltip (kdt->detail_key_type, text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (kdt->detail_owner_trust),
                      gpa_key_ownertrust_string (key));

  text = gpa_creation_date_string (key->subkeys->timestamp);
  gtk_label_set_text (GTK_LABEL (kdt->detail_creation), text);
  g_free (text);

#if GPGME_VERSION_NUMBER >= 0x010100  /* GPGME >= 1.10.0 */
  text = gpa_update_origin_string (key->last_update, key->origin);
  gtk_label_set_text (GTK_LABEL (kdt->detail_last_update), text);
  g_free (text);
#endif

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
  kdt->detail_fingerprint = add_details_row
    (table, table_row++, _("Fingerprint:"), TRUE);
  kdt->detail_key_id = add_details_row
    (table, table_row++, _("Key ID:"), TRUE);
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
  kdt->detail_last_update = add_details_row
    (table, table_row++, _("Last update:"), FALSE);

  gtk_notebook_append_page (GTK_NOTEBOOK (kdt), scrolled,
                            gtk_label_new (_("Details")));
}


/* Create and append new page with USER ID info for KEY.  If KEY is NULL
   remove an existing USER ID page. */
static void
build_uid_page (GpaKeyDetails *kdt, gpgme_key_t key)
{
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *uidlist;
  int pnum;

  /* First remove an existing page.  */
  if (kdt->uid_page)
    {
      pnum = gtk_notebook_page_num (GTK_NOTEBOOK (kdt), kdt->uid_page);
      if (pnum >= 0)
        gtk_notebook_remove_page (GTK_NOTEBOOK (kdt), pnum);
      kdt->uid_page = NULL;
      if (kdt->uid_list)
        {
          g_object_unref (kdt->uid_list);
          kdt->uid_list = NULL;
        }
    }
  if (!key)
    return;

  /* Create a new page.  */
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  uidlist = gpa_uid_list_new ();
  gtk_container_add (GTK_CONTAINER (scrolled), uidlist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  kdt->uid_list = uidlist;
  g_object_ref (kdt->uid_list);
  kdt->uid_page = vbox;
  gtk_notebook_append_page (GTK_NOTEBOOK (kdt), kdt->uid_page,
                            gtk_label_new (_("User IDs")));

  /* Fill this page.  */
  gpa_uid_list_set_key (kdt->uid_list, key);
}


/* Add the signatures page to the notebook.  */
static void
build_signatures_page (GpaKeyDetails *kdt, gpgme_key_t key)
{
  GtkWidget *label;
  GtkWidget *vbox, *hbox;
  GtkWidget *scrolled;
  int pnum;

  if (kdt->signatures_page)
    {
      if (kdt->signatures_uids)
        g_signal_handlers_disconnect_by_func
          (G_OBJECT (kdt->signatures_uids),
           G_CALLBACK (signatures_uid_changed), kdt);

      pnum = gtk_notebook_page_num (GTK_NOTEBOOK (kdt), kdt->signatures_page);
      if (pnum >= 0)
        gtk_notebook_remove_page (GTK_NOTEBOOK (kdt), pnum);
      kdt->signatures_page = NULL;
      kdt->signatures_uids = NULL;
    }
  if (kdt->signatures_list)
    {
      g_object_unref (kdt->signatures_list);
      kdt->signatures_list = NULL;
    }
  if (kdt->certchain_list)
    {
      g_object_unref (kdt->certchain_list);
      kdt->certchain_list = NULL;
    }
  if (!key)
    return;

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  /* If there is more than one OpenPGP UID, we need a select button.  */
  if (key->uids && key->uids->next && key->protocol == GPGME_PROTOCOL_OpenPGP)
    {
      hbox = gtk_hbox_new (FALSE, 5);
      label = gtk_label_new (_("Show signatures on user name:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      kdt->signatures_uids = gtk_combo_box_new_text ();
      gtk_box_pack_start (GTK_BOX (hbox), kdt->signatures_uids, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      /* Connect the signal to update the list of user IDs in the
         signatures page of the notebook.  */
      g_signal_connect (G_OBJECT (kdt->signatures_uids), "changed",
                        G_CALLBACK (signatures_uid_changed), kdt);
    }

  /* Signature list.  */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  if (key->protocol == GPGME_PROTOCOL_OpenPGP)
    {
      kdt->signatures_list = gpa_siglist_new ();
      g_object_ref (kdt->signatures_list);
      gtk_container_add (GTK_CONTAINER (scrolled), kdt->signatures_list);
    }
  else
    {
      kdt->certchain_list = gpa_certchain_new ();
      g_object_ref (kdt->certchain_list);
      gtk_container_add (GTK_CONTAINER (scrolled), kdt->certchain_list);
    }
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  kdt->signatures_page = vbox;
  gtk_notebook_append_page (GTK_NOTEBOOK (kdt), vbox,
                            gtk_label_new
                            (kdt->certchain_list? _("Chain"):_("Signatures")));


  /* Fill this page.  */
  if (kdt->certchain_list)
    {
      gpa_certchain_update (kdt->certchain_list, key);
    }
  else if (kdt->signatures_uids)
    {
      gpgme_user_id_t uid;
      GtkComboBox *combo;
      int i;

      /* Make the combo widget's width shrink as much as
	 possible. This (hopefully) fixes the previous behaviour
	 correctly: displaying a key with slightly longer signed UIDs
	 caused the top-level window to pseudo-randomly increase it's
	 size (which couldn't even be undone by the user anymore).  */
      combo = GTK_COMBO_BOX (kdt->signatures_uids);
      gtk_widget_set_size_request (GTK_WIDGET (combo), 0, -1);

      gtk_combo_box_append_text (combo, _("All signatures"));
      gtk_combo_box_set_active (combo, 0);
      for (i=1, uid = key->uids; uid; i++, uid = uid->next)
	{
	  gchar *uid_string = gpa_gpgme_key_get_userid (uid);
	  gtk_combo_box_append_text (combo, uid_string);
	  g_free (uid_string);
	}

      gpa_siglist_set_signatures (kdt->signatures_list, key, -1);
    }
  else
    {
      gpa_siglist_set_signatures (kdt->signatures_list, key, 0);
    }
}


/* Create and append new page with all subkeys for KEY.  If KEY is
   NULL remove an existing subkeys page. */
static void
build_subkeys_page (GpaKeyDetails *kdt, gpgme_key_t key)
{
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *subkeylist;
  int pnum;

  /* First remove an existing page.  */
  if (kdt->subkeys_page)
    {
      pnum = gtk_notebook_page_num (GTK_NOTEBOOK (kdt), kdt->subkeys_page);
      if (pnum >= 0)
        gtk_notebook_remove_page (GTK_NOTEBOOK (kdt), pnum);
      kdt->subkeys_page = NULL;
      if (kdt->subkeys_list)
        {
          g_object_unref (kdt->subkeys_list);
          kdt->subkeys_list = NULL;
        }
    }
  if (!key)
    return;

  /* Create a new page.  */
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
  g_object_ref (kdt->subkeys_list);
  kdt->subkeys_page = vbox;
  gtk_notebook_append_page (GTK_NOTEBOOK (kdt), kdt->subkeys_page,
                            gtk_label_new
                            (key->protocol == GPGME_PROTOCOL_OpenPGP
                             ? _("Subkeys") : _("Key")));

  /* Fill this page.  */
  gpa_subkey_list_set_key (kdt->subkeys_list, key);


}


/* Create and append new page with TOFU info for KEY.  If KEY is NULL
   remove an existing TOFU page. */
static void
build_tofu_page (GpaKeyDetails *kdt, gpgme_key_t key)
{
#ifdef ENABLE_TOFU_INFO
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *tofulist;
  int pnum;

  /* First remove an existing page.  */
  if (kdt->tofu_page)
    {
      pnum = gtk_notebook_page_num (GTK_NOTEBOOK (kdt), kdt->tofu_page);
      if (pnum >= 0)
        gtk_notebook_remove_page (GTK_NOTEBOOK (kdt), pnum);
      kdt->tofu_page = NULL;
      if (kdt->tofu_list)
        {
          g_object_unref (kdt->tofu_list);
          kdt->tofu_list = NULL;
        }
    }
  if (!key)
    return;

  /* Create a new page.  */
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  tofulist = gpa_tofu_list_new ();
  gtk_container_add (GTK_CONTAINER (scrolled), tofulist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  kdt->tofu_list = tofulist;
  g_object_ref (kdt->tofu_list);
  kdt->tofu_page = vbox;
  gtk_notebook_append_page (GTK_NOTEBOOK (kdt), kdt->tofu_page,
                            gtk_label_new (_("Tofu")));

  /* Fill this page.  */
  gpa_tofu_list_set_key (kdt->tofu_list, key);
#endif /*ENABLE_TOFU_INFO*/
}


/* Signal handler for the "changed_ui_mode" signal.  */
static void
ui_mode_changed (GpaOptions *options, gpointer param)
{
  GpaKeyDetails *kdt = param;

  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    {
      build_uid_page (kdt, NULL);
      build_signatures_page (kdt, NULL);
      build_subkeys_page (kdt, NULL);
    }
  else
    {
      build_uid_page (kdt, kdt->current_key);
      build_signatures_page (kdt, kdt->current_key);
      build_subkeys_page (kdt, kdt->current_key);
    }
  build_tofu_page (kdt, kdt->current_key);
  gtk_notebook_set_show_tabs
    (GTK_NOTEBOOK (kdt), gtk_notebook_get_n_pages (GTK_NOTEBOOK (kdt)) > 1);
  gtk_widget_show_all (GTK_WIDGET (kdt));
}


/* This function constructs the container holding all widgets making
   up this data widget.  It is called during instance creation.  */
static void
construct_main_widget (GpaKeyDetails *kdt)
{
  /* Details Page */
  construct_details_page (kdt);

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
  GpaKeyDetails *kdt = GPA_KEY_DETAILS (instance);

  construct_main_widget (kdt);
}


static void
gpa_key_details_finalize (GObject *object)
{
  GpaKeyDetails *kdt = GPA_KEY_DETAILS (object);

  if (kdt->current_key)
    {
      gpgme_key_unref (kdt->current_key);
      kdt->current_key = NULL;
    }
  if (kdt->uid_list)
    {
      g_object_unref (kdt->uid_list);
      kdt->uid_list = NULL;
    }
  if (kdt->signatures_list)
    {
      g_object_unref (kdt->signatures_list);
      kdt->signatures_list = NULL;
    }
  if (kdt->certchain_list)
    {
      g_object_unref (kdt->certchain_list);
      kdt->certchain_list = NULL;
    }
  if (kdt->subkeys_list)
    {
      g_object_unref (kdt->subkeys_list);
      kdt->subkeys_list = NULL;
    }
  if (kdt->tofu_list)
    {
      g_object_unref (kdt->tofu_list);
      kdt->tofu_list = NULL;
    }

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
  GtkWidget *widget;
  int pnum;

  g_return_if_fail (GPA_IS_KEY_DETAILS (keydetails));
  kdt = GPA_KEY_DETAILS (keydetails);

  /* Save the currently selected page.  */
  pnum = gtk_notebook_get_current_page (GTK_NOTEBOOK (kdt));
  if (pnum >= 0
      && (widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK (kdt), pnum)))
    {
      if (widget == kdt->uid_page)
        pnum = 1;
      else if (widget == kdt->signatures_page)
        pnum = 2;
      else if (widget == kdt->subkeys_page)
        pnum = 3;
#ifdef ENABLE_TOFU_INFO
      else if (widget == kdt->tofu_page)
        pnum = 4;
#endif /*ENABLE_TOFU_INFO*/
      else
        pnum = 0;
    }
  else
    pnum = 0;


  if (kdt->current_key)
    {
      gpgme_key_unref (kdt->current_key);
      kdt->current_key = NULL;
    }

  if (key && keycount == 1)
    {
      gpgme_key_ref (key);
      kdt->current_key = key;
      details_page_fill_key (kdt, key);

      /* Depend the generation of pages on the mode of the UI.  */
      if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
        {
          build_uid_page (kdt, NULL);
          build_signatures_page (kdt, NULL);
          build_subkeys_page (kdt, NULL);
        }
      else
        {
          build_uid_page (kdt, key);
          build_signatures_page (kdt, key);
          build_subkeys_page (kdt, key);
        }
    }
  else
    {
      details_page_fill_num_keys (kdt, keycount);
      build_signatures_page (kdt, NULL);
    }
  build_tofu_page (kdt, key);

  gtk_notebook_set_show_tabs
    (GTK_NOTEBOOK (kdt), gtk_notebook_get_n_pages (GTK_NOTEBOOK (kdt)) > 1);

  gtk_widget_show_all (keydetails);

  /* Try to select the last selected page.  */
  if (pnum == 1 && kdt->uid_page)
    pnum = gtk_notebook_page_num (GTK_NOTEBOOK (kdt), kdt->uid_page);
  else if (pnum == 2 && kdt->signatures_page)
    pnum = gtk_notebook_page_num (GTK_NOTEBOOK (kdt), kdt->signatures_page);
  else if (pnum == 3 && kdt->subkeys_page)
    pnum = gtk_notebook_page_num (GTK_NOTEBOOK (kdt), kdt->subkeys_page);
#ifdef ENABLE_TOFU_INFO
  else if (pnum == 4 && kdt->tofu_page)
    pnum = gtk_notebook_page_num (GTK_NOTEBOOK (kdt), kdt->tofu_page);
#endif /*ENABLE_TOFU_INFO*/
  else
    pnum = 0;
  gtk_notebook_set_current_page (GTK_NOTEBOOK (kdt), pnum);
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
    gpa_gpgme_error (err);
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

  if (!any)
    {
      gpgme_set_protocol (ctx, GPGME_PROTOCOL_CMS);

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
    }

  gpgme_release (ctx);
  if (!any)
    gpa_key_details_update (keydetails, NULL, 0);
}

