/* keyring.c  -  The GNU Privacy Assistant
 *      Copyright (C) 2000, 2001 G-N-U GmbH.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "gpa.h"
#include <config.h>
#include <gpgme.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gtktools.h"
#include "icons.h"
#include "helpmenu.h"
#include "gpapastrings.h"
#include "gpawidgets.h"
#include "ownertrustdlg.h"
#include "keysigndlg.h"
#include "keyimportdlg.h"
#include "keyexportdlg.h"
#include "keygendlg.h"
#include "keygenwizard.h"
#include "keyeditdlg.h"
#include "keydeletedlg.h"
#include "keyimpseldlg.h"
#include "keylist.h"
#include "siglist.h"
#include "keyring.h"
#include "gpgmetools.h"
#include "gpgmeedit.h"
#include "keytable.h"
#include "server_access.h"
#include "options.h"

#include "gpakeydeleteop.h"

/*
 *      The public keyring editor
 */

/* Struct passed to all signal handlers of the keyring editor as user
 * data */
struct _GPAKeyringEditor {

  /* The toplevel window of the editor */
  GtkWidget *window;

  /* The central list of keys */
  GpaKeyList *keylist;

  /* The "Show Ownertrust" toggle button */
  GtkWidget *toggle_show;

  /* The details notebook */
  GtkWidget *notebook_details;

  /* idle handler id for updates of the notebook. Will be nonzero
   * whenever a handler is currently set and zero otherwise */
  guint details_idle_id;

  /* Widgets in the details notebook page */
  GtkWidget *details_num_label;
  GtkWidget *details_table;
  GtkWidget *detail_public_private;
  GtkWidget *detail_name;
  GtkWidget *detail_fingerprint;
  GtkWidget *detail_expiry;
  GtkWidget *detail_key_id;
  GtkWidget *detail_owner_trust;
  GtkWidget *detail_key_trust;
  GtkWidget *detail_key_type;
  GtkWidget *detail_creation;

  /* The signatures list in the notebook */
  GtkWidget *signatures_list;
  GtkWidget *signatures_uids;
  GtkWidget *signatures_label;

  /* Labels in the status bar */
  GtkWidget *status_key_user;
  GtkWidget *status_key_id;

  /* List of sensitive widgets. See below */
  GList * selection_sensitive_widgets;

  /* The currently selected key */
  gpgme_key_t current_key;
};
typedef struct _GPAKeyringEditor GPAKeyringEditor;


/*
 *      Internal API
 */

static gboolean keyring_editor_has_selection (gpointer param);
static gboolean keyring_editor_has_single_selection (gpointer param);
static gpgme_key_t keyring_editor_current_key (GPAKeyringEditor * editor);

static void keyring_update_details_notebook (GPAKeyringEditor *editor);

static void toolbar_edit_key (GtkWidget *widget, gpointer param);
static void toolbar_remove_key (GtkWidget *widget, gpointer param);
static void toolbar_sign_key (GtkWidget *widget, gpointer param);
static void toolbar_export_key (GtkWidget *widget, gpointer param);
static void toolbar_import_keys (GtkWidget *widget, gpointer param);
static void keyring_editor_sign (gpointer param);
static void keyring_editor_edit (gpointer param);
static void keyring_editor_trust (gpointer param);
static void keyring_editor_import (gpointer param);
static void keyring_editor_export (gpointer param);
static void keyring_editor_backup (gpointer param);

/*
 * A simple sensitivity callback mechanism
 *
 * The basic idea is that buttons (and other widgets like menu items as
 * well) should know when they should be sensitive or not. The
 * implementation here is very simple and quite specific for the keyring
 * editor's needs.
 *
 * We maintain a list of sensitive widgets each of which has a
 * sensitivity callback associated with them as the "gpa_sensitivity"
 * data. The callback returns TRUE when the widget should be sensitive
 * and FALSE otherwise.
 *
 * Whenever the selection in the key list widget changes we call
 * update_selection_sensitive_widgets which iterates through the widgets
 * in the list, calls the sensitivity callback and changes the widget's
 * sensitivity accordingly.
 */

/* Prototype of a sensitivity callback. Return TRUE if the widget should
 * be senstitive, FALSE otherwise. The parameter is a pointer to the
 * GPAKeyringEditor struct.
 */
typedef gboolean (*SensitivityFunc)(gpointer);


/* Add widget to the list of sensitive widgets of editor
 */
static void
add_selection_sensitive_widget (GPAKeyringEditor *editor,
                                GtkWidget *widget,
                                SensitivityFunc callback)
{
  gtk_object_set_data (GTK_OBJECT (widget), "gpa_sensitivity", callback);
  editor->selection_sensitive_widgets \
    = g_list_append(editor->selection_sensitive_widgets, widget);
}


/* Update the sensitivity of the widget data and pass param through to
 * the sensitivity callback. Usable as iterator function in
 * g_list_foreach */
static void
update_selection_sensitive_widget (gpointer data, gpointer param)
{
  SensitivityFunc func;

  func = gtk_object_get_data (GTK_OBJECT (data), "gpa_sensitivity");
  gtk_widget_set_sensitive (GTK_WIDGET (data), func (param));
}


/* Call update_selection_sensitive_widget for all widgets in the list of
 * sensitive widgets and pass editor through as the user data parameter
 */
static void
update_selection_sensitive_widgets (GPAKeyringEditor * editor)
{
  g_list_foreach (editor->selection_sensitive_widgets,
                  update_selection_sensitive_widget,
                  (gpointer) editor);
}


/* Return TRUE if the key list widget of the keyring editor has at least
 * one selected item. Usable as a sensitivity callback.
 */
static gboolean
keyring_editor_has_selection (gpointer param)
{
  GPAKeyringEditor * editor = param;

  return gpa_keylist_has_selection (editor->keylist);
}


/* Return TRUE if the key list widget of the keyring editor has exactly
 * one selected item.  Usable as a sensitivity callback.
 */
static gboolean
keyring_editor_has_single_selection (gpointer param)
{
  GPAKeyringEditor * editor = param;

  return gpa_keylist_has_single_selection (editor->keylist);
}

/* Return TRUE if the key list widget of the keyring editor has exactly
 * one selected item and it's a private key.  Usable as a sensitivity
 * callback.
 */
static gboolean
keyring_editor_has_private_selected (gpointer param)
{
  GPAKeyringEditor * editor = param;

  return gpa_keylist_has_single_secret_selection 
    (GPA_KEYLIST(editor->keylist));
}

/*
 * Operations
 */

static void
gpa_keyring_editor_changed_wot_cb (gpointer data)
{
  GPAKeyringEditor *editor = data;
  gpa_keylist_start_reload (editor->keylist);  
}

static void
register_operation (GPAKeyringEditor * editor, GpaKeyOperation *op)
{
  g_signal_connect_swapped (G_OBJECT (op), "changed_wot",
			    G_CALLBACK (gpa_keyring_editor_changed_wot_cb),
			    editor);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (gpa_operation_destroy), editor); 
}


/* delete the selected keys */
static void
keyring_editor_delete (GPAKeyringEditor * editor)
{
  GList *selection = gpa_keylist_get_selected_keys (editor->keylist);
  GpaKeyDeleteOperation *op = gpa_key_delete_operation_new (editor->window,
							    selection);
  register_operation (editor, op);
}


/* Return true, if the public key key has been signed by the key with
 * the id key_id, otherwise return FALSE. The window parameter is needed
 * for error reporting */
static gboolean
key_has_been_signed (const gpgme_key_t key, 
		     const gpgme_key_t signer_key)
{
  gint uid, idx;
  gboolean uid_signed, key_signed;
  const char *signer_id;

  signer_id = gpgme_key_get_string_attr (signer_key, GPGME_ATTR_KEYID, 0, 0);
  /* We consider the key signed if all user ID's have been signed */
  key_signed = TRUE;
  for (uid = 0;
       key_signed &&
         gpgme_key_get_string_attr (key, GPGME_ATTR_USERID, 0, uid); uid++)
    {
      const gchar *keyid;
      
      uid_signed = FALSE;
      for (idx = 0; !uid_signed && (keyid = gpgme_key_sig_get_string_attr
                                    (key, uid, GPGME_ATTR_KEYID, NULL, idx));
           idx++)
        {
          if (g_str_equal (signer_id, keyid))
            {
              uid_signed = TRUE;
            }
        }
      key_signed = key_signed && uid_signed;
    }
  
  return key_signed;
}

/* Return true if the key sign button should be sensitive, i.e. if
 * there's at least one selected key and there is a default key.
 */
static gboolean
keyring_editor_can_sign (gpointer param)
{
  GPAKeyringEditor * editor = param;
  const gpgme_key_t default_key = gpa_options_get_default_key
    (gpa_options_get_instance ());
  gboolean result = FALSE;

  if (keyring_editor_has_selection (param) && default_key)
    {
      /* the most important requirements have been met, now find out
       * whether the selected key was already signed with the default
       * key */
      gpgme_key_t key = keyring_editor_current_key (editor);
      result = !key_has_been_signed (key, default_key);
    }
  return result;
} /* keyring_editor_can_sign */


/* sign the selected keys */
static void
keyring_editor_sign (gpointer param)
{
  GPAKeyringEditor *editor = param;
  gpgme_key_t key, signer_key;
  gpg_error_t err;
  gboolean sign_locally = FALSE;
  GList *selection;
  gint signed_count = 0;
  gpgme_ctx_t ctx = gpa_gpgme_new ();

  if (!gpa_keylist_has_selection (editor->keylist))
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No keys selected for signing."), editor->window);
      return;
    }

  signer_key = gpa_options_get_default_key (gpa_options_get_instance ());
  if (!signer_key)
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No private key for signing."), editor->window);
      return;
    }

  for (selection = gpa_keylist_get_selected_keys (editor->keylist); 
       selection; selection = g_list_next (selection))
    {
      key = selection->data;
      if (gpa_key_sign_run_dialog (editor->window, key, &sign_locally))
        {
          err = gpa_gpgme_edit_sign (ctx, key, signer_key, sign_locally);
          if (gpg_err_code (err) == GPG_ERR_NO_ERROR)
            {
              signed_count++;
            }
          else if (gpg_err_code (err) == GPG_ERR_BAD_PASSPHRASE)
            {
              gpa_window_error (_("Wrong passphrase!"),
                                editor->window);
            }
          else if (gpg_err_code (err) == GPG_ERR_UNUSABLE_PUBKEY)
            {
              /* Couldn't sign because the key was expired */
              gpa_window_error (_("This key has expired! "
                                  "Unable to sign."), editor->window);
            }
          else if (gpg_err_code (err) == GPG_ERR_CONFLICT)
            {
              /* Couldn't sign because the key was already signed */
              gpa_window_error (_("This key has already been signed with "
                                  "your own!"), editor->window);
            }
          else if (gpg_err_code (err) == GPG_ERR_NO_SECKEY)
            {
              /* Couldn't sign because there is no default key */
              gpa_window_error (_("You haven't selected a default key "
                                  "to sign with!"), editor->window);
            }
          else if (gpg_err_code (err) == GPG_ERR_CANCELED)
            {
              /* Do nothing, the user should know if he cancelled the
               * operation */
            }
          else
            {
              gpa_gpgme_error (err);
            }
        }
      selection = g_list_next (selection);
    }
  g_list_free (selection);
  /* Update the signatures details page and the widgets because some
   * depend on what signatures a key has
   */
  if (signed_count > 0)
    {
      /* Reload the list of keys: a new signature may change the 
       * trust values on several keys.*/
      gpa_keylist_start_reload (editor->keylist);
      keyring_update_details_notebook (editor);
      update_selection_sensitive_widgets (editor);
    }

  gpgme_release (ctx);
}

/* Invoke the "edit key" dialog */
static void
keyring_editor_edit (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gpgme_key_t key = keyring_editor_current_key (editor);

  if (key)
    {
      /* Should be called when just one key is selected.
       */
      if (gpa_key_edit_dialog_run (editor->window, key))
        {
	  gpa_keylist_start_reload (editor->keylist);
          update_selection_sensitive_widgets (editor);
          keyring_update_details_notebook (editor);
        }
    }
}

static void
keyring_editor_trust (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gpgme_key_t key = keyring_editor_current_key (editor);

  if (key)
    {
      if (gpa_ownertrust_run_dialog (key, editor->window))
        {
	  gpa_keylist_start_reload (editor->keylist);
          update_selection_sensitive_widgets (editor);
          keyring_update_details_notebook (editor);
        }
    }
}


static gboolean
keyring_editor_import_get_source (GPAKeyringEditor *editor, gpgme_data_t *data)
{
  gchar *filename, *server, *key_id;
  gpg_error_t err;

  if (key_import_dialog_run (editor->window, &filename, &server, &key_id))
    {
      if (filename)
        {
          /* Read keys from the user specified file.
           */
          err = gpa_gpgme_data_new_from_file (data, filename, editor->window);
	  /* Check if a file error ocurred */
          if (gpgme_err_code_to_errno (gpg_err_code (err)))
            {
              return FALSE;
            }
          else if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
            {
              gpa_gpgme_error (err);
            }
        }
      else if (server)
        {
          if (!server_get_key (server, key_id, data, editor->window))
            {
              g_free (filename);
              g_free (server);
              return FALSE;
            }
        }
      g_free (filename);
      g_free (server);
      return TRUE;
    }
  else
    {
      return FALSE; 
    }
}

static void
keyring_editor_import_do_import (GPAKeyringEditor *editor, gpgme_data_t data)
{
  gpg_error_t err;
  gpgme_ctx_t ctx = gpa_gpgme_new ();

  /* Import the key */
  err = gpgme_op_import (ctx, data);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR &&
      gpg_err_code (err) != GPG_ERR_NO_DATA)
    {
      gpa_gpgme_error (err);
    }
  /* Load the keys we just imported */
  if (gpg_err_code (err) != GPG_ERR_NO_DATA)
    {
      gpgme_import_result_t info;
      
      info = gpgme_op_import_result (ctx);
      /* If keys were imported, load them */
      if (info->imports)
        {
	  gpa_keylist_start_reload (editor->keylist);
        }
      key_import_results_dialog_run (editor->window, info);
    }
  gpgme_release (ctx);
  /* update the widgets
   */
  update_selection_sensitive_widgets (editor);
}

/* retrieve a key from the server */
static void
keyring_editor_import (gpointer param)
{
  GPAKeyringEditor *editor = param;
  gpgme_data_t data;

  if (keyring_editor_import_get_source (editor, &data))
    {
      keyring_editor_import_do_import (editor, data);
      gpgme_data_release (data);
    }
}

static void
keyring_editor_export_do_export (GPAKeyringEditor *editor, gpgme_data_t *data,
                                 gboolean armored)
{
  gpg_error_t err;
  GList *selection = gpa_keylist_get_selected_keys (editor->keylist);
  gpgme_ctx_t ctx = gpa_gpgme_new ();
  const gchar **patterns = NULL;
  int i;

  /* Create the data buffer */
  err = gpgme_data_new (data);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    gpa_gpgme_error (err);
  gpgme_set_armor (ctx, armored);
  /* Create the set of keys to export */
  patterns = g_malloc0 (sizeof(gchar*)*(g_list_length(selection)+1));
  for (i = 0; selection; i++, selection = g_list_next (selection))
    {
      gpgme_key_t key = (gpgme_key_t) selection->data;
      patterns[i] = key->subkeys->fpr;
    }
  /* Export to the gpgme_data_t */
  err = gpgme_op_export_ext (ctx, patterns, 0, *data);
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    gpa_gpgme_error (err);
  /* Clean up */
  gpgme_release (ctx);
  g_free (patterns);
}

/* export the selected keys to a file
 */
static void
keyring_editor_export (gpointer param)
{
  GPAKeyringEditor *editor = param;
  gchar *filename, *server;
  gboolean armored;

  if (!gpa_keylist_has_selection (editor->keylist))
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No keys selected to export."), editor->window);
      return;
    }

  if (key_export_dialog_run (editor->window, &filename, &server, &armored))
    {
      gpgme_data_t data;
      FILE *file = NULL;

      /* First: check any preconditions to the export */
      if (filename)
        {
          file = gpa_fopen (filename, editor->window);
          if (!file)
            {
              gchar *message = g_strdup_printf ("%s: %s", filename, 
						strerror(errno));
              gpa_window_error (message, editor->window);
	      g_free (message);
              g_free (filename);
              g_free (server);
              return;
            }
        }

      /* Export to a data buffer */
      keyring_editor_export_do_export (editor, &data, armored);
      
      /* Write the data somewhere */
      if (filename)
        {
          /* Export the selected keys to the user specified file. */
          if (file)
            {
              /* Dump the gpgme_data_t to the file */
              dump_data_to_file (data, file);
              fclose (file);
            }
        }
      else if (server)
        {
          /* Export the selected key to the user specified server.
           */
	  gpgme_key_t key = keyring_editor_current_key (editor);
	  server_send_keys (server, gpa_gpgme_key_get_short_keyid (key, 0),
			    data, editor->window);
        }
      g_free (filename);
      g_free (server);
      gpgme_data_release (data);
    }
}

/* backup the default keys */
static void
keyring_editor_backup (gpointer param)
{
  GPAKeyringEditor *editor = param;
  gpgme_key_t key = keyring_editor_current_key (editor);

  key_backup_dialog_run (editor->window, key);
}

/* Run the advanced key generation dialog and if the user clicked OK,
 * generate a new key pair and updat the key list
 */
static void
keyring_editor_generate_key_advanced (gpointer param)
{
  GPAKeyringEditor * editor = param;
  GPAKeyGenParameters * params;
  gpg_error_t err;
  gpgme_key_t key;

  params = gpa_key_gen_run_dialog(editor->window);
  if (params)
    {
      err = gpa_generate_key (params, &key);
      if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
        {
          gpa_gpgme_error (err);
        }
      gpgme_key_unref (key);
      gpa_key_gen_free_parameters (params);

      /* finally, update the default key if there is none because now
       * there is at least one secret key, update the key list and the
       * sensitive widgets because some may depend on whether secret
       * keys are available
       */
      gpa_keylist_start_reload (editor->keylist);
      gpa_options_update_default_key (gpa_options_get_instance ());
      update_selection_sensitive_widgets (editor);
    }
} /* keyring_editor_generate_key_advanced */


/* Call the key generation wizard and update the key list if necessary */
static void
keyring_editor_generate_key_simple (gpointer param)
{
  GPAKeyringEditor * editor = param;

  if (gpa_keygen_wizard_run (editor->window))
    {
      /* update the default key if there is none because now
       * there is at least one secret key, update the key list and the
       * sensitive widgets because some may depend on whether secret
       * keys are available
       */
      gpa_keylist_start_reload (editor->keylist);
      gpa_options_update_default_key (gpa_options_get_instance ());
      update_selection_sensitive_widgets (editor);
    }
} /* keyring_editor_generate_key_simple */


/* Depending on the simple_ui flag call either
 * keyring_editor_generate_key_advanced or
 * keyring_editor_generate_key_simple
 */
static void
keyring_editor_generate_key (gpointer param)
{
  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    keyring_editor_generate_key_simple (param);
  else
    keyring_editor_generate_key_advanced (param);
} /* keyring_editor_generate_key */



/* Return the the currently selected key. NULL if no key is selected */
static gpgme_key_t
keyring_editor_current_key (GPAKeyringEditor *editor)
{
  return editor->current_key;
}

/* Update everything that has to be updated when the selection in the
 * key list changes.
 */
static void
keyring_selection_update_widgets (GPAKeyringEditor * editor)
{
  update_selection_sensitive_widgets (editor);
  keyring_update_details_notebook (editor);
}  

/* Signal handler for selection changes.
 */
static void
keyring_editor_selection_changed (GtkTreeSelection *treeselection, 
				  gpointer param)
{
  GPAKeyringEditor *editor = param;
  /* Update the current key */
  if (editor->current_key)
    {
      /* Remove the previous one */
      gpgme_key_unref (editor->current_key);
      editor->current_key = NULL;
    }
  /* Load the new one */
  if (gpa_keylist_has_single_selection (editor->keylist)) 
    {
      gpg_error_t err;
      GList *selection = gpa_keylist_get_selected_keys (editor->keylist);
      gpgme_key_t key = (gpgme_key_t) selection->data;
      gpgme_ctx_t ctx = gpa_gpgme_new ();
      int old_mode = gpgme_get_keylist_mode (ctx);
      /* With all the signatures */
      gpgme_set_keylist_mode (ctx, old_mode | GPGME_KEYLIST_MODE_SIGS);
      err = gpgme_get_key (ctx, key->subkeys->fpr, &key, FALSE);
      if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
	{
	  gpa_gpgme_warning (err);
	}
      gpgme_set_keylist_mode (ctx, old_mode);
      g_list_free (selection);
      editor->current_key = key;
      gpgme_release (ctx);
    }
  keyring_selection_update_widgets (editor);
}

/* Signal handler for the map signal. If the simplified_ui flag is set
 * and there's no private key in the key ring, ask the user whether he
 * wants to generate a key. If so, call keyring_editor_generate_key()
 * which runs the appropriate dialog.
 * Also, if the simplified_ui flag is set, remind the user if he has
 * not yet created a backup copy of his private key.
 */
static void
keyring_editor_mapped (gpointer param)
{
  static gboolean asked_about_key_generation = FALSE;
  static gboolean asked_about_key_backup = FALSE;
  GPAKeyringEditor * editor = param;

  if (gpa_options_get_simplified_ui (gpa_options_get_instance ()))
    {
      /* We assume that the only reason a user might not have a default key
       * is because he has no private keys.
       */
      if (!asked_about_key_generation
          && !gpa_options_get_default_key (gpa_options_get_instance()))
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
	    {
	      keyring_editor_generate_key (param);
	    }
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
              key_backup_dialog_run (editor->window,
                                     gpa_options_get_default_key (gpa_options_get_instance ()));
	    }
          asked_about_key_backup = TRUE;
        }
    }
} /* keyring_editor_mapped */


/* close the keyring editor */
static void
keyring_editor_close (gpointer param)
{
  GPAKeyringEditor * editor = param;

  gtk_widget_destroy (editor->window);
} /* keyring_editor_close */


/* free the data structures associated with the keyring editor */
static void
keyring_editor_destroy (gpointer param)
{
  GPAKeyringEditor * editor = param;

  g_list_free (editor->selection_sensitive_widgets);
  g_free (editor);
} /* keyring_editor_destroy */

/* select all keys in the keyring */
static void
keyring_editor_select_all (gpointer param)
{
  GPAKeyringEditor * editor = param;
  GtkTreeSelection *selection = 
    gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->keylist));

  gtk_tree_selection_select_all (selection);
}

/* Paste the clipboard into the keyring */
static void
keyring_editor_paste (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gpgme_data_t data;
  gpg_error_t err;
  gchar *text = gtk_clipboard_wait_for_text (gtk_clipboard_get
                                             (GDK_SELECTION_CLIPBOARD));

  if (text)
    {
      /* Fill the data from the selection clipboard.
       */
      err = gpgme_data_new_from_mem (&data, text, strlen (text), FALSE);
    }
  else
    {
      /* If the keyboard was empty, create an empty data
       */
      err = gpgme_data_new (&data);
    }
  if (gpg_err_code (err) != GPG_ERR_NO_ERROR)
    {
      gpa_gpgme_error (err);
    }
  /* Import */
  keyring_editor_import_do_import (editor, data);
  gpgme_data_release (data);
  if (text)
    {
      g_free (text);
    }
}

/* Copy the keys into the clipboard */
static void
keyring_editor_copy (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gpgme_data_t data;

  /* Export to a data buffer */
  keyring_editor_export_do_export (editor, &data, TRUE);
  /* Write the data to the clipboard.
   */
  dump_data_to_clipboard (data, gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
  gpgme_data_release (data);
}

/* Create and return the menu bar for the key ring editor */
static GtkWidget *
keyring_editor_menubar_new (GtkWidget * window,
                            GPAKeyringEditor * editor)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *factory;
  GtkItemFactoryEntry file_menu[] = {
    {_("/_File"), NULL, NULL, 0, "<Branch>"},
    {_("/File/_Close"), NULL, keyring_editor_close, 0, "<StockItem>",
     GTK_STOCK_CLOSE},
    {_("/File/_Quit"), NULL, gtk_main_quit, 0, "<StockItem>", GTK_STOCK_QUIT},
  };
  GtkItemFactoryEntry edit_menu[] = {
    {_("/_Edit"), NULL, NULL, 0, "<Branch>"},
    {_("/Edit/_Copy"), NULL, keyring_editor_copy, 0, "<StockItem>",
     GTK_STOCK_COPY},
    {_("/Edit/_Paste"), NULL, keyring_editor_paste, 0, "<StockItem>",
     GTK_STOCK_PASTE},
    {_("/Edit/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/Edit/Select _All"), "<control>A", keyring_editor_select_all, 0, NULL},
    {_("/Edit/sep2"), NULL, NULL, 0, "<Separator>"},
    {_("/Edit/Pr_eferences..."), NULL, gpa_open_settings_dialog, 0,
     "<StockItem>", GTK_STOCK_PREFERENCES},
  };
  GtkItemFactoryEntry keys_menu[] = {
    {_("/_Keys"), NULL, NULL, 0, "<Branch>"},
    {_("/Keys/_New Key..."), NULL, keyring_editor_generate_key, 0,
     "<StockItem>", GTK_STOCK_NEW},
    {_("/Keys/_Delete Keys..."), NULL, keyring_editor_delete, 0,
     "<StockItem>", GTK_STOCK_DELETE},
    {_("/Keys/sep1"), NULL, NULL, 0, "<Separator>"},
    {_("/Keys/_Sign Keys..."), NULL, keyring_editor_sign, 0, NULL},
    {_("/Keys/Set _Owner Trust..."), NULL, keyring_editor_trust, 0, NULL},
    {_("/Keys/_Edit Private Key..."), NULL, keyring_editor_edit, 0, NULL},
    {_("/Keys/sep2"), NULL, NULL, 0, "<Separator>"},    
    {_("/Keys/_Import Keys..."), NULL, keyring_editor_import, 0, NULL},
    {_("/Keys/E_xport Keys..."), NULL, keyring_editor_export, 0, NULL},
    {_("/Keys/_Backup..."), NULL, keyring_editor_backup, 0, NULL},
  };
  GtkItemFactoryEntry win_menu[] = {
    {_("/_Windows"), NULL, NULL, 0, "<Branch>"},
    {_("/Windows/_Filemanager"), NULL, gpa_open_filemanager, 0, NULL},
    {_("/Windows/_Keyring Editor"), NULL, gpa_open_keyring_editor, 0, NULL},
  };
  GtkWidget *item;

  accel_group = gtk_accel_group_new ();
  factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
  gtk_item_factory_create_items (factory,
                                 sizeof (file_menu) / sizeof (file_menu[0]),
                                 file_menu, editor);
  gtk_item_factory_create_items (factory,
                                 sizeof (edit_menu) / sizeof (edit_menu[0]),
                                 edit_menu, editor);
  gtk_item_factory_create_items (factory,
                                 sizeof (keys_menu) / sizeof (keys_menu[0]),
                                 keys_menu, editor);
  gtk_item_factory_create_items (factory,
                                 sizeof (win_menu) / sizeof (win_menu[0]),
                                 win_menu, editor);
  gpa_help_menu_add_to_factory (factory, window);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  /* The menu paths given here MUST NOT contain underscores. Tough luck for
   * translators :-( */
  /* Items that must only be available if a key is selected */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Keys/Export Keys..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_selection);
    }
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Keys/Delete Keys..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_selection);
    }
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Edit/Copy"));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_selection);
    }
  /* Only if there is only ONE key selected */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Keys/Set Owner Trust..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_single_selection);
    }
  /* If the keys can be signed... */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Keys/Sign Keys..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_can_sign);
    }
  /* If the selected key has a private key */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Keys/Edit Private Key..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_private_selected);
    }
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Keys/Backup..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_private_selected);
    }

  return gtk_item_factory_get_widget (factory, "<main>");
}

/* Create the popup menu for the key list */
static GtkWidget *
keyring_editor_popup_menu_new (GtkWidget * window,
                               GPAKeyringEditor * editor)
{
  GtkItemFactory *factory;
  GtkItemFactoryEntry popup_menu[] = {
    {_("/_Copy"), NULL, keyring_editor_copy, 0, "<StockItem>",
     GTK_STOCK_COPY},
    {_("/_Paste"), NULL, keyring_editor_paste, 0, "<StockItem>",
     GTK_STOCK_PASTE},
    {_("/_Delete Keys..."), NULL, keyring_editor_delete, 0,
     "<StockItem>", GTK_STOCK_DELETE},
    {"/sep1", NULL, NULL, 0, "<Separator>"},
    {_("/_Sign Keys..."), NULL, keyring_editor_sign, 0, NULL},
    {_("/Set _Owner Trust..."), NULL, keyring_editor_trust, 0, NULL},
    {_("/_Edit Private Key..."), NULL, keyring_editor_edit, 0, NULL},
    {"/sep2", NULL, NULL, 0, "<Separator>"},
    {_("/E_xport Keys..."), NULL, keyring_editor_export, 0, NULL},
    {_("/_Backup..."), NULL, keyring_editor_backup, 0, NULL},
  };
  GtkWidget *item;

  factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", NULL);
  gtk_item_factory_create_items (factory,
                                 sizeof (popup_menu) / sizeof (popup_menu[0]),
                                 popup_menu, editor);

  /* Only if there is only ONE key selected */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Set Owner Trust..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_single_selection);
    }
  /* If the keys can be signed... */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Sign Keys..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_can_sign);
    }
  /* If the selected key has a private key */
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Edit Private Key..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_private_selected);
    }
  item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY(factory),
                                      _("/Backup..."));
  if (item)
    {
      add_selection_sensitive_widget (editor, item,
                                      keyring_editor_has_private_selected);
    }

  return gtk_item_factory_get_widget (factory, "<main>");
}

/*
 *      The details notebook
 */ 

/* add a single row to the details table */
static GtkWidget *
add_details_row (GtkWidget * table, gint row, gchar *text,
                 gboolean selectable)
{
  GtkWidget * widget;

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

/* Callback for the popdown menu on the signatures page */
static void
signatures_uid_selected (GtkOptionMenu *optionmenu, gpointer user_data)
{
  GPAKeyringEditor *editor = user_data;
  gpgme_key_t key = keyring_editor_current_key (editor);

  gpa_siglist_set_signatures (editor->signatures_list, key, 
                              gtk_option_menu_get_history 
                              (GTK_OPTION_MENU (editor->signatures_uids))-1);
}

/* Create and return the Details/Signatures notebook
 */
static GtkWidget *
keyring_details_notebook (GPAKeyringEditor *editor)
{
  GtkWidget * notebook;
  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * vbox;
  GtkWidget * scrolled;
  GtkWidget * viewport;
  GtkWidget * siglist;
  GtkWidget * options;
  GtkWidget * hbox;
  gint table_row;

  notebook = gtk_notebook_new ();

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
  gtk_container_add (GTK_CONTAINER (viewport), vbox);
  gtk_container_add (GTK_CONTAINER (scrolled), viewport);

  label = gtk_label_new ("");
  editor->details_num_label = label;
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  
  table = gtk_table_new (2, 6, FALSE);
  editor->details_table = table;
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);

  table_row = 0;
  editor->detail_public_private = add_details_row (table, table_row++,
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

  /* Signatures Page */
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

  /* UID menu and label */
  hbox = gtk_hbox_new (FALSE, 5);
  label = gtk_label_new (_("Show signatures on user name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  options = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), options, TRUE, TRUE, 0);
  gtk_widget_set_sensitive (options, FALSE);
  editor->signatures_uids = options;
  editor->signatures_label = label;
  g_signal_connect (G_OBJECT (options), "changed",
                    G_CALLBACK (signatures_uid_selected), editor);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  /* Signature list */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  siglist = gpa_siglist_new ();
  editor->signatures_list = siglist;
  gtk_container_add (GTK_CONTAINER (scrolled), siglist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Signatures")));

  editor->notebook_details = notebook;
  return notebook;
}


/* Fill the details page of the details notebook with the properties of
 * the publix key key */
static void
keyring_details_page_fill_key (GPAKeyringEditor * editor, gpgme_key_t key)
{
  gchar * text;
  gchar * uid;
  gint i;

  if (gpa_keytable_lookup_key (gpa_keytable_get_secret_instance(), 
			       key->subkeys->fpr) != NULL)
    {
      gtk_label_set_text (GTK_LABEL (editor->detail_public_private),
                          _("The key has both a private and a public part"));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (editor->detail_public_private),
                          _("The key has only a public part"));
    }

  /* One user ID on each line */
  text = gpa_gpgme_key_get_userid (key, 0);
  for (i = 1; (uid = gpa_gpgme_key_get_userid (key, i)) != NULL; i++)
    {
      gchar *tmp = text;
      text = g_strconcat (text, "\n", uid, NULL);
      g_free (tmp);
    }
  gtk_label_set_text (GTK_LABEL (editor->detail_name), text);
  g_free (text);

  text = (gchar*) gpa_gpgme_key_get_short_keyid (key, 0);
  gtk_label_set_text (GTK_LABEL (editor->detail_key_id), text);

  text = gpa_gpgme_key_get_fingerprint (key, 0);
  gtk_label_set_text (GTK_LABEL (editor->detail_fingerprint), text);
  g_free (text);
  text = gpa_expiry_date_string (
          gpgme_key_get_ulong_attr (key, GPGME_ATTR_EXPIRE, NULL, 0));
  gtk_label_set_text (GTK_LABEL (editor->detail_expiry), text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (editor->detail_key_trust),
                      gpa_key_validity_string (key));

  text = g_strdup_printf (_("%s %li bits"),
                          gpgme_key_get_string_attr (key, GPGME_ATTR_ALGO,
                                                    NULL, 0),
                          gpgme_key_get_ulong_attr (key, GPGME_ATTR_LEN,
                                                    NULL, 0));
  gtk_label_set_text (GTK_LABEL (editor->detail_key_type), text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (editor->detail_owner_trust),
                      gpa_key_ownertrust_string (key));

  text = gpa_creation_date_string (gpgme_key_get_ulong_attr 
                                   (key, GPGME_ATTR_CREATED, NULL, 0));
  gtk_label_set_text (GTK_LABEL (editor->detail_creation), text);
  g_free (text);

  gtk_widget_hide (editor->details_num_label);
  gtk_widget_show (editor->details_table);
} /* keyring_details_page_fill_key */


/* Show the number of keys num_key in the details page of the details
 * notebook and make sure that that page is in front */
static void
keyring_details_page_fill_num_keys (GPAKeyringEditor * editor, gint num_key)
{
  if (!num_key)
    {
      gtk_label_set_text (GTK_LABEL (editor->details_num_label),
                          _("No keys selected"));
    }
  else
    {
      gchar * text = g_strdup_printf (_("%d keys selected"), num_key);
      gtk_label_set_text (GTK_LABEL (editor->details_num_label), text);
      g_free (text);
    }

  gtk_widget_show (editor->details_num_label);
  gtk_widget_hide (editor->details_table);

  /* Assume that the 0th page is the details page. This should be done
   * better */
  gtk_notebook_set_page (GTK_NOTEBOOK (editor->notebook_details), 0);
} /* keyring_details_page_fill_num_keys */


/* Fill the signatures page of the details notebook with the signatures
 * of the public key key */
static void
keyring_signatures_page_fill_key (GPAKeyringEditor * editor, gpgme_key_t key)
{
  GtkWidget *menu;
  GtkWidget *label;
  gchar *uid;
  int i;

  /* Create the menu for the popdown UID list, if there is more than une UID
   */
  if (key->uids && key->uids->next)
    {
      menu = gtk_menu_new ();
      label = gtk_menu_item_new_with_label (_("All signatures"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), label);
      for (i = 0; (uid = gpa_gpgme_key_get_userid (key, i)) != NULL; i++)
	{
	  label = gtk_menu_item_new_with_label (uid);
	  gtk_menu_shell_append (GTK_MENU_SHELL (menu), label);
	  g_free (uid);
	}
      gtk_option_menu_set_menu (GTK_OPTION_MENU (editor->signatures_uids), 
				menu);
      gtk_widget_show_all (menu);
      gtk_widget_show (editor->signatures_uids);
      gtk_widget_show (editor->signatures_label);
      gtk_widget_set_sensitive (editor->signatures_uids, TRUE);
      /* Add the signatures */
      gpa_siglist_set_signatures (editor->signatures_list, key, -1);
    }
  else
    {
      /* If there is just one uid, display its signatures explicitly,
       * and don't show the list of uids */
      gtk_widget_hide (editor->signatures_uids);
      gtk_widget_hide (editor->signatures_label);
      gpa_siglist_set_signatures (editor->signatures_list, key, 0);
    } 
} /* keyring_signatures_page_fill_key */
  

/* Empty the list of signatures in the details notebook */
static void
keyring_signatures_page_empty (GPAKeyringEditor * editor)
{
  gtk_widget_set_sensitive (editor->signatures_uids, FALSE);
  gtk_option_menu_remove_menu (GTK_OPTION_MENU (editor->signatures_uids));
  gpa_siglist_set_signatures (editor->signatures_list, NULL, 0);
} /* keyring_signatures_page_empty */


/* Update the details notebook according to the current selection. This
 * means that if there's exactly one key selected, display it's
 * properties in the pages, otherwise show the number of currently
 * selected keys */
static int
idle_update_details (gpointer param)
{
  GPAKeyringEditor * editor = param;

  if (gpa_keylist_has_single_selection (editor->keylist))
    {
      gpgme_key_t key = keyring_editor_current_key (editor);
      keyring_details_page_fill_key (editor, key);
      keyring_signatures_page_fill_key (editor, key);
    }
  else
    {
      GList *selection = gpa_keylist_get_selected_keys (editor->keylist);
      keyring_details_page_fill_num_keys (editor, g_list_length (selection));
      keyring_signatures_page_empty (editor);
      g_list_free (selection);
    }

  /* Set the idle id to NULL to indicate that the idle handler has been
   * run */
  editor->details_idle_id = 0;
  
  /* Return 0 to indicate that this function shouldn't be called again
   * by GTK, only when we expicitly add it again */
  return 0;
}

/* Add an idle handler to update the details notebook, but only when
 * none has been set yet */
static void
keyring_update_details_notebook (GPAKeyringEditor * editor)
{
  if (!editor->details_idle_id)
    {
      editor->details_idle_id = gtk_idle_add (idle_update_details, editor);
    }
}

/* Change the keylist to brief listing */
static void
keyring_set_brief_listing (GtkWidget *widget, gpointer param)
{
  GPAKeyringEditor * editor = param;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gpa_keylist_set_brief (editor->keylist);
      gpa_options_set_detailed_view (gpa_options_get_instance (), FALSE);
    }
}


/* Change the keylist to detailed listing */
static void
keyring_set_detailed_listing (GtkWidget *widget, gpointer param)
{
  GPAKeyringEditor * editor = param;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gpa_keylist_set_detailed (editor->keylist);
      gpa_options_set_detailed_view (gpa_options_get_instance (), TRUE);
    }
}


static void
toolbar_edit_key (GtkWidget *widget, gpointer param)
{
  keyring_editor_edit (param);
}

static void
toolbar_remove_key (GtkWidget *widget, gpointer param)
{
  keyring_editor_delete (param);
}

static void
toolbar_sign_key (GtkWidget *widget, gpointer param)
{
  keyring_editor_sign (param);
}


static void
toolbar_export_key (GtkWidget *widget, gpointer param)
{
  keyring_editor_export (param);
}

static void
toolbar_import_keys (GtkWidget *widget, gpointer param)
{
  keyring_editor_import (param);
}

#if 0
static void
toolbar_preferences (GtkWidget *widget, gpointer param)
{
  gpa_open_settings_dialog ();
}
#endif

static GtkWidget *
keyring_toolbar_new (GtkWidget * window, GPAKeyringEditor *editor)
{
  GtkWidget *toolbar;
  GtkWidget *icon;
  GtkWidget *item;
  GtkWidget *button;
  GtkWidget *view_b;
  GtkWidget *view_d;

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar),
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
  
  icon = gpa_create_icon_widget (window, "edit");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Edit"),
                                  _("Edit the selected private key"),
                                  _("edit key"), icon,
                                  GTK_SIGNAL_FUNC (toolbar_edit_key),
                                  editor);
  add_selection_sensitive_widget (editor, item,
                                  keyring_editor_has_private_selected);

  
  item = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_DELETE,
                                   _("Remove the selected key"),
                                   _("remove key"),
                                   GTK_SIGNAL_FUNC (toolbar_remove_key),
                                   editor, -1);
  add_selection_sensitive_widget (editor, item,
                                  keyring_editor_has_selection);

  icon = gpa_create_icon_widget (window, "sign");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Sign"),
                                  _("Sign the selected key"), _("sign key"),
                                  icon, GTK_SIGNAL_FUNC (toolbar_sign_key),
                                  editor);
  add_selection_sensitive_widget (editor, item,
                                  keyring_editor_can_sign);
  
  icon = gpa_create_icon_widget (window, "import");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Import"),
                                  _("Import Keys"), _("import keys"),
                                  icon, GTK_SIGNAL_FUNC (toolbar_import_keys),
                                  editor);

  icon = gpa_create_icon_widget (window, "export");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Export"),
                                  _("Export Keys"), _("export keys"),
                                  icon, GTK_SIGNAL_FUNC (toolbar_export_key),
                                  editor);
  add_selection_sensitive_widget (editor, item,
                                  keyring_editor_has_selection);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));


  button = gtk_radio_button_new (NULL);
  icon = gpa_create_icon_widget (window, "brief");
  view_b = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                       GTK_TOOLBAR_CHILD_RADIOBUTTON, button,
                                       _("Brief"), _("Show Brief Keylist"),
                                       _("brief"), icon,
                                       GTK_SIGNAL_FUNC (keyring_set_brief_listing),
                                       editor);
  /*gtk_signal_handler_block_by_data (GTK_OBJECT (item), editor);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), TRUE);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (item), editor);*/

  button = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (button));
  icon = gpa_create_icon_widget (window, "detailed");
  view_d = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                       GTK_TOOLBAR_CHILD_RADIOBUTTON, button,
                                       _("Detailed"), _("Show Key Details"),
                                       _("detailed"), icon,
                                       GTK_SIGNAL_FUNC (keyring_set_detailed_listing),
                                       editor);
   /* Set brief or detailed button active according to option */
   if (gpa_options_get_detailed_view (gpa_options_get_instance()) )
     {
       item = view_d;
     }
   else
     {
       item = view_b;
     }
   gtk_signal_handler_block_by_data (GTK_OBJECT (item), editor);
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), TRUE);
   gtk_signal_handler_unblock_by_data (GTK_OBJECT (item), editor);
 
#if 0
  /* Disabled for now. The long label causes the toolbar to grow too much.
   * See http://bugzilla.gnome.org/show_bug.cgi?id=75086
   */
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  item = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), 
                                   GTK_STOCK_PREFERENCES,
                                   _("Open the Preferences dialog"),
                                   _("preferences"),
                                   GTK_SIGNAL_FUNC (toolbar_preferences),
                                   editor, -1);
#endif

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  icon = gpa_create_icon_widget (window, "openfile");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Files"),
                                  _("Open the File Manager"),
                                  _("file manager"), icon, GTK_SIGNAL_FUNC (gpa_open_filemanager),
                                  NULL);

#if 0  /* Help is not available yet. :-( */
  icon = gpa_create_icon_widget (window, "help");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Help"),
                                  _("Understanding the GNU Privacy Assistant"),
                                  _("help"), icon, GTK_SIGNAL_FUNC (help_help),
                                  NULL);
#endif

  return toolbar;
} /* keyring_toolbar_new */


static GtkWidget *
keyring_statusbar_new (GPAKeyringEditor *editor)
{
  GtkWidget * hbox;
  GtkWidget * label;

  hbox = gtk_hbox_new (FALSE, 0);

  label = gtk_label_new (_("Selected Default Key:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);

  label = gtk_label_new ("");
  editor->status_key_user = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  
  label = gtk_label_new ("");
  editor->status_key_id = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);

  return hbox;
} /* keyring_statusbar_new */


/* Update the status bar */
static void
keyring_update_status_bar (GPAKeyringEditor * editor)
{
  gpgme_key_t key = gpa_options_get_default_key (gpa_options_get_instance ());
  gchar *string;

  if (key)
    {
      string =  gpa_gpgme_key_get_userid (key, 0);
      gtk_label_set_text (GTK_LABEL (editor->status_key_user), string);
      g_free (string);
      gtk_label_set_text (GTK_LABEL (editor->status_key_id),
                          gpa_gpgme_key_get_short_keyid (key, 0));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (editor->status_key_user), "");
      gtk_label_set_text (GTK_LABEL (editor->status_key_id), "");
    }     
}

static gint
display_popup_menu (GtkWidget *widget, GdkEvent *event, GpaKeyList *list)
{
  GtkMenu *menu;
  GdkEventButton *event_button;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_MENU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  /* The "widget" is the menu that was supplied when 
   * g_signal_connect_swapped() was called.
   */
  menu = GTK_MENU (widget);
  
  if (event->type == GDK_BUTTON_PRESS)
    {
      event_button = (GdkEventButton *) event;
      if (event_button->button == 3)
	{
	  GtkTreeSelection *selection = 
	    gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	  GtkTreePath *path;
	  GtkTreeIter iter;
          /* Make sure the clicked key is selected */
	  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list), 
					     event_button->x,
					     event_button->y, 
					     &path, NULL,
					     NULL, NULL))
	    {
	      gtk_tree_model_get_iter (gtk_tree_view_get_model 
				       (GTK_TREE_VIEW(list)), &iter, path);
	      if (!gtk_tree_selection_iter_is_selected (selection, &iter))
		{	      
		  gtk_tree_selection_unselect_all (selection);
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

/* signal handler for the "changed_default_key" signal. Update the
 * status bar and the selection sensitive widgets because some depend on
 * the default key */
static void
keyring_default_key_changed (GpaOptions *options, gpointer param)
{
  GPAKeyringEditor * editor = param;

  keyring_update_status_bar (editor);  
  update_selection_sensitive_widgets (editor);
}

/* Create and return a new key ring editor window */
GtkWidget *
keyring_editor_new (void)
{
  GPAKeyringEditor *editor;
  GtkAccelGroup *accel_group;

  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scrolled;
  GtkWidget *keylist;
  GtkWidget *notebook;
  GtkWidget *toolbar;
  GtkWidget *hbox;
  GtkWidget *icon;
  GtkWidget *paned;
  GtkWidget *statusbar;

  gchar *markup;

  editor = g_malloc(sizeof(GPAKeyringEditor));
  editor->selection_sensitive_widgets = NULL;
  editor->details_idle_id = 0;

  window = editor->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window),
                        _("GNU Privacy Assistant - Keyring Editor"));
  gtk_object_set_data_full (GTK_OBJECT (window), "user_data", editor,
                            keyring_editor_destroy);
  gtk_window_set_default_size (GTK_WINDOW (window), 600, 600);
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_signal_connect_object (GTK_OBJECT (window), "map",
                             GTK_SIGNAL_FUNC (keyring_editor_mapped),
                             (gpointer)editor);
  /* Realize the window so that we can create pixmaps without warnings */
  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_box_pack_start (GTK_BOX (vbox),
                      keyring_editor_menubar_new (window, editor),
                      FALSE, TRUE, 0);

  toolbar = keyring_toolbar_new(window, editor);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);


  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  
  icon = gpa_create_icon_widget (window, "keyring");
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);

  label = gtk_label_new (NULL);
  markup = g_strdup_printf ("<span font_desc=\"16\">%s</span>",
                            _("Keyring Editor"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 10);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);


  paned = gtk_vpaned_new ();
  gtk_box_pack_start (GTK_BOX (vbox), paned, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (paned), 5);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_pack1 (GTK_PANED (paned), scrolled, TRUE, TRUE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  keylist = gpa_keylist_new (window);
  editor->keylist = GPA_KEYLIST (keylist);
  if (gpa_options_get_detailed_view (gpa_options_get_instance()))
    {
      gpa_keylist_set_detailed (editor->keylist);
    }
  else
    {
      gpa_keylist_set_brief (editor->keylist);
    }

  gtk_container_add (GTK_CONTAINER (scrolled), keylist);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection 
			      (GTK_TREE_VIEW (keylist))),
		    "changed", G_CALLBACK (keyring_editor_selection_changed),
		    (gpointer) editor);

  g_signal_connect_swapped (GTK_OBJECT (keylist), "button_press_event",
                            G_CALLBACK (display_popup_menu),
                            GTK_OBJECT (keyring_editor_popup_menu_new 
                                        (window, editor)));

  notebook = keyring_details_notebook (editor);
  gtk_paned_pack2 (GTK_PANED (paned), notebook, TRUE, TRUE);
  gtk_paned_set_position (GTK_PANED (paned), 250);

  statusbar = keyring_statusbar_new (editor);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);
  g_signal_connect (G_OBJECT (gpa_options_get_instance ()),
		    "changed_default_key",
                    (GCallback)keyring_default_key_changed, editor);

  keyring_update_status_bar (editor);
  update_selection_sensitive_widgets (editor);
  keyring_update_details_notebook (editor);

  editor->current_key = NULL;

  return window;
} /* keyring_editor_new */

