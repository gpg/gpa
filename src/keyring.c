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
#include "keyring.h"

/*
 *	The public keyring editor
 */

struct _GPAKeyringEditor {
  GtkWidget *window;
  GtkCList  *clist_keys;
  GtkWidget *toggle_show;
  GList * selection_sensitive_widgets;
};
typedef struct _GPAKeyringEditor GPAKeyringEditor;

/* A simple sensitivity callback mechanism */
typedef gboolean (*SensitivityFunc)(gpointer);

static void
add_selection_sensitive_widget (GPAKeyringEditor * editor,
				GtkWidget * widget)
{
  editor->selection_sensitive_widgets \
    = g_list_append(editor->selection_sensitive_widgets, widget);
}

static void
update_selection_sensitive_widget (gpointer data, gpointer param)
{
  SensitivityFunc func;

  func = gtk_object_get_data (GTK_OBJECT (data), "gpa_sensitivity");
  gtk_widget_set_sensitive (GTK_WIDGET (data), func (param));
}

static void
update_selection_sensitive_widgets (GPAKeyringEditor * editor)
{
  g_list_foreach (editor->selection_sensitive_widgets,
		  update_selection_sensitive_widget,
		  (gpointer) editor);
}

static gboolean
keyring_editor_has_selection (gpointer param)
{
  GPAKeyringEditor * editor = param;

  return (editor->clist_keys->selection != NULL);
}

static gboolean
keyring_editor_has_single_selection (gpointer param)
{
  GPAKeyringEditor * editor = param;

  return (g_list_length (editor->clist_keys->selection) == 1);
}

void
keyring_editor_selection_changed (GtkWidget * clistKeys, gint row,
				  gint column, GdkEventButton * event,
				  gpointer param)
{
    GPAKeyringEditor * editor = param;

    update_selection_sensitive_widgets (editor);
}

void
keyring_editor_end_selection (GtkWidget * clistKeys, gpointer param)
{
    GPAKeyringEditor * editor = param;

    update_selection_sensitive_widgets (editor);
}



/* Fill the GtkCList with the keys */
void
keyring_editor_fill_keylist (GPAKeyringEditor * editor)
{
  gint contentsCountKeys;
  gchar *contents[5];
  GpapaKeytrust trust;
  GpapaPublicKey *key;
  GDate *expiryDate;
  int show_trust;
  int row;

  show_trust \
      = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (editor->toggle_show));
  gtk_clist_clear (editor->clist_keys);
  contentsCountKeys = gpapa_get_public_key_count(gpa_callback, editor->window);
  while (contentsCountKeys > 0)
    {
      contentsCountKeys--;
      key = gpapa_get_public_key_by_index (contentsCountKeys, gpa_callback,
					   editor->window);
      contents[0] = gpapa_key_get_name (GPAPA_KEY (key), gpa_callback,
					editor->window);
      trust = gpapa_public_key_get_keytrust (key, gpa_callback,
					     editor->window);
      contents[1] = gpa_keytrust_string (trust);
      if (show_trust)
	contents[2] = gpa_ownertrust_string (gpapa_public_key_get_ownertrust
					     (key, gpa_callback,
					      editor->window));
      else
	contents[2] = "";
      expiryDate = gpapa_key_get_expiry_date (GPAPA_KEY (key), gpa_callback,
					      editor->window);
      contents[3] = gpa_expiry_date_string (expiryDate);
      contents[4] = gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
					      editor->window);
      row = gtk_clist_append (editor->clist_keys, contents);
      gtk_clist_set_row_data_full (editor->clist_keys, row,
				   xstrdup (contents[4]), free);
    } /* while */
} /* keyring_editor_fill_keylist */


/* Edit the ownertrust level of the currently selected key */
static void
keyring_editor_edit_trust (gpointer param)
{
  GPAKeyringEditor * editor = param;
  GpapaPublicKey *key;
  GpapaOwnertrust ownertrust;
  gint row;
  gchar * key_id;
  gboolean result;

  /* find out which key is selected */
  if (!editor->clist_keys->selection)
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No key selected for editing."), editor->window);
      return;
    }

  row = GPOINTER_TO_INT (editor->clist_keys->selection->data);
  key_id = gtk_clist_get_row_data (editor->clist_keys, row);
  key = gpapa_get_public_key_by_ID (key_id, gpa_callback, editor->window);

  /* Let the user select a new one */
  result = gpa_ownertrust_run_dialog (key, editor->window,
				      "keyring_editor_public_edit_trust.tip",
				      &ownertrust);

  /* If the user clicked OK, set the new trust level */
  if (result)
    gpapa_public_key_set_ownertrust (key, ownertrust, gpa_callback,
				     editor->window);
} /* keyring_editor_edit_trust */




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

/* delete the selected keys */
static void
keyring_editor_delete (gpointer param)
{
  GPAKeyringEditor * editor = param;
  gint row;
  gchar * key_id;
  GpapaPublicKey *key;

  while (editor->clist_keys->selection)
    {
      row = GPOINTER_TO_INT (editor->clist_keys->selection->data);
      key_id = gtk_clist_get_row_data (editor->clist_keys, row);
      key = gpapa_get_public_key_by_ID (key_id, gpa_callback, editor->window);
      gpapa_public_key_delete (key, gpa_callback, editor->window);
      gtk_clist_remove (editor->clist_keys, row);
    }
} /* keyring_editor_delete */


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
  GpapaSignType sign_type = GPAPA_KEY_SIGN_LOCALLY;
  GList * selection;

  if (!editor->clist_keys->selection)
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No keys selected for signing."), editor->window);
      return;
    }

  if (!gpa_key_sign_run_dialog (editor->window, &sign_type, &private_key_id,
				&passphrase))
    return;


  selection = editor->clist_keys->selection;
  while (selection)
    {
      row = GPOINTER_TO_INT (editor->clist_keys->selection->data);
      key_id = gtk_clist_get_row_data (editor->clist_keys, row);
				   
      key = gpapa_get_public_key_by_ID (key_id, gpa_callback, editor->window);
      gpapa_public_key_sign (key, private_key_id, passphrase, sign_type,
			     gpa_callback, editor->window);
      selection = g_list_next (selection);
    }
      
  free (private_key_id);
  free (passphrase);
    
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
  gchar * filename;
  gboolean armored;
    

  if (!editor->clist_keys->selection)
    {
      /* this shouldn't happen because the button should be grayed out
       * in this case
       */
      gpa_window_error (_("No keys selected to export."), editor->window);
      return;
    } /* if */

  if (key_export_dialog_run (editor->window, &filename, &armored))
    {
      printf ("export to %s; armored %d\n", filename, armored);
      free (filename);
    }
}


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


/* generate a new key pair */
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
      printf ("User ID %s\n", params->userID);
      printf ("Email %s\n", params->email);
      printf ("Comment %s\n", params->comment);
      printf ("Password %s\n", params->password);
      printf ("Algorithm %s\n", gpa_algorithm_string(params->algo));
      printf ("Key Size %d\n", params->keysize);
      if (params->expiryDate)
	{
	  gchar buf[256];
	  g_date_strftime (buf, 256, "%d.%m.%Y", params->expiryDate);
	  printf ("Expiry Date %s\n", buf);
	}
      else if (params->interval)
	{
	  printf ("Expire in %d%c\n", params->interval, params->unit);
	}
      else
	{
	  printf ("Expire never\n");
	}

      gpapa_create_key_pair (&publicKey, &secretKey, params->password,
			     params->algo, params->keysize, params->userID,
			     params->email, params->comment,
			     gpa_callback, editor->window);
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

      /* finally, update the key list. */
      keyring_editor_fill_keylist (editor);
    }
}

static void
keyring_editor_generate_key_simple (gpointer param)
{
  GPAKeyringEditor * editor = param;

  if (gpa_keygen_wizard_run (editor->window))
    keyring_editor_fill_keylist (editor);
}

static void
keyring_editor_generate_key (gpointer param)
{
  if (gpa_simplified_ui ())
    keyring_editor_generate_key_simple (param);
  else
    keyring_editor_generate_key_advanced (param);
}

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
					_("You don not have a private key yet."
					  " Do you want to generate one now"
					  " (recommended) or do it later?"),
					buttons);
	  printf ("message box result: %s\n", result);
	  if (strcmp(result, _("Generate key now")) == 0)
	    keyring_editor_generate_key (param);
	  asked_about_key_generation = TRUE;
	}
    }
}

/* Create a new keyring editor */

GtkWidget *
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
    {_("/Keys/_Generate Key"), NULL, keyring_editor_generate_key, 0, NULL},
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


#define GPA_SENSITIVE_BUTTON(button, callback) \
  gtk_object_set_data (GTK_OBJECT (button), "gpa_sensitivity", callback); \
  add_selection_sensitive_widget (editor, (button))


GtkWidget *
keyring_editor_new (void)
{
  GPAKeyringEditor *editor;
  GtkAccelGroup *accelGroup;

  GtkWidget *windowPublic;
  GtkWidget *vboxPublic;
  GtkWidget *vboxKeys;
  GtkWidget *labelRingname;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;
  GtkWidget *hboxAction;
  GtkWidget *tableKey;
  GtkWidget *buttonEditKey;
  GtkWidget *buttonSign, *buttonSignBox;
  GtkWidget *buttonSend;
  GtkWidget *buttonReceive;
  GtkWidget *buttonExportKey;
  GtkWidget *buttonDelete, *buttonDeleteBox;
  GtkWidget *tableTrust;
  GtkWidget *toggleShow;
  GtkWidget *buttonEditTrust;
  GtkWidget *buttonExportTrust;
  GtkWidget *vboxTrust;
  GtkWidget *checkerArmor;
  GtkWidget *hSeparatorPublic;
  GtkWidget *hButtonBoxPublic;
  GtkWidget *buttonClose;

  gchar *titlesKeys[] = {
    _("Key owner"), _("Key trust"), _("Ownertrust"), _("Expiry date"),
    _("Key ID")
  };
  gint i;

  editor = xmalloc(sizeof(GPAKeyringEditor));
  editor->selection_sensitive_widgets = NULL;

  windowPublic = editor->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (windowPublic),
			_("GNU Privacy Assistant - Keyring Editor"));
  gtk_object_set_data_full (GTK_OBJECT (windowPublic), "user_data", editor,
			    keyring_editor_destroy);
  gtk_window_set_default_size (GTK_WINDOW(windowPublic), 572, 400);
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowPublic), accelGroup);
  gtk_signal_connect_object (GTK_OBJECT (windowPublic), "map",
			     GTK_SIGNAL_FUNC (keyring_editor_mapped),
			     (gpointer)editor);

  vboxPublic = gtk_vbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vboxPublic),
		      keyring_editor_menubar_new (windowPublic, editor),
		      FALSE, TRUE, 0);
  
  vboxKeys = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxKeys), 5);
  labelRingname = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (vboxKeys), labelRingname, FALSE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (labelRingname), 0, 0.5);
  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  clistKeys = gtk_clist_new_with_titles (5, titlesKeys);
  editor->clist_keys = GTK_CLIST (clistKeys);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys),
				GTK_SELECTION_EXTENDED);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 0, 185);
  for (i = 1; i < 4; i++)
    {
      gtk_clist_set_column_width (GTK_CLIST (clistKeys), i, 100);
      gtk_clist_set_column_justification (GTK_CLIST (clistKeys), i,
					  GTK_JUSTIFY_CENTER);
    }				/* for */
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 4, 120);
  for (i = 0; i < 5; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistKeys), i);
  gpa_connect_by_accelerator (GTK_LABEL (labelRingname), clistKeys,
			      accelGroup, _("_Public key Ring"));
  gtk_signal_connect (GTK_OBJECT (clistKeys), "select-row",
		      GTK_SIGNAL_FUNC (keyring_editor_selection_changed),
		      (gpointer) editor);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "unselect-row",
		      GTK_SIGNAL_FUNC (keyring_editor_selection_changed),
		      (gpointer) editor);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "end-selection",
		      GTK_SIGNAL_FUNC (keyring_editor_end_selection),
		      (gpointer) editor);
  /*  gtk_signal_connect (GTK_OBJECT (clistKeys), "button-press-event",
		      GTK_SIGNAL_FUNC (keyring_openPublic_evalMouse),
		      (gpointer) editor);*/
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_box_pack_start (GTK_BOX (vboxKeys), scrollerKeys, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxPublic), vboxKeys, TRUE, TRUE, 0);
  hboxAction = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxAction), 5);
  tableKey = gtk_table_new (3, 2, TRUE);
  buttonEditKey = gpa_button_new (accelGroup, _("_Edit key"));
/*  gtk_signal_connect_object (GTK_OBJECT (buttonEditKey), "clicked",
		      GTK_SIGNAL_FUNC (keyring_openPublic_editKey),
		      (gpointer) editor);*/
  gtk_table_attach (GTK_TABLE (tableKey), buttonEditKey, 0, 1, 0, 1, GTK_FILL,
		    GTK_FILL, 0, 0);
  GPA_SENSITIVE_BUTTON (buttonEditKey, keyring_editor_has_single_selection);

  /* the 'sign key' button */
  buttonSign = gtk_button_new();
  gtk_table_attach (GTK_TABLE (tableKey), buttonSign, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  /* associate this button with an icon */
  buttonSignBox = gpa_xpm_label_box(windowPublic, "gpa_sign_small",
				  _("_Sign keys..."), buttonSign, accelGroup);
  gtk_container_add (GTK_CONTAINER (buttonSign), buttonSignBox);
  GPA_SENSITIVE_BUTTON (buttonSign, keyring_editor_has_selection);
  gtk_signal_connect_object (GTK_OBJECT (buttonSign), "clicked",
			     GTK_SIGNAL_FUNC (keyring_editor_sign),
			     (gpointer)editor);

  buttonSend = gpa_button_new (accelGroup, _("Se_nd keys..."));
/*  gtk_signal_connect_object (GTK_OBJECT (buttonSend), "clicked",
		      GTK_SIGNAL_FUNC (keyring_openPublic_send),
		      (gpointer) editor);*/
  gtk_table_attach (GTK_TABLE (tableKey), buttonSend, 0, 1, 1, 2, GTK_FILL,
		    GTK_FILL, 0, 0);
  GPA_SENSITIVE_BUTTON (buttonSend, keyring_editor_has_selection);

  buttonReceive = gpa_button_new (accelGroup, _("_Receive keys..."));
  gtk_signal_connect_object (GTK_OBJECT (buttonReceive), "clicked",
			     GTK_SIGNAL_FUNC (keyring_editor_receive),
			     (gpointer) editor);
  gtk_table_attach (GTK_TABLE (tableKey), buttonReceive, 1, 2, 1, 2, GTK_FILL,
		    GTK_FILL, 0, 0);
  
  buttonExportKey = gpa_button_new (accelGroup, _("E_xport keys..."));
  gtk_signal_connect_object (GTK_OBJECT (buttonExportKey), "clicked",
			     GTK_SIGNAL_FUNC (keyring_editor_export),
			     (gpointer) editor);
  gtk_table_attach (GTK_TABLE (tableKey), buttonExportKey, 0, 1, 2, 3,
		    GTK_FILL, GTK_FILL, 0, 0);
  GPA_SENSITIVE_BUTTON (buttonExportKey, keyring_editor_has_selection);

  /* the 'delete key' button */
  buttonDelete = gtk_button_new();
  gtk_signal_connect_object (GTK_OBJECT (buttonDelete), "clicked",
			     GTK_SIGNAL_FUNC (keyring_editor_delete),
			     (gpointer) editor);
  gtk_table_attach (GTK_TABLE (tableKey), buttonDelete, 1, 2, 2, 3, GTK_FILL,
		    GTK_FILL, 0, 0);
  /* associate this button with an icon */
  buttonDeleteBox = gpa_xpm_label_box(windowPublic, "trash",
				  _("_Delete keys"), buttonDelete, accelGroup);
  gtk_container_add (GTK_CONTAINER (buttonDelete), buttonDeleteBox);
  GPA_SENSITIVE_BUTTON (buttonDelete, keyring_editor_has_selection);

  gtk_box_pack_start (GTK_BOX (hboxAction), tableKey, FALSE, FALSE, 0);

  vboxTrust = gtk_vbox_new (FALSE, 0);
  checkerArmor = gpa_check_button_new (accelGroup, _("ex_port armored"));
  gtk_box_pack_end (GTK_BOX (vboxTrust), checkerArmor, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hboxAction), vboxTrust, FALSE, FALSE, 0);

  tableTrust = gtk_table_new (3, 1, TRUE);
  toggleShow = editor->toggle_show \
    = gpa_toggle_button_new (accelGroup, _("S_how ownertrust"));
  gtk_signal_connect_object (GTK_OBJECT (toggleShow), "clicked",
			     GTK_SIGNAL_FUNC(keyring_editor_toggle_show_trust),
			     (gpointer) editor);
  gtk_table_attach (GTK_TABLE (tableTrust), toggleShow, 0, 1, 0, 1, GTK_FILL,
		    GTK_FILL, 0, 0);
  keyring_editor_fill_keylist (editor);
  buttonEditTrust = gpa_button_new (accelGroup, _("Edit _ownertrust..."));
  gtk_signal_connect_object (GTK_OBJECT (buttonEditTrust), "clicked",
			     GTK_SIGNAL_FUNC(keyring_editor_edit_trust),
			     (gpointer) editor);
  GPA_SENSITIVE_BUTTON (buttonEditTrust, keyring_editor_has_single_selection);

  gtk_table_attach (GTK_TABLE (tableTrust), buttonEditTrust, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  buttonExportTrust = gpa_button_new (accelGroup, _("Export o_wnertrust..."));
/*  gtk_signal_connect_object (GTK_OBJECT (buttonExportTrust), "clicked",
		      GTK_SIGNAL_FUNC (keyring_openPublic_exportTrust),
		      (gpointer) editor);*/
  gtk_table_attach (GTK_TABLE (tableTrust), buttonExportTrust, 0, 1, 2, 3,
		    GTK_FILL, GTK_FILL, 0, 0);
  GPA_SENSITIVE_BUTTON (buttonExportTrust, keyring_editor_has_selection);

  gtk_box_pack_end (GTK_BOX (hboxAction), tableTrust, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vboxPublic), hboxAction, FALSE, FALSE, 0);
  hSeparatorPublic = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vboxPublic), hSeparatorPublic, FALSE, FALSE,
		      0);
  hButtonBoxPublic = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxPublic),
			     GTK_BUTTONBOX_END);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxPublic), 5);
  buttonClose = gpa_button_new (accelGroup, _("_Close"));
  gtk_widget_add_accelerator (buttonClose, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  gtk_signal_connect_object (GTK_OBJECT (buttonClose), "clicked",
			     GTK_SIGNAL_FUNC (keyring_editor_close),
			     (gpointer) editor);
  gtk_container_add (GTK_CONTAINER (hButtonBoxPublic), buttonClose);
  gtk_box_pack_start (GTK_BOX (vboxPublic), hButtonBoxPublic, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowPublic), vboxPublic);

  update_selection_sensitive_widgets (editor);

  return windowPublic;
} /* keyring_editor_new */
#undef GPA_SENSITIVE_BUTTON

