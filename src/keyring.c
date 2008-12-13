/* keyring.c - The GNU Privacy Assistant keyring.
   Copyright (C) 2000, 2001 G-N-U GmbH.
   Copyright (C) 2005, 2008 g10 Code GmbH.

   This file is part of GPA.

   GPA is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GPA is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpgme.h>

#include "gpa.h"
#include "gtktools.h"
#include "icons.h"
#include "helpmenu.h"
#include "gpapastrings.h"
#include "gpawidgets.h"
#include "ownertrustdlg.h"
#include "keysigndlg.h"
#include "keygendlg.h"
#include "keygenwizard.h"
#include "keyeditdlg.h"
#include "keydeletedlg.h"
#include "keylist.h"
#include "siglist.h"
#include "keyring.h"
#include "gpgmetools.h"
#include "gpgmeedit.h"
#include "keytable.h"
#include "server_access.h"
#include "options.h"

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

#include "gpabackupop.h"

#include "gpagenkeyadvop.h"
#include "gpagenkeysimpleop.h"

#if ! GTK_CHECK_VERSION (2, 10, 0)
#define GTK_STOCK_SELECT_ALL "gtk-select-all"
#endif


/* The public keyring editor.  */

/* Constants for the pages in the details notebook.  */
enum
  {
    GPA_KEYRING_EDITOR_DETAILS,
    GPA_KEYRING_EDITOR_SIGNATURES,
    GPA_KEYRING_EDITOR_SUBKEYS,
  };


/* Struct passed to all signal handlers of the keyring editor as user
   data.  */
struct _GPAKeyringEditor
{
  /* The toplevel window of the editor.  */
  GtkWidget *window;

  /* The central list of keys.  */
  GpaKeyList *keylist;

  /* The "Show Ownertrust" toggle button.  */
  GtkWidget *toggle_show;

  /* The details notebook.  */
  GtkWidget *notebook_details;

  /* Idle handler id for updates of the notebook.  Will be nonzero
     whenever a handler is currently set and zero otherwise.  */
  guint details_idle_id;

  /* Widgets in the details notebook page.  */
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

  /* The signatures list in the notebook.  */
  GtkWidget *signatures_list;
  GtkWidget *signatures_uids;
  gint signatures_count;
  GtkWidget *signatures_hbox;

  /* The subkeys list in the notebook.  */
  GtkWidget *subkeys_list;
  GtkWidget *subkeys_page;
  
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

typedef struct _GPAKeyringEditor GPAKeyringEditor;


/* Internal API.  */

/* Forward declarations.  */
static int idle_update_details (gpointer param);
static void keyring_update_details_notebook (GPAKeyringEditor *editor);


/* A simple sensitivity callback mechanism.

   The basic idea is that buttons (and other widgets like menu items
   as well) should know when they should be sensitive or not.  The
   implementation here is very simple and quite specific for the
   keyring editor's needs.
  
   We maintain a list of sensitive widgets each of which has a
   sensitivity callback associated with them as the "gpa_sensitivity"
   data.  The callback returns TRUE when the widget should be
   sensitive and FALSE otherwise.
  
   Whenever the selection in the key list widget changes we call
   update_selection_sensitive_actions which iterates through the
   widgets in the list, calls the sensitivity callback and changes the
   widget's sensitivity accordingly.  */

/* Prototype of a sensitivity callback.  Return TRUE if the widget
   should be senstitive, FALSE otherwise.  The parameter is a pointer
   to the GPAKeyringEditor struct.  */
typedef gboolean (*SensitivityFunc) (gpointer);


/* Add widget to the list of sensitive widgets of editor.  */
static void
add_selection_sensitive_action (GPAKeyringEditor *editor,
                                GtkAction *action,
                                SensitivityFunc callback)
{
  g_object_set_data (G_OBJECT (action), "gpa_sensitivity", callback);
  editor->selection_sensitive_actions
    = g_list_append (editor->selection_sensitive_actions, action);
}


/* Update the sensitivity of the widget data and pass param through to
   the sensitivity callback.  Usable as iterator function in
   g_list_foreach.  */
static void
update_selection_sensitive_action (gpointer data, gpointer param)
{
  SensitivityFunc func;

  func = g_object_get_data (G_OBJECT (data), "gpa_sensitivity");
  gtk_action_set_sensitive (GTK_ACTION (data), func (param));
}


/* Call update_selection_sensitive_widget for all widgets in the list
   of sensitive widgets and pass editor through as the user data
   parameter.  */
static void
update_selection_sensitive_actions (GPAKeyringEditor *editor)
{
  g_list_foreach (editor->selection_sensitive_actions,
                  update_selection_sensitive_action, editor);
}

/* Disable all the widgets in the list of sensitive widgets.  To be used while
   the selection changes.  */
static void
disable_selection_sensitive_actions (GPAKeyringEditor *editor)
{
  GList *cur;
  
  cur = editor->selection_sensitive_actions;
  while (cur)
    {
      gtk_action_set_sensitive (GTK_ACTION (cur->data), FALSE);
      cur = g_list_next (cur);
    }
}


/* Return TRUE if the key list widget of the keyring editor has at
   least one selected item.  Usable as a sensitivity callback.  */
static gboolean
keyring_editor_has_selection (gpointer param)
{
  GPAKeyringEditor *editor = param;

  return gpa_keylist_has_selection (editor->keylist);
}


/* Return TRUE if the key list widget of the keyring editor has
   exactly one selected item.  Usable as a sensitivity callback.  */
static gboolean
keyring_editor_has_single_selection (gpointer param)
{
  GPAKeyringEditor *editor = param;

  return gpa_keylist_has_single_selection (editor->keylist);
}

/* Return TRUE if the key list widget of the keyring editor has
   exactly one selected OpenPGP item.  Usable as a sensitivity
   callback.  */
static gboolean
keyring_editor_has_single_selection_OpenPGP (gpointer param)
{
  GPAKeyringEditor *editor = param;
  int result = 0;

  if (gpa_keylist_has_single_selection (editor->keylist))
    {
      gpgme_key_t key = gpa_keylist_get_selected_key (editor->keylist);
      if (key && key->protocol == GPGME_PROTOCOL_OpenPGP)
        result = 1;
      gpgme_key_unref (key);
    }

  return result;
}

/* Return TRUE if the key list widget of the keyring editor has
   exactly one selected item and it is a private key.  Usable as a
   sensitivity callback.  */
static gboolean
keyring_editor_has_private_selected (gpointer param)
{
  GPAKeyringEditor *editor = param;

  return gpa_keylist_has_single_secret_selection 
    (GPA_KEYLIST(editor->keylist));
}


/* Return the the currently selected key. NULL if no key is selected.  */
static gpgme_key_t
keyring_editor_current_key (GPAKeyringEditor *editor)
{
  return editor->current_key;
}


/* Operations.  */

static void
gpa_keyring_editor_changed_wot_cb (gpointer data)
{
  GPAKeyringEditor *editor = data;
  gpa_keylist_start_reload (editor->keylist);  
}


static void
gpa_keyring_editor_changed_wot_secret_cb (gpointer data)
{
  GPAKeyringEditor *editor = data;

  gpa_keylist_imported_secret_key (editor->keylist);
  gpa_keylist_start_reload (editor->keylist);  
}

static void
gpa_keyring_editor_key_modified (GpaKeyEditDialog *dialog, gpgme_key_t key,
				 gpointer data)
{
  GPAKeyringEditor *editor = data;
  gpa_keylist_start_reload (editor->keylist);  
}


static void
gpa_keyring_editor_new_key_cb (gpointer data, const gchar *fpr)
{
  GPAKeyringEditor *editor = data;
  
  gpa_keylist_new_key (GPA_KEYLIST (editor->keylist), fpr);

  gpa_options_update_default_key (gpa_options_get_instance ());
}


static void
register_key_operation (GPAKeyringEditor *editor, GpaKeyOperation *op)
{
  g_signal_connect_swapped (G_OBJECT (op), "changed_wot",
			    G_CALLBACK (gpa_keyring_editor_changed_wot_cb),
			    editor);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), editor); 
}


static void
register_import_operation (GPAKeyringEditor *editor, GpaImportOperation *op)
{
  g_signal_connect_swapped (G_OBJECT (op), "imported_keys",
			    G_CALLBACK (gpa_keyring_editor_changed_wot_cb),
			    editor);
  g_signal_connect_swapped 
    (G_OBJECT (op), "imported_secret_keys",
     G_CALLBACK (gpa_keyring_editor_changed_wot_secret_cb),
     editor);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), editor); 
}


static void
register_generate_operation (GPAKeyringEditor *editor, GpaGenKeyOperation *op)
{
  g_signal_connect_swapped (G_OBJECT (op), "generated_key",
			    G_CALLBACK (gpa_keyring_editor_new_key_cb),
			    editor);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), editor); 
}


static void
register_operation (GPAKeyringEditor *editor, GpaOperation *op)
{
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), editor); 
}


/* delete the selected keys */
static void
keyring_editor_delete (GtkAction *action, GPAKeyringEditor *editor)
{
  GList *selection = gpa_keylist_get_selected_keys (editor->keylist);
  GpaKeyDeleteOperation *op = gpa_key_delete_operation_new (editor->window,
							    selection);
  register_key_operation (editor, GPA_KEY_OPERATION (op));
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
keyring_editor_can_sign (gpointer param)
{
  gboolean result = FALSE;
  gpgme_key_t default_key;

  default_key = gpa_options_get_default_key (gpa_options_get_instance ());

  if (default_key && keyring_editor_has_single_selection (param))
    {
      /* The most important requirements have been met, now check if
	 the selected key was already signed with the default key.  */
      GPAKeyringEditor *editor = param;
      gpgme_key_t key = keyring_editor_current_key (editor);
      result = ! key_has_been_signed (key, default_key);
    }
  else if (default_key && keyring_editor_has_selection (param))
    /* Always allow signing many keys at once.  */
    result = TRUE;
  return result;
}


/* sign the selected keys */
static void
keyring_editor_sign (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GList *selection;
  GpaKeySignOperation *op;

  if (! gpa_keylist_has_selection (editor->keylist))
    {
      /* This shouldn't happen because the button should be grayed out
	 in this case.  */
      gpa_window_error (_("No keys selected for signing."), editor->window);
      return;
    }

  selection = gpa_keylist_get_selected_keys (editor->keylist);
  op = gpa_key_sign_operation_new (editor->window, selection);
  register_key_operation (editor, GPA_KEY_OPERATION (op));
}

/* Invoke the "edit key" dialog.  */
static void
keyring_editor_edit (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  gpgme_key_t key;
  GtkWidget *dialog;

  if (! keyring_editor_has_private_selected (editor))
    return;
  key = keyring_editor_current_key (editor);
  if (! key)
    return;

  dialog = gpa_key_edit_dialog_new (editor->window, key);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
      
  g_signal_connect (G_OBJECT (dialog), "key_modified",
		    G_CALLBACK (gpa_keyring_editor_key_modified), editor);
  gtk_widget_show_all (dialog);
}


static void
keyring_editor_trust (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GList *selection;
  GpaKeyTrustOperation *op;

  /* FIXME: Key trust operation currently does not support more than
     one key at a time.  */
  if (! keyring_editor_has_single_selection (editor))
    return;

  selection = gpa_keylist_get_selected_keys (editor->keylist);
  op = gpa_key_trust_operation_new (editor->window, selection);
  register_key_operation (editor, GPA_KEY_OPERATION (op));
}


/* Import keys.  */
static void
keyring_editor_import (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GpaImportFileOperation *op;

  op = gpa_import_file_operation_new (editor->window);
  register_import_operation (editor, GPA_IMPORT_OPERATION (op));
}


/* Export the selected keys to a file.  */
static void
keyring_editor_export (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GList *selection;
  GpaExportFileOperation *op;

  selection = gpa_keylist_get_selected_keys (editor->keylist);
  if (! selection)
    return;

  op = gpa_export_file_operation_new (editor->window, selection);
  register_operation (editor, GPA_OPERATION (op));
}


/* Import a key from the keyserver.  */
static void
keyring_editor_retrieve (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GpaImportServerOperation *op;

  op = gpa_import_server_operation_new (editor->window);
  register_import_operation (editor, GPA_IMPORT_OPERATION (op));
}


/* Send a key to the keyserver.  */
static void
keyring_editor_send (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GList *selection;
  GpaExportServerOperation *op;

  /* FIXME: The export-to-server operation currently only supports
     exporting one key at a time.  */
  if (! keyring_editor_has_single_selection (editor))
    return;

  selection = gpa_keylist_get_selected_keys (editor->keylist);
  op = gpa_export_server_operation_new (editor->window, selection);
  register_operation (editor, GPA_OPERATION (op));
}


/* Backup the default keys.  */
static void
keyring_editor_backup (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  gpgme_key_t key;
  GpaBackupOperation *op;

  if (! keyring_editor_has_private_selected (editor))
    return;
  key = keyring_editor_current_key (editor);
  if (! key)
    return;

  op = gpa_backup_operation_new (editor->window, key);
  register_operation (editor, GPA_OPERATION (op));
}


/* Run the advanced key generation dialog and if the user clicked OK,
   generate a new key pair and update the key list.  */
static void
keyring_editor_generate_key_advanced (gpointer param)
{
  GPAKeyringEditor *editor = param;
  GpaGenKeyAdvancedOperation *op;

  op = gpa_gen_key_advanced_operation_new (editor->window);
  register_generate_operation (editor, GPA_GEN_KEY_OPERATION (op));
}


/* Call the key generation wizard and update the key list if necessary */
static void
keyring_editor_generate_key_simple (gpointer param)
{
  GPAKeyringEditor *editor = param;
  GpaGenKeySimpleOperation *op;

  op = gpa_gen_key_simple_operation_new (editor->window);
  register_generate_operation (editor, GPA_GEN_KEY_OPERATION (op));
}


/* Generate a key.  */
static void
keyring_editor_generate_key (GtkAction *action, gpointer param)
{
  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    keyring_editor_generate_key_simple (param);
  else
    keyring_editor_generate_key_advanced (param);
}


/* Update everything that has to be updated when the selection in the
   key list changes.  */
static void
keyring_selection_update_actions (GPAKeyringEditor *editor)
{
  update_selection_sensitive_actions (editor);
  keyring_update_details_notebook (editor);
}  


/* Callback for key listings.  Used to receive and set the new current
   key.  */
static void
keyring_editor_key_listed (GpaContext *ctx, gpgme_key_t key, gpointer param)
{
  GPAKeyringEditor *editor = param;

  if (editor->current_key)
    gpgme_key_unref (editor->current_key);
  editor->current_key = key;

  keyring_selection_update_actions (editor);
}


/* Signal handler for selection changes.  */
static void
keyring_editor_selection_changed (GtkTreeSelection *treeselection, 
				  gpointer param)
{
  GPAKeyringEditor *editor = param;
  
  /* Some other piece of the keyring wants us to ignore this signal.  */
  if (editor->freeze_selection)
    return;

  /* Update the current key.  */
  if (editor->current_key)
    {
      /* Remove the previous one.  */
      gpgme_key_unref (editor->current_key);
      editor->current_key = NULL;
    }

  /* Abort retrieval of the current key.  */
  if (gpa_context_busy (editor->ctx))
    gpgme_op_keylist_end (editor->ctx->ctx);

  /* Load the new one.  */
  if (gpa_keylist_has_single_selection (editor->keylist)) 
    {
      gpg_error_t err;
      GList *selection;
      gpgme_key_t key;
      int old_mode;

      selection = gpa_keylist_get_selected_keys (editor->keylist);
      key = (gpgme_key_t) selection->data;
      old_mode = gpgme_get_keylist_mode (editor->ctx->ctx);

      /* With all the signatures.  Note that we should not save and
         restore the old protocol because the protocol should not be
         changed before the gpgme_op_keylist_end.  Saving and
         restoring the keylist mode is okay. */
      gpgme_set_keylist_mode (editor->ctx->ctx, 
			      old_mode | GPGME_KEYLIST_MODE_SIGS);
      gpgme_set_protocol (editor->ctx->ctx, key->protocol);
      err = gpgme_op_keylist_start (editor->ctx->ctx, key->subkeys->fpr, 
				    FALSE);
      if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
	gpa_gpgme_warning (err);

      gpgme_set_keylist_mode (editor->ctx->ctx, old_mode);
      g_list_free (selection);

      /* Make sure the actions that depend on a current key are
	 disabled.  */
      disable_selection_sensitive_actions (editor);
    }
  else
    keyring_selection_update_actions (editor);
}

/* Signal handler for the map signal.  If the simplified_ui flag is
   set and there's no private key in the key ring, ask the user
   whether he wants to generate a key.  If so, call
   keyring_editor_generate_key which runs the appropriate dialog.
   Also, if the simplified_ui flag is set, remind the user if he has
   not yet created a backup copy of his private key.  */
static void
keyring_editor_mapped (gpointer param)
{
  static gboolean asked_about_key_generation = FALSE;
  static gboolean asked_about_key_backup = FALSE;
  GPAKeyringEditor *editor = param;

  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    {
      /* FIXME: We assume that the only reason a user might not have a
         default key is because he has no private keys.  */
      if (! asked_about_key_generation
          && ! gpa_options_get_default_key (gpa_options_get_instance()))
        {
	  GtkWidget *dialog;
	  GtkResponseType response;

	  dialog = gtk_message_dialog_new (GTK_WINDOW (editor->window),
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
	    keyring_editor_generate_key (NULL, param);
	  asked_about_key_generation = TRUE;
        }
      else if (!asked_about_key_backup
               && !gpa_options_get_backup_generated 
	       (gpa_options_get_instance ())
               && !gpa_options_get_default_key (gpa_options_get_instance()))
        {
	  GtkWidget *dialog;
	  GtkResponseType response;

	  dialog = gtk_message_dialog_new (GTK_WINDOW (editor->window),
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
	      GpaBackupOperation *op = gpa_backup_operation_new 
		(editor->window, gpa_options_get_default_key 
		 (gpa_options_get_instance ()));
	      register_operation (editor, GPA_OPERATION (op));
	    }
          asked_about_key_backup = TRUE;
        }
    }
}


/* Close the keyring editor.  */
static void
keyring_editor_close (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;

  gtk_widget_destroy (editor->window);
}


/* Free the data structures associated with the keyring editor.  */
static void
keyring_editor_destroy (gpointer param)
{
  GPAKeyringEditor *editor = param;

  g_list_free (editor->selection_sensitive_actions);
  g_free (editor);
}


/* select all keys in the keyring */
static void
keyring_editor_select_all (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->keylist));
  gtk_tree_selection_select_all (selection);
}


/* Paste the clipboard into the keyring.  */
static void
keyring_editor_paste (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GpaImportClipboardOperation *op;

  op = gpa_import_clipboard_operation_new (editor->window);
  register_import_operation (editor, GPA_IMPORT_OPERATION (op));
}


/* Copy the keys into the clipboard.  */
static void
keyring_editor_copy (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  GList *selection;
  GpaExportClipboardOperation *op;

  selection = gpa_keylist_get_selected_keys (editor->keylist);
  if (! selection)
    return;

  op = gpa_export_clipboard_operation_new (editor->window, selection);
  register_operation (editor, GPA_OPERATION (op));
}


/* Reload the key list.  */
static void
keyring_editor_refresh (GtkAction *action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  
  gpa_keylist_start_reload (editor->keylist);
}


static void
keyring_set_listing_cb (GtkAction *action,
			GtkRadioAction *current_action, gpointer param)
{
  GPAKeyringEditor *editor = param;
  gint detailed;

  detailed = gtk_radio_action_get_current_value
    (GTK_RADIO_ACTION (current_action));

  if (detailed)
    {
      gpa_keylist_set_detailed (editor->keylist);
      gpa_options_set_detailed_view (gpa_options_get_instance (), TRUE);
    }
  else
    {
      gpa_keylist_set_brief (editor->keylist);
      gpa_options_set_detailed_view (gpa_options_get_instance (), FALSE);
    }
}


/* Create and return the menu bar for the key ring editor.  */
static void
keyring_editor_action_new (GPAKeyringEditor *editor,
			   GtkWidget **menu, GtkWidget **toolbar,
			   GtkWidget **popup)
{
  static const GtkActionEntry entries[] =
    {
      /* Toplevel.  */
      { "File", NULL, N_("_File"), NULL },
      { "Edit", NULL, N_("_Edit"), NULL },
      { "Keys", NULL, N_("_Keys"), NULL },
      { "Server", NULL, N_("_Server"), NULL },

      /* File menu.  */
      { "FileClose", GTK_STOCK_CLOSE, NULL, NULL,
	"FIXME", G_CALLBACK (keyring_editor_close) },
      { "FileQuit", GTK_STOCK_QUIT, NULL, NULL,
	N_("Quit the program"), G_CALLBACK (gtk_main_quit) },

      /* Edit menu.  */
      { "EditCopy", GTK_STOCK_COPY, NULL, NULL,
	N_("Copy the selection"), G_CALLBACK (keyring_editor_copy) },
      { "EditPaste", GTK_STOCK_PASTE, NULL, NULL,
	N_("Paste the clipboard"), G_CALLBACK (keyring_editor_paste) },
      { "EditSelectAll", GTK_STOCK_SELECT_ALL, NULL, "<control>A",
	N_("Select all certificates"),
	G_CALLBACK (keyring_editor_select_all) },

      /* Keys menu.  */
      { "KeysRefresh", GTK_STOCK_REFRESH, NULL, NULL,
	N_("Refresh the keyring"), G_CALLBACK (keyring_editor_refresh) },
      { "KeysNew", GTK_STOCK_NEW, N_("_New key..."), NULL,
	N_("Generate a new key"), G_CALLBACK (keyring_editor_generate_key) },
      { "KeysDelete", GTK_STOCK_DELETE, N_("_Delete keys"), NULL,
	N_("Remove the selected key"), G_CALLBACK (keyring_editor_delete) },
      { "KeysSign", GPA_STOCK_SIGN, N_("_Sign Keys..."), NULL,
	N_("Sign the selected key"), G_CALLBACK (keyring_editor_sign) },
      { "KeysSetOwnerTrust", NULL, N_("Set _Owner Trust..."), NULL,
	N_("Set owner trust of the selected key"),
	G_CALLBACK (keyring_editor_trust) },
      { "KeysEditPrivateKey", GPA_STOCK_EDIT, N_("_Edit Private Key..."), NULL,
	N_("Edit the selected private key"),
	G_CALLBACK (keyring_editor_edit) },
      { "KeysImport", GPA_STOCK_IMPORT, N_("_Import Keys..."), NULL,
	N_("Import Keys"), G_CALLBACK (keyring_editor_import) },
      { "KeysExport", GPA_STOCK_EXPORT, N_("E_xport Keys..."), NULL,
	N_("Export Keys"), G_CALLBACK (keyring_editor_export) },
      { "KeysBackup", NULL, N_("_Backup..."), NULL,
	N_("Backup key"), G_CALLBACK (keyring_editor_backup) },

      /* Server menu.  */
      { "ServerRetrieve", NULL, N_("_Retrieve Keys..."), NULL,
	N_("Retrieve keys from server"),
	G_CALLBACK (keyring_editor_retrieve) },
      { "ServerSend", NULL, N_("_Send Keys..."), NULL,
	N_("Send keys to server"), G_CALLBACK (keyring_editor_send) }
    };

  static const GtkRadioActionEntry radio_entries[] =
    {
      { "DetailsBrief", GPA_STOCK_BRIEF, NULL, NULL,
	N_("Show Brief Keylist"), 0 },
      { "DetailsDetailed", GPA_STOCK_DETAILED, NULL, NULL,
	N_("Show Key Details"), 1 }
    };

  static const char *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='File'>"
    "      <menuitem action='FileClose'/>"
    "      <menuitem action='FileQuit'/>"
    "    </menu>"
    "    <menu action='Edit'>"
    "      <menuitem action='EditCopy'/>"
    "      <menuitem action='EditPaste'/>"
    "      <separator/>"
    "      <menuitem action='EditSelectAll'/>"
    "      <separator/>"
    "      <menuitem action='EditPreferences'/>"
    "      <menuitem action='EditBackendPreferences'/>"
    "    </menu>"
    "    <menu action='Keys'>"
    "      <menuitem action='KeysRefresh'/>"
    "      <separator/>"
    "      <menuitem action='KeysNew'/>"
    "      <menuitem action='KeysDelete'/>"
    "      <separator/>"
    "      <menuitem action='KeysSign'/>"
    "      <menuitem action='KeysSetOwnerTrust'/>"
    "      <menuitem action='KeysEditPrivateKey'/>"
    "      <separator/>"
    "      <menuitem action='KeysImport'/>"
    "      <menuitem action='KeysExport'/>"
    "      <menuitem action='KeysBackup'/>"
    "    </menu>"
    "    <menu action='Windows'>"
    "      <menuitem action='WindowsKeyringEditor'/>"
    "      <menuitem action='WindowsFileManager'/>"
    "      <menuitem action='WindowsClipboard'/>"
    "      <menuitem action='WindowsCardManager'/>"
    "    </menu>"
    "    <menu action='Server'>"
    "      <menuitem action='ServerRetrieve'/>"
    "      <menuitem action='ServerSend'/>"
    "    </menu>"
    "    <menu action='Help'>"
#if 0
    "      <menuitem action='HelpContents'/>"
#endif
    "      <menuitem action='HelpAbout'/>"
    "    </menu>"
    "  </menubar>"
    "  <toolbar name='ToolBar'>"
    "    <toolitem action='KeysEditPrivateKey'/>"
    "    <toolitem action='KeysDelete'/>"
    "    <toolitem action='KeysSign'/>"
    "    <toolitem action='KeysImport'/>"
    "    <toolitem action='KeysExport'/>"
    "    <separator/>"
    "    <toolitem action='DetailsBrief'/>"
    "    <toolitem action='DetailsDetailed'/>"
    "    <separator/>"
    "    <toolitem action='EditPreferences'/>"
    "    <separator/>"
    "    <toolitem action='KeysRefresh'/>"
    "    <separator/>"
    "    <toolitem action='WindowsFileManager'/>"
    "    <toolitem action='WindowsClipboard'/>"
    "    <toolitem action='WindowsCardManager'/>"
#if 0
    "    <toolitem action='HelpContents'/>"
#endif
    "  </toolbar>"
    "  <popup name='PopupMenu'>"
    "    <menuitem action='EditCopy'/>"
    "    <menuitem action='EditPaste'/>"
    "    <menuitem action='KeysDelete'/>"
    "    <separator/>"
    "    <menuitem action='KeysSign'/>"
    "    <menuitem action='KeysSetOwnerTrust'/>"
    "    <menuitem action='KeysEditPrivateKey'/>"
    "    <separator/>"
    "    <menuitem action='KeysExport'/>"
    "    <menuitem action='ServerSend'/>"
    "    <menuitem action='KeysBackup'/>"
    "  </popup>"
    "</ui>";

  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GtkAction *action;
  GtkUIManager *ui_manager;
  GError *error;
  int detailed;

  detailed = gpa_options_get_detailed_view (gpa_options_get_instance());

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, PACKAGE);
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries),
				editor);
  gtk_action_group_add_radio_actions (action_group, radio_entries,
				      G_N_ELEMENTS (radio_entries),
				      detailed ? 1 : 0,
				      G_CALLBACK (keyring_set_listing_cb),
				      editor);
  gtk_action_group_add_actions (action_group, gpa_help_menu_action_entries,
				G_N_ELEMENTS (gpa_help_menu_action_entries),
				editor->window);
  gtk_action_group_add_actions (action_group, gpa_windows_menu_action_entries,
				G_N_ELEMENTS (gpa_windows_menu_action_entries),
				editor->window);
  gtk_action_group_add_actions
    (action_group, gpa_preferences_menu_action_entries,
     G_N_ELEMENTS (gpa_preferences_menu_action_entries), editor->window);
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (editor->window), accel_group);
  if (! gtk_ui_manager_add_ui_from_string (ui_manager, ui_description,
					   -1, &error))
    {
      g_message ("building keyring menus failed: %s", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }

  /* Fixup the icon theme labels which are too long for the toolbar.  */
  action = gtk_action_group_get_action (action_group, "KeysEditPrivateKey");
  g_object_set (action, "short_label", _("Edit"), NULL);
  action = gtk_action_group_get_action (action_group, "KeysDelete");
  g_object_set (action, "short_label", _("Delete"), NULL);
  action = gtk_action_group_get_action (action_group, "KeysSign");
  g_object_set (action, "short_label", _("Sign"), NULL);
  action = gtk_action_group_get_action (action_group, "KeysExport");
  g_object_set (action, "short_label", _("Export"), NULL);
  action = gtk_action_group_get_action (action_group, "KeysImport");
  g_object_set (action, "short_label", _("Import"), NULL);
  action = gtk_action_group_get_action (action_group, "WindowsFileManager");
  g_object_set (action, "short_label", _("Files"), NULL);
  action = gtk_action_group_get_action (action_group, "WindowsCardManager");
  g_object_set (action, "short_label", _("Card"), NULL);

  /* Take care of sensitiveness of widgets.  */
  action = gtk_action_group_get_action (action_group, "EditCopy");
  add_selection_sensitive_action (editor, action,
                                  keyring_editor_has_selection);
  action = gtk_action_group_get_action (action_group, "KeysDelete");
  add_selection_sensitive_action (editor, action,
                                  keyring_editor_has_selection);
  action = gtk_action_group_get_action (action_group, "KeysExport");
  add_selection_sensitive_action (editor, action,
                                  keyring_editor_has_selection);

  action = gtk_action_group_get_action (action_group, "ServerSend");
  add_selection_sensitive_action (editor, action,
				  keyring_editor_has_single_selection);

  action = gtk_action_group_get_action (action_group, "KeysSetOwnerTrust");
  add_selection_sensitive_action (editor, action,
				  keyring_editor_has_single_selection_OpenPGP);

  action = gtk_action_group_get_action (action_group, "KeysSign");
  add_selection_sensitive_action (editor, action,
				  keyring_editor_can_sign);

  action = gtk_action_group_get_action (action_group, "KeysEditPrivateKey");
  add_selection_sensitive_action (editor, action,
                                  keyring_editor_has_private_selected);
  action = gtk_action_group_get_action (action_group, "KeysBackup");
  add_selection_sensitive_action (editor, action,
                                  keyring_editor_has_private_selected);

  *menu = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  *toolbar = gtk_ui_manager_get_widget (ui_manager, "/ToolBar");
  gpa_toolbar_set_homogeneous (GTK_TOOLBAR (*toolbar), FALSE);

  *popup = gtk_ui_manager_get_widget (ui_manager, "/PopupMenu");
}


/* The details notebook.  */ 

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


/* Callback for the popdown menu on the signatures page.  */
static void
signatures_uid_selected (GtkComboBox *combo, gpointer user_data)
{
  GPAKeyringEditor *editor = user_data;
  gpgme_key_t key = keyring_editor_current_key (editor);

  /* We subtract one, as the first entry with index 0 means "all user
     names".  */
  gpa_siglist_set_signatures
    (editor->signatures_list, key,
     gtk_combo_box_get_active (GTK_COMBO_BOX (combo)) - 1);
}


/* Add the subkeys page from the notebook.  */
static void
keyring_editor_add_subkeys_page (GPAKeyringEditor *editor)
{
  if (!editor->subkeys_page)
    {
      GtkWidget *vbox;
      GtkWidget *scrolled;
      GtkWidget *subkeylist;

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
      editor->subkeys_list = subkeylist;
      editor->subkeys_page = vbox;
      gtk_notebook_append_page (GTK_NOTEBOOK (editor->notebook_details),
				editor->subkeys_page,
				gtk_label_new (_("Subkeys")));
      gtk_widget_show_all (editor->notebook_details);
      keyring_update_details_notebook (editor);
      idle_update_details (editor);
    }
}


/* Remove the subkeys page from the notebook.  */
static void
keyring_editor_remove_subkeys_page (GPAKeyringEditor *editor)
{
  if (editor->subkeys_page)
    {
      gtk_notebook_remove_page (GTK_NOTEBOOK (editor->notebook_details),
				GPA_KEYRING_EDITOR_SUBKEYS);
      editor->subkeys_list = NULL;
      editor->subkeys_page = NULL;
    }
}


/* Create and return the Details/Signatures notebook.  */
static GtkWidget *
keyring_details_notebook (GPAKeyringEditor *editor)
{
  GtkWidget *notebook;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *scrolled;
  GtkWidget *viewport;
  GtkWidget *siglist;
  GtkWidget *combo;
  GtkWidget *hbox;
  gint table_row;

  notebook = gtk_notebook_new ();
  editor->notebook_details = notebook;

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
  editor->details_num_label = label;
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  
  table = gtk_table_new (2, 7, FALSE);
  editor->details_table = table;
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);

  table_row = 0;
  editor->detail_public_private = add_details_row (table, table_row++,
                                                   "", TRUE);
  editor->detail_capabilities = add_details_row (table, table_row++,
						 "", TRUE);
  editor->detail_name = add_details_row (table, table_row++,
                                         _("User Name:"), TRUE);
  editor->detail_key_id = add_details_row (table, table_row++,
                                           _("Key ID:"), TRUE);
  editor->detail_fingerprint = add_details_row (table, table_row++,
                                                _("Fingerprint:"), TRUE);
  editor->detail_expiry = add_details_row (table, table_row++,
                                           _("Expires at:"), FALSE); 
  editor->detail_owner_trust = add_details_row (table, table_row++,
                                                _("Owner Trust:"), FALSE);
  editor->detail_key_trust = add_details_row (table, table_row++,
                                              _("Key Validity:"), FALSE);
  editor->detail_key_type = add_details_row (table, table_row++,
                                             _("Key Type:"), FALSE);
  editor->detail_creation = add_details_row (table, table_row++,
                                             _("Created at:"), FALSE);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled,
                            gtk_label_new (_("Details")));

  /* Signatures Page.  */
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  /* UID menu and label.  */
  hbox = gtk_hbox_new (FALSE, 5);
  label = gtk_label_new (_("Show signatures on user name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  combo = gtk_combo_box_new_text ();
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_set_sensitive (combo, FALSE);
  editor->signatures_uids = combo;
  editor->signatures_count = 0;
  editor->signatures_hbox = hbox;
  g_signal_connect (G_OBJECT (combo), "changed",
                    G_CALLBACK (signatures_uid_selected), editor);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* Signature list.  */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  siglist = gpa_siglist_new ();
  editor->signatures_list = siglist;
  gtk_container_add (GTK_CONTAINER (scrolled), siglist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Signatures")));

  /* Subkeys page.  */
  editor->subkeys_list = NULL;
  editor->subkeys_page = NULL;

  if (! gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    keyring_editor_add_subkeys_page (editor);

  return notebook;
}


/* Fill the details page of the details notebook with the properties
   of the public key.  */
static void
keyring_details_page_fill_key (GPAKeyringEditor *editor, gpgme_key_t key)
{
  gpgme_user_id_t uid;
  gchar *text;

  if (gpa_keytable_lookup_key (gpa_keytable_get_secret_instance(), 
			       key->subkeys->fpr) != NULL)
    gtk_label_set_text (GTK_LABEL (editor->detail_public_private),
			_("The key has both a private and a public part"));
  else
    gtk_label_set_text (GTK_LABEL (editor->detail_public_private),
			_("The key has only a public part"));

  gtk_label_set_text (GTK_LABEL (editor->detail_capabilities),
		      gpa_get_key_capabilities_text (key));

  /* One user ID on each line.  */
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
  gtk_label_set_text (GTK_LABEL (editor->detail_name), text);
  g_free (text);

  text = (gchar*) gpa_gpgme_key_get_short_keyid (key);
  gtk_label_set_text (GTK_LABEL (editor->detail_key_id), text);

  text = gpa_gpgme_key_format_fingerprint (key->subkeys->fpr);
  gtk_label_set_text (GTK_LABEL (editor->detail_fingerprint), text);
  g_free (text);
  text = gpa_expiry_date_string (key->subkeys->expires);
  gtk_label_set_text (GTK_LABEL (editor->detail_expiry), text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (editor->detail_key_trust),
                      gpa_key_validity_string (key));

  text = g_strdup_printf (_("%s %u bits"),
			  gpgme_pubkey_algo_name (key->subkeys->pubkey_algo),
			  key->subkeys->length);
  gtk_label_set_text (GTK_LABEL (editor->detail_key_type), text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (editor->detail_owner_trust),
                      gpa_key_ownertrust_string (key));

  text = gpa_creation_date_string (key->subkeys->timestamp);
  gtk_label_set_text (GTK_LABEL (editor->detail_creation), text);
  g_free (text);

  gtk_widget_hide (editor->details_num_label);
  gtk_widget_show (editor->details_table);
}


/* Show the number of keys num_key in the details page of the details
   notebook and make sure that that page is in front.  */
static void
keyring_details_page_fill_num_keys (GPAKeyringEditor *editor, gint num_key)
{
  if (! num_key)
    gtk_label_set_text (GTK_LABEL (editor->details_num_label),
			_("No keys selected"));
  else
    {
      char *text = g_strdup_printf (ngettext("%d key selected",
                                             "%d keys selected",
                                             num_key), num_key); 

      gtk_label_set_text (GTK_LABEL (editor->details_num_label), text);
      g_free (text);
    }

  gtk_widget_show (editor->details_num_label);
  gtk_widget_hide (editor->details_table);

  /* FIXME: Assumes that the 0th page is the details page.  This
     should be done better.  */
  gtk_notebook_set_current_page (GTK_NOTEBOOK (editor->notebook_details), 0);
}


/* Fill the signatures page of the details notebook with the signatures
   of the public key key.  */
static void
keyring_signatures_page_fill_key (GPAKeyringEditor *editor, gpgme_key_t key)
{
  GtkComboBox *combo;
  gint i;

  combo = GTK_COMBO_BOX (editor->signatures_uids);
  for (i = editor->signatures_count - 1; i >= 0; i--)
    gtk_combo_box_remove_text (combo, i);
  editor->signatures_count = 0;

  /* Create the popdown UID list if there is more than one UID.  */
  if (key->uids && key->uids->next)
    {
      gpgme_user_id_t uid;

      gtk_combo_box_append_text (combo, _("All signatures"));
      gtk_combo_box_set_active (combo, 0);
      i = 1;

      uid = key->uids;
      while (uid)
	{
	  gchar *uid_string = gpa_gpgme_key_get_userid (uid);
	  gtk_combo_box_append_text (combo, uid_string);
	  g_free (uid_string);
	  uid = uid->next;
	  i++;
	}
      editor->signatures_count = i;

      gtk_widget_show (editor->signatures_hbox);
      gtk_widget_set_sensitive (editor->signatures_uids, TRUE);
      /* Add the signatures.  */
      gpa_siglist_set_signatures (editor->signatures_list, key, -1);
    }
  else
    {
      /* If there is just one uid, display its signatures explicitly,
         and do not show the list of uids.  */
      gtk_widget_hide (editor->signatures_hbox);
      gpa_siglist_set_signatures (editor->signatures_list, key, 0);
    }
}


/* Empty the list of signatures in the details notebook.  */
static void
keyring_signatures_page_empty (GPAKeyringEditor *editor)
{
  GtkComboBox *combo;
  gint i;

  gpa_siglist_set_signatures (editor->signatures_list, NULL, 0);

  combo = GTK_COMBO_BOX (editor->signatures_uids);
  gtk_widget_set_sensitive (GTK_WIDGET (combo), FALSE);
  for (i = editor->signatures_count - 1; i >= 0; i--)
    gtk_combo_box_remove_text (combo, i);
  editor->signatures_count = 0;
}


/* Fill the subkeys page.  */
static void
keyring_subkeys_page_fill_key (GPAKeyringEditor *editor, gpgme_key_t key)
{
  if (editor->subkeys_page)
    gpa_subkey_list_set_key (editor->subkeys_list, key);
}


/* Empty the list of subkeys.  */
static void
keyring_subkeys_page_empty (GPAKeyringEditor *editor)
{
  if (editor->subkeys_page)
    gpa_subkey_list_set_key (editor->subkeys_list, NULL);
}


/* Update the details notebook according to the current selection.
   This means that if there is exactly one key selected, display its
   properties in the pages, otherwise show the number of currently
   selected keys.  */
static int
idle_update_details (gpointer param)
{
  GPAKeyringEditor *editor = param;

  if (gpa_keylist_has_single_selection (editor->keylist))
    {
      gpgme_key_t key = keyring_editor_current_key (editor);
      if (! key)
	{
	  /* There is a single key selected, but the current key is
	     NULL.  This means the key has not been returned yet, so
	     we exit the function asking GTK to run it again when
	     there is time.  */
	  return TRUE;
	}
      keyring_details_page_fill_key (editor, key);
      keyring_signatures_page_fill_key (editor, key);
      keyring_subkeys_page_fill_key (editor, key);
    }
  else
    {
      GList *selection = gpa_keylist_get_selected_keys (editor->keylist);
      keyring_details_page_fill_num_keys (editor, g_list_length (selection));
      keyring_signatures_page_empty (editor);
      keyring_subkeys_page_empty (editor);
      g_list_free (selection);
    }

  /* Set the idle id to NULL to indicate that the idle handler has
     been run.  */
  editor->details_idle_id = 0;
  
  /* Return 0 to indicate that this function shouldn't be called again
     by GTK, only when we expicitly add it again */
  return 0;
}


/* Add an idle handler to update the details notebook, but only when
   none has been set yet.  */
static void
keyring_update_details_notebook (GPAKeyringEditor *editor)
{
  if (! editor->details_idle_id)
    editor->details_idle_id = g_idle_add (idle_update_details, editor);
}



/* Status bar handling.  */
static GtkWidget *
keyring_statusbar_new (GPAKeyringEditor *editor)
{
  GtkWidget *hbox;
  GtkWidget *label;

  hbox = gtk_hbox_new (FALSE, 0);

  label = gtk_label_new ("");
  editor->status_label = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("");
  editor->status_key_id = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

  label = gtk_label_new ("");
  editor->status_key_user = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  
  return hbox;
}


/* Update the status bar.  */
static void
keyring_update_status_bar (GPAKeyringEditor *editor)
{
  gpgme_key_t key = gpa_options_get_default_key (gpa_options_get_instance ());

  if (key)
    {
      gchar *string;

      gtk_label_set_text (GTK_LABEL (editor->status_label),
			  _("Selected default key:"));
      string = gpa_gpgme_key_get_userid (key->uids);
      gtk_label_set_text (GTK_LABEL (editor->status_key_user), string);
      g_free (string);
      gtk_label_set_text (GTK_LABEL (editor->status_key_id),
                          gpa_gpgme_key_get_short_keyid (key));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (editor->status_label),
			  _("No default key selected in the preferences."));
      gtk_label_set_text (GTK_LABEL (editor->status_key_user), "");
      gtk_label_set_text (GTK_LABEL (editor->status_key_id), "");
    }     
}


/* The context menu of the keyring list.  */
static gint
display_popup_menu (GPAKeyringEditor *editor, GdkEvent *event, 
		    GpaKeyList *list)
{
  GtkMenu *menu;
  GdkEventButton *event_button;

  g_return_val_if_fail (editor != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  menu = GTK_MENU (editor->popup_menu);
  
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
		  editor->freeze_selection++;
		  gtk_tree_selection_unselect_all (selection);
		  editor->freeze_selection--;
		  gtk_tree_selection_select_path (selection, path);
		}
	      gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
			      event_button->button, event_button->time);
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
  GPAKeyringEditor *editor = param;

  /* Update the status bar and the selection sensitive widgets because
     some depend on the default key.  */
  keyring_update_status_bar (editor);
  update_selection_sensitive_actions (editor);
}


/* Signal handler for the "changed_ui_mode" signal.  */
static void
keyring_ui_mode_changed (GpaOptions *options, gpointer param)
{
  GPAKeyringEditor *editor = param;

  /* Toggles the subkeys page in the details notebook.  */
  if (! gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    keyring_editor_add_subkeys_page (editor);
  else
    keyring_editor_remove_subkeys_page (editor);
}


/* Create and return a new keyring editor dialog.  The dialog is
   hidden by default.  */
GtkWidget *
keyring_editor_new (void)
{
  GPAKeyringEditor *editor;
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scrolled;
  GtkWidget *keylist;
  GtkWidget *notebook;
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

  editor = g_malloc (sizeof (GPAKeyringEditor));
  editor->selection_sensitive_actions = NULL;
  editor->details_idle_id = 0;

  window = editor->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window),
                        _("GNU Privacy Assistant - Keyring Editor"));
  /* We use this for the destructor.  */
  g_object_set_data_full (G_OBJECT (window), "user_data", editor,
			  keyring_editor_destroy);
  gtk_window_set_default_size (GTK_WINDOW (window), 680, 600);
  g_signal_connect_swapped (G_OBJECT (window), "map",
			    G_CALLBACK (keyring_editor_mapped), editor);
  /* Realize the window so that we can create pixmaps without warnings.  */
  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  keyring_editor_action_new (editor, &menubar, &toolbar,
			     &editor->popup_menu);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);

  /* Add a fancy label that tells us: This is the keyring editor.  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);
  
  icon = gtk_image_new_from_stock (GPA_STOCK_KEYRING_EDITOR, GTK_ICON_SIZE_DND);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);

  label = gtk_label_new (NULL);
  markup = g_strdup_printf ("<span font_desc=\"16\">%s</span>",
                            _("Keyring Editor"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 10);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  paned = gtk_vpaned_new ();

  main_box = gtk_hbox_new (TRUE, 0);
  align = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_alignment_get_padding (GTK_ALIGNMENT (align),
                             &pt, &pb, &pl, &pr);
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
  /* FIXME: Which shadow type?  */
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
				       GTK_SHADOW_IN);
  keylist = gpa_keylist_new (window);
  editor->keylist = GPA_KEYLIST (keylist);
  if (gpa_options_get_detailed_view (gpa_options_get_instance()))
    gpa_keylist_set_detailed (editor->keylist);
  else
    gpa_keylist_set_brief (editor->keylist);

  gtk_container_add (GTK_CONTAINER (scrolled), keylist);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection 
			      (GTK_TREE_VIEW (keylist))),
		    "changed", G_CALLBACK (keyring_editor_selection_changed),
		    editor);

  g_signal_connect_swapped (G_OBJECT (keylist), "button_press_event",
                            G_CALLBACK (display_popup_menu), editor);

  notebook = keyring_details_notebook (editor);
  gtk_paned_pack2 (GTK_PANED (paned), notebook, TRUE, TRUE);
  gtk_paned_set_position (GTK_PANED (paned), 250);

  statusbar = keyring_statusbar_new (editor);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);
  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_default_key",
                    G_CALLBACK (keyring_default_key_changed), editor);
  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_ui_mode",
                    G_CALLBACK (keyring_ui_mode_changed), editor);

  keyring_update_status_bar (editor);
  update_selection_sensitive_actions (editor);
  keyring_update_details_notebook (editor);

  editor->current_key = NULL;
  editor->ctx = gpa_context_new ();
  editor->freeze_selection = 0;

  g_signal_connect (G_OBJECT (editor->ctx), "next_key",
		    G_CALLBACK (keyring_editor_key_listed), editor);

  return window;
}
