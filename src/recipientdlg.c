/* recipientdlg.c - A dialog to select a mail recipient.
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
#include "gpakeyselector.h"
#include "recipientdlg.h"


struct _RecipientDlg 
{
  GtkDialog parent;
  
  GtkWidget *clist_keys;
};


struct _RecipientDlgClass 
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
    PROP_FORCE_ARMOR
  };


/* Identifiers for the columns of the RECPLIST.  */
enum
  {
    RECPLIST_MAILBOX,   /* The rfc822 mailbox to whom a key needs to
                           be associated.  */
    RECPLIST_HAS_PGP,   /* A PGP certificate is available.  */
    RECPLIST_HAS_X509,  /* An X.509 certificate is available.  */
    RECPLIST_KEYID,     /* The key ID of the associated key. */

    RECPLIST_N_COLUMNS
  };



static void
add_tooltip (GtkWidget *widget, const char *text)
{
#if GTK_CHECK_VERSION (2, 12, 0)
  if (widget && text && *text)
    gtk_widget_set_tooltip_text (widget, text);
#endif
}

static void
set_column_title (GtkTreeViewColumn *column,
                  const char *title, const char *tooltip)
{
  GtkWidget *label;

  label = gtk_label_new (title);
  /* We need to show the label before setting the widget.  */
  gtk_widget_show (label);
  gtk_tree_view_column_set_widget (column, label);
  add_tooltip (gtk_tree_view_column_get_widget (column), tooltip);
}



/* Create the main list of this dialog.  */
static GtkWidget *
recplist_window_new (void)
{
  GtkListStore *store;
  GtkWidget *list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* Create a model and associate a view.  */
  store = gtk_list_store_new (RECPLIST_N_COLUMNS,
			      G_TYPE_STRING,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
			      G_TYPE_STRING);
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);

  /* Define the columns.  */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes 
    (_("Recipient"), renderer, "text", RECPLIST_MAILBOX, NULL);
  set_column_title (column, _("Recipient"),
                    _("Shows the recipients of the message."
                      " A key needs to be assigned to each recipient."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", RECPLIST_HAS_PGP, NULL);
  set_column_title (column, "P", _("Checked if at least one matching"
                                   " OpenPGP certificate has been found.")); 
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", RECPLIST_HAS_X509, NULL);
  set_column_title (column, "X", _("Checked if at least one matching"
                                   " X.509 certificate has been found.")); 
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes 
    (NULL, renderer, "text", RECPLIST_KEYID, NULL);
  set_column_title (column, _("Key ID"),
                    _("Shows the key ID of the selected key or an indication"
                      " that a key needs to be selected."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);


  return list;
}


/* Parse one recipient, this is the working horse of parse_recipeints. */
static void
parse_one_recipient (gpgme_ctx_t ctx, GtkListStore *store, GtkTreeIter *iter,
                     const char *mailbox)
{
  gpgme_key_t key, key2;
  int any_pgp = 0, any_x509 = 0, any_ambiguous = 0, any_unusable = 0;
  char *infostr = NULL;

  key = NULL;

  gpgme_set_protocol (ctx, GPGME_PROTOCOL_OpenPGP);
  if (!gpgme_op_keylist_start (ctx, mailbox, 0))
    {
      if (!gpgme_op_keylist_next (ctx, &key))
        {
          any_pgp++;
          if (!gpgme_op_keylist_next (ctx, &key2))
            {
              any_ambiguous++;
              gpgme_key_unref (key);
              gpgme_key_unref (key2);
              key = key2 = NULL;
            }
        }
    }
  gpgme_op_keylist_end (ctx);
  if (key)
    {
      /* fixme: We should put the key into a list.  It would be best
         to use a hidden columnin the store.  Need to figure out how
         to do that.  */
      if (key->revoked || key->disabled || key->expired)
        any_unusable++;
      else if (!infostr)
        infostr = gpa_gpgme_key_get_userid (key->uids);
    }
  gpgme_key_unref (key);
  key = NULL;

  gpgme_set_protocol (ctx, GPGME_PROTOCOL_CMS);
  if (!gpgme_op_keylist_start (ctx, mailbox, 0))
    {
      if (!gpgme_op_keylist_next (ctx, &key))
        {
          any_x509++;
          if (!gpgme_op_keylist_next (ctx, &key2))
            {
              any_ambiguous++;
              gpgme_key_unref (key);
              gpgme_key_unref (key2);
              key = key2 = NULL;
            }
        }
    }
  gpgme_op_keylist_end (ctx);
  if (key)
    {
      /* fixme: We should put the key into a list.  It would be best
         to use a hidden column in the store.  Need to figure out how
         to do that.  */
      if (key->revoked || key->disabled || key->expired)
        any_unusable++;
      else if (!infostr)
        infostr = gpa_gpgme_key_get_userid (key->uids);
    }
  gpgme_key_unref (key);
  
  if (!infostr)
    infostr = g_strdup (any_ambiguous? 
                        _("[ambiguous key - click to select]"):
                        any_unusable?
                        _("[unusable key - click to select another one]"):
                        _("[click to select]"));
  
  g_print ("   pgp=%d x509=%d infostr=`%s'\n", any_pgp, any_x509, infostr);
  gtk_list_store_set (store, iter,
                      RECPLIST_HAS_PGP, any_pgp,
                      RECPLIST_HAS_X509, any_x509,
                      RECPLIST_KEYID, infostr,
                      -1);
  g_free (infostr);
}


/* Parse the list of recipeints, find possible keys and update the
   store.  */
static void
parse_recipients (GtkListStore *store)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gpg_error_t err;
  gpgme_ctx_t ctx;

  err = gpgme_new (&ctx);
  if (err)
    gpa_gpgme_error (err);


  model = GTK_TREE_MODEL (store);
  /* Walk through the list, reading each row. */
  if (gtk_tree_model_get_iter_first (model, &iter))
    do
      {
        char *mailbox;
        
        gtk_tree_model_get (model, &iter, 
                            RECPLIST_MAILBOX, &mailbox,
                            -1);
        
        /* Do something with the data */
        g_print ("mailbox `%s'\n", mailbox);
        parse_one_recipient (ctx, store, &iter, mailbox);
        g_free (mailbox);
      }
    while (gtk_tree_model_iter_next (model, &iter));
  
  gpgme_release (ctx);
}



static void
recplist_row_activated_cb (GtkTreeView       *tree_view,
                           GtkTreePath       *path,
                           GtkTreeViewColumn *column,
                           gpointer           user_data)    
{
  RecipientDlg *dialog = user_data;
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *mailbox;

  g_print ("tree view row-activated received\n");
  model = gtk_tree_view_get_model (tree_view);
  if (!gtk_tree_model_get_iter (model, &iter, path))
    return; 
        
  gtk_tree_model_get (model, &iter, 
                      RECPLIST_MAILBOX, &mailbox,
                      -1);
        
  g_print ("  mailbox is `%s'\n", mailbox);
  g_free (mailbox);
}



/************************************************************ 
 ******************   Object Management  ********************
 ************************************************************/

static void
recipient_dlg_get_property (GObject *object, guint prop_id,
                            GValue *value, GParamSpec *pspec)
{
  RecipientDlg *dialog = RECIPIENT_DLG (object);
  
  switch (prop_id)
    {
    case PROP_WINDOW:
      g_value_set_object (value,
			  gtk_window_get_transient_for (GTK_WINDOW (dialog)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
recipient_dlg_set_property (GObject *object, guint prop_id,
                            const GValue *value, GParamSpec *pspec)
{
  RecipientDlg *dialog = RECIPIENT_DLG (object);

  switch (prop_id)
    {
    case PROP_WINDOW:
      gtk_window_set_transient_for (GTK_WINDOW (dialog),
				    g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
recipient_dlg_finalize (GObject *object)
{  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
recipient_dlg_init (RecipientDlg *dialog)
{

}


static GObject*
recipient_dlg_constructor (GType type, guint n_construct_properties,
                           GObjectConstructParam *construct_properties)
{
  GObject *object;
  RecipientDlg *dialog;
  GtkAccelGroup *accelGroup;
  GtkWidget *vboxEncrypt;
  GtkWidget *labelKeys;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  dialog = RECIPIENT_DLG (object);


  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  GTK_STOCK_OK, GTK_RESPONSE_OK,
			  _("_Cancel"), GTK_RESPONSE_CANCEL, NULL);
  gtk_window_set_title (GTK_WINDOW (dialog),
                        _("Select keys for recipients"));
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, 
				     FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dialog), accelGroup);

  vboxEncrypt = GTK_DIALOG (dialog)->vbox;
  gtk_container_set_border_width (GTK_CONTAINER (vboxEncrypt), 5);

  labelKeys = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (labelKeys), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), labelKeys, FALSE, FALSE, 0);

  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scrollerKeys),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vboxEncrypt), scrollerKeys, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrollerKeys, 400, 200);

  clistKeys = recplist_window_new ();
  g_signal_connect (G_OBJECT (GTK_TREE_VIEW (clistKeys)),
                    "row-activated",
                    G_CALLBACK (recplist_row_activated_cb), dialog);
  dialog->clist_keys = clistKeys;
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gpa_connect_by_accelerator (GTK_LABEL (labelKeys), clistKeys, accelGroup,
			      _("_Recipient list"));

 

  return object;
}


static void
recipient_dlg_class_init (RecipientDlgClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->constructor = recipient_dlg_constructor;
  object_class->finalize = recipient_dlg_finalize;
  object_class->set_property = recipient_dlg_set_property;
  object_class->get_property = recipient_dlg_get_property;

  g_object_class_install_property
    (object_class,
     PROP_WINDOW,
     g_param_spec_object 
     ("window", "Parent window",
      "Parent window", GTK_TYPE_WIDGET,
      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

}


GType
recipient_dlg_get_type (void)
{
  static GType this_type;
  
  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (RecipientDlgClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) recipient_dlg_class_init,
	  NULL, /* class_finalize */
	  NULL, /* class_data */

	  sizeof (RecipientDlg),
	  0,    /* n_preallocs */
	  (GInstanceInitFunc) recipient_dlg_init,
	};
      
      this_type = g_type_register_static (GTK_TYPE_DIALOG,
                                          "RecipientDlg",
                                          &this_info, 0);
    }
  
  return this_type;
}



/************************************************************ 
 **********************  Public API  ************************
 ************************************************************/

GtkWidget *
recipient_dlg_new (GtkWidget *parent)
{
  RecipientDlg *dialog;
  
  dialog = g_object_new (RECIPIENT_DLG_TYPE,
			 "window", parent,
			 NULL);

  return GTK_WIDGET(dialog);
}


/* Put the recipients into the list.  */
void 
recipient_dlg_set_recipients (GtkWidget *object, GSList *recipients)
{
  RecipientDlg *dialog;
  GtkListStore *store;
  GSList *recp;
  GtkTreeIter iter;
  const char *name;

  g_return_if_fail (object);
  dialog = RECIPIENT_DLG (object);

  store = GTK_LIST_STORE (gtk_tree_view_get_model
                          (GTK_TREE_VIEW (dialog->clist_keys)));

  gtk_list_store_clear (store);
  for (recp = recipients; recp; recp = g_slist_next (recp))
    {
      name = recp->data;
      if (name && *name)
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              RECPLIST_MAILBOX, name,
                              RECPLIST_HAS_PGP, FALSE,
                              RECPLIST_HAS_X509, FALSE,
                              RECPLIST_KEYID,  "",
                              -1);
        }    
    }
  parse_recipients (store);
}



