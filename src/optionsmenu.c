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
  gchar *contentsKeys[2];

  contentsKeys[0] = (gchar*) gpgme_key_get_string_attr (key, GPGME_ATTR_FPR,
							NULL, 0);
  contentsKeys[1] = gpa_gpgme_key_get_userid (key, 0);

  gtk_clist_prepend (GTK_CLIST (clistKeys), (gchar**) contentsKeys);
  g_free (contentsKeys[1]);
}

static void
options_key (gpointer param)
{
  GpaWindowKeeper *keeper;
  GtkAccelGroup *accelGroup;
  gchar *titlesKeys[] = { _("Fingerprint"), _("User identity / role") };
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
