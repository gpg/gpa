/* keyring.c  -	 The GNU Privacy Assistant
 *	Copyright (C) 2000, 2001 G-N-U GmbH.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <gpapa.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include "gtktools.h"
#include "icons.h"
#include "gpa.h"
#include "filemenu.h"
#include "optionsmenu.h"
#include "helpmenu.h"
#include "gpapastrings.h"
#include "ownertrustdlg.h"
#include "keysigndlg.h"
#include "keyreceivedlg.h"
#include "keyexportdlg.h"
#include "keygendlg.h"
#include "keygenwizard.h"
#include "keyeditdlg.h"
#include "keylist.h"
#include "siglist.h"
#include "keyring.h"

/*
 *	The public keyring editor
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

  /* Labels in the details notebook page */
  GtkWidget *detail_name;
  GtkWidget *detail_fingerprint;
  GtkWidget *detail_expiry;

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
 *	Internal API
 */ 

static gboolean keyring_editor_has_selection (gpointer param);
static gboolean keyring_editor_has_single_selection (gpointer param);
static GpapaPublicKey *keyring_editor_current_key (GPAKeyringEditor * editor);
static gchar *keyring_editor_current_key_id (GPAKeyringEditor * editor);

static void keyring_update_details_page (GPAKeyringEditor * editor);
static void keyring_update_signatures_page (GPAKeyringEditor * editor);

/*
 *
 */

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


/* Add widget to the list of sensitive widgets of editor */
static void
add_selection_sensitive_widget (GPAKeyringEditor * editor,
				GtkWidget * widget,
				SensitivityFunc callback)
{
  gtk_object_set_data (GTK_OBJECT (widget), "gpa_sensitivity", callback);
  editor->selection_sensitive_widgets \
    = g_list_append(editor->selection_sensitive_widgets, widget);
} /* add_selection_sensitive_widget */


/* Update the sensitivity of the widget data and pass param through to
 * the sensitivity callback. Usable as iterator function in
 * g_list_foreach */
static void
update_selection_sensitive_widget (gpointer data, gpointer param)
{
  SensitivityFunc func;

  func = gtk_object_get_data (GTK_OBJECT (data), "gpa_sensitivity");
  gtk_widget_set_sensitive (GTK_WIDGET (data), func (param));
} /* update_selection_sensitive_widget */


/* Call update_selection_sensitive_widget for all widgets in the list of
 * sensitive widgets and pass editor through as the user data parameter
 */
static void
update_selection_sensitive_widgets (GPAKeyringEditor * editor)
{
  g_list_foreach (editor->selection_sensitive_widgets,
		  update_selection_sensitive_widget,
		  (gpointer) editor);
} /* update_selection_sensitive_widgets */


/* Return TRUE if the key list widget of the keyring editor has at least
 * one selected item. Usable as a sensitivity callback.
 */
static gboolean
keyring_editor_has_selection (gpointer param)
{
  GPAKeyringEditor * editor = param;

  return gpa_keylist_has_selection (editor->clist_keys);
} /* keyring_editor_has_selection */


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


#if 0
/* toggle the visibility of the ownertrust values */
static void
keyring_editor_toggle_show_trust (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gint i;
  GpapaPublicKey *key;
  gchar *contents;
  GdkPixmap *icon;
  GdkBitmap *mask;
  GpapaOwnertrust trust;
  int show_trust;
  gchar * key_id;

  show_trust \
    = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (editor->toggle_show));

  for (i = 0; i < editor->clist_keys->rows; i++)
    {
      if (show_trust)
	{
	  key_id = gtk_clist_get_row_data (editor->clist_keys, i);
	  key = gpapa_get_public_key_by_ID (key_id, gpa_callback,
					    editor->window);

	  trust = gpapa_public_key_get_ownertrust (key, gpa_callback,
						   editor->window);
	  contents = gpa_ownertrust_string (trust);
	  icon = gpa_create_icon_pixmap (editor->window,
					 gpa_ownertrust_icon_name (trust),
					 &mask);
	  if (icon)
	    gtk_clist_set_pixtext (editor->clist_keys, i, 2, contents, 8,
				   icon, mask);
	}
      else
	{
	  gtk_clist_set_text (editor->clist_keys, i, 2, "");
	}
    }
} /* keyring_editor_toggle_show_trust */
#endif

/* delete the selected keys */
static void
keyring_editor_delete (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gint row;
  gchar * key_id;
  GpapaPublicKey *public_key;
  GpapaSecretKey *secret_key;
  GList * selection;
  gchar message[500];
  gchar * reply;
  const gchar * buttons[] = {_("Ok"), _("Cancel"), NULL};

  selection = gpa_keylist_selection (editor->clist_keys);
  while (selection)
    {
      row = GPOINTER_TO_INT (selection->data);
      key_id = gtk_clist_get_row_data (GTK_CLIST (editor->clist_keys), row);
      public_key = gpapa_get_public_key_by_ID (key_id, gpa_callback,
					       editor->window);
      secret_key = gpapa_get_secret_key_by_ID (key_id, gpa_callback,
					       editor->window);

      /* XXX we should probably display more information about the key
       * (fingerprint?). We also shouldn't use a fixed size array. */
      sprintf (message, _("Are you sure that you want to delete the"
			  " following key?\n"
			  "%.300s\n"),
	       gpapa_key_get_name (GPAPA_KEY (public_key), gpa_callback,
				   editor->window));
      reply = gpa_message_box_run (editor->window, _("Delete Key"),
				   message, buttons);
      if (!reply || strcmp (reply, _("Cancel")) == 0)
	break;

      if (secret_key)
	{
	  gpapa_secret_key_delete (secret_key, gpa_callback, editor->window);
	}
      gpapa_public_key_delete (public_key, gpa_callback, editor->window);
      selection = g_list_next (selection);
    }
  keyring_editor_fill_keylist (editor);
} /* keyring_editor_delete */


/* Return true if the key sign button should be sensitive, i.e. if
 * there's at least one selected key and there is a default key.
 */
static gboolean
keyring_editor_can_sign (gpointer param)
{
  GPAKeyringEditor * editor = param;
  GpapaPublicKey * key;
  gchar * key_id;
  gchar * default_key_id = gpa_default_key ();
  GList * signatures;
  gboolean result = FALSE;

  if (keyring_editor_has_selection (param) && default_key_id)
    {
      /* the most important requirements have been met, now find out
       * whether the selected key was already signed with the default
       * key */
      key_id = keyring_editor_current_key_id (editor);
      key = gpapa_get_public_key_by_ID (key_id, gpa_callback, editor->window);

      if (key)
	{
	  result = TRUE;
	  signatures = gpapa_public_key_get_signatures (key, gpa_callback,
							editor->window);
	  while (signatures)
	    {
	      GpapaSignature *sig = signatures->data;
	      gchar * sig_id = gpapa_signature_get_identifier (sig,
							       gpa_callback,
							       editor->window);
	      if (strcmp (sig_id, default_key_id) == 0)
		{
		  result = FALSE;
		  break;
		}
	      signatures = g_list_next (signatures);
	    }
	}
    }
  return result;
} /* keyring_editor_can_sign */


/* sign the selected keys */
static void
keyring_editor_sign (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gchar * private_key_id;
  gchar * passphrase;
  gint row;
  gchar * key_id;
  GpapaPublicKey *key;
  GpapaSignType sign_type = GPAPA_KEY_SIGN_NORMAL;
  GList * selection;

  if (!gpa_keylist_has_selection (editor->clist_keys))
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No keys selected for signing."), editor->window);
      return;
    }

  private_key_id = gpa_default_key ();
  if (!private_key_id)
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
      key_id = gtk_clist_get_row_data (GTK_CLIST (editor->clist_keys), row);
				   
      key = gpapa_get_public_key_by_ID (key_id, gpa_callback, editor->window);
      if (gpa_key_sign_run_dialog (editor->window, key, &sign_type,
				   &passphrase))
	{
	  gpapa_public_key_sign (key, private_key_id, passphrase, sign_type,
				 gpa_callback, editor->window);
	  free (passphrase);
	}
      selection = g_list_next (selection);
    }

  /* Update the signatures details page and the widgets because some
   * depend on what signatures a key has*/
  keyring_update_signatures_page (editor);
  update_selection_sensitive_widgets (editor);
} /* keyring_editor_sign */


/* retrieve a key from the server */
static void
keyring_editor_receive (gpointer param)
{
  GPAKeyringEditor * editor = param;

  key_receive_run_dialog (editor->window);
}


/* export the selected keys to a file */
static void
keyring_editor_export (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gchar * filename, *server;
  gboolean armored;

  if (!gpa_keylist_has_selection (editor->clist_keys))
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No keys selected to export."), editor->window);
      return;
    } /* if */

  if (key_export_dialog_run (editor->window, &filename, &server, &armored))
    {
      if (filename)
	{
	  /* Export the selected key to the user specified file. FIXME:
	   * We should really export all selected keys, but gpapa
	   * currently can't export multiple keys to one file */
	  GpapaPublicKey * key = gpa_keylist_current_key (editor->clist_keys);
	  gpapa_public_key_export (key, filename, armored,
				   gpa_callback, editor->window);
	}
      else if (server)
	{
	  /* Export the selected key to the user specified server.
	   * FIXME: We should really export all selected keys */
	  GList * selection = gpa_keylist_selection (editor->clist_keys);
	  gchar * key_id;
	  gint row;
	  GpapaPublicKey * key;

	  while (selection)
	    {
	      row = GPOINTER_TO_INT (selection->data);
	      key_id = gtk_clist_get_row_data (GTK_CLIST (editor->clist_keys),
					       row);
	      key = gpapa_get_public_key_by_ID (key_id, gpa_callback,
						editor->window);

	      gpapa_public_key_send_to_server (key, server, gpa_callback,
					       editor->window);
	      selection = g_list_next (selection);
	    }
	}
      else
	{
	  /* Should never happen because the dialog should ensure that
	   * either filename or server is not NULL */
	  printf ("neither filename nor server supplied\n");
	}
      free (filename);
      free (server);
    } /* if */
} /* keyring_editor_export */


/* Run the advanced key generation dialog and if the user clicked OK,
 * generate a new key pair and updat the key list
 */
static void
keyring_editor_generate_key_advanced (gpointer param)
{
  GPAKeyringEditor * editor = param;
  GPAKeyGenParameters * params;
  GpapaPublicKey *publicKey;
  GpapaSecretKey *secretKey;

  params = gpa_key_gen_run_dialog(editor->window);
  if (params)
    {
      gpapa_create_key_pair (&publicKey, &secretKey, params->password,
			     params->algo, params->keysize, params->userID,
			     params->email, params->comment,
			     gpa_callback, editor->window);
      if (!publicKey || !secretKey)
	{
	  /* something went wrong. The gpa_callback should have popped
	   * up an message box with an error message, so simply clean up
	   * *and return here
	   */
	  gpa_key_gen_free_parameters (params);
	  return;
	}
	  
      if (params->expiryDate)
	{
	  gpapa_key_set_expiry_date (GPAPA_KEY (publicKey), params->expiryDate,
				     gpa_callback, editor->window);
	  gpapa_key_set_expiry_date (GPAPA_KEY (secretKey), params->expiryDate,
				     gpa_callback, editor->window);
	}
      else if (params->interval)
	{
	  gpapa_key_set_expiry_time (GPAPA_KEY (publicKey), params->interval,
				     params->unit, gpa_callback,
				     editor->window);
	  gpapa_key_set_expiry_time (GPAPA_KEY (secretKey), params->interval,
				     params->unit, gpa_callback,
				     editor->window);
	}
      else
	{
	  gpapa_key_set_expiry_date (GPAPA_KEY (publicKey), NULL, gpa_callback,
				     editor->window);
	  gpapa_key_set_expiry_date (GPAPA_KEY (secretKey), NULL, gpa_callback,
				     editor->window);
	}

      if (params->generate_revocation)
	gpapa_secret_key_create_revocation (secretKey, gpa_callback,
					    editor->window);
      if (params->send_to_server)
	{
	  printf ("send key to server\n");
	  /*
	    gpapa_public_key_send_to_server (publicKey, global_keyserver,
					   gpa_callback, editor->window);
	  */
	}
      gpa_key_gen_free_parameters (params);

      /* finally, update the default key if there is none because now
       * there is at least one secret key, update the key list and the
       * sensitive widgets because some may depend on whether secret
       * keys are available
       */
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


/* Return the currently selected key. NULL if no key is selected */
static GpapaPublicKey *
keyring_editor_current_key (GPAKeyringEditor *editor)
{
  return gpa_keylist_current_key (editor->clist_keys);
}


/* Update everything that has to be updated when the selection in the
 * key list changes.
 */
static void
keyring_selection_update_widgets (GPAKeyringEditor * editor)
{
  update_selection_sensitive_widgets (editor);
  keyring_update_details_page (editor);
  keyring_update_signatures_page (editor);
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



/* Signal handler for he map signal. If the simplified_ui flag is set
 * and there's no private key in the key ring, ask the user whether he
 * wants to generate a key. If so, call keyring_editor_generate_key
 * which calls runs the appropriate dialog
 */
static void
keyring_editor_mapped (gpointer param)
{
  static gboolean asked_about_key_generation = FALSE;
  GPAKeyringEditor * editor = param;

  if (gpa_simplified_ui ())
    {
      if (gpapa_get_secret_key_count (gpa_callback, editor->window) == 0
	  && !asked_about_key_generation)
	{
	  const gchar * buttons[] = {_("Generate key now"), _("Do it later"),
				     NULL};
	  gchar * result;
	  result = gpa_message_box_run (editor->window, _("No key defined"),
					_("You do not have a private key yet."
					  " Do you want to generate one now"
					  " (recommended) or do it later?"),
					buttons);
	  printf ("message box result: %s\n", result);
	  if (strcmp(result, _("Generate key now")) == 0)
	    keyring_editor_generate_key (param);
	  asked_about_key_generation = TRUE;
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
    {_("/File/_Close"), NULL, keyring_editor_close, 0, NULL},
    {_("/File/_Quit"), "<control>Q", file_quit, 0, NULL},
  };
  GtkItemFactoryEntry keys_menu[] = {
    {_("/_Keys"), NULL, NULL, 0, "<Branch>"},
    {_("/Keys/_Generate Key..."), NULL, keyring_editor_generate_key, 0, NULL},
    /*{_("/Keys/Generate _Revocation Certificate"), NULL,
					 keys_generateRevocation, 0, NULL},*/
    /*{_("/Keys/_Import Keys"), NULL, keys_import, 0, NULL},*/
    /*{_("/Keys/Import _Ownertrust"), NULL, keys_importOwnertrust, 0, NULL},*/
    /*{_("/Keys/_Update Trust Database"), NULL, keys_updateTrust, 0, NULL},*/
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
  gpa_options_menu_add_to_factory (factory, window);
  gtk_item_factory_create_items (factory,
				 sizeof (keys_menu) / sizeof (keys_menu[0]),
				 keys_menu, editor);
  gtk_item_factory_create_items (factory,
				 sizeof (win_menu) / sizeof (win_menu[0]),
				 win_menu, editor);
  gpa_help_menu_add_to_factory (factory, window);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  return gtk_item_factory_get_widget (factory, "<main>");
}

/*
 *	The details notebook
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
  GtkWidget * vbox;
  GtkWidget * scrolled;
  GtkWidget * siglist;

  notebook = gtk_notebook_new ();

  /* Details Page */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  
  editor->detail_name = add_details_row (table, 0, _("User Name:"), FALSE);
  editor->detail_fingerprint = add_details_row (table, 1, _("Fingerprint:"),
						TRUE);
  editor->detail_expiry = add_details_row (table, 2, _("Expires at:"), FALSE); 

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table,
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

  return notebook;
}


/* Update the labels of the details notebook page to the values of the
 * currently selected key
 */
static void
keyring_update_details_page (GPAKeyringEditor * editor)
{
  GpapaPublicKey * key = keyring_editor_current_key (editor);
  GDate * expiry_date;
  gchar * text;

  if (key)
    {
      text = gpapa_key_get_name (GPAPA_KEY (key), gpa_callback,
				 editor->window);
      gtk_label_set_text (GTK_LABEL (editor->detail_name), text);

      text = gpapa_public_key_get_fingerprint (key, gpa_callback,
					       editor->window);
      /*gtk_label_set_text (GTK_LABEL (editor->detail_fingerprint), text);*/
      gtk_entry_set_text (GTK_ENTRY (editor->detail_fingerprint), text);

      expiry_date = gpapa_key_get_expiry_date (GPAPA_KEY (key), gpa_callback,
					       editor->window);
      text = gpa_expiry_date_string (expiry_date);
      gtk_label_set_text (GTK_LABEL (editor->detail_expiry), text);
    }
} /* keyring_update_details_page */


static void
keyring_update_signatures_page (GPAKeyringEditor * editor)
{
  GpapaPublicKey * key = keyring_editor_current_key (editor);
  GList * signatures;
  gchar * key_id = NULL;

  /* in the simplified UI we don't want to list the self signatures */
  if (key)
    {
      if (gpa_simplified_ui ())
	{
	  key_id = gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
					     editor->window);
	}

      signatures = gpapa_public_key_get_signatures (key, gpa_callback,
						    editor->window);
      gpa_siglist_set_signatures (editor->signatures_list, signatures, key_id);
    }
  else
    gpa_siglist_set_signatures (editor->signatures_list, NULL, NULL);
} /* keyring_update_signatures_page */


/* definitions for the brief and detailed key list. The names are at the
 * end so that they automatically take up all the surplus horizontal
 * space allocated to he list because they usually need the most
 * horizontal space.
 */
static GPAKeyListColumn keylist_columns_brief[] =
{GPA_KEYLIST_ID, GPA_KEYLIST_NAME};

static GPAKeyListColumn keylist_columns_detailed[] =
{GPA_KEYLIST_ID, GPA_KEYLIST_EXPIRYDATE, GPA_KEYLIST_OWNERTRUST,
 GPA_KEYLIST_KEYTRUST, GPA_KEYLIST_NAME};


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
}

static GtkWidget *
keyring_toolbar_new (GtkWidget * window, GPAKeyringEditor *editor)
{
  GtkWidget *toolbar;
  GtkWidget *icon;
  GtkWidget *item;
  GtkWidget *button;

  toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  
  icon = gpa_create_icon_widget (window, "help");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Edit"),
				  _("Edit the selected key"), _("edit key"),
				  icon, GTK_SIGNAL_FUNC (toolbar_edit_key),
				  editor);
  add_selection_sensitive_widget (editor, item,
				  keyring_editor_has_single_selection);

  icon = gpa_create_icon_widget (window, "trash");
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
  
  icon = gpa_create_icon_widget (window, "openfile");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Import"),
				  _("Import Keys"), _("import keys"),
				  icon, GTK_SIGNAL_FUNC (toolbar_import_keys),
				  editor);

  icon = gpa_create_icon_widget (window, "openfile");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Export"),
				  _("Export Keys"), _("export keys"),
				  icon, GTK_SIGNAL_FUNC (toolbar_export_key),
				  editor);
  add_selection_sensitive_widget (editor, item,
				  keyring_editor_has_selection);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));


  button = gtk_radio_button_new (NULL);
  item = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
				     GTK_TOOLBAR_CHILD_RADIOBUTTON, button,
				     _("Brief"), _("Show Brief Keylist"),
				     _("brief"), NULL,
				   GTK_SIGNAL_FUNC (keyring_set_brief_listing),
				     editor);
  gtk_signal_handler_block_by_data (GTK_OBJECT (item), editor);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (item), TRUE);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (item), editor);

  button = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (button));
  gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
			      GTK_TOOLBAR_CHILD_RADIOBUTTON, button,
			      _("Details"), _("Show Key Details"),
			      _("details"), NULL,
			      GTK_SIGNAL_FUNC (keyring_set_detailed_listing),
			      editor);
  
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  icon = gpa_create_icon_widget (window, "help");
  item = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Help"),
				  _("Understanding the GNU Privacy Assistant"),
				  _("help"), icon, GTK_SIGNAL_FUNC (help_help),
				  NULL);

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
  gchar * key_id = gpa_default_key ();
  GpapaPublicKey * key;

  if (key_id)
    {
      key = gpapa_get_public_key_by_ID (key_id, gpa_callback, editor->window);
      gtk_label_set_text (GTK_LABEL (editor->status_key_user),
			  gpapa_key_get_name (GPAPA_KEY (key), gpa_callback,
					      editor->window));
      gtk_label_set_text (GTK_LABEL (editor->status_key_id),
			  gpapa_key_get_identifier (GPAPA_KEY (key),
						    gpa_callback,
						    editor->window));
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

  editor = xmalloc(sizeof(GPAKeyringEditor));
  editor->selection_sensitive_widgets = NULL;

  window = editor->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window),
			_("GNU Privacy Assistant - Keyring Editor"));
  gtk_object_set_data_full (GTK_OBJECT (window), "user_data", editor,
			    keyring_editor_destroy);
  /*gtk_window_set_default_size (GTK_WINDOW(window), 572, 400);*/
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_signal_connect_object (GTK_OBJECT (window), "map",
			     GTK_SIGNAL_FUNC (keyring_editor_mapped),
			     (gpointer)editor);

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

  label = gtk_label_new ("Keyring Editor");
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
				keylist_columns_brief, 10,
				window);
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

  statusbar = keyring_statusbar_new (editor);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (window), "gpa_default_key_changed",
		      (GtkSignalFunc)keyring_default_key_changed, editor);

  keyring_update_status_bar (editor);
  update_selection_sensitive_widgets (editor);

  return window;
} /* keyring_editor_new */

