/* optionsmenu.c  -  The GNU Privacy Assistant
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "gpa.h"
#include <config.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gpawindowkeeper.h"
#include "gtktools.h"
#include "keysmenu.h"
#include "keyserver.h"


static void
options_keyserver_set (gpointer param)
{
  gpointer *localParam;
  GtkWidget *comboServer;
  GpaWindowKeeper *keeperServer;
  gpointer paramDone[2];
  GtkWidget *entry;

  localParam = (gpointer *) param;
  comboServer = (GtkWidget *) localParam[0];
  keeperServer = (GpaWindowKeeper *) localParam[1];

  entry = GTK_COMBO (comboServer)->entry;
  keyserver_set_current (gtk_entry_get_text (GTK_ENTRY (entry)));

  paramDone[0] = keeperServer;
  paramDone[1] = NULL;
  gpa_window_destroy (paramDone);
}


static void
options_keyserver_destroy (GtkWidget * widget, gpointer param)
{
  gtk_main_quit ();
}


static void
options_keyserver (gpointer param)
{
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gpointer *paramClose;
  gpointer *paramSet;
  GtkWidget * main_window = param;

  GtkWidget *windowServer;
  GtkWidget *vboxServer;
  GtkWidget *hboxServer;
  GtkWidget *labelServer;
  GtkWidget *comboServer;
  GtkWidget *hButtonBoxServer;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSet;

  keeper = gpa_windowKeeper_new ();
  windowServer = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpa_windowKeeper_set_window (keeper, windowServer);
  gtk_window_set_title (GTK_WINDOW (windowServer), _("Set key server"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowServer), accelGroup);
  gtk_signal_connect (GTK_OBJECT (windowServer), "destroy",
		      GTK_SIGNAL_FUNC (options_keyserver_destroy), NULL);

  vboxServer = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxServer), 5);
  hboxServer = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hboxServer), 5);
  labelServer = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hboxServer), labelServer, FALSE, FALSE, 0);
  comboServer = gtk_combo_new ();
  gpa_connect_by_accelerator (GTK_LABEL (labelServer),
			      GTK_COMBO (comboServer)->entry, accelGroup,
			      _("_Key server: "));
  gtk_combo_set_value_in_list (GTK_COMBO (comboServer), FALSE, FALSE);

  gtk_combo_set_popdown_strings (GTK_COMBO (comboServer), 
                                 keyserver_get_as_glist () );
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (comboServer)->entry),
		      keyserver_get_current (TRUE));
  gtk_box_pack_start (GTK_BOX (hboxServer), comboServer, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxServer), hboxServer, TRUE, TRUE, 0);
  hButtonBoxServer = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxServer),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxServer), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxServer), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  buttonSet = gpa_button_new (accelGroup, _("_Set"));
  paramSet = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSet);
  paramSet[0] = comboServer;
  paramSet[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonSet), "clicked",
			     GTK_SIGNAL_FUNC (options_keyserver_set),
			     (gpointer) paramSet);
  gtk_container_add (GTK_CONTAINER (hButtonBoxServer), buttonSet);
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxServer), buttonCancel);
  gtk_box_pack_start (GTK_BOX (vboxServer), hButtonBoxServer, FALSE, FALSE,
		      0);
  gtk_container_add (GTK_CONTAINER (windowServer), vboxServer);

  gtk_window_set_modal (GTK_WINDOW (windowServer), TRUE);
  gpa_window_show_centered (windowServer, main_window);
  gtk_main ();
} /* options_keyserver */

/* This confuses novice users --> disabled for now.
 * This can be put into the "advanced" mode, some day.
 */
#if 0

/* This uses gpapa. If it is ever enabled again, update it */

#include <gpapa.h>

static void
options_recipients_fillDefault (gpointer data, gpointer userData)
{
/* var */
  const gchar *keyID;
  GtkWidget *clistDefault;
  GpapaPublicKey *key;
  gchar *contentsDefault[2];
/* commands */
  keyID = (gchar *) data;
  clistDefault = (GtkWidget *) userData;
  key = gpapa_get_public_key_by_ID (keyID, gpa_callback, global_windowMain);
  contentsDefault[0] =
    gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, global_windowMain);
  contentsDefault[1] = (gchar *) keyID;
  gtk_clist_append (GTK_CLIST (clistDefault), contentsDefault);
}

static void
options_recipients_set (gpointer param)
{
/* var */
  gpointer *localParam;
  GtkWidget *clistDefault;
  GpaWindowKeeper *keeperDefault;
  gint rows, i;
  gchar *keyID;
/* commands */
  localParam = (gpointer *) param;
  keeperDefault = (GpaWindowKeeper *) localParam[2];
  clistDefault = (GtkWidget *) localParam[3];
  g_list_free (global_defaultRecipients);
  global_defaultRecipients = NULL;
  rows = GTK_CLIST (clistDefault)->rows;
  for (i = 0; i < rows; i++)
    {
      gtk_clist_get_text (GTK_CLIST (clistDefault), i, 0, &keyID);
      global_defaultRecipients =
	g_list_append (global_defaultRecipients, xstrdup_or_null (keyID));
    }
  gpa_recipientWindow_close (param);
}


static void
options_recipients_destroy (GtkWidget * widget, gpointer param)
{
  gtk_main_quit ();
}

static void
options_recipients (gpointer param)
{
  GpaWindowKeeper *keeper;
  gint contentsKeyCount;
  GtkAccelGroup *accelGroup;
  gchar *titlesAnyClist[] = { _("Key ID"), _("Key owner") };
  gint i;
  GList **recipientsSelected = NULL;
  gchar *contentsAnyClist[2];
  GpapaPublicKey *key;
  GList **keysSelected = NULL;
  static gint columnKeyID = 1;
  gpointer *paramKeys;
  gpointer *paramRemove;
  gpointer *paramAdd;
  gpointer *paramCancel;
  gpointer *paramSet;
  GtkWidget *parent = param;

  GtkWidget *windowRecipients;
  GtkWidget *vboxRecipients;
  GtkWidget *hboxRecipients;
  GtkWidget *vboxDefault;
  GtkWidget *labelJfdDefault;
  GtkWidget *labelDefault;
  GtkWidget *scrollerDefault;
  GtkWidget *clistDefault;
  GtkWidget *vboxKeys;
  GtkWidget *labelJfdKeys;
  GtkWidget *labelKeys;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;
  GtkWidget *hButtonBoxRecipients;
  GtkWidget *buttonDelete;
  GtkWidget *buttonAdd;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSet;

  contentsKeyCount =
    gpapa_get_public_key_count (gpa_callback, global_windowMain);
  if (!contentsKeyCount)
    {
      gpa_window_error (_
			("No public keys available to denote\nas default recipients."),
global_windowMain);
      return;
    } /* if */

  keeper = gpa_windowKeeper_new ();
  windowRecipients = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpa_windowKeeper_set_window (keeper, windowRecipients);
  gtk_window_set_title (GTK_WINDOW (windowRecipients),
			_("Set default recipients"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowRecipients), accelGroup);
  gtk_signal_connect (GTK_OBJECT (windowRecipients), "destroy",
		      GTK_SIGNAL_FUNC (options_recipients_destroy), NULL);
  
  vboxRecipients = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxRecipients), 5);
  hboxRecipients = gtk_hbox_new (TRUE, 0);
  vboxDefault = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxDefault), 5);
  labelDefault = gtk_label_new ("");
  labelJfdDefault =
    gpa_widget_hjustified_new (labelDefault, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxDefault), labelJfdDefault, FALSE, FALSE,
		      0);
  scrollerDefault = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerDefault, 250, 140);
  clistDefault = gtk_clist_new_with_titles (2, titlesAnyClist);
  gtk_clist_set_column_width (GTK_CLIST (clistDefault), 0, 120);
  gtk_clist_set_column_width (GTK_CLIST (clistDefault), 1, 200);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistDefault), i);
  gtk_clist_set_selection_mode (GTK_CLIST (clistDefault),
				GTK_SELECTION_EXTENDED);
  recipientsSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, recipientsSelected);
  *recipientsSelected = NULL;
  gtk_signal_connect (GTK_OBJECT (clistDefault), "select-row",
		      GTK_SIGNAL_FUNC (gpa_selectRecipient),
		      (gpointer) recipientsSelected);
  gtk_signal_connect (GTK_OBJECT (clistDefault), "unselect-row",
		      GTK_SIGNAL_FUNC (gpa_unselectRecipient),
		      (gpointer) recipientsSelected);
  gpa_connect_by_accelerator (GTK_LABEL (labelDefault), clistDefault,
			      accelGroup, _("_Recipients"));
  g_list_foreach (global_defaultRecipients, options_recipients_fillDefault,
		  clistDefault);
  gtk_container_add (GTK_CONTAINER (scrollerDefault), clistDefault);
  gtk_box_pack_start (GTK_BOX (vboxDefault), scrollerDefault, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hboxRecipients), vboxDefault, TRUE, TRUE, 0);
  vboxKeys = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxKeys), 5);
  labelKeys = gtk_label_new ("");
  labelJfdKeys = gpa_widget_hjustified_new (labelKeys, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxKeys), labelJfdKeys, FALSE, FALSE, 0);
  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerKeys, 250, 140);
  clistKeys = gtk_clist_new_with_titles (2, titlesAnyClist);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 0, 120);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 1, 200);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistKeys), i);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys),
				GTK_SELECTION_EXTENDED);
  gpa_connect_by_accelerator (GTK_LABEL (labelKeys), clistKeys, accelGroup,
			      _("_Public keys"));
  keysSelected = (GList **) xmalloc (sizeof (GList *));
  gpa_windowKeeper_add_param (keeper, keysSelected);
  *keysSelected = NULL;
  paramKeys = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramKeys);
  paramKeys[0] = keysSelected;
  paramKeys[1] = &columnKeyID;
  paramKeys[2] = windowRecipients;
  gtk_signal_connect (GTK_OBJECT (clistKeys), "select-row",
		      GTK_SIGNAL_FUNC (keys_selectKey), (gpointer) paramKeys);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "unselect-row",
		      GTK_SIGNAL_FUNC (keys_unselectKey),
		      (gpointer) paramKeys);
  while (contentsKeyCount)
    {
      contentsKeyCount--;
      key =
	gpapa_get_public_key_by_index (contentsKeyCount, gpa_callback,
				       global_windowMain);
      contentsAnyClist[0] =
	gpapa_key_get_identifier (GPAPA_KEY (key), gpa_callback,
				  global_windowMain);
      contentsAnyClist[1] =
	gpapa_key_get_name (GPAPA_KEY (key), gpa_callback, global_windowMain);
      gtk_clist_prepend (GTK_CLIST (clistKeys), contentsAnyClist);
    }				/* while */
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gtk_box_pack_start (GTK_BOX (vboxKeys), scrollerKeys, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hboxRecipients), vboxKeys, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxRecipients), hboxRecipients, TRUE, TRUE,
		      0);
  hButtonBoxRecipients = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxRecipients),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxRecipients), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxRecipients), 5);
  buttonDelete = gpa_button_new (accelGroup, _("_Remove from recipients"));
  paramRemove = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramRemove);
  paramRemove[0] = recipientsSelected;
  paramRemove[1] = clistDefault;
  paramRemove[2] = windowRecipients;
  gtk_signal_connect_object (GTK_OBJECT (buttonDelete), "clicked",
			     GTK_SIGNAL_FUNC (gpa_removeRecipients),
			     (gpointer) paramRemove);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRecipients), buttonDelete);
  buttonAdd = gpa_button_new (accelGroup, _("_Add to recipients"));
  paramAdd = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramAdd);
  paramAdd[0] = keysSelected;
  paramAdd[1] = clistKeys;
  paramAdd[2] = clistDefault;
  paramAdd[3] = windowRecipients;
  gtk_signal_connect_object (GTK_OBJECT (buttonAdd), "clicked",
			     GTK_SIGNAL_FUNC (gpa_addRecipients),
			     (gpointer) paramAdd);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRecipients), buttonAdd);
  buttonCancel = gpa_button_new (accelGroup, _("_Cancel"));
  paramCancel = (gpointer *) xmalloc (3 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramCancel);
  paramCancel[0] = recipientsSelected;
  paramCancel[1] = keysSelected;
  paramCancel[2] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonCancel), "clicked",
			     GTK_SIGNAL_FUNC (gpa_recipientWindow_close),
			     (gpointer) paramCancel);
  gtk_widget_add_accelerator (buttonCancel, "clicked", accelGroup, GDK_Escape,
			      0, 0);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRecipients), buttonCancel);
  buttonSet = gpa_button_new (accelGroup, _("_Set"));
  paramSet = (gpointer *) xmalloc (4 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSet);
  paramSet[0] = recipientsSelected;
  paramSet[1] = keysSelected;
  paramSet[2] = keeper;
  paramSet[3] = clistDefault;
  gtk_signal_connect_object (GTK_OBJECT (buttonSet), "clicked",
			     GTK_SIGNAL_FUNC (options_recipients_set),
			     (gpointer) paramSet);
  gtk_container_add (GTK_CONTAINER (hButtonBoxRecipients), buttonSet);
  gtk_box_pack_start (GTK_BOX (vboxRecipients), hButtonBoxRecipients, FALSE,
		      FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowRecipients), vboxRecipients);

  gtk_window_set_modal (GTK_WINDOW (windowRecipients), TRUE);
  gpa_window_show_centered (windowRecipients, parent);
  gtk_main ();
} /* options_recipients */

#endif /* disabled */


static void
options_key_select (GtkCList * clist, gint row, gint column,
		    GdkEventButton * event, gpointer userData)
{
/* var */
  gchar **keyID;
/* commands */
  keyID = (gchar **) userData;
  gtk_clist_get_text (clist, row, 0, keyID);
}

static void
options_key_set (gpointer param)
{
/* var */
  gpointer *localParam;
  gchar **keyID;
  GpaWindowKeeper *keeperKey;
  gpointer paramDone[2];
/* commands */
  localParam = (gpointer *) param;
  keyID = (gchar **) localParam[0];
  keeperKey = (GpaWindowKeeper *) localParam[1];
  gpa_set_default_key (xstrdup_or_null (*keyID));
  paramDone[0] = keeperKey;
  paramDone[1] = NULL;
  gpa_window_destroy (paramDone);
}				/* options_key_set */


static void
options_key_destroy (GtkWidget * widget, gpointer param)
{
  gtk_main_quit ();
}

/* Callback for a gpa_keytable_foreach, that adds a key to the CList */
static void
add_key (const gchar *id, GpgmeKey key, GtkWidget *clistKeys)
{
  const gchar *contentsKeys[2];

  contentsKeys[0] = gpgme_key_get_string_attr (key, GPGME_ATTR_KEYID, NULL, 0);
  contentsKeys[1] = gpgme_key_get_string_attr (key, GPGME_ATTR_USERID, NULL, 0);

  gtk_clist_prepend (GTK_CLIST (clistKeys), (gchar**) contentsKeys);
}

static void
options_key (gpointer param)
{
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gchar *titlesKeys[] = { _("Key ID"), _("User identity / role") };
  gchar **keyID;
  gint i, rows;
  gpointer *paramSet;
  gpointer *paramClose;
  GtkWidget *parent = param;

  GtkWidget *windowKey;
  GtkWidget *vboxKey;
  GtkWidget *vboxKeys;
  GtkWidget *labelJfdKeys;
  GtkWidget *labelKeys;
  GtkWidget *scrollerKeys;
  GtkWidget *clistKeys;
  GtkWidget *hButtonBoxKey;
  GtkWidget *buttonCancel;
  GtkWidget *buttonSet;

  if (!gpa_keytable_secret_size (keytable))
    {
      gpa_window_error (_("No secret keys available to\n"
			  "select a default key from."), global_windowMain);
      return;
    }
  keeper = gpa_windowKeeper_new ();

  windowKey = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gpa_windowKeeper_set_window (keeper, windowKey);
  gtk_window_set_title (GTK_WINDOW (windowKey), _("Set default key"));
  accelGroup = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (windowKey), accelGroup);
  gtk_signal_connect (GTK_OBJECT (windowKey), "destroy",
		      (GtkSignalFunc)options_key_destroy, NULL);

  vboxKey = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxKey), 5);
  vboxKeys = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vboxKeys), 5);
  labelKeys = gtk_label_new ("");
  labelJfdKeys = gpa_widget_hjustified_new (labelKeys, GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vboxKeys), labelJfdKeys, FALSE, FALSE, 0);
  scrollerKeys = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrollerKeys, 250, 120);
  clistKeys = gtk_clist_new_with_titles (2, titlesKeys);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 0, 120);
  gtk_clist_set_column_width (GTK_CLIST (clistKeys), 1, 200);
  for (i = 0; i < 2; i++)
    gtk_clist_column_title_passive (GTK_CLIST (clistKeys), i);
  gtk_clist_set_selection_mode (GTK_CLIST (clistKeys), GTK_SELECTION_SINGLE);
  keyID = (gchar **) xmalloc (sizeof (gchar *));
  gpa_windowKeeper_add_param (keeper, keyID);
  gtk_signal_connect (GTK_OBJECT (clistKeys), "select-row",
		      GTK_SIGNAL_FUNC (options_key_select), (gpointer) keyID);
  /* Add the keys to the list */
  gpa_keytable_secret_foreach (keytable, (GPATableFunc)add_key, clistKeys);
  if (gpa_default_key ())
    {
      i = 0;
      rows = GTK_CLIST (clistKeys)->rows;
      gtk_clist_get_text (GTK_CLIST (clistKeys), i, 0, keyID);
      while (i < rows && strcmp (gpa_default_key (), *keyID) != 0)
	{
	  i++;
	  if (i < rows)
	    gtk_clist_get_text (GTK_CLIST (clistKeys), i, 0, keyID);
	}			/* while */
      if (i < rows)
	gtk_clist_select_row (GTK_CLIST (clistKeys), i, 0);
    }				/* if */
  gtk_container_add (GTK_CONTAINER (scrollerKeys), clistKeys);
  gpa_connect_by_accelerator (GTK_LABEL (labelKeys), clistKeys, accelGroup,
			      _("Default _key:"));
  gtk_box_pack_start (GTK_BOX (vboxKeys), scrollerKeys, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vboxKey), vboxKeys, TRUE, TRUE, 0);
  hButtonBoxKey = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hButtonBoxKey),
			     GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hButtonBoxKey), 10);
  gtk_container_set_border_width (GTK_CONTAINER (hButtonBoxKey), 5);
  paramClose = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramClose);
  paramClose[0] = keeper;
  paramClose[1] = NULL;
  buttonCancel = gpa_buttonCancel_new (accelGroup, _("_Cancel"), paramClose);
  gtk_container_add (GTK_CONTAINER (hButtonBoxKey), buttonCancel);
  buttonSet = gpa_button_new (accelGroup, _("_Set"));
  paramSet = (gpointer *) xmalloc (2 * sizeof (gpointer));
  gpa_windowKeeper_add_param (keeper, paramSet);
  paramSet[0] = keyID;
  paramSet[1] = keeper;
  gtk_signal_connect_object (GTK_OBJECT (buttonSet), "clicked",
			     GTK_SIGNAL_FUNC (options_key_set),
			     (gpointer) paramSet);
  gtk_container_add (GTK_CONTAINER (hButtonBoxKey), buttonSet);
  gtk_box_pack_start (GTK_BOX (vboxKey), hButtonBoxKey, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (windowKey), vboxKey);

  gtk_window_set_modal (GTK_WINDOW (windowKey), TRUE);
  gpa_window_show_centered (windowKey, parent);
  gtk_main ();
} /* options_key */


void
gpa_options_menu_add_to_factory (GtkItemFactory *factory, GtkWidget *window)
{
  GtkItemFactoryEntry menu[] = {
    {_("/_Options"), NULL, NULL, 0, "<Branch>"},
    {_("/Options/_Keyserver"), NULL, options_keyserver, 0, NULL},
/*    {_("/Options/Default _Recipients"), NULL, options_recipients, 0, NULL}, */
    {_("/Options/_Default Key"), NULL, options_key, 0, NULL},
  };

  gtk_item_factory_create_items (factory, sizeof (menu) / sizeof (menu[0]),
				 menu, window);
}
