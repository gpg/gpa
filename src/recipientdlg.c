/* recipientdlg.c - A dialog to select a mail recipient.
   Copyright (C) 2008 g10 Code GmbH.

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
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>

#include "gpa.h"
#include "i18n.h"

#include "gtktools.h"
#include "selectkeydlg.h"
#include "recipientdlg.h"


struct _RecipientDlg
{
  GtkDialog parent;

  GtkWidget *clist_keys;
  GtkWidget *statushint;
  GtkWidget *radio_pgp;
  GtkWidget *radio_x509;
  GtkWidget *radio_auto;

  GtkWidget *popup_menu;

  /* Flag to disable updates of the status hint.  This is actual a
     counter with updates only allowed if it is zero.  */
  int freeze_update_statushint;

  /* Flag to disable any key selection.  This is used while a key
     selection is active.  Implemented as a counter.  */
  int freeze_key_selection;

  /* Set if this dialog has usable key to be passed back to the
     caller.  You need to call update_statushint to set it.  */
  int usable;

  /* The selected protocol.  This is also set by update_statushint.  */
  gpgme_protocol_t selected_protocol;
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


/* For performance reasons we truncate the listing of ambiguous keys
   at a reasonable value.  */
#define TRUNCATE_KEYSEARCH_AT 40


/* An object to keep information about keys.  */
struct keyinfo_s
{
  /* An array with associated key(s) or NULL if none found/selected.  */
  gpgme_key_t *keys;

  /* The allocated size of the KEYS array.  This includes the
     terminating NULL entry.  */
  unsigned int dimof_keys;

  /* If set, indicates that the KEYS array has been truncated.  */
  int truncated:1;
};


/* Management information for each recipient.  This data is used per
   recipient.  */
struct userdata_s
{
  /* The recipient's address.  */
  char *mailbox;

  /* Information about PGP keys.  */
  struct keyinfo_s pgp;

  /* Information about X.509 keys.  */
  struct keyinfo_s x509;

  /* If the user has set this field, no encryption key will be
     required for the recipient.  */
  int ignore_recipient;

};


/* Identifiers for the columns of the RECPLIST.  */
enum
  {
    RECPLIST_MAILBOX,   /* The rfc822 mailbox to whom a key needs to
                           be associated.  */
    RECPLIST_HAS_PGP,   /* A PGP certificate is available.  */
    RECPLIST_HAS_X509,  /* An X.509 certificate is available.  */
    RECPLIST_KEYID,     /* The key ID of the associated key. */

    RECPLIST_USERDATA,  /* Pointer to management information (struct
                           userdata_s *).  */

    RECPLIST_N_COLUMNS
  };





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
			      G_TYPE_STRING,
                              G_TYPE_POINTER);

  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);

  /* Define the columns.  */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "markup", RECPLIST_MAILBOX, NULL);
  gpa_set_column_title (column, _("Recipient"),
                        _("Shows the recipients of the message."
                          " A key needs to be assigned to each recipient."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", RECPLIST_HAS_PGP, NULL);
  gpa_set_column_title (column, "PGP",
                        _("Checked if at least one matching"
                          " OpenPGP certificate has been found."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "active", RECPLIST_HAS_X509, NULL);
  gpa_set_column_title (column, "X.509",
                        _("Checked if at least one matching"
                          " X.509 certificate for use with S/MIME"
                          " has been found."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes
    (NULL, renderer, "text", RECPLIST_KEYID, NULL);
  gpa_set_column_title (column,
                        _("Key ID"),
                        _("Shows the key ID of the selected key or"
                          " an indication that a key needs to be selected."));
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);


  return list;
}


/* Get an interator for the selected row.  Store it in ITER and
   returns the mdeol.  if nothing is selected NULL is return and ITER
   is not valid.  */
static GtkTreeModel *
get_selected_row (RecipientDlg *dialog, GtkTreeIter *iter)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;

  g_return_val_if_fail (dialog, NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->clist_keys));
  if (gtk_tree_selection_count_selected_rows (selection) == 1
      && gtk_tree_selection_get_selected (selection, &model, iter))
    return model;
  return NULL;
}


/* Compute and display a new help text for the statushint.  */
static void
update_statushint (RecipientDlg *dialog)
{
  gpgme_protocol_t req_protocol, sel_protocol;
  GtkTreeModel *model;
  GtkTreeIter iter;
  int missing_keys = 0;
  int ambiguous_pgp_keys = 0;
  int ambiguous_x509_keys = 0;
  int n_pgp_keys = 0;
  int n_x509_keys = 0;
  int n_keys = 0;
  const char *hint;
  int okay = 0;

  if (dialog->freeze_update_statushint)
    return;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->clist_keys));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_pgp)))
    req_protocol = GPGME_PROTOCOL_OpenPGP;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                         (dialog->radio_x509)))
    req_protocol = GPGME_PROTOCOL_CMS;
  else
    req_protocol = GPGME_PROTOCOL_UNKNOWN;

  sel_protocol = GPGME_PROTOCOL_UNKNOWN;

  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          gboolean has_pgp, has_x509;
          struct userdata_s *info;

          gtk_tree_model_get (model, &iter,
                              RECPLIST_HAS_PGP, &has_pgp,
                              RECPLIST_HAS_X509, &has_x509,
                              RECPLIST_USERDATA, &info,
                              -1);
          if (!info)
            missing_keys++;  /* Oops */
          else if (info->ignore_recipient)
            ;
          else if (!info->pgp.keys && !info->x509.keys)
            missing_keys++;
          else if ((req_protocol == GPGME_PROTOCOL_OpenPGP && !has_pgp)
                   ||(req_protocol == GPGME_PROTOCOL_CMS && !has_x509))
            ; /* Not of the requested protocol.  */
          else
            {
              n_keys++;
              if (info->pgp.keys && info->pgp.keys[0])
                {
                  n_pgp_keys++;
                  if (info->pgp.keys[1])
                    ambiguous_pgp_keys++;
                }
              if (info->x509.keys && info->x509.keys[0])
                {
                  n_x509_keys++;
                  if (info->x509.keys[1])
                    ambiguous_x509_keys++;
                }
            }
        }
      while (gtk_tree_model_iter_next (model, &iter));
    }

  if (req_protocol == GPGME_PROTOCOL_UNKNOWN)
    {
      /* We select the protocol with the most available keys,
         preferring PGP.  */
      if (n_pgp_keys >= n_x509_keys)
        sel_protocol = GPGME_PROTOCOL_OpenPGP;
      else if (n_x509_keys)
        sel_protocol = GPGME_PROTOCOL_CMS;
    }
  else
    sel_protocol = req_protocol;


  if (missing_keys)
    hint = _("You need to select a key for each recipient.\n"
             "To select a key right-click on the respective line.");
  else if ((sel_protocol == GPGME_PROTOCOL_OpenPGP
            && ambiguous_pgp_keys)
           || (sel_protocol == GPGME_PROTOCOL_CMS
               && ambiguous_x509_keys)
           || (sel_protocol == GPGME_PROTOCOL_UNKNOWN
               && (ambiguous_pgp_keys || ambiguous_x509_keys )))
    hint = _("You need to select exactly one key for each recipient.\n"
             "To select a key right-click on the respective line.");
  else if ((sel_protocol == GPGME_PROTOCOL_OpenPGP
            && n_keys != n_pgp_keys)
           || (sel_protocol == GPGME_PROTOCOL_CMS
               && n_keys != n_x509_keys)
           || (sel_protocol == GPGME_PROTOCOL_UNKNOWN))
    hint = _("Although you selected keys for all recipients "
             "a common encryption protocol can't be used. "
             "Please decide on one protocol by clicking one "
             "of the above radio buttons.");
  else if (n_pgp_keys && sel_protocol == GPGME_PROTOCOL_OpenPGP)
    {
      hint = _("Using OpenPGP for encryption.");
      okay = 1;
    }
  else if (n_x509_keys && sel_protocol == GPGME_PROTOCOL_CMS)
    {
      hint = _("Using S/MIME for encryption.");
      okay = 1;
    }
  else
    hint = _("No recipients - encryption is not possible");

  gtk_label_set_text (GTK_LABEL (dialog->statushint), hint);
  gtk_label_set_line_wrap (GTK_LABEL (dialog->statushint), TRUE);

  dialog->usable = okay;
  dialog->selected_protocol = sel_protocol;
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK,
				     okay);
}


/* Add KEY to the keyarray of KEYINFO.  Ownership of KEY is moved to
   KEYARRAY.  Returns the number of keys in KEYINFO.  */
static unsigned int
append_key_to_keyinfo (struct keyinfo_s *keyinfo, gpgme_key_t key)
{
  unsigned int nkeys;

  if (!keyinfo->keys)
    {
      keyinfo->dimof_keys = 5; /* Space for 4 keys.  */
      keyinfo->keys = g_new (gpgme_key_t, keyinfo->dimof_keys);
      keyinfo->keys[0] = NULL;
    }
  for (nkeys=0; keyinfo->keys[nkeys]; nkeys++)
    ;
  /* Note that we silently skip a KEY of NULL because we can't store
     a NULL in the array.  */
  if (key)
    {
      if (nkeys+1 >= keyinfo->dimof_keys)
        {
          keyinfo->dimof_keys += 10;
          keyinfo->keys = g_renew (gpgme_key_t, keyinfo->keys,
                                   keyinfo->dimof_keys);
        }
      keyinfo->keys[nkeys++] = key;
      keyinfo->keys[nkeys] = NULL;
    }
  return nkeys;
}


/* Clear the content of a keyinfo object.  */
static void
clear_keyinfo (struct keyinfo_s *keyinfo)
{
  unsigned int nkeys;

  if (keyinfo)
    {
      if (keyinfo->keys)
        {
          for (nkeys=0; keyinfo->keys[nkeys]; nkeys++)
            gpgme_key_unref (keyinfo->keys[nkeys]);
          g_free (keyinfo->keys);
          keyinfo->keys = NULL;
        }
      keyinfo->dimof_keys = 0;
      keyinfo->truncated = 0;
    }
}


/* Update the row in the list described by by STORE and ITER.  The new
   data shall be taken from INFO.  */
static void
update_recplist_row (GtkListStore *store, GtkTreeIter *iter,
                     struct userdata_s *info)
{
  char *infostr = NULL;
  char *oldinfostr = NULL;
  int any_pgp = 0, any_x509 = 0;
  gpgme_key_t key;
  char *mailbox;
  char *oldmailbox;

  if (info->pgp.keys && info->pgp.keys[0])
    any_pgp = 1;
  if (info->x509.keys && info->x509.keys[0])
    any_x509 = 1;

  if (info->ignore_recipient)
    infostr = NULL;
  else if (any_pgp && any_x509 && info->pgp.keys[1] && info->x509.keys[1])
    infostr = g_strdup (_("[Ambiguous keys. Right-click to select]"));
  else if (any_pgp && info->pgp.keys[1])
    infostr = g_strdup (_("[Ambiguous PGP key.  Right-click to select]"));
  else if (any_x509 && info->x509.keys[1])
    infostr = g_strdup (_("[Ambiguous X.509 key. Right-click to select]"));
  else if (any_pgp && !info->pgp.keys[1])
    {
      /* Exactly one key found.  */
      key = info->pgp.keys[0];
      infostr = gpa_gpgme_key_get_userid (key->uids);
    }
  else if (any_x509 && !info->x509.keys[1])
    {
      key = info->x509.keys[0];
      infostr = gpa_gpgme_key_get_userid (key->uids);
    }
  else
    infostr = g_strdup (_("[Right-click to select]"));


  mailbox = g_markup_printf_escaped ("<span strikethrough='%s'>%s</span>",
                                     info->ignore_recipient? "true":"false",
                                     info->mailbox);

  g_print ("   mbox=`%s' fmt=`%s'\n", info->mailbox, mailbox);
  gtk_tree_model_get (GTK_TREE_MODEL (store), iter,
                      RECPLIST_MAILBOX, &oldmailbox,
                      RECPLIST_KEYID, &oldinfostr,
                      -1);
  gtk_list_store_set (store, iter,
                      RECPLIST_MAILBOX, mailbox,
                      RECPLIST_HAS_PGP, any_pgp,
                      RECPLIST_HAS_X509, any_x509,
                      RECPLIST_KEYID, infostr,
                      -1);
  g_free (oldmailbox);
  g_free (mailbox);
  g_free (infostr);
  g_free (oldinfostr);
}


/* Parse one recipient, this is the working horse of parse_recipeints. */
static void
parse_one_recipient (gpgme_ctx_t ctx, GtkListStore *store, GtkTreeIter *iter,
                     struct userdata_s *info)
{
  static int have_locate = -1;
  gpgme_key_t key = NULL;
  gpgme_keylist_mode_t mode;

  if (have_locate == -1)
    have_locate = is_gpg_version_at_least ("2.0.10");

  g_return_if_fail (info);

  clear_keyinfo (&info->pgp);
  gpgme_set_protocol (ctx, GPGME_PROTOCOL_OpenPGP);
  mode = gpgme_get_keylist_mode (ctx);
  if (have_locate)
    gpgme_set_keylist_mode (ctx, (mode | (GPGME_KEYLIST_MODE_LOCAL
                                          | GPGME_KEYLIST_MODE_EXTERN)));
  if (!gpgme_op_keylist_start (ctx, info->mailbox, 0))
    {
      while (!gpgme_op_keylist_next (ctx, &key))
        {
          if (key->revoked || key->disabled || key->expired
              || !key->can_encrypt)
            gpgme_key_unref (key);
          else if (append_key_to_keyinfo (&info->pgp, key)
                   >= TRUNCATE_KEYSEARCH_AT)
            {
              /* Note that the truncation flag is not 100% correct.  In
                 case the next iteration would not yield a new key we
                 have not actually truncated the search.  */
              info->pgp.truncated = 1;
              break;
            }
        }
    }
  gpgme_op_keylist_end (ctx);
  gpgme_set_keylist_mode (ctx, mode);

  clear_keyinfo (&info->x509);
  gpgme_set_protocol (ctx, GPGME_PROTOCOL_CMS);
  if (!gpgme_op_keylist_start (ctx, info->mailbox, 0))
    {
      while (!gpgme_op_keylist_next (ctx, &key))
        {
          if (key->revoked || key->disabled || key->expired
              || !key->can_encrypt)
            gpgme_key_unref (key);
          else if (append_key_to_keyinfo (&info->x509,key)
                   >= TRUNCATE_KEYSEARCH_AT)
            {
              info->x509.truncated = 1;
              break;
            }
        }
    }
  gpgme_op_keylist_end (ctx);

  update_recplist_row (store, iter, info);
}


/* Parse the list of recipients, find possible keys and update the
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
        struct userdata_s *info;

        gtk_tree_model_get (model, &iter,
                            RECPLIST_USERDATA, &info,
                            -1);

        /* Do something with the data */
        /*g_print ("parsing mailbox `%s'\n", info? info->mailbox:"(null)");*/
        parse_one_recipient (ctx, store, &iter,info);
      }
    while (gtk_tree_model_iter_next (model, &iter));

  gpgme_release (ctx);
}


/* Callback for the row-activated signal.  */
static void
recplist_row_activated_cb (GtkTreeView       *tree_view,
                           GtkTreePath       *path,
                           GtkTreeViewColumn *column,
                           gpointer           user_data)
{
  /*RecipientDlg *dialog = user_data;*/
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *mailbox;

  model = gtk_tree_view_get_model (tree_view);
  if (!gtk_tree_model_get_iter (model, &iter, path))
    return;

  gtk_tree_model_get (model, &iter,
                      RECPLIST_MAILBOX, &mailbox,
                      -1);
  g_free (mailbox);
}


static void
recplist_row_changed_cb (GtkTreeModel *model, GtkTreePath *path,
                         GtkTreeIter *changediter, gpointer user_data)
{
  RecipientDlg *dialog = user_data;

  g_return_if_fail (dialog);
  update_statushint (dialog);
}


static void
rbutton_toggled_cb (GtkToggleButton *button, gpointer user_data)
{
  RecipientDlg *dialog = user_data;
  GtkTreeViewColumn *column;
  int pgp = FALSE;
  int x509 = FALSE;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_pgp)))
    {
      pgp = TRUE;
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                         (dialog->radio_x509)))
    {
      x509 = TRUE;
    }
  else
    {
      pgp = TRUE;
      x509 = TRUE;
    }

  column = gtk_tree_view_get_column (GTK_TREE_VIEW (dialog->clist_keys),
                                     RECPLIST_HAS_PGP);
  gtk_tree_view_column_set_visible (column, pgp);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (dialog->clist_keys),
                                     RECPLIST_HAS_X509);
  gtk_tree_view_column_set_visible (column, x509);
  update_statushint (dialog);
}



/* The select key selection has returned.  */
static void
select_key_response_cb (SelectKeyDlg *seldlg, int response, void *user_data)
{
  RecipientDlg *dialog = user_data;
  gpgme_key_t key;

  if (response != GTK_RESPONSE_OK)
    {
      /* The dialog was canceled */
      gtk_widget_destroy (GTK_WIDGET (seldlg));
      dialog->freeze_key_selection--;
      return;
    }

  key = select_key_dlg_get_key (seldlg);
  if (key)
    {
      GtkTreeModel *model;
      GtkTreeIter iter;
      struct userdata_s *info = NULL;
      char *uidstr = gpa_gpgme_key_get_userid (key->uids);

      g_free (uidstr);
      if ((model = get_selected_row (dialog, &iter)))
        {
          gtk_tree_model_get (model, &iter, RECPLIST_USERDATA, &info, -1);
          if (info)
            {
              if (key->protocol == GPGME_PROTOCOL_OpenPGP)
                {
                  clear_keyinfo (&info->pgp);
                  gpgme_key_ref (key);
                  append_key_to_keyinfo (&info->pgp, key);
                }
              else if (key->protocol == GPGME_PROTOCOL_CMS)
                {
                  clear_keyinfo (&info->x509);
                  gpgme_key_ref (key);
                  append_key_to_keyinfo (&info->x509, key);
                }
              update_recplist_row (GTK_LIST_STORE (model), &iter, info);
              update_statushint (dialog);
            }
        }
      gpgme_key_unref (key);
    }

  gtk_widget_destroy (GTK_WIDGET (seldlg));
  dialog->freeze_key_selection--;
}



static void
do_select_key (RecipientDlg *dialog, gpgme_protocol_t protocol)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  struct userdata_s *info = NULL;
  SelectKeyDlg *seldlg;

  if ((model = get_selected_row (dialog, &iter)))
    {
      gtk_tree_model_get (model, &iter, RECPLIST_USERDATA, &info, -1);
      if (info)
        {
          gpgme_key_t *keys;

          if (protocol == GPGME_PROTOCOL_OpenPGP)
            keys = info->pgp.keys;
          else if (protocol == GPGME_PROTOCOL_CMS)
            keys = info->x509.keys;
          else
            keys = NULL;

          seldlg = select_key_dlg_new_with_keys (GTK_WIDGET (dialog),
                                                 protocol,
                                                 keys,
                                                 info->mailbox);
          g_signal_connect (G_OBJECT (seldlg), "response",
                            G_CALLBACK (select_key_response_cb), dialog);
          gtk_widget_show_all (GTK_WIDGET (seldlg));
        }
    }
  else
    dialog->freeze_key_selection--;
}


static void
recplist_popup_pgp (GtkAction *action, RecipientDlg *dialog)
{
  do_select_key (dialog, GPGME_PROTOCOL_OpenPGP);
}


static void
recplist_popup_x509 (GtkAction *action, RecipientDlg *dialog)
{
  do_select_key (dialog, GPGME_PROTOCOL_CMS);
}


static void
recplist_popup_ignore (GtkAction *action, RecipientDlg *dialog)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  struct userdata_s *info;

  if ((model = get_selected_row (dialog, &iter)))
    {
      gtk_tree_model_get (model, &iter, RECPLIST_USERDATA, &info, -1);
      info->ignore_recipient = !info->ignore_recipient;
      update_recplist_row (GTK_LIST_STORE (model), &iter, info);
      update_statushint (dialog);
    }
  dialog->freeze_key_selection--;
}


/* Left Click on the list auto selects an action.  */
static void
recplist_default_action (RecipientDlg *dialog)
{

  dialog->freeze_key_selection--;
}


static gint
recplist_display_popup_menu (RecipientDlg *dialog, GdkEvent *event,
                             GtkListStore *list)
{
  GtkMenu *menu;
  GdkEventButton *event_button;

  g_return_val_if_fail (dialog, FALSE);
  g_return_val_if_fail (event, FALSE);

  menu = GTK_MENU (dialog->popup_menu);

  if (event->type == GDK_BUTTON_PRESS)
    {
      event_button = (GdkEventButton *) event;
      if (event_button->button == 1
          || event_button->button == 3)
	{
	  GtkTreeSelection *selection;
	  GtkTreePath *path;
	  GtkTreeIter iter;

          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
          /* Make sure the clicked key is selected.  */
	  if (!dialog->freeze_key_selection
              && gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list),
                                                event_button->x,
                                                event_button->y,
                                                &path, NULL,
                                                NULL, NULL))
	    {
              dialog->freeze_key_selection++;
	      gtk_tree_model_get_iter (gtk_tree_view_get_model
				       (GTK_TREE_VIEW (list)), &iter, path);
	      if (!gtk_tree_selection_iter_is_selected (selection, &iter))
		{
		  gtk_tree_selection_unselect_all (selection);
		  gtk_tree_selection_select_path (selection, path);
		}
              if (event_button->button == 1)
                recplist_default_action (dialog);
              else
                gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
                                event_button->button, event_button->time);
	    }
	  return TRUE;
	}
    }

  return FALSE;
}


/* Create the popup menu for the recipient list.  */
static GtkWidget *
recplist_popup_menu_new (GtkWidget *window, RecipientDlg *dialog)
{
  static const GtkActionEntry entries[] =
    {
      /* Toplevel.  */
      { "SelectPGPKey", NULL, N_("Select _PGP key..."), NULL,
	NULL, G_CALLBACK (recplist_popup_pgp) },
      { "SelectCMSKey", NULL, N_("Select _S\\/MIME key..."), NULL,
	NULL, G_CALLBACK (recplist_popup_x509) },
      { "ToggleIgnoreFlag", NULL, N_("Toggle _Ignore flag"), NULL,
	NULL, G_CALLBACK (recplist_popup_ignore) }
    };

  static const char *ui_description =
    "<ui>"
    "  <popup name='PopupMenu'>"
    "    <menuitem action='SelectPGPKey'/>"
    "    <menuitem action='SelectCMSKey'/>"
    "    <menuitem action='ToggleIgnoreFlag'/>"
    "  </popup>"
    "</ui>";

  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GError *error;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, PACKAGE);
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries),
				dialog);
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  if (! gtk_ui_manager_add_ui_from_string (ui_manager, ui_description,
					   -1, &error))
    {
      g_message ("building clipboard menus failed: %s", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }

  return gtk_ui_manager_get_widget (ui_manager, "/PopupMenu");
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
  /* Fixme:  Release the store.  */
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
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *widget;
  GtkWidget *labelKeys;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;

  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  dialog = RECIPIENT_DLG (object);

  gpa_window_set_title (GTK_WINDOW (dialog), _("Select keys for recipients"));
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

  labelKeys = gtk_label_new_with_mnemonic (_("_Recipient list"));
  gtk_misc_set_alignment (GTK_MISC (labelKeys), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), labelKeys, FALSE, FALSE, 0);

  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy  (GTK_SCROLLED_WINDOW (scrollerKeys),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrollerKeys, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrollerKeys, 400, 200);

  clistKeys = recplist_window_new ();
  dialog->clist_keys = clistKeys;
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_label_set_mnemonic_widget (GTK_LABEL (labelKeys), clistKeys);


  dialog->popup_menu = recplist_popup_menu_new (dialog->clist_keys, dialog);
  g_signal_connect_swapped (GTK_OBJECT (dialog->clist_keys),
                            "button_press_event",
                            G_CALLBACK (recplist_display_popup_menu), dialog);

  dialog->radio_pgp  = gtk_radio_button_new_with_mnemonic
    (NULL, _("Use _PGP"));
  dialog->radio_x509 = gtk_radio_button_new_with_mnemonic
    (gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog->radio_pgp)),
     _("Use _X.509"));
  dialog->radio_auto = gtk_radio_button_new_with_mnemonic
    (gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog->radio_pgp)),
     _("_Auto selection"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->radio_auto), TRUE);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), dialog->radio_pgp,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), dialog->radio_x509, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), dialog->radio_auto, FALSE, FALSE, 0);

  widget = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  dialog->statushint = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->statushint, FALSE, FALSE, 0);


  g_signal_connect (G_OBJECT (GTK_TREE_VIEW (dialog->clist_keys)),
                    "row-activated",
                    G_CALLBACK (recplist_row_activated_cb), dialog);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_model
                              (GTK_TREE_VIEW (dialog->clist_keys))),
                    "row-changed",
                    G_CALLBACK (recplist_row_changed_cb), dialog);

  g_signal_connect (G_OBJECT (dialog->radio_pgp),
                    "toggled",
                    G_CALLBACK (rbutton_toggled_cb), dialog);
  g_signal_connect (G_OBJECT (dialog->radio_x509),
                    "toggled",
                    G_CALLBACK (rbutton_toggled_cb), dialog);
  g_signal_connect (G_OBJECT (dialog->radio_auto),
                    "toggled",
                    G_CALLBACK (rbutton_toggled_cb), dialog);


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

RecipientDlg *
recipient_dlg_new (GtkWidget *parent)
{
  RecipientDlg *dialog;

  dialog = g_object_new (RECIPIENT_DLG_TYPE,
			 "window", parent,
			 NULL);

  return dialog;
}


/* Put RECIPIENTS into the list.  PROTOCOL select the default protocol. */
void
recipient_dlg_set_recipients (RecipientDlg *dialog, GSList *recipients,
                              gpgme_protocol_t protocol)
{
  GtkListStore *store;
  GSList *recp;
  GtkTreeIter iter;
  const char *name;
  GtkWidget *widget;

  g_return_if_fail (dialog);

  dialog->freeze_update_statushint++;

  if (protocol == GPGME_PROTOCOL_OpenPGP)
    widget = dialog->radio_pgp;
  else if (protocol == GPGME_PROTOCOL_CMS)
    widget = dialog->radio_x509;
  else
    widget = dialog->radio_auto;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

  if (widget != dialog->radio_auto)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (dialog->radio_pgp), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (dialog->radio_x509), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (dialog->radio_auto), FALSE);
    }

  store = GTK_LIST_STORE (gtk_tree_view_get_model
                          (GTK_TREE_VIEW (dialog->clist_keys)));

  gtk_list_store_clear (store);
  for (recp = recipients; recp; recp = g_slist_next (recp))
    {
      name = recp->data;
      if (name && *name)
        {
          struct userdata_s *info = g_malloc0 (sizeof *info);

          info->mailbox = g_strdup (name);
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              RECPLIST_MAILBOX, g_strdup (""),
                              RECPLIST_HAS_PGP, FALSE,
                              RECPLIST_HAS_X509, FALSE,
                              RECPLIST_KEYID,  NULL,
                              RECPLIST_USERDATA, info,
                              -1);
        }
    }

  parse_recipients (store);
  dialog->freeze_update_statushint--;
  update_statushint (dialog);
}


/* Return the selected keys as well as the selected protocol.  */
gpgme_key_t *
recipient_dlg_get_keys (RecipientDlg *dialog, gpgme_protocol_t *r_protocol)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  size_t idx, nkeys;
  gpgme_key_t key, *keyarray;
  gpgme_protocol_t protocol;

  g_return_val_if_fail (dialog, NULL);

  if (!dialog->usable)
    return NULL;  /* No valid keys available.  */
  protocol = dialog->selected_protocol;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->clist_keys));

  /* Count the number of possible keys.  */
  nkeys = 0;
  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      do
        nkeys++;
      while (gtk_tree_model_iter_next (model, &iter));
    }
  keyarray = g_new (gpgme_key_t, nkeys+1);
  idx = 0;
  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          char *mailbox;
          struct userdata_s *info;

          if (idx >= nkeys)
            {
              g_debug ("key list grew unexpectedly\n");
              break;
            }

          gtk_tree_model_get (model, &iter,
                              RECPLIST_MAILBOX, &mailbox,
                              RECPLIST_USERDATA, &info,
                              -1);
          if (info && !info->ignore_recipient)
            {
              if (protocol == GPGME_PROTOCOL_OpenPGP && info->pgp.keys)
                key = info->pgp.keys[0];
              else if (protocol == GPGME_PROTOCOL_CMS && info->x509.keys)
                key = info->x509.keys[0];
              else
                key = NULL;
              if (key)
                {
                  gpgme_key_ref (key);
                  keyarray[idx++] = key;
                }
            }
          g_free (mailbox);
        }
      while (gtk_tree_model_iter_next (model, &iter));
    }
  g_assert (idx < nkeys+1);
  keyarray[idx] = NULL;

  if (r_protocol)
    *r_protocol = protocol;

  return keyarray;
}



