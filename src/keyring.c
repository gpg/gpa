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
#include <gpapa.h>
#include <gpgme.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gtktools.h"
#include "icons.h"
#include "filemenu.h"
#include "optionsmenu.h"
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

/*
 *      The public keyring editor
 */

/* Struct passed to all signal handlers of the keyring editor as user
 * data */
struct _GPAKeyringEditor {

  /* The toplevel window of the editor */
  GtkWidget *window;

  /* The central list of keys */
  GtkWidget *clist_keys;

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

  /* Labels in the status bar */
  GtkWidget *status_key_user;
  GtkWidget *status_key_id;

  /* List of sensitive widgets. See below */
  GList * selection_sensitive_widgets;
};
typedef struct _GPAKeyringEditor GPAKeyringEditor;


/*
 *      Internal API
 */

static gboolean key_has_been_signed (GpgmeKey key, gchar * key_id,
                                     GtkWidget * window);

static gboolean keyring_editor_has_selection (gpointer param);
static gboolean keyring_editor_has_single_selection (gpointer param);
static gchar *keyring_editor_current_key_id (GPAKeyringEditor * editor);

static void keyring_update_details_notebook (GPAKeyringEditor *editor);

static void toolbar_edit_key (GtkWidget *widget, gpointer param);
static void toolbar_remove_key (GtkWidget *widget, gpointer param);
static void toolbar_sign_key (GtkWidget *widget, gpointer param);
static void toolbar_export_key (GtkWidget *widget, gpointer param);
static void toolbar_import_keys (GtkWidget *widget, gpointer param);
static void keyring_editor_sign (gpointer param);
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

  return gpa_keylist_has_selection (editor->clist_keys);
}


/* Return TRUE if the key list widget of the keyring editor has exactly
 * one selected item.  Usable as a sensitivity callback.
 */
static gboolean
keyring_editor_has_single_selection (gpointer param)
{
  GPAKeyringEditor * editor = param;

  return gpa_keylist_has_single_selection (editor->clist_keys);
}



/* Fill the GtkCList with the keys */
/* XXX This function is also used to update the list after it may have
 * been changed (e.g. by deleting keys), but for that the current
 * implementation is broken because the selection information is lost.
 */
static void
keyring_editor_fill_keylist (GPAKeyringEditor * editor)
{
  gpa_keylist_update_list (editor->clist_keys);
} /* keyring_editor_fill_keylist */


/* delete the selected keys */
static void
keyring_editor_delete (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gint row;
  gchar * fpr;
  GpgmeKey key;
  GList * selection;
  gboolean has_secret;
  GpgmeError err;

  selection = gpa_keylist_selection (editor->clist_keys);
  gtk_clist_freeze (GTK_CLIST(editor->clist_keys));
  while (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      fpr = gtk_clist_get_row_data (GTK_CLIST (editor->clist_keys), row);
      key = gpa_keytable_lookup (keytable, fpr);
      has_secret = gpa_keytable_secret_lookup (keytable, fpr) != NULL;
      if (!gpa_delete_dialog_run (editor->window, key, has_secret))
        break;
      err = gpgme_op_delete (ctx, key, 1);
      selection = g_list_next (selection);
      if (err == GPGME_No_Error)
        {
          gtk_clist_remove (GTK_CLIST(editor->clist_keys), row);
          gpa_keytable_remove (keytable, fpr);
        }
      else
        {
          gpa_gpgme_error (err);
        }
    }
  gtk_clist_thaw (GTK_CLIST(editor->clist_keys));
} /* keyring_editor_delete */


/* Return true, if the public key key has been signed by the key with
 * the id key_id, otherwise return FALSE. The window parameter is needed
 * for error reporting */
static gboolean
key_has_been_signed (GpgmeKey key, gchar * key_id, GtkWidget * window)
{
  /* FIXME: When we can list signatures with GPGME, implement this */
  return 0;
}

/* Return true if the key sign button should be sensitive, i.e. if
 * there's at least one selected key and there is a default key.
 */
static gboolean
keyring_editor_can_sign (gpointer param)
{
  GPAKeyringEditor * editor = param;
  GpgmeKey key;
  gchar * key_id;
  gchar * default_key_id = gpa_default_key ();
  gboolean result = FALSE;

  if (keyring_editor_has_selection (param) && default_key_id)
    {
      /* the most important requirements have been met, now find out
       * whether the selected key was already signed with the default
       * key */
      key_id = keyring_editor_current_key_id (editor);
      key = gpa_keytable_lookup (keytable, key_id);

      if (key)
        {
          result = !key_has_been_signed (key, default_key_id,
                                         editor->window);
        }
    }
  return result;
} /* keyring_editor_can_sign */


/* sign the selected keys */
static void
keyring_editor_sign (gpointer param)
{
  GPAKeyringEditor *editor = param;
  gchar *private_key_fpr;
  gint row;
  gchar *key_fpr;
  GpgmeKey key;
  GpgmeError err;
  gboolean sign_locally = FALSE;
  GList *selection;
  gint signed_count = 0;

  if (!gpa_keylist_has_selection (editor->clist_keys))
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No keys selected for signing."), editor->window);
      return;
    }

  private_key_fpr = gpa_default_key ();
  if (!private_key_fpr)
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No private key for signing."), editor->window);
      return;
    }

  selection = gpa_keylist_selection (editor->clist_keys);
  while (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      key_fpr = gtk_clist_get_row_data (GTK_CLIST (editor->clist_keys), row);
                                   
      key = gpa_keytable_lookup (keytable, key_fpr);
      if (!key_has_been_signed (key, private_key_fpr, editor->window))
        {
          if (gpa_key_sign_run_dialog (editor->window, key, &sign_locally))
            {
              err = gpa_gpgme_edit_sign (key, private_key_fpr, sign_locally);
              if (err == GPGME_No_Error)
                {
                  signed_count++;
                }
              else if (err == GPGME_No_Passphrase)
                {
                  gpa_window_error (_("Wrong passphrase!"),
                                    editor->window);
                }
              else if (err == GPGME_Invalid_Key)
                {
                  /* Couldn't sign because the key was expired */
                  gpa_window_error (_("This key has expired! "
                                      "Unable to sign."), editor->window);
                }
              else if (err == GPGME_Canceled)
                {
                  /* Do nothing, the user should know if he cancelled the
                   * operation */
                }
              else
                {
                  gpa_gpgme_error (err);
                }
            }
        }
      selection = g_list_next (selection);
    }

  /* Update the signatures details page and the widgets because some
   * depend on what signatures a key has
   */
  if (signed_count > 0)
    {
      /* Reload the list of keys: a new signature may change the 
       * trust values on several keys.*/
      gpa_keytable_reload (keytable);
      keyring_editor_fill_keylist (editor);
      keyring_update_details_notebook (editor);
      update_selection_sensitive_widgets (editor);
    }
}


/* retrieve a key from the server */
static void
keyring_editor_import (gpointer param)
{
  GPAKeyringEditor *editor = param;
  gchar *filename, *server, *key_id;
  GpgmeData data;
  GpgmeError err;

  if (key_import_dialog_run (editor->window, &filename, &server, &key_id))
    {
      if (filename)
        {
          /* Read keys from the user specified file.
           */
          err = gpgme_data_new_from_file (&data, filename, 1);
          if (err == GPGME_File_Error)
            {
              gchar message[256];
              g_snprintf (message, sizeof(message), "%s: %s",
                          filename, strerror(errno));
              gpa_window_error (message, editor->window);
              return;
            }
          else if (err != GPGME_No_Error)
            {
              gpa_gpgme_error (err);
            }
        }
      else if (server)
        {
          /* Somehow fill a GpgmeData with the keys from the server */
        }
      else
        {
          /* Fill the data from the selection clipboard.
           */
          err = gpgme_data_new (&data);
          if (err != GPGME_No_Error)
            {
              gpa_gpgme_error (err);
            }
          fill_data_from_clipboard (data, gtk_clipboard_get
                                    (GDK_SELECTION_CLIPBOARD));
        }
      free (filename);
      free (server);
      /* Import the key */
      err = gpgme_op_import (ctx, data);
      if (err != GPGME_No_Error &&
          err != GPGME_No_Data)
        {
          gpa_gpgme_error (err);
        }
      gpgme_data_release (data);
      /* Reload the list of keys to get the imported keys */
      if (err != GPGME_No_Data)
        {
          gpa_keytable_reload (keytable);
        }
      /* update the widgets
       */
      keyring_editor_fill_keylist (editor);
      update_selection_sensitive_widgets (editor);
    }
}


/* export the selected keys to a file
 */
static void
keyring_editor_export (gpointer param)
{
  GPAKeyringEditor *editor = param;
  gchar *filename, *server;
  gboolean armored;

  if (!gpa_keylist_has_selection (editor->clist_keys))
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No keys selected to export."), editor->window);
      return;
    }

  if (key_export_dialog_run (editor->window, &filename, &server, &armored))
    {
      GpgmeError err;
      GpgmeData data;
      GpgmeRecipients rset;
      GList *selection = gpa_keylist_selection (editor->clist_keys);
      FILE *file = NULL;

      /* First: check any preconditions to the export (file/server accessible,
       * etc) */
      if (filename)
        {
          file = fopen (filename, "w");
          if (!file)
            {
              gchar message[256];
              g_snprintf (message, sizeof(message), "%s: %s",
                          filename, strerror(errno));
              gpa_window_error (message, editor->window);
              free (filename);
              free (server);
              return;
            }
        }
      else if (server)
        {
          /* Server */
        }
      else
        {
          /* Clipboard: No checks needed */
        }

      /* Create the data buffer */
      err = gpgme_data_new (&data);
      if (err != GPGME_No_Error)
        gpa_gpgme_error (err);
      gpgme_set_armor (ctx, armored);
      /* Create the set of keys to export */
      err = gpgme_recipients_new (&rset);
      if (err != GPGME_No_Error)
        gpa_gpgme_error (err);
      while (selection)
        {
          gint row = GPOINTER_TO_INT (selection->data);
          gchar *key_id = gtk_clist_get_row_data
            (GTK_CLIST (editor->clist_keys), row);
          err = gpgme_recipients_add_name (rset, key_id);
          if (err != GPGME_No_Error)
            gpa_gpgme_error (err);
          selection = g_list_next (selection);
        }
      /* Export to the GpgmeData */
      err = gpgme_op_export (ctx, rset, data);
      if (err != GPGME_No_Error)
        gpa_gpgme_error (err);
      
      /* Write the data somewhere */
      if (filename)
        {
          /* Export the selected keys to the user specified file. */
          if (file)
            {
              /* Dump the GpgmeData to the file */
              dump_data_to_file (data, file);
              fclose (file);
            }
        }
      else if (server)
        {
          /* Export the selected key to the user specified server.
           */
        }
      else
        {
          /* Write the data to the clipboard.
           */
          dump_data_to_clipboard (data, gtk_clipboard_get
                                  (GDK_SELECTION_CLIPBOARD));
        }
      g_free (filename);
      g_free (server);
      gpgme_data_release (data);
    }
}

/* Return TRUE if filename is a directory */
static gboolean
isdir (const gchar * filename)
{
  struct stat statbuf;

  return (stat (filename, &statbuf) == 0
          && S_ISDIR (statbuf.st_mode));
}

/* backup the default keys */
static void
keyring_editor_backup (gpointer param)
{
  GPAKeyringEditor *editor = param;
  gchar *fpr, *filename;

  fpr = gpa_default_key ();
  if (!fpr)
    {
      /* this shouldn't happen because the menu item should be grayed out
       * in this case
       */
      gpa_window_error (_("No private key to backup."), editor->window);
      return;
    }

  if (key_backup_dialog_run (editor->window, &filename, fpr))
    {
      gboolean cancelled = FALSE;
      
      if (g_file_test (filename, (G_FILE_TEST_EXISTS)))
	{
	  const gchar *buttons[] = {_("_Overwrite"), _("_Cancel"), NULL};
	  gchar *message = g_strdup_printf (_("The file %s already exists.\n"
					      "Do you want to overwrite it?"),
					    filename);
	  gchar *reply = gpa_message_box_run (editor->window, _("File exists"),
					      message, buttons);
	  if (!reply || strcmp (reply, _("_Overwrite")) != 0)
	    cancelled = TRUE;
	  g_free (message);
	}
      if (!cancelled)
	{
	  if (gpa_backup_key (fpr, filename))
	    {
	      gchar *message;
	      message = g_strdup_printf (_("A copy of your secret key has "
					   "been made to the file:\n\n"
					   "\t\"%s\"\n\n"
					   "This is sensitive information, "
					   "and should be stored carefully\n"
					   "(for example, in a floppy disk "
					   "kept in a safe place)."),
					 filename);
	      gpa_window_message (message, editor->window);
	      g_free (message);
	      gpa_remember_backup_generated ();
	    }
	}
    }
}

/* Run the advanced key generation dialog and if the user clicked OK,
 * generate a new key pair and updat the key list
 */
static void
keyring_editor_generate_key_advanced (gpointer param)
{
  GPAKeyringEditor * editor = param;
  GPAKeyGenParameters * params;
  GpgmeError err;

  params = gpa_key_gen_run_dialog(editor->window);
  if (params)
    {
      err = gpa_generate_key (params);
      if (err == GPGME_General_Error)
        {
          gpa_window_error (gpgme_strerror (err), editor->window);
        }
      else if (err != GPGME_No_Error)
        {
          gpa_gpgme_error (err);
        }
      gpa_key_gen_free_parameters (params);

      /* finally, update the default key if there is none because now
       * there is at least one secret key, update the key list and the
       * sensitive widgets because some may depend on whether secret
       * keys are available
       */
      gpa_keytable_reload (keytable);
      gpa_update_default_key ();
      keyring_editor_fill_keylist (editor);
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
      gpa_keytable_reload (keytable);
      gpa_update_default_key ();
      keyring_editor_fill_keylist (editor);
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
  if (gpa_simplified_ui ())
    keyring_editor_generate_key_simple (param);
  else
    keyring_editor_generate_key_advanced (param);
} /* keyring_editor_generate_key */



/* Return the id of the currently selected key. NULL if no key is selected */
static gchar *
keyring_editor_current_key_id (GPAKeyringEditor *editor)
{
  return gpa_keylist_current_key_id (editor->clist_keys);
}

#if 0
/* Return the currently selected key. NULL if no key is selected */
static GpgmeKey
keyring_editor_current_key (GPAKeyringEditor *editor)
{
  return gpa_keylist_current_key (editor->clist_keys);
}
#endif

/* Update everything that has to be updated when the selection in the
 * key list changes.
 */
static void
keyring_selection_update_widgets (GPAKeyringEditor * editor)
{
  update_selection_sensitive_widgets (editor);
  keyring_update_details_notebook (editor);
}  

/* Signal handler for select-row and unselect-row. Call
 * update_selection_sensitive_widgets */
static void
keyring_editor_selection_changed (GtkWidget * clistKeys, gint row,
                                  gint column, GdkEventButton * event,
                                  gpointer param)
{
  keyring_selection_update_widgets (param);
} /* keyring_editor_selection_changed */


/* Signal handler for end-selection. Call
 * update_selection_sensitive_widgets */
static void
keyring_editor_end_selection (GtkWidget * clistKeys, gpointer param)
{
  keyring_selection_update_widgets (param);
} /* keyring_editor_end_selection */



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

  if (gpa_simplified_ui ())
    {
      if (!asked_about_key_generation
          && gpa_keytable_secret_size (keytable) == 0)
        {
          const gchar * buttons[] = {_("_Generate key now"), _("Do it _later"),
                                     NULL};
          gchar * result;
          result = gpa_message_box_run (editor->window, _("No key defined"),
                                        _("You do not have a private key yet."
                                          " Do you want to generate one now"
                                          " (recommended) or do it later?"),
                                        buttons);
          if (result && strcmp(result, _("_Generate key now")) == 0)
            keyring_editor_generate_key (param);
          asked_about_key_generation = TRUE;
        }
      else if (!asked_about_key_backup
               && !gpa_backup_generated ()
               && gpa_keytable_secret_size (keytable) != 0)
        {
          const gchar * buttons[] = {_("_Backup key now"), _("Do it _later"),
                                     NULL};
          gchar * result;
          result = gpa_message_box_run (editor->window, _("No key backup"),
                                        _("You do not have a backup copy of"
                                          " your private key yet."
                                          " Do you want to backup your key now"
                                          " (recommended) or do it later?"),
                                        buttons);
          if (result && strcmp(result, _("_Backup key now")) == 0)
            keyring_editor_backup (param);
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
  free (editor);
} /* keyring_editor_destroy */


/* Create and return the menu bar for the key ring editor */
static GtkWidget *
keyring_editor_menubar_new (GtkWidget * window,
                            GPAKeyringEditor * editor)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *factory;
  GtkItemFactoryEntry file_menu[] = {
    {_("/_File"), NULL, NULL, 0, "<Branch>"},
    {_("/File/_Close"), "<control>W", keyring_editor_close, 0, NULL},
    {_("/File/_Quit"), "<control>Q", file_quit, 0, NULL},
  };
  GtkItemFactoryEntry keys_menu[] = {
    {_("/_Keys"), NULL, NULL, 0, "<Branch>"},
    {_("/Keys/_Generate Key..."), NULL, keyring_editor_generate_key, 0, NULL},
    {_("/Keys/_Delete Keys..."), NULL, keyring_editor_delete, 0, NULL},
    {_("/Keys/_Sign Keys..."), NULL, keyring_editor_sign, 0, NULL},
    {_("/Keys/_Import Keys..."), NULL, keyring_editor_import, 0, NULL},
    {_("/Keys/_Export Keys..."), NULL, keyring_editor_export, 0, NULL},
    {_("/Keys/_Backup..."), NULL, keyring_editor_backup, 0, NULL},
  };
  GtkItemFactoryEntry win_menu[] = {
    {_("/_Windows"), NULL, NULL, 0, "<Branch>"},
    {_("/Windows/_Filemanager"), NULL, gpa_open_filemanager, 0, NULL},
    {_("/Windows/_Keyring Editor"), NULL, gpa_open_keyring_editor, 0, NULL},
  };

  accel_group = gtk_accel_group_new ();
  factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
  gtk_item_factory_create_items (factory,
                                 sizeof (file_menu) / sizeof (file_menu[0]),
                                 file_menu, editor);
  gtk_item_factory_create_items (factory,
                                 sizeof (keys_menu) / sizeof (keys_menu[0]),
                                 keys_menu, editor);
  gpa_options_menu_add_to_factory (factory, window);
  gtk_item_factory_create_items (factory,
                                 sizeof (win_menu) / sizeof (win_menu[0]),
                                 win_menu, editor);
  gpa_help_menu_add_to_factory (factory, window);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

#if 0  /* @@@ :-( */
  item = gtk_item_factory_get_widget (factory, _("/Keys/_Sign Keys..."));
  printf ("%x\n", item);
  add_selection_sensitive_widget (editor, item,
                                  keyring_editor_can_sign);
  item = gtk_item_factory_get_widget (factory, _("/Keys/_Export Keys..."));
  printf ("%x\n", item);
  add_selection_sensitive_widget (editor, item,
                                  keyring_editor_has_selection);
#endif

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
                    GTK_FILL, 0, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (widget), 1.0, 0.5);

  if (!selectable)
    {
      widget = gtk_label_new ("");
      gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    }
  else
    {
      widget = gtk_entry_new ();
      gtk_editable_set_editable (GTK_EDITABLE (widget), FALSE);
    }
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, row, row + 1,
                    GTK_FILL | GTK_EXPAND, 0, 0, 0);

  return widget;
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
  GtkWidget * siglist;
  gint table_row;

  notebook = gtk_notebook_new ();

  /* Details Page */
  vbox = gtk_vbox_new (FALSE, 0);

  label = gtk_label_new ("");
  editor->details_num_label = label;
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
  
  table = gtk_table_new (2, 6, FALSE);
  editor->details_table = table;
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);

  table_row = 0;
  editor->detail_name = add_details_row (table, table_row++,
                                         _("User Name:"), FALSE);
  editor->detail_key_id = add_details_row (table, table_row++,
                                           _("Key ID:"), FALSE);
  editor->detail_fingerprint = add_details_row (table, table_row++,
                                                _("Fingerprint:"), TRUE);
  editor->detail_expiry = add_details_row (table, table_row++,
                                           _("Expires at:"), FALSE); 
  editor->detail_owner_trust = add_details_row (table, table_row++,
                                                _("Owner Trust:"), FALSE);
  editor->detail_key_trust = add_details_row (table, table_row++,
                                              _("Key Trust:"), FALSE);
  editor->detail_key_type = add_details_row (table, table_row++,
                                             _("Key Type:"), FALSE);
  editor->detail_creation = add_details_row (table, table_row++,
                                             _("Created at:"), FALSE);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Details")));

  /* Signatures Page */
  vbox = gtk_vbox_new (FALSE, 0);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  siglist = gpa_siglist_new (editor->window);
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
keyring_details_page_fill_key (GPAKeyringEditor * editor, GpgmeKey key)
{
  GpgmeValidity key_trust;
  GpgmeValidity owner_trust;
  gchar * text;

  text = gpa_gpgme_key_get_userid (key, 0);
  gtk_label_set_text (GTK_LABEL (editor->detail_name), text);
  g_free (text);

  text = (gchar*) gpgme_key_get_string_attr (key, GPGME_ATTR_KEYID, NULL, 0);
  gtk_label_set_text (GTK_LABEL (editor->detail_key_id), text);

  text = (gchar*) gpgme_key_get_string_attr (key, GPGME_ATTR_FPR, NULL, 0);
  gtk_entry_set_text (GTK_ENTRY (editor->detail_fingerprint), text);
  text = gpa_expiry_date_string (
          gpgme_key_get_ulong_attr (key, GPGME_ATTR_EXPIRE, NULL, 0));
  gtk_label_set_text (GTK_LABEL (editor->detail_expiry), text);
  g_free (text);

  key_trust = gpgme_key_get_ulong_attr (key, GPGME_ATTR_VALIDITY, NULL, 0);
  text = gpa_trust_string (key_trust);
  gtk_label_set_text (GTK_LABEL (editor->detail_key_trust), text);

  owner_trust = gpgme_key_get_ulong_attr (key, GPGME_ATTR_OTRUST, NULL, 0);
  text = gpa_trust_string (owner_trust);
  gtk_label_set_text (GTK_LABEL (editor->detail_owner_trust), text);

  if (gpa_keytable_secret_lookup (keytable, gpgme_key_get_string_attr 
                                  (key, GPGME_ATTR_FPR, NULL, 0)))
    {
      gtk_label_set_text (GTK_LABEL (editor->detail_key_type),
                          _("The key has both a private and a public part"));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (editor->detail_key_type),
                          _("The key has only a public part"));
    }
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
      free (text);
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
keyring_signatures_page_fill_key (GPAKeyringEditor * editor, GpgmeKey key)
{
#if 0
  GList * signatures;
  gchar * key_id = NULL;

  /* in the simplified UI we don't want to list the self signatures */
  if (gpa_simplified_ui ())
    {
      key_id = gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
                                         editor->window);
    }

  signatures = gpapa_public_key_get_signatures (key, gpa_callback,
                                                editor->window);
  gpa_siglist_set_signatures (editor->signatures_list, signatures, key_id);
#endif
} /* keyring_signatures_page_fill_key */


/* Empty the list of signatures in the details notebook */
static void
keyring_signatures_page_empty (GPAKeyringEditor * editor)
{
  gpa_siglist_set_signatures (editor->signatures_list, NULL, NULL);
} /* keyring_signatures_page_empty */


/* Update the details notebook according to the current selection. This
 * means that if there's exactly one key selected, display it's
 * properties in the pages, otherwise show the number of currently
 * selected keys */
static int
idle_update_details (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gint num_selected = gpa_keylist_selection_length (editor->clist_keys);

  if (num_selected == 1)
    {
      gchar * key_id = keyring_editor_current_key_id (editor);
      GpgmeKey key;

      key = gpa_keytable_lookup (keytable, key_id);
      keyring_details_page_fill_key (editor, key);
      keyring_signatures_page_fill_key (editor, key);
    }
  else
    {
      keyring_details_page_fill_num_keys (editor, num_selected);
      keyring_signatures_page_empty (editor);
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
                                          


/* definitions for the brief and detailed key list. The names are at the
 * end so that they automatically take up all the surplus horizontal
 * space allocated to he list because they usually need the most
 * horizontal space.
 */
static GPAKeyListColumn keylist_columns_brief[] =
{GPA_KEYLIST_KEY_TYPE_PIXMAP, GPA_KEYLIST_ID, GPA_KEYLIST_NAME};

static GPAKeyListColumn keylist_columns_detailed[] =
{GPA_KEYLIST_KEY_TYPE_PIXMAP, GPA_KEYLIST_ID, GPA_KEYLIST_EXPIRYDATE,
 GPA_KEYLIST_OWNERTRUST, GPA_KEYLIST_KEYTRUST, GPA_KEYLIST_NAME};


/* Change the keylist to brief listing */
static void
keyring_set_brief_listing (GtkWidget *widget, gpointer param)
{
  GPAKeyringEditor * editor = param;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gpa_keylist_set_column_defs (editor->clist_keys, 
                                   (sizeof keylist_columns_brief)
                                   / (sizeof keylist_columns_brief[0]),
                                   keylist_columns_brief);
      gpa_keylist_update_list (editor->clist_keys);
    }
} /* keyring_set_brief_listing */


/* Change the keylist to detailed listing */
static void
keyring_set_detailed_listing (GtkWidget *widget, gpointer param)
{
  GPAKeyringEditor * editor = param;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gpa_keylist_set_column_defs (editor->clist_keys, 
                                   (sizeof keylist_columns_detailed)
                                   / (sizeof keylist_columns_detailed[0]),
                                   keylist_columns_detailed);
      gpa_keylist_update_list (editor->clist_keys);
    }
} /* keyring_set_detailed_listing */


static void
toolbar_edit_key (GtkWidget *widget, gpointer param)
{
  GPAKeyringEditor * editor = param;  
  gchar * key_id = keyring_editor_current_key_id (editor);

  if (key_id)
    {
      if (gpa_key_edit_dialog_run (editor->window, key_id))
        {
          keyring_editor_fill_keylist (editor);
          update_selection_sensitive_widgets (editor);
          keyring_update_details_notebook (editor);
        }
    }
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

static GtkWidget *
keyring_toolbar_new (GtkWidget * window, GPAKeyringEditor *editor)
{
  GtkWidget *toolbar;
  GtkWidget *icon;
  GtkWidget *item;
  GtkWidget *button;

#ifdef __NEW_GTK__
  toolbar = gtk_toolbar_new ();
#else
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
#endif
  
  icon = gpa_create_icon_widget (window, "edit");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Edit"),
                                  _("Edit the selected key"), _("edit key"),
                                  icon, GTK_SIGNAL_FUNC (toolbar_edit_key),
                                  editor);
  add_selection_sensitive_widget (editor, item,
                                  keyring_editor_has_single_selection);

  icon = gpa_create_icon_widget (window, "delete");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Remove"),
                                  _("Remove the selected key"),
                                  _("remove key"), icon,
                                  GTK_SIGNAL_FUNC (toolbar_remove_key),
                                  editor);
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
  item = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                                     GTK_TOOLBAR_CHILD_RADIOBUTTON, button,
                                     _("Brief"), _("Show Brief Keylist"),
                                     _("brief"), icon,
                                   GTK_SIGNAL_FUNC (keyring_set_brief_listing),
                                     editor);
  gtk_signal_handler_block_by_data (GTK_OBJECT (item), editor);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), TRUE);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (item), editor);

  button = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (button));
  icon = gpa_create_icon_widget (window, "detailed");
  gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
                              GTK_TOOLBAR_CHILD_RADIOBUTTON, button,
                              _("Detailed"), _("Show Key Details"),
                              _("detailed"), icon,
                              GTK_SIGNAL_FUNC (keyring_set_detailed_listing),
                              editor);
  
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
  gchar * fpr = gpa_default_key ();
  GpgmeKey key;
  gchar *string;

  if (fpr
      && (key = gpa_keytable_lookup (keytable, fpr)))
    {
      string =  gpa_gpgme_key_get_userid (key, 0);
      gtk_label_set_text (GTK_LABEL (editor->status_key_user), string);
      g_free (string);
      gtk_label_set_text (GTK_LABEL (editor->status_key_id),
                          gpgme_key_get_string_attr (key, GPGME_ATTR_KEYID,
                                                     NULL, 0));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (editor->status_key_user), "");
      gtk_label_set_text (GTK_LABEL (editor->status_key_id), "");
    }     
}

/* signal handler for the "gpa_default_key_changed" signal. Update the
 * status bar and the selection sensitive widgets because some depend on
 * the default key */
static void
keyring_default_key_changed (gpointer param)
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

  editor = g_malloc(sizeof(GPAKeyringEditor));
  editor->selection_sensitive_widgets = NULL;
  editor->details_idle_id = 0;

  window = editor->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window),
                        _("GNU Privacy Assistant - Keyring Editor"));
  gtk_object_set_data_full (GTK_OBJECT (window), "user_data", editor,
                            keyring_editor_destroy);
  gtk_window_set_default_size (GTK_WINDOW (window), 580, 460);
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

  label = gtk_label_new (_("Keyring Editor"));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 10);
  gtk_widget_set_name (label, "big-label");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);


  paned = gtk_vpaned_new ();
  gtk_box_pack_start (GTK_BOX (vbox), paned, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (paned), 5);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_pack1 (GTK_PANED (paned), scrolled, TRUE, TRUE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  keylist =  gpa_keylist_new ((sizeof keylist_columns_brief)
                                / (sizeof keylist_columns_brief[0]),
                                keylist_columns_brief, 10, window);
  editor->clist_keys = keylist;
  gtk_container_add (GTK_CONTAINER (scrolled), keylist);
  /*
  gpa_connect_by_accelerator (GTK_LABEL (label), keylist,
                              accel_group, _("_Public key Ring"));*/

  gtk_signal_connect (GTK_OBJECT (keylist), "select-row",
                      GTK_SIGNAL_FUNC (keyring_editor_selection_changed),
                      (gpointer) editor);
  gtk_signal_connect (GTK_OBJECT (keylist), "unselect-row",
                      GTK_SIGNAL_FUNC (keyring_editor_selection_changed),
                      (gpointer) editor);
  gtk_signal_connect (GTK_OBJECT (keylist), "end-selection",
                      GTK_SIGNAL_FUNC (keyring_editor_end_selection),
                      (gpointer) editor);
  /*  gtk_signal_connect (GTK_OBJECT (keylist), "button-press-event",
                      GTK_SIGNAL_FUNC (keyring_openPublic_evalMouse),
                      (gpointer) editor);*/

  notebook = keyring_details_notebook (editor);
  gtk_paned_pack2 (GTK_PANED (paned), notebook, TRUE, TRUE);
  gtk_paned_set_position (GTK_PANED (paned), 200);

  statusbar = keyring_statusbar_new (editor);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (window), "gpa_default_key_changed",
                      (GtkSignalFunc)keyring_default_key_changed, editor);

  keyring_update_status_bar (editor);
  update_selection_sensitive_widgets (editor);
  keyring_update_details_notebook (editor);

  return window;
} /* keyring_editor_new */

