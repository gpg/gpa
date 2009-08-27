/* gpa-key-window.c - per-key window
 * Copyright (C) 2009 g10 Code GmbH.
 *
 * This file is part of GPA.
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpa.h"   
#include "keytable.h"
#include "gpa-key-window.h"
#include "helpmenu.h"
#include "gpaexportfileop.h"
#include "icons.h"




/************************************************************ 
 *******************   Implementation   *********************
 ************************************************************/

static void
register_operation (GpaKeyWindow *self, GpaOperation *op)
{
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), self); 
}

/* Export the selected keys to a file.  */
static void
key_export (GtkAction *action, gpointer param)
{
  GpaKeyWindow *self = param;
  GpaExportFileOperation *op;
  GList *list;

  /* Create list with exactly one element: the gpgme key object. */
  list = g_list_append (NULL, self->key);

  op = gpa_export_file_operation_new (GTK_WIDGET (self), list);
  register_operation (self, GPA_OPERATION (op));
}

/* Handle menu item "Key/Close".  */
static void
key_close (GtkAction *action, gpointer param)
{
  GpaKeyWindow *key_window = param;
  gtk_widget_destroy (GTK_WIDGET (key_window));
}

/* Construct the menu [..].. */
static void
key_window_action_new (GpaKeyWindow *key_window, GtkWidget **menubar)
{
  static const GtkActionEntry entries[] =
    {
      /* Toplevel.  */
      { "Key", NULL, N_("_Key"), NULL },

      /* File menu.  */
      { "KeyExport", GPA_STOCK_EXPORT, N_("E_xport Key..."), NULL,
	N_("Export Key"), G_CALLBACK (key_export) },
      { "KeyClose", GTK_STOCK_CLOSE, NULL, NULL,
	N_("Close key"), G_CALLBACK (key_close) },
      { "KeyQuit", GTK_STOCK_QUIT, NULL, NULL,
	N_("Quit the program"), G_CALLBACK (gtk_main_quit) },
    };

  static const char *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='Key'>"
    "      <menuitem action='KeyExport'/>"
    "      <menuitem action='KeyClose'/>"
    "      <menuitem action='KeyQuit'/>"
    "    </menu>"
    "    <menu action='Help'>"
    "      <menuitem action='HelpAbout'/>"
    "    </menu>"
    "  </menubar>"
    "</ui>";

  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GError *error;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, PACKAGE);
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries),
				key_window);
  gtk_action_group_add_actions (action_group, gpa_help_menu_action_entries,
				G_N_ELEMENTS (gpa_help_menu_action_entries),
				key_window);

  key_window->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (key_window->ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (key_window->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (key_window), accel_group);
  if (! gtk_ui_manager_add_ui_from_string (key_window->ui_manager,
					   ui_description, -1, &error))
    {
      g_message ("building cardman menus failed: %s", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }

  /* Fixup the icon theme labels which are too long for the toolbar.  */

  *menubar = gtk_ui_manager_get_widget (key_window->ui_manager, "/MainMenu");
}




static void
gpa_key_window_class_init (void *class_ptr, void *class_data)
{
}

static void
close_button_cb (GtkButton *button, gpointer user_data)
{
  gtk_widget_destroy (GTK_WIDGET (user_data));
}

static void
gpa_key_window_init (GTypeInstance *instance, gpointer class_ptr)
{
  GpaKeyWindow *key_window = GPA_KEY_WINDOW (instance);
  GtkWidget *vbox;
  GtkWidget *button_box;
  GtkWidget *close_button;
  GtkWidget *halign;
  GtkWidget *menubar;
  GtkWidget *label;
  GtkWidget *title_hbox;
  GtkWidget *icon;

  key_window_action_new (key_window, &menubar);

  gtk_window_set_default_size (GTK_WINDOW (key_window), 600, 480);
  key_window->key_details = gpa_key_details_new ();

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);

  button_box = gtk_hbox_new (FALSE, 6);
  close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);

  g_signal_connect (G_OBJECT (close_button), "clicked",
		    G_CALLBACK (close_button_cb), key_window);

  halign = gtk_alignment_new (1, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (halign), close_button);
  gtk_container_add (GTK_CONTAINER (button_box), halign);

  gtk_container_set_border_width (GTK_CONTAINER (key_window), 0);

  label = gtk_label_new (NULL);
  title_hbox = gtk_hbox_new (FALSE, 4);
  icon = gtk_image_new ();
  gtk_container_set_border_width (GTK_CONTAINER (title_hbox), 12);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (title_hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (title_hbox), icon, FALSE, FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), title_hbox, FALSE, FALSE, 4);
  key_window->icon = icon;
  key_window->title_label = label;

  gtk_box_pack_start (GTK_BOX (vbox), key_window->key_details,
		      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 0);
  

  gtk_container_add (GTK_CONTAINER (key_window), vbox);
  gtk_widget_show_all (vbox);

  
}

/* Construct the class.  */
GType
gpa_key_window_get_type (void)
{
  static GType this_type = 0;
  
  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaKeyWindowClass),
	  NULL,
	  NULL,
	  (GClassInitFunc) gpa_key_window_class_init,
	  NULL,
	  NULL, /* class_data */
	  sizeof (GpaKeyWindow),
	  0,    /* n_preallocs */
	  (GInstanceInitFunc) gpa_key_window_init
	};
      
      this_type = g_type_register_static (GTK_TYPE_WINDOW,
                                          "GpaKeyWindow",
                                          &this_info, 0);
    }
  
  return this_type;
}



/* Create and return a new GpaKeyWindow widget. */
GtkWidget *
gpa_key_window_new (void)
{
  return GTK_WIDGET (g_object_new (GPA_KEY_WINDOW_TYPE, NULL));
}


/* Update the key window WIDGET with the key KEY. */
void
gpa_key_window_update (GtkWidget *widget, gpgme_key_t key)
{
  GpaKeyWindow *key_window;
  char *title_markup;
  char *uid;
  gpgme_key_t seckey;
  const char *stock;

  g_return_if_fail (IS_GPA_KEY_WINDOW (widget));
  key_window = GPA_KEY_WINDOW (widget);

  if (!key)
    return;

  gpgme_key_unref (key_window->key);
  gpgme_key_ref (key);
  key_window->key = key;

  /* Try to fetch secret part.  */
  if (key->subkeys->fpr)
    seckey = gpa_keytable_lookup_key (gpa_keytable_get_secret_instance (),
				      key->subkeys->fpr);
  else
    seckey = NULL;

  title_markup = NULL;
  uid = NULL;

  if (key->uids)
    uid = key->uids->uid;
  else
    uid = "";
  gtk_window_set_title (GTK_WINDOW (key_window), uid);

  title_markup = g_markup_printf_escaped ("<b>%s</b>",
					  key->uids ?
					  key->uids->uid : _("(empty)"));
  
  gtk_label_set_markup (GTK_LABEL (key_window->title_label), title_markup);
  g_free (title_markup);

  if (seckey)
    {
      if (seckey->subkeys && seckey->subkeys->is_cardkey)
	stock = GPA_STOCK_SECRET_CARDKEY;
      else
	stock = GPA_STOCK_SECRET_KEY;
    }
  else
    stock = GPA_STOCK_PUBLIC_KEY;

  gtk_image_set_from_stock (GTK_IMAGE (key_window->icon),
			    stock, GTK_ICON_SIZE_LARGE_TOOLBAR);

  gpa_key_details_update (key_window->key_details, key);
}
