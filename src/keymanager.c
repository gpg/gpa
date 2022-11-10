/* keymanager.c  -  The Key Manager.
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
 * GPA is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <gpgme.h>

#include "gpa.h"
#include "gtktools.h"
#include "icons.h"
#include "helpmenu.h"
#include "gpawidgets.h"
#include "ownertrustdlg.h"
#include "keysigndlg.h"
#include "keygendlg.h"
#include "keygenwizard.h"
#include "keyeditdlg.h"
#include "keydeletedlg.h"
#include "keylist.h"
#include "siglist.h"
#include "gpgmetools.h"
#include "gpgmeedit.h"
#include "keytable.h"
#include "server-access.h"
#include "options.h"
#include "convert.h"

#include "gpasubkeylist.h"

#include "gpakeydeleteop.h"
#include "gpakeysignop.h"
#include "gpakeytrustop.h"

#include "gpaexportfileop.h"
#include "gpaexportclipop.h"
#include "gpaexportserverop.h"

#include "gpaimportfileop.h"
#include "gpaimportclipop.h"
#include "gpaimportserverop.h"
#include "gpaimportbykeyidop.h"

#include "gpabackupop.h"

#include "gpagenkeyadvop.h"
#include "gpagenkeysimpleop.h"

#include "gpa-key-details.h"

#include "keymanager.h"


#if ! GTK_CHECK_VERSION (2, 10, 0)
#define GTK_STOCK_SELECT_ALL "gtk-select-all"
#endif


/* Object's class definition.  */
struct _GpaKeyManagerClass
{
  GtkWindowClass parent_class;
};


/* Object definition.  */
struct _GpaKeyManager
{
  GtkWindow parent_instance;

  /* The central list of keys.  */
  GpaKeyList *keylist;

  /* The "Show Ownertrust" toggle button.  */
  GtkWidget *toggle_show;

  /* The details widget.  */
  GtkWidget *details;

  /* Idle handler id for updates of the details widget.  Will be
     nonzero whenever a handler is currently set and zero
     otherwise.  */
  guint details_idle_id;

  /* Labels in the status bar.  */
  GtkWidget *status_label;
  GtkWidget *status_key_user;
  GtkWidget *status_key_id;

  /* The popup menu.  */
  GtkWidget *popup_menu;

  /* List of sensitive widgets.  See below.  */
  GList *selection_sensitive_actions;

  /* The currently selected key.  */
  gpgme_key_t current_key;

  /* Context used for retrieving the current key.  */
  GpaContext *ctx;

  /* Hack: warn the selection callback to ignore changes. Don't, ever,
     assign a value directly.  Raise and lower it with increments.  */
  int freeze_selection;
};


/* There is only one instance of the card manager class.  Use a global
   variable to keep track of it.  */
static GpaKeyManager *this_instance;


/* Prototype of a sensitivity callback.  Return TRUE if the widget
   should be sensitive, FALSE otherwise.  The parameter is a pointer
   to the instance.  */
typedef gboolean (*sensitivity_func_t) (gpointer);


/* Local prototypes */
static int idle_update_details (gpointer param);
static void keyring_update_details (GpaKeyManager *self);

static void gpa_key_manager_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/

/* A simple sensitivity callback mechanism.

   The basic idea is that buttons (and other widgets like menu items
   as well) should know when they should be sensitive or not.  The
   implementation here is very simple and quite specific for the
   key manager's needs.

   We maintain a list of sensitive widgets each of which has a
   sensitivity callback associated with them as the "gpa_sensitivity"
   data.  The callback returns TRUE when the widget should be
   sensitive and FALSE otherwise.

   Whenever the selection in the key list widget changes we call
   update_selection_sensitive_actions which iterates through the
   widgets in the list, calls the sensitivity callback and changes the
   widget's sensitivity accordingly.  */


/* Add widget to the list of sensitive widgets of editor.  */
static void
add_selection_sensitive_action (GpaKeyManager *self,
                                GSimpleAction *action,
                                sensitivity_func_t callback)
{
  g_object_set_data (G_OBJECT (action), "gpa_sensitivity", callback);
  self->selection_sensitive_actions
    = g_list_append (self->selection_sensitive_actions, action);
}


/* Update the sensitivity of the widget data and pass param through to
   the sensitivity callback.  Usable as iterator function in
   g_list_foreach.  */
static void
update_selection_sensitive_action (gpointer data, gpointer param)
{
  sensitivity_func_t func;

  func = g_object_get_data (G_OBJECT (data), "gpa_sensitivity");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (data), func (param));
}


/* Call update_selection_sensitive_widget for all widgets in the list
   of sensitive widgets and pass self through as the user data
   parameter.  */
static void
update_selection_sensitive_actions (GpaKeyManager *self)
{
  g_list_foreach (self->selection_sensitive_actions,
                  update_selection_sensitive_action, self);
}

/* Disable all the widgets in the list of sensitive widgets.  To be used while
   the selection changes.  */
static void
disable_selection_sensitive_actions (GpaKeyManager *self)
{
  GList *cur;

  cur = self->selection_sensitive_actions;
  while (cur)
    {
      g_simple_action_set_enabled (G_SIMPLE_ACTION (cur->data), FALSE);
      cur = g_list_next (cur);
    }
}



/* Helper functions to cope with selections.  */

/* Return TRUE if the key list widget of the key manager has at
   least one selected item.  Usable as a sensitivity callback.  */
static gboolean
key_manager_has_selection (gpointer param)
{
  GpaKeyManager *self = param;

  return gpa_keylist_has_selection (self->keylist);
}


/* Return TRUE if the key list widget of the key manager has
   exactly one selected item.  Usable as a sensitivity callback.  */
static gboolean
key_manager_has_single_selection (gpointer param)
{
  GpaKeyManager *self = param;

  return gpa_keylist_has_single_selection (self->keylist);
}


/* Return TRUE if the key list widget of the key manager has
   exactly one selected OpenPGP item.  Usable as a sensitivity
   callback.  */
static gboolean
key_manager_has_single_selection_OpenPGP (gpointer param)
{
  GpaKeyManager *self = param;
  int result = 0;

  if (gpa_keylist_has_single_selection (self->keylist))
    {
      gpgme_key_t key = gpa_keylist_get_selected_key (self->keylist);
      if (key && key->protocol == GPGME_PROTOCOL_OpenPGP)
        result = 1;
      gpgme_key_unref (key);
    }

  return result;
}

/* Return TRUE if the key list widget of the key manager has
   exactly one selected item and it is a private key.  Usable as a
   sensitivity callback.  */
static gboolean
key_manager_has_private_selected (gpointer param)
{
  GpaKeyManager *self = param;

  return gpa_keylist_has_single_secret_selection
    (GPA_KEYLIST(self->keylist));
}


/* Return the the currently selected key. NULL if no key is selected.  */
static gpgme_key_t
key_manager_current_key (GpaKeyManager *self)
{
  return self->current_key;
}



/* Action callbacks.  */


static void
gpa_key_manager_changed_wot_cb (gpointer data)
{
  GpaKeyManager *self = data;
  gpa_keylist_start_reload (self->keylist);
}


static void
gpa_key_manager_changed_wot_secret_cb (gpointer data)
{
  GpaKeyManager *self = data;

  gpa_keylist_imported_secret_key (self->keylist);
  gpa_keylist_start_reload (self->keylist);
}

static void
gpa_key_manager_key_modified (GpaKeyEditDialog *dialog, gpgme_key_t key,
				 gpointer data)
{
  GpaKeyManager *self = data;
  gpa_keylist_start_reload (self->keylist);
}


static void
gpa_key_manager_new_key_cb (gpointer data, const gchar *fpr)
{
  GpaKeyManager *self = data;

  gpa_keylist_new_key (GPA_KEYLIST (self->keylist), fpr);

  gpa_options_update_default_key (gpa_options_get_instance ());
}


static void
register_key_operation (GpaKeyManager *self, GpaKeyOperation *op)
{
  g_signal_connect_swapped (G_OBJECT (op), "changed_wot",
			    G_CALLBACK (gpa_key_manager_changed_wot_cb),
			    self);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), self);
}


static void
register_import_operation (GpaKeyManager *self, GpaImportOperation *op)
{
  g_signal_connect_swapped (G_OBJECT (op), "imported_keys",
			    G_CALLBACK (gpa_key_manager_changed_wot_cb),
			    self);
  g_signal_connect_swapped
    (G_OBJECT (op), "imported_secret_keys",
     G_CALLBACK (gpa_key_manager_changed_wot_secret_cb),
     self);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), self);
}


static void
register_generate_operation (GpaKeyManager *self, GpaGenKeyOperation *op)
{
  g_signal_connect_swapped (G_OBJECT (op), "generated_key",
			    G_CALLBACK (gpa_key_manager_new_key_cb),
			    self);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), self);
}


static void
register_operation (GpaKeyManager *self, GpaOperation *op)
{
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), self);
}


/* delete the selected keys */
static void
key_manager_delete (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection = gpa_keylist_get_selected_keys (self->keylist,
                                                    GPGME_PROTOCOL_UNKNOWN);
  GpaKeyDeleteOperation *op = gpa_key_delete_operation_new (GTK_WIDGET (self),
							    selection);
  register_key_operation (self, GPA_KEY_OPERATION (op));
}


/* Return true if the public key key has been signed by the key with
   the id key_id, otherwise return FALSE.  */
static gboolean
key_has_been_signed (const gpgme_key_t key,
		     const gpgme_key_t signer_key)
{
  gboolean uid_signed, key_signed;
  const char *signer_id;
  gpgme_key_sig_t sig;
  gpgme_user_id_t uid;

  signer_id = signer_key->subkeys->keyid;
  /* We consider the key signed if all user IDs have been signed.  */
  key_signed = TRUE;
  for (uid = key->uids; key_signed && uid; uid = uid->next)
    {
      uid_signed = FALSE;
      for (sig = uid->signatures; !uid_signed && sig; sig = sig->next)
	if (g_str_equal (signer_id, sig->keyid))
	  uid_signed = TRUE;
      key_signed = key_signed && uid_signed;
    }

  return key_signed;
}


/* Return true if the key sign button should be sensitive, i.e. if
   there is at least one selected key and there is a default key.  */
static gboolean
key_manager_can_sign (gpointer param)
{
  gboolean result = FALSE;
  gpgme_key_t default_key;

  default_key = gpa_options_get_default_key (gpa_options_get_instance ());

  if (default_key && key_manager_has_single_selection (param))
    {
      /* The most important requirements have been met, now check if
	 the selected key was already signed with the default key.  */
      GpaKeyManager *self = param;
      gpgme_key_t key = key_manager_current_key (self);
      if (key->protocol == GPGME_PROTOCOL_OpenPGP)
        result = ! key_has_been_signed (key, default_key);
    }
  else if (default_key && key_manager_has_selection (param))
    /* Always allow signing many keys at once.  */
    result = TRUE;
  return result;
}


/* sign the selected keys */
static void
key_manager_sign (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection;
  GpaKeySignOperation *op;

  if (! gpa_keylist_has_selection (self->keylist))
    {
      /* This shouldn't happen because the button should be grayed out
	 in this case.  */
      gpa_window_error (_("No keys selected for signing."), GTK_WIDGET (self));
      return;
    }

  selection = gpa_keylist_get_selected_keys (self->keylist,
                                             GPGME_PROTOCOL_OpenPGP);
  if (selection)
    {
      op = gpa_key_sign_operation_new (GTK_WIDGET (self), selection);
      register_key_operation (self, GPA_KEY_OPERATION (op));
    }
}

/* Invoke the "edit key" dialog.  */
static void
key_manager_edit (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  gpgme_key_t key;
  GtkWidget *dialog;

  if (! key_manager_has_private_selected (self))
    return;
  key = key_manager_current_key (self);
  if (! key)
    return;

  dialog = gpa_key_edit_dialog_new (GTK_WIDGET (self), key);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  g_signal_connect (G_OBJECT (dialog), "key_modified",
		    G_CALLBACK (gpa_key_manager_key_modified), self);
  gtk_widget_show_all (dialog);
}


static void
key_manager_trust (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection;
  GpaKeyTrustOperation *op;

  /* FIXME: Key trust operation currently does not support more than
     one key at a time.  */
  if (! key_manager_has_single_selection (self))
    return;

  selection = gpa_keylist_get_selected_keys (self->keylist,
                                             GPGME_PROTOCOL_OpenPGP);
  if (selection)
    {
      op = gpa_key_trust_operation_new (GTK_WIDGET (self), selection);
      register_key_operation (self, GPA_KEY_OPERATION (op));
    }
}


/* Import keys.  */
static void
key_manager_import (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GpaImportFileOperation *op;

  op = gpa_import_file_operation_new (GTK_WIDGET (self));
  register_import_operation (self, GPA_IMPORT_OPERATION (op));
}


/* Export the selected keys to a file.  */
static void
key_manager_export (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection;
  GpaExportFileOperation *op;

  selection = gpa_keylist_get_selected_keys (self->keylist,
                                             GPGME_PROTOCOL_UNKNOWN);
  if (! selection)
    return;

  op = gpa_export_file_operation_new (GTK_WIDGET (self), selection);
  register_operation (self, GPA_OPERATION (op));
}


/* Import a key from the keyserver.  */
#ifdef ENABLE_KEYSERVER_SUPPORT
static void
key_manager_retrieve (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GpaImportServerOperation *op;

  op = gpa_import_server_operation_new (GTK_WIDGET (self));
  register_import_operation (self, GPA_IMPORT_OPERATION (op));
}
#endif /*ENABLE_KEYSERVER_SUPPORT*/


/* Refresh keys from the keyserver.  */
#ifdef ENABLE_KEYSERVER_SUPPORT
static void
key_manager_refresh_keys (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GpaImportByKeyidOperation *op;
  GList *selection;

  /* FIXME: The refresh-from-server operation currently only supports
     one key at a time.  */
  if (!key_manager_has_single_selection (self))
    return;

  selection = gpa_keylist_get_selected_keys (self->keylist,
                                             GPGME_PROTOCOL_OPENPGP);
  if (selection)
    {
      op = gpa_import_bykeyid_operation_new (GTK_WIDGET (self),
                                             (gpgme_key_t) selection->data);
      register_import_operation (self, GPA_IMPORT_OPERATION (op));
    }
}
#endif /*ENABLE_KEYSERVER_SUPPORT*/


/* Send a key to the keyserver.  */
#ifdef ENABLE_KEYSERVER_SUPPORT
static void
key_manager_send (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection;
  GpaExportServerOperation *op;

  /* FIXME: The export-to-server operation currently only supports
     exporting one key at a time.  */
  if (! key_manager_has_single_selection (self))
    return;

  selection = gpa_keylist_get_selected_keys (self->keylist,
                                             GPGME_PROTOCOL_OPENPGP);
  if (selection)
    {
      op = gpa_export_server_operation_new (GTK_WIDGET (self), selection);
      register_operation (self, GPA_OPERATION (op));
    }
}
#endif /*ENABLE_KEYSERVER_SUPPORT*/


/* Backup the default keys.  */
static void
key_manager_backup (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  gpgme_key_t key;
  GpaBackupOperation *op;

  if (! key_manager_has_private_selected (self))
    return;
  key = key_manager_current_key (self);
  if (! key)
    return;

  op = gpa_backup_operation_new (GTK_WIDGET (self), key);
  register_operation (self, GPA_OPERATION (op));
}


/* Run the advanced key generation dialog and if the user clicked OK,
   generate a new key pair and update the key list.  */
static void
key_manager_generate_key_advanced (gpointer param)
{
  GpaKeyManager *self = param;
  GpaGenKeyAdvancedOperation *op;

  op = gpa_gen_key_advanced_operation_new (GTK_WIDGET (self));
  register_generate_operation (self, GPA_GEN_KEY_OPERATION (op));
}


/* Call the key generation wizard and update the key list if necessary */
static void
key_manager_generate_key_simple (gpointer param)
{
  GpaKeyManager *self = param;
  GpaGenKeySimpleOperation *op;

  op = gpa_gen_key_simple_operation_new (GTK_WIDGET (self));
  register_generate_operation (self, GPA_GEN_KEY_OPERATION (op));
}


/* Generate a key.  */
static void
key_manager_generate_key (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    key_manager_generate_key_simple (param);
  else
    key_manager_generate_key_advanced (param);
}


/* Update everything that has to be updated when the selection in the
   key list changes.  */
static void
keyring_selection_update_actions (GpaKeyManager *self)
{
  update_selection_sensitive_actions (self);
  keyring_update_details (self);
}


/* Callback for key listings invoked with the "next_key" signal.  Used
   to receive and set the new current key.  */
static void
key_manager_key_listed (GpaContext *ctx, gpgme_key_t key, gpointer param)
{
  GpaKeyManager *self = param;

  gpgme_key_unref (self->current_key);
  self->current_key = key;

  keyring_selection_update_actions (self);
}


/* FIXME: CHECK! Signal handler for selection changes. */
static void
key_manager_selection_changed (GtkTreeSelection *treeselection,
				  gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection;

  /* Some other piece of the keyring wants us to ignore this signal.  */
  if (self->freeze_selection)
    return;

  /* Update the current key.  */
  if (self->current_key)
    {
      /* Remove the previous one.  */
      gpgme_key_unref (self->current_key);
      self->current_key = NULL;
    }

  /* Abort retrieval of the current key.  */
  if (gpa_context_busy (self->ctx))
    gpgme_op_keylist_end (self->ctx->ctx);

  /* Load the new one.  */
  if (gpa_keylist_has_single_selection (self->keylist)
      && (selection = gpa_keylist_get_selected_keys (self->keylist,
                                                     GPGME_PROTOCOL_UNKNOWN)))
    {
      gpg_error_t err;
      gpgme_key_t key;
      int old_mode;

      key = (gpgme_key_t) selection->data;
      old_mode = gpgme_get_keylist_mode (self->ctx->ctx);

      /* With all the signatures and validating for the sake of X.509.
         Note that we should not save and restore the old protocol
         because the protocol should not be changed before the
         gpgme_op_keylist_end.  Saving and restoring the keylist mode
         is okay. */
      gpgme_set_keylist_mode (self->ctx->ctx,
			      (old_mode
#ifdef GPGME_KEYLIST_MODE_WITH_TOFU
                               | GPGME_KEYLIST_MODE_WITH_TOFU
#endif
                               | GPGME_KEYLIST_MODE_SIGS
                               | GPGME_KEYLIST_MODE_VALIDATE));
      gpgme_set_protocol (self->ctx->ctx, key->protocol);
      err = gpgme_op_keylist_start (self->ctx->ctx, key->subkeys->fpr,
				    FALSE);
      if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
	gpa_gpgme_warning (err);

      gpgme_set_keylist_mode (self->ctx->ctx, old_mode);
      g_list_free (selection);

      /* Make sure the actions that depend on a current key are
	 disabled.  */
      disable_selection_sensitive_actions (self);
    }
  else
    keyring_selection_update_actions (self);
}


/* FIXME:  CHECK THIS!
   Signal handler for the map signal.  If the simplified_ui flag is
   set and there's no private key in the key ring, ask the user
   whether he wants to generate a key.  If so, call
   key_manager_generate_key which runs the appropriate dialog.
   Also, if the simplified_ui flag is set, remind the user if he has
   not yet created a backup copy of his private key.  */
static void
key_manager_mapped (gpointer param)
{
  static gboolean asked_about_key_generation = FALSE;
  static gboolean asked_about_key_backup = FALSE;
  GpaKeyManager *self = param;

  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    {
      /* FIXME: We assume that the only reason a user might not have a
         default key is because he has no private keys.  */
      if (! asked_about_key_generation
          && ! gpa_options_have_default_key (gpa_options_get_instance()))
        {
	  GtkWidget *dialog;
	  GtkResponseType response;

	  dialog = gtk_message_dialog_new (GTK_WINDOW (GTK_WIDGET (self)),
					   GTK_DIALOG_MODAL,
					   GTK_MESSAGE_QUESTION,
					   GTK_BUTTONS_NONE,
					   _("You do not have a private key "
					     "yet. Do you want to generate "
					     "one now (recommended) or do it"
					     " later?"));
	  gtk_dialog_add_buttons (GTK_DIALOG (dialog), _("_Generate key now"),
				  GTK_RESPONSE_OK, _("Do it _later"),
				  GTK_RESPONSE_CANCEL, NULL);
	  response = gtk_dialog_run (GTK_DIALOG (dialog));
	  gtk_widget_destroy (dialog);
          if (response == GTK_RESPONSE_OK)
	    key_manager_generate_key (NULL, NULL, param);
	  asked_about_key_generation = TRUE;
        }
      else if (!asked_about_key_backup
               && !gpa_options_get_backup_generated
	       (gpa_options_get_instance ())
               && !gpa_options_have_default_key (gpa_options_get_instance()))
        {
	  GtkWidget *dialog;
	  GtkResponseType response;

	  dialog = gtk_message_dialog_new (GTK_WINDOW (GTK_WIDGET (self)),
					   GTK_DIALOG_MODAL,
					   GTK_MESSAGE_QUESTION,
					   GTK_BUTTONS_NONE,
					   _("You do not have a backup copy of"
					     " your private key yet."
					     " Do you want to backup your key "
					     "now (recommended) or do it "
					     "later?"));
	  gtk_dialog_add_buttons (GTK_DIALOG (dialog), _("_Backup key now"),
				  GTK_RESPONSE_OK, _("Do it _later"),
				  GTK_RESPONSE_CANCEL, NULL);
	  response = gtk_dialog_run (GTK_DIALOG (dialog));
	  gtk_widget_destroy (dialog);
          if (response == GTK_RESPONSE_OK)
	    {
              gpgme_key_t key;
	      GpaBackupOperation *op;

              key = gpa_options_get_default_key (gpa_options_get_instance ());
              if (key)
                {
                  op = gpa_backup_operation_new (GTK_WIDGET (self), key);
                  register_operation (self, GPA_OPERATION (op));
                }
	    }
          asked_about_key_backup = TRUE;
        }
    }
}


/* Close the key manager.  */
static void
key_manager_close (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  gtk_widget_destroy (GTK_WIDGET (this_instance));
}

/* Quit the program */
static void
key_manager_quit (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  g_application_quit (G_APPLICATION (get_gpa_application ()));
}


/* select all keys in the keyring */
static void
key_manager_select_all (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->keylist));
  gtk_tree_selection_select_all (selection);
}


/* Paste the clipboard into the keyring.  */
static void
key_manager_paste (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GpaImportClipboardOperation *op;

  op = gpa_import_clipboard_operation_new (GTK_WIDGET (self));
  register_import_operation (self, GPA_IMPORT_OPERATION (op));
}


/* Copy the keys into the clipboard.  */
static void
key_manager_copy (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection;
  GpaExportClipboardOperation *op;

  selection = gpa_keylist_get_selected_keys (self->keylist,
                                             GPGME_PROTOCOL_UNKNOWN);
  if (! selection)
    return;

  op = gpa_export_clipboard_operation_new (GTK_WIDGET (self), selection, 0);
  register_operation (self, GPA_OPERATION (op));
}


/* Copy the secret keys into the clipboard.  */
static void
key_manager_copy_sec (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection;
  GpaExportClipboardOperation *op;

  selection = gpa_keylist_get_selected_keys (self->keylist,
                                             GPGME_PROTOCOL_UNKNOWN);
  if (!selection || g_list_length (selection) != 1)
    return;

  op = gpa_export_clipboard_operation_new (GTK_WIDGET (self), selection, 1);
  register_operation (self, GPA_OPERATION (op));
}


/* Copy the fingerprints of the keys into the clipboard.  */
static void
key_manager_copy_fpr (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  GpaKeyManager *self = param;
  GList *selection, *item;
  gpgme_protocol_t prot = GPGME_PROTOCOL_UNKNOWN;
  gpgme_key_t key;
  char *buffer = NULL;
  char *p;

  selection = gpa_keylist_get_selected_keys (self->keylist,
                                             GPGME_PROTOCOL_UNKNOWN);
  if (!selection)
    return;

  for (item = selection; item; item = g_list_next (item))
    {
      key = (gpgme_key_t)item->data;

      if (prot == GPGME_PROTOCOL_UNKNOWN)
        prot = key->protocol;
      else if (prot != key->protocol)
        {
          gpa_window_error
            (_("Only keys of the same procotol may be copied."), NULL);
          g_free (buffer);
          buffer = NULL;
          break;
        }
      if (prot == GPGME_PROTOCOL_UNKNOWN)
        ;
      else if (!buffer)
        buffer = g_strconcat (key->subkeys->fpr, "\n", NULL);
      else
        {
          p = g_strconcat (buffer, key->subkeys->fpr, "\n", NULL);
          g_free (buffer);
          buffer = p;
        }
    }
  if (buffer)
    {
      gtk_clipboard_set_text (gtk_clipboard_get
                              (GDK_SELECTION_PRIMARY), buffer, -1);
      gtk_clipboard_set_text (gtk_clipboard_get
                              (GDK_SELECTION_CLIPBOARD), buffer, -1);
    }

  g_free (buffer);
}


/* Reload the key list.  */
static void
key_manager_refresh (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
  GpaKeyManager *self = user_data;

  /* Hack: To force reloading of secret keys we claim that a secret
     key has been imported.  */
  gpa_keylist_imported_secret_key (self->keylist);

  gpa_keylist_start_reload (self->keylist);
}


static void
keyring_set_listing_cb (GSimpleAction *action,
			GtkRadioAction *current_action, gpointer param)
{
  GpaKeyManager *self = param;
  gint detailed;

  detailed = gtk_radio_action_get_current_value
    (GTK_RADIO_ACTION (current_action));

  if (detailed)
    {
      gpa_keylist_set_detailed (self->keylist);
      gpa_options_set_detailed_view (gpa_options_get_instance (), TRUE);
    }
  else
    {
      gpa_keylist_set_brief (self->keylist);
      gpa_options_set_detailed_view (gpa_options_get_instance (), FALSE);
    }
}

/* Create and return the menu bar for the key ring editor.  */
static void
key_manager_action_new (GpaKeyManager *self,
                        GtkWidget **menu, GtkWidget **toolbar,
                        GtkWidget **popup)
{

  static const
    GActionEntry entries[] =
  {
      { "file", NULL },
      { "edit", NULL },
      { "keys", NULL },

      { "file_close", key_manager_close },
      { "file_quit", key_manager_quit },

      { "edit_copy", key_manager_copy },
      { "edit_copy_fingerprint", key_manager_copy_fpr },
      { "edit_copy_sec", key_manager_copy_sec },

      { "edit_paste", key_manager_paste },
      { "edit_select_all", key_manager_select_all },

      { "keys_refresh", key_manager_refresh },
      { "keys_new", key_manager_generate_key },
      { "keys_delete", key_manager_delete },
      { "keys_sign", key_manager_sign },
      { "keys_set_owner_trust", key_manager_trust },
      { "keys_edit_private_key", key_manager_edit },
      { "keys_import_keys", key_manager_import },
      { "keys_export_keys", key_manager_export},
      { "keys_backup_key", key_manager_backup},

#ifdef ENABLE_KEYSERVER_SUPPORT
      { "server_retrive", key_manager_retrieve },
      { "server_refresh", key_manager_refresh_keys },
      { "server_send", key_manager_send },
#endif
  };

  static const GtkRadioActionEntry radio_entries[] =
    {
      { "DetailsBrief", "brief", NULL, NULL,
	N_("Show Brief Keylist"), 0 },
      { "DetailsDetailed", GPA_STOCK_DETAILED, NULL, NULL,
	N_("Show Key Details"), 1 }
    };

  static const char *menu_string =
    "<interface>"
    "<menu id='menu'>"
        "<submenu>"
          "<attribute name='label' translatable='yes'>_File</attribute>"
          "<item>"
            "<attribute name='label' translatable='yes'>Close</attribute>"
            "<attribute name='action'>app.file_close</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Quit</attribute>"
            "<attribute name='action'>app.file_quit</attribute>"
          "</item>"
        "</submenu>"
        "<submenu>"
          "<attribute name='label' translatable='yes'>Edit</attribute>"
          "<item>"
            "<attribute name='label' translatable='yes'>Copy</attribute>"
            "<attribute name='action'>app.edit_copy</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Paste</attribute>"
            "<attribute name='action'>app.edit_paste</attribute>"
          "</item>"
          "<section>"
          "<item>"
            "<attribute name='label' translatable='yes'>Select All</attribute>"
            "<attribute name='action'>app.edit_select_all</attribute>"
          "</item>"
          "</section>"
          "<section>"
          "<item>"
            "<attribute name='label' translatable='yes'>Preferences</attribute>"
            "<attribute name='action'>app.edit_preferences</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Backend Preferences</attribute>"
            "<attribute name='action'>app.edit_backend_preferences</attribute>"
          "</item>"
          "</section>"
        "</submenu>"
        "<submenu>"
          "<attribute name='label' translatable='yes'>Keys</attribute>"
          "<item>"
            "<attribute name='label' translatable='yes'>Refresh</attribute>"
            "<attribute name='action'>app.keys_refresh</attribute>"
          "</item>"
          "<section>"
          "<item>"
            "<attribute name='label' translatable='yes'>New Key</attribute>"
            "<attribute name='action'>app.keys_new</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Delete Keys</attribute>"
            "<attribute name='action'>app.keys_delete</attribute>"
          "</item>"
          "</section>"
          "<section>"
          "<item>"
            "<attribute name='label' translatable='yes'>Sign Keys</attribute>"
            "<attribute name='action'>app.keys_sign</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Set Owner Trust</attribute>"
            "<attribute name='action'>app.keys_set_owner_trust</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Edit Private Key</attribute>"
            "<attribute name='action'>app.keys_edit_private_key</attribute>"
          "</item>"
          "</section>"
          "<section>"
          "<item>"
            "<attribute name='label' translatable='yes'>Import Keys</attribute>"
            "<attribute name='action'>app.keys_import_keys</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Export Keys</attribute>"
            "<attribute name='action'>app.keys_export_keys</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Backup...</attribute>"
            "<attribute name='action'>app.keys_backup_key</attribute>"
          "</item>"
          "</section>"
        "</submenu>"
        "<submenu>"
          "<attribute name='label' translatable='yes'>Windows</attribute>"
          "<item>"
            "<attribute name='label' translatable='yes'>Keyring Manager</attribute>"
            "<attribute name='action'>app.windows_keyring_editor</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>File Manager</attribute>"
            "<attribute name='action'>app.windows_file_manager</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Clipboard</attribute>"
            "<attribute name='action'>app.windows_clipboard</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Card Manager</attribute>"
            "<attribute name='action'>app.windows_card_manager</attribute>"
          "</item>"
        "</submenu>"
#ifdef ENABLE_KEYSERVER_SUPPORT
        "<submenu>"
          "<attribute name='label' translatable='yes'>Server</attribute>"
          "<item>"
            "<attribute name='label' translatable='yes'>Retieve Keys...</attribute>"
            "<attribute name='action'>app.server_retrive</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Send Keys...</attribute>"
            "<attribute name='action'>app.server_send</attribute>"
          "</item>"
        "</submenu>"
#endif // ENABLE_KEYSERVER_SUPPORT
        "<submenu>"
          "<attribute name='label' translatable='yes'>Help</attribute>"
          "<item>"
            "<attribute name='label' translatable='yes'>About</attribute>"
            "<attribute name='action'>app.help_about</attribute>"
          "</item>"
        "</submenu>"
    "</menu>"

    "<object id='toolbar' class='GtkToolbar'>"
      "<property name='visible'>True</property>"
      "<property name='can_focus'>False</property>"
      "<property name='show_arrow'>False</property>"
      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='action-name'>app.keys_edit_private_key</property>"
          "<property name='icon_name'>edit</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Edit private key...</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"
      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='action-name'>app.keys_delete</property>"
          "<property name='icon_name'>delete</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Remove the selected key</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='action-name'>app.keys_sign</property>"
          "<property name='icon_name'>sign</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Sign the Selected Key</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='action-name'>app.keys_import_keys</property>"
          "<property name='icon_name'>import</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Import Keys</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='action-name'>app.keys_export_keys</property>"
          "<property name='icon_name'>export</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Export Keys</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkSeparatorToolItem'>"
        "</object>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton' id='brief_test'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='action-name'>app.windows_file_manager</property>"
          "<property name='icon_name'>brief</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Show Brief Keylist</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton' id='detailed'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='action-name'>app.windows_file_manager</property>"
          "<property name='icon_name'>detailed</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Show Key Details</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkSeparatorToolItem'>"
        "</object>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='icon_name'>preferences-desktop</property>"
          "<property name='action-name'>app.edit_preferences</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Configure the application</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkSeparatorToolItem'>"
        "</object>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='icon_name'>view-refresh</property>"
          "<property name='action-name'>app.keys_refresh</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Refresh the keyring</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkSeparatorToolItem'>"
        "</object>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='icon_name'>folder</property>"
          "<property name='action-name'>app.windows_file_manager</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Open the file manager</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='icon_name'>edit-paste</property>"
          "<property name='action-name'>app.windows_clipboard</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Open the clipboard</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"

      "<child>"
        "<object class='GtkToolButton'>"
          "<property name='visible'>True</property>"
          "<property name='can_focus'>False</property>"
          "<property name='use_underline'>True</property>"
          "<property name='icon_name'>smartcard</property>"
          "<property name='action-name'>app.windows_card_manager</property>"
          "<property name='has_tooltip'>true</property>"
          "<property name='tooltip-text'>Open the card manager</property>"
        "</object>"
        "<packing>"
          "<property name='expand'>False</property>"
          "<property name='homogeneous'>True</property>"
        "</packing>"
      "</child>"
    "</object>"
    "<menu id='popupmenu'>"
        "<section>"
          // "<attribute name='display-hint'>horizontal-buttons</attribute>"
          "<item>"
            "<attribute name='label' translatable='yes'>Copy Fingerprint</attribute>"
            "<attribute name='action'>app.edit_copy_fingerprint</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Copy</attribute>"
            "<attribute name='action'>app.edit_copy</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Paste</attribute>"
            "<attribute name='action'>app.edit_paste</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Delete Keys</attribute>"
            "<attribute name='action'>app.keys_delete</attribute>"
          "</item>"
        "</section>"
        "<section>"
          "<item>"
            "<attribute name='label' translatable='yes'>Sign Keys</attribute>"
            "<attribute name='action'>app.keys_sign</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Set Ownertrust</attribute>"
            "<attribute name='action'>app.keys_set_owner_trust</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Edit Private Key</attribute>"
            "<attribute name='action'>app.keys_edit_private_key</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Copy Private Key</attribute>"
            "<attribute name='action'>app.key_manager_copy_fpr</attribute>"
          "</item>"
        "</section>"
        "<section>"
          "<item>"
            "<attribute name='label' translatable='yes'>Export Keys</attribute>"
            "<attribute name='action'>app.key_manager_export</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Update Keys</attribute>"
            "<attribute name='action'>app.key_manager_refresh</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Send Keys</attribute>"
            "<attribute name='action'>app.server_send</attribute>"
          "</item>"
          "<item>"
            "<attribute name='label' translatable='yes'>Backup</attribute>"
            "<attribute name='action'>app.keys_backup_key</attribute>"
          "</item>"
        "</section>"
      "</menu>"

  "</interface>";

  GError **err;

  GtkBuilder *gtk_builder = gtk_builder_new ();

  if (gtk_builder_add_from_string( gtk_builder, icons_string , -1, err) == 0) {
    printf("ERROR icons: %s \n", (*err)->message);
  }

  if (gtk_builder_add_from_string( gtk_builder, menu_string , -1, err) == 0) {
    printf("ERROR menu: %s \n", (*err)->message);
  }

  GMenuModel *menu_bar_model = G_MENU_MODEL (gtk_builder_get_object (GTK_BUILDER (gtk_builder), "menu"));
  *menu = gtk_menu_bar_new_from_model (menu_bar_model);

  GMenuModel *popup_menu_model = G_MENU_MODEL (gtk_builder_get_object (GTK_BUILDER (gtk_builder), "popupmenu"));
  *popup = gtk_menu_new_from_model (popup_menu_model);

  GtkWidget *grid = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (gtk_builder), "toolbar"));

  GtkCssProvider *css_provider = gtk_css_provider_new();
  GdkDisplay *display = gdk_display_get_default();
  GdkScreen *screen = gdk_display_get_default_screen (display);
  gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_css_provider_load_from_data(css_provider,
                                        "#toolbar {\n"
                                        //" padding-left: 55px;\n"
                                        // " padding-right: 5px;\n"
                                        // " border-radius: 3px;\n"
                                        "}\n", -1, NULL);

  GtkStyleContext *style_context;

  style_context = gtk_widget_get_style_context (GTK_WIDGET (grid));


  //gtk_widget_add_css_class (grid, "toolbar");

  // We must set the name to the toolbar for css to recognize it
  gtk_widget_set_name(GTK_WIDGET(grid), "toolbar");

  gtk_style_context_add_class (style_context, "toolbar");

  GApplication *gpa_app = G_APPLICATION (get_gpa_application ());


  g_action_map_add_action_entries (G_ACTION_MAP (gpa_app),
                                    gpa_windows_menu_g_action_entries,
                                    G_N_ELEMENTS (gpa_windows_menu_g_action_entries),
                                    self);

  g_action_map_add_action_entries (G_ACTION_MAP (gpa_app),
                                    entries,
                                    G_N_ELEMENTS (entries),
                                    self);

  g_action_map_add_action_entries (G_ACTION_MAP (gpa_app),
                                    gpa_help_menu_g_action_entries,
                                    G_N_ELEMENTS (gpa_help_menu_g_action_entries),
                                    self);

  g_action_map_add_action_entries (G_ACTION_MAP (gpa_app),
                                    gpa_preferences_menu_g_action_entries,
                                    G_N_ELEMENTS (gpa_preferences_menu_g_action_entries),
                                    self);


  GSimpleAction *action;
  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "edit_copy");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_selection);

  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "edit_copy_fingerprint");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_single_selection);

  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "edit_copy_sec");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_single_selection);

  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "keys_delete");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_selection);

  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "keys_export_keys");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_selection);


#ifdef ENABLE_KEYSERVER_SUPPORT

  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "server_refresh");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_single_selection);

  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "server_send");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_single_selection);

#endif // ENABLE_KEYSERVER_SUPPORT


  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "keys_set_owner_trust");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_single_selection_OpenPGP);

  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "keys_sign");
  add_selection_sensitive_action (self, action,
                                  key_manager_can_sign);

  action = (GSimpleAction*)g_action_map_lookup_action (G_ACTION_MAP (gpa_app), "keys_edit_private_key");
  add_selection_sensitive_action (self, action,
                                  key_manager_has_private_selected);

  *toolbar = GTK_WIDGET (grid);
}


/* Update the details widget according to the current selection.  This
   means that if there is exactly one key selected, display its
   properties in the pages, otherwise show the number of currently
   selected keys.  */
static int
idle_update_details (gpointer param)
{
  GpaKeyManager *self = param;

  if (gpa_keylist_has_single_selection (self->keylist))
    {
      gpgme_key_t key = key_manager_current_key (self);
      if (! key)
	{
	  /* There is a single key selected, but the current key is
	     NULL.  This means the key has not been returned yet, so
	     we exit the function asking GTK to run it again when
	     there is time.  */
	  return TRUE;
	}
      gpa_key_details_update (self->details, key, 1);
    }
  else
    {
      GList *selection = gpa_keylist_get_selected_keys (self->keylist,
                                                        GPGME_PROTOCOL_UNKNOWN);
      if (selection)
        {
          gpa_key_details_update (self->details, NULL,
                                  g_list_length (selection));
          g_list_free (selection);
        }
    }

  /* Set the idle id to NULL to indicate that the idle handler has
     been run.  */
  self->details_idle_id = 0;

  /* Return false to indicate that this function shouldn't be called
     again by GTK, only when we expicitly add it again.  */
  return FALSE;
}


/* Add an idle handler to update the details, but only when none has
   been set yet.  */
static void
keyring_update_details (GpaKeyManager *self)
{
  if (! self->details_idle_id)
    self->details_idle_id = g_idle_add (idle_update_details, self);
}



/* Status bar handling.  */
static GtkWidget *
keyring_statusbar_new (GpaKeyManager *self)
{
  GtkWidget *hbox;
  GtkWidget *label;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  label = gtk_label_new ("");
  self->status_label = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("");
  self->status_key_id = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

  label = gtk_label_new ("");
  self->status_key_user = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  return hbox;
}


/* Update the status bar.  */
static void
keyring_update_status_bar (GpaKeyManager *self)
{
  gpgme_key_t key = gpa_options_get_default_key (gpa_options_get_instance ());

  if (key)
    {
      gchar *string;

      gtk_label_set_text (GTK_LABEL (self->status_label),
			  _("Selected default key:"));
      string = gpa_gpgme_key_get_userid (key->uids);
      gtk_label_set_text (GTK_LABEL (self->status_key_user), string);
      g_free (string);
      gtk_label_set_text (GTK_LABEL (self->status_key_id),
                          gpa_gpgme_key_get_short_keyid (key));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (self->status_label),
			  _("No default key selected in the preferences."));
      gtk_label_set_text (GTK_LABEL (self->status_key_user), "");
      gtk_label_set_text (GTK_LABEL (self->status_key_id), "");
    }
}


/* FIXME: Check. */
/* The context menu of the keyring list.  This is the callback for the
   "button_press_event" signal.  */
static gint
display_popup_menu (GpaKeyManager *self, GdkEvent *event, GpaKeyList *list)
{
  GtkMenu *menu;
  GdkEventButton *event_button;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  menu = GTK_MENU (self->popup_menu);

  if (event->type == GDK_BUTTON_PRESS)
  {
    event_button = (GdkEventButton *) event;
    if (event_button->button == 3)
    {
      GtkTreeSelection *selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
      GtkTreePath *path;
      GtkTreeIter iter;
          /* Make sure the clicked key is selected.  */
      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list),
					     event_button->x,
					     event_button->y,
					     &path, NULL,
					     NULL, NULL))
      {
	      gtk_tree_model_get_iter (gtk_tree_view_get_model
				       (GTK_TREE_VIEW(list)), &iter, path);

        if (! gtk_tree_selection_iter_is_selected (selection, &iter))
        {
          /* Block selection updates.  */
          self->freeze_selection++;
          gtk_tree_selection_unselect_all (selection);
          self->freeze_selection--;
          gtk_tree_selection_select_path (selection, path);
        }

        gtk_menu_popup_at_pointer (menu, NULL);

      }

      return TRUE;
    }
  }

  return FALSE;
}


/* Signal handler for the "changed_default_key" signal.  */
static void
keyring_default_key_changed (GpaOptions *options, gpointer param)
{
  GpaKeyManager *self = param;

  /* Update the status bar and the selection sensitive widgets because
     some depend on the default key.  */
  keyring_update_status_bar (self);
  update_selection_sensitive_actions (self);
}


/* Create all the widgets of this window.  */
static void
construct_widgets (GpaKeyManager *self)
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scrolled;
  GtkWidget *keylist;
  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkWidget *hbox;
  GtkWidget *icon;
  GtkWidget *paned;
  GtkWidget *statusbar;
  GtkWidget *main_box;
  GtkWidget *align;
  gchar *markup;
  guint pt, pb, pl, pr;

  gpa_window_set_title (GTK_WINDOW (self), _("Key Manager"));
  gtk_window_set_default_size (GTK_WINDOW (self), 680, 600);


  g_signal_connect_swapped (G_OBJECT (self), "map",
			    G_CALLBACK (key_manager_mapped), self);


  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), vbox);

  /* FIXME: Check next line.  */
  key_manager_action_new (self, &menubar, &toolbar, &self->popup_menu);

  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);

  /* Add a fancy label that tells us: This is the key manager.  */
  /* FIXME: We should have a common function for this.  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);

  icon = gtk_image_new_from_resource ("/org/gnupg/gpa/keyringeditor.xpm");
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);

  label = gtk_label_new (NULL);
  markup = g_strdup_printf ("<span font_desc=\"16\">%s</span>",
                            _("Key Manager"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 10);
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
  gtk_widget_set_valign (GTK_WIDGET (label), GTK_ALIGN_CENTER);

  paned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);

  main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  align = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_alignment_get_padding (GTK_ALIGNMENT (align), &pt, &pb, &pl, &pr);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), pt, pb + 5,
                             pl + 5, pr + 5);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (main_box), paned, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (align), main_box);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_pack1 (GTK_PANED (paned), scrolled, TRUE, TRUE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  /* FIXME: Which shadow type - get it from a global resource?  */
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_IN);

  keylist = gpa_keylist_new (GTK_WIDGET (self));
  self->keylist = GPA_KEYLIST (keylist);
  if (gpa_options_get_detailed_view (gpa_options_get_instance()))
    gpa_keylist_set_detailed (self->keylist);
  else
    gpa_keylist_set_brief (self->keylist);

  gtk_container_add (GTK_CONTAINER (scrolled), keylist);

  gtk_menu_attach_to_widget (GTK_MENU (self->popup_menu), GTK_WIDGET (keylist), NULL);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection
			      (GTK_TREE_VIEW (keylist))),
		    "changed", G_CALLBACK (key_manager_selection_changed),
		    self);

  g_signal_connect_swapped (G_OBJECT (keylist), "button_press_event",
                            G_CALLBACK (display_popup_menu), self);

  self->details = gpa_key_details_new ();
  gtk_paned_pack2 (GTK_PANED (paned), self->details, TRUE, TRUE);
  gtk_paned_set_position (GTK_PANED (paned), 250);

  statusbar = keyring_statusbar_new (self);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);
  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_default_key",
                    G_CALLBACK (keyring_default_key_changed), self);

  keyring_update_status_bar (self);
  update_selection_sensitive_actions (self);
  keyring_update_details (self);

  self->current_key = NULL;
  self->ctx = gpa_context_new ();
  self->freeze_selection = 0;

  g_signal_connect (G_OBJECT (self->ctx), "next_key",
		    G_CALLBACK (key_manager_key_listed), self);

}


/* Callback for the "destroy" signal.  */
static void
gpa_key_manager_closed (GtkWidget *widget, gpointer param)
{
  this_instance = NULL;
}



/************************************************************
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_key_manager_class_init (void *class_ptr, void *class_data)
{
  GpaKeyManagerClass *klass = class_ptr;

  G_OBJECT_CLASS (klass)->finalize = gpa_key_manager_finalize;
}


static void
gpa_key_manager_init (GTypeInstance *instance, void *class_ptr)
{
  GpaKeyManager *self = GPA_KEY_MANAGER (instance);

  construct_widgets (self);

  g_signal_connect (self, "destroy",
                    G_CALLBACK (gpa_key_manager_closed), self);

}


static void
gpa_key_manager_finalize (GObject *object)
{
  GpaKeyManager *self = GPA_KEY_MANAGER (object);

  g_list_free (self->selection_sensitive_actions);
  self->selection_sensitive_actions = NULL;

  G_OBJECT_CLASS (g_type_class_peek_parent
                  (GPA_KEY_MANAGER_GET_CLASS (self)))->finalize (object);
}


GType
gpa_key_manager_get_type (void)
{
  static GType this_type = 0;

  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaKeyManagerClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  gpa_key_manager_class_init,
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  sizeof (GpaKeyManager),
	  0,    /* n_preallocs */
	  gpa_key_manager_init,
	};

      this_type = g_type_register_static (GTK_TYPE_WINDOW,
                                          "GpaKeyManager",
                                          &this_info, 0);
    }

  return this_type;
}


/************************************************************
 **********************  Public API  ************************
 ************************************************************/

/* Return the instance of the the key manager.  If a new instance has
   been created, TRUE is stored at R_CREATED, otherwise FALSE.  */
GtkWidget *
gpa_key_manager_get_instance (gboolean *r_created)
{
  if (r_created)
    *r_created = !this_instance;
  if (!this_instance)
    {
      this_instance = g_object_new (GPA_KEY_MANAGER_TYPE, NULL);
      /*FIXME      load_all_keys (this_instance);*/
    }
  return GTK_WIDGET (this_instance);
}


gboolean
gpa_key_manager_is_open (void)
{
  return !!this_instance;
}


/* Return true if we should ask for a first time key generation.
 *
 * This function basically duplicates the conditions from
 * key_manager_mapped.  However that function mus be used from a
 * key_manager context and can't easily be used from other GPA
 * components.  */
gboolean
key_manager_maybe_firsttime (void)
{
  if (!gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    return FALSE;

  if (!gpa_options_have_default_key (gpa_options_get_instance()))
    return TRUE;

  if (!gpa_options_get_backup_generated (gpa_options_get_instance ()))
    return TRUE;

  return FALSE;
}
