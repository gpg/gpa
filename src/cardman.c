/* cardman.c  -  The GNU Privacy Assistant: card manager.
   Copyright (C) 2008 g10 Code GmbH

   This file is part of GPA.

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

/* The card manager window.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "gpa.h"   
#include "gpapastrings.h"

#include "gtktools.h"
#include "gpawidgets.h"
#include "siglist.h"
#include "helpmenu.h"
#include "icons.h"
#include "cardman.h"

#include "gpacardreloadop.h"
#include "gpagenkeycardop.h"


/* Object and class definition.  */
struct _GpaCardManager
{
  GtkWindow parent;

  GtkWidget *window;

  GtkUIManager *ui_manager;

  GtkWidget *card_widget;     /* The container of all the info widgets.  */

  GtkWidget *entrySerialno;
  GtkWidget *entryVersion;
  GtkWidget *entryManufacturer;
  GtkWidget *entryLogin;
  GtkWidget *entryLanguage;
  GtkWidget *entryPubkeyUrl;
  GtkWidget *entryFirstName;
  GtkWidget *entryLastName;
  GtkWidget *entryKeySig;
  GtkWidget *entryKeyEnc;
  GtkWidget *entryKeyAuth;
#if 0
  GtkWidget *comboSex;
  GList *selection_sensitive_actions; /* ? */
#endif

  int have_card;             /* True, if a supported card is in the reader.  */
  gpa_filewatch_id_t watch;  /* For watching the reader status file.  */
  int in_card_reload;        /* Sentinel for card_reload.  */
};

struct _GpaCardManagerClass 
{
  GtkWindowClass parent_class;
};

/* There is only one instance of the card manager class.  Use a global
   variable to keep track of it.  */
static GpaCardManager *instance;

/* We also need to save the parent class. */
static GObjectClass *parent_class;

#if 0
/* FIXME: I guess we should use a more intelligent handling for widget
   sensitivity similar to that in fileman.c. -mo */
/* Definition of the sensitivity function type.  */
typedef gboolean (*sensitivity_func_t)(gpointer);
#endif


/* Local prototypes */



/************************************************************ 
 *******************   Implementation   *********************
 ************************************************************/

static void
update_title (GpaCardManager *cardman)
{
  const char *title = _("GNU Privacy Assistant - Card Manager");

  if (cardman->have_card)
    gtk_window_set_title (GTK_WINDOW (cardman), title);
  else
    {
      char *tmp;
      
      tmp = g_strdup_printf ("%s (%s)", title, _("no card"));
      gtk_window_set_title (GTK_WINDOW (cardman), tmp);
      xfree (tmp);
    }
}

static void
update_info_visibility (GpaCardManager *cardman)
{
  if (cardman->have_card)
    gtk_widget_show_all (cardman->card_widget);
  else
    gtk_widget_hide_all (cardman->card_widget);
}


/* This is the callback used by the GpaCardReloadOp object. It's
   called for each data item during a reload operation and updates the
   according widgets. */
static void
card_reload_cb (void *opaque,
                const char *identifier, int idx, const void *value)
{
  GpaCardManager *cardman = opaque;
  const char *string = value;

  if (!strcmp (identifier, "AID"))
    {
      if (idx == 0)
        {
          cardman->have_card = !!*string;
          update_info_visibility (cardman);
          update_title (cardman);
        }
    }
  else if (strcmp (identifier, "serial") == 0 && idx == 0)
    {
      while (*string == '0' && string[1])
        string++;
      gtk_entry_set_text (GTK_ENTRY (cardman->entrySerialno), string);
    }
  else if (strcmp (identifier, "login") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryLogin), string);
  else if (strcmp (identifier, "name") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryFirstName), string);
  else if (strcmp (identifier, "name") == 0 && idx == 1)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryLastName), string);
  else if (strcmp (identifier, "lang") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryLanguage), string);
  else if (strcmp (identifier, "url") == 0 && idx == 0)
    {
      char *tmp = decode_c_string (string);
      gtk_entry_set_text (GTK_ENTRY (cardman->entryPubkeyUrl), tmp);
      xfree (tmp);
    }
  else if (strcmp (identifier, "vendor") == 0 && idx == 1)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryManufacturer), string);
  else if (strcmp (identifier, "version") == 0 && idx == 0)
    {
      char buffer[6];
      char *p;

      if (strlen (string) == 4)
        {
          /* Reformat the version number to be better human readable.  */
          p = buffer;
          if (string[0] != '0')
            *p++ = string[0];
          *p++ = string[1];
          *p++ = '.';
          if (string[2] != '0')
            *p++ = string[2];
          *p++ = string[3];
          string = buffer;
        }
      gtk_entry_set_text (GTK_ENTRY (cardman->entryVersion), string);
    }
  else if (strcmp (identifier, "fpr") == 0 && idx == 0)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryKeySig), string);
  else if (strcmp (identifier, "fpr") == 0 && idx == 1)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryKeyEnc), string);
  else if (strcmp (identifier, "fpr") == 0 && idx == 2)
    gtk_entry_set_text (GTK_ENTRY (cardman->entryKeyAuth), string);
}


/* This function is called to trigger a card-reload.  */
static void
card_reload (GpaCardManager *cardman)
{
  GpaCardReloadOperation *op;

  if (!cardman->in_card_reload)
    {
      cardman->in_card_reload++;
      op = gpa_card_reload_operation_new (cardman->window, 
                                          card_reload_cb, cardman);
      g_signal_connect (G_OBJECT (op), "completed",
                        G_CALLBACK (g_object_unref), cardman);

      /* Fixme: We should decrement the sentinel only after finishing
         the operation, so that we don't run them over and over if the
         user clicks too often.  Note however that the primary reason
         for this sentinel is to avoid concurrent reloads triggered by
         the user and by the file watcher.  */
      cardman->in_card_reload--;
    }
}


/* This function is called when the user triggers a card-reload. */
static void
card_reload_action (GtkAction *action, gpointer param)
{
  GpaCardManager *cardman = param;

  card_reload (cardman);
}


#if 0
/* This function is called when the user triggers a card-reload. */
static void
card_edit (GtkAction *action, gpointer param)
{
  GpaCardManager *cardman = param;
  gpg_error_t err;

  fprintf (stderr, "CARD_EDIT\n");
  //err = gpa_gpgme_card_edit_modify_start (CTX, "123");
}
#endif

/* This function is called to triggers a key-generation.  */
static void
card_genkey (GpaCardManager *cardman)
{
  GpaGenKeyCardOperation *op;

  op = gpa_gen_key_card_operation_new (cardman->window);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), cardman);
}


/* This function is called when the user triggers a key-generation.  */
static void
card_genkey_action (GtkAction *action, gpointer param)
{
  GpaCardManager *cardman = param;

  card_genkey (cardman);
}


static void
watcher_cb (void *opaque, const char *filename, const char *reason)
{
  GpaCardManager *cardman = opaque;

  if (cardman && strchr (reason, 'c') )
    card_reload (cardman);
}


/* Construct the card manager menu and toolbar widgets and return
   them. */
static void
cardman_action_new (GpaCardManager *cardman, GtkWidget **menubar,
		    GtkWidget **toolbar)
{
  static const GtkActionEntry entries[] =
    {
      /* Toplevel.  */
      { "File", NULL, N_("_File"), NULL },
      { "Card", NULL, N_("_Card"), NULL },

      /* File menu.  */
      { "FileQuit", GTK_STOCK_QUIT, NULL, NULL,
	N_("Quit the program"), G_CALLBACK (gtk_main_quit) },

      /* Card menu.  */
      { "CardReload", GTK_STOCK_REFRESH, NULL, NULL,
	N_("Reload card information"), G_CALLBACK (card_reload_action) },
#if 0
      /* FIXME: not yet implemented. */
      { "CardEdit", GTK_STOCK_EDIT, NULL, NULL,
	N_("Edit card information"), G_CALLBACK (card_edit) },
#endif
      { "CardGenkey", GTK_STOCK_NEW, "Generate new key...", NULL,
	N_("Generate new key on card"), G_CALLBACK (card_genkey_action) },
    };

  static const char *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='File'>"
    "      <menuitem action='FileQuit'/>"
    "    </menu>"
    "    <menu action='Card'>"
    "      <menuitem action='CardReload'/>"
#if 0
    "      <menuitem action='CardEdit'/>"
#endif
    "      <menuitem action='CardGenkey'/>"
    "    </menu>"
    "    <menu action='Windows'>"
    "      <menuitem action='WindowsKeyringEditor'/>"
    "      <menuitem action='WindowsFileManager'/>"
    "      <menuitem action='WindowsClipboard'/>"
    "      <menuitem action='WindowsCardManager'/>"
    "    </menu>"
    "    <menu action='Help'>"
#if 0
    "      <menuitem action='HelpContents'/>"
#endif
    "      <menuitem action='HelpAbout'/>"
    "    </menu>"
    "  </menubar>"
    "  <toolbar name='ToolBar'>"
    "    <toolitem action='CardReload'/>"
#if 0
    "    <toolitem action='CardEdit'/>"
#endif
    "    <separator/>"
    "    <toolitem action='WindowsKeyringEditor'/>"
    "    <toolitem action='WindowsFileManager'/>"
    "    <toolitem action='WindowsClipboard'/>"
#if 0
    "    <toolitem action='HelpContents'/>"
#endif
    "  </toolbar>"
    "</ui>";

  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GtkAction *action;
  GError *error;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries),
				cardman);
  gtk_action_group_add_actions (action_group, gpa_help_menu_action_entries,
				G_N_ELEMENTS (gpa_help_menu_action_entries),
				cardman);
  gtk_action_group_add_actions (action_group, gpa_windows_menu_action_entries,
				G_N_ELEMENTS (gpa_windows_menu_action_entries),
				cardman);
  gtk_action_group_add_actions
    (action_group, gpa_preferences_menu_action_entries,
     G_N_ELEMENTS (gpa_preferences_menu_action_entries), cardman);
  cardman->ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (cardman->ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (cardman->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (cardman), accel_group);
  if (! gtk_ui_manager_add_ui_from_string (cardman->ui_manager, ui_description,
					   -1, &error))
    {
      g_message ("building cardman menus failed: %s", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }

  /* Fixup the icon theme labels which are too long for the toolbar.  */
  action = gtk_action_group_get_action (action_group, "WindowsKeyringEditor");
  g_object_set (action, "short_label", _("Keyring"), NULL);
  action = gtk_action_group_get_action (action_group, "WindowsFileManager");
  g_object_set (action, "short_label", _("Files"), NULL);

  *menubar = gtk_ui_manager_get_widget (cardman->ui_manager, "/MainMenu");
  *toolbar = gtk_ui_manager_get_widget (cardman->ui_manager, "/ToolBar");
  gpa_toolbar_set_homogeneous (GTK_TOOLBAR (*toolbar), FALSE);
}


/* Callback for the destroy signal.  */
static void
card_manager_closed (GtkWidget *widget, gpointer param)
{
  instance = NULL;
}


/* This function constructs the container holding the card "form". It
   updates CARDMAN with new references to the entry widgets, etc.  */
static GtkWidget *
construct_card_widget (GpaCardManager *cardman)
{
  GtkWidget *table;
  int rowidx = 0;

  table = gtk_table_new (4, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);

#define ADD_TABLE_ROW(label, widget) \
  { \
    GtkWidget *tmp_label = gtk_label_new (_(label)); \
    gtk_table_attach (GTK_TABLE (table), tmp_label, 0, 1, \
                      rowidx, rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0); \
    gtk_table_attach (GTK_TABLE (table), widget, 1, 2, \
                      rowidx, rowidx + 1, GTK_FILL, GTK_SHRINK, 0, 0); \
    rowidx++; \
  }
  
  cardman->entrySerialno = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entrySerialno), FALSE);
  ADD_TABLE_ROW ("Serial Number: ", cardman->entrySerialno);

  cardman->entryVersion = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryVersion), FALSE);
  ADD_TABLE_ROW ("Card Version: ", cardman->entryVersion);

  cardman->entryManufacturer = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryManufacturer), FALSE);
  ADD_TABLE_ROW ("Manufacturer: ", cardman->entryManufacturer);

  cardman->entryFirstName = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryFirstName), FALSE);
  ADD_TABLE_ROW ("First Name: ", cardman->entryFirstName);

  cardman->entryLastName = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryLastName), FALSE);
  ADD_TABLE_ROW ("Last Name: ", cardman->entryLastName);

#if 0
  cardman->comboSex = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (cardman->comboSex), _("Unspecified"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (cardman->comboSex), _("Female"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (cardman->comboSex), _("Male"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (cardman->comboSex), 0);
  ADD_TABLE_ROW ("Sex: ", cardman->comboSex);
#endif

  cardman->entryLanguage = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryLanguage), FALSE);
  ADD_TABLE_ROW ("Language: ", cardman->entryLanguage);

  cardman->entryLogin = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryLogin), FALSE);
  ADD_TABLE_ROW ("Login Data: ", cardman->entryLogin);

  cardman->entryPubkeyUrl = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryPubkeyUrl), FALSE);
  ADD_TABLE_ROW ("Public key URL: ", cardman->entryPubkeyUrl);

  cardman->entryKeySig = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryKeySig), FALSE);
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entryKeySig), 42);
  ADD_TABLE_ROW ("Signature Key: ", cardman->entryKeySig);

  cardman->entryKeyEnc = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryKeyEnc), FALSE);
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entryKeyEnc), 42);
  ADD_TABLE_ROW ("Encryption Key: ", cardman->entryKeyEnc);

  cardman->entryKeyAuth = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (cardman->entryKeyAuth), FALSE);
  gtk_entry_set_width_chars (GTK_ENTRY (cardman->entryKeyAuth), 42);
  ADD_TABLE_ROW ("Authentication Key: ", cardman->entryKeyAuth);

  return table;
}




/************************************************************ 
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_card_manager_finalize (GObject *object)
{  
  /* FIXME: Remove the watch object and all other resources.  */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
gpa_card_manager_init (GpaCardManager *cardman)
{
  char *fname;

  fname = g_build_filename (gnupg_homedir, "reader_0.status", NULL);
  cardman->watch = gpa_add_filewatch (fname, "c", watcher_cb, cardman);
  xfree (fname);
}

/* Construct a new class object of GpaCardManager.  */
static GObject*
gpa_card_manager_constructor (GType type,
			      guint n_construct_properties,
			      GObjectConstructParam *construct_properties)
{
  GObject *object;
  GpaCardManager *cardman;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *icon;
  gchar *markup;
  GtkWidget *menubar;
  GtkWidget *toolbar;

  /* Invoke parent's constructor.  */
  object = parent_class->constructor (type,
				      n_construct_properties,
				      construct_properties);
  cardman = GPA_CARD_MANAGER (object);

  cardman->have_card = 1;

  cardman->entryLogin = NULL;

  cardman->entrySerialno = NULL;
  cardman->entryVersion = NULL;
  cardman->entryManufacturer = NULL;
  cardman->entryLogin = NULL;
  cardman->entryLanguage = NULL;
  cardman->entryPubkeyUrl = NULL;
  cardman->entryFirstName = NULL;
  cardman->entryLastName = NULL;
  cardman->entryKeySig = NULL;
  cardman->entryKeyEnc = NULL;
  cardman->entryKeyAuth = NULL;


  /* Initialize.  */
  update_title (cardman);
  gtk_window_set_default_size (GTK_WINDOW (cardman), 640, 480);
  /* Realize the window so that we can create pixmaps without warnings.  */
  gtk_widget_realize (GTK_WIDGET (cardman));

  /* Use a vbox to show the menu, toolbar and the file container.  */
  vbox = gtk_vbox_new (FALSE, 0);

  /* Get the menu and the toolbar.  */
  cardman_action_new (cardman, &menubar, &toolbar);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);


  /* Add a fancy label that tells us: This is the card manager.  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);
  
  icon = gtk_image_new_from_stock (GPA_STOCK_CARDMAN, GTK_ICON_SIZE_DND);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);

  label = gtk_label_new (NULL);
  markup = g_strdup_printf ("<span font_desc=\"16\">%s</span>",
                            _("Card Manager"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 10);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  cardman->card_widget = construct_card_widget (cardman);
  gtk_box_pack_start (GTK_BOX (vbox), cardman->card_widget, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (cardman), vbox);

  g_signal_connect (object, "destroy",
                    G_CALLBACK (card_manager_closed), object);

  gtk_widget_show_all (cardman->card_widget);
  gtk_widget_hide_all (cardman->card_widget);
  return object;
}


static void
gpa_card_manager_class_init (GpaCardManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->constructor = gpa_card_manager_constructor;
  object_class->finalize = gpa_card_manager_finalize;
}


GType
gpa_card_manager_get_type (void)
{
  static GType this_type = 0;
  
  if (!this_type)
    {
      static const GTypeInfo this_info =
	{
	  sizeof (GpaCardManagerClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gpa_card_manager_class_init,
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  sizeof (GpaCardManager),
	  0,    /* n_preallocs */
	  (GInstanceInitFunc) gpa_card_manager_init,
	};
      
      this_type = g_type_register_static (GTK_TYPE_WINDOW,
                                          "GpaCardManager",
                                          &this_info, 0);
    }
  
  return this_type;
}


/************************************************************ 
 **********************  Public API  ************************
 ************************************************************/
GtkWidget *
gpa_card_manager_get_instance (void)
{
  if (!instance)
    {
      instance = g_object_new (GPA_CARD_MANAGER_TYPE, NULL);  
      card_reload (instance);
    }
  return GTK_WIDGET (instance);
}


gboolean 
gpa_card_manager_is_open (void)
{
  return (instance != NULL);
}
