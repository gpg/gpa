/* cardman.c  -  The GNU Privacy Assistant: card manager.
   Copyright (C) 2008, 2009 g10 Code GmbH

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

#include "gpa.h"   

#include "gtktools.h"
#include "gpawidgets.h"
#include "siglist.h"
#include "helpmenu.h"
#include "icons.h"
#include "cardman.h"
#include "convert.h"

#include "gpagenkeycardop.h"

#include "cm-object.h"
#include "cm-openpgp.h"
#include "cm-geldkarte.h"
#include "cm-netkey.h"



/* Object's class definition.  */
struct _GpaCardManagerClass 
{
  GtkWindowClass parent_class;
};


/* Object definition.  */
struct _GpaCardManager
{
  GtkWindow parent_instance;

  GtkUIManager *ui_manager;

  
  GtkWidget *card_container;  /* The container holding the card widget.  */
  GtkWidget *card_widget;     /* The widget to display a card applciation.  */


  /* Labels in the status bar.  */
  GtkWidget *status_label;
  GtkWidget *status_text;

  const char *cardtypename;  /* String with the card type's name.  */
  GType cardtype;            /* Widget type of a supported card.  */

  gpa_filewatch_id_t watch;  /* For watching the reader status file.  */
  int in_card_reload;        /* Sentinel for card_reload.  */


  gpgme_ctx_t gpgagent;      /* Gpgme context for the assuan
                                connection with the gpg-agent.  */

};

/* There is only one instance of the card manager class.  Use a global
   variable to keep track of it.  */
static GpaCardManager *this_instance;


/* Local prototypes */
static void update_card_widget (GpaCardManager *cardman);

static void gpa_card_manager_finalize (GObject *object);



/************************************************************ 
 *******************   Implementation   *********************
 ************************************************************/

/* Status bar handling.  */

static GtkWidget *
statusbar_new (GpaCardManager *cardman)
{
  GtkWidget *align;
  GtkWidget *hbox;
  GtkWidget *label;

  hbox = gtk_hbox_new (FALSE, 0);
 
  label = gtk_label_new (_("Status: "));
  cardman->status_label = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("");
  cardman->status_text = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
  
  align = gtk_alignment_new (0, 1, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), hbox);

  return align;
}


static void
statusbar_update (GpaCardManager *cardman, const char *text)
{
  gtk_label_set_text (GTK_LABEL (cardman->status_text), text);
}


static void
update_title (GpaCardManager *cardman)
{
  const char *title = _("GNU Privacy Assistant - Card Manager");

  if (cardman->cardtype == G_TYPE_NONE)
    gtk_window_set_title (GTK_WINDOW (cardman), title);
  else
    {
      char *tmp;
      
      tmp = g_strdup_printf ("%s (%s)", title, cardman->cardtypename);
      gtk_window_set_title (GTK_WINDOW (cardman), tmp);
      xfree (tmp);
    }
}



static void
update_info_visibility (GpaCardManager *cardman)
{
  if (cardman->cardtype != G_TYPE_NONE)
    {
      char *tmp = g_strdup_printf (_("%s card detected."),
                                   cardman->cardtypename);
      statusbar_update (cardman, tmp);
      xfree (tmp);
    }
  else
    {
      statusbar_update (cardman, _("Checking for card..."));
    }
}



static gpg_error_t
scd_data_cb (void *opaque, const void *data, size_t datalen)
{
/*   g_debug ("DATA_CB: datalen=%d", (int)datalen); */
  return 0;
}     


static gpg_error_t
scd_inq_cb (void *opaque, const char *name, const char *args,
            gpgme_assuan_sendfnc_t sendfnc,
            gpgme_assuan_sendfnc_ctx_t sendfnc_value)
{
/*   g_debug ("INQ_CB: name=`%s' args=`%s'", name, args); */

  return 0;
}     


static gpg_error_t
scd_status_cb (void *opaque, const char *status, const char *args)
{
  GpaCardManager *cardman = opaque;

/*   g_debug ("STATUS_CB: status=`%s'  args=`%s'", status, args); */

  if (!strcmp (status, "APPTYPE"))
    {
      cardman->cardtype = G_TYPE_NONE;
      if (!strcmp (args, "OPENPGP"))
        {
          cardman->cardtype = GPA_CM_OPENPGP_TYPE;
          cardman->cardtypename = "OpenPGP";
        }
      else if (!strcmp (args, "NKS"))
        {
          cardman->cardtype = GPA_CM_NETKEY_TYPE;
          cardman->cardtypename = "NetKey";
        }
      else if (!strcmp (args, "DINSIG"))
        cardman->cardtypename = "DINSIG";
      else if (!strcmp (args, "P15"))
        cardman->cardtypename = "PKCS#15";
      else if (!strcmp (args, "GELDKARTE"))
        {
          cardman->cardtype = GPA_CM_GELDKARTE_TYPE;
          cardman->cardtypename = "Geldkarte";
        }
      else
        cardman->cardtypename = "Unknown";

    }

  return 0;
}     


/* This function is called to trigger a card-reload.  */
static void
card_reload (GpaCardManager *cardman)
{
  gpg_error_t err;
  const char *command;

  if (!cardman->gpgagent)
    return;  /* No support for GPGME_PROTOCOL_ASSUAN.  */
    
  if (!cardman->in_card_reload)
    {
      cardman->in_card_reload++;
      
      update_info_visibility (cardman);

      cardman->cardtype = G_TYPE_NONE;
      cardman->cardtypename = "Unknown";

      /* The first thing we need to do is to issue the SERIALNO
         command; this makes sure that scdaemon initalizes the card if
         that has not yet been done.  */
      command = "SCD SERIALNO";
      err = gpgme_op_assuan_transact (cardman->gpgagent,
                                      command,
                                      scd_data_cb, NULL,
                                      scd_inq_cb, NULL,
                                      scd_status_cb, cardman);
      if (!err)
        err = gpgme_op_assuan_result (cardman->gpgagent);

      if (gpg_err_code (err) == GPG_ERR_CARD_NOT_PRESENT)
        ;
      else if (err)
        {
          g_debug ("assuan command `%s' failed: %s <%s>\n", 
                   command, gpg_strerror (err), gpg_strsource (err));
          statusbar_update (cardman, _("Error accessing card"));
        }

 
      if (!err)
        {
          /* Now we need to get the APPTYPE of the card so that the
             correct GpaCM* object can can act on the data.  */
          command = "SCD GETATTR APPTYPE";
          err = gpgme_op_assuan_transact (cardman->gpgagent,
                                          command,
                                          scd_data_cb, NULL,
                                          scd_inq_cb, NULL,
                                          scd_status_cb, cardman);
          if (!err)
            err = gpgme_op_assuan_result (cardman->gpgagent);

          if (gpg_err_code (err) == GPG_ERR_CARD_NOT_PRESENT)
            statusbar_update (cardman, _("No card"));
          else if (err)
            {
              g_debug ("assuan command `%s' failed: %s <%s>\n", 
                       command, gpg_strerror (err), gpg_strsource (err));
              statusbar_update (cardman, _("Error accessing card"));
            }
        }
      
      update_card_widget (cardman);
      update_title (cardman);
      
      update_info_visibility (cardman);
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


/* Idle queue callback to do a reload.  */
static gboolean
card_reload_idle_cb (void *user_data)
{
  GpaCardManager *cardman = user_data;
  
  card_reload (cardman);
  g_object_unref (cardman);

  return FALSE;  /* Remove us from the idle queue. */
}


/* Signal handler for the completed signal of the key generation. */
static void
card_genkey_completed (GpaCardManager *cardman, gpg_error_t err)
{
  g_object_ref (cardman);
  g_idle_add (card_reload_idle_cb, cardman);
}


/* This function is called to triggers a key-generation.  */
static void
card_genkey (GpaCardManager *cardman)
{
  GpaGenKeyCardOperation *op;
  gpg_error_t err;

  if (cardman->cardtype != GPA_CM_OPENPGP_TYPE)
    return;  /* Not possible.  */
  if (!cardman->gpgagent)
    {
      g_debug ("Ooops: no assuan context");
      return;
    }

  /* Note: This test works only with GnuPG > 2.0.10 but that version
     is anyway required for the card manager to work correctly.  */
  err = gpgme_op_assuan_transact (cardman->gpgagent,
                                  "SCD GETINFO deny_admin",
                                  NULL, NULL, NULL, NULL, NULL, NULL);
  if (!err)
    err = gpgme_op_assuan_result (cardman->gpgagent);
  if (!err)
    {
      gpa_window_error ("Admin commands are disabled in scdamon.\n"
                        "Key generation is not possible.", NULL); 
      return;
    }


  op = gpa_gen_key_card_operation_new (GTK_WIDGET (cardman));
  g_signal_connect_swapped (G_OBJECT (op), "completed",
                            G_CALLBACK (card_genkey_completed), cardman);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), NULL);
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

  if (cardman && strchr (reason, 'w') )
    {
/*       reader_status_t reader_status; */

/*       reader_status = reader_status_from_file (filename); */
/*       if (reader_status == READER_STATUS_PRESENT) */
/* 	statusbar_update (cardman, _("Reloading card data...")); */
      card_reload (cardman);
    }
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
      { "Edit", NULL, N_("_Edit"), NULL },
      { "Card", NULL, N_("_Card"), NULL },

      /* File menu.  */
      { "FileQuit", GTK_STOCK_QUIT, NULL, NULL,
	N_("Quit the program"), G_CALLBACK (gtk_main_quit) },

      /* Card menu.  */
      { "CardReload", GTK_STOCK_REFRESH, NULL, NULL,
	N_("Reload card information"), G_CALLBACK (card_reload_action) },
      { "CardGenkey", GTK_STOCK_NEW, "Generate new key...", NULL,
	N_("Generate new key on card"), G_CALLBACK (card_genkey_action) },
    };

  static const char *ui_description =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='File'>"
    "      <menuitem action='FileQuit'/>"
    "    </menu>"
    "    <menu action='Edit'>"
    "      <menuitem action='EditPreferences'/>"
    "      <menuitem action='EditBackendPreferences'/>"
    "    </menu>"
    "    <menu action='Card'>"
    "      <menuitem action='CardReload'/>"
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
    "    <toolitem action='EditPreferences'/>"
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
  this_instance = NULL;
}


static void
update_card_widget (GpaCardManager *cardman)
{
  if (cardman->card_widget)
    {
      gtk_widget_destroy (cardman->card_widget);
      cardman->card_widget = NULL;
    }
  /* Fixme: We should use a signal to get and reload the card widget.  */
  if (cardman->cardtype == GPA_CM_OPENPGP_TYPE)
    {
      cardman->card_widget = gpa_cm_openpgp_new ();
      gpa_cm_openpgp_reload (cardman->card_widget, cardman->gpgagent);
    }
  else if (cardman->cardtype == GPA_CM_GELDKARTE_TYPE)
    {
      cardman->card_widget = gpa_cm_geldkarte_new ();
      gpa_cm_geldkarte_reload (cardman->card_widget, cardman->gpgagent);
    }
  else if (cardman->cardtype == GPA_CM_NETKEY_TYPE)
    {
      cardman->card_widget = gpa_cm_netkey_new ();
      gpa_cm_netkey_reload (cardman->card_widget, cardman->gpgagent);
    }
  else
    {
      cardman->card_widget = gtk_label_new (_("No card found."));
    }

  gtk_scrolled_window_add_with_viewport 
    (GTK_SCROLLED_WINDOW (cardman->card_container), cardman->card_widget);

  gtk_widget_show_all (cardman->card_widget);
}


static void
construct_widgets (GpaCardManager *cardman)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *icon;
  gchar *markup;
  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkWidget *statusbar;

  /* Set a default size for the main window.  */
  gtk_window_set_default_size (GTK_WINDOW (cardman), 680, 480);

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

  /* Create a container (a scolled windows) which will take the actual
     card widgte.  This container is required so that we can easily
     change to a differet card widget. */
  cardman->card_container = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy 
    (GTK_SCROLLED_WINDOW (cardman->card_container),
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), cardman->card_container, TRUE, TRUE, 0);

  /* Update the container using the current card application.  */
  update_card_widget (cardman);

  statusbar = statusbar_new (cardman);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (cardman), vbox);

}


/************************************************************ 
 ******************   Object Management  ********************
 ************************************************************/

static void
gpa_card_manager_class_init (void *class_ptr, void *class_data)
{
  GpaCardManagerClass *klass = class_ptr;

  G_OBJECT_CLASS (klass)->finalize = gpa_card_manager_finalize;
}


static void
gpa_card_manager_init (GTypeInstance *instance, void *class_ptr)
{
  GpaCardManager *cardman = GPA_CARD_MANAGER (instance);
  gpg_error_t err;
  char *fname;

  cardman->cardtype = G_TYPE_NONE;
  cardman->cardtypename = "Unknown";
  update_title (cardman);

  construct_widgets (cardman);

  g_signal_connect (cardman, "destroy",
                    G_CALLBACK (card_manager_closed), cardman);


  fname = g_build_filename (gnupg_homedir, "reader_0.status", NULL);
  cardman->watch = gpa_add_filewatch (fname, "w", watcher_cb, cardman);
  xfree (fname);

  err = gpgme_new (&cardman->gpgagent);
  if (err)
    gpa_gpgme_error (err);

  err = gpgme_set_protocol (cardman->gpgagent, GPGME_PROTOCOL_ASSUAN);
  if (err)
    {
      if (gpg_err_code (err) == GPG_ERR_INV_VALUE)
        gpa_window_error (_("The GPGME library is too old to "
                            "support smartcards."), NULL);
      else
        gpa_gpgme_warning (err);
      gpgme_release (cardman->gpgagent);
      cardman->gpgagent = NULL;
    }
}


static void
gpa_card_manager_finalize (GObject *object)
{  
  GpaCardManager *cardman = GPA_CARD_MANAGER (object);

  gpgme_release (cardman->gpgagent);
  cardman->gpgagent = NULL;

  /* FIXME: Remove the watch object and all other resources.  */

  G_OBJECT_CLASS (g_type_class_peek_parent 
                  (GPA_CM_OPENPGP_GET_CLASS (cardman)))->finalize (object);
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
	  gpa_card_manager_class_init,
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  sizeof (GpaCardManager),
	  0,    /* n_preallocs */
	  gpa_card_manager_init,
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
  if (!this_instance)
    {
      this_instance = g_object_new (GPA_CARD_MANAGER_TYPE, NULL);  
      card_reload (this_instance);
    }
  return GTK_WIDGET (this_instance);
}


gboolean 
gpa_card_manager_is_open (void)
{
  return !!this_instance;
}
