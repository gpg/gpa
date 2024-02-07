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
#include "membuf.h"

#include "gpagenkeycardop.h"

#include "cm-object.h"
#include "cm-openpgp.h"
#include "cm-geldkarte.h"
#include "cm-netkey.h"
#include "cm-dinsig.h"
#include "cm-piv.h"
#include "cm-unknown.h"


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


  GtkWidget *app_selector;    /* Combo Box to select the application.  */

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


  guint ticker_timeout_id;   /* Source Id of the timeout ticker or 0.  */


  struct {
    int card_any;           /* Any card event counter seen.  */
    unsigned int card;      /* Last seen card event counter.  */
  } eventcounter;

};

/* There is only one instance of the card manager class.  Use a global
   variable to keep track of it.  */
static GpaCardManager *this_instance;


/* Local prototypes */
static void start_ticker (GpaCardManager *cardman);
static void update_card_widget (GpaCardManager *cardman, const char *err_desc);

static void gpa_card_manager_finalize (GObject *object);



/************************************************************
 *******************   Implementation   *********************
 ************************************************************/

/* Status bar handling.  */

static GtkWidget *
statusbar_new (GpaCardManager *cardman)
{
  GtkWidget *hbox;
  GtkWidget *label;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  label = gtk_label_new (NULL);
  cardman->status_label = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new ("");
  cardman->status_text = label;
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

  gtk_widget_set_halign (GTK_WIDGET (hbox), 0);
  gtk_widget_set_valign (GTK_WIDGET (hbox), 1);

  return hbox;
}


static void
statusbar_update (GpaCardManager *cardman, const char *text)
{
  gtk_label_set_text (GTK_LABEL (cardman->status_text), text);
}


/* Signal handler for GpaCMObject's "update-status".  */
static void
statusbar_update_cb (GpaCardManager *cardman, gchar *status)
{
  gtk_label_set_text (GTK_LABEL (cardman->status_text), status);
}

/* Signal handler for GpaCMObject's "alert-dialog".  */
static void
alert_dialog_cb (GpaCardManager *cardman, gchar *msg)
{
  gpa_window_error (msg, GTK_WIDGET (cardman));
}


static void
update_title (GpaCardManager *cardman)
{
  const char *title = _("Card Manager");

  if (cardman->cardtype == G_TYPE_NONE)
    gpa_window_set_title (GTK_WINDOW (cardman), title);
  else
    {
      char *tmp;

      tmp = g_strdup_printf ("%s (%s)", title, cardman->cardtypename);
      gpa_window_set_title (GTK_WINDOW (cardman), tmp);
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
            gpgme_data_t *r_data)
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
      if (!g_ascii_strcasecmp (args, "openpgp"))
        {
          cardman->cardtype = GPA_CM_OPENPGP_TYPE;
          cardman->cardtypename = "OpenPGP";
        }
      else if (!g_ascii_strcasecmp (args, "piv"))
        {
          cardman->cardtype = GPA_CM_PIV_TYPE;
          cardman->cardtypename = "PIV";
        }
      else if (!g_ascii_strcasecmp (args, "nks"))
        {
          cardman->cardtype = GPA_CM_NETKEY_TYPE;
          cardman->cardtypename = "NetKey";
        }
      else if (!g_ascii_strcasecmp (args, "dinsig"))
        {
          cardman->cardtype = GPA_CM_DINSIG_TYPE;
          cardman->cardtypename = "DINSIG";
        }
      else if (!g_ascii_strcasecmp (args, "P15"))
        cardman->cardtypename = "PKCS#15";
      else if (!g_ascii_strcasecmp (args, "geldkarte"))
        {
          cardman->cardtype = GPA_CM_GELDKARTE_TYPE;
          cardman->cardtypename = "Geldkarte";
        }
      else if (!g_ascii_strcasecmp (args, "undefined"))
        {
          cardman->cardtype = GPA_CM_UNKNOWN_TYPE;
          cardman->cardtypename = "UNKNOWN";
        }
      else
        cardman->cardtypename = "Unknown";

    }
  else if ( !strcmp (status, "EVENTCOUNTER") )
    {
      unsigned int count;

      if (sscanf (args, "%*u %*u %u ", &count) == 1)
        {
          cardman->eventcounter.card_any = 1;
          cardman->eventcounter.card = count;
        }
    }

  return 0;
}


/* Idle queue callback to mark a relaod operation finished.  */
static gboolean
card_reload_finish_idle_cb (void *user_data)
{
  GpaCardManager *cardman = user_data;

  cardman->in_card_reload--;
  g_object_unref (cardman);

  return FALSE;  /* Remove us from the idle queue. */
}


/* This function is called to trigger a card-reload.  */
static void
card_reload (GpaCardManager *cardman)
{
  gpg_error_t err, operr;
  const char *application;
  char *command_buf = NULL;
  const char *command;
  const char *err_desc = NULL;
  char *err_desc_buffer = NULL;
  int auto_app;

  if (!cardman->gpgagent)
    return;  /* No support for GPGME_PROTOCOL_ASSUAN.  */

  /* Start the ticker if not yet done.  */
  start_ticker (cardman);
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
      if (cardman->app_selector
          && (gtk_combo_box_get_active
              (GTK_COMBO_BOX (cardman->app_selector)) > 0)
          && (application = gtk_combo_box_text_get_active_text
              (GTK_COMBO_BOX_TEXT (cardman->app_selector))))
        {
          command_buf = g_strdup_printf ("%s %s", command, application);
          command = command_buf;
          auto_app = 0;
        }
      else
        auto_app = 1;
      err = gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                          command,
                                          scd_data_cb, NULL,
                                          scd_inq_cb, NULL,
                                          scd_status_cb, cardman, &operr);
      if (!err)
        {
          err = operr;
          if (!auto_app
              && gpg_err_source (err) == GPG_ERR_SOURCE_SCD
              && gpg_err_code (err) == GPG_ERR_CONFLICT)
            {
              /* Not in auto select mode and the scdaemon told us
                 about a conflicting use.  We now do a restart and try
                 again to display an application selection conflict
                 error only if it is not due to our own connection to
                 the scdaemon.  */
              if (!gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                                 "SCD RESTART",
                                                 NULL, NULL, NULL, NULL,
                                                 NULL, NULL, &operr)
                  && !operr)
                {
                  err = gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                                      command,
                                                      scd_data_cb, NULL,
                                                      scd_inq_cb, NULL,
                                                      scd_status_cb, cardman,
                                                      &operr);
                  if (!err)
                    err = operr;
                }
            }
        }


      if (gpg_err_code (err) == GPG_ERR_CARD_NOT_PRESENT
          || gpg_err_code (err) == GPG_ERR_CARD_REMOVED)
        {
          err_desc = _("No card found.");
        }
      else if (gpg_err_source (err) == GPG_ERR_SOURCE_SCD
               && gpg_err_code (err) == GPG_ERR_CONFLICT)
        {
          err_desc = auto_app
            ? _("The selected card application is currently not available.")
            : _("Another process is using a different card application "
                "than the selected one.\n\n"
                "You may change the application selection mode to "
                "\"Auto\" to select the active application.");
        }
      else if (!auto_app
               && gpg_err_source (err) == GPG_ERR_SOURCE_SCD
               && gpg_err_code (err) == GPG_ERR_NOT_SUPPORTED)
        {
          err_desc =
            _("The selected card application is not available.");
        }
      else if (err)
        {
          g_debug ("assuan command `%s' failed: %s <%s>\n",
                   command, gpg_strerror (err), gpg_strsource (err));
          if (!gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                             "SCD SERIALNO undefined",
                                             NULL, NULL, NULL, NULL,
                                             NULL, NULL, &operr) && !operr)
            err = 0;
          else
            {
              err_desc = _("Error accessing the card.");
              statusbar_update (cardman, _("Error accessing card"));
            }
        }
      g_free (command_buf);


      if (!err)
        {
          /* Get the event counter to avoid a duplicate reload due to
             the ticker.  */
          gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                        "GETEVENTCOUNTER",
                                        NULL, NULL,
                                        NULL, NULL,
                                        scd_status_cb, cardman, NULL);

          /* Now we need to get the APPTYPE of the card so that the
             correct GpaCM* object can can act on the data.  */
          command = "SCD GETATTR APPTYPE";
          err = gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                              command,
                                              scd_data_cb, NULL,
                                              scd_inq_cb, NULL,
                                              scd_status_cb, cardman, &operr);
          if (!err)
            err = operr;

          if (gpg_err_code (err) == GPG_ERR_CARD_NOT_PRESENT
              || gpg_err_code (err) == GPG_ERR_CARD_REMOVED)
            statusbar_update (cardman, _("No card"));
          else if (err)
            {
              g_debug ("assuan command `%s' failed: %s <%s>\n",
                       command, gpg_strerror (err), gpg_strsource (err));
              statusbar_update (cardman, _("Error accessing card"));
            }
        }


      update_card_widget (cardman, err_desc);
      g_free (err_desc_buffer);
      err_desc_buffer = NULL;
      err_desc = NULL;
      update_title (cardman);

      update_info_visibility (cardman);
      /* We decrement our lock using a idle handler with lo priority.
         This gives us a better chance not to do a reload a second
         time on behalf of the file watcher or ticker.  */
      g_object_ref (cardman);
      g_idle_add_full (G_PRIORITY_LOW,
                       card_reload_finish_idle_cb, cardman, NULL);
    }
}


/* This function is called when the user triggers a card-reload. */
static void
card_reload_action (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
  // GpaCardManager *cardman = param;
  GpaCardManager *cardman = (GpaCardManager*)user_data;


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


static gpg_error_t
geteventcounter_status_cb (void *opaque, const char *status, const char *args)
{
  GpaCardManager *cardman = opaque;

  if ( !strcmp (status, "EVENTCOUNTER") )
    {
      unsigned int count;

      /* We don't check while we are already reloading a card.  The
         last action of the card reload code will also update the
         event counter.  */
      if (!cardman->in_card_reload
          && sscanf (args, "%*u %*u %u ", &count) == 1)
        {
          if (cardman->eventcounter.card_any
              && cardman->eventcounter.card != count)
            {
              /* Actually we should not do a reload based only on the
                 eventcounter but check the actual card status first.
                 However simply triggering a reload is not different
                 from the user hitting the reload button.  */
              g_object_ref (cardman);
              g_idle_add (card_reload_idle_cb, cardman);
            }
          cardman->eventcounter.card_any = 1;
          cardman->eventcounter.card = count;
        }
    }

  return 0;
}

/* This function is called by the timeout ticker started by
   start_ticker.  It is used to poll scdaemon to detect a card status
   change.  */
static gboolean
ticker_cb (gpointer user_data)
{
  GpaCardManager *cardman = user_data;

  if (!cardman || !cardman->ticker_timeout_id || !cardman->gpgagent
      || cardman->in_card_reload)
    return TRUE;  /* Keep on ticking.  */

  /* Note that we are single threaded and thus there is no need to
     lock the assuan context.  */

  gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                "GETEVENTCOUNTER",
                                NULL, NULL,
                                NULL, NULL,
                                geteventcounter_status_cb, cardman, NULL);

  return TRUE;  /* Keep on ticking.  */
}


/* If no ticker is active start one.  */
static void
start_ticker (GpaCardManager *cardman)
{
  if (disable_ticker)
    return;

  if (!cardman->ticker_timeout_id)
    {
#if GTK_CHECK_VERSION (2, 14, 0)
      cardman->ticker_timeout_id = g_timeout_add_seconds (1,
                                                          ticker_cb, cardman);
#else
      cardman->ticker_timeout_id = g_timeout_add (1000, ticker_cb, cardman);
#endif
    }
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
  gpg_error_t err, operr;
  GpaGenKeyCardOperation *op;
  char *keyattr;

  if (cardman->cardtype != GPA_CM_OPENPGP_TYPE)
    return;  /* Not possible.  */
  if (!cardman->gpgagent)
    {
      g_debug ("Ooops: no assuan context");
      return;
    }

  /* Note: This test works only with GnuPG > 2.0.10 but that version
     is anyway required for the card manager to work correctly.  */
  err = gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                      "SCD GETINFO deny_admin",
                                      NULL, NULL, NULL, NULL, NULL, NULL,
                                      &operr);
  if (!err)
    err = operr;

  if (!err)
    {
      gpa_window_error ("Admin commands are disabled in scdamon.\n"
                        "Key generation is not possible.", NULL);
      return;
    }

  keyattr = (cardman->card_widget
             ? gpa_cm_openpgp_get_key_attributes (cardman->card_widget)
             : NULL);

  op = gpa_gen_key_card_operation_new (GTK_WIDGET (cardman), keyattr);
  g_signal_connect_swapped (G_OBJECT (op), "completed",
                            G_CALLBACK (card_genkey_completed), cardman);
  g_signal_connect (G_OBJECT (op), "completed",
		    G_CALLBACK (g_object_unref), NULL);
}


/* This function is called when the user triggers a key-generation.  */
static void
card_genkey_action (GSimpleAction *simple, GVariant *parameter, gpointer user_data)
{
  // GpaCardManager *cardman = param;
  GpaCardManager *cardman = this_instance;

  card_genkey (cardman);
}


static void
watcher_cb (void *opaque, const char *filename, const char *reason)
{
  GpaCardManager *cardman = opaque;

  if (cardman && strchr (reason, 'w') && !cardman->in_card_reload)
    {
      card_reload (cardman);
    }
}


/* Handle menu item "File/Close".  */
static void
file_close (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  // GpaCardManager *cardman = this:ubsta;
  gtk_widget_destroy (GTK_WIDGET (this_instance));
}

/* Handle menu item "File/Close".  */
static void
file_quit (GSimpleAction *simple, GVariant *parameter, gpointer param)
{
  // GpaCardManager *cardman = this:ubsta;
  // gtk_widget_destroy (GTK_WIDGET (this_instance));

  g_application_quit (G_APPLICATION (get_gpa_application ()));
}


/* Construct the card manager menu and toolbar widgets and return
   them. */
static void
cardman_action_new (GpaCardManager *cardman, GtkWidget **menu_bar,
		    GtkWidget **toolbar)
{
  static const GActionEntry entries[] =
    {
      // Toplevel
      { "File", NULL },
      { "Edit", NULL },
      { "Card", NULL },

      // File menu
      { "file_close", file_close },
      { "file_quit", file_quit },

      { "card_reload", card_reload_action },
      { "card_genkey", card_genkey_action },
    };

  /*
  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GtkAction *action;
  GError *error;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, PACKAGE);
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

  // Fixup the icon theme labels which are too long for the toolbar.
  action = gtk_action_group_get_action (action_group, "WindowsKeyringEditor");
  g_object_set (action, "short_label", _("Keyring"), NULL);
  action = gtk_action_group_get_action (action_group, "WindowsFileManager");
  g_object_set (action, "short_label", _("Files"), NULL);
  */

  /*

    File
      Close
      Quit

    Edit
      Preferences
      Backend Preferences

    Card
      Refresh
      Generate Key

    Windows
      Keyring Manager
      File Manager
      Clipboard
      Card Manager

    Help
      About
  */

  static const char *menu_string = ""
  "<interface>"
  "<menu id='menu'>"
      "<submenu>"
        "<attribute name='label' translatable='yes'>_File</attribute>"
        "<item>"
          "<attribute name='label' translatable='yes'>Close</attribute>"
          "<attribute name='action'>app.file_close</attribute>"
        "</item>"
        "<item>"
          "<attribute name='label' translatable='yes'>Quit</attribute>"
          "<attribute name='action'>app.file_quit</attribute>"
        "</item>"
      "</submenu>"
      "<submenu>"
        "<attribute name='label' translatable='yes'>Edit</attribute>"
        "<item>"
          "<attribute name='label' translatable='yes'>Preferences</attribute>"
          "<attribute name='action'>app.edit_preferences</attribute>"
        "</item>"
        "<item>"
          "<attribute name='label' translatable='yes'>Backend Preferences</attribute>"
          "<attribute name='action'>app.edit_backend_preferences</attribute>"
        "</item>"
      "</submenu>"
      "<submenu>"
        "<attribute name='label' translatable='yes'>Card</attribute>"
        "<item>"
          "<attribute name='label' translatable='yes'>Refresh</attribute>"
          "<attribute name='action'>app.card_reload</attribute>"
        "</item>"
        "<item>"
          "<attribute name='label' translatable='yes'>Generate Key</attribute>"
          "<attribute name='action'>app.card_genkey</attribute>"
        "</item>"
      "</submenu>"
      "<submenu>"
        "<attribute name='label' translatable='yes'>Windows</attribute>"
        "<item>"
          "<attribute name='label' translatable='yes'>Keyring Manager</attribute>"
          "<attribute name='action'>app.windows_keyring_editor</attribute>"
        "</item>"
        "<item>"
          "<attribute name='label' translatable='yes'>File Manager</attribute>"
          "<attribute name='action'>app.windows_file_manager</attribute>"
        "</item>"
        "<item>"
          "<attribute name='label' translatable='yes'>Clipboard</attribute>"
          "<attribute name='action'>app.windows_clipboard</attribute>"
        "</item>"
        "<item>"
          "<attribute name='label' translatable='yes'>Card Manager</attribute>"
          "<attribute name='action'>app.windows_card_manager</attribute>"
        "</item>"
      "</submenu>"
      "<submenu>"
        "<attribute name='label' translatable='yes'>Help</attribute>"
        "<item>"
          "<attribute name='label' translatable='yes'>About</attribute>"
          "<attribute name='action'>app.help_about</attribute>"
        "</item>"
      "</submenu>"
  "</menu>"

  "<object id='toolbar' class='GtkToolbar'>"
    "<property name='visible'>True</property>"
    "<property name='can_focus'>False</property>"
    "<property name='show_arrow'>False</property>"

    "<child>"
      "<object class='GtkToolButton'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='use_underline'>True</property>"
        "<property name='icon_name'>view-refresh</property>"
        "<property name='has_tooltip'>true</property>"
        "<property name='tooltip-text'>Reload card information</property>"
      "</object>"
      "<packing>"
        "<property name='expand'>False</property>"
        "<property name='homogeneous'>True</property>"
      "</packing>"
    "</child>"

    "<child>"
      "<object class='GtkSeparatorToolItem'>"
      "</object>"
    "</child>"

    "<child>"
      "<object class='GtkToolButton'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='use_underline'>True</property>"
        "<property name='icon_name'>preferences-desktop</property>"
        "<property name='has_tooltip'>true</property>"
        "<property name='tooltip-text'>Configure the application</property>"
      "</object>"
      "<packing>"
        "<property name='expand'>False</property>"
        "<property name='homogeneous'>True</property>"
      "</packing>"
    "</child>"

    "<child>"
      "<object class='GtkSeparatorToolItem'>"
      "</object>"
    "</child>"

    "<child>"
      "<object class='GtkToolButton'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='use_underline'>True</property>"
        "<property name='icon_name'>keyring</property>"
        "<property name='action-name'>app.windows_keyring_editor</property>"
        "<property name='has_tooltip'>true</property>"
        "<property name='tooltip-text'>Open the keyring editor</property>"
      "</object>"
      "<packing>"
        "<property name='expand'>False</property>"
        "<property name='homogeneous'>True</property>"
      "</packing>"
    "</child>"

    "<child>"
      "<object class='GtkToolButton'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='use_underline'>True</property>"
        "<property name='icon_name'>folder</property>"
        "<property name='action-name'>app.windows_file_manager</property>"
        "<property name='has_tooltip'>true</property>"
        "<property name='tooltip-text'>Open the file manager</property>"
      "</object>"
      "<packing>"
        "<property name='expand'>False</property>"
        "<property name='homogeneous'>True</property>"
      "</packing>"
    "</child>"

    "<child>"
      "<object class='GtkToolButton'>"
        "<property name='visible'>True</property>"
        "<property name='can_focus'>False</property>"
        "<property name='use_underline'>True</property>"
        "<property name='icon_name'>edit-paste</property>"
        "<property name='action-name'>app.windows_clipboard</property>"
        "<property name='has_tooltip'>true</property>"
        "<property name='tooltip-text'>Open the clipboard</property>"
      "</object>"
      "<packing>"
        "<property name='expand'>False</property>"
        "<property name='homogeneous'>True</property>"
      "</packing>"
    "</child>"

  "</object>"

  "</interface>";

  GtkBuilder *gtk_builder = gtk_builder_new_from_string (menu_string, -1);
  GMenuModel *menu_bar_model = G_MENU_MODEL (gtk_builder_get_object (GTK_BUILDER (gtk_builder), "menu"));
  *menu_bar = gtk_menu_bar_new_from_model (menu_bar_model);

  // GObject *grid = gtk_builder_get_object (GTK_BUILDER (gtk_builder), "toolbar");
  *toolbar = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (gtk_builder), "toolbar"));

  GtkApplication *gpa_app = get_gpa_application ();

  g_action_map_add_action_entries (G_ACTION_MAP (gpa_app),
                                    gpa_windows_menu_g_action_entries,
                                    G_N_ELEMENTS (gpa_windows_menu_g_action_entries),
                                    cardman);

  g_action_map_add_action_entries (G_ACTION_MAP (gpa_app),
                                    entries,
                                    G_N_ELEMENTS (entries),
                                    cardman);

  g_action_map_add_action_entries (G_ACTION_MAP (gpa_app),
                                    gpa_help_menu_g_action_entries,
                                    G_N_ELEMENTS (gpa_help_menu_g_action_entries),
                                    cardman);

  g_action_map_add_action_entries (G_ACTION_MAP (gpa_app),
                                    gpa_preferences_menu_g_action_entries,
                                    G_N_ELEMENTS (gpa_preferences_menu_g_action_entries),
                                    cardman);


}


/* Callback for the destroy signal.  */
static void
card_manager_closed (GtkWidget *widget, gpointer param)
{
  this_instance = NULL;
}


static void
update_card_widget (GpaCardManager *cardman, const char *error_description)
{
  if (cardman->card_widget)
    {
      gtk_widget_destroy (cardman->card_widget);
      cardman->card_widget = NULL;
    }

  if (cardman->cardtype == GPA_CM_OPENPGP_TYPE)
    {
      cardman->card_widget = gpa_cm_openpgp_new ();
    }
  else if (cardman->cardtype == GPA_CM_PIV_TYPE)
    {
      cardman->card_widget = gpa_cm_piv_new ();
    }
  else if (cardman->cardtype == GPA_CM_GELDKARTE_TYPE)
    {
      cardman->card_widget = gpa_cm_geldkarte_new ();
    }
  else if (cardman->cardtype == GPA_CM_NETKEY_TYPE)
    {
      cardman->card_widget = gpa_cm_netkey_new ();
    }
  else if (cardman->cardtype == GPA_CM_DINSIG_TYPE)
    {
      cardman->card_widget = gpa_cm_dinsig_new ();
    }
  else if (cardman->cardtype == GPA_CM_UNKNOWN_TYPE)
    {
      cardman->card_widget = gpa_cm_unknown_new ();
    }
  else
    {
      if (!error_description)
        error_description = _("This card application is not yet supported.");
      cardman->card_widget = gtk_label_new (error_description);
    }

  /* The following is basically what's in GTKs own
     gtk_scrolled_window_add_with_viewport:

    https://salsa.debian.org/gnome-team/gtk3/-/blob/debian/master/gtk/gtkscrolledwindow.c#L4151
  */

  GtkWidget *viewport;
  GtkBin *bin;
  GtkWidget *child_widget;

  GtkScrolledWindow *scrolled_window = GTK_SCROLLED_WINDOW (cardman->card_container);

  bin = GTK_BIN (scrolled_window);

  child_widget = gtk_bin_get_child (bin);

  if (child_widget)
  {
    g_return_if_fail (GTK_IS_VIEWPORT (child_widget));
    g_return_if_fail (gtk_bin_get_child (GTK_BIN (child_widget)) == NULL);

    viewport = child_widget;
  }
  else
  {
    viewport = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (scrolled_window),
                               gtk_scrolled_window_get_vadjustment (scrolled_window));

    gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  }

  gtk_widget_show (viewport);

  gtk_container_add (GTK_CONTAINER (viewport), cardman->card_widget);

  /* We need to do the reload after a show_all so that a card
     application may hide parts of its window.  */
  if (GPA_IS_CM_OBJECT (cardman->card_widget))
    {
      g_signal_connect_swapped (G_OBJECT (cardman->card_widget),
                                "update-status",
                                G_CALLBACK (statusbar_update_cb), cardman);
      g_signal_connect_swapped (G_OBJECT (cardman->card_widget),
				"alert-dialog",
				G_CALLBACK (alert_dialog_cb), cardman);

      /* Fixme: We should use a signal to reload the card widget
         instead of using a class test in each reload fucntion.  */
      gpa_cm_openpgp_reload (cardman->card_widget, cardman->gpgagent);
      gpa_cm_geldkarte_reload (cardman->card_widget, cardman->gpgagent);
      gpa_cm_netkey_reload (cardman->card_widget, cardman->gpgagent);
      gpa_cm_piv_reload (cardman->card_widget, cardman->gpgagent);
      gpa_cm_dinsig_reload (cardman->card_widget, cardman->gpgagent);
      gpa_cm_unknown_reload (cardman->card_widget, cardman->gpgagent);
    }
}


/* Handler for the "changed" signal of the application selector.  */
static void
app_selector_changed_cb (GtkComboBox *cbox, void *opaque)
{
  GpaCardManager *cardman = opaque;

  g_object_ref (cardman);
  g_idle_add (card_reload_idle_cb, cardman);
}


/* Assuan data callback used by setup_app_selector.  */
static gpg_error_t
setup_app_selector_data_cb (void *opaque, const void *data, size_t datalen)
{
  membuf_t *mb = opaque;

  put_membuf (mb, data, datalen);

  return 0;
}


/* Fill the app_selection box with the available applications.  */
static void
setup_app_selector (GpaCardManager *cardman)
{
  gpg_error_t err, operr;
  membuf_t mb;
  char *string;
  char *p, *p0, *p1;

  if (!cardman->gpgagent || !cardman->app_selector)
    return;

  init_membuf (&mb, 256);

  err = gpgme_op_assuan_transact_ext (cardman->gpgagent,
                                      "SCD GETINFO app_list",
                                      setup_app_selector_data_cb, &mb,
                                      NULL, NULL, NULL, NULL, &operr);
  if (err || operr)
    {
      g_free (get_membuf (&mb, NULL));
      return;
    }
  /* Make sure the data is a string and get it. */
  put_membuf (&mb, "", 1);
  string = get_membuf (&mb, NULL);
  if (!string)
    return; /* Out of core.  */

  for (p=p0=string; *p; p++)
    {
      if (*p == '\n')
        {
          *p = 0;
          p1 = strchr (p0, ':');
          if (p1)
            *p1 = 0;
          gtk_combo_box_text_append
            (GTK_COMBO_BOX_TEXT (cardman->app_selector), NULL, p0);
          if (p[1])
            p0 = p+1;
        }
    }

  g_free (string);
}


static void
construct_widgets (GpaCardManager *cardman)
{
  GtkWidget *vbox;
  GtkWidget *hbox, *hbox1, *hbox2;
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
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  /* Get the menu and the toolbar.  */
  cardman_action_new (cardman, &menubar, &toolbar);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, TRUE, 0);

  /* Add a fancy label that tells us: This is the card manager.  */
  hbox1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  GtkBuilder *gtk_builder = gtk_builder_new_from_string (icons_string, -1);

  icon = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (gtk_builder), "smartcard"));

  gtk_box_pack_start (GTK_BOX (hbox1), icon, FALSE, TRUE, 0);

  label = gtk_label_new (NULL);
  markup = g_strdup_printf ("<span font_desc=\"16\">%s</span>",
                            _("Card Manager"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (hbox1), label, TRUE, TRUE, 10);
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
  gtk_widget_set_valign (GTK_WIDGET (label), GTK_ALIGN_CENTER);

  /* Add a application selection box.  */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  label = gtk_label_new (_("Application selection:"));
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 5);
  cardman->app_selector = gtk_combo_box_text_new ();
  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (cardman->app_selector), NULL,
                             _("Auto"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (cardman->app_selector), 0);
  gtk_box_pack_start (GTK_BOX (hbox2), cardman->app_selector, FALSE, TRUE, 0);

  /* Put Card Manager label and application selector into the same line.  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), hbox1, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);


  /* Create a container (a scolled window) which will take the actual
     card widget.  This container is required so that we can easily
     change to a differet card widget. */
  cardman->card_container = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy
    (GTK_SCROLLED_WINDOW (cardman->card_container),
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), cardman->card_container, TRUE, TRUE, 0);

  /* Update the container using the current card application.  */
  update_card_widget (cardman, NULL);

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


  /* We use the file watcher to speed up card change detection.  If it
     does not work (i.e. on non Linux based systems) the ticker takes
     care of it.  */
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

  setup_app_selector (cardman);
  if (cardman->app_selector)
    g_signal_connect (cardman->app_selector, "changed",
                      G_CALLBACK (app_selector_changed_cb), cardman);


}


static void
gpa_card_manager_finalize (GObject *object)
{
  GpaCardManager *cardman = GPA_CARD_MANAGER (object);

  if (cardman->gpgagent)
    {
      gpgme_release (cardman->gpgagent);
      cardman->gpgagent = NULL;
    }

  if (cardman->ticker_timeout_id)
    {
      g_source_remove (cardman->ticker_timeout_id);
      cardman->ticker_timeout_id = 0;
    }

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
