/* selectkeydlg.c - A dialog to select a key.
 * Copyright (C) 2008 g10 Code GmbH.
 *
 * This file is part of GPA
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

#include <config.h>

#include "gpa.h"
#include "i18n.h"

#include "gtktools.h"
#include "keylist.h"
#include "selectkeydlg.h"


struct _SelectKeyDlg
{
  GtkDialog parent;

  GpaKeyList *keylist;

  gpgme_protocol_t protocol;
  gpgme_key_t *initial_keys;
  const char *initial_pattern;
};


struct _SelectKeyDlgClass
{
  GtkDialogClass parent_class;

};


/* The parent class.  */
static GObjectClass *parent_class;


/* Indentifiers for our properties. */
enum
  {
    PROP_0,
    PROP_WINDOW,
    PROP_PROTOCOL,
    PROP_INITIAL_KEYS,
    PROP_INITIAL_PATTERN
  };





/* Signal handler for selection changes of the keylist.  */
static void
keylist_selection_changed_cb (GtkTreeSelection *treeselection,
                              gpointer user_data)
{
  SelectKeyDlg *dialog = user_data;
  gboolean okay;

  g_debug ("keyring_selection_changed_cb called");
  okay = gpa_keylist_has_single_selection (dialog->keylist);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_OK, okay);
}





/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

static void
select_key_dlg_get_property (GObject *object, guint prop_id,
                             GValue *value, GParamSpec *pspec)
{
  SelectKeyDlg *dialog = SELECT_KEY_DLG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value,
			  gtk_window_get_transient_for (GTK_WINDOW (dialog)));
      break;
    case PROP_PROTOCOL:
      g_value_set_int (value, dialog->protocol);
      break;
    case PROP_INITIAL_KEYS:
      g_value_set_pointer (value, dialog->initial_keys);
      break;
    case PROP_INITIAL_PATTERN:
      g_value_set_string (value, dialog->initial_pattern);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
select_key_dlg_set_property (GObject *object, guint prop_id,
                            const GValue *value, GParamSpec *pspec)
{
  SelectKeyDlg *dialog = SELECT_KEY_DLG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      gtk_window_set_transient_for (GTK_WINDOW (dialog),
				    g_value_get_object (value));
      break;
    case PROP_PROTOCOL:
      dialog->protocol = g_value_get_int (value);
      break;
    case PROP_INITIAL_KEYS:
      dialog->initial_keys = (gpgme_key_t*)g_value_get_pointer (value);
      break;
    case PROP_INITIAL_PATTERN:
      dialog->initial_pattern = g_value_get_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
select_key_dlg_finalize (GObject *object)
{
  SelectKeyDlg *dialog = SELECT_KEY_DLG (object);

  gpa_gpgme_release_keyarray (dialog->initial_keys);

  /* Fixme:  Release the store.  */


  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
select_key_dlg_init (SelectKeyDlg *dialog)
{
}


static GObject*
select_key_dlg_constructor (GType type, guint n_construct_properties,
                            GObjectConstructParam *construct_properties)
{
  GObject *object;
  SelectKeyDlg *dialog;
  GtkWidget *vbox;
  GtkWidget *scroller;

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  dialog = SELECT_KEY_DLG (object);
  gpa_window_set_title (GTK_WINDOW (dialog), _("Select a key"));
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			  GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK,
				     FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  vbox = GTK_DIALOG (dialog)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scroller),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scroller, 400, 200);

  /* Create the keylist and initialize if with our initial keys.
     Because we don't need them then anymore, release our own copy of
     the keys.  */
  dialog->keylist = gpa_keylist_new_with_keys (GTK_WIDGET (dialog),
                                               TRUE,
                                               dialog->protocol,
                                               dialog->initial_keys,
                                               dialog->initial_pattern,
                                               KEY_USAGE_ENCR, TRUE);
  gpa_gpgme_release_keyarray (dialog->initial_keys);
  dialog->initial_keys = NULL;
  gtk_container_add (GTK_CONTAINER (scroller), GTK_WIDGET(dialog->keylist));


  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection
			      (GTK_TREE_VIEW (dialog->keylist))),
		    "changed",
                    G_CALLBACK (keylist_selection_changed_cb), dialog);


  return object;
}


static void
select_key_dlg_class_init (SelectKeyDlgClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = select_key_dlg_constructor;
  object_class->finalize = select_key_dlg_finalize;
  object_class->set_property = select_key_dlg_set_property;
  object_class->get_property = select_key_dlg_get_property;

  g_object_class_install_property
    (object_class,
     PROP_WINDOW,
     g_param_spec_object
     ("window", "Parent window",
      "Parent window", GTK_TYPE_WIDGET,
      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

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
      "A string with pattern to be used for the search boxor NULL.",
      NULL,
      G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
}


GType
select_key_dlg_get_type (void)
{
  static GType this_type;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (SelectKeyDlgClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) select_key_dlg_class_init,
	  NULL, /* class_finalize */
	  NULL, /* class_data */

	  sizeof (SelectKeyDlg),
	  0,    /* n_preallocs */
	  (GInstanceInitFunc) select_key_dlg_init,
	};

      this_type = g_type_register_static (GTK_TYPE_DIALOG,
                                          "SelectKeyDlg",
                                          &this_info, 0);
    }

  return this_type;
}



/************************************************************
 **********************  Public API  ************************
 ************************************************************/

SelectKeyDlg *
select_key_dlg_new (GtkWidget *parent)
{
  SelectKeyDlg *dialog;

  dialog = g_object_new (SELECT_KEY_DLG_TYPE,
			 "window", parent,
			 NULL);

  return dialog;
}


/* Same as select_key_dlg_new but with the options to select the
   protocol and to set initial keys and a search string.  */
SelectKeyDlg *
select_key_dlg_new_with_keys (GtkWidget *parent,
                              gpgme_protocol_t protocol,
                              gpgme_key_t *keys, const char *pattern)
{
  SelectKeyDlg *dialog;

  dialog = g_object_new (SELECT_KEY_DLG_TYPE,
                         "window", parent,
                         "protocol", (int)protocol,
                         "initial-keys", gpa_gpgme_copy_keyarray (keys),
                         "initial-pattern", pattern,
                         NULL);

  return dialog;
}



/* Return the selected key.  */
gpgme_key_t
select_key_dlg_get_key (SelectKeyDlg *dialog)
{
  gpgme_key_t key;

  g_return_val_if_fail (dialog, NULL);
  g_return_val_if_fail (dialog->keylist, NULL);

  key = gpa_keylist_get_selected_key (dialog->keylist);

  return key;
}

/* Return an array of selected keys.

   FIXME: For now it returns only one key. */
gpgme_key_t *
select_key_dlg_get_keys (SelectKeyDlg *dialog)
{
  gpgme_key_t *keyarray;

  g_return_val_if_fail (dialog, NULL);
  g_return_val_if_fail (dialog->keylist, NULL);

  keyarray = g_new (gpgme_key_t, 1+1);
  keyarray[0] = gpa_keylist_get_selected_key (dialog->keylist);
  keyarray[1] = NULL;

  return keyarray;
}


